/*
 * pelicannon-k66f-firmware.h
 *
 *  Created on: Mar 16, 2018
 *      Author: cvance
 */

#ifndef PELICANNON_K66F_FIRMWARE_H_
#define PELICANNON_K66F_FIRMWARE_H_

#include "board.h"
#include "fsl_i2c_cmsis.h"
#include "fsl_debug_console.h"

/**
 * Enables and muxes the Debug UART
 * Breakpointable application global assert
 */
#define DEVELOPMENT 1

/* Development sub settings */

/*! @brief	Drives GPIOs so an oscilloscope or logic analyzer can view jitter in the ninedof data synchronization*/
#define NINEDOF_GPIO_DEBUG_TIMING 0
/*! @brief	Endlessly drives the motor forwards and backwards*/
#define MOTOR_TEST 0
/*! @brief	Outputs every 25 samples from the accelerometer/gyrometer/magnetmometer to the debug UART*/
#define NINEDOF_DEBUG_VALUES 0

/* Stepper motor settings */
/**
 * @brief	Stepper motor steps per rotation
 * Step this many steps left or right to move 2*pi or 360 degrees
 */
#define STEPPER_MOTOR_STEPS_PER_ROTATION 200
/*! @brief	Speed at which the stepper motor is driven in rotations per minute*/
#define STEPPER_MOTOR_RPM 60 / 4

/*! @brief   Maximum steps to take in either direction */
#define STEPPER_GUARD 45

/* Preprocessor safety check */
#if !DEVELOPMENT && (GPIO_DEBUG_MODE || MOTOR_TEST || NINEDOF_GPIO_DEBUG_TIMING)
#error "Invalid settings for production mode"
#endif

#if DEVELOPMENT
#define DPRINTF PRINTF
#else
#define DPRINTF(format, args...) ((void)0)
#endif

#if DEVELOPMENT
void application_assert(int);
#else
inline void application_assert(int);
#endif

#define ASSERT(x) application_assert((x))
/**
 * @brief	If the assertion is false, display a message and call the application assert handler
 */
#define ASSERT_MSG(condition, message) if(!(condition)){\
	DPRINTF(message);\
	ASSERT(0);\
	}
/**
 * @brief	If the assertion is not false, display a message and call the application assert handler
 */
#define ASSERT_NOT_MSG(condition, message) if((condition)){\
	DPRINTF(message);\
	ASSERT(0);\
	}

#endif /* PELICANNON_K66F_FIRMWARE_H_ */
