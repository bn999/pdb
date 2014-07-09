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

#include <__cross_studio_io.h>
#include "main.h"
#include "board.h"
#include "rcc.h"
#include "aq_timer.h"
#include "sensors.h"
#include "serial.h"
#include "digital.h"
#include "idle.h"
#include "supervisor.h"
#include "comm.h"
#include "esc.h"
#include "osd.h"
#include "opto.h"
#include "ut.h"
#include "graph.h"
#include "render.h"
#include <stdio.h>

void main(void) {
    rccConfiguration();
    timerInit();

    utInit(idleFunc, 0, IDLE_STACK_SIZE);

    canInit();
    commInit();
    supervisorInit();
    sensorsInit();
    escInit();
    osdInit();
    optoInit();
    graphInit();
    renderInit();

    utStart();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
    debug_printf("Wrong parameters value: file %s on line %d\r\n", file, line);

    while (1)
        ;
}
#endif
