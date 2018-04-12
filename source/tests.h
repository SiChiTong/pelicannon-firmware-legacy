/*
 * tests.h
 *
 *  Created on: Mar 26, 2018
 *      Author: cvance
 */

#ifndef TESTS_H_
#define TESTS_H_

#include "pelicannon-k66f-firmware.h"

#if MOTOR_TEST
void Motor_Test_Task(void* pvParam);
#endif

#endif /* TESTS_H_ */
