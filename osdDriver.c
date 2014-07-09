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

#include "osd.h"
#include "rcc.h"
#include "util.h"
#include "render.h"
#include "aq_timer.h"
#include "switch.h"
#include <string.h>

#include "digital.h"
digitalPin *tp;

osdStruct_t osdData;

uint32_t osdPixBufWhite[OSD_HORZ_PIXELS/32*OSD_VERT_PIXELS] __attribute__((section(".bss2")));
uint32_t osdPixBufBlack[OSD_HORZ_PIXELS/32*OSD_VERT_PIXELS] __attribute__((section(".bss2")));

uint32_t osdZero __attribute__((section(".bss2")));
uint32_t osdOne __attribute__((section(".bss2")));

// hard code these to the last bits of SRAM2 so that they can be accessed
// as a separate bus maxtrix slave by DMA, reducing contention for SRAM1
uint32_t *osdPixLineWhite = (uint32_t *)(0x2001ffa0);
uint32_t *osdPixLineBlack = (uint32_t *)(0x2001ff50);

uint32_t osdDacTxBuf __attribute__((section(".bss2")));

void osdSendDac(uint16_t data) {
    ((uint8_t *)&osdDacTxBuf)[0] = ((uint8_t *)&data)[1];
    ((uint8_t *)&osdDacTxBuf)[1] = ((uint8_t *)&data)[0];

    osdData.spiFlag = 0;
    spiTransaction(osdData.dac, &osdDacTxBuf, &osdDacTxBuf, 2);

    while (!osdData.spiFlag)
        ;
}

void osdInitDac(void) {
    timerDelayMilli(10);

    osdData.dac = spiClientInit(SPI1, SPI_BaudRatePrescaler_4, GPIOC, GPIO_Pin_14, &osdData.spiFlag, 0);

    // white
    osdSendDac(((0b0001)<<12) | (OSD_DAC_WHITE_LEVEL<<4));

    // black
    osdSendDac(((0b0101)<<12) | (OSD_DAC_BLACK_LEVEL<<4));
}

void osdInitSync(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    // External line PC15 for odd/even
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // External Interrupt line PC4 for vertical sync
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource4);

    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    osdData.vidSwitch = digitalInit(GPIOC, GPIO_Pin_13, 0);
    osdData.a0 = digitalInit(GPIOB, GPIO_Pin_4, 0);
    osdData.a1 = digitalInit(GPIOC, GPIO_Pin_11, 0);
//    osdData.cvbsHi = digitalInit(GPIOA, GPIO_Pin_1, 0);

    switchRegister(osdData.vidSwitch, "VIDEO_SOURCE");
}

void osdInitPixClock(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    TIM_BDTRInitTypeDef TIM_BDTRInitStruct;
    NVIC_InitTypeDef NVIC_InitStructure;

    // gated pixel clock slave (TIM1)    - 168MHz
    TIM_TimeBaseStructure.TIM_Period = (rccClocks.PCLK2_Frequency*2 / OSD_SPI_FREQ)-1;    // 13Mhz
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_BDTRStructInit(&TIM_BDTRInitStruct);
    TIM_BDTRInitStruct.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
    TIM_BDTRInitStruct.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRInitStruct.TIM_OSSIState = TIM_OSSIState_Enable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStruct);

    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 6;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Gated);
    TIM_SelectInputTrigger(TIM1, TIM_TS_ITR1);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

    // pixel clock gate master (TIM2)    - 84MHz
    TIM_TimeBaseStructure.TIM_Period = (OSD_HORZ_PIXELS+OSD_HORZ_OFFSET)-1+64;
    TIM_TimeBaseStructure.TIM_Prescaler = (rccClocks.PCLK2_Frequency / OSD_SPI_FREQ)-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = OSD_HORZ_OFFSET;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_OC2Ref);
    TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Enable);
    TIM_SelectOnePulseMode(TIM2, TIM_OPMode_Single);
    TIM_SelectInputTrigger(TIM2, TIM_TS_ETRF);
    TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Trigger);
    TIM_ETRConfig(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 1);

    TIM_ITConfig(TIM2, TIM_IT_Trigger, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void osdInitPixShift(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    // SPI1
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

    // SPI SCK / MISO pin configuration
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Connect SPI pins to Alternate Function
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource3, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);

    SPI_I2S_DeInit(SPI1);
    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

    DMA_DeInit(DMA2_Stream5);
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel = DMA_Channel_3;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)osdPixLineWhite;
    DMA_InitStructure.DMA_BufferSize = OSD_HORZ_PIXELS/8 + 2;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;                // note the buffer must be word aligned
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream5, &DMA_InitStructure);

    DMA_ITConfig(DMA2_Stream5, DMA_IT_TC, ENABLE);

    // Enable RX DMA global Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // SPI3
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

    // SPI SCK / MISO pin configuration
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Connect SPI pins to Alternate Function
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);

    SPI_I2S_DeInit(SPI3);
    SPI_Init(SPI3, &SPI_InitStructure);
    SPI_Cmd(SPI3, ENABLE);
    SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);

    DMA_DeInit(DMA1_Stream7);
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI3->DR;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)osdPixLineBlack;
    DMA_Init(DMA1_Stream7, &DMA_InitStructure);
}

static void inline osdDMAMemCpyWhite(uint32_t *src) {
    DMA2_Stream1->CR &= ~(uint32_t)DMA_SxCR_EN;
    DMA2->LIFCR = (uint32_t)(DMA_IT_TEIF1 | DMA_IT_DMEIF1 | DMA_IT_FEIF1 | DMA_IT_TCIF1 | DMA_IT_HTIF1);
    DMA2_Stream1->PAR = (uint32_t)src;
    DMA2_Stream1->CR |= (uint32_t)DMA_SxCR_EN;
}

static void inline osdDMAMemCpyBlack(uint32_t *src) {
    DMA2_Stream6->CR &= ~(uint32_t)DMA_SxCR_EN;
    DMA2->HIFCR = (uint32_t)(DMA_IT_TEIF6 | DMA_IT_DMEIF6 | DMA_IT_FEIF6 | DMA_IT_TCIF6 | DMA_IT_HTIF6);
    DMA2_Stream6->PAR = (uint32_t)src;
    DMA2_Stream6->CR |= (uint32_t)DMA_SxCR_EN;
}

static void osdDMAMemset(uint32_t *dest, uint32_t *src, uint32_t n) {
    DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;
    DMA2->LIFCR = (uint32_t)(DMA_IT_TEIF3 | DMA_IT_DMEIF3 | DMA_IT_FEIF3 | DMA_IT_TCIF3 | DMA_IT_HTIF3);

    DMA2_Stream3->PAR = (uint32_t)src;
    DMA2_Stream3->M0AR = (uint32_t)dest;
    DMA2_Stream3->NDTR = n;
    DMA2_Stream3->CR |= (uint32_t)DMA_SxCR_EN;

    while ((DMA2->LISR & (uint32_t)DMA_IT_TCIF3) == 0)
        ;
}

void osdDMAClear(uint8_t color) {
    uint32_t *src;

    src = (color == 1 || color == 3) ? &osdOne : &osdZero;
    osdDMAMemset(osdPixBufWhite, src, sizeof(osdPixBufWhite)/sizeof(uint32_t));

    src = (color == 2 || color == 3) ? &osdOne : &osdZero;
    osdDMAMemset(osdPixBufBlack, src, sizeof(osdPixBufWhite)/sizeof(uint32_t));
}

void osdDMAMemCpyInit(void) {
    DMA_InitTypeDef DMA_InitStructure;

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr = 0;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)osdPixLineWhite;
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToMemory;
    DMA_InitStructure.DMA_BufferSize = OSD_HORZ_PIXELS/32;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;

    // white line buf
    DMA_DeInit(DMA2_Stream1);
    DMA_Init(DMA2_Stream1, &DMA_InitStructure);

    // black line buf
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)osdPixLineBlack;
    DMA_DeInit(DMA2_Stream6);
    DMA_Init(DMA2_Stream6, &DMA_InitStructure);

    // pixel buffers clear
    DMA_InitStructure.DMA_BufferSize = sizeof(osdPixBufWhite)/sizeof(uint32_t);
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_DeInit(DMA2_Stream3);
    DMA_Init(DMA2_Stream3, &DMA_InitStructure);

    osdZero = 0;
    osdOne = -1;

    osdDMAMemset(osdPixLineWhite, &osdZero, OSD_HORZ_PIXELS/32+2);
    osdDMAMemset(osdPixLineBlack, &osdZero, OSD_HORZ_PIXELS/32+2);
};

// from TC
static void inline osdShadow(uint32_t *wh, uint32_t *bl, uint32_t *bL, int16_t line) {
    uint32_t a, a0, a2; // prev
    uint32_t b, b0, b2; // cur
    uint32_t c, c0, c2; // next
    int i;

    // prev, cur, next points in line
    //   a0,  b0,  c0: prev line
    //    a,   b,   c: cur  line
    //   a2,  b2,  c2: next line

    a = 0;
    a0 = 0;
    a2 = 0;
    b = __swap32(wh[0]);
    b0 = 0;
    if (line > 0)
        b0 = __swap32(wh[-OSD_HORZ_PIXELS / 32]);
    c0 = 0;
    b2 = __swap32(wh[OSD_HORZ_PIXELS / 32]);

    for (i = 1; i < OSD_HORZ_PIXELS / 32; i++) {
        c = __swap32(wh[i]);
        if (line > 0)
            c0 = __swap32(wh[i - OSD_HORZ_PIXELS / 32]);
        c2 = __swap32(wh[i + OSD_HORZ_PIXELS / 32 ]);

        bL[i - 1] = bl[i - 1] | __swap32((
                   a | (b << 1)  | (b >> 1)  | (c >> 31)             // right in prev, left in cur, right in cur, left in next
                | a0 | (b0 << 1) | (b0 >> 1) | (c0 >> 31) | b0
                | a2 | (b2 << 1) | (b2 >> 1) | (c2 >> 31) | b2
            ) & ~b);

        a = b << 31;
        b = c;
        a0 = b0 << 31;
        b0 = c0;
        a2 = b2 << 31;
        b2 = c2;
    }

    bL[i - 1] = bl[i - 1] | __swap32((
               a | (b << 1)  | (b >> 1)  /* | (c >> 31)  */
            | a0 | (b0 << 1) | (b0 >> 1) /* | (c0 >> 31) */ | b0
            | a2 | (b2 << 1) | (b2 >> 1) /* | (c2 >> 31) */ | b2
        ) & ~b);
}

// vertical sync
void EXTI4_IRQHandler(void) {
    EXTI_ClearITPendingBit(EXTI_Line4);

    // disable this interrupt
    EXTI->IMR &= ~EXTI_Line4;

    // enable horizontal sync input
    TIM2->SMCR = TIM2->SMCR & (uint16_t)~TIM_SMCR_SMS | TIM_SlaveMode_Trigger;

    osdData.vc++;
    osdData.hc = 0;
    osdData.base = 0;

    if (OSD_OE)
        osdData.base = OSD_HORZ_PIXELS/32;
}

// end of SPI TX
void DMA2_Stream5_IRQHandler(void) {
    int lineAddr;

    TIM1->CR1 &= (uint16_t)~TIM_CR1_CEN;

    DMA2_Stream5->CR &= ~(uint32_t)DMA_SxCR_EN;
    DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;

    DMA2->HIFCR = (uint32_t)(DMA_IT_TEIF5 | DMA_IT_DMEIF5 | DMA_IT_FEIF5 | DMA_IT_TCIF5 | DMA_IT_HTIF5);
    DMA1->HIFCR = (uint32_t)(DMA_IT_TEIF7 | DMA_IT_DMEIF7 | DMA_IT_FEIF7 | DMA_IT_TCIF7 | DMA_IT_HTIF7);

    // enable horizontal sync input
    TIM2->SMCR = TIM2->SMCR & (uint16_t)~TIM_SMCR_SMS | TIM_SlaveMode_Trigger;

    // setup next line to be sent
    lineAddr = osdData.base + (osdData.lc*2) * OSD_HORZ_PIXELS/32;

    // copy white line
    osdDMAMemCpyWhite(&osdPixBufWhite[lineAddr]);

    if (osdData.shadow)
        // create black shadows around white/gray pixels
        osdShadow(&osdPixBufWhite[lineAddr], &osdPixBufBlack[lineAddr], osdPixLineBlack, osdData.lc);
    else
        // copy black line
        osdDMAMemCpyBlack(&osdPixBufBlack[lineAddr]);

    osdData.lc++;

    osdData.inLine = 0;
}

// horizontal sync
void TIM2_IRQHandler(void) {
    int lineAddr;

    TIM2->SR = (uint16_t)~TIM_IT_Trigger;

    if (osdData.inLine) {
        osdData.timingErrors++;

        return;
    }

    osdData.hc++;

    if (osdData.hc < OSD_VERT_OFFSET || osdData.hc >= OSD_VERT_OFFSET+OSD_VERT_PIXELS/2) {
        // notify render thread it is safe to proceed
        if (osdData.lc && OSD_OE && renderData.task)
            utRunThread(renderData.task);

        // enable vertical sync interrupt
        EXTI->IMR |= EXTI_Line4;
        osdData.lc = 0;

        return;
    }

    // disable horizontal sync input
    TIM2->SMCR &= (uint16_t)~TIM_SMCR_SMS;
    osdData.inLine = 1;

    TIM1->CR1 |= (uint16_t)TIM_CR1_CEN;

    // setup a transparent line to kick things off
    if (osdData.lc == 0) {
        osdDMAMemset(osdPixLineWhite, &osdZero, OSD_HORZ_PIXELS/32);
        osdDMAMemset(osdPixLineBlack, &osdZero, OSD_HORZ_PIXELS/32);
    }

    // white SPI DMA
    DMA2_Stream5->NDTR = OSD_HORZ_PIXELS/8+2;    // peripheral data size (byte)
    DMA2_Stream5->CR |= (uint32_t)DMA_SxCR_EN;

    // black SPI DMA
    DMA1_Stream7->NDTR = OSD_HORZ_PIXELS/8+2;    // peripheral data size (byte)
    DMA1_Stream7->CR |= (uint32_t)DMA_SxCR_EN;
}

void osdDriverInit(void) {
//tp = digitalInit(GPIOA, GPIO_Pin_6);
    osdInitDac();
    osdDMAMemCpyInit();
    osdInitSync();
    osdInitPixClock();
    osdInitPixShift();

    osdData.shadow = 1;
}
