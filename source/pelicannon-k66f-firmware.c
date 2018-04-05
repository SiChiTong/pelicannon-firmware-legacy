/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    pelicannon-k66f-firmware.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK66F18.h"
#include "fsl_debug_console.h"

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


/*
 * @brief   Application entry point.
 */
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

	NineDoF_InitPins();
	JetsonTK1_InitPins();
	DualHBridge_InitPins();

    /* Lower GPIO Port ISR priorities to minimum level below FreeRTOS task level */
    NVIC_SetPriority(PORTA_IRQn, 0x02);
    NVIC_SetPriority(PORTC_IRQn, 0x02);
    NVIC_SetPriority(TK1_UART_IRQn, 0x02);

    /* Initialize functionality groups before starting tasks to avoid race conditions */
    NineDoF_Init();
    DualHBridge_Init();
    TK1_Init();

    /* Create tasks */
    if (xTaskCreate(Ninedof_Task, "Ninedof_Task", 1024, NULL, configMAX_PRIORITIES-1, 0) != pdPASS){
    	PRINTF("Failed to create Ninedof_Task\r\n");
    	while(1);
    }

    if (xTaskCreate(DualHBridge_Task, "DualHBridge_Task", 1024, NULL, configMAX_PRIORITIES-2, 0) != pdPASS){
    	PRINTF("Failed to create DualHBridge_Task\r\n");
    	while(1);
    }

    if (xTaskCreate(TK1_Motor_Task, "TK1_Motor_Task", 1024, NULL, configMAX_PRIORITIES-1, 0) != pdPASS){
    	PRINTF("Failed to create TK1_Motor_Taskk\r\n");
    	while(1);
    }

    if (xTaskCreate(TK1_Ninedof_Task, "TK1_Ninedof_Task", 1024, NULL, configMAX_PRIORITIES-1, 0) != pdPASS){
    	PRINTF("Failed to create TK1_Ninedof_Task\r\n");
    	while(1);
    }

    /* Tests */
#ifdef MOTOR_TEST
    if (xTaskCreate(Motor_Test_Task, "Motor_Test_Task", 1024, NULL, configMAX_PRIORITIES-1, 0) != pdPASS){
    	PRINTF("Failed to create Motor_Test_Task\r\n");
    	while(1);
    }
#endif

    vTaskStartScheduler();
    while(1);
}


