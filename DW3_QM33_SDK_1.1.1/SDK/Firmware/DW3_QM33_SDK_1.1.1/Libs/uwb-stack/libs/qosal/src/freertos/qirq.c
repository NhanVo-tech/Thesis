/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qirq.h"

#include "FreeRTOS.h"
#include "qprivate.h"
#include "task.h"

unsigned int qirq_lock(void)
{
	vPortEnterCritical();
	return 0;
}

void qirq_unlock(unsigned int key)
{
	vPortExitCritical();
}
