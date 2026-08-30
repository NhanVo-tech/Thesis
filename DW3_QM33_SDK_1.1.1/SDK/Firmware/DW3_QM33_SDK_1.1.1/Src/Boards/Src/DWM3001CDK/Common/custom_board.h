/**
 * @file      custom_board.h
 *
 * @brief     Pin mapping description for DWM3001CDK
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"
#include "nrf_spim.h"

/* LEDs definitions for PCA10056. */
#define LEDS_NUMBER 4

/* If AOA chip is populated on the PCB without second antenna, this shall be set to (1). */
#define AOA_CHIP_ON_NON_AOA_PCB (0)

#define LED_1                   NRF_GPIO_PIN_MAP(0, 4)  /* D9 on the schematics. */
#define LED_2                   NRF_GPIO_PIN_MAP(0, 5)  /* D10 on the schematics. */
#define LED_3                   NRF_GPIO_PIN_MAP(0, 22) /* D11 on the schematics. */
#define LED_4                   NRF_GPIO_PIN_MAP(0, 14) /* D12 on the schematics. */
#define LED_START               LED_1
#define LED_STOP                LED_4

#define LEDS_ACTIVE_STATE       0

#define LEDS_LIST                  \
    {                              \
        LED_1, LED_2, LED_3, LED_4 \
    }

#define LEDS_INV_MASK        LEDS_MASK

#define BSP_LED_0            4
#define BSP_LED_1            5
#define BSP_LED_2            22
#define BSP_LED_3            14

#define BUTTONS_NUMBER       1

#define BUTTON_1             2

#define BUTTON_PULL          NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST \
    {                \
        BUTTON_1     \
    }

#define BSP_BUTTON_0   BUTTON_1

#define RX_PIN_NUMBER  15
#define TX_PIN_NUMBER  19
#define CTS_PIN_NUMBER (-1)
#define RTS_PIN_NUMBER (-1)
#define HWFC           false


#define LED_ERROR      BSP_LED_0
/* UART symbolic constants. */
#define UART_0_TX_PIN TX_PIN_NUMBER
#define UART_0_RX_PIN RX_PIN_NUMBER

#ifdef __cplusplus
}
#endif
