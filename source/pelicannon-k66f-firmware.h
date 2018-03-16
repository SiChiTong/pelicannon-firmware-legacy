/*
 * pelicannon-k66f-firmware.h
 *
 *  Created on: Mar 16, 2018
 *      Author: cvance
 */

#ifndef PELICANNON_K66F_FIRMWARE_H_
#define PELICANNON_K66F_FIRMWARE_H_

#include "fsl_i2c_cmsis.h"

#define XM_I2C_ADDRESS 0x1D
#define XM_I2C_DEVICE Driver_I2C0
#define XM_I2C_EVENT I2C0_SignalEvent_t
#define XM_I2C_INDEX 0

#define GYRO_I2C_ADDRESS 0x21
#define GYRO_I2C_DEVICE Driver_I2C0
#define GYRO_I2C_EVENT I2C0_SignalEvent_t
#define GYRO_I2C_INDEX 0

#endif /* PELICANNON_K66F_FIRMWARE_H_ */