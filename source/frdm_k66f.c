/*
 * cmsis.c
 *
 *  Created on: Mar 16, 2018
 *      Author: cvance
 */

#include "frdm_k66f.h"
#include "fsl_clock.h"

gpioHandleKSDK_t FXAS21002_INT1 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 29,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t FXAS21002_INT2 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 28,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t FXOS8700_INT1 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 17,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

gpioHandleKSDK_t FXOS8700_INT2 = {.base = GPIOC,
                         .portBase = PORTC,
                         .pinNumber = 13,
                         .mask = 1 << (0),
                         .irq = PORTC_IRQn,
                         .clockName = kCLOCK_PortC,
                         .portNumber = PORTC_NUM};

gpioHandleKSDK_t GPIO_DEBUG_1 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 4,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_DEBUG_2 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 26,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

gpioHandleKSDK_t GPIO_DEBUG_3 = {.base = GPIOA,
                         .portBase = PORTA,
                         .pinNumber = 27,
                         .mask = 1 << (0),
                         .irq = PORTA_IRQn,
                         .clockName = kCLOCK_PortA,
                         .portNumber = PORTA_NUM};

uint32_t I2C0_GetFreq(void){
	return CLOCK_GetFreq(I2C0_CLK_SRC);
}
void I2C0_InitPins(void){

}
void I2C0_DeinitPins(void){

}
