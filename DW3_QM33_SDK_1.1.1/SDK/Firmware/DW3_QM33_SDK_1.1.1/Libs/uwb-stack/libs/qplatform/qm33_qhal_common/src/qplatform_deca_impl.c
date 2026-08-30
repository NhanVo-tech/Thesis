/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "deca_device_api.h"
#include "qplatform.h"
#include "qplatform_internal.h"
#include "qtime.h"

/* Declare platform specific functions to be used by decadriver. Declared in deca_device_api.h */

void deca_sleep(unsigned int time_ms)
{
	qtime_msleep(time_ms);
}

void deca_usleep(unsigned long time_us)
{
	qtime_usleep(time_us);
}

decaIrqStatus_t decamutexon(void)
{
	uint32_t irq_pin_value = qplatform_uwb_read_irq_pin_value();
	if (irq_pin_value)
		qplatform_uwb_interrupt_disable();
	return irq_pin_value;
}

void decamutexoff(decaIrqStatus_t s)
{
	if (s)
		qplatform_uwb_interrupt_enable();
}
