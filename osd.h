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

#ifndef _osd_h
#define _osd_h

#include "digital.h"
#include "spi.h"
#include "can.h"

#define OSD_V2D(v)			((uint8_t)(256.0f * v / 3.3f))
#define OSD_DAC_WHITE_LEVEL		OSD_V2D(2.4f)
#define OSD_DAC_BLACK_LEVEL		OSD_V2D(0.9f)
//#define OSD_DAC_WHITE_LEVEL		OSD_V2D(1.3f)
//#define OSD_DAC_BLACK_LEVEL		OSD_V2D(0.30f)
#define OSD_SPI_FREQ			12000000			// 12MHz
#define OSD_HORZ_PIXELS			576				// pixels
#define OSD_VERT_PIXELS			448				// pixels
#define OSD_HORZ_OFFSET			94				// pixels
#define OSD_VERT_OFFSET			22				// lines
#define OSD_OE				((GPIOC->IDR & GPIO_Pin_15) != 0)

enum osdTelemetryTypes {
    OSD_TELEM_STATUS = 1,
    OSD_TELEM_LAT_LON,
    OSD_TELEM_VELNE,
    OSD_TELEM_VELD,
    OSD_TELEM_ALT,
    OSD_TELEM_HOME,
    OSD_TELEM_RC_QUALITY,
    OSD_TELEM_GPS_HACC,
    OSD_TELEM_Q1_Q2,
    OSD_TELEM_Q3_Q4,
    OSD_TELEM_NUM
};

typedef struct {
    uint8_t state;      // supervisor state
    uint8_t mode;       // nav mode
    double lat, lon;
    float velNED[3];
    float alt;
    float homeDist;
    float homeBrng;
    float rcQuality;
    float gpsHacc;
    float q[4];
} osdTelemData_t;

typedef struct {
    osdTelemData_t data;
    canNode_t *node;
    digitalPin *vidSwitch;
    digitalPin *a0, *a1;
    digitalPin *cvbsHi;
    uint32_t vc;		// vertical sync count
    uint32_t hc;		// horizontal sync count
    uint32_t lc;		// line count
    uint32_t base;		// base address (odd / even)
    uint32_t timingErrors;
    spiClient_t *dac;
    volatile uint32_t spiFlag;
    float telemValues[OSD_TELEM_NUM];
    uint8_t shadow;
    uint8_t inLine;
    uint8_t armed;
    uint8_t dataValid;
    uint8_t lastSeqId;
} osdStruct_t;

extern osdStruct_t osdData;

extern uint32_t osdPixBufWhite[OSD_HORZ_PIXELS/32*OSD_VERT_PIXELS];
extern uint32_t osdPixBufBlack[OSD_HORZ_PIXELS/32*OSD_VERT_PIXELS];

extern void osdInit(void);
extern void osdDMAClear(uint8_t color);
extern void osdDriverInit(void);

#endif
