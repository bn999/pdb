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

#ifndef _switch_h
#define _switch_h

#include "can.h"
#include "digital.h"

#define SWITCH_NUM      5

typedef struct {
    canNode_t *nodes[SWITCH_NUM];
    digitalPin *pins[SWITCH_NUM];
    char *names[SWITCH_NUM];
    uint8_t num;
} switchStruct_t;

extern void switchRegister(digitalPin *pin, char *name);

#endif