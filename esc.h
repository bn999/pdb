#ifndef _esc_h
#define _esc_h

#include "can.h"
#include "pwm.h"
#include "board.h"

#define ESC_PWM_FREQ	    400		    // Hz
#define ESC_ID_OFFSET	    17              // start at ID 17

#define ESC_PWM_ARM	    975
#define ESC_PWM_MIN	    1000
#define ESC_PWM_START	    1125
#define ESC_PWM_MAX	    1950
#define ESC_SCALE	    ((1<<16) - 1)   // internal, unitless scale of motor output (0 -> 65535)
#define ESC_TIMEOUT	    200000	    // us (0.2s)

enum escStates {
    ESC_STATE_DISARMED = 0,
    ESC_STATE_STOPPED,
    ESC_STATE_NOCOMM,
    ESC_STATE_STARTING,
    ESC_STATE_RUNNING
};

typedef struct {
    uint32_t lastUpdate;
    pwmPortStruct_t *pwm;
    canNode_t *canNode;
    uint16_t value;
    uint8_t state;
} escStruct_t;

extern void escInit(void);
extern void escTimeoutCheck(void);

#endif