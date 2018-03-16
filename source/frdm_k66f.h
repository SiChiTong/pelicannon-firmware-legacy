#ifndef FRDM_K66F_H_
#define FRDM_K66F_H_

#include "MK66F18.h"

#include "fsl_i2c_cmsis.h"
#include "register_io_i2c.h"

uint32_t I2C0_GetFreq(void);
void I2C0_InitPins(void);
void I2C0_DeinitPins(void);

#endif /* FRDM_K66F_H_ */
