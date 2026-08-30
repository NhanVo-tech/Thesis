/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "uci/uci_message.h"

#include <uwbmac/fira_helper.h>

/**
 * enum meas_seq_template_type - Measurement step type.
 *
 * @TEMPLATE_RANGING:
 *     Ranging measure.
 * @TEMPLATE_COMBO:
 *     Both azimuth and elevation measure.
 * @TEMPLATE_AZIMUTH:
 *     Azimuth measure.
 * @TEMPLATE_ELEVATION:
 *     Elevation measure.
 * @TEMPLATE_NB:
 *     Internal use.
 */
enum meas_seq_template_type {
	TEMPLATE_RANGING = 0,
	TEMPLATE_COMBO = 1,
	TEMPLATE_AZIMUTH = 2,
	TEMPLATE_ELEVATION = 3,
	TEMPLATE_NB
};

/**
 * enum aoa_result_req - Angle of Arrival result request.
 *
 * @AOA_RESULT_REQ_DISABLE:
 *     Disable Angle of arrival results.
 * @AOA_RESULT_REQ_ENABLE:
 *     Enable Angle of arrival results of all angles.
 * @AOA_RESULT_REQ_AZIMUTH:
 *     Enable Angle of arrival results of azimuth.
 * @AOA_RESULT_REQ_ELEVATION:
 *     Enable Angle of arrival results of elevation.
 * @AOA_RESULT_REQ_INTERLEAVE:
 *     Enable Angle of arrival results according to measurements requests.
 */
enum aoa_result_req {
	AOA_RESULT_REQ_DISABLE,
	AOA_RESULT_REQ_ENABLE,
	AOA_RESULT_REQ_AZIMUTH,
	AOA_RESULT_REQ_ELEVATION,
	AOA_RESULT_REQ_INTERLEAVE = 0XF0,
};

/**
 * struct antenna_parameters - Definition of antennas parameters.
 */
struct antenna_parameters {
	/**
	 * @ch5: Measurement sequence step for channel 5.
	 */
	struct fira_measurement_sequence_step ch5[TEMPLATE_NB];
	/**
	 * @ch9: Measurement sequence step for channel 9.
	 */
	struct fira_measurement_sequence_step ch9[TEMPLATE_NB];
	/**
	 * @aoa_capability: Angle Of Arrival capability: 0 (no AOA), 1 (Azimuth
	 * only), 2 (Azimuth and Elevation).
	 */
	uint8_t aoa_capability;
};

enum qerr uci_backend_set_antenna_flex_config(
	struct uci *uci, struct antenna_parameters *antennas, struct uci_message_parser *parser,
	struct uci_message_builder *builder, uint16_t rsp_mt_gid_oid,
	int8_t *(*get_antenna_flex_key_address)(struct antenna_parameters *, uint8_t, uint8_t));

enum qerr uci_backend_get_antenna_flex_config(
	struct uci *uci, struct antenna_parameters *antennas, struct uci_message_parser *parser,
	struct uci_message_builder *builder, uint16_t rsp_mt_gid_oid,
	int8_t *(*get_antenna_flex_key_address)(struct antenna_parameters *, uint8_t, uint8_t));
