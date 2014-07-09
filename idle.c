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

#include "idle.h"
#include "ut.h"
#include "graph.h"

volatile uint32_t idleCounter;
volatile uint32_t idleMinCycles = 0xffffffff;

volatile uint32_t *DWT_CYCCNT = (uint32_t *)0xE0001004;
volatile uint32_t *DWT_CONTROL = (uint32_t *)0xE0001000;
volatile uint32_t *SCB_DEMCR = (uint32_t *)0xE000EDFC;

void idleFunc(void *unused) {
    uint32_t thisCycles, lastCycles = 0;
    volatile uint32_t cycles;
    int i = 0;

    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CONTROL = *DWT_CONTROL | 1; // enable the counter

    while (1) {
        idleCounter++;

        thisCycles = *DWT_CYCCNT;
        cycles = thisCycles - lastCycles;
        lastCycles = thisCycles;

        // record shortest number of instructions for loop
        if (cycles < idleMinCycles)
            idleMinCycles = cycles;
    }
}