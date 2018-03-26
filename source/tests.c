/*
 * tests.c
 *
 *  Created on: Mar 26, 2018
 *      Author: cvance
 */

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

/* Application Include */
#include "pelicannon-k66f-firmware.h"
#include "ninedof.h"
#include "tk1.h"
#include "dualhbridge.h"

#include "tests.h"

#ifdef MOTOR_TEST
void Motor_Test_Task(void* pvParam){
	for(;;){
		DualHBridge_Step(200);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		DualHBridge_Step(-200);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}
#endif
