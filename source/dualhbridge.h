/*
 * dualhbridge.h
 *
 *  Created on: Mar 25, 2018
 *      Author: cvance
 */

#ifndef DUALHBRIDGE_H_
#define DUALHBRIDGE_H_

void DualHBridge_Task(void *pvParameters);
void DualHBridge_Init();

int DualHBridge_GetPosition();
int DualHBridge_StepsLeft();

void DualHBridge_Step(int steps);
void DualHBridge_Abort();

#endif /* DUALHBRIDGE_H_ */
