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

#ifndef _sensosr_h
#define _sensors_h

#include "can.h"

#define SENSORS_VBAT            0
#define SENSORS_AMP             1
#define SENSORS_TEMP            2
#define SENSORS_NUM		3

typedef struct {
    uint32_t lastTick;
    float values[SENSORS_NUM];
    float mAh;
    canNode_t *canNodes[SENSORS_NUM];
    uint16_t rates[SENSORS_NUM];
} sensorsStruct_t;

extern sensorsStruct_t sensorsData;

extern void sensorsInit(void);
extern void sensorsUpdate(uint32_t loop);

#endif