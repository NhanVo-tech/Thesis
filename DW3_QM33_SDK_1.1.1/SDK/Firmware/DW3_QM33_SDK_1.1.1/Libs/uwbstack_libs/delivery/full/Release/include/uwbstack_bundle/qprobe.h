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
 * DOC: QPROBE_ERROR_OVERFLOW
 *
 * Bit used in per probe error variable to indicate when an overflow occurred.
 */
/* Bit for overflow indication in error value. */
#define QPROBE_ERROR_OVERFLOW 1 << 7
/* Number of histogram split. */
#define QPROBE_HISTO_BUCKET 4

/**
 * enum qprobe_id - Probe identificators.
 * @QPROBE_IDLE: To measure the time CPU spend in idle.
 * @QPROBE_L1_TX_TO_TX_DONE: To measure the time from L1 TX prep to frame is sent.
 * @QPROBE_L1_RX_TO_RX_DONE: To measure the time from L1 RX prep to frame is received
 * or error is detected.
 * @QPROBE_L1_READ_DIAGNOSTICS: To measure the time by L1 to read diagnostics data.
 * @QPROBE_ROUND_REPORT: To measure the time from end of round to end of last
 * UCI fragment transmission.
 * @QPROBE_KEY_DERIVATION: To measure the time for key derivation.
 * @QPROBE_FRAME_CIPHERING: To measure the time for frame ciphering.
 * @QPROBE_FRAME_DECIPHERING: To measure the time for frame deciphering.
 * @QPROBE_S3A: To measure the time taken for S3A entering to exiting and restoring
 * the system state.
 * @QPROBE_ID_MAX: Number of QPROBE defined.
 */
enum qprobe_id {
	QPROBE_IDLE,
	QPROBE_L1_TX_TO_TX_DONE,
	QPROBE_L1_RX_TO_RX_DONE,
	QPROBE_L1_READ_DIAGNOSTICS,
	QPROBE_ROUND_REPORT,
	QPROBE_KEY_DERIVATION,
	QPROBE_FRAME_CIPHERING,
	QPROBE_FRAME_DECIPHERING,
	QPROBE_S3A,
	QPROBE_ID_MAX,
};

/**
 * struct qprobe_probe_config_t - Per probe constant configuration
 *
 * Contain the probe's settings defined at compile time
 */
struct qprobe_probe_config_t {
	/** @name: Pointers to the probe names. */
	const char *name;
	/**
	 * @histo_separator_us: separator value in microseconds between
	 * histogram buckets
	 */
	uint32_t histo_separator_us[QPROBE_HISTO_BUCKET - 1];
};
/**
 * struct qprobe_config_t - Qprobes constant configuration
 *
 * Contain the probe's settings defined at compile time
 */
struct qprobe_config_t {
	/** @probe: probes static configuration */
	struct qprobe_probe_config_t probe[QPROBE_ID_MAX];
};

/**
 * struct qprobe_data_t - Probe runtime context and measurements
 */
struct qprobe_data_t {
	/**
	 * @start_time_us: Time in microseconds when the qprobe_start was
	 * called
	 */
	uint64_t start_time_us;
	/**
	 * @total_us: cumulative duration spend between qprobe's start and stop
	 */
	uint64_t total_us;
	/** @max_us: maximum duration in microseconds between start and stop */
	uint32_t max_us;
	/** @min_us: minimum duration in microseconds between start and stop */
	uint32_t min_us;
	/** @histo: number of duration in each histogram bucket */
	uint8_t histo[QPROBE_HISTO_BUCKET];
	/**
	 * @error: number of stop call without valid start_time.
	 *         when >= QPROBE_ERROR_OVERFLOW, a counter overflow happened
	 */
	uint8_t error;
};

/**
 * struct qprobe_runtime_t - Probe runtime context and measurements
 */
struct qprobe_runtime_t {
	/** @reset_time_us: boot time or last time when qprobe were reset */
	uint64_t reset_time_us;
	/** @probe: probes measurement and status */
	struct qprobe_data_t probe[QPROBE_ID_MAX];
};

/**
 * qprobe_start() - Save a start time for given probe
 * @id: identificator of the probe
 */
void qprobe_start(enum qprobe_id id);

/**
 * qprobe_stop() - Stop the given probe and compute time
 *
 * @id: identificator of the probe
 */
void qprobe_stop(enum qprobe_id id);

/**
 * qprobe_reset() - Reset all probes measurement
 *
 * Return: QERR_SUCCESS if reset succeed, or the error code.
 */
enum qerr qprobe_reset(void);

/**
 * qprobe_get_data() - Get the qprobe data
 * @runtime: pointer to the runtime data.
 * @config: pointer to the configuration data.
 *
 * Return: QERR_SUCCESS if get succeed, or the error code.
 */
enum qerr qprobe_get_data(struct qprobe_runtime_t *runtime, struct qprobe_config_t *config);

#ifdef __cplusplus
}
#endif
