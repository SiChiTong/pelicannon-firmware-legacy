/*
 * dualhbridge.c
 *
 *  Created on: Mar 25, 2018
 *      Author: cvance
 */

#include <stdio.h>
#include <math.h>
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
#define DUALHBRIDGE_EVENT_WAKEUP (1<<0)
#define DUALHBRIDGE_EVENT_ABORT (1<<1)

static EventGroupHandle_t dualhbridge_event_group = NULL;

static SemaphoreHandle_t dualhbridge_data_mutex = NULL;

/* Variables */
static int dualhbridge_target_steps = 0;
static int dualhbridge_current_steps = 0;
static TickType_t dualhbridge_stepdelay;

void DualHBridge_SetRPM(int RPM){
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);

	float steps_per_ms = (RPM * DUALHBRIDGE_STEPS_PER_ROTATION) / (60.0 * 1000.0);
	float steps_ms_delay = (1.0 / steps_per_ms);

	dualhbridge_stepdelay = (TickType_t) (ceil(steps_ms_delay / portTICK_PERIOD_MS));
	if (dualhbridge_stepdelay == 0)
		dualhbridge_stepdelay = 1;

	xSemaphoreGive(dualhbridge_data_mutex);
}

int DualHBridge_GetPosition(){
	return dualhbridge_current_steps;
}

int DualHBridge_StepsLeft(){
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	int steps_left = dualhbridge_target_steps - dualhbridge_current_steps;
	xSemaphoreGive(dualhbridge_data_mutex);
	return steps_left;
}

void DualHBridge_Step(int steps){
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	dualhbridge_target_steps += steps;
	xSemaphoreGive(dualhbridge_data_mutex);

	xEventGroupSetBits(dualhbridge_event_group, DUALHBRIDGE_EVENT_WAKEUP);
}

void DualHBridge_Abort(){
	xEventGroupSetBits(dualhbridge_event_group, DUALHBRIDGE_EVENT_ABORT);

	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	dualhbridge_target_steps = dualhbridge_current_steps;
	xSemaphoreGive(dualhbridge_data_mutex);
}

void DualHBridge_Init(){

	dualhbridge_event_group = xEventGroupCreate();
	dualhbridge_data_mutex = xSemaphoreCreateMutex();

	/* Init GPIOs */
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

    gpioDriver->pin_init(&GPIO_MOTOR_A1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    gpioDriver->pin_init(&GPIO_MOTOR_A2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    gpioDriver->pin_init(&GPIO_MOTOR_B1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    gpioDriver->pin_init(&GPIO_MOTOR_B2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);

	DualHBridge_SetRPM(DUALHBRIDGE_RPM);

}


void DualHBridge__Step(int stepNumber){
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	switch(stepNumber){
	case 0:
		gpioDriver->write_pin(&GPIO_MOTOR_A1, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_A2, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_B1, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_B2, 0);
		break;
	case 1:
		gpioDriver->write_pin(&GPIO_MOTOR_A1, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_A2, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_B1, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_B2, 0);
		break;
	case 2:
		gpioDriver->write_pin(&GPIO_MOTOR_A1, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_A2, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_B1, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_B2, 1);
		break;
	case 3:
		gpioDriver->write_pin(&GPIO_MOTOR_A1, 1);
		gpioDriver->write_pin(&GPIO_MOTOR_A2, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_B1, 0);
		gpioDriver->write_pin(&GPIO_MOTOR_B2, 1);
		break;
	default:
		ASSERT(pdFALSE);

	}
}

void DualHBridge_Task(void *pvParameters){

    EventBits_t event_set;

	for(;;){
		event_set = xEventGroupWaitBits(dualhbridge_event_group,
										DUALHBRIDGE_EVENT_WAKEUP | DUALHBRIDGE_EVENT_ABORT,
										 pdTRUE,
										 pdFALSE,
										 portMAX_DELAY);


		if (event_set & DUALHBRIDGE_EVENT_WAKEUP){

			while(dualhbridge_target_steps != dualhbridge_current_steps){

				//Check for abort flag before stepping
				event_set = xEventGroupWaitBits(dualhbridge_event_group,
												DUALHBRIDGE_EVENT_ABORT,
												 pdTRUE,
												 pdFALSE,
												 0);

				if (event_set & DUALHBRIDGE_EVENT_ABORT)
					break;

				xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
				if (dualhbridge_current_steps != dualhbridge_target_steps){

					if(dualhbridge_target_steps < dualhbridge_current_steps){
						//STEP Forwards
						DualHBridge__Step((dualhbridge_current_steps+1) % 4);
						dualhbridge_current_steps++;
					}else if(dualhbridge_target_steps > dualhbridge_current_steps){
						//STEP Back
						DualHBridge__Step((dualhbridge_current_steps-1) % 4);
						dualhbridge_current_steps--;
					}

				}else{
					xSemaphoreGive(dualhbridge_data_mutex);
					break;
				}
				xSemaphoreGive(dualhbridge_data_mutex);

				//Delay till next step
				event_set = xEventGroupWaitBits(dualhbridge_event_group,
												DUALHBRIDGE_EVENT_ABORT,
												 pdTRUE,
												 pdFALSE,
												 dualhbridge_stepdelay);

				if (event_set & DUALHBRIDGE_EVENT_ABORT)
					break;

			}

		} else if(event_set & DUALHBRIDGE_EVENT_ABORT){
			//Eat the event
		}

	}
}
