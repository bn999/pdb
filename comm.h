#ifndef _comm_h
#define _comm_h

#include "serial.h"
#include "can.h"

#define COMM_NUM_PORTS              3
#define COMM_DEFAULT_BAUD_RATE      115200

enum commStates {
    CAN_UART_STATE_DISARMED = 0,
    CAN_UART_STATE_ARMED
};

enum {
    COMM_BAUD_RATE = 0,
    COMM_PARAM_NUM
};

typedef struct {
    serialPort_t *s;
    canNode_t *node;
    uint32_t baud;
    uint8_t paramName[16];
    uint8_t state;
} commStruct_t;

extern commStruct_t commData[COMM_NUM_PORTS];

extern void commInit(void);
extern void commCheck(void);

#endif