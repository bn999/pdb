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

#include "util.h"

uint32_t *dmaHeap[UTIL_DMA_HEAP_SIZE] __attribute__((section(".bss2")));

utilStruct_t utilData;

void *callocDma(uint16_t count, uint16_t size) {
    uint32_t words;

    // round up to word size
    words = (count*size + sizeof(int)-1) / sizeof(int);

    if ((utilData.dmaHeapUsed + words) > UTIL_DMA_HEAP_SIZE) {
	return 0;
//	AQ_NOTICE("Out of data SRAM!\n");
    }
    else {
	utilData.dmaHeapUsed += words;
    }

    return (void *)(dmaHeap + utilData.dmaHeapUsed - words);
}

float compassNormalize(float heading) {
    while (heading < 0.0f)
	heading += 360.0f;
    while (heading >= 360.0f)
	heading -= 360.0f;

    return heading;
}

float compassDiff(float a, float b) {
    float diff;

    diff = a - b;

    if (diff > 180.0f)
        diff -= 360.0f;
    else if (diff < -180.0f)
        diff += 360.0f;

    return diff;
}

