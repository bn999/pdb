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

#include "supervisor.h"
#include "idle.h"
#include "rcc.h"
#include "esc.h"
#include <stdio.h>

supervisorStruct_t supervisorData;

void supervisorCode(void *unused) {
    uint32_t lastCounter;
    uint32_t loop = 0;

    while (1) {
        // 1Hz blink
        if (!(loop % (UT_TICK_FREQ/2)))
            digitalTogg(supervisorData.status);

        // calculate idle time
        if (!(loop % (UT_TICK_FREQ / 100))) {
            supervisorData.idlePercent = (idleCounter - lastCounter) * 100.0f / (rccClocks.SYSCLK_Frequency / idleMinCycles) * 100.0f;
            lastCounter = idleCounter;
        }

        escTimeoutCheck();

        loop++;

        utYield(1);
    }
}

void supervisorInit(void) {
    supervisorData.status = digitalInit(SUPERVISOR_READY_PORT, SUPERVISOR_READY_PIN, 0);

    supervisorData.task = utCreateTask(supervisorCode, 0, 100, SUPERVISOR_STACK_SIZE);
}