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

#include "switch.h"

switchStruct_t switchData;

static inline int switchCmd(canNode_t *node, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    switch (doc) {
	case CAN_CMD_OFF:
            digitalLo(switchData.pins[node->id - 1]);
            ret = 1;
	    break;

	case CAN_CMD_ON:
            digitalHi(switchData.pins[node->id - 1]);
            ret = 1;
	    break;

	default:
	    break;
    }

    return ret;
}

int switchCanCallback(void *node, uint32_t fid, uint32_t doc, uint32_t *data, uint8_t n) {
    int ret = -1;

    node = (canNode_t *)node;

    switch (fid) {
	case CAN_FID_CMD:
	    ret = switchCmd(node, doc, data, n);
	    break;

	case CAN_FID_SET:
	case CAN_FID_GET:
	default:
	    break;
    }


    return ret;
}

void switchRegister(digitalPin *pin, char *name) {
    uint8_t n = switchData.num;

    // register with the CAN controller
    switchData.nodes[n] = canRegister(CAN_TYPE_SWITCH, n+1, switchCanCallback);

    switchData.pins[n] = pin;
    switchData.names[n] = name;

    switchData.num++;
}