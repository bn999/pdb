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

#ifndef _analog_h
#define _analog_h

#include "main.h"

#define ANALOG_CHANNELS		    3
#define ANALOG_SAMPLES		    32
#define ANALOG_SAMPLE_TIME	    ADC_SampleTime_480Cycles
#define ANALOG_REF_VOLTAGE	    3.3f
#define ANALOG_DIVISOR		    ((double)ANALOG_REF_VOLTAGE / (double)4096.0 / (double)ANALOG_SAMPLES)

#define ANALOG_VIN_SLOPE	    ((ANALOG_VIN_RTOP + ANALOG_VIN_RBOT) / ANALOG_VIN_RBOT)
#define ANALOG_AMP_SLOPE	    (1.0f / (ANALOG_AMP_SENSITIVITY * ANALOG_REF_VOLTAGE / 5.0f))
#define ANALOG_AMP_NOLOAD           0.05f       // amps
#define ANALOG_AMP_OFFSET_ADDR      0x1FFF7800

#define ANALOG_TEMP_V25             0.76f
#define ANALOG_TEMP_AVG_SLOPE       (1.0f / 2.5f * 1000.0f)

#define ANALOG_VOLTS_VIN	    0
#define ANALOG_VOLTS_AMP	    1
#define ANALOG_VOLTS_TEMP           2

typedef struct {
    uint32_t rawChannels[ANALOG_CHANNELS];
    float voltages[ANALOG_CHANNELS];
    float ampOffset;
} analogStruct_t;

extern analogStruct_t analogData;

extern void analogInit(void);
extern void analogDecode(float *values);

#endif