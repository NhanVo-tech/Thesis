/**
 * @file      type2ab_evb.h
 *
 * @brief     Pin mapping description corresponding to Murata Type 2AB EVB
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "nrf_gpio.h"
#include "type2ab.h"

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */

/* ENABLE_USB_PRINT Macro is uncommented then Segger RTT Print will be enabled*/
/* #define ENABLE_USB_PRINT */

/* Enable IMU */
#define CONFIG_IMU_ENABLE 1

/**
 * +---------------------------------------------+ *
 * |     LEDs definitions for Type2AB EVB        | *
 * +=============================================+ *
 * |  port&pin      |            Type            | *
 * +---------------------------------------------+ *
 * |  P0.28         |      LED_1/BSP_LED_0       | *
 * +---------------------------------------------+ *
 * |  P0.21         |      LED_2/BSP_LED_1       | *
 * +---------------------------------------------+ *
 **/
#define LEDS_NUMBER 2
#define LED_1       NRF_GPIO_PIN_MAP(0, 28)
#define LED_2       NRF_GPIO_PIN_MAP(0, 21)
#define LED_START   LED_1
#define LED_STOP    LED_2
#define BSP_LED_0   LED_1
#define BSP_LED_1   LED_2
#define LEDS_LIST    \
    {                \
        LED_1, LED_2 \
    }
#define LEDS_INV_MASK     LEDS_MASK
#define LEDS_ACTIVE_STATE 1

/**
 * +---------------------------------------------+ *
 * |    BUTTONs definitions for Type2AB EVB      | *
 * +=============================================+ *
 * |  port&pin            |         Type         | *
 * +---------------------------------------------+ *
 * |  P0.30               |         BUTTON_1     | *
 * +---------------------------------------------+ *
 **/

#define BUTTONS_NUMBER 1
#define BUTTON_1       NRF_GPIO_PIN_MAP(0, 30)
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL
#define BUTTONS_LIST \
    {                \
        BUTTON_1     \
    }
#define BUTTONS_ACTIVE_STATE 0

/**
 * +---------------------------------------------+ *
 * |    UART definitions for Type2AB EVB         | *
 * +=============================================+ *
 * |  port&pin      |            Type            | *
 * +---------------------------------------------+ *
 * |  P0.07         |           UART_TX          | *
 * +---------------------------------------------+ *
 * |  P0.08         |           UART_RX          | *
 * +---------------------------------------------+ *
 * |  disable       |           UART_RTS         | *
 * +---------------------------------------------+ *
 * |  disable       |           UART_CTS         | *
 * +---------------------------------------------+ *
 **/
#define UART_0_TX_PIN  NRF_GPIO_PIN_MAP(0, 7)
#define UART_0_RX_PIN  NRF_GPIO_PIN_MAP(0, 8)
#define CTS_PIN_NUMBER UART_PIN_DISCONNECTED
#define RTS_PIN_NUMBER UART_PIN_DISCONNECTED

#define REV_EnumToStr(x) \
    {                    \
        x, x##_NUM       \
    }

#define Type2AB_JS_UNLABLED_EVB_NUM "UNLABLED"
#define Type2AB_JS_1032_EVB_NUM     "1032"
#define Type2AB_JS_1069_EVB_NUM     "1069"
#define Type2AB_JS_0989_EVB_NUM     "0989"
#define Type2AB_JS_1055_EVB_NUM     "1055"

enum
{
    Type2AB_JS_UNLABLED_EVB = 0,
    Type2AB_JS_1032_EVB,
    Type2AB_JS_1069_EVB,
    Type2AB_JS_0989_EVB,
    Type2AB_JS_1055_EVB,
};

#define TYPE2AB_EVB_REV_DEFAULT           Type2AB_JS_1055_EVB

#define TYPE2AB_EVB_REV_3_D_M_5           (0.022777110597040736f) /* Distance between centers of antennas, ~(L_M/2), m, CH5 */
#define TYPE2AB_EVB_REV_3_D_M_9           (0.017883104683497245f) /* Distance between centers of antennas, ~(L_M/2), m, CH9 */
#define TYPE2AB_EVB_REV_3_CH5_ANT_DLY     (0.5 * 32)              /* External Antenna Delay value */
#define TYPE2AB_EVB_REV_3_CH9_ANT_DLY     (0.5 * 26)              /* External Antenna Delay value */
#define TYPE2AB_EVB_REV_3_CH5_PDOA_OFFSET 6                       /* PDoA code offset (degree) */
#define TYPE2AB_EVB_REV_3_CH9_PDOA_OFFSET -4                      /* PDoA code offset (degree) */

#define TYPE2AB_EVB_REV_4_D_M_5           (0.01476f) /* Distance between centers of antennas, ~(L_M/2), m, CH5 */
#define TYPE2AB_EVB_REV_4_D_M_9           (0.01476f) /* Distance between centers of antennas, ~(L_M/2), m, CH9 */
#define TYPE2AB_EVB_REV_4_CH5_ANT_DLY     (0.5 * 80) /* External Channel 5 Antenna Delay value, ant_dly = diff(m)/299702547.0/15.65 *10^12 */
#define TYPE2AB_EVB_REV_4_CH9_ANT_DLY     (0.5 * 60) /* External Channel 9 Antenna Delay value, ant_dly = diff(m)/299702547.0/15.65 *10^12 */
#define TYPE2AB_EVB_REV_4_CH5_PDOA_OFFSET -3         /* PDoA code offset (degree) */
#define TYPE2AB_EVB_REV_4_CH9_PDOA_OFFSET -20        /* PDoA code offset (degree) */

#ifdef __cplusplus
}
#endif /* end of __cplusplus */
