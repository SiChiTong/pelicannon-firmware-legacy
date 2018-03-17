#ifndef FRDM_K66F_H_
#define FRDM_K66F_H_

#include "MK66F18.h"

#include "fsl_i2c_cmsis.h"
#include "register_io_i2c.h"
#include "gpio_driver.h"
#include "pin_mux.h"

extern gpioHandleKSDK_t FXAS21002_INT1;
extern gpioHandleKSDK_t FXAS21002_INT2;
extern gpioHandleKSDK_t FXOS8700_INT1;
extern gpioHandleKSDK_t FXOS8700_INT2;

// Header 1 Row 2 GPIO
extern gpioHandleKSDK_t GPIO_PTA27;
extern gpioHandleKSDK_t GPIO_PTA26;
extern gpioHandleKSDK_t GPIO_PTA4;
extern gpioHandleKSDK_t GPIO_PTA6;
extern gpioHandleKSDK_t GPIO_PTA7;
extern gpioHandleKSDK_t GPIO_PTA8;
extern gpioHandleKSDK_t GPIO_PTA9;
extern gpioHandleKSDK_t GPIO_PTA1;

// Header 4 Row 2 GPIO
extern gpioHandleKSDK_t GPIO_PTA25;
#define MOTOR_A1 GPIO_PTA25
extern gpioHandleKSDK_t GPIO_PTC2;
#define MOTOR_A2 GPIO_PTC2
extern gpioHandleKSDK_t GPIO_PTC5;
#define MOTOR_B1 GPIO_PTC5
extern gpioHandleKSDK_t GPIO_PTC12;
#define MOTOR_B2 GPIO_PTC12

uint32_t I2C0_GetFreq(void);
void I2C0_InitPins(void);
void I2C0_DeinitPins(void);

#endif /* FRDM_K66F_H_ */
