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

#include "main.h"
#include "analog.h"
#include "aq_timer.h"

analogStruct_t analogData;
uint16_t rawSamples[ANALOG_SAMPLES*ANALOG_CHANNELS] __attribute__((section(".bss2")));

void analogDecode(float *values) {
    int i, j;

    for (i = 0; i < ANALOG_CHANNELS; i++)
        analogData.rawChannels[i] = 0;

    for (i = 0; i < ANALOG_SAMPLES; i++)
        for (j = 0; j < ANALOG_CHANNELS; j++)
            analogData.rawChannels[j] += rawSamples[i * ANALOG_CHANNELS + j];

    for (i = 0; i < ANALOG_CHANNELS; i++)
        analogData.voltages[i] = analogData.rawChannels[i] * ANALOG_DIVISOR;

    if (values) {
        values[0] = analogData.voltages[ANALOG_VOLTS_VIN] * ANALOG_VIN_SLOPE;
        values[1] = (analogData.voltages[ANALOG_VOLTS_AMP] - analogData.ampOffset) * ANALOG_AMP_SLOPE;
        values[2] = ((analogData.voltages[ANALOG_VOLTS_TEMP] - ANALOG_TEMP_V25) * ANALOG_TEMP_AVG_SLOPE) + 25.0f;
    }
}

float analogAmpOffset(void) {
    float offset;
    int i;

    // offset stored yet?
    if (*(uint32_t *)ANALOG_AMP_OFFSET_ADDR != 0xFFFFFFFF) {
        offset = *(float *)ANALOG_AMP_OFFSET_ADDR;
        // check for user override
        if (*(uint32_t *)(ANALOG_AMP_OFFSET_ADDR+4) != 0xFFFFFFFF)
            offset = *(float *)(ANALOG_AMP_OFFSET_ADDR+4);
    }
    // calculate offset
    else {
        offset = 0.0f;

        // get average current sensor voltage over 100ms
        timerDelayMilli(50);
        for (i = 0; i < 100; i++) {
            timerDelayMilli(1);
            analogDecode(0);
            offset += analogData.voltages[ANALOG_VOLTS_AMP];
        }
        offset = (offset / 100.0f) - (ANALOG_AMP_NOLOAD / ANALOG_AMP_SLOPE);

        // store in OTP
//        FLASH_Unlock();
//        FLASH_ProgramWord(ANALOG_AMP_OFFSET_ADDR, *(uint32_t *)&offset);
//        FLASH_Lock();
    }

    return offset;
}

void analogInit(void) {
    DMA_InitTypeDef DMA_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_TempSensorVrefintCmd(ENABLE);

    DMA_DeInit(ANALOG_DMA_STREAM);
    DMA_InitStructure.DMA_Channel = ANALOG_DMA_CHANNEL;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rawSamples;
    DMA_InitStructure.DMA_PeripheralBaseAddr = ((uint32_t)ADC1+0x4c);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = ANALOG_SAMPLES*ANALOG_CHANNELS;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(ANALOG_DMA_STREAM, &DMA_InitStructure);

    DMA_Cmd(ANALOG_DMA_STREAM, ENABLE);

    // ADC Common Init
    ADC_CommonStructInit(&ADC_CommonInitStructure);
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
    ADC_CommonInit(&ADC_CommonInitStructure);

    // ADC1 configuration
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = ANALOG_CHANNELS;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ANALOG_CHANNEL_VIN, 1, ANALOG_SAMPLE_TIME);
    ADC_RegularChannelConfig(ADC1, ANALOG_CHANNEL_AMP, 2, ANALOG_SAMPLE_TIME);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 3, ANALOG_SAMPLE_TIME);

    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_SoftwareStartConv(ADC1);

    analogData.ampOffset = analogAmpOffset();
}
