/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qsignal.h"

#include <zephyr.h>

#define LOG_TAG "qsignal"
#include "qlog.h"

struct qsignal {
	struct k_poll_signal signal;
	struct k_poll_event event;
};

#ifndef CONFIG_QOSAL_MAX_SIGNAL
#define CONFIG_QOSAL_MAX_SIGNAL 1
#endif

#define MEM_SLAB_ALIGNMENT 4

#ifndef K_MEM_SLAB_DEFINE_STATIC
/* To handle zephyr version without the flag */
#define K_MEM_SLAB_DEFINE_STATIC(a, b, c, d) static K_MEM_SLAB_DEFINE(a, b, c, d)
#endif

K_MEM_SLAB_DEFINE_STATIC(qosal_signal_slab, sizeof(struct qsignal), CONFIG_QOSAL_MAX_SIGNAL,
			 MEM_SLAB_ALIGNMENT);

struct qsignal *qsignal_init(void)
{
	struct qsignal *signal;
	int r = k_mem_slab_alloc(&qosal_signal_slab, (void **)&signal, K_NO_WAIT);
	if (r) {
		QLOGE("No signal available in memory slab");
		return NULL;
	}

	memset(signal, 0, sizeof(struct qsignal));
	k_poll_signal_init(&signal->signal);
	k_poll_event_init(&signal->event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &signal->signal);
	return signal;
}

void qsignal_deinit(struct qsignal *signal)
{
	if (!signal)
		return;

	k_mem_slab_free(&qosal_signal_slab, (void **)&signal);
}

enum qerr qsignal_raise(struct qsignal *signal, int value)
{
	int r;

	if (!signal)
		return QERR_EINVAL;

	r = k_poll_signal_raise(&signal->signal, value);
	return qerr_convert_os_to_qerr(r);
}

enum qerr qsignal_wait(struct qsignal *signal, int *value, uint32_t timeout_ms)
{
	int r;
	k_timeout_t timeout;

	if (!signal || !value)
		return QERR_EINVAL;

	if (timeout_ms == QOSAL_WAIT_FOREVER)
		timeout = K_FOREVER;
	else
		timeout = K_MSEC(timeout_ms);

	r = k_poll(&signal->event, 1, timeout);

	if (!r) {
		*value = signal->event.signal->result;
		signal->event.signal->signaled = 0;
		signal->event.state = K_POLL_STATE_NOT_READY;
	} else {
		*value = 0;
	}

	return qerr_convert_os_to_qerr(r);
}
