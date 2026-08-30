/**
 * @file      app_ble.h
 *
 * @brief     Functions ble app
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ACCESSORY_RANGING_ROLE
#define ACCESSORY_RANGING_ROLE (0) /**< Responder 0, Initiator 1 */
#endif

void send_qnis_data(uint16_t conn_handle, uint8_t *buffer, uint16_t data_len);
void handle_niq_data(uint16_t conn_handle, const uint8_t *data, int data_len);
void ble_evt_disconnected_handler(uint16_t conn_handle);
void ble_evt_connected_handler(uint16_t conn_handle);
