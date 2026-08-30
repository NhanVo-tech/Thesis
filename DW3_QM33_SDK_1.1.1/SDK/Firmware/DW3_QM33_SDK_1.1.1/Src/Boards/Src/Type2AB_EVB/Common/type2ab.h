/**
 * @file      type2ab.h
 *
 * @brief     Pin mapping description corresponding to Murata Type 2AB Module
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "nrf_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif /* end of __cplusplus */


/**
 * +---------------------------------------------+ *
 * |     Pins connected to LIS2DW12              | *
 * +=============================================+ *
 * |  port&pin      |            Type            | *
 * +---------------------------------------------+ *
 * |  P0.19         |           SCL/SPC          | *
 * +---------------------------------------------+ *
 * |  P0.22         |           SDA/SDI/SDO      | *
 * +---------------------------------------------+ *
 * |  P1.1          |           INT1             | *
 * +---------------------------------------------+ *
 * |  P1.4          |           INT2             | *
 * +---------------------------------------------+ *
 * |  P1.3          |           CS               | *
 * +---------------------------------------------+ *
 * |  P1.0          |           SDO/SA0          | *
 * +---------------------------------------------+ *
 **/
#define LIS2DW12_IIC_SCL  NRF_GPIO_PIN_MAP(0, 19)
#define LIS2DW12_IIC_SDA  NRF_GPIO_PIN_MAP(0, 22)
#define LIS2DW12_INT1     NRF_GPIO_PIN_MAP(1, 1)
#define LIS2DW12_INT2     NRF_GPIO_PIN_MAP(1, 4)
#define LIS2DW12_SPI_CS   NRF_GPIO_PIN_MAP(1, 3)
#define LIS2DW12_SPI_MISO NRF_GPIO_PIN_MAP(1, 0)
#define LIS2DW12_SPI_SCLK NRF_GPIO_PIN_MAP(0, 19)
#define LIS2DW12_SPI_MOSI NRF_GPIO_PIN_MAP(0, 22)

/**
 * +---------------------------------------------+ *
 * |     Pins connected to crystal oscillator    | *
 * +=============================================+ *
 * |  port&pin      |            Type            | *
 * +---------------------------------------------+ *
 * |  XC1           |           32 MHz           | *
 * +---------------------------------------------+ *
 * |  XC2           |           32 MHz           | *
 * +---------------------------------------------+ *
 **/
#define EXTERNAL_32MHZ_CRYSTAL 1

#ifdef __cplusplus
}
#endif /* end of __cplusplus */
