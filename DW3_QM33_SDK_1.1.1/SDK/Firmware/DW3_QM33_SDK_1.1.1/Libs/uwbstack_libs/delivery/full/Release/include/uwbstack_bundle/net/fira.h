/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qerr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * fira_test_crypto() - Run some tests on crypto mode supported.
 *
 * The goal of the function is to test the all key mode supported with other
 * libraries linked (Low layer hardware).
 * Run the FIRA CONSORTIUM UWB MAC TECHNICAL REQUIREMENTS version 1.3.0 test
 * vectors for Static STS and Dynamic STS (Provisioned STS is ran instead of
 * pure dynamic)
 *
 * Context: The function shall be called once and before any fira_init
 *          functions.
 *
 * Return: QERR_SUCCESS on success or error value otherwise.
 */
enum qerr fira_test_crypto(void);

/**
 * fira_init() - Registers FiRa protocol in the MAC.
 *
 * The protocol have to be register once before to be used.
 * Once registered, the protocol is ready to be instanciated.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_init(void);

/**
 * fira_exit() - Unregisters the FiRa protocol.
 *
 * All instanciations of this protocol can continue to run. Only new
 * instantiations of this protocol are no longer possible.
 */
void fira_exit(void);

#ifdef __cplusplus
}
#endif
