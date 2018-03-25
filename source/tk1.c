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
#include "dualhbridge.h"

/* Synchronization Primitives */
#define TK1_EVENT_UART_DRDY (1<<0)
static EventGroupHandle_t tk1_event_group = NULL;


void TK1_Init(){
	tk1_event_group = xEventGroupCreate();
}

void TK1_Task(void *pvParameters){

}
