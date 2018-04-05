/*
 * tk1.c
 *
 *  Created on: Mar 25, 2018
 *      Author: cvance
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_uart.h"
#include "fsl_debug_console.h"
#include "MK66F18.h"

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

/* Application Include */
#include "pelicannon-k66f-firmware.h"
#include "tk1.h"
#include "dualhbridge.h"
#include "ninedof.h"

/* Synchronization Primitives */
#define TK1_EVENT_MOTOR_CMD (1<<0)
static EventGroupHandle_t tk1_motor_event_group = NULL;
#define TK1_EVENT_9DOF_DRDY (1<<0)
static EventGroupHandle_t tk1_ninedof_event_group = NULL;


static volatile int tk1_uart_buffer_size = 0;
static char tk1_uart_buffer[TK1_UART_RX_BUFFER];

void UART1_RX_TX_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;
    char data;

    /* If new data arrived. */
    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(TK1_UART))
    {
        data = UART_ReadByte(TK1_UART);

        //Buffer overrun prevention
        if(tk1_uart_buffer_size == TK1_UART_RX_BUFFER){
        	tk1_uart_buffer_size = 0;
        }

        //Store byte
        if (tk1_uart_buffer_size < TK1_UART_RX_BUFFER){
                	tk1_uart_buffer[tk1_uart_buffer_size] = data;
                	tk1_uart_buffer_size++;
        }

        //End of command marker
        if(data == ';'){
        	//Null terminator
        	tk1_uart_buffer[tk1_uart_buffer_size] = 0;

            //Notify the task to handle the command
            xHigherPriorityTaskWoken = pdFALSE;
            xResult = xEventGroupSetBitsFromISR(tk1_motor_event_group, TK1_EVENT_MOTOR_CMD, &xHigherPriorityTaskWoken);
            if( xResult != pdFAIL )
              {
                  /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
                  switch should be requested.  The macro used is port specific and will
                  be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
                  the documentation page for the port being used. */
                  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
              }

        }
    }
}


void TK1_Init(){
    uart_config_t config;

	tk1_motor_event_group = xEventGroupCreate();
	tk1_ninedof_event_group = xEventGroupCreate();


    UART_GetDefaultConfig(&config);
    config.baudRate_Bps = TK1_UART_BAUD;
    config.enableTx = true;
    config.enableRx = true;

    UART_Init(TK1_UART, &config, TK1_UART_CLK_FREQ);

    UART_EnableInterrupts(TK1_UART, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
    EnableIRQ(TK1_UART_IRQn);

}

void TK1_NineDof_DataReady(){
	xEventGroupSetBits(tk1_ninedof_event_group, TK1_EVENT_9DOF_DRDY);
}

void TK1_Ninedof_Task(void *pvParameters){
#ifdef NINEDOF_DEBUG
	int debug_output_counter = 0;
#endif
	fxas21002_gyrodata_t g;
	fxos8700_accelmagdata_t xm;
    EventBits_t event_set;
	char data_send_buffer[128];

	for(;;){
		event_set = xEventGroupWaitBits(tk1_ninedof_event_group,
										TK1_EVENT_9DOF_DRDY,
										 pdTRUE,
										 pdFALSE,
										 portMAX_DELAY);

		if (event_set & TK1_EVENT_9DOF_DRDY){
			Ninedof_CopyData(&g, &xm);

			sprintf(data_send_buffer, "G:%d,%d,%d\r\nA:%d,%d,%d\r\nM:%d,%d,%d\r\n", g.gyro[0], g.gyro[1], g.gyro[2],
					xm.accel[0], xm.accel[1], xm.accel[2], xm.mag[0], xm.mag[1], xm.mag[2]);

#ifdef NINEDOF_DEBUG
			debug_output_counter++;
			if (debug_output_counter >= 25){
	            PRINTF(data_send_buffer);
				debug_output_counter = 0;
			}
#endif
			UART_WriteBlocking(TK1_UART, data_send_buffer, strlen(data_send_buffer));

		}

	}
}

void TK1_Motor_Task(void *pvParameters){

    EventBits_t event_set;

	for(;;){
		event_set = xEventGroupWaitBits(tk1_motor_event_group,
										TK1_EVENT_MOTOR_CMD,
										 pdTRUE,
										 pdFALSE,
										 portMAX_DELAY);

		if (event_set & TK1_EVENT_MOTOR_CMD){

			if(strcmp(tk1_uart_buffer, "l;") == 0){
				DualHBridge_Step(-DUALHBRIDGE_STEPS_PER_ROTATION);
			}else if(strcmp(tk1_uart_buffer, "r;") == 0){
				DualHBridge_Step(DUALHBRIDGE_STEPS_PER_ROTATION);
			}else if(strcmp(tk1_uart_buffer, "s;") == 0){
				DualHBridge_Abort();
			}

			tk1_uart_buffer_size = 0;
		}


	}
}
