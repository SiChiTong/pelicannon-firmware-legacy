/**
 * @file	dualhbridge.c
 * @author	Carroll Vance
 * @brief	Controls dual h-bridge to drive a stepper motor using four GPIOs
 */
#include <stdio.h>
#include <stdlib.h>
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
/*!@brief	Wakes up the DualHBridge_Task when stepping is needed*/
#define DUALHBRIDGE_EVENT_WAKEUP (1<<0)
/*! @brief	Set target position to the current one, stopping the motor*/
#define DUALHBRIDGE_EVENT_ABORT (1<<1)

/*! @brief	Event used to wake the motor task to new commands*/
static EventGroupHandle_t dualhbridge_event_group = NULL;
/*! @brief	Locks access to step variables between tasks*/
static SemaphoreHandle_t dualhbridge_data_mutex = NULL;

/* Variables */
/*! @brief	Step target relative to initialized zero position*/
static int dualhbridge_target_steps = 0;
/*! @brief	Current number of steps from initialized zero position*/
static int dualhbridge_current_steps = 0;
/*! @brief	Delay in ticks between each step*/
static TickType_t dualhbridge_stepdelay;

/**
 * @brief	Sets the RPM of the motor
 * @usage	Can be called from any task
 * @param	RPM Rotations Per Minute
 */
void DualHBridge_SetRPM(int RPM) {
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);

	float steps_per_ms = (RPM * STEPPER_MOTOR_STEPS_PER_ROTATION)
			/ (60.0 * 1000.0);
	float steps_ms_delay = (1.0 / steps_per_ms);

	dualhbridge_stepdelay = (TickType_t) (ceil(
			steps_ms_delay / portTICK_PERIOD_MS));
	if (dualhbridge_stepdelay == 0)
		dualhbridge_stepdelay = 1;

	xSemaphoreGive(dualhbridge_data_mutex);
}

/**
 * @brief	Gets the current position in steps relative to initialized zero position
 * @usage	Can be called from any task
 * @return	Returns the number of steps from the initialized zero position
 */
int DualHBridge_GetPosition() {
	return dualhbridge_current_steps;
}

/**
 * @brief	Gets the number of steps left to turn
 * @usage	Can be called from any task
 * @return	Returns the number of steps left to complete
 */
int DualHBridge_StepsLeft() {
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	int steps_left = dualhbridge_target_steps - dualhbridge_current_steps;
	xSemaphoreGive(dualhbridge_data_mutex);
	return steps_left;
}

/**
 * @brief	Steps the motor positive or negative steps
 * @usage	Can be called from any task
 * @param	steps Number of steps to move
 */
void DualHBridge_Step(int steps) {
	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	dualhbridge_target_steps += steps;
	xSemaphoreGive(dualhbridge_data_mutex);

	xEventGroupSetBits(dualhbridge_event_group, DUALHBRIDGE_EVENT_WAKEUP);
}

/**
 * @brief	Stops the motor from moving after the current step completes
 * @usage	Can be called from any task
 */
void DualHBridge_Abort() {
	xEventGroupSetBits(dualhbridge_event_group, DUALHBRIDGE_EVENT_ABORT);

	xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
	dualhbridge_target_steps = dualhbridge_current_steps;
	xSemaphoreGive(dualhbridge_data_mutex);
}

/**
 * @brief	Initialize the dual h-bridge functionality group
 * @usage	Should only be called from application entry point
 */
void DualHBridge_Init() {

	dualhbridge_event_group = xEventGroupCreate();
	dualhbridge_data_mutex = xSemaphoreCreateMutex();
	ASSERT_NOT_MSG(!dualhbridge_event_group || !dualhbridge_data_mutex, "Failed to initialize DualHBridge synchronization primitives\r\n");

	/* Init GPIOs */
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	gpioDriver->pin_init(&GPIO_MOTOR_A1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
	gpioDriver->pin_init(&GPIO_MOTOR_A2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
	gpioDriver->pin_init(&GPIO_MOTOR_B1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
	gpioDriver->pin_init(&GPIO_MOTOR_B2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);

	DualHBridge_SetRPM(STEPPER_MOTOR_RPM);

}

/**
 * @brief	Steps the motor a single step based on the step number
 * @usage	Should only be called by DualHBridge_Task
 * @param	step_number Next step number, from 0 to 3
 */
void DualHBridge__Step(int step_number) {
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	switch (step_number) {
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

/**
 * @brief	Handles requests to move step the motor
 * @usage	Created through xTaskCreate
 * @param	pvParameters Unused
 */
void DualHBridge_Task(void *pvParameters) {

	EventBits_t event_set;

	for (;;) {
		event_set = xEventGroupWaitBits(dualhbridge_event_group,
		DUALHBRIDGE_EVENT_WAKEUP | DUALHBRIDGE_EVENT_ABORT,
		pdTRUE,
		pdFALSE,
		portMAX_DELAY);

		if (event_set & DUALHBRIDGE_EVENT_WAKEUP) {

			while (dualhbridge_target_steps != dualhbridge_current_steps) {

				//Check for abort flag before stepping
				event_set = xEventGroupWaitBits(dualhbridge_event_group,
				DUALHBRIDGE_EVENT_ABORT,
				pdTRUE,
				pdFALSE, 0);

				if (event_set & DUALHBRIDGE_EVENT_ABORT)
					break;

				xSemaphoreTake(dualhbridge_data_mutex, portMAX_DELAY);
				if (dualhbridge_current_steps != dualhbridge_target_steps) {

					if (dualhbridge_current_steps < dualhbridge_target_steps) {
						//STEP Forwards
						DualHBridge__Step(
								abs(dualhbridge_current_steps + 1) % 4);
						dualhbridge_current_steps++;
					} else if (dualhbridge_current_steps
							> dualhbridge_target_steps) {
						//STEP Back
						DualHBridge__Step(
								abs(dualhbridge_current_steps - 1) % 4);
						dualhbridge_current_steps--;
					}

				} else {
					xSemaphoreGive(dualhbridge_data_mutex);
					break;
				}
				xSemaphoreGive(dualhbridge_data_mutex);

				//Delay till next step
				event_set = xEventGroupWaitBits(dualhbridge_event_group,
				DUALHBRIDGE_EVENT_ABORT,
				pdTRUE,
				pdFALSE, dualhbridge_stepdelay);

				if (event_set & DUALHBRIDGE_EVENT_ABORT)
					break;

			}

		}

	}
}
