/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "FreeRTOS.h"
#include "qirq.h"
#include "task.h"

#include <stdbool.h>

bool qprivate_is_in_isr()
{
	uint32_t ipsr_register;

	/* Read IPSR register. */
	__asm__ volatile("MRS %0, ipsr" : "=r"(ipsr_register));

	return !!ipsr_register;
}
