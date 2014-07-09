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

#include "graph.h"
#include "main.h"
#include "osd.h"
#include "font.h"
#include <math.h>
#include <stdlib.h>

graphStruct_t graphData;

void static inline _graphSetCoordinate(uint16_t x, uint16_t y) {
    graphData.lastX = x;
    graphData.lastY = y;
}

void graphSetCoordinate(uint16_t x, uint16_t y) {
    _graphSetCoordinate(x, y);
}

void graphSetDoublePlot(uint8_t a) {
    graphData.doublePlot = a;
}

// bitband version
void static inline _graphPlot(uint16_t x, uint16_t y, uint8_t color) {
    register uint32_t offset;

#ifdef DEBUG
    while (x < 0 || x >= OSD_VERT_PIXELS || y < 0 || y >= OSD_HORZ_PIXELS)
        __NOP;
#endif

//    offset = (x * OSD_HORZ_PIXELS/8 + y/8) * 32 + (7 - (y%8)) * 4;
    offset = x * OSD_HORZ_PIXELS + y + 7 - 2*(y%8);

    graphData.bbBaseWhite[offset] = (color & 0x01);
    graphData.bbBaseBlack[offset] = (color & 0x02)>>1;

    if (graphData.doublePlot) {
        graphData.bbBaseWhite[offset-OSD_HORZ_PIXELS] = (color & 0x01);
        graphData.bbBaseBlack[offset-OSD_HORZ_PIXELS] = (color & 0x02)>>1;

        offset = x * OSD_HORZ_PIXELS + (y-1) + 7 - 2*((y-1)%8);

        graphData.bbBaseWhite[offset] = (color & 0x01);
        graphData.bbBaseBlack[offset] = (color & 0x02)>>1;

        graphData.bbBaseWhite[offset-OSD_HORZ_PIXELS] = (color & 0x01);
        graphData.bbBaseBlack[offset-OSD_HORZ_PIXELS] = (color & 0x02)>>1;
    }

    _graphSetCoordinate(x, y);
}

void graphPlot(uint16_t x, uint16_t y, uint8_t color) {
    _graphPlot(x, y, color);
}

// From TC
void osdDrawEllipseRect(int x0, int y0, int x1, int y1, int color) {
    int a = abs(x1-x0);
    int b = abs(y1-y0);
    int b1 = b&1;                       // values of diameter
    long dx = 4 * (1 - a) * b * b;
    long dy = 4 *(b1 + 1) * a * a;      // error increment
    long err = dx + dy + b1 * a * a;
    long e2;                            // error of 1.step

    // if called with swapped points
    if (x0 > x1) {
        x0 = x1;
        x1 += a;
    }

    // .. exchange them
    if (y0 > y1)
        y0 = y1;
    y0 += (b + 1) / 2;
    y1 = y0 - b1;   // starting pixel
    a *= 8 * a;
    b1 = 8 * b * b;

    do {
        _graphPlot(x1, y0, color); //   I. Quadrant
        _graphPlot(x0, y0, color); //  II. Quadrant
        _graphPlot(x0, y1, color); // III. Quadrant
        _graphPlot(x1, y1, color); //  IV. Quadrant

        e2 = 2 * err;
        if (e2 <= dy) {
            y0++;
            y1--;
            err += dy += a;
        }  // y step

        if (e2 >= dx || 2 * err > dy) {
            x0++;
            x1--;
            err += dx += b1;
        } // x step
    } while (x0 <= x1);

    while (y0 - y1 < b) {                   // too early stop of flat ellipses a=1
        _graphPlot(x0-1, y0, color);        // -> finish tip of ellipse
        _graphPlot(x1+1, y0++, color);
        _graphPlot(x0-1, y1, color);
        _graphPlot(x1+1, y1--, color);
    }
}

// From TC
void graphCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
    int offset_x;
    int offset_y;
    int16_t error;

    // Draw only a pixel if radius is zero.
    if (radius == 0) {
        _graphPlot(x, y, color);
        return;
    }
    // Set up start iterators.
    offset_x = 0;
    offset_y = radius;
    error = 3 - 2 * radius;

    // Iterate offsetX from 0 to radius.
    while (offset_x <= offset_y) {
        _graphPlot(x + offset_y, y - offset_x, color);
        _graphPlot(x + offset_x, y - offset_y, color);
        _graphPlot(x - offset_x, y - offset_y, color);
        _graphPlot(x - offset_y, y - offset_x, color);
        _graphPlot(x - offset_y, y + offset_x, color);
        _graphPlot(x - offset_x, y + offset_y, color);
        _graphPlot(x + offset_x, y + offset_y, color);
        _graphPlot(x + offset_y, y + offset_x, color);

        // Update error value and step offset_y when required.
        if (error < 0) {
            error += ((offset_x << 2) + 6);
        } else {
            error += (((offset_x - offset_y) << 2) + 10);
            --offset_y;
        }

        // Next X.
        ++offset_x;
    }
}

// Bresenham's line algorithm
void graphLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int err = dx - dy;
    int sx, sy;
    int e2;

    sx = (x0 < x1) ? 1 : -1;

    sy = (y0 < y1) ? 1 : -1;

    while (1) {
        _graphPlot(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2*err;

        if (e2 > -dy) {
            err = err - dy;
            x0 = x0 + sx;
        }

        if (x0 == x1 && y0 == y1) {
            _graphPlot(x0, y0, color);
            break;
        }

        if (e2 <  dx) {
            err = err + dx;
            y0 = y0 + sy;
        }
    }
}

void graphLineTo(uint16_t x, uint16_t y, uint8_t color) {
    graphLine(graphData.lastX, graphData.lastY, x, y, color);
}

void graphSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, uint8_t fill) {
    if (!fill) {
        graphLine(x0, y0, x1, y0, color);
        graphLine(x0, y0, x0, y1, color);
        graphLine(x1, y1, x1, y0, color);
        graphLine(x1, y1, x0, y1, color);
    }
    else {
        int x, y;
        int sx = 0, sy = 0;

        if (x0 < x1)
                sx = 1;
        else if (x0 > x1)
                sx = -1;

        if (y0 < y1)
                sy = 1;
        else if (y0 > y1)
                sy = -1;

        x = x0;
        do {
            y = y0;
            do {
                _graphPlot(x, y, color);
                y += sy;
            } while (y != y1);
            x += sx;
        } while (x != x1);
    }
}

void graphDrawChar(uint16_t x, uint16_t y, const int8_t *vec, float scale, uint8_t color) {
    int i;

    _graphSetCoordinate(x - vec[3] * scale, y + vec[2] * scale);

    i = 2;
    do {
        // pen up
        if (vec[i*2] == -1 && vec[i*2+1] == -1) {
            i++;
            _graphSetCoordinate(x - vec[i*2+1] * scale, y + vec[i*2] * scale);
            i++;
        }
        else {
            graphLineTo(x - vec[i*2+1] * scale, y + vec[i*2] * scale, color);
            i++;
        }
    } while (i <= vec[0]);
}

uint8_t graphChar(uint16_t x, uint16_t y, uint8_t ascii, float scale, uint8_t color) {
    ascii -= 32;

    graphDrawChar(x, y, fontSimplex[ascii], scale, color);

    return fontSimplex[ascii][1] + 1;
}

void graphString(uint16_t x, uint16_t y, char *str, float scale, uint8_t color) {
    int yPos = 0;
    int i = 0;

    while (str[i]) {
        yPos += graphChar(x, y + (yPos * scale), str[i], scale, color);
        i++;
    }
}

extern const uint8_t logoWhite[];
extern const uint8_t logoWhite_x[];
extern const uint8_t logoBlack[];
extern const uint8_t logoBlack_x[];

void graphLoadCompressed(uint8_t *v, uint8_t *p, uint8_t *pixBuf, uint32_t size) {
    uint32_t i, j;

    i = 0;
    while (i < size) {
        if (*p & (1<<7)) {
            for (j = 0; j < (*p & 0x7f); j++)
                pixBuf[i++] = *v;
            v++;
        }
        else {
            for (j = 0; j < *p; j++)
                pixBuf[i++] = *v++;
        }
        p++;
    }
}

void graphInit(void) {
    graphData.bbBaseWhite = (uint32_t *)(SRAM1_BB_BASE + ((uint32_t)osdPixBufWhite - SRAM1_BASE) * 32);
    graphData.bbBaseBlack = (uint32_t *)(SRAM1_BB_BASE + ((uint32_t)osdPixBufBlack - SRAM1_BASE) * 32);

    osdDMAClear(0);

    graphLoadCompressed((uint8_t *)logoWhite, (uint8_t *)logoWhite_x, (uint8_t *)osdPixBufWhite, sizeof(osdPixBufWhite));
    graphLoadCompressed((uint8_t *)logoBlack, (uint8_t *)logoBlack_x, (uint8_t *)osdPixBufBlack, sizeof(osdPixBufBlack));

    graphSetDoublePlot(1);
    graphString(430, 475, VERSION, 0.7f, 1);
}