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

#include "sensors.h"
#include "analog.h"
#include "aq_timer.h"
#include "ut.h"
#include <math.h>

sensorsStruct_t sensorsData;

static inline int sensorsCanGet(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    switch (doc) {
	case CAN_DATA_VALUE:
            if (node->id < SENSORS_NUM) {
                ((float *)data)[0] = sensorsData.values[node->id];
                canReply(node, 4);
                ret = 0;
            }
	    break;

	default:
	    break;
    }

    return ret;
}

static inline void sensorsReset(void) {
    int i;

    for (i = 0; i < SENSORS_NUM; i++)
        sensorsData.rates[i] = 0;
}

static inline int sensorsCanCmd(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    switch (doc) {
	case CAN_CMD_ARM:
	case CAN_CMD_DISARM:
	case CAN_CMD_TELEM_VALUE:
	    ret = 1;
	    break;

	case CAN_CMD_TELEM_RATE:
            if (node->id < SENSORS_NUM) {
                    sensorsData.rates[node->id] = UT_TICK_FREQ / *((uint16_t *)data);
                    ret = 1;
            }
	    break;

	default:
	    break;
    }

    return ret;
}

int sensorsCanCallback(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    node = (canNode_t *)node;

    switch (fid) {
	case CAN_FID_RESET_BUS:
            sensorsReset();
            ret = 0;
            break;

	case CAN_FID_GET:
	    ret = sensorsCanGet(node, doc, data, n);
	    break;

	case CAN_FID_CMD:
	    ret = sensorsCanCmd(node, doc, data, n);
	    break;

        case CAN_FID_ACK:
        case CAN_FID_NACK:
            ret = 0;
            break;

	case CAN_FID_SET:
	default:
	    break;
    }

    return ret;
}

void sensorsTelem(uint32_t loop) {
    int i;

    for (i = 0; i < SENSORS_NUM; i++)
        if (sensorsData.rates[i] && !((loop+i) % sensorsData.rates[i]))
            canSend(sensorsData.canNodes[i], CAN_LCC_NORMAL | CAN_TT_NODE | CAN_FID_TELEM | (CAN_TELEM_VALUE<<19), 0, 4, &sensorsData.values[i]);
}

void sensorsUpdate(uint32_t loop) {
    analogDecode(&sensorsData.values[0]);

    sensorsData.mAh += sensorsData.values[SENSORS_AMP] * (1.0f / (60.0f * 60.0f)) * (utTick() - sensorsData.lastTick);
    sensorsData.lastTick = utTick();

    sensorsTelem(loop);
}

void sensorsInit(void) {
    analogInit();

    // register each with the CAN controller
    sensorsData.canNodes[CAN_SENSORS_PDB_BATV] = canRegister(CAN_TYPE_SENSOR, CAN_SENSORS_PDB_BATV, sensorsCanCallback);
    sensorsData.canNodes[CAN_SENSORS_PDB_BATA] = canRegister(CAN_TYPE_SENSOR, CAN_SENSORS_PDB_BATA, sensorsCanCallback);
    sensorsData.canNodes[CAN_SENSORS_PDB_TEMP] = canRegister(CAN_TYPE_SENSOR, CAN_SENSORS_PDB_TEMP, sensorsCanCallback);
}