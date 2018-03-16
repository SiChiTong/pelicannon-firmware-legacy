/*
 * cmsis.c
 *
 *  Created on: Mar 16, 2018
 *      Author: cvance
 */

#include "frdm_k66f.h"
#include "fsl_clock.h"

uint32_t I2C0_GetFreq(void){
	return CLOCK_GetFreq(I2C0_CLK_SRC);
}
void I2C0_InitPins(void){

}
void I2C0_DeinitPins(void){

}
