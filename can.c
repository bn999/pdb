/*
    This file is part of AutoQuad.

    AutoQuad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AutoQuad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2014  Bill Nesbitt
*/

#include "main.h"
#include "can.h"
#include "aq_timer.h"
#include "sensors.h"
#include "comm.h"
#include "xxhash.h"
#include "ut.h"
#include <__cross_studio_io.h>

canDataStruct_t canData;

static inline uint32_t canGetSeqId(canNode_t *node) {
    uint32_t seqId;

    seqId = node->seqId;
    node->seqId = (node->seqId + 1) & 0x3f;

    return seqId;
}

static inline int8_t canGetFreeMailbox(void) {
    int8_t mailbox;

    if ((CAN_CAN->TSR&CAN_TSR_TME0) == CAN_TSR_TME0)
        mailbox = 0;
    else if ((CAN_CAN->TSR&CAN_TSR_TME1) == CAN_TSR_TME1)
        mailbox = 1;
    else if ((CAN_CAN->TSR&CAN_TSR_TME2) == CAN_TSR_TME2)
        mailbox = 2;
    else
        mailbox = -1;

    return mailbox;
}

static void inline _canSend(canNode_t *node, uint32_t id, uint8_t tid, uint8_t seqId, uint8_t n, void *data) {
    uint32_t *d = data;
    canBuf_t *txPtr;

    txPtr = &canData.txMsgs[canData.txHead];

    txPtr->TIR = id | (node->networkId<<14) | ((tid & 0x1f)<<9) | (seqId<<3) | CAN_Id_Extended;

    n = n & 0xf;
    txPtr->TDTR = n;

    if (n) {
        txPtr->TDLR = *d++;
        txPtr->TDHR = *d;
    }

    canData.txHead = (canData.txHead + 1) % CAN_BUF_SIZE;

    // trigger transmit ISR
    NVIC->STIR = CAN_TX_IRQ;
}

uint8_t canSend(canNode_t *node, uint32_t id, uint8_t tid, uint8_t n, void *data) {
    uint8_t seqId;

    seqId = canGetSeqId(node);
    _canSend(node, id, tid, seqId, n, data);
}

static inline void canNack(canPacket_t *pkt) {
    canNode_t *node = canData.netIds[pkt->tid];

    _canSend(node, CAN_LCC_NORMAL | CAN_TT_NODE | CAN_FID_NACK, pkt->sid, pkt->seq, 0, 0);
}

static inline void canAck(canPacket_t *pkt) {
    canNode_t *node = canData.netIds[pkt->tid];

    _canSend(node, CAN_LCC_NORMAL | CAN_TT_NODE | CAN_FID_ACK, pkt->sid, pkt->seq, 0, 0);
}

void canReply(canNode_t *node, uint8_t size) {
    _canSend(node, CAN_LCC_NORMAL | CAN_TT_NODE | CAN_FID_REPLY | (node->pkt->doc<<19), node->pkt->sid, node->pkt->seq, size, node->pkt->data);
}

static inline void __canSendGetAddr(canNode_t *node) {
    uint8_t d[8];

    *((uint32_t *)&d[0]) = node->uuid;

    d[4] = node->type;
    d[5] = node->id;

    _canSend(node, CAN_LCC_NORMAL | CAN_TT_NODE | CAN_FID_REQ_ADDR, 0, canGetSeqId(node), 6, d);
}

static inline void _canSendGetAddr(void) {
    int i;

    for (i = 0; i < canData.numNodes; i++)
	if (canData.nodes[i].networkId == 0)
	    __canSendGetAddr(&canData.nodes[i]);
}

static inline void canSetupFilters(void) {
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    int f = 0;
    int group;
    int node;

    //  bus reset
    CAN_FilterInitStructure.CAN_FilterNumber = f++;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = CAN_FID_RESET_BUS>>16;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = CAN_FID_MASK>>16;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    // listen for CAN_FID_GRANT_ADDR
    CAN_FilterInitStructure.CAN_FilterNumber = f++;
    CAN_FilterInitStructure.CAN_FilterIdHigh = CAN_FID_GRANT_ADDR>>16;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = CAN_FID_MASK>>16;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInit(&CAN_FilterInitStructure);

    // add all nodes
    for (node = 0; node < canData.numNodes; node++) {
        CAN_FilterInitStructure.CAN_FilterNumber = f++;
        CAN_FilterInitStructure.CAN_FilterIdHigh = CAN_TT_NODE>>16;
        CAN_FilterInitStructure.CAN_FilterIdLow = canData.nodes[node].networkId<<9;
        CAN_FilterInitStructure.CAN_FilterMaskIdHigh = CAN_TT_MASK>>16;
        CAN_FilterInitStructure.CAN_FilterMaskIdLow = CAN_TID_MASK;
        CAN_FilterInit(&CAN_FilterInitStructure);
    }

    // add all groups
    for (group = 0; group < 8; group++) {
        // check each node
        for (node = 0; node < canData.numNodes; node++) {
            // if it belongs to group
            if (canData.nodes[node].groupId == group) {
                CAN_FilterInitStructure.CAN_FilterNumber = f++;
                CAN_FilterInitStructure.CAN_FilterIdHigh = CAN_TT_GROUP>>16;
                CAN_FilterInitStructure.CAN_FilterIdLow = group<<9;
                CAN_FilterInitStructure.CAN_FilterMaskIdHigh = CAN_TT_MASK>>16;
                CAN_FilterInitStructure.CAN_FilterMaskIdLow = CAN_TID_MASK;
                CAN_FilterInit(&CAN_FilterInitStructure);
                break;
            }
        }
    }
}

static inline void canProcessAddr(canPacket_t *pkt) {
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    canNode_t *node = 0;
    int i;

    // one of our UUIDs?
    for (i = 0; i < canData.numNodes; i++)
	if (canData.nodes[i].uuid == *((uint32_t *)&pkt->data[0]))
	    node = &canData.nodes[i];

    if (node) {
	if (node->networkId == 0)
	    canData.needNetIds--;

	node->networkId = pkt->tid;
	node->groupId = ((uint8_t *)pkt->data)[4];
	node->subGroupId = ((uint8_t *)pkt->data)[5];

	canData.netIds[node->networkId] = node;

        canSetupFilters();
    }
}

static void inline canBusReset(void) {
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    int i;

    // disarm all nodes
    for (i = 0; i < canData.numNodes; i++) {
	canNode_t *node = &canData.nodes[i];

	if (node) {
	    node->callback(node, (uint8_t)CAN_FID_CMD, CAN_CMD_DISARM, 0, 0);
	    node->callback(node, (uint8_t)CAN_FID_RESET_BUS, 0, 0, 0);

	    node->networkId = 0;
	    node->groupId = 0;
	    node->subGroupId = 0;
	}
    }

    canData.needNetIds = canData.numNodes;

    canSetupFilters();

    // ask for new address
    _canSendGetAddr();
}

static inline int canProcessSet(canPacket_t *pkt) {
    canNode_t *node = canData.netIds[pkt->tid];
    int ret = 0;

    if (node) {
        switch (pkt->doc) {
        case CAN_DATA_GROUP:
            node->groupId = ((uint8_t *)pkt->data)[0];
            node->subGroupId = ((uint8_t *)pkt->data)[1];
            canSetupFilters();
            ret = 1;
            break;

        default:
            ret = node->callback(node, CAN_FID_SET, pkt->doc, pkt->data, pkt->n);
            break;
        }
    }

    return ret;
}

static inline int canProcessGet(canPacket_t *pkt) {
    canNode_t *node = canData.netIds[pkt->tid];
    uint8_t *p1, *p2;
    int ret = 0;

    if (node) {
	node->pkt = pkt;

	switch (pkt->doc) {
            case CAN_DATA_GROUP:
                ((uint8_t *)pkt->data)[0] = node->groupId;
                ((uint8_t *)pkt->data)[1] = node->subGroupId;
                canReply(node, 2);
                break;

            case CAN_DATA_TYPE:
                ((uint8_t *)pkt->data)[0] = node->type;
                canReply(node, 1);
                break;

            case CAN_DATA_ID:
                ((uint8_t *)pkt->data)[0] = node->id;
                canReply(node, 1);
                break;

            // TODO: move to esc module
            case CAN_DATA_VERSION:
                p1 = (uint8_t *)pkt->data;
                p2 = (uint8_t *)VERSION;

                while (*p2)
                    *p1++ = *p2++;

                *p1 = 0;

                canReply(node, 8);
                break;

            default:
                ret = node->callback(node, CAN_FID_GET, pkt->doc, pkt->data, pkt->n);
                break;
            }
    }

    return ret;
}

static inline void canProcessSetpoint10(canPacket_t *pkt) {
    canGroup10_t *gPkt10 = (canGroup10_t *)pkt->data;
    int i;

    for (i = 0; i < canData.numNodes; i++) {
	canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

	if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
	    // group
	    if (node->groupId == pkt->tid) {
		int32_t val = -1;

		switch (node->subGroupId) {
		    case 1:
			val = gPkt10->value1;
			break;
		    case 2:
			val = gPkt10->value2;
			break;
		    case 3:
			val = gPkt10->value3;
			break;
		    case 4:
			val = gPkt10->value4;
			break;
		    case 5:
			val = gPkt10->value5;
			break;
		    case 6:
			val = gPkt10->value6;
			break;
		}

                if (val > -1)
                    node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT10, &val, 4);
	    }
	    // node
	    else if (node->networkId == pkt->tid) {
		node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT10, pkt->data, 4);
	    }
	}
    }
}

static inline void canProcessSetpoint12(canPacket_t *pkt) {
    canGroup12_t *gPkt12 = (canGroup12_t *)pkt->data;
    int i;

    for (i = 0; i < canData.numNodes; i++) {
	canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

	if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
	    // group
	    if (node->groupId == pkt->tid) {
		int32_t val = -1;

		switch (node->subGroupId) {
		    case 1:
			val = gPkt12->value1;
			break;
		    case 2:
			val = gPkt12->value2;
			break;
		    case 3:
			val = gPkt12->value3;
			break;
		    case 4:
			val = gPkt12->value4;
			break;
		    case 5:
			val = gPkt12->value5;
			break;
		}

                if (val > -1)
                    node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT12, &val, 4);
	    }
	    // node
	    else if (node->networkId == pkt->tid) {
		node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT12, pkt->data, 4);
	    }
	}
    }
}

static inline void canProcessSetpoint16(canPacket_t *pkt) {
    canGroup16_t *gPkt16 = (canGroup16_t *)pkt->data;
    int i;

    for (i = 0; i < canData.numNodes; i++) {
	canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

	if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
	    // group
	    if (node->groupId == pkt->tid) {
		int32_t val = -1;

		switch (node->subGroupId) {
		    case 1:
			val = gPkt16->value1;
			break;
		    case 2:
			val = gPkt16->value2;
			break;
		    case 3:
			val = gPkt16->value3;
			break;
		    case 4:
			val = gPkt16->value4;
			break;
		}

                if (val > -1)
                    node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT16, &val, 4);
	    }
	    // node
	    else if (node->networkId == pkt->tid) {
		node->callback(node, CAN_FID_CMD, CAN_CMD_SETPOINT16,pkt->data, 4);
	    }
	}
    }
}

static inline void canProcessRpm(canPacket_t *pkt) {
    canGroup16_t *gPkt16 = (canGroup16_t *)pkt->data;
    int i;

    for (i = 0; i < canData.numNodes; i++) {
        canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

        if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
            // group
            if (node->groupId == pkt->tid) {
                int32_t val = -1;

                switch (node->subGroupId) {
                    case 1:
                        val = gPkt16->value1;
                        break;
                    case 2:
                        val = gPkt16->value2;
                        break;
                    case 3:
                        val = gPkt16->value3;
                        break;
                    case 4:
                        val = gPkt16->value4;
                        break;
                }

                if (val > -1)
                    node->callback(node, CAN_FID_CMD, CAN_CMD_RPM, &val, 4);
            }
            // node
            else if (node->networkId == pkt->tid) {
                node->callback(node, CAN_FID_CMD, CAN_CMD_RPM, pkt->data, 4);
            }
        }
    }
}

static inline void canProcessArm(canPacket_t *pkt) {
    int i;

    for (i = 0; i < canData.numNodes; i++) {
        canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

        if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
            if (node->groupId == pkt->tid)
                node->callback(node, CAN_FID_CMD, CAN_CMD_ARM, 0, 0);
        }
        // node
        else if (node->networkId == pkt->tid) {
            node->callback(node, CAN_FID_CMD, CAN_CMD_ARM, 0, 0);
        }
    }
}

static inline void canProcessDisarm(canPacket_t *pkt) {
    int i;

    for (i = 0; i < canData.numNodes; i++) {
        canNode_t *node = &canData.nodes[i];

        if (!node)
            continue;

        if ((pkt->id & CAN_TT_MASK) != CAN_TT_NODE) {
            if (node->groupId == pkt->tid)
            node->callback(node, CAN_FID_CMD, CAN_CMD_DISARM, 0, 0);
        }
        // node
        else if (node->networkId == pkt->tid) {
            node->callback(node, CAN_FID_CMD, CAN_CMD_DISARM, 0, 0);
        }
    }
}

static inline int canProcessCmd(canPacket_t *pkt) {
    int ret = 0;

    switch (pkt->doc) {
    // group commands
    case CAN_CMD_SETPOINT10:
        canProcessSetpoint10(pkt);
        break;

    case CAN_CMD_SETPOINT12:
        canProcessSetpoint12(pkt);
        break;

    case CAN_CMD_SETPOINT16:
        canProcessSetpoint16(pkt);
        break;

    case CAN_CMD_RPM:
        canProcessRpm(pkt);
        break;

    case CAN_CMD_ARM:
        canProcessArm(pkt);
        break;

    case CAN_CMD_DISARM:
        canProcessDisarm(pkt);
        break;

    // everything else
    default:
        if (canData.netIds[pkt->tid])
            ret = canData.netIds[pkt->tid]->callback(canData.netIds[pkt->tid], CAN_FID_CMD, pkt->doc, pkt->data, pkt->n);
        break;
    }

    return ret;
}

void canProcessMessage(canBuf_t *rx) {
    canPacket_t pkt;
    int ret = -1;

    pkt.id = rx->TIR;
    pkt.doc = (pkt.id & CAN_DOC_MASK)>>19;
    pkt.sid = (pkt.id & CAN_SID_MASK)>>14;
    pkt.tid = (pkt.id & CAN_TID_MASK)>>9;
    pkt.seq = (pkt.id & CAN_SEQ_MASK)>>3;
    pkt.data = (uint32_t *)&rx->TDLR;
    pkt.n = rx->TDTR;
    canData.packetsReceived++;

//debug_printf("CAN: tid = %d, fid = %x, doc = %d, seq = %d\n", pkt.tid, pkt.id & CAN_FID_MASK, pkt.doc, pkt.seq);
    switch (pkt.id & CAN_FID_MASK) {
        case CAN_FID_RESET_BUS:
            canBusReset();
            break;

        case CAN_FID_GRANT_ADDR:
            if (canData.needNetIds)
                canProcessAddr(&pkt);
            break;

        case CAN_FID_CMD:
            ret = canProcessCmd(&pkt);
            break;

        case CAN_FID_SET:
            ret = canProcessSet(&pkt);
            break;

        case CAN_FID_GET:
            ret = canProcessGet(&pkt);
            break;

        case CAN_FID_PING:
            ret = 1;
            break;

        case CAN_FID_ACK:
            if (canData.netIds[pkt.tid])
                canData.netIds[pkt.tid]->callback(canData.netIds[pkt.tid], CAN_FID_ACK, pkt.seq, 0, 0);
            ret = 0;
            break;

        case CAN_FID_NACK:
            if (canData.netIds[pkt.tid])
                canData.netIds[pkt.tid]->callback(canData.netIds[pkt.tid], CAN_FID_NACK, pkt.seq, 0, 0);
            ret = 0;
            break;

        case CAN_FID_TELEM:
            if (canData.netIds[pkt.tid])
                ret = canData.netIds[pkt.tid]->callback(canData.netIds[pkt.tid], CAN_FID_TELEM, pkt.doc, pkt.data, pkt.n);
            break;

        default:
            break;
    }

    if (ret > 0)
        canAck(&pkt);
    else if (ret < 0)
        canNack(&pkt);
}

void canCode(void *p) {
    uint32_t loops = 0;
    canBuf_t *rx;

    while (1) {
        // keep trying to get addresses if needed
        if (canData.needNetIds && !(loops % (UT_TICK_FREQ / 100)))
            _canSendGetAddr();

        while (canData.rxTail != canData.rxHead) {
            rx = &canData.rxMsgs[canData.rxTail];

            canProcessMessage(rx);

            canData.rxTail = (canData.rxTail + 1) % CAN_BUF_SIZE;
        }

        sensorsUpdate(loops);
        commCheck();

        loops++;

        utYield(1);
    }
}

void canLowLevelInit(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_InitTypeDef CAN_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;

    // Connect CAN pins to AF
    GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_RX_SOURCE, CAN_AF_PORT);
    GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_TX_SOURCE, CAN_AF_PORT);

    // Configure CAN RX and TX pins
    GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN | CAN_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStructure);

    RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);

    // CAN register init
    CAN_DeInit(CAN_CAN);

    // CAN cell init
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = ENABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_2tq;

    // CAN Baudrate = ~1 Mbps (CAN clocked at 42 MHz)
    CAN_InitStructure.CAN_BS1 = CAN_BS1_5tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
    CAN_InitStructure.CAN_Prescaler = 3;
    CAN_Init(CAN_CAN, &CAN_InitStructure);

    // all filters for CAN1
    CAN_SlaveStartBank(27);

    // accept all to start with
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = CAN_RX0_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = CAN_TX_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable FIFO 0 message pending Interrupt
    CAN_ITConfig(CAN_CAN, CAN_IT_FMP0, ENABLE);

    // Enable TX FIFO empty Interrupt
    CAN_ITConfig(CAN_CAN, CAN_IT_TME, ENABLE);
}

void canInit(void) {
    canLowLevelInit();
    canBusReset();

    utCreateTask(canCode, 0, 50, CAN_STACK_SIZE);
}

canNode_t *canRegister(uint8_t type, uint8_t id, canCallback_t *callback) {
    canNode_t *node = 0;

    if (canData.numNodes < CAN_INSTANCES) {
        node = &canData.nodes[canData.numNodes++];

        node->callback = callback;
        node->uuid = XXH32((void *)CAN_UUID, 3*4, 0) + (type<<8) + id;
        node->type = type;
        node->id = id;

        canData.needNetIds++;
    }

    return node;
}

void CAN_RX0_HANDLER(void) {
    canBuf_t *rx = &canData.rxMsgs[canData.rxHead];

    rx->TIR = (CAN_CAN->sFIFOMailBox[CAN_FIFO0].RIR>>3)<<3;
    rx->TDLR = CAN_CAN->sFIFOMailBox[CAN_FIFO0].RDLR;
    rx->TDHR = CAN_CAN->sFIFOMailBox[CAN_FIFO0].RDHR;
    rx->TDTR = CAN_CAN->sFIFOMailBox[CAN_FIFO0].RDTR & (uint8_t)0x0F;

    // release FIFO
    CAN_CAN->RF0R |= CAN_RF0R_RFOM0;

    canData.rxHead = (canData.rxHead + 1) % CAN_BUF_SIZE;

    if (canData.rxHead == canData.rxTail)
        canData.rxOverrun++;
}

void CAN_TX_HANDLER(void) {
    int8_t mailbox;
    canBuf_t *txPtr;

    CAN_ClearITPendingBit(CAN_CAN, CAN_IT_TME);

    if (canData.txHead != canData.txTail && (mailbox = canGetFreeMailbox()) == 0) {
//    while (canData.txHead != canData.txTail && (mailbox = canGetFreeMailbox()) >= 0) {
        txPtr = &canData.txMsgs[canData.txTail];

        CAN_CAN->sTxMailBox[mailbox].TDTR = txPtr->TDTR;

        CAN_CAN->sTxMailBox[mailbox].TDLR = txPtr->TDLR;
        CAN_CAN->sTxMailBox[mailbox].TDHR = txPtr->TDHR;

        // go
        CAN_CAN->sTxMailBox[mailbox].TIR = txPtr->TIR | 0x1;

        canData.txTail = (canData.txTail + 1) % CAN_BUF_SIZE;
        canData.packetsSent++;
    }
}
