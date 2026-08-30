/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qtracing.h"

#include <kernel.h>

qtracing_cb_t tracing_cb = NULL;

#if defined(CONFIG_LOG)
#include <logging/log_ctrl.h>

#define TRACE_TIMESTAMP_FREQ (1000000U)

#ifndef CONFIG_UWB_ZEPHYR_SPECIFIC
#include <logging/log.h>
#include <logging/log_core.h>
LOG_MODULE_REGISTER(tracing, LOG_LEVEL_INF);

static void qtracing_print_cb(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	z_log_msg2_runtime_vcreate(CONFIG_LOG_DOMAIN_ID,
#ifdef CONFIG_LOG_RUNTIME_FILTERING
				   __log_current_dynamic_data,
#else
				   __log_current_const_data,
#endif
				   LOG_LEVEL_INF, NULL, 0, 0, fmt, ap);
	va_end(ap);
}
#endif /* CONFIG_UWB_ZEPHYR_SPECIFIC. */

static log_timestamp_t qtracing_get_timestamp_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}
#endif /* CONFIG_LOG */

enum qerr qtracing_init(void)
{
#ifdef CONFIG_LOG
	log_set_timestamp_func(qtracing_get_timestamp_us, TRACE_TIMESTAMP_FREQ);
#ifndef CONFIG_UWB_ZEPHYR_SPECIFIC
	tracing_cb = qtracing_print_cb;
#endif
	return QERR_SUCCESS;
#else /* CONFIG_LOG. */
	return QERR_ENOTSUP;
#endif
}
