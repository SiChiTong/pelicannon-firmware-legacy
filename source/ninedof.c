/**
 * @file	ninedof.c
 * @author	Carroll Vance
 * @brief	Initializes and synchronizes reads FXOS8700 and FXAS21002 sensors using data ready interupts
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
/*! @brief	Used to wake Ninedof_Task when new data is available*/
static EventGroupHandle_t ninedof_event_group = NULL;
/*! @brief	Locks access to the current sensor readings*/
static SemaphoreHandle_t ninedof_data_mutex = NULL;

/*! @brief	Locks access to the current sensor readings*/
static ARM_DRIVER_I2C *NineDoF_I2Cdrv = &G_I2C_DEVICE;

/* Accelerometer/magnometer use the same I2C bus as gyrometer */
/*! @brief	FXAS21002 Gyroscope I2C Driver*/
static fxas21002_i2c_sensorhandle_t fxas21002Driver;
/*! @brief	FXOS8700 Accelerometer/Magnetometer I2C Driver*/
static fxos8700_i2c_sensorhandle_t fxos8700Driver;

/* Device Configuration */

/*! @brief	FXOS8700 Initialization Registers*/
const registerwritelist_t fxos8700_Config_ISR[] = {
/*! System and Control registers. */
/*! Configure the FXOS8700 to 100Hz sampling rate. */
{ FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_DR_HYBRID_100_HZ,
		FXOS8700_CTRL_REG1_DR_MASK },
		{ FXOS8700_CTRL_REG3, FXOS8700_CTRL_REG3_IPOL_ACTIVE_HIGH
				| FXOS8700_CTRL_REG3_PP_OD_PUSH_PULL,
		FXOS8700_CTRL_REG3_IPOL_MASK | FXOS8700_CTRL_REG3_PP_OD_MASK }, /*! Active High, Push-Pull */
		{ FXOS8700_CTRL_REG4, FXOS8700_CTRL_REG4_INT_EN_DRDY_EN,
		FXOS8700_CTRL_REG4_INT_EN_DRDY_MASK }, /*! Data Ready Event. */
		{ FXOS8700_CTRL_REG5, FXOS8700_CTRL_REG5_INT_CFG_DRDY_INT2,
				FXOS8700_CTRL_REG5_INT_CFG_DRDY_MASK }, /*! INT2 Pin  */
		{ FXOS8700_M_CTRL_REG1, FXOS8700_M_CTRL_REG1_M_HMS_HYBRID_MODE,
				FXOS8700_M_CTRL_REG1_M_HMS_MASK }, { FXOS8700_M_CTRL_REG2,
				FXOS8700_M_CTRL_REG2_M_AUTOINC_HYBRID_MODE,
				FXOS8700_M_CTRL_REG2_M_AUTOINC_MASK },
		__END_WRITE_DATA__ };

/*! @brief	FXAS21002 Initialization Registers*/
const registerwritelist_t fxas21002_Config_ISR[] = {
		/*! Configure CTRL_REG1 register to put FXAS21002 to 100Hz sampling rate. */
		{ FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_DR_100HZ,
				FXAS21002_CTRL_REG1_DR_MASK },
		/*! Configure CTRL_REG2 register to set interrupt configuration settings. */
		{ FXAS21002_CTRL_REG2, FXAS21002_CTRL_REG2_IPOL_ACTIVE_HIGH
				| FXAS21002_CTRL_REG2_INT_EN_DRDY_ENABLE |
				FXAS21002_CTRL_REG2_INT_CFG_DRDY_INT1,
		FXAS21002_CTRL_REG2_IPOL_MASK | FXAS21002_CTRL_REG2_INT_EN_DRDY_MASK
				| FXAS21002_CTRL_REG2_INT_CFG_DRDY_MASK },
		__END_WRITE_DATA__ };

#define XM_DATA_READ_SIZE 12
/*! @brief	FXOS8700 I2C Read Sequence*/
const registerreadlist_t fxos8700_Read[] = { { .readFrom = FXOS8700_OUT_X_MSB,
		.numBytes = XM_DATA_READ_SIZE },
__END_READ_DATA__ };

#define G_DATA_READ_SIZE 6
/*! @brief	FXAS21002 I2C Read Sequence*/
const registerreadlist_t fxas21002_Read[] = { { .readFrom = FXAS21002_OUT_X_MSB,
		.numBytes = G_DATA_READ_SIZE }, __END_READ_DATA__ };

/*! @brief	FXAS21002 I2C Status Sequence*/
const registerreadlist_t fxas21002_Status[] = { { .readFrom = FXAS21002_STATUS,
		.numBytes = 1 }, __END_READ_DATA__ };

/* ISR Routines */

/**
 * @brief	Handle ISR from FXAS21002
 * @usage	Should only be called by the NVIC driver
 * @param	pUserData Unused
 */
void FXAS21002_ISR(void *pUserData) {
	BaseType_t xHigherPriorityTaskWoken, xResult;

#if GPIO_DEBUG_MODE
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	gpioDriver->write_pin(&GPIO_DEBUG_1, 1);
#endif

	xHigherPriorityTaskWoken = pdFALSE;

	xResult = xEventGroupSetBitsFromISR(ninedof_event_group, NINEDOF_EVENT_G,
			&xHigherPriorityTaskWoken);
	if (xResult != pdFAIL) {
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		 switch should be requested.  The macro used is port specific and will
		 be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		 the documentation page for the port being used. */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

}

/**
 * @brief	Handle ISR from FXOS8700
 * @usage	Should only be called by the NVIC driver
 * @param	pUserData Unused
 */
void FXOS8700_ISR(void *pUserData) {
	BaseType_t xHigherPriorityTaskWoken, xResult;

#if GPIO_DEBUG_MODE
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	gpioDriver->write_pin(&GPIO_DEBUG_2, 1);
#endif

	xHigherPriorityTaskWoken = pdFALSE;

	xResult = xEventGroupSetBitsFromISR(ninedof_event_group, NINEDOF_EVENT_XM,
			&xHigherPriorityTaskWoken);
	if (xResult != pdFAIL) {
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		 switch should be requested.  The macro used is port specific and will
		 be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		 the documentation page for the port being used. */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

}

/**
 * @brief	Initialize the NineDoF functionality group
 * @usage	Should only be called by the application entry point
 */
void NineDoF_Init() {
	int32_t status;

	/* Create event groups */
	ninedof_event_group = xEventGroupCreate();
	ninedof_data_mutex = xSemaphoreCreateMutex();
	ASSERT_NOT_MSG(!ninedof_event_group || !ninedof_data_mutex, "Failed to initialize Ninedof synchronization primitives\r\n");


	/* Setup Interupts */
	GENERIC_DRIVER_GPIO *gpioDriver = &Driver_GPIO_KSDK;

	gpioDriver->pin_init(&FXAS21002_INT1, GPIO_DIRECTION_IN, NULL,
			&FXAS21002_ISR, NULL);
	gpioDriver->pin_init(&FXOS8700_INT2, GPIO_DIRECTION_IN, NULL, &FXOS8700_ISR,
			NULL);

#if GPIO_DEBUG_MODE
	gpioDriver->pin_init(&GPIO_DEBUG_1, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
	gpioDriver->pin_init(&GPIO_DEBUG_2, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
	gpioDriver->pin_init(&GPIO_DEBUG_3, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
#endif

	/*! Initialize the I2C driver. */
	status = NineDoF_I2Cdrv->Initialize(I2C0_SignalEvent_t);
	ASSERT_NOT_MSG(ARM_DRIVER_OK != status, "I2C Initialization Failed\r\n");

	/*! Set the I2C Power mode. */
	status = NineDoF_I2Cdrv->PowerControl(ARM_POWER_FULL);
	ASSERT_NOT_MSG(ARM_DRIVER_OK != status, "I2C Power Mode setting Failed\r\n");

	/*! Set the I2C bus speed. */
	status = NineDoF_I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
	ASSERT_NOT_MSG(ARM_DRIVER_OK != status, "I2C Control Mode setting Failed\r\n");

	/*! Initialize the FXAS21002 sensor driver. */
	status = FXAS21002_I2C_Initialize(&fxas21002Driver, &G_I2C_DEVICE,
			G_I2C_INDEX, G_I2C_ADDRESS,
			FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);
	ASSERT_NOT_MSG(SENSOR_ERROR_NONE != status, "FXAS21002 Initialization Failed\r\n");

	/*! Configure the FXAS21002 sensor driver. */
	status = FXAS21002_I2C_Configure(&fxas21002Driver, fxas21002_Config_ISR);
	ASSERT_NOT_MSG(SENSOR_ERROR_NONE != status, "FXAS21002 Sensor Configuration Failed\r\n");

	/*! Initialize the FXOS8700 sensor driver. */
	status = FXOS8700_I2C_Initialize(&fxos8700Driver, &XM_I2C_DEVICE,
			XM_I2C_INDEX, XM_I2C_ADDRESS,
			FXOS8700_WHO_AM_I_PROD_VALUE);
	ASSERT_NOT_MSG(SENSOR_ERROR_NONE != status, "FXOS8700 Initialization Failed\r\n");

	/*! Configure the FXOS8700 sensor driver. */
	status = FXOS8700_I2C_Configure(&fxos8700Driver, fxos8700_Config_ISR);
	ASSERT_NOT_MSG(SENSOR_ERROR_NONE != status, "FXOS8700 Sensor Configuration Failed\r\n");

}

/*!@brief	Stores the latest gyrometer data from the FXAS21002*/
static fxas21002_gyrodata_t ninedof_data_G;
/*!@brief	Stores the latest accelerometer/magnetometer data from the FXOS8700*/
static fxos8700_accelmagdata_t ninedof_data_XM;

/**
 * @brief	Copy the current Ninedof data to another location
 * @usage	Can be called by any task
 * @param	g	Pointer to a fxas21002_gyrodata_t structure which will receive the latest data from the FXAS21002
 * @param 	m	Pointer to a fxos8700_accelmagdata_t structure which will receive the latest data from the FXOS8700
 */
void Ninedof_CopyData(fxas21002_gyrodata_t* g, fxos8700_accelmagdata_t* xm) {
	xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);
	memcpy(g, &ninedof_data_G, sizeof(ninedof_data_G));
	memcpy(xm, &ninedof_data_XM, sizeof(ninedof_data_XM));
	xSemaphoreGive(ninedof_data_mutex);
}

/**
 * @brief	Synchronizes reads to the NineDoF sensors based on data ready events
 * @usage	Called by FreeRTOS after xTaskCreate
 * @param 	vParameters	Unused
 * Tells the TK1 module that data is ready to send
 */
void Ninedof_Task(void *pvParameters) {

	uint32_t flag_xm, flag_g;

	int32_t status;

	EventBits_t event_set;

	uint8_t rawData_G[G_DATA_READ_SIZE];
	uint8_t rawData_XM[XM_DATA_READ_SIZE];

	flag_xm = 0;
	flag_g = 0;
	for (;;) {

#if GPIO_DEBUG_MODE
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

		if (flag_xm && flag_g) {

#if GPIO_DEBUG_MODE
			gpioDriver->write_pin(&GPIO_DEBUG_3, 1);
#endif

			flag_xm = 0;
			flag_g = 0;

			TK1_NineDof_DataReady();

#if GPIO_DEBUG_MODE
			gpioDriver->write_pin(&GPIO_DEBUG_3, 0);
#endif

		} else if (flag_xm > 1) {

			/* Sometimes an interupt is missed for the gyrometer.
			 * If we have data for the accelerometer but not gyrometer,
			 * query status of gyrometer. If the watermark event or FIFO is overun,
			 * set the event to read the gyrometer data.
			 */
			uint8_t register_status;
			FXAS21002_I2C_ReadData(&fxas21002Driver, fxas21002_Status,
					&register_status);

			//If an overflow occured or data is waiting in the FIFO
			if (register_status & 0xC0)
				xEventGroupSetBits(ninedof_event_group, NINEDOF_EVENT_G);

		}

		event_set = xEventGroupWaitBits(ninedof_event_group,
		NINEDOF_EVENT_XM | NINEDOF_EVENT_G,
		pdTRUE,
		pdFALSE,
		portMAX_DELAY);

		if (event_set & NINEDOF_EVENT_XM) {
			flag_xm += 1;

			/*! Read the raw sensor data from the FXOS8700. */
			status = FXOS8700_I2C_ReadData(&fxos8700Driver, fxos8700_Read,
					rawData_XM);
			if (ARM_DRIVER_OK != status) {
				DPRINTF("FXOS8700 Read Failed.\r\n");
				return;
			}

			xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);

			ninedof_data_XM.accel[0] = (int16_t) (((rawData_XM[0] << 8)
					| rawData_XM[1])) >> 2;
			ninedof_data_XM.accel[1] = (int16_t) (((rawData_XM[2] << 8)
					| rawData_XM[3])) >> 2;
			ninedof_data_XM.accel[2] = (int16_t) (((rawData_XM[4] << 8)
					| rawData_XM[5])) >> 2;

			ninedof_data_XM.mag[0] = ((int16_t) rawData_XM[6] << 8)
					| rawData_XM[7];
			ninedof_data_XM.mag[1] = ((int16_t) rawData_XM[8] << 8)
					| rawData_XM[9];
			ninedof_data_XM.mag[2] = ((int16_t) rawData_XM[10] << 8)
					| rawData_XM[11];
			xSemaphoreGive(ninedof_data_mutex);

		}

		if (event_set & NINEDOF_EVENT_G) {
			flag_g += 1;

			/*! Read the raw sensor data from the FXAS21002. */
			status = FXAS21002_I2C_ReadData(&fxas21002Driver, fxas21002_Read,
					rawData_G);
			if (ARM_DRIVER_OK != status) {
				DPRINTF("FXAS21002 Read Failed.\r\n");
				return;
			}

			xSemaphoreTake(ninedof_data_mutex, portMAX_DELAY);
			/*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
			ninedof_data_G.gyro[0] = ((int16_t) rawData_G[0] << 8)
					| rawData_G[1];
			ninedof_data_G.gyro[1] = ((int16_t) rawData_G[2] << 8)
					| rawData_G[3];
			ninedof_data_G.gyro[2] = ((int16_t) rawData_G[4] << 8)
					| rawData_G[5];
			xSemaphoreGive(ninedof_data_mutex);

		}

	}
}
