/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qerr.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct system_stats_t - Contains power statistics details for one state.
 */
struct system_stats_t {
	/**
	 * @start_time_us: current start time in us.
	 */
	uint64_t start_time_us;
	/**
	 *@duration_us: total duration of the state in us.
	 */
	uint64_t duration_us;
	/**
	 *@count: number of activations.
	 */
	uint32_t count;
};

/**
 * enum system_pwr_state - Power states.
 * @SYS_PWR_STATE_S1: S1 state.
 * @SYS_PWR_STATE_S3: S3 state.
 * @SYS_PWR_STATE_MAX: Number of power states.
 */
enum system_pwr_state { SYS_PWR_STATE_S1, SYS_PWR_STATE_S3, SYS_PWR_STATE_MAX };

/**
 * struct system_power_stats_t - Contains power statistics details for all states.
 */
struct system_power_stats_t {
	/**
	 * @state_stats: statistics for each state.
	 */
	struct system_stats_t state_stats[SYS_PWR_STATE_MAX];
};

/**
 * system_stats_start() - Start measuring the time spent in a power state.
 * @id: system power state id.
 */
void system_stats_start(enum system_pwr_state id);

/**
 * system_stats_stop() - Stop measuring the time spent in a power state.
 * @id: system power state id.
 */
void system_stats_stop(enum system_pwr_state id);

/**
 * system_stats_reset() - Reset power state statistics.
 *
 * Return: QERR_SUCCESS on success, error code otherwise.
 */
enum qerr system_stats_reset(void);

/**
 * system_stats_get_data() - Get power state statistics.
 * @sys_power_stats: pointer to the structure to be filled with statistics.
 *
 * Return: QERR_SUCCESS on success, error code otherwise.
 */
enum qerr system_stats_get_data(struct system_power_stats_t *sys_power_stats);

#ifdef __cplusplus
}
#endif
