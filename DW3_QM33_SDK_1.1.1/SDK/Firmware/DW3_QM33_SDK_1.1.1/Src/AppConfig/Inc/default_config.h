/**
 * @file      default_config.h
 *
 * @brief     Default config file for NVM initialization
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

#include "app.h"

extern const app_definition_t idle_app[];

/* UWB config. */
#define APP_DEFAULT_CHANNEL        9
#define APP_DEFAULT_TXPREAMBLENGTH DWT_PLEN_64
#define APP_DEFAULT_RXPAC          DWT_PAC8
#define APP_DEFAULT_PCODE          10
#define APP_DEFAULT_NSSFD          DWT_SFD_IEEE_4Z
#define APP_DEFAULT_DATARATE       DWT_BR_6M8
#define APP_DEFAULT_PHRMODE        DWT_PHRMODE_STD
#define APP_DEFAULT_PHRRATE        DWT_PHRRATE_STD
#define APP_DEFAULT_SFDTO          (64 + 1 + 8 - 8)
#define APP_DEFAULT_STS_MODE       DWT_STS_MODE_OFF
#define APP_DEFAULT_STS_LENGTH     DWT_STS_LEN_64

/* 1 to re-load STS Key & IV after each Rx & Tx:: Listener, InitN, RespN. */
#define DEFAULT_STS_STATIC 1

/* Run-time config */
#define DEFAULT_DIAG_READING 0
#define DEFAULT_DEBUG        0 /* If 1, then the LED_RED used to show an error, if any. */

#define UWB_SYS_TIME_UNIT    (1.0 / 499.2e6 / 128.0)
#define DEFAULT_ANTD_BASE    (513.484f * 1e-9 / UWB_SYS_TIME_UNIT) /* Total antenna delay. */
#define DEFAULT_ANTD         (uint16_t)(0.5 * DEFAULT_ANTD_BASE)

/* NVM PAGE for store configuration. */
#define FCONFIG_SIZE 0x400 /* Can be up to 0x1000. */

#ifdef __cplusplus
}
#endif
