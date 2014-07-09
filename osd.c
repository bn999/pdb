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

#include "osd.h"
#include "util.h"

static inline void osdArm(void) {
    uint8_t data[2];
    int i;

    osdData.armed = 1;

    for (i = 1; i < OSD_TELEM_NUM; i++) {
        data[0] = i - 1;
        data[1] = i;

        osdData.lastSeqId = canSend(osdData.node, CAN_LCC_INFO | CAN_TT_NODE | CAN_FID_CMD | (CAN_CMD_TELEM_VALUE<<19), 0, 2, data);
    }

    *(uint16_t *)data = 30;     // 30 Hz

    osdData.lastSeqId = canSend(osdData.node, CAN_LCC_INFO | CAN_TT_NODE | CAN_FID_CMD | (CAN_CMD_TELEM_RATE<<19), 0, 2, data);
}

static inline void osdDisarm(void) {
    osdData.armed = 0;
}

static inline void osdTelemReceive(uint8_t doc, uint32_t *data, uint8_t n) {
    switch (doc) {
        case OSD_TELEM_STATUS:
            osdData.data.state = ((uint8_t *)data)[0];
            osdData.data.mode = ((uint8_t *)data)[1];
            break;

        case OSD_TELEM_LAT_LON:
            osdData.data.lat = (double)data[0] / (double)1e7;
            osdData.data.lon = (double)data[1] / (double)1e7;
            break;

        case OSD_TELEM_VELNE:
            osdData.data.velNED[0] = ((float *)data)[0];
            osdData.data.velNED[1] = ((float *)data)[1];
            break;

        case OSD_TELEM_VELD:
            osdData.data.velNED[2] = ((float *)data)[0];
            break;

        case OSD_TELEM_ALT:
            osdData.data.alt = ((float *)data)[0];
            break;

        case OSD_TELEM_HOME:
            osdData.data.homeDist = ((float *)data)[0];
            osdData.data.homeBrng = ((float *)data)[1] * RAD_TO_DEG;
            break;

        case OSD_TELEM_RC_QUALITY:
            osdData.data.rcQuality = ((float *)data)[0];
            break;

        case OSD_TELEM_GPS_HACC:
            osdData.data.gpsHacc = ((float *)data)[0];
            break;

        case OSD_TELEM_Q1_Q2:
            osdData.data.q[0] = ((float *)data)[0];
            osdData.data.q[1] = ((float *)data)[1];
            break;

        case OSD_TELEM_Q3_Q4:
            osdData.data.q[2] = ((float *)data)[0];
            osdData.data.q[3] = ((float *)data)[1];
            osdData.dataValid = 1;
            break;
    }
}

static inline int osdCmd(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    switch (doc) {
	case CAN_CMD_ARM:
            osdArm();
            ret = 1;
            break;

	case CAN_CMD_DISARM:
            osdDisarm();
	    ret = 1;
	    break;

	default:
	    break;
    }

    return ret;
}

int osdCanCallback(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    node = (canNode_t *)node;

    switch (fid) {
	case CAN_FID_CMD:
	    ret = osdCmd(node, doc, data, n);
	    break;

        case CAN_FID_TELEM:
            osdTelemReceive(doc, data, n);
            ret = 0;
            break;

        case CAN_FID_ACK:
        case CAN_FID_NACK:
            ret = 0;
            break;

	case CAN_FID_SET:
	case CAN_FID_GET:
	default:
	    break;
    }

    return ret;
}

void osdInit(void) {
    osdDriverInit();

    // register with the CAN controller
    osdData.node = canRegister(CAN_TYPE_OSD, 1, osdCanCallback);
}