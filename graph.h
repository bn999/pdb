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

#ifndef _graph_h
#define _graph_h

#include "osd.h"

typedef struct {
    uint32_t *bbBaseWhite;
    uint32_t *bbBaseBlack;
    uint16_t lastX;
    uint16_t lastY;
    uint8_t doublePlot;
} graphStruct_t;

extern void graphInit(void);
extern void graphPlot(uint16_t x, uint16_t y, uint8_t color);
extern void graphLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color);
extern void graphSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, uint8_t fill);
extern void graphLineTo(uint16_t x, uint16_t y, uint8_t color);
extern uint8_t graphChar(uint16_t x, uint16_t y, uint8_t ascii, float scale, uint8_t color);
extern void graphString(uint16_t x, uint16_t y, char *str, float scale, uint8_t color);
extern void graphSetCoordinate(uint16_t x, uint16_t y);
extern void graphSetDoublePlot(uint8_t a);
extern void graphCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
extern void osdDrawEllipseRect(int x0, int y0, int x1, int y1, int color);

#endif