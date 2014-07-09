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

#ifndef _util_h
#define _util_h

#include <stdint.h>

#ifndef M_PI
#define M_PI			3.14159265f
#define M_PI_2			(M_PI / 2.0f)
#endif

#define RAD_TO_DEG		(180.0f / M_PI)
#define DEG_TO_RAD		(M_PI / 180.0f)

#define UTIL_DMA_HEAP_SIZE	1024

#define constrain(v, lo, hi)    ((v < lo) ? lo : ((v > hi) ? hi : v))
#define __swap32(x)             (uint32_t)((x & 0xff) << 24 | (x & 0xff00) << 8 | (x & 0xff0000) >> 8 | (x & 0xff000000) >> 24)
#define PERIPH2BB(addr, bit)    ((uint32_t *)(PERIPH_BB_BASE + ((addr) - PERIPH_BASE) * 32 + ((bit) * 4)))

typedef struct {
    uint16_t dmaHeapUsed;
} utilStruct_t;

extern void *callocDma(uint16_t count, uint16_t size);
extern float compassNormalize(float heading);
extern float compassDiff(float a, float b);

#endif