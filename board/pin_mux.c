/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

/* clang-format off */
/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Pins v4.0
processor: MK66FN2M0xxx18
package_id: MK66FN2M0VMD18
mcu_data: ksdk2_0
processor_version: 3.0.1
board: FRDM-K66F
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */
/* clang-format on */

#include "fsl_common.h"
#include "fsl_port.h"
#include "pin_mux.h"

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitBootPins
 * Description   : Calls initialization functions.
 *
 * END ****************************************************************************************************************/
void BOARD_InitBootPins(void)
{
    BOARD_InitPins();
}

/* clang-format off */
/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
BOARD_InitPins:
- options: {callFromInitBoot: 'true', coreID: core0, enableClock: 'true'}
- pin_list:
  - {pin_num: E10, peripheral: UART0, signal: RX, pin_signal: TSI0_CH9/PTB16/SPI1_SOUT/UART0_RX/FTM_CLKIN0/FB_AD17/SDRAM_D17/EWM_IN/TPM_CLKIN0}
  - {pin_num: E9, peripheral: UART0, signal: TX, pin_signal: TSI0_CH10/PTB17/SPI1_SIN/UART0_TX/FTM_CLKIN1/FB_AD16/SDRAM_D16/EWM_OUT_b/TPM_CLKIN1}
  - {pin_num: C9, peripheral: I2C0, signal: SCL, pin_signal: PTD8/LLWU_P24/I2C0_SCL/LPUART0_RX/FB_A16, open_drain: enable, pull_select: up, pull_enable: enable}
  - {pin_num: B9, peripheral: I2C0, signal: SDA, pin_signal: PTD9/I2C0_SDA/LPUART0_TX/FB_A17, open_drain: enable, pull_select: up, pull_enable: enable}
  - {pin_num: H11, peripheral: GPIOA, signal: 'GPIO, 29', pin_signal: PTA29/MII0_COL/FB_A24, pull_select: down}
  - {pin_num: H12, peripheral: GPIOA, signal: 'GPIO, 28', pin_signal: PTA28/MII0_TXER/FB_A25}
  - {pin_num: D5, peripheral: GPIOC, signal: 'GPIO, 17', pin_signal: PTC17/CAN1_TX/UART3_TX/ENET0_1588_TMR1/FB_CS4_b/FB_TSIZ0/FB_BE31_24_BLS7_0_b/SDRAM_DQM3}
  - {pin_num: D6, peripheral: GPIOC, signal: 'GPIO, 13', pin_signal: PTC13/UART4_CTS_b/FTM_CLKIN1/FB_AD26/SDRAM_D26/TPM_CLKIN1}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */
/* clang-format on */

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitPins
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 * END ****************************************************************************************************************/
void BOARD_InitPins(void)
{
    /* Port A Clock Gate Control: Clock enabled */
    CLOCK_EnableClock(kCLOCK_PortA);
    /* Port B Clock Gate Control: Clock enabled */
    CLOCK_EnableClock(kCLOCK_PortB);
    /* Port C Clock Gate Control: Clock enabled */
    CLOCK_EnableClock(kCLOCK_PortC);
    /* Port D Clock Gate Control: Clock enabled */
    CLOCK_EnableClock(kCLOCK_PortD);

    /* PORTA28 (pin H12) is configured as PTA28 */
    PORT_SetPinMux(BOARD_INITPINS_GYRO_INT2_PORT, BOARD_INITPINS_GYRO_INT2_PIN, kPORT_MuxAsGpio);

    /* PORTA29 (pin H11) is configured as PTA29 */
    PORT_SetPinMux(BOARD_INITPINS_GYRO_INT1_PORT, BOARD_INITPINS_GYRO_INT1_PIN, kPORT_MuxAsGpio);

    PORTA->PCR[29] = ((PORTA->PCR[29] &
                       /* Mask bits to zero which are setting */
                       (~(PORT_PCR_PS_MASK | PORT_PCR_ISF_MASK)))

                      /* Pull Select: Internal pulldown resistor is enabled on the corresponding pin, if the
                       * corresponding PE field is set. */
                      | PORT_PCR_PS(kPORT_PullDown));

    /* PORTB16 (pin E10) is configured as UART0_RX */
    PORT_SetPinMux(BOARD_INITPINS_DEBUG_UART_RX_PORT, BOARD_INITPINS_DEBUG_UART_RX_PIN, kPORT_MuxAlt3);

    /* PORTB17 (pin E9) is configured as UART0_TX */
    PORT_SetPinMux(BOARD_INITPINS_DEBUG_UART_TX_PORT, BOARD_INITPINS_DEBUG_UART_TX_PIN, kPORT_MuxAlt3);

    /* PORTC13 (pin D6) is configured as PTC13 */
    PORT_SetPinMux(BOARD_INITPINS_ACCEL_INT2_PORT, BOARD_INITPINS_ACCEL_INT2_PIN, kPORT_MuxAsGpio);

    /* PORTC17 (pin D5) is configured as PTC17 */
    PORT_SetPinMux(BOARD_INITPINS_ACCEL_INT1_PORT, BOARD_INITPINS_ACCEL_INT1_PIN, kPORT_MuxAsGpio);

    /* PORTD8 (pin C9) is configured as I2C0_SCL */
    PORT_SetPinMux(PORTD, 8U, kPORT_MuxAlt2);

    PORTD->PCR[8] = ((PORTD->PCR[8] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_ODE_MASK | PORT_PCR_ISF_MASK)))

                     /* Pull Select: Internal pullup resistor is enabled on the corresponding pin, if the
                      * corresponding PE field is set. */
                     | (uint32_t)(kPORT_PullUp)

                     /* Open Drain Enable: Open drain output is enabled on the corresponding pin, if the pin is
                      * configured as a digital output. */
                     | PORT_PCR_ODE(kPORT_OpenDrainEnable));

    /* PORTD9 (pin B9) is configured as I2C0_SDA */
    PORT_SetPinMux(PORTD, 9U, kPORT_MuxAlt2);

    PORTD->PCR[9] = ((PORTD->PCR[9] &
                      /* Mask bits to zero which are setting */
                      (~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_ODE_MASK | PORT_PCR_ISF_MASK)))

                     /* Pull Select: Internal pullup resistor is enabled on the corresponding pin, if the
                      * corresponding PE field is set. */
                     | (uint32_t)(kPORT_PullUp)

                     /* Open Drain Enable: Open drain output is enabled on the corresponding pin, if the pin is
                      * configured as a digital output. */
                     | PORT_PCR_ODE(kPORT_OpenDrainEnable));

    SIM->SOPT5 = ((SIM->SOPT5 &
                   /* Mask bits to zero which are setting */
                   (~(SIM_SOPT5_UART0TXSRC_MASK)))

                  /* UART 0 transmit data source select: UART0_TX pin. */
                  | SIM_SOPT5_UART0TXSRC(SOPT5_UART0TXSRC_UART_TX));
}
/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/