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

#include "render.h"
#include "osd.h"
#include "graph.h"
#include "sensors.h"
#include "util.h"
#include "aq_timer.h"
#include "ut.h"
#include <stdint.h>
#include <intrinsics.h>
#include <stdio.h>
#include <math.h>

#define RENDER_SCALE        10000

float cubePoints[8][3] = {
    {-50, -50, -50},
    {-50, +50, -50},
    {+50, +50, -50},
    {+50, -50, -50},
    {-50, -50, +50},
    {-50, +50, +50},
    {+50, +50, +50},
    {+50, -50, +50},
};

uint8_t cubeFaces[6][5] = {
    {0, 1, 2, 3, 0},
    {1, 5, 6, 2, 1},
    {2, 6, 7, 3, 2},
    {0, 4, 5, 1, 0},
    {0, 3, 7, 4, 0},
    {7, 6, 5, 4, 7},
};

renderStruct_t renderData;

void renderRotateVectorByQuat(float *vr, float *v, float *q) {
    float w, x, y, z;

    w = q[0];
    x = q[1];
    y = q[2];
    z = q[3];

    vr[0] = w*w*v[0] + 2.0f*y*w*v[2] - 2.0f*z*w*v[1] + x*x*v[0] + 2.0f*y*x*v[1] + 2.0f*z*x*v[2] - z*z*v[0] - y*y*v[0];
    vr[1] = 2.0f*x*y*v[0] + y*y*v[1] + 2.0f*z*y*v[2] + 2.0f*w*z*v[0] - z*z*v[1] + w*w*v[1] - 2.0f*x*w*v[2] - x*x*v[1];
    vr[2] = 2.0f*x*z*v[0] + 2.0f*y*z*v[1] + z*z*v[2] - 2.0f*w*y*v[0] - y*y*v[2] + 2.0f*w*x*v[1] - x*x*v[2] + w*w*v[2];
}

static void renderRotateQuat(float *qOut, float *qIn, float *rate, float dt) {
    float q[4];
    float r[3];

    r[0] = rate[0] * -0.5f;
    r[1] = rate[1] * -0.5f;
    r[2] = rate[2] * -0.5f;

    q[0] = qIn[0];
    q[1] = qIn[1];
    q[2] = qIn[2];
    q[3] = qIn[3];

    // rotate
    qOut[0] =       q[0] + r[0]*q[1] + r[1]*q[2] + r[2]*q[3];
    qOut[1] = -r[0]*q[0] +      q[1] - r[2]*q[2] + r[1]*q[3];
    qOut[2] = -r[1]*q[0] + r[2]*q[1] +      q[2] - r[0]*q[3];
    qOut[3] = -r[2]*q[0] - r[1]*q[1] + r[0]*q[2] +      q[3];
}

void renderNormalizeQuat(float *qr, float *q) {
    float norm;

    norm = 1.0f / __sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);

    qr[0] = q[0] * norm;
    qr[1] = q[1] * norm;
    qr[2] = q[2] * norm;
    qr[3] = q[3] * norm;
}

void renderQuatExtractEuler(float *q, float *yaw, float *pitch, float *roll) {
    float q0, q1, q2, q3;

    q0 = q[1];
    q1 = q[2];
    q2 = q[3];
    q3 = q[0];

    *yaw = atan2f((2.0f * (q0 * q1 + q3 * q2)), (q3*q3 - q2*q2 - q1*q1 + q0*q0));
    *pitch = asinf(-2.0f * (q0 * q2 - q1 * q3));
    *roll = atanf((2.0f * (q1 * q2 + q0 * q3)) / (q3*q3 + q2*q2 - q1*q1 -q0*q0));
}

static inline void renderProject(float *x, float *y, float z) {
    float zs = (1.0f / (z + renderData.scale3D));

    *x = OSD_VERT_PIXELS/2 + *x * renderData.scale3D * zs;
    *y = OSD_HORZ_PIXELS/2 + *y * renderData.scale3D * zs;
}

void renderPlot3D(float x, float y, float z, uint8_t color) {
    renderProject(&x, &y, z);
    graphPlot((uint16_t)x, (uint16_t)y, color);
}

void renderLineTo3D(float x, float y, float z, uint8_t color) {
    renderProject(&x, &y, z);
    graphLineTo((uint16_t)x, (uint16_t)y, color);
}

void renderFaces(float p[8][3]) {
    int i, j;

    // faces
    for (i = 0; i < 6; i++) {
        renderPlot3D(p[cubeFaces[i][0]][0], p[cubeFaces[i][0]][1], p[cubeFaces[i][0]][2], RENDER_COLOR);

        // points
        for (j = 1; j < 5; j++)
            renderLineTo3D(p[cubeFaces[i][j]][0], p[cubeFaces[i][j]][1], p[cubeFaces[i][j]][2], RENDER_COLOR);
    }
}

void renderCube(float *q) {
    float newPoints[8][3];
    int i;

    for (i = 0; i < 8; i++)
        renderRotateVectorByQuat(newPoints[i], cubePoints[i], q);

    renderFaces(newPoints);
}

void renderPower(uint16_t x, uint16_t y) {
    uint32_t s;
    char str[32];

    graphSetDoublePlot(1);

    snprintf(str, sizeof(str), "%4.1fV", renderData.smoothedVolt);
    graphString(x, y, str, 0.6f, RENDER_COLOR);

    snprintf(str, sizeof(str), "%4.1fA", renderData.smoothedAmp);
    graphString(x+22, y, str, 0.6f, RENDER_COLOR);

    snprintf(str, sizeof(str), "%4dmA", (int)sensorsData.mAh);
    graphString(x+44, y, str, 0.6f, RENDER_COLOR);

    s = renderData.totalTicks / UT_TICK_FREQ;
    snprintf(str, sizeof(str), "%02d:%02d:%02d", s/3600, (s/60)%60, s%60);
    graphString(x+66, y, str, 0.5f, RENDER_COLOR);
}

#define OSD_VARIO_WIDTH     100
#define OSD_VARIO_HEIGHT    68
#define OSD_VARIO_SCALE     10.0f
#define OSD_VARIO_AVG       4
#define OSD_VARIO_IND_SIZE  5

void renderVelGraph(uint16_t x, uint16_t y) {
    static float vel[OSD_VARIO_WIDTH];
    static uint8_t vIndex = 0;
    static uint8_t vCount = 0;
    char str[32];
    int i, j;

    // avg data
    vel[vIndex] += renderData.vel;

    graphSetDoublePlot(1);

    // axes
    graphLine(x+OSD_VARIO_HEIGHT/2, y, x+OSD_VARIO_HEIGHT/2, y+OSD_VARIO_WIDTH, RENDER_COLOR);
    graphLine(x-OSD_VARIO_HEIGHT/2, y+OSD_VARIO_WIDTH, x+OSD_VARIO_HEIGHT/2, y+OSD_VARIO_WIDTH, RENDER_COLOR);

    // graph
    j = (vIndex + 1) % OSD_VARIO_WIDTH;
    graphSetCoordinate(x +OSD_VARIO_HEIGHT/2 - (constrain(vel[j]/OSD_VARIO_AVG, 0, +OSD_VARIO_SCALE) / OSD_VARIO_SCALE * OSD_VARIO_HEIGHT), y+i);

    for (i = 0; i < OSD_VARIO_WIDTH-1; i++) {
        graphLineTo(x +OSD_VARIO_HEIGHT/2 - (constrain(vel[j]/OSD_VARIO_AVG, 0, +OSD_VARIO_SCALE) / OSD_VARIO_SCALE * OSD_VARIO_HEIGHT), y+i, RENDER_COLOR);

        j =  (j + 1) % OSD_VARIO_WIDTH;
    }

    // indicator
    if (--j < 0)
        j = OSD_VARIO_WIDTH;

    i = x +OSD_VARIO_HEIGHT/2 - (constrain(vel[j]/OSD_VARIO_AVG, 0, +OSD_VARIO_SCALE) / OSD_VARIO_SCALE * OSD_VARIO_HEIGHT);

    graphLine(i, y+OSD_VARIO_WIDTH+3, i-OSD_VARIO_IND_SIZE, y+OSD_VARIO_WIDTH+OSD_VARIO_IND_SIZE+3, RENDER_COLOR);
    graphLineTo(i+OSD_VARIO_IND_SIZE, y+OSD_VARIO_WIDTH+OSD_VARIO_IND_SIZE+3, RENDER_COLOR);
    graphLineTo(i, y+OSD_VARIO_WIDTH+3, RENDER_COLOR);

    // vertical velocity text
    graphSquare(x - OSD_VARIO_HEIGHT/4, y, x - OSD_VARIO_HEIGHT, y+56, 0, RENDER_COLOR);

    snprintf(str, sizeof(str), "%4.1f", vel[j]/OSD_VARIO_AVG);
    graphString(x - OSD_VARIO_HEIGHT/4-3, y-2, str, 0.55f, RENDER_COLOR);
    graphChar(x - OSD_VARIO_HEIGHT/4-12, y+40, 'm', 0.35f, RENDER_COLOR);
    graphChar(x - OSD_VARIO_HEIGHT/4-1, y+46, 's', 0.55f, RENDER_COLOR);

    if (++vCount == OSD_VARIO_AVG) {
        vIndex = (vIndex + 1) % OSD_VARIO_WIDTH;
        vCount = 0;
        vel[vIndex] = 0.0f;
    }
}

#define OSD_BEARING_RADIUS      45
#define OSD_BEARING_IND_SIZE    7

static void inline renderRelBrngYawRot(int16_t x1, int16_t y1, int16_t *x2, int16_t *y2) {
    *x2 = x1 * renderData.relBrgCos - y1 * renderData.relBrgSin;
    *y2 = y1 * renderData.relBrgCos + x1 * renderData.relBrgSin;
}

static void inline renderYawRot(int16_t x1, int16_t y1, int16_t *x2, int16_t *y2) {
    *x2 = x1 * renderData.yawCos + y1 * renderData.yawSin;
    *y2 = y1 * renderData.yawCos - x1 * renderData.yawSin;
}

void renderBearing(int16_t x, int16_t y) {
    char str[32];
    int16_t x1, y1;

    graphSetDoublePlot(1);

    // relative velocity
    if (renderData.vel > 0.5f) {
        renderYawRot(renderData.scaledVelN*OSD_BEARING_RADIUS, renderData.scaledVelE*OSD_BEARING_RADIUS, &x1, &y1);

        graphLine(x, y, x - x1, y + y1, RENDER_COLOR);
    }

    // home direction indicator
    if (osdData.data.homeDist > 0.5f) {
        renderRelBrngYawRot(-OSD_BEARING_RADIUS+OSD_BEARING_IND_SIZE/2, 0, &x1, &y1);
        graphSetCoordinate(x+x1, y+y1);

        renderRelBrngYawRot(-OSD_BEARING_RADIUS + OSD_BEARING_IND_SIZE+OSD_BEARING_IND_SIZE/2, -OSD_BEARING_IND_SIZE, &x1, &y1);
        graphLineTo(x+x1, y+y1, RENDER_COLOR);

        renderRelBrngYawRot(-OSD_BEARING_RADIUS + OSD_BEARING_IND_SIZE+OSD_BEARING_IND_SIZE/2, +OSD_BEARING_IND_SIZE, &x1, &y1);
        graphLineTo(x+x1, y+y1, RENDER_COLOR);

        renderRelBrngYawRot(-OSD_BEARING_RADIUS+OSD_BEARING_IND_SIZE/2, 0, &x1, &y1);
        graphLineTo(x+x1, y+y1, RENDER_COLOR);
    }

    // clear area for text
    graphSquare(x-8, y-25, x+8, y+25, 0, RENDER_COLOR);

    // distance to home
    if (osdData.data.homeDist < 9999.0f) {
        snprintf(str, sizeof(str), "%4.0fm", osdData.data.homeDist);
        graphString(x+6, y-34, str, 0.55f, RENDER_COLOR);
    }
    else {
        graphString(x+6, y-34, "----m", 0.55f, RENDER_COLOR);
    }

    graphCircle(x, y, OSD_BEARING_RADIUS, RENDER_COLOR);
}

void renderReticle(void) {
    graphSetDoublePlot(0);
    osdDrawEllipseRect(OSD_VERT_PIXELS/2-5, OSD_HORZ_PIXELS/2-6, OSD_VERT_PIXELS/2+5, OSD_HORZ_PIXELS/2+6, RENDER_COLOR);
    graphLine(OSD_VERT_PIXELS/2, OSD_HORZ_PIXELS/2-17, OSD_VERT_PIXELS/2, OSD_HORZ_PIXELS/2-11, RENDER_COLOR);
    graphLine(OSD_VERT_PIXELS/2, OSD_HORZ_PIXELS/2+17, OSD_VERT_PIXELS/2, OSD_HORZ_PIXELS/2+11, RENDER_COLOR);
    graphLine(OSD_VERT_PIXELS/2-9, OSD_HORZ_PIXELS/2, OSD_VERT_PIXELS/2-13, OSD_HORZ_PIXELS/2, RENDER_COLOR);
}

#define OSD_HEADING_WIDTH       250
#define OSD_HEADING_HEIGHT      10
#define OSS_HEADING_IND_SIZE    5

void renderHeading(uint16_t x, uint16_t y) {
    char str[32];
    uint16_t y1, x1;
    int a, i;

    graphSetDoublePlot(1);

    a = (int)(renderData.yaw - 80.0f) / 10 * 10;

    // at least 180 degs
    for (i = a; i != compassNormalize(a + 190.0f); i = compassNormalize(i + 10.0f)) {
        y1 = y + compassDiff(i, renderData.yaw) * OSD_HEADING_WIDTH * (1.0f / 180.0f);
        x1 = x;

        // cardinal directions
        if (i == 0) {
            x1 -= 5;
            graphChar(x1-12, y1-6, 'N', 0.6f, RENDER_COLOR);
        }
        else if (i == 90) {
            x1 -= 5;
            graphChar(x1-12, y1-6, 'E', 0.6f, RENDER_COLOR);
        }
        else if (i == 180) {
            x1 -= 5;
            graphChar(x1-12, y1-6, 'S', 0.6f, RENDER_COLOR);
        }
        else if (i == 270) {
            x1 -= 5;
            graphChar(x1-12, y1-6, 'W', 0.6f, RENDER_COLOR);
        }

        graphLine(x1 - OSD_HEADING_HEIGHT/2, y1, x + OSD_HEADING_HEIGHT/2, y1, RENDER_COLOR);
    }

    // indicator
    graphLine(x + OSD_HEADING_HEIGHT/2, y, x + OSD_HEADING_HEIGHT/2 + OSS_HEADING_IND_SIZE, y - OSS_HEADING_IND_SIZE, RENDER_COLOR);
    graphLineTo(x + OSD_HEADING_HEIGHT/2 + OSS_HEADING_IND_SIZE, y + OSS_HEADING_IND_SIZE, RENDER_COLOR);
    graphLineTo(x + OSD_HEADING_HEIGHT/2, y, RENDER_COLOR);

    // heading
    itoa((int)renderData.yaw, str, 10);
    graphString(x + OSD_HEADING_HEIGHT + 20, y-20, str, 0.7f, RENDER_COLOR);
}

void renderSignal(uint16_t x, uint16_t y) {
    char str[32];

    graphSetDoublePlot(1);

    if (osdData.data.state == STATE_INITIALIZING) {
        graphString(x, y, "Initializing", 0.6f, RENDER_COLOR);
    }
    else if (osdData.data.state & STATE_DISARMED) {
        graphString(x, y, "Disarmed", 0.6f, RENDER_COLOR);
    }
    else {
        switch (osdData.data.mode) {
            case NAV_STATUS_MANUAL:
                graphString(x, y, "Manual", 0.6f, RENDER_COLOR);
                break;

            case NAV_STATUS_ALTHOLD:
                graphString(x, y, "ALT hold", 0.6f, RENDER_COLOR);
                break;

            case NAV_STATUS_POSHOLD:
                graphString(x, y, "POS hold", 0.6f, RENDER_COLOR);
                break;

            case NAV_STATUS_DVH:
                graphString(x, y, "DVH", 0.6f, RENDER_COLOR);
                break;

            case NAV_STATUS_MISSION:
                graphString(x, y, "Mission", 0.6f, RENDER_COLOR);
                break;

        }
    }

    if (osdData.data.gpsHacc >= 10.0f || osdData.data.gpsHacc == 0.0f) {
        graphString(x+22, y, "GPS  N/A", 0.6f, RENDER_COLOR);
    }
    else {
        snprintf(str, sizeof(str), "GPS %3.1fm", osdData.data.gpsHacc);
        graphString(x+22, y, str, 0.6f, RENDER_COLOR);
    }

    snprintf(str, sizeof(str), "RC  %3.0f%%", osdData.data.rcQuality);
    graphString(x+44, y, str, 0.6f, RENDER_COLOR);
}

#define OSD_ALT_HEIGHT      250
#define OSD_ALT_WIDTH       10

void renderAltitude(uint16_t x, uint16_t y) {
    char str[32];
    float alt;
    uint16_t y1, x1;
    float i;
    int a;

    graphSetDoublePlot(1);

    alt = osdData.data.alt - renderData.groundAlt;

    a = (int)(alt - 20.0f) / 2 * 2;

    for (i = a; (int)i != a+40; i += 2) {
        x1 = x + (alt - i) * OSD_ALT_HEIGHT * (1.0f / 40.0f);
        y1 = y;

        // every 10m of altitude
        if ((int)i == ((int)i / 10 * 10)) {
            y1 -= 5;
            // don't print if too close to center indicator
            if (x1 > (x + 25) || x1 < (x - 25)) {
                if (fabsf(i) <= 9999.0f) {
                    itoa((int)i, str, 10);
                    graphString(x1+6, y + OSD_ALT_WIDTH/2 + 15, str, 0.55f, RENDER_COLOR);
                }
            }
        }

        graphLine(x1, y1 - OSD_ALT_WIDTH/2, x1, y + OSD_ALT_WIDTH/2, RENDER_COLOR);
    }

    // indicator box
    graphLine(x, y, x - 15, y + 15, RENDER_COLOR);
    graphLineTo(x - 15, y + 72, RENDER_COLOR);
    graphLineTo(x + 15, y + 72, RENDER_COLOR);
    graphLineTo(x + 15, y + 15, RENDER_COLOR);
    graphLineTo(x, y, RENDER_COLOR);

    if (fabsf(alt) <= 9999.0f) {
        itoa((int)alt, str, 10);
        graphString(x+7, y + 14, str, 0.7f, RENDER_COLOR);
    }
    else {
        graphString(x+7, y + 14, "----", 0.7f, RENDER_COLOR);
    }
}

#define OSD_VEL_HEIGHT      200
#define OSD_VEL_WIDTH       10
#define OSD_VEL_SCALE       8

void renderVertVelocity(uint16_t x, uint16_t y) {
    static float avgVertVel;
    char str[32];
    uint16_t y1, x1;
    float i;
    int a;

    graphSetDoublePlot(1);

    avgVertVel += (osdData.data.velNED[2] - avgVertVel) * 0.1f;

    a = (int)(-avgVertVel - 0.5f - OSD_VEL_SCALE/2);

    for (i = a; i <= a+OSD_VEL_SCALE; i += 0.5f) {
        x1 = x + (-avgVertVel - i) * OSD_VEL_HEIGHT * (1.0f / OSD_VEL_SCALE);
        y1 = y;

        // every 2m/s of velocity
        if ((i*2.0f) == ((int)i * 2) && !((int)i%2)) {
            y1 -= 5;
            // don't print if too close to center indicator
            if (x1 > (x + 20) || x1 < (x - 20)) {
                if (fabsf(i) <= 99.0f) {
                    snprintf(str, sizeof(str), "%+2.0f", i);
                    graphString(x1+7, y - OSD_VEL_WIDTH/2 - 45, str, 0.45f, RENDER_COLOR);
                }
            }
        }

        graphLine(x1, y1 - OSD_VEL_WIDTH/2, x1, y + OSD_VEL_WIDTH/2, RENDER_COLOR);
    }

    // indicator box
    graphLine(x, y, x - 10, y - 15, RENDER_COLOR);
    graphLineTo(x - 10, y - 72, RENDER_COLOR);
    graphLineTo(x + 10, y - 72, RENDER_COLOR);
    graphLineTo(x + 10, y - 15, RENDER_COLOR);
    graphLineTo(x, y, RENDER_COLOR);

    if (fabsf(avgVertVel) <= 99.0f) {
        snprintf(str, sizeof(str), "%+5.1f", -avgVertVel);
        graphString(x+6, y - 75, str, 0.5f, RENDER_COLOR);
    }
}

// takes 3 vector sets and rotates based on roll
void static inline renderLadderRung(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, int deg) {
    char str[32];
    int16_t xt1, yt1, xt2, yt2;

    // transform points
    xt1 = x1 * renderData.rollCos - y1 * renderData.rollSin;
    yt1 = y1 * renderData.rollCos + x1 * renderData.rollSin;

    xt2 = x2 * renderData.rollCos - y2 * renderData.rollSin;
    yt2 = y2 * renderData.rollCos + x2 * renderData.rollSin;

    graphLine(OSD_VERT_PIXELS/2 + xt1, yt1 + OSD_HORZ_PIXELS/2, OSD_VERT_PIXELS/2 + xt2, yt2 + OSD_HORZ_PIXELS/2, RENDER_COLOR);

    xt1 = x3 * renderData.rollCos - y3 * renderData.rollSin;
    yt1 = y3 * renderData.rollCos + x3 * renderData.rollSin;

    graphLineTo(OSD_VERT_PIXELS/2 + xt1, yt1 + OSD_HORZ_PIXELS/2, RENDER_COLOR);

    // no text for 0 deg
    if (deg != 0) {
        itoa(deg, str, 10);

        // adjust text position
        xt1 += 5;
        if (y3 < 0)
            yt1 -= 35;
        else
            yt1 += 5;

        graphString(OSD_VERT_PIXELS/2 + xt1, yt1 + OSD_HORZ_PIXELS/2, str, 0.4f, RENDER_COLOR);
    }
}

#define OSD_LADDER_HEIGHT   300

void renderLadder() {
    int16_t x, y;
    int a, i;

    if (osdData.dataValid) {
        graphSetDoublePlot(1);

        a = (int)(renderData.pitch - 15.0f);
        if (a >= 0)
            a = (a + 10) / 10 * 10;
        else
            a = a / 10 * 10;

        for (i = a; i != (a + 30); i += 10) {
            x = (renderData.pitch - i) * OSD_LADDER_HEIGHT * (1.0f / 30.0f);

            // rung format for level
            if (i == 0) {
                renderLadderRung(x, -25, x, -25, x, -125, 0);
                renderLadderRung(x, +25, x, +25, x, +125, 0);
            }
            // positive rung format
            else if (i > 0) {
                renderLadderRung(x+10, -25, x, -25, x, -75, i);
                renderLadderRung(x+10, +25, x, +25, x, +75, i);
            }
            // negative rung format
            else {
                renderLadderRung(x-10, -25, x, -25, x+15, -65, i);
                renderLadderRung(x-10, +25, x, +25, x+15, +65, i);
            }
        }
    }
}

void renderInfo(uint16_t x, uint16_t y) {
    static uint8_t toggle = 0;

    if (!(renderData.loops % 30))
        toggle = (toggle) ? 0 : 1;

    if (toggle) {
        graphSetDoublePlot(1);

        if (osdData.data.state & STATE_LOW_BATTERY2)
            graphString(x, y, "Low BAT2 - Land now", 0.9, RENDER_COLOR);
        else if (osdData.data.state & STATE_LOW_BATTERY1)
            graphString(x, y, "Low BAT1 - Head home", 0.9, RENDER_COLOR);
        else if (osdData.data.state & STATE_RADIO_LOSS2)
            graphString(x, y, "Radio Loss stage 2", 0.9, RENDER_COLOR);
        else if (osdData.data.state & STATE_RADIO_LOSS1)
            graphString(x, y, "Radio Loss stage 1", 0.9, RENDER_COLOR);
    }
}

#define OSD_RADAR_MIN_SCALE     100
#define OSD_RADAR_RADIUS        200
#define OSD_RADAR_ICON_SIZE     8

void renderRadar(void) {
    int16_t x, y;
    int16_t x1, y1;

    if (osdData.data.homeDist > 10.0f && osdData.data.homeDist < 9999.0f) {
        graphSetDoublePlot(1);

        if (renderData.scaleRadar == 0.0f)
            renderData.scaleRadar = OSD_RADAR_MIN_SCALE;

        // auto scaling
        while (osdData.data.homeDist > renderData.scaleRadar)
            renderData.scaleRadar *= 2.0f;

        while (osdData.data.homeDist*4.0f < renderData.scaleRadar && renderData.scaleRadar > OSD_RADAR_MIN_SCALE)
            renderData.scaleRadar *= 0.5f;

        x = -osdData.data.homeDist * renderData.brgCos * OSD_RADAR_RADIUS / renderData.scaleRadar;
        y = osdData.data.homeDist * renderData.brgSin * OSD_RADAR_RADIUS / renderData.scaleRadar;

        renderYawRot(0, -OSD_RADAR_ICON_SIZE, &x1, &y1);
        graphSetCoordinate(OSD_VERT_PIXELS/2+x+x1, OSD_HORZ_PIXELS/2+y+y1);

        renderYawRot(-OSD_RADAR_ICON_SIZE, 0, &x1, &y1);
        graphLineTo(OSD_VERT_PIXELS/2+x+x1, OSD_HORZ_PIXELS/2+y+y1,RENDER_COLOR);

        renderYawRot(0, +OSD_RADAR_ICON_SIZE, &x1, &y1);
        graphLineTo(OSD_VERT_PIXELS/2+x+x1, OSD_HORZ_PIXELS/2+y+y1, RENDER_COLOR);
    }
}

void renderCode(void *unused) {
    // allow splash screen for 5 seconds after startup
    while (utTick() < 5000)
        utSleep();

    while (1) {
        utSleep();

        osdDMAClear(0);

        // try to order things top to bottom to keep ahead of scan line
//        renderCube(osdData.data.q);
        renderPower(23, 8);
        renderInfo(30, 100);
        renderBearing(50, 528);
        renderLadder();
        renderAltitude(OSD_VERT_PIXELS/2-4, 500);
        renderVertVelocity(OSD_VERT_PIXELS/2, 75);
        renderReticle();
        renderRadar();
        renderHeading(410, OSD_HORZ_PIXELS/2);
        renderSignal(393, 8);
        renderVelGraph(400, 464);

        // do this stuff at the end so that we don't take up precious draw time
        renderData.smoothedVolt = (renderData.smoothedVolt * RENDER_POWER_FILTER) + sensorsData.values[SENSORS_VBAT] * (1.0f - RENDER_POWER_FILTER);
        renderData.smoothedAmp = (renderData.smoothedAmp * RENDER_POWER_FILTER) + sensorsData.values[SENSORS_AMP] * (1.0f - RENDER_POWER_FILTER);

        if (osdData.dataValid) {
            renderQuatExtractEuler(osdData.data.q, &renderData.yaw, &renderData.pitch, &renderData.roll);
            renderData.yaw = compassNormalize(renderData.yaw * RAD_TO_DEG);
            renderData.pitch *= RAD_TO_DEG;
            renderData.roll *= RAD_TO_DEG;

            //    x' = x cos f - y sin f
            //    y' = y cos f + x sin f
            renderData.yawCos = cosf(renderData.yaw * DEG_TO_RAD);
            renderData.yawSin = sinf(renderData.yaw * DEG_TO_RAD);

            renderData.rollCos = cosf(renderData.roll * DEG_TO_RAD);
            renderData.rollSin = sinf(renderData.roll * DEG_TO_RAD);

            renderData.relBrgCos = cosf(compassDiff(renderData.yaw, osdData.data.homeBrng + 180.0f) * DEG_TO_RAD);
            renderData.relBrgSin = sinf(compassDiff(renderData.yaw, osdData.data.homeBrng + 180.0f) * DEG_TO_RAD);

            renderData.brgCos = cosf(osdData.data.homeBrng * DEG_TO_RAD);
            renderData.brgSin = sinf(osdData.data.homeBrng * DEG_TO_RAD);

            renderData.vel = __sqrtf(osdData.data.velNED[0]*osdData.data.velNED[0] + osdData.data.velNED[1]*osdData.data.velNED[1]);
            renderData.scaledVelN = osdData.data.velNED[0] / renderData.vel;
            renderData.scaledVelE = osdData.data.velNED[1] / renderData.vel;

            if (osdData.data.state & STATE_FLYING) {
                renderData.totalTicks += (utTick() - renderData.oldTicks);

                if (renderData.groundAlt == 0.0f)
                    renderData.groundAlt = osdData.data.alt;
            }
            else if (osdData.data.state & STATE_DISARMED) {
                    renderData.groundAlt = 0.0f;
            }

            renderData.oldTicks = utTick();
        }

        renderData.loops++;
    }
}

void renderInit(void) {
    renderData.scale3D = RENDER_SCALE;
    renderData.task = utCreateTask(renderCode, 0, 20, RENDER_STACK_SIZE);
}