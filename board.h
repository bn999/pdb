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

#ifndef _board_h
#define _board_h

#define ANALOG_VIN_RTOP         10.0f
#define ANALOG_VIN_RBOT         1.0f
#define ANALOG_AMP_SENSITIVITY	0.0267f	    // volts/A
//#define ANALOG_AMP_OFFSET_FACT	0.11645f

#define	ANALOG_CHANNEL_VIN	    ADC_Channel_10
#define	ANALOG_CHANNEL_AMP	    ADC_Channel_11

#define ANALOG_DMA_STREAM       DMA2_Stream4
#define ANALOG_DMA_CHANNEL      DMA_Channel_0
#define ANALOG_DMA_FLAGS        (DMA_IT_TEIF4 | DMA_IT_DMEIF4 | DMA_IT_FEIF4 | DMA_IT_TCIF4 | DMA_IT_HTIF4)
#define ANALOG_DMA_TC_FLAG      DMA_IT_TCIF4
#define ANALOG_DMA_ISR          DMA2->HISR
#define ANALOG_DMA_IRQ          DMA2_Stream4_IRQn
#define ANALOG_DMA_HANDLER      DMA2_Stream4_IRQHandler


#define SERIAL_UART1_PORT	GPIOA
#define SERIAL_UART1_RX_PIN	GPIO_Pin_10
#define SERIAL_UART1_TX_PIN	GPIO_Pin_9
//#define SERIAL_UART1_CTS_PIN	GPIO_Pin_11
//#define SERIAL_UART1_RTS_PIN	GPIO_Pin_12
#define SERIAL_UART1_RX_SOURCE	GPIO_PinSource10
#define SERIAL_UART1_TX_SOURCE	GPIO_PinSource9
//#define SERIAL_UART1_CTS_SOURCE	GPIO_PinSource11
//#define SERIAL_UART1_RTS_SOURCE	GPIO_PinSource12
#define SERIAL_UART1_RX_DMA_ST	DMA2_Stream2
#define SERIAL_UART1_TX_DMA_ST	DMA2_Stream7
#define SERIAL_UART1_RX_DMA_CH	DMA_Channel_4
#define SERIAL_UART1_TX_DMA_CH	DMA_Channel_4
#define SERIAL_UART1_TX_DMA_IT	DMA2_Stream7_IRQHandler
#define SERIAL_UART1_TX_IRQn	DMA2_Stream7_IRQn
#define SERIAL_UART1_RX_TC_FLAG	DMA_FLAG_TCIF2
#define SERIAL_UART1_RX_HT_FLAG	DMA_FLAG_HTIF2
#define SERIAL_UART1_RX_TE_FLAG	DMA_FLAG_TEIF2
#define SERIAL_UART1_RX_DM_FLAG	DMA_FLAG_DMEIF2
#define SERIAL_UART1_RX_FE_FLAG	DMA_FLAG_FEIF2
#define SERIAL_UART1_TX_TC_FLAG	DMA_FLAG_TCIF7
#define SERIAL_UART1_TX_HT_FLAG	DMA_FLAG_HTIF7
#define SERIAL_UART1_TX_TE_FLAG	DMA_FLAG_TEIF7
#define SERIAL_UART1_TX_DM_FLAG	DMA_FLAG_DMEIF7
#define SERIAL_UART1_TX_FE_FLAG	DMA_FLAG_FEIF7

#define SERIAL_UART2_PORT	GPIOA
#define SERIAL_UART2_RX_PIN	GPIO_Pin_3
#define SERIAL_UART2_TX_PIN	GPIO_Pin_2
//#define SERIAL_UART2_CTS_PIN	GPIO_Pin_3
//#define SERIAL_UART2_RTS_PIN	GPIO_Pin_4
#define SERIAL_UART2_RX_SOURCE	GPIO_PinSource3
#define SERIAL_UART2_TX_SOURCE	GPIO_PinSource2
//#define SERIAL_UART2_CTS_SOURCE	GPIO_PinSource3
//#define SERIAL_UART2_RTS_SOURCE	GPIO_PinSource4
#define SERIAL_UART2_RX_DMA_ST	DMA1_Stream5
#define SERIAL_UART2_TX_DMA_ST	DMA1_Stream6
#define SERIAL_UART2_RX_DMA_CH	DMA_Channel_4
#define SERIAL_UART2_TX_DMA_CH	DMA_Channel_4
#define SERIAL_UART2_TX_DMA_IT	DMA1_Stream6_IRQHandler
#define SERIAL_UART2_TX_IRQn	DMA1_Stream6_IRQn
#define SERIAL_UART2_RX_TC_FLAG	DMA_FLAG_TCIF5
#define SERIAL_UART2_RX_HT_FLAG	DMA_FLAG_HTIF5
#define SERIAL_UART2_RX_TE_FLAG	DMA_FLAG_TEIF5
#define SERIAL_UART2_RX_DM_FLAG	DMA_FLAG_DMEIF5
#define SERIAL_UART2_RX_FE_FLAG	DMA_FLAG_FEIF5
#define SERIAL_UART2_TX_TC_FLAG	DMA_FLAG_TCIF6
#define SERIAL_UART2_TX_HT_FLAG	DMA_FLAG_HTIF6
#define SERIAL_UART2_TX_TE_FLAG	DMA_FLAG_TEIF6
#define SERIAL_UART2_TX_DM_FLAG	DMA_FLAG_DMEIF6
#define SERIAL_UART2_TX_FE_FLAG	DMA_FLAG_FEIF6

#define SERIAL_UART5_PORT_TX	GPIOC
#define SERIAL_UART5_PORT_RX	GPIOD
#define SERIAL_UART5_TX_PIN	GPIO_Pin_12
#define SERIAL_UART5_RX_PIN	GPIO_Pin_2
#define SERIAL_UART5_TX_SOURCE	GPIO_PinSource12
#define SERIAL_UART5_RX_SOURCE	GPIO_PinSource2
#define SERIAL_UART5_RX_DMA_ST	DMA1_Stream0
#define SERIAL_UART5_TX_DMA_ST	DMA1_Stream7
#define SERIAL_UART5_RX_DMA_CH	DMA_Channel_4
#define SERIAL_UART5_TX_DMA_CH	DMA_Channel_4
#define SERIAL_UART5_TX_DMA_IT	DMA1_Stream7_IRQHandler
#define SERIAL_UART5_TX_IRQn	DMA1_Stream7_IRQn
#define SERIAL_UART5_RX_TC_FLAG	DMA_FLAG_TCIF0
#define SERIAL_UART5_RX_HT_FLAG	DMA_FLAG_HTIF0
#define SERIAL_UART5_RX_TE_FLAG	DMA_FLAG_TEIF0
#define SERIAL_UART5_RX_DM_FLAG	DMA_FLAG_DMEIF0
#define SERIAL_UART5_RX_FE_FLAG	DMA_FLAG_FEIF0
#define SERIAL_UART5_TX_TC_FLAG	DMA_FLAG_TCIF7
#define SERIAL_UART5_TX_HT_FLAG	DMA_FLAG_HTIF7
#define SERIAL_UART5_TX_TE_FLAG	DMA_FLAG_TEIF7
#define SERIAL_UART5_TX_DM_FLAG	DMA_FLAG_DMEIF7
#define SERIAL_UART5_TX_FE_FLAG	DMA_FLAG_FEIF7

#define SUPERVISOR_READY_PORT	GPIOA
#define SUPERVISOR_READY_PIN	GPIO_Pin_15

enum pwmPorts {
    PWM_1 = 0,
    PWM_2,
    PWM_3,
    PWM_4,
    PWM_5,
    PWM_6,
    PWM_7,
    PWM_8,
    PWM_9,
    PWM_10,
    PWM_11,
    PWM_12,
    PWM_NUM_PORTS
};

#define PPM_PWM_CHANNEL		8	// which PWM channel to use for PPM capture

#define PWM_TIMERS  const TIM_TypeDef *pwmTimers[] = { \
    TIM3, \
    TIM3, \
    TIM3, \
    TIM3, \
    TIM4, \
    TIM4, \
    TIM12, \
    TIM12, \
    TIM8, \
    TIM8, \
    TIM8, \
    TIM8 \
};

#define	PWM_AFS	    const uint8_t pwmAFs[] = { \
    GPIO_AF_TIM3, \
    GPIO_AF_TIM3, \
    GPIO_AF_TIM3, \
    GPIO_AF_TIM3, \
    GPIO_AF_TIM4, \
    GPIO_AF_TIM4, \
    GPIO_AF_TIM12, \
    GPIO_AF_TIM12, \
    GPIO_AF_TIM8, \
    GPIO_AF_TIM8, \
    GPIO_AF_TIM8, \
    GPIO_AF_TIM8 \
};

#define PWM_PORTS   const GPIO_TypeDef *pwmPorts[] = { \
    GPIOA, \
    GPIOA, \
    GPIOB, \
    GPIOB, \
    GPIOB, \
    GPIOB, \
    GPIOB, \
    GPIOB, \
    GPIOC, \
    GPIOC, \
    GPIOC, \
    GPIOC \
};

#define PWM_PINS    const uint32_t pwmPins[] = { \
    GPIO_Pin_6, \
    GPIO_Pin_7, \
    GPIO_Pin_0, \
    GPIO_Pin_1, \
    GPIO_Pin_8, \
    GPIO_Pin_9, \
    GPIO_Pin_14, \
    GPIO_Pin_15, \
    GPIO_Pin_6, \
    GPIO_Pin_7, \
    GPIO_Pin_8, \
    GPIO_Pin_9 \
};

#define PWM_PINSOURCES	const uint16_t pwmPinSources[] = { \
    GPIO_PinSource6, \
    GPIO_PinSource7, \
    GPIO_PinSource0, \
    GPIO_PinSource1, \
    GPIO_PinSource8, \
    GPIO_PinSource9, \
    GPIO_PinSource14, \
    GPIO_PinSource15, \
    GPIO_PinSource6, \
    GPIO_PinSource7, \
    GPIO_PinSource8, \
    GPIO_PinSource9, \
};

#define PWM_TIMERCHANNELS   const uint8_t pwmTimerChannels[] = { \
    TIM_Channel_1, \
    TIM_Channel_2, \
    TIM_Channel_3, \
    TIM_Channel_4, \
    TIM_Channel_3, \
    TIM_Channel_4, \
    TIM_Channel_1, \
    TIM_Channel_2, \
    TIM_Channel_1, \
    TIM_Channel_2, \
    TIM_Channel_3, \
    TIM_Channel_4 \
};

#define PWM_BDTRS   const uint8_t pwmBDTRs[] = { \
    0, \
    0, \
    0, \
    0, \
    0, \
    0, \
    0, \
    0, \
    1, \
    1, \
    1, \
    1 \
};

#define PWM_CLOCKS  const uint32_t pwmClocks[] = { \
    84000000, \
    84000000, \
    84000000, \
    84000000, \
    84000000, \
    84000000, \
    84000000, \
    84000000, \
    168000000, \
    168000000, \
    168000000, \
    168000000 \
};

#define PWM_IC_IRQS  const uint8_t pwmIcIrqChannels[] = { \
    TIM3_IRQn, \
    TIM3_IRQn, \
    TIM3_IRQn, \
    TIM3_IRQn, \
    TIM4_IRQn, \
    TIM4_IRQn, \
    TIM8_BRK_TIM12_IRQn, \
    TIM8_BRK_TIM12_IRQn, \
    TIM8_CC_IRQn, \
    TIM8_CC_IRQn, \
    TIM8_CC_IRQn, \
    TIM8_CC_IRQn \
};

#define PWM_IRQ_TIM3_CH1	0
#define PWM_IRQ_TIM3_CH2	1
#define PWM_IRQ_TIM3_CH3	2
#define PWM_IRQ_TIM3_CH4	3
#define PWM_IRQ_TIM4_CH3	4
#define PWM_IRQ_TIM4_CH4	5
#define PWM_IRQ_TIM12_CH1	6
#define PWM_IRQ_TIM12_CH2	7
#define PWM_IRQ_TIM8_CH1	8
#define PWM_IRQ_TIM8_CH2	9
#define PWM_IRQ_TIM8_CH3	10
#define PWM_IRQ_TIM8_CH4	11

#define PWM_PRESCALE		1000000

#endif