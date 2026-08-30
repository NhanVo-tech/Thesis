/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qerr.h"

#include "FreeRTOS.h"

enum qerr qerr_convert_os_to_qerr(int error)
{
	return (error == pdFAIL) ? QERR_EIO : QERR_SUCCESS;
}

int qerr_convert_qerr_to_os(enum qerr error)
{
	return (error == QERR_SUCCESS) ? pdPASS : pdFAIL;
}
