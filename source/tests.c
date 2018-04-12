/**
 * @file	tests.c
 * @author	Carroll Vance
 * @brief	Tests for various hardware components
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

#if MOTOR_TEST
/**
 * @brief Loops the motor through full rotations positive and negative
 * @usage Called by FreeRTOS after xTaskCreate
 * @param pvParam Unused
 */
void Motor_Test_Task(void* pvParam){
	for(;;){
		DualHBridge_Step(DUALHBRIDGE_STEPS_PER_ROTATION);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		DualHBridge_Step(-DUALHBRIDGE_STEPS_PER_ROTATION);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}
#endif
