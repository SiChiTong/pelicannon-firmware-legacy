/**
 * @file	tk1.c
 * @author	Carroll Vance
 * @brief	Jetson TK1 Communication Module
 */
#include <stdio.h>
#include <stdlib.h>
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
/*! @brief	Event which is set on new motor comands being received*/
static EventGroupHandle_t tk1_motor_event_group = NULL;
#define TK1_EVENT_9DOF_DRDY (1<<0)
/*! @brief	Event which is set on new ninedof data being ready*/
static EventGroupHandle_t tk1_ninedof_event_group = NULL;

/*! @brief	Amount of data in tk1_uart_buffer*/
static volatile int tk1_uart_buffer_size = 0;
/*! @brief Buffers received data from the TK1 UART*/
static char tk1_uart_buffer[TK1_UART_RX_BUFFER];

/**
 * @brief Handles IRQ from UART1 triggered by RX from TK1 sending us motor commands
 * @usage Should only be called by the NVIC driver
 */
void UART1_RX_TX_IRQHandler(void) {
	BaseType_t xHigherPriorityTaskWoken, xResult;
	char data;

	/* If new data arrived. */
	if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag)
			& UART_GetStatusFlags(TK1_UART)) {
		data = UART_ReadByte(TK1_UART);

		//Buffer overrun prevention
		if (tk1_uart_buffer_size == TK1_UART_RX_BUFFER) {
			memset(tk1_uart_buffer, 0, sizeof(tk1_uart_buffer));
			tk1_uart_buffer_size = 0;
		}

		//Store byte
		if (tk1_uart_buffer_size < TK1_UART_RX_BUFFER) {
			tk1_uart_buffer[tk1_uart_buffer_size] = data;
			tk1_uart_buffer_size++;
		}

		//End of command marker
		if (data == ';') {
			//Null terminator
			tk1_uart_buffer[tk1_uart_buffer_size] = 0;

			//Notify the task to handle the command
			xHigherPriorityTaskWoken = pdFALSE;
			xResult = xEventGroupSetBitsFromISR(tk1_motor_event_group,
					TK1_EVENT_MOTOR_CMD, &xHigherPriorityTaskWoken);
			if (xResult != pdFAIL) {
				/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
				 switch should be requested.  The macro used is port specific and will
				 be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
				 the documentation page for the port being used. */
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			}

		}
	}
}

/**
 * @brief Initializes the TK1 functional group
 * @usage Should only be called by the application entry point
 */
void TK1_Init() {
	uart_config_t config;
	status_t status;

	tk1_motor_event_group = xEventGroupCreate();
	tk1_ninedof_event_group = xEventGroupCreate();

	ASSERT_NOT_MSG(!tk1_motor_event_group || !tk1_ninedof_event_group, "Failed to initialize tk1 synchronization primitives\r\n");

	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = TK1_UART_BAUD;
	config.enableTx = true;
	config.enableRx = true;

	status = UART_Init(TK1_UART, &config, TK1_UART_CLK_FREQ);
	ASSERT_NOT_MSG(status != kStatus_Success, "Failed to initialize TK1_UART\r\n");

	UART_EnableInterrupts(TK1_UART,
			kUART_RxDataRegFullInterruptEnable
					| kUART_RxOverrunInterruptEnable);
	status = EnableIRQ(TK1_UART_IRQn);
	ASSERT_NOT_MSG(status != kStatus_Success, "Failed to enable TK1_UART_IRQn");

}

/**
 * @brief Called whenever new mag/gyro/accel data is available
 * @usage Should only be called by Ninedof_Task in ninedof.c
 */
void TK1_NineDof_DataReady() {
	xEventGroupSetBits(tk1_ninedof_event_group, TK1_EVENT_9DOF_DRDY);
}

/**
 * @brief When new mag/gyro/accel data is recieved send it via UART to the TK1
 * @usage Called by FreeRTOS after xTaskCreate
 * @param pvParameters Unused
 */
void TK1_Ninedof_Task(void *pvParameters) {
#if NINEDOF_GPIO_DEBUG_TIMING
	int debug_output_counter = 0;
#endif
	fxas21002_gyrodata_t g;
	fxos8700_accelmagdata_t xm;
	EventBits_t event_set;
	uint8_t data_send_buffer[128];

	for (;;) {
		event_set = xEventGroupWaitBits(tk1_ninedof_event_group,
		TK1_EVENT_9DOF_DRDY,
		pdTRUE,
		pdFALSE,
		portMAX_DELAY);

		if (event_set & TK1_EVENT_9DOF_DRDY) {
			Ninedof_CopyData(&g, &xm);

#if NINEDOF_GPIO_DEBUG_TIMING
			sprintf(data_send_buffer, "G:%d,%d,%d\r\nA:%d,%d,%d\r\nM:%d,%d,%d\r\n", g.gyro[0], g.gyro[1], g.gyro[2],
					xm.accel[0], xm.accel[1], xm.accel[2], xm.mag[0], xm.mag[1], xm.mag[2]);

			debug_output_counter++;
			if (debug_output_counter >= 25) {
				PRINTF(data_send_buffer);
				debug_output_counter = 0;
			}
#endif

			data_send_buffer[0] = 0xDE;
			data_send_buffer[1] = 0xAD;
			data_send_buffer[2] = 0xBE;
			data_send_buffer[3] = 0xEF;

			memcpy(data_send_buffer + 4, g.gyro, 6);
			memcpy(data_send_buffer + 10, xm.accel, 6);
			memcpy(data_send_buffer + 16, xm.mag, 6);

			UART_WriteBlocking(TK1_UART, data_send_buffer, 22);

		}

	}
}

/**
 * @brief Handles motor commands from the TK1
 * @usage Called by FreeRTOS after xTaskCreate
 * @param pvParameters Unused
 */
void TK1_Motor_Task(void *pvParameters) {

	EventBits_t event_set;

	for (;;) {
		event_set = xEventGroupWaitBits(tk1_motor_event_group,
		TK1_EVENT_MOTOR_CMD,
		pdTRUE,
		pdFALSE,
		portMAX_DELAY);

		if (event_set & TK1_EVENT_MOTOR_CMD) {

			switch (tk1_uart_buffer[0]) {

			/*
			 * Step Command
			 * Message Format: S:+-123;
			 * Where +-123 is the number of steps
			 */
			case 'S':
				if (tk1_uart_buffer[1] != ':')
					break;

				//Scan for end
				for (int i = 2; i < sizeof(tk1_uart_buffer); i++) {
					if (tk1_uart_buffer[i] == ';') {
						//Null terminate for atoi
						tk1_uart_buffer[i] = 0x00;
						int steps = atoi(&tk1_uart_buffer[2]);
						DualHBridge_Step(steps);
						break;
					}
				}

				break;

			/*
			 * Abort Command
			 * Message Format: A;
			 */
			case 'A':
				if (tk1_uart_buffer[1] == ';')
					DualHBridge_Abort();
				break;
			}

			//Clear the UART Buffer
			memset(tk1_uart_buffer, 0, sizeof(tk1_uart_buffer));
			tk1_uart_buffer_size = 0;
		}

	}
}
