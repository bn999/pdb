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

#ifndef _can_h
#define _can_h

#include "stm32f4xx_can.h"

#define CAN_CAN			CAN1
#define CAN_CLK			RCC_APB1Periph_CAN1
#define CAN_RX_PIN		GPIO_Pin_11
#define CAN_TX_PIN		GPIO_Pin_12
#define CAN_GPIO_PORT		GPIOA
#define CAN_AF_PORT		GPIO_AF_CAN1
#define CAN_RX_SOURCE		GPIO_PinSource11
#define CAN_TX_SOURCE		GPIO_PinSource12
#define CAN_RX0_IRQ		CAN1_RX0_IRQn
#define CAN_RX0_HANDLER		CAN1_RX0_IRQHandler
#define CAN_TX_IRQ		CAN1_TX_IRQn
#define CAN_TX_HANDLER		CAN1_TX_IRQHandler

#define CAN_UUID		0x1FFF7A10
#define CAN_INSTANCES		32
#define CAN_BUF_SIZE		64				    // depth of the application layer FIFO
#define CAN_STACK_SIZE		256

// Logical Communications Channel
// 2 bits [28:27]
#define CAN_LCC_MASK	    ((uint32_t)0x3<<30)
#define CAN_LCC_EXCEPTION   ((uint32_t)0x0<<30)
#define CAN_LCC_HIGH	    ((uint32_t)0x1<<30)
#define CAN_LCC_NORMAL	    ((uint32_t)0x2<<30)
#define CAN_LCC_INFO	    ((uint32_t)0x3<<30)

// Target Type
// 1 bit [26:26]
#define CAN_TT_MASK         ((uint32_t)0x1<<29)
#define CAN_TT_GROUP        ((uint32_t)0x0<<29)
#define CAN_TT_NODE         ((uint32_t)0x1<<29)

// Function ID
// 4 bits [25:22]
#define CAN_FID_MASK	    ((uint32_t)0xf<<25)
#define CAN_FID_RESET_BUS   ((uint32_t)0x0<<25)
#define CAN_FID_ACK         ((uint32_t)0x1<<25)
#define CAN_FID_NACK	    ((uint32_t)0x2<<25)
#define CAN_FID_CMD         ((uint32_t)0x3<<25)
#define CAN_FID_GET         ((uint32_t)0x4<<25)
#define CAN_FID_SET         ((uint32_t)0x5<<25)
#define CAN_FID_REPLY	    ((uint32_t)0x6<<25)
#define CAN_FID_REQ_ADDR    ((uint32_t)0x7<<25)
#define CAN_FID_GRANT_ADDR  ((uint32_t)0x8<<25)
#define CAN_FID_ERROR	    ((uint32_t)0x9<<25)
#define CAN_FID_PING	    ((uint32_t)0xa<<25)
#define CAN_FID_TELEM	    ((uint32_t)0xb<<25)

// Data Object Code
// 6 bits [21:16]
#define CAN_DOC_MASK	    ((uint32_t)0x3f<<19)

// Source ID
// 5 bits [15:11]
#define CAN_SID_MASK	    ((uint32_t)0x1f<<14)

// Target ID
// 5 bits [10:6]
#define CAN_TID_MASK	    ((uint32_t)0x1f<<9)

// Sequence ID
// 6 bits [5:0]
#define CAN_SEQ_MASK	    ((uint32_t)0x3f<<3)

// types
enum {
    CAN_TYPE_ESC = 1,
    CAN_TYPE_SERVO,
    CAN_TYPE_SENSOR,
    CAN_TYPE_SWITCH,
    CAN_TYPE_OSD,
    CAN_TYPE_UART,
    CAN_TYPE_HUB,
    CAN_TYPE_NUM
};

// commands
enum {
    CAN_CMD_DISARM = 1,
    CAN_CMD_ARM,
    CAN_CMD_START,
    CAN_CMD_STOP,
    CAN_CMD_SETPOINT10,
    CAN_CMD_SETPOINT12,
    CAN_CMD_SETPOINT16,
    CAN_CMD_RPM,
    CAN_CMD_CFG_READ,
    CAN_CMD_CFG_WRITE,
    CAN_CMD_CFG_DEFAULT,
    CAN_CMD_TELEM_RATE,
    CAN_CMD_TELEM_VALUE,
    CAN_CMD_BEEP,
    CAN_CMD_POS,
    CAN_CMD_USER_DEFINED,
    CAN_CMD_RESET,
    CAN_CMD_STREAM,
    CAN_CMD_ON,
    CAN_CMD_OFF
};

// data types
enum {
    CAN_DATA_GROUP = 1,
    CAN_DATA_TYPE,
    CAN_DATA_ID,
    CAN_DATA_INPUT_MODE,
    CAN_DATA_RUN_MODE,
    CAN_DATA_STATE,
    CAN_DATA_PARAM_ID,
    CAN_DATA_TELEM,
    CAN_DATA_VERSION,
    CAN_DATA_VALUE,
    CAN_DATA_PARAM_NAME1,
    CAN_DATA_PARAM_NAME2
};

// sensor types
enum {
    CAN_SENSORS_PDB_BATV = 0,
    CAN_SENSORS_PDB_BATA,
    CAN_SENSORS_PDB_TEMP,
    CAN_SENSORS_GIMBAL_ACCX,
    CAN_SENSORS_GIMBAL_ACCY,
    CAN_SENSORS_GIMBAL_ACCZ,
    CAN_SENSORS_GIMBAL_GYOX,
    CAN_SENSORS_GIMBAL_GYOY,
    CAN_SENSORS_GIMBAL_GYOZ,
    CAN_SENSORS_NUM
};

// telemetry values
enum {
    CAN_TELEM_NONE = 0,
    CAN_TELEM_STATUS,
    CAN_TELEM_STATE,
    CAN_TELEM_TEMP,
    CAN_TELEM_VIN,
    CAN_TELEM_AMPS,
    CAN_TELEM_RPM,
    CAN_TELEM_ERRORS,
    CAN_TELEM_VALUE,
    CAN_TELEM_NUM
};

typedef struct {
    uint32_t id;
    uint32_t *data;
    uint8_t sid;
    uint8_t tid;
    uint8_t seq;
    uint8_t doc;
    uint8_t n;
} canPacket_t;

typedef struct {
    unsigned int value1 : 10;
    unsigned int value2 : 10;
    unsigned int value3 : 10;
    unsigned int value4 : 10;
    unsigned int value5 : 10;
    unsigned int value6 : 10;
    unsigned int unused : 4;
} __attribute__((packed)) canGroup10_t;

typedef struct {
    unsigned int value1 : 12;
    unsigned int value2 : 12;
    unsigned int value3 : 12;
    unsigned int value4 : 12;
    unsigned int value5 : 12;
    unsigned int unused : 4;
} __attribute__((packed)) canGroup12_t;

typedef struct {
    uint16_t value1;
    uint16_t value2;
    uint16_t value3;
    uint16_t value4;
} __attribute__((packed)) canGroup16_t;

typedef int canCallback_t(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n);

typedef struct {
    canPacket_t *pkt;
    canCallback_t *callback;
    uint32_t uuid;
    uint32_t networkId;
    uint32_t groupId;
    uint32_t subGroupId;
    uint32_t seqId;
    uint8_t type;
    uint8_t id;
} canNode_t;

typedef struct {
    uint32_t TIR;
    uint32_t TDLR;
    uint32_t TDHR;
    uint8_t TDTR;
} canBuf_t;

typedef struct {
    canBuf_t rxMsgs[CAN_BUF_SIZE];
    canBuf_t txMsgs[CAN_BUF_SIZE];
    canNode_t nodes[CAN_INSTANCES];
    canNode_t *netIds[(CAN_TID_MASK>>9)+1];	// nodes cross referenced by networkId
    uint32_t validMicros;
    uint32_t mailboxFull;
    uint32_t packetsReceived;
    uint32_t packetsSent;
    uint32_t rxOverrun;
    volatile uint8_t rxHead, rxTail;
    volatile uint8_t txHead, txTail;
    uint8_t numNodes;
    uint8_t needNetIds;
} canDataStruct_t;

extern canDataStruct_t canData;

extern void canInit(void);
extern void canProcess(void);
extern void canSetConstants(void);
extern canNode_t *canRegister(uint8_t type, uint8_t id, canCallback_t *callback);
extern void canReply(canNode_t *node, uint8_t size);
extern uint8_t canSend(canNode_t *node, uint32_t id, uint8_t tid, uint8_t n, void *data);

#endif