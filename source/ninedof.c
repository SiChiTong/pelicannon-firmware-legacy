/*
 * ninedof.c
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
#include "core_cm4.h"

/* ISSDK Includes */
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "fxas21002_drv.h"
#include "fxos8700_drv.h"

#include "pelicannon-k66f-firmware.h"
#include "ninedof.h"
#include "tk1.h"

void NineDoF_Init();

/* Synchronization Primitives */
#define NINEDOF_EVENT_XM (1<<0)
#define NINEDOF_EVENT_G (1<<1)
EventGroupHandle_t ninedof_event_group = NULL;

static SemaphoreHandle_t ninedof_data_mutex = NULL;


/* Device Handles */
/* Accelerometer/magnometer use the same I2C bus as gyrometer */
ARM_DRIVER_I2C *I2Cdrv = &GYRO_I2C_DEVICE;
fxas21002_i2c_sensorhandle_t fxas21002Driver;
fxos8700_i2c_sensorhandle_t fxos8700Driver;

/* Device Configuration */
const registerwritelist_t fxos8700_Config_ISR[] = {
    /*! System and Control registers. */
    /*! Configure the FXOS8700 to 100Hz sampling rate. */
    {FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_DR_HYBRID_100_HZ, FXOS8700_CTRL_REG1_DR_MASK},
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
const registerreadlist_t fxas21002_Status[] = {
  {.readFrom = FXAS21002_STATUS, .numBytes = 1}, __END_READ_DATA__};

/* ISR Routines */
void FXAS21002_ISR(void *pUserData){
    BaseType_t xHigherPriorityTaskWoken, xResult;

#ifdef GPIO_DEBUG_MODE
    GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

    gpioDriver->write_pin(&GPIO_DEBUG_1, 1);
#endif

    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(ninedof_event_group, NINEDOF_EVENT_G, &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
      {
          /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
          switch should be requested.  The macro used is port specific and will
          be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
          the documentation page for the port being used. */
          portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
      }



}

void FXOS8700_ISR(void *pUserData){
    BaseType_t xHigherPriorityTaskWoken, xResult;

#ifdef GPIO_DEBUG_MODE
    GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

    gpioDriver->write_pin(&GPIO_DEBUG_2, 1);
#endif

    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(ninedof_event_group, NINEDOF_EVENT_XM, &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
      {
          /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
          switch should be requested.  The macro used is port specific and will
          be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
          the documentation page for the port being used. */
          portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
      }

}

void NineDoF_Init(){
    int32_t status;

    /* Create event groups */
    ninedof_event_group = xEventGroupCreate();

	ninedof_data_mutex = xSemaphoreCreateMutex();

    /* Setup Interupts */
    GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

    gpioDriver->pin_init(&FXAS21002_INT1, GPIO_DIRECTION_IN, NULL, &FXAS21002_ISR, NULL);
    gpioDriver->pin_init(&FXOS8700_INT2, GPIO_DIRECTION_IN, NULL, &FXOS8700_ISR, NULL);

#ifdef GPIO_DEBUG_MODE
    gpioDriver->pin_init(&GPIO_DEBUG_1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    gpioDriver->pin_init(&GPIO_DEBUG_2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    gpioDriver->pin_init(&GPIO_DEBUG_3, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
#endif

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C0_SignalEvent_t);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("I2C Initialization Failed\r\n");
        return;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("I2C Power Mode setting Failed\r\n");
        return;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("I2C Control Mode setting Failed\r\n");
        return;
    }

    /*! Initialize the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Initialize(&fxas21002Driver, &GYRO_I2C_DEVICE, GYRO_I2C_INDEX, GYRO_I2C_ADDRESS,
                                      FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("FXAS21002 Initialization Failed\r\n");
        return;
    }

    /*! Configure the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Configure(&fxas21002Driver, fxas21002_Config_ISR);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("FXAS21002 Sensor Configuration Failed, Err = %d\r\n", status);
        return;
    }

    /*! Initialize the FXOS8700 sensor driver. */
    status = FXOS8700_I2C_Initialize(&fxos8700Driver, &XM_I2C_DEVICE, XM_I2C_INDEX, XM_I2C_ADDRESS,
                                     FXOS8700_WHO_AM_I_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("FXOS8700 Initialization Failed\r\n");
        return;
    }

    /*! Configure the FXOS8700 sensor driver. */
    status = FXOS8700_I2C_Configure(&fxos8700Driver, fxos8700_Config_ISR);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("FXOS8700 Sensor Configuration Failed, Err = %d\r\n", status);
        return;
    }

}

static fxas21002_gyrodata_t ninedof_data_G;
static fxos8700_accelmagdata_t ninedof_data_XM;


void Ninedof_CopyData(fxas21002_gyrodata_t* g, fxos8700_accelmagdata_t* xm){
	xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);
	memcpy(g, &ninedof_data_G, sizeof(ninedof_data_G));
	memcpy(xm, &ninedof_data_XM, sizeof(ninedof_data_XM));
	xSemaphoreGive(ninedof_data_mutex);
}

void Ninedof_Task(void *pvParameters){

	uint32_t flag_xm, flag_g;

    int32_t status;

    EventBits_t event_set;

    uint8_t rawData_G[G_DATA_READ_SIZE];
    uint8_t rawData_XM[XM_DATA_READ_SIZE];

    flag_xm = 0; flag_g = 0;
    for(;;){

#ifdef GPIO_DEBUG_MODE
    	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

		if (gpioDriver->read_pin(&FXOS8700_INT2))
		    gpioDriver->write_pin(&GPIO_DEBUG_2, 1);
		else
		    gpioDriver->write_pin(&GPIO_DEBUG_2, 0);

		if (gpioDriver->read_pin(&FXAS21002_INT1))
		    gpioDriver->write_pin(&GPIO_DEBUG_1, 1);
		else
		    gpioDriver->write_pin(&GPIO_DEBUG_1, 0);
#endif

    	if (flag_xm && flag_g){

#ifdef GPIO_DEBUG_MODE
    	    gpioDriver->write_pin(&GPIO_DEBUG_3, 1);
#endif

    		flag_xm = 0;
    		flag_g = 0;

    		TK1_NineDof_DataReady();

#ifdef GPIO_DEBUG_MODE
    		gpioDriver->write_pin(&GPIO_DEBUG_3, 0);
#endif

    	} else if(flag_xm){

    		/* Sometimes an interupt is missed for the gyrometer.
    		 * If we have data for the accelerometer but not gyrometer,
    		 * query status of gyrometer. If the watermark event or FIFO is overun,
    		 * set the event to read the gyrometer data.
    		 */
    		uint8_t register_status;
    		FXAS21002_I2C_ReadData(&fxas21002Driver, fxas21002_Status, &register_status);

    		//If an overflow occured or data is waiting in the FIFO
    		if(register_status & 0xC0){
    			xEventGroupSetBits(ninedof_event_group, NINEDOF_EVENT_G);
    		}

    	}

		event_set = xEventGroupWaitBits(ninedof_event_group,
										NINEDOF_EVENT_XM | NINEDOF_EVENT_G,
										 pdTRUE,
										 pdFALSE,
										 portMAX_DELAY);


		if (event_set & NINEDOF_EVENT_XM){
			flag_xm = 1;


	        /*! Read the raw sensor data from the FXOS8700. */
	        status = FXOS8700_I2C_ReadData(&fxos8700Driver, fxos8700_Read, rawData_XM);
	        if (ARM_DRIVER_OK != status)
	        {
	            PRINTF("FXOS8700 Read Failed.\r\n");
	            return;
	        }

	    	xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);
	        ninedof_data_XM.mag[0] = ((int16_t)rawData_XM[0] << 8) | rawData_XM[1];
	        ninedof_data_XM.mag[1] = ((int16_t)rawData_XM[2] << 8) | rawData_XM[3];
	        ninedof_data_XM.mag[2] = ((int16_t)rawData_XM[4] << 8) | rawData_XM[5];

	        ninedof_data_XM.accel[0] = ((int16_t)rawData_XM[6] << 8) | rawData_XM[7];
	        //ninedof_data_XM.accel[0] /= 4;
	        ninedof_data_XM.accel[1] = ((int16_t)rawData_XM[8] << 8) | rawData_XM[9];
	        //ninedof_data_XM.accel[1] /= 4;
	        ninedof_data_XM.accel[2] = ((int16_t)rawData_XM[10] << 8) | rawData_XM[11];
	        //ninedof_data_XM.accel[2] /= 4;
	    	xSemaphoreGive(ninedof_data_mutex);

		}

		if (event_set & NINEDOF_EVENT_G){
			flag_g = 1;

	        /*! Read the raw sensor data from the FXAS21002. */
	        status = FXAS21002_I2C_ReadData(&fxas21002Driver, fxas21002_Read, rawData_G);
	        if (ARM_DRIVER_OK != status)
	        {
	            PRINTF("FXAS21002 Read Failed.\r\n");
	            return;
	        }

	    	xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);
	        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
	        ninedof_data_G.gyro[0] = ((int16_t)rawData_G[0] << 8) | rawData_G[1];
	        ninedof_data_G.gyro[1] = ((int16_t)rawData_G[2] << 8) | rawData_G[3];
	        ninedof_data_G.gyro[2] = ((int16_t)rawData_G[4] << 8) | rawData_G[5];
	    	xSemaphoreGive(ninedof_data_mutex);

		}

    }
}
