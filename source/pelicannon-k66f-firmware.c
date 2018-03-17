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

/* CMSIS Includes */
#include "Driver_I2C.h"
#include "frdm_k66f.h"

/* ISSDK Includes */
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "fxas21002_drv.h"
#include "fxos8700_drv.h"

/* Application Include */
#include "pelicannon-k66f-firmware.h"

/* Local Prototypes */
static void ninedof_task(void *pvParameters);
void Sensors_Init();

/* Synchronization Primitives */
#define EVENT_XM (1<<0)
#define EVENT_G (1<<1)
static EventGroupHandle_t ninedof_event_group = NULL;


/* Device Handles */
/* Accelerometer/magnometer use the same I2C bus as gyrometer */
ARM_DRIVER_I2C *I2Cdrv = &GYRO_I2C_DEVICE;
fxas21002_i2c_sensorhandle_t fxas21002Driver;
fxos8700_i2c_sensorhandle_t fxos8700Driver;

/* Device Configuration */
const registerwritelist_t fxos8700_Config_ISR[] = {
    /*! System and Control registers. */
    /*! Configure the FXOS8700 to 100Hz sampling rate. */
    {FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_DR_HYBRID_200_HZ, FXOS8700_CTRL_REG1_DR_MASK},
    {FXOS8700_CTRL_REG3, FXOS8700_CTRL_REG3_IPOL_ACTIVE_HIGH | FXOS8700_CTRL_REG3_PP_OD_PUSH_PULL,
     FXOS8700_CTRL_REG3_IPOL_MASK | FXOS8700_CTRL_REG3_PP_OD_MASK}, /*! Active High, Push-Pull */
    {FXOS8700_CTRL_REG4, FXOS8700_CTRL_REG4_INT_EN_DRDY_EN,
     FXOS8700_CTRL_REG4_INT_EN_DRDY_MASK}, /*! Data Ready Event. */
    {FXOS8700_CTRL_REG5, FXOS8700_CTRL_REG5_INT_CFG_DRDY_INT2, FXOS8700_CTRL_REG5_INT_CFG_DRDY_MASK}, /*! INT2 Pin  */
    /*! Configure the fxos8700 to Hybrid mode */
    {FXOS8700_M_CTRL_REG2, FXOS8700_M_CTRL_REG2_M_AUTOINC_HYBRID_MODE, FXOS8700_M_CTRL_REG2_M_AUTOINC_MASK},
    __END_WRITE_DATA__};

/*! Prepare the register write list to configure FXAS21002 in non-FIFO mode. */
const registerwritelist_t fxas21002_Config_ISR[] = {
    /*! Configure CTRL_REG1 register to put FXAS21002 to 100Hz sampling rate. */
    {FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_DR_100HZ, FXAS21002_CTRL_REG1_DR_MASK},
    /*! Configure CTRL_REG2 register to set interrupt configuration settings. */
    {FXAS21002_CTRL_REG2, FXAS21002_CTRL_REG2_IPOL_ACTIVE_HIGH | FXAS21002_CTRL_REG2_INT_EN_DRDY_ENABLE |
                              FXAS21002_CTRL_REG2_INT_CFG_DRDY_INT1,
     FXAS21002_CTRL_REG2_IPOL_MASK | FXAS21002_CTRL_REG2_INT_EN_DRDY_MASK | FXAS21002_CTRL_REG2_INT_CFG_DRDY_MASK},
    __END_WRITE_DATA__};

/*! Command definition to read the Accel and Mag Data */
#define XM_DATA_READ_SIZE 12
const registerreadlist_t fxos8700_Read[] = {{.readFrom = FXOS8700_OUT_X_MSB, .numBytes = XM_DATA_READ_SIZE},
                                                  __END_READ_DATA__};


/*! Prepare the register read list to read the raw gyro data from the FXAS21002. */
#define G_DATA_READ_SIZE 6
const registerreadlist_t fxas21002_Read[] = {
    {.readFrom = FXAS21002_OUT_X_MSB, .numBytes = G_DATA_READ_SIZE}, __END_READ_DATA__};

/* ISR Routines */
void FXAS21002_ISR(void *pUserData){
	xEventGroupSetBits(ninedof_event_group, EVENT_G);
}

void FXOS8700_ISR(void *pUserData){
	xEventGroupSetBits(ninedof_event_group, EVENT_XM);
}


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

    /* Initialize Synchronization Primitives */
    ninedof_event_group = xEventGroupCreate();

    /* Setup Interupts */
    GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

    gpioDriver->pin_init(&FXAS21002_INT1, GPIO_DIRECTION_IN, NULL, &FXAS21002_ISR, NULL);
    gpioDriver->pin_init(&FXOS8700_INT2, GPIO_DIRECTION_IN, NULL, &FXOS8700_ISR, NULL);

    //Configure I2C and Mag/Accel/Gyro
    Sensors_Init();

    /* Create tasks */
    if (xTaskCreate(ninedof_task, "ninedof_task", 1024, 0, configMAX_PRIORITIES-1, 0) != pdPASS){
    	PRINTF("\r\n Failed to create ninedof_task\r\n");
    	while(1);
    }

    vTaskStartScheduler();
    for (;;);
}

void Sensors_Init(){
    int32_t status;



    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C0_SignalEvent_t);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Initialization Failed\r\n");
        return;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Power Mode setting Failed\r\n");
        return;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Control Mode setting Failed\r\n");
        return;
    }

    /*! Initialize the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Initialize(&fxas21002Driver, &GYRO_I2C_DEVICE, GYRO_I2C_INDEX, GYRO_I2C_ADDRESS,
                                      FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXAS21002 Initialization Failed\r\n");
        return;
    }

    /*! Configure the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Configure(&fxas21002Driver, fxas21002_Config_ISR);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXAS21002 Sensor Configuration Failed, Err = %d\r\n", status);
        return;
    }

    /*! Initialize the FXOS8700 sensor driver. */
    status = FXOS8700_I2C_Initialize(&fxos8700Driver, &XM_I2C_DEVICE, XM_I2C_INDEX, XM_I2C_ADDRESS,
                                     FXOS8700_WHO_AM_I_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXOS8700 Initialization Failed\r\n");
        return;
    }

    /*! Configure the FXOS8700 sensor driver. */
    status = FXOS8700_I2C_Configure(&fxos8700Driver, fxos8700_Config_ISR);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXOS8700 Sensor Configuration Failed, Err = %d\r\n", status);
        return;
    }

    PRINTF("\r\n Succesfully initialized sensors!\r\n");

}

static void ninedof_task(void *pvParameters){
    int32_t status;

    EventBits_t event_set;
    EventBits_t event_wait_for = EVENT_XM | EVENT_G;

    uint8_t rawData_G[G_DATA_READ_SIZE];
    uint8_t rawData_XM[XM_DATA_READ_SIZE];

    fxas21002_gyrodata_t data_G;
    fxos8700_accelmagdata_t data_XM;

    uint32_t flag_xm, flag_g;
    flag_xm = 0; flag_g = 0;
    for(;;){

    	if (flag_xm && flag_g){
    		flag_xm = 0;
    		flag_g = 0;
    		event_wait_for = EVENT_XM | EVENT_G;

    		printf("\r\n Woot!\r\n");
    	} else if(flag_xm){
    		event_wait_for = EVENT_G;
    	} else if(flag_g){
    		event_wait_for = EVENT_XM;
    	}

		event_set = xEventGroupWaitBits(ninedof_event_group,
										 event_wait_for,
										 pdTRUE,
										 pdFALSE,
										 portMAX_DELAY);

		if (event_set & EVENT_XM){
			flag_xm = 1;

	        /*! Read the raw sensor data from the fxos8700. */
	        status = FXOS8700_I2C_ReadData(&fxos8700Driver, fxos8700_Read, rawData_XM);
	        if (ARM_DRIVER_OK != status)
	        {
	            PRINTF("\r\n FXOS8700 Read Failed. \r\n");
	            return -1;
	        }

	        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
	        data_XM.mag[0] = ((int16_t)rawData_XM[0] << 8) | rawData_XM[1];
	        data_XM.mag[1] = ((int16_t)rawData_XM[2] << 8) | rawData_XM[3];
	        data_XM.mag[2] = ((int16_t)rawData_XM[4] << 8) | rawData_XM[5];

	        data_XM.accel[0] = ((int16_t)rawData_XM[6] << 8) | rawData_XM[7];
	        data_XM.accel[0] /= 4;
	        data_XM.accel[1] = ((int16_t)rawData_XM[8] << 8) | rawData_XM[9];
	        data_XM.accel[1] /= 4;
	        data_XM.accel[2] = ((int16_t)rawData_XM[10] << 8) | rawData_XM[11];
	        data_XM.accel[2] /= 4;
		}

		if (event_set & EVENT_G){
			flag_g = 1;

	        /*! Read the raw sensor data from the FXAS21002. */
	        status = FXAS21002_I2C_ReadData(&fxas21002Driver, fxas21002_Read, rawData_G);
	        if (ARM_DRIVER_OK != status)
	        {
	            PRINTF("\r\n FXAS21002 Read Failed. \r\n");
	            return;
	        }

	        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
	        data_G.gyro[0] = ((int16_t)rawData_G[0] << 8) | rawData_G[1];
	        data_G.gyro[1] = ((int16_t)rawData_G[2] << 8) | rawData_G[3];
	        data_G.gyro[2] = ((int16_t)rawData_G[4] << 8) | rawData_G[5];
		}

    }
}
