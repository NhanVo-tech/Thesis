/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <uci/uci.h>

#if !defined(CONFIG_SE)
/**
 * uci_mctt_dynsts_backend_init() - Attach this backend to the uci context to
 * bridge uci communication to the MAC.
 * @uci: UCI context.
 *
 * Return: 0 or an error code.
 */
void uci_mctt_dynsts_backend_init(struct uci *uci);
#else
static inline void uci_mctt_dynsts_backend_init(struct uci *uci)
{
}
#endif
