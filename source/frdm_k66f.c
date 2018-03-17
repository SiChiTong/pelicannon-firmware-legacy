/*
 * cmsis.c
 *
 *  Created on: Mar 16, 2018
 *      Author: cvance
 */

#include "frdm_k66f.h"
#include "fsl_clock.h"
#include "pin_mux.h"

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

uint32_t I2C0_GetFreq(void){return CLOCK_GetFreq(I2C0_CLK_SRC);}
void I2C0_InitPins(void){}
void I2C0_DeinitPins(void){}
