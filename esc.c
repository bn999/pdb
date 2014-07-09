#include "esc.h"
#include "pwm.h"
#include "util.h"
#include "aq_timer.h"

escStruct_t escData[PWM_NUM_PORTS];

static inline void escDisarm(escStruct_t *esc) {
    *esc->pwm->ccr = 0;
    esc->state = ESC_STATE_DISARMED;
}

static inline void escArm(escStruct_t *esc) {
    *esc->pwm->ccr = ESC_PWM_ARM;
    esc->state = ESC_STATE_STOPPED;
}

static inline void escStart(escStruct_t *esc) {
    *esc->pwm->ccr = ESC_PWM_START;
    esc->state = ESC_STATE_RUNNING;
}

static inline void escStop(escStruct_t *esc) {
    escArm(esc);
}

static inline void escSetpoint(escStruct_t *esc, uint32_t value) {
    esc->value = value;

    if (esc->state > ESC_STATE_DISARMED)
	*esc->pwm->ccr = constrain(value * (ESC_PWM_MAX - ESC_PWM_MIN) / ESC_SCALE + ESC_PWM_START, ESC_PWM_START, ESC_PWM_MAX);
    else
	*esc->pwm->ccr = 0;

    esc->lastUpdate = timerMicros();
}

static inline int escGet(canNode_t *node, uint32_t doc, uint32_t *data) {
    escStruct_t *esc = &escData[node->id - ESC_ID_OFFSET];
    int ret = 0;

    switch (doc) {
	case CAN_DATA_STATE:
	    ((uint8_t *)data)[0] = esc->state;
	    canReply(node, 1);
	    ret = 0;
	    break;

	// cases not supported
	case CAN_DATA_TELEM:
	default:
	    ret = -1;
	    break;
    }

//    case CAN_DATA_INPUT_MODE:
//	((uint8_t *)pkt->data)[0] = inputMode;
//	canReply(pkt, 1);
//	break;
//
//    case CAN_DATA_RUN_MODE:
//	((uint8_t *)pkt->data)[0] = runMode;
//	canReply(pkt, 1);
//	break;
//
//    case CAN_DATA_PARAM:
//	if (*((uint16_t *)pkt->data) < CONFIG_NUM_PARAMS) {
//	    float val = p[*((uint16_t *)pkt->data)];
//
//	    *(float *)(pkt->data) = val;
//	    canReply(pkt, 4);
//	}
//	else {
//	    canNack(pkt);
//	}
//	break;
//

    return ret;
}

static inline int escSet(canNode_t *node, uint32_t doc, uint32_t *data) {
    int ret = 0;

    switch (doc) {
	// cases not supported
	case CAN_DATA_STATE:
	case CAN_DATA_TELEM:
	default:
	    ret = -1;
	    break;
    }
//    case CAN_DATA_INPUT_MODE:
//	if (*(uint8_t *)pkt->data < ESC_INPUT_MAX) {
//	    inputMode = *(uint8_t *)pkt->data;
//	    canAck(pkt);
//	}
//	else {
//	    canNack(pkt);
//	}
//	break;
//
//    case CAN_DATA_RUN_MODE:
//	if (*(uint8_t *)pkt->data < NUM_RUN_MODES) {
//	    runDisarm(REASON_CAN_USER);
//	    runMode = *(uint8_t *)pkt->data;
//	    canAck(pkt);
//	}
//	else {
//	    canNack(pkt);
//	}
//	break;
//
//    case CAN_DATA_PARAM:
//	canSetParam(pkt);
//	break;

    return ret;
}

static inline int escCmd(canNode_t *node, uint32_t doc, uint32_t *data) {
    escStruct_t *esc = &escData[node->id - ESC_ID_OFFSET];
    int ret = 0;

    switch (doc) {
	case CAN_CMD_SETPOINT10:
	    escSetpoint(esc, (*data)<<6);
	    break;

	case CAN_CMD_SETPOINT12:
	    escSetpoint(esc, (*data)<<4);
	    break;

	case CAN_CMD_SETPOINT16:
	    escSetpoint(esc, (*data));
	    break;

	case CAN_CMD_RPM:
	    ret = -1;
	    break;

	case CAN_CMD_ARM:
	    escArm(esc);
	    ret = 1;
	    break;

	case CAN_CMD_DISARM:
	    escDisarm(esc);
	    ret = 1;
	    break;

	case CAN_CMD_START:
	    escStart(esc);
	    ret = 1;
	    break;

	case CAN_CMD_STOP:
	    escStop(esc);
	    ret = 1;
	    break;

//    case CAN_CMD_CFG_READ:
//	if (state <= ESC_STATE_STOPPED) {
//	    configReadFlash();
//	    canAck(pkt);
//	}
//	else {
//	    canNack(pkt);
//	}
//	break;
//
//    case CAN_CMD_CFG_WRITE:
//	if (state <= ESC_STATE_STOPPED && configWriteFlash())
//	    canAck(pkt);
//	else
//	    canNack(pkt);
//	break;
//
//    case CAN_CMD_CFG_DEFAULT:
//	if (state <= ESC_STATE_STOPPED) {
//	    configLoadDefault();
//	    canAck(pkt);
//	}
//	else {
//	    canNack(pkt);
//	}
//	break;
//
//
	// cases not supported
	case CAN_CMD_TELEM_RATE:
	case CAN_CMD_TELEM_VALUE:
	case CAN_CMD_RESET:
	case CAN_CMD_BEEP:
	case CAN_CMD_POS:
	case CAN_CMD_USER_DEFINED:
	default:
	    ret = -1;
	    break;
    }

    return ret;
}

int escCanCallback(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = 0;

    node = (canNode_t *)node;

    switch (fid) {
	case CAN_FID_SET:
	    ret = escSet(node, doc, data);
	    break;

	case CAN_FID_GET:
	    ret = escGet(node, doc, data);
	    break;

	case CAN_FID_CMD:
	    ret = escCmd(node, doc, data);
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

void escInit(void) {
    int i;

    for (i = 0; i < PWM_NUM_PORTS; i++) {
        // setup hardware ports
        escData[i].pwm = pwmInitOut(i, PWM_PRESCALE/ESC_PWM_FREQ, 0);

        // register each with the CAN controller
        escData[i].canNode = canRegister(CAN_TYPE_ESC, ESC_ID_OFFSET+i, escCanCallback);
    }
}

void escTimeoutCheck(void) {
    int i;

    for (i = 0; i < PWM_NUM_PORTS; i++)
        if (escData[i].state > ESC_STATE_DISARMED && (timerMicros() - escData[i].lastUpdate) > ESC_TIMEOUT)
            escDisarm(&escData[i]);
}