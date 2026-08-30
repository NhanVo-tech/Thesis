/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qirq.h"

#include <irq.h>

unsigned int qirq_lock(void)
{
	return irq_lock();
}

void qirq_unlock(unsigned int key)
{
	irq_unlock(key);
}
