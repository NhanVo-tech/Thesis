/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#define QOSAL_PRINT_TRACE(...)

#if defined(NRF_LOG_ENABLED) && defined(NRF_LOG_BACKEND_RTT_ENABLED)

#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif
#define LOG_INTERNAL_8(type, todump...) nrf_log_frontend_std_0(type, "WARNING:Too long to print...")
#define LOG_INTERNAL_9(type, todump...) nrf_log_frontend_std_0(type, "WARNING:Too long to print...")

#include <nrf_log_internal.h>
#include <nrf_log_types.h>

#define QOSAL_IMPL_LOG_INFO(...) \
	LOG_INTERNAL(LOG_SEVERITY_MOD_ID(NRF_LOG_SEVERITY_INFO), ##__VA_ARGS__);
#define QOSAL_IMPL_LOG_ERR(...) \
	LOG_INTERNAL(LOG_SEVERITY_MOD_ID(NRF_LOG_SEVERITY_ERROR), ##__VA_ARGS__);
#define QOSAL_IMPL_LOG_WARN(...) \
	LOG_INTERNAL(LOG_SEVERITY_MOD_ID(NRF_LOG_SEVERITY_WARNING), ##__VA_ARGS__);
#define QOSAL_IMPL_LOG_DEBUG(...) \
	LOG_INTERNAL(LOG_SEVERITY_MOD_ID(NRF_LOG_SEVERITY_DEBUG), ##__VA_ARGS__);

#else /* defined(NRF_LOG_ENABLED) && defined(NRF_LOG_BACKEND_RTT_ENABLED). */

#define QOSAL_IMPL_LOG_INFO(...)
#define QOSAL_IMPL_LOG_ERR(...)
#define QOSAL_IMPL_LOG_WARN(...)
#define QOSAL_IMPL_LOG_DEBUG(...)

#endif
