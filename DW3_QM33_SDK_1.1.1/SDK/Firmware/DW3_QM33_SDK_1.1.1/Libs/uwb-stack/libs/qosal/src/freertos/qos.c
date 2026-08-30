/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qos.h"

#include "FreeRTOS.h"
#include "task.h"

enum qerr qos_start(void)
{
	vTaskStartScheduler();

	return QERR_SUCCESS;
}
