/*
 * ninedof.h
 *
 *  Created on: Mar 25, 2018
 *      Author: cvance
 */

#ifndef NINEDOF_H_
#define NINEDOF_H_

#include "fxas21002_drv.h"
#include "fxos8700_drv.h"

void Ninedof_Task(void *pvParameters);
void NineDoF_Init();
void Ninedof_CopyData(fxas21002_gyrodata_t* g, fxos8700_accelmagdata_t* xm);

#endif /* NINEDOF_H_ */
