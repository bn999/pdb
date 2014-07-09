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

#ifndef _render_h
#define _render_h

#include "ut.h"

#define RENDER_STACK_SIZE       128
#define RENDER_COLOR            1

#define NAV_STATUS_MANUAL	0x00					    // full manual control
#define NAV_STATUS_ALTHOLD	0x01					    // altitude hold only
#define NAV_STATUS_POSHOLD	0x02					    // altitude & position hold
#define NAV_STATUS_DVH		0x03					    // dynamic velocity hold cut through
#define NAV_STATUS_MISSION	0x04					    // autonomous mission

#define RENDER_POWER_FILTER     0.97f

enum FCSupervisorStates {
    STATE_INITIALIZING	= 0x00,
    STATE_CALIBRATION	= 0x01,
    STATE_DISARMED	= 0x02,
    STATE_ARMED		= 0x04,
    STATE_FLYING	= 0x08,
    STATE_RADIO_LOSS1	= 0x10,
    STATE_RADIO_LOSS2	= 0x20,
    STATE_LOW_BATTERY1	= 0x40,
    STATE_LOW_BATTERY2	= 0x80
};

typedef struct {
    uint32_t totalTicks, oldTicks;
    taskStruct_t *task;
    uint32_t loops;
    float scale3D;
    float scaleRadar;
    float yaw, pitch, roll;
    float yawCos, yawSin;
    float rollCos, rollSin;
    float brgCos, brgSin;
    float relBrgCos, relBrgSin;
    float vel;
    float scaledVelN, scaledVelE;
    float smoothedVolt, smoothedAmp;
    float groundAlt;
} renderStruct_t;

extern renderStruct_t renderData;

extern void renderInit(void);

#endif