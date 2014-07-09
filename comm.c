#include "comm.h"
#include <string.h>

commStruct_t commData[COMM_NUM_PORTS];

const char *commParameterStrings[] = {
    "BAUD_RATE"
};

static inline void commWrite(commStruct_t *comm, uint8_t *data, uint8_t n) {
    int i;

    for (i = 0; i <  n; i++)
        serialWrite(comm->s, data[i]);
}

static inline void commArm(commStruct_t *comm) {
    comm->state = CAN_UART_STATE_ARMED;
}

static inline void commDisarm(commStruct_t *comm) {
    comm->state = CAN_UART_STATE_DISARMED;
}

static inline int commSetParam(commStruct_t *comm, uint32_t *data) {
    uint32_t paramId = data[0];
    float value = *(float *)(&data[1]);
    int ret = -1;

    switch (paramId) {
        case COMM_BAUD_RATE:
            comm->baud = (uint32_t)value;
            serialChangeBaud(comm->s, comm->baud);
            ret = 1;

	default:
	    break;
    }

    return ret;
}

static inline int commGetParam(commStruct_t *comm, uint32_t *data) {
    int ret = -1;

    switch (*(uint16_t *)data) {
        case COMM_BAUD_RATE:
            *(float *)(comm->node->pkt->data) = (float)comm->baud;
            canReply(comm->node, 4);
            ret = 0;
            break;

	default:
	    break;
    }
}

static inline int commCmd(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    commStruct_t *comm = &commData[node->id - 1];
    int ret = -1;

    switch (doc) {
	case CAN_CMD_ARM:
            commArm(comm);
            ret = 1;
            break;

	case CAN_CMD_DISARM:
            commDisarm(comm);
	    ret = 1;
	    break;

	case CAN_CMD_STREAM:
	    commWrite(comm, (uint8_t *)data, n);
	    ret = 0;
	    break;

	default:
	    break;
    }

    return ret;
}

static inline int16_t commGetParamIdByName(uint8_t *name) {
    int16_t paramId = -1;
    int i;

    for (i = 0; i < COMM_PARAM_NUM; i++)
        if (!strncasecmp(commParameterStrings[i], name, 16))
            return i;

    return paramId;
}

static inline int commGet(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    commStruct_t *comm = &commData[node->id - 1];
    int16_t paramId;
    int ret = -1;

    switch (doc) {
	case CAN_DATA_STATE:
	    ((uint8_t *)data)[0] = comm->state;
	    canReply(node, 1);
	    ret = 0;
	    break;

        case CAN_DATA_PARAM_ID:
            ret = commGetParam(comm, data);
            break;

        case CAN_DATA_PARAM_NAME1:
            ((uint32_t *)comm->paramName)[0] = data[0];
            ((uint32_t *)comm->paramName)[1] = data[1];
            ret = 1;
            break;

        case CAN_DATA_PARAM_NAME2:
            ((uint32_t *)comm->paramName)[2] = data[0];
            ((uint32_t *)comm->paramName)[3] = data[1];

	    paramId = commGetParamIdByName(comm->paramName);

            if (paramId >= 0) {
                ((uint16_t *)data)[0] = paramId;
                canReply(node, 2);
                ret = 0;
            }
            else {
                ret = -1;
            }
            break;

	default:
	    break;
    }

    return ret;
}

static inline int commSet(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    commStruct_t *comm = &commData[node->id - 1];
    int ret = -1;

    switch (doc) {
        case CAN_DATA_PARAM_ID:
            ret = commSetParam(comm, data);
	    break;

	default:
	    break;
    }

    return ret;
}

int commCanCallback(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret;

    node = (canNode_t *)node;

    switch (fid) {
	case CAN_FID_SET:
	    ret = commSet(node, doc, data, n);
	    break;

	case CAN_FID_GET:
	    ret = commGet(node, doc, data, n);
	    break;

	case CAN_FID_CMD:
	    ret = commCmd(node, doc, data, n);
	    break;

        case CAN_FID_ACK:
        case CAN_FID_NACK:
            ret = 0;
            break;

	default:
	    ret = -1;
	    break;
    }

    return ret;
}

static void inline commCanSend(commStruct_t *comm, void *data, uint8_t n) {
    canSend(comm->node, CAN_LCC_INFO | CAN_TT_NODE | CAN_FID_CMD | (CAN_CMD_STREAM<<19), 0, n, data);
}

void commCheck(void) {
    uint8_t data[8];
    int n;
    int i;

    for (i = 0; i < COMM_NUM_PORTS; i++) {
        if (commData[i].s && commData[i].state == CAN_UART_STATE_ARMED) {
            n = 0;

            while (serialAvailable(commData[i].s)) {
                data[n++] = serialRead(commData[i].s);

                // send
                if (n == 8) {
                    commCanSend(&commData[i], data, n);
                    n = 0;
                }
            }

            // send remaining
            if (n > 0)
                commCanSend(&commData[i], data, n);
        }
    }
}

void commInit(void) {
    commData[0].baud = COMM_DEFAULT_BAUD_RATE;
    commData[1].baud = COMM_DEFAULT_BAUD_RATE;

    commData[0].s = serialOpen(USART1, commData[0].baud, USART_HardwareFlowControl_None, 256, 512);
    commData[1].s = serialOpen(USART2, commData[1].baud, USART_HardwareFlowControl_None, 256, 512);

    // register each with the CAN controller
    commData[0].node = canRegister(CAN_TYPE_UART, 1, commCanCallback);
    commData[1].node = canRegister(CAN_TYPE_UART, 2, commCanCallback);
}
