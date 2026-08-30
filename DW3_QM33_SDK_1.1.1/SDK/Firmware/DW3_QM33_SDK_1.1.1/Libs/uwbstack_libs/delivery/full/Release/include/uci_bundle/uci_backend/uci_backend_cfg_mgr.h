/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#ifndef UCI_BACKEND_CFG_MGR_H
#define UCI_BACKEND_CFG_MGR_H

#include <uci/uci.h>

/**
 * uci_backend_cfg_mgr_register() - Register config manager uci backend.
 * @uci: UCI context.
 * @uwbmac_context: UWB MAC context.
 */
void uci_backend_cfg_mgr_register(struct uci *uci, struct uwbmac_context *uwbmac_context);

/**
 * uci_backend_cfg_mgr_unregister() - Unregister config manager uci backend.
 */
void uci_backend_cfg_mgr_unregister(void);

#endif // UCI_BACKEND_CFG_MGR_H
