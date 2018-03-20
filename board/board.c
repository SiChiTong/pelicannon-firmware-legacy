/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include "fsl_device_registers.h"
#include "board.h"
#include "fsl_clock.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

// Mag/Accel & Gyro Interrupt GPIO
gpioHandleKSDK_t GPIO_PTA29 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 29,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA28 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 28,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTC17 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 17,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

gpioHandleKSDK_t GPIO_PTC13 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 13,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};


// Header 1 Row 2 GPIO
gpioHandleKSDK_t GPIO_PTA27 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 27,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA26 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 26,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA4 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 4,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA6 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 6,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA7 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 7,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA8 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 8,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA9 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 9,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTA1 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 1,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

//Header 4 Row 2 GPIO
gpioHandleKSDK_t GPIO_PTA25 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 25,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_PTC2 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 2,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

gpioHandleKSDK_t GPIO_PTC5 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 5,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

gpioHandleKSDK_t GPIO_PTC12 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 12,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

/*******************************************************************************
 * Code
 ******************************************************************************/

/* Initialize debug console. */
void BOARD_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq = BOARD_DEBUG_UART_CLK_FREQ;
    DbgConsole_Init(BOARD_DEBUG_UART_BASEADDR, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}

uint32_t I2C0_GetFreq(void){return CLOCK_GetFreq(I2C0_CLK_SRC);}
void I2C0_InitPins(void){}
void I2C0_DeinitPins(void){}
