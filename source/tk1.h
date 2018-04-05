/*
 * tk1.h
 *
 *  Created on: Mar 25, 2018
 *      Author: cvance
 */

#ifndef TK1_H_
#define TK1_H_

#include "clock_config.h"
#include "fsl_gpio.h"
#include "fsl_uart.h"

#include "MK66F18.h"

#define TK1_UART UART1
#define TK1_UART_IRQn UART1_RX_TX_IRQn
#define TK1_UART_IRQ_HANDLER UART1_RX_TX_IRQHandler
#define TK1_UART_BAUD 115200U
#define TK1_UART_CLK_FREQ CLOCK_GetFreq(SYS_CLK)
#define TK1_UART_RX_BUFFER 32

void TK1_Motor_Task(void *pvParameters);
void TK1_Ninedof_Task(void *pvParameters);
void TK1_Init();
void TK1_NineDof_DataReady();

#endif /* TK1_H_ */
