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

#include "opto.h"
#include "switch.h"

optoStruct_t optoData;

void optoInit(void) {
    optoData.cam1 = digitalInit(GPIOC, GPIO_Pin_2, 0);
    optoData.cam2 = digitalInit(GPIOC, GPIO_Pin_3, 0);

    switchRegister(optoData.cam1, "OPTO_CAM1");
    switchRegister(optoData.cam2, "OPTO_CAM2");
}