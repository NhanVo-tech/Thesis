/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "quwbs/msg.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * This file is generated automatically, edit with care.
 */

/**
 * enum quwbs_fira_session_state_reason - Reason for session state change.
 */
enum quwbs_fira_session_state_reason {
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_COMMAND: Change requested using session management command.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_COMMAND = 0,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_MAX_RETRY: Stopped due to maximum attempts reached with no response.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_MAX_RETRY = 1,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_MAX_MEASUREMENT: Stopped due to maximum number of measurements reached.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_MAX_MEASUREMENT = 2,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_SUSPENDED_IN_BAND: Session is still active but suspended due to in-band signaling.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_SUSPENDED_IN_BAND = 3,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_RESUMED_IN_BAND: Session is resumed due to in-band signaling.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_RESUMED_IN_BAND = 3,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_STOPPED_IN_BAND: Stopped due to in-band signaling from the controller.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_STOPPED_IN_BAND = 5,
	/**
	 * @QUWBS_FIRA_SESSION_STATE_REASON_ERROR_INVALID_OWR_UT_RANDOM_WINDOW: OWR UL-TDoA random window is larger than Tx interval.
	 */
	QUWBS_FIRA_SESSION_STATE_REASON_ERROR_INVALID_OWR_UT_RANDOM_WINDOW = 29,
};

/**
 * enum quwbs_fira_measurement_status - Measurement status.
 */
enum quwbs_fira_measurement_status {
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_SUCCESS: Success.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_SUCCESS,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_TX_FAILED: Tx failed.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_TX_FAILED,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_TIMEOUT: Rx timeout.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_TIMEOUT,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_DEC_ERROR: Rx PHY decoding error.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_DEC_ERROR,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_TOA_ERROR: Rx PHY ToA error.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_TOA_ERROR,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_STS_ERROR: Rx PHY STS error.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_PHY_STS_ERROR,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_MAC_DEC_ERROR: Rx MAC decoding error.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_MAC_DEC_ERROR,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_STATUS_RX_MAC_IE_MISSING: Rx MAC IE missing.
	 */
	QUWBS_FIRA_MEASUREMENT_STATUS_RX_MAC_IE_MISSING,
};

/**
 * struct quwbs_fira_aoa_measurements - AoA measurements.
 */
struct quwbs_fira_aoa_measurements {
	/**
	 * @rx_antenna_set: Antenna set used for the measurement.
	 */
	uint8_t rx_antenna_set;
	/**
	 * @aoa_2pi: Estimation of reception angle.
	 */
	int16_t aoa_2pi;
	/**
	 * @aoa_fom_100: Estimation of AoA reliability.
	 */
	uint8_t aoa_fom_100;
	/**
	 * @pdoa_2pi: Estimation of reception phase difference.
	 */
	int16_t pdoa_2pi;
};

/**
 * enum quwbs_fira_los - Line of sight indicator.
 */
enum quwbs_fira_los {
	/**
	 * @QUWBS_FIRA_LOS_NLOS: Non-line of sight.
	 */
	QUWBS_FIRA_LOS_NLOS = 0,
	/**
	 * @QUWBS_FIRA_LOS_LOS: Line of sight.
	 */
	QUWBS_FIRA_LOS_LOS = 1,
	/**
	 * @QUWBS_FIRA_LOS_UNDETERMINED: Unable to determine.
	 */
	QUWBS_FIRA_LOS_UNDETERMINED = 255,
};

/**
 * enum quwbs_fira_measurement_type - Type of ranging measurement.
 */
enum quwbs_fira_measurement_type {
	/**
	 * @QUWBS_FIRA_MEASUREMENT_TYPE_OWR_UT: OWR UL-TDoA measurement.
	 */
	QUWBS_FIRA_MEASUREMENT_TYPE_OWR_UT = 0,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_TYPE_TWR: TWR measurement.
	 */
	QUWBS_FIRA_MEASUREMENT_TYPE_TWR = 1,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_TYPE_OWR_DT: OWR DL-TDoA measurement.
	 */
	QUWBS_FIRA_MEASUREMENT_TYPE_OWR_DT = 2,
	/**
	 * @QUWBS_FIRA_MEASUREMENT_TYPE_OWR_FOR_AOA: OWR for AoA measurement.
	 */
	QUWBS_FIRA_MEASUREMENT_TYPE_OWR_FOR_AOA = 3,
};

/**
 * struct quwbs_fira_twr_measurements - TWR measurements.
 */
struct quwbs_fira_twr_measurements {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	struct quwbs_fira_twr_measurements *next;
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @stopped: Ranging was stopped as requested.
	 */
	bool stopped;
	/**
	 * @status: Measurement status.
	 */
	enum quwbs_fira_measurement_status status;
	/**
	 * @slot_index: In case of failure, slot index where the error was detected.
	 */
	uint8_t slot_index;
	/**
	 * @los: Line of sight indicator.
	 */
	enum quwbs_fira_los los;
	/**
	 * @distance_cm: Distance in cm.
	 */
	int32_t distance_cm;
	/**
	 * @local_aoa_azimuth: Estimation of reception angle in the azimuth.
	 */
	struct quwbs_fira_aoa_measurements *local_aoa_azimuth;
	/**
	 * @local_aoa_elevation: Estimation of reception angle in the elevation.
	 */
	struct quwbs_fira_aoa_measurements *local_aoa_elevation;
	/**
	 * @remote_aoa_azimuth_2pi: Estimation of reception angle in the azimuth of the participating device.
	 */
	int16_t remote_aoa_azimuth_2pi;
	/**
	 * @remote_aoa_elevation_pi: Estimation of reception angle in the elevation of the participating device.
	 */
	int16_t remote_aoa_elevation_pi;
	/**
	 * @remote_aoa_azimuth_fom_100: Estimation of azimuth reliability of the participating device.
	 */
	uint8_t remote_aoa_azimuth_fom_100;
	/**
	 * @remote_aoa_elevation_fom_100: Estimation of elevation reliability of the participating device.
	 */
	uint8_t remote_aoa_elevation_fom_100;
};

/**
 * struct quwbs_fira_owr_aoa_measurements - OWR for AoA measurements.
 */
struct quwbs_fira_owr_aoa_measurements {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	struct quwbs_fira_owr_aoa_measurements *next;
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @status: Measurement status.
	 */
	enum quwbs_fira_measurement_status status;
	/**
	 * @los: Line of sight indicator.
	 */
	enum quwbs_fira_los los;
	/**
	 * @frame_sequence_number: Sequence number as received in MHR.
	 */
	uint8_t frame_sequence_number;
	/**
	 * @block_index: Block index number as received in the OWR message from the advertiser.
	 */
	uint16_t block_index;
	/**
	 * @local_aoa_azimuth: Estimation of reception angle in the azimuth.
	 */
	struct quwbs_fira_aoa_measurements *local_aoa_azimuth;
	/**
	 * @local_aoa_elevation: Estimation of reception angle in the elevation.
	 */
	struct quwbs_fira_aoa_measurements *local_aoa_elevation;
};

/**
 * struct quwbs_fira_owr_dt_measurements - OWR DL-TDOA measurements.
 */
struct quwbs_fira_owr_dt_measurements {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	struct quwbs_fira_owr_dt_measurements *next;
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @status: Measurement status.
	 */
	enum quwbs_fira_measurement_status status;
};

/**
 * enum quwbs_fira_capabilities_tag - Device capabilities variant tag.
 */
enum quwbs_fira_capabilities_tag {
	/**
	 * @QUWBS_FIRA_CAPABILITIES_TAG_PHY_VERSION_RANGE: FiRa PHY version range supported.
	 *
	 * Associated variant type: &struct quwbs_fira_capabilities_variant_u32.
	 */
	QUWBS_FIRA_CAPABILITIES_TAG_PHY_VERSION_RANGE = 2,
	/**
	 * @QUWBS_FIRA_CAPABILITIES_TAG_MAC_VERSION_RANGE: FiRa MAC version range supported.
	 *
	 * Associated variant type: &struct quwbs_fira_capabilities_variant_u32.
	 */
	QUWBS_FIRA_CAPABILITIES_TAG_MAC_VERSION_RANGE = 3,
	/**
	 * @QUWBS_FIRA_CAPABILITIES_TAG_DEVICE_TYPE: Supported device types.
	 *
	 * Associated variant type: &struct quwbs_fira_capabilities_variant_u8.
	 */
	QUWBS_FIRA_CAPABILITIES_TAG_DEVICE_TYPE = 4,
	/**
	 * @QUWBS_FIRA_CAPABILITIES_TAG_DEVICE_ROLE: Supported device roles.
	 *
	 * Associated variant type: &struct quwbs_fira_capabilities_variant_u8.
	 */
	QUWBS_FIRA_CAPABILITIES_TAG_DEVICE_ROLE = 5,
};

union quwbs_fira_capabilities;

/**
 * struct quwbs_fira_capabilities_base - Device capabilities variant base structure.
 */
struct quwbs_fira_capabilities_base {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_capabilities *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_capabilities_tag tag;
};

/**
 * struct quwbs_fira_capabilities_variant_u32 - Device capabilities variant for u32.
 */
struct quwbs_fira_capabilities_variant_u32 {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_capabilities *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_capabilities_tag tag;
	/**
	 * @value: Variant value.
	 */
	uint32_t value;
};

/**
 * struct quwbs_fira_capabilities_variant_u8 - Device capabilities variant for u8.
 */
struct quwbs_fira_capabilities_variant_u8 {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_capabilities *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_capabilities_tag tag;
	/**
	 * @value: Variant value.
	 */
	uint8_t value;
};

/**
 * struct quwbs_fira_capabilities - Device capabilities.
 */
union quwbs_fira_capabilities {
	/**
	 * @base: Base structure.
	 */
	struct quwbs_fira_capabilities_base base;
	/**
	 * @phy_version_range: FiRa PHY version range supported.
	 *
	 * - b0-b7 = lower bound major version.
	 * - b8-b15 = lower bound minor version.
	 * - b16-b23 = higher bound major version.
	 * - b24-b31 = higher bound minor version.
	 */
	struct quwbs_fira_capabilities_variant_u32 phy_version_range;
	/**
	 * @mac_version_range: FiRa MAC version range supported.
	 *
	 * - b0-b7 = lower bound major version.
	 * - b8-b15 = lower bound minor version.
	 * - b16-b23 = higher bound major version.
	 * - b24-b31 = higher bound minor version.
	 */
	struct quwbs_fira_capabilities_variant_u32 mac_version_range;
	/**
	 * @device_type: Supported device types.
	 *
	 * - b0 = controller
	 * - b1 = controlee
	 */
	struct quwbs_fira_capabilities_variant_u8 device_type;
	/**
	 * @device_role: Supported device roles.
	 *
	 * - b0 = responder
	 * - b1 = initiator
	 * - b2 = OWR UL-TDoA synchronization anchor
	 * - b3 = OWR UL-TDoA anchor
	 * - b4 = OWR UL-TDoA tag
	 * - b5 = advertiser
	 * - b6 = OWR DL-TDoA anchor
	 * - b7 = OWR DL-TDoA tag
	 */
	struct quwbs_fira_capabilities_variant_u8 device_role;
};

/**
 * enum quwbs_fira_ranging_measurements_tag - One type of ranging measurements variant tag.
 */
enum quwbs_fira_ranging_measurements_tag {
	/**
	 * @QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_TWR_MEASUREMENTS: TWR measurements.
	 *
	 * Associated variant type: &struct quwbs_fira_ranging_measurements_variant_twr_measurements_list.
	 */
	QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_TWR_MEASUREMENTS = 1,
	/**
	 * @QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_OWR_AOA_MEASUREMENTS: OWR for AoA measurements.
	 *
	 * Associated variant type: &struct quwbs_fira_ranging_measurements_variant_owr_aoa_measurements_list.
	 */
	QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_OWR_AOA_MEASUREMENTS = 3,
	/**
	 * @QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_OWR_DT_MEASUREMENTS: OWR DL-TDOA measurements.
	 *
	 * Associated variant type: &struct quwbs_fira_ranging_measurements_variant_owr_dt_measurements_list.
	 */
	QUWBS_FIRA_RANGING_MEASUREMENTS_TAG_OWR_DT_MEASUREMENTS = 2,
};

/**
 * struct quwbs_fira_ranging_measurements_base - One type of ranging measurements variant base structure.
 */
struct quwbs_fira_ranging_measurements_base {
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_ranging_measurements_tag tag;
};

/**
 * struct quwbs_fira_ranging_measurements_variant_owr_aoa_measurements_list - One type of ranging measurements variant for owr_aoa_measurements_list.
 */
struct quwbs_fira_ranging_measurements_variant_owr_aoa_measurements_list {
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_ranging_measurements_tag tag;
	/**
	 * @value: Variant value.
	 */
	struct quwbs_fira_owr_aoa_measurements *value;
};

/**
 * struct quwbs_fira_ranging_measurements_variant_owr_dt_measurements_list - One type of ranging measurements variant for owr_dt_measurements_list.
 */
struct quwbs_fira_ranging_measurements_variant_owr_dt_measurements_list {
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_ranging_measurements_tag tag;
	/**
	 * @value: Variant value.
	 */
	struct quwbs_fira_owr_dt_measurements *value;
};

/**
 * struct quwbs_fira_ranging_measurements_variant_twr_measurements_list - One type of ranging measurements variant for twr_measurements_list.
 */
struct quwbs_fira_ranging_measurements_variant_twr_measurements_list {
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_ranging_measurements_tag tag;
	/**
	 * @value: Variant value.
	 */
	struct quwbs_fira_twr_measurements *value;
};

/**
 * struct quwbs_fira_ranging_measurements - One type of ranging measurements.
 */
union quwbs_fira_ranging_measurements {
	/**
	 * @base: Base structure.
	 */
	struct quwbs_fira_ranging_measurements_base base;
	/**
	 * @twr_measurements: TWR measurements.
	 */
	struct quwbs_fira_ranging_measurements_variant_twr_measurements_list twr_measurements;
	/**
	 * @owr_aoa_measurements: OWR for AoA measurements.
	 */
	struct quwbs_fira_ranging_measurements_variant_owr_aoa_measurements_list owr_aoa_measurements;
	/**
	 * @owr_dt_measurements: OWR DL-TDOA measurements.
	 */
	struct quwbs_fira_ranging_measurements_variant_owr_dt_measurements_list owr_dt_measurements;
};

/**
 * struct quwbs_fira_ranging_data - Session ranging data.
 */
struct quwbs_fira_ranging_data {
	/**
	 * @session_state: Current session state.
	 */
	enum quwbs_fbs_session_state session_state;
	/**
	 * @session_state_reason: Reason for current session state.
	 */
	enum quwbs_fira_session_state_reason session_state_reason;
	/**
	 * @block_index: Current block index.
	 */
	uint32_t block_index;
	/**
	 * @timestamp_ns: Timestamp in nanoseconds in MAC time reference.
	 */
	uint64_t timestamp_ns;
	/**
	 * @ranging_interval_ms: Current ranging interval.
	 *
	 * This is (block size * (stride + 1)).
	 */
	uint32_t ranging_interval_ms;
	/**
	 * @measurement_type: Type of ranging measurement.
	 */
	enum quwbs_fira_measurement_type measurement_type;
	/**
	 * @measurements: List of ranging measurements.
	 */
	union quwbs_fira_ranging_measurements *measurements;
};

/**
 * enum quwbs_fira_ranging_round_usage - Ranging round usage.
 */
enum quwbs_fira_ranging_round_usage {
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_UT: OWR UL-TDoA.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_UT = 0,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_SS_TWR_DEFERRED: SS-TWR with deferred mode.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_SS_TWR_DEFERRED = 1,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_DS_TWR_DEFERRED: DS-TWR with deferred mode.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_DS_TWR_DEFERRED = 2,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_SS_TWR_NON_DEFERRED: SS-TWR with non-deferred mode.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_SS_TWR_NON_DEFERRED = 3,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_DS_TWR_NON_DEFERRED: DS-TWR with non-deferred mode.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_DS_TWR_NON_DEFERRED = 4,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_DT: OWR DL-TDoA.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_DT = 5,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_AOA: OWR for AoA.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_OWR_AOA = 6,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED: eSS-TWR with non-deferred mode for contention-based ranging.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED = 7,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED: aDS-TWR for contention-based ranging.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED = 8,
	/**
	 * @QUWBS_FIRA_RANGING_ROUND_USAGE_UNDEFINED: Ranging round usage is undefined.
	 */
	QUWBS_FIRA_RANGING_ROUND_USAGE_UNDEFINED = 255,
};

/**
 * enum quwbs_fira_multi_node_mode - Multi-node mode.
 */
enum quwbs_fira_multi_node_mode {
	/**
	 * @QUWBS_FIRA_MULTI_NODE_MODE_UNICAST: Unicast.
	 */
	QUWBS_FIRA_MULTI_NODE_MODE_UNICAST = 0,
	/**
	 * @QUWBS_FIRA_MULTI_NODE_MODE_ONE_TO_MANY: One to many.
	 */
	QUWBS_FIRA_MULTI_NODE_MODE_ONE_TO_MANY = 1,
	/**
	 * @QUWBS_FIRA_MULTI_NODE_MODE_MANY_TO_MANY: Many to many.
	 */
	QUWBS_FIRA_MULTI_NODE_MODE_MANY_TO_MANY = 2,
	/**
	 * @QUWBS_FIRA_MULTI_NODE_MODE_UNDEFINED: Multi-node mode is undefined.
	 */
	QUWBS_FIRA_MULTI_NODE_MODE_UNDEFINED = 255,
};

/**
 * enum quwbs_fira_schedule_mode - Slot scheduling mode.
 */
enum quwbs_fira_schedule_mode {
	/**
	 * @QUWBS_FIRA_SCHEDULE_MODE_CONTENTION_BASED: Contention-based.
	 */
	QUWBS_FIRA_SCHEDULE_MODE_CONTENTION_BASED = 1,
	/**
	 * @QUWBS_FIRA_SCHEDULE_MODE_TIME_SCHEDULED: Time-scheduled.
	 */
	QUWBS_FIRA_SCHEDULE_MODE_TIME_SCHEDULED = 2,
	/**
	 * @QUWBS_FIRA_SCHEDULE_MODE_HYBRID_BASED: Hybrid-based.
	 */
	QUWBS_FIRA_SCHEDULE_MODE_HYBRID_BASED = 3,
	/**
	 * @QUWBS_FIRA_SCHEDULE_MODE_UNDEFINED: Scheduling mode is undefined.
	 */
	QUWBS_FIRA_SCHEDULE_MODE_UNDEFINED = 255,
};

/**
 * enum quwbs_fira_session_param_tag - Session parameter variant tag.
 */
enum quwbs_fira_session_param_tag {
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_DEVICE_TYPE: Controlee or controller.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_device_type.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_DEVICE_TYPE,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_DEVICE_ROLE: Role played by the device.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_device_role.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_DEVICE_ROLE,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_RANGING_ROUND_USAGE: Ranging round usage.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_ranging_round_usage.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_RANGING_ROUND_USAGE,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_MULTI_NODE_MODE: Multi-node mode.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_multi_node_mode.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_MULTI_NODE_MODE,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_SHORT_ADDR: Device address for this session.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u16.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_SHORT_ADDR,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_DESTINATION_SHORT_ADDR: Controller short address.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u16.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_DESTINATION_SHORT_ADDR,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_SCHEDULE_MODE: Slot scheduling mode.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_schedule_mode.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_SCHEDULE_MODE,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_TIME0_NS: Initiation time in nanoseconds in MAC time reference.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u64.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_TIME0_NS,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_SLOT_DURATION_RSTU: Duration of a slot in RSTU.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u32.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_SLOT_DURATION_RSTU,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_BLOCK_DURATION_MS: Block size in unit of 1200 RSTU (same as ms).
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u32.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_BLOCK_DURATION_MS,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_ROUND_DURATION_SLOTS: Number of slots per ranging round.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u32.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_ROUND_DURATION_SLOTS,
	/**
	 * @QUWBS_FIRA_SESSION_PARAM_TAG_BLOCK_STRIDE_LENGTH: Number of block to stride.
	 *
	 * Associated variant type: &struct quwbs_fira_session_param_variant_u32.
	 */
	QUWBS_FIRA_SESSION_PARAM_TAG_BLOCK_STRIDE_LENGTH,
};

union quwbs_fira_session_param;

/**
 * struct quwbs_fira_session_param_base - Session parameter variant base structure.
 */
struct quwbs_fira_session_param_base {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
};

/**
 * struct quwbs_fira_session_param_variant_device_role - Session parameter variant for device_role.
 */
struct quwbs_fira_session_param_variant_device_role {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	enum quwbs_fbs_device_role value;
};

/**
 * struct quwbs_fira_session_param_variant_device_type - Session parameter variant for device_type.
 */
struct quwbs_fira_session_param_variant_device_type {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	enum quwbs_fbs_device_type value;
};

/**
 * struct quwbs_fira_session_param_variant_multi_node_mode - Session parameter variant for multi_node_mode.
 */
struct quwbs_fira_session_param_variant_multi_node_mode {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	enum quwbs_fira_multi_node_mode value;
};

/**
 * struct quwbs_fira_session_param_variant_ranging_round_usage - Session parameter variant for ranging_round_usage.
 */
struct quwbs_fira_session_param_variant_ranging_round_usage {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	enum quwbs_fira_ranging_round_usage value;
};

/**
 * struct quwbs_fira_session_param_variant_schedule_mode - Session parameter variant for schedule_mode.
 */
struct quwbs_fira_session_param_variant_schedule_mode {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	enum quwbs_fira_schedule_mode value;
};

/**
 * struct quwbs_fira_session_param_variant_u16 - Session parameter variant for u16.
 */
struct quwbs_fira_session_param_variant_u16 {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	uint16_t value;
};

/**
 * struct quwbs_fira_session_param_variant_u32 - Session parameter variant for u32.
 */
struct quwbs_fira_session_param_variant_u32 {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	uint32_t value;
};

/**
 * struct quwbs_fira_session_param_variant_u64 - Session parameter variant for u64.
 */
struct quwbs_fira_session_param_variant_u64 {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	union quwbs_fira_session_param *next;
	/**
	 * @tag: Variant tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @value: Variant value.
	 */
	uint64_t value;
};

/**
 * struct quwbs_fira_session_param - Session parameter.
 */
union quwbs_fira_session_param {
	/**
	 * @base: Base structure.
	 */
	struct quwbs_fira_session_param_base base;
	/**
	 * @device_type: Controlee or controller.
	 */
	struct quwbs_fira_session_param_variant_device_type device_type;
	/**
	 * @device_role: Role played by the device.
	 */
	struct quwbs_fira_session_param_variant_device_role device_role;
	/**
	 * @ranging_round_usage: Ranging round usage.
	 */
	struct quwbs_fira_session_param_variant_ranging_round_usage ranging_round_usage;
	/**
	 * @multi_node_mode: Multi-node mode.
	 */
	struct quwbs_fira_session_param_variant_multi_node_mode multi_node_mode;
	/**
	 * @short_addr: Device address for this session.
	 */
	struct quwbs_fira_session_param_variant_u16 short_addr;
	/**
	 * @destination_short_addr: Controller short address.
	 */
	struct quwbs_fira_session_param_variant_u16 destination_short_addr;
	/**
	 * @schedule_mode: Slot scheduling mode.
	 */
	struct quwbs_fira_session_param_variant_schedule_mode schedule_mode;
	/**
	 * @time0_ns: Initiation time in nanoseconds in MAC time reference.
	 *
	 * Use 0 for immediate, determined by MAC
	 */
	struct quwbs_fira_session_param_variant_u64 time0_ns;
	/**
	 * @slot_duration_rstu: Duration of a slot in RSTU.
	 *
	 * UCI: ``SLOT_DURATION``
	 */
	struct quwbs_fira_session_param_variant_u32 slot_duration_rstu;
	/**
	 * @block_duration_ms: Block size in unit of 1200 RSTU (same as ms).
	 *
	 * UCI: ``RANGING_INTERVAL``
	 */
	struct quwbs_fira_session_param_variant_u32 block_duration_ms;
	/**
	 * @round_duration_slots: Number of slots per ranging round.
	 *
	 * UCI: ``SLOTS_PER_RR``
	 */
	struct quwbs_fira_session_param_variant_u32 round_duration_slots;
	/**
	 * @block_stride_length: Number of block to stride.
	 *
	 * Can be changed when the session is active
	 */
	struct quwbs_fira_session_param_variant_u32 block_stride_length;
};

/**
 * struct quwbs_fira_session_param_tags - Array of session parameter tags.
 */
struct quwbs_fira_session_param_tags {
	/**
	 * @next: Next fragment or NULL if last one.
	 */
	struct quwbs_fira_session_param_tags *next;
	/**
	 * @len: Length of fragment, number of elements.
	 */
	size_t len;
	/**
	 * @items: Pointer to items of fragment.
	 */
	enum quwbs_fira_session_param_tag *items;
};

/**
 * struct quwbs_fira_session_param_status - Session parameter setting status.
 */
struct quwbs_fira_session_param_status {
	/**
	 * @next: Next element in list or NULL if last one.
	 */
	struct quwbs_fira_session_param_status *next;
	/**
	 * @tag: Session parameter tag.
	 */
	enum quwbs_fira_session_param_tag tag;
	/**
	 * @status: Setting status.
	 */
	enum quwbs_fbs_status status;
};

/**
 * enum quwbs_fira_call_id - FiRa call identifier.
 */
enum quwbs_fira_call_id {
	/**
	 * @QUWBS_FIRA_CALL_ID_GET_CAPABILITIES: Get capabilities information.
	 */
	QUWBS_FIRA_CALL_ID_GET_CAPABILITIES,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_INIT: Create a new session.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_INIT,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_DEINIT: Delete a session.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_DEINIT,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_START: Start a session.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_START,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_STOP: Stop a session.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_STOP,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_GET_STATE: Get session current state.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_GET_STATE,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_DATA_SEND: Send data message.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_DATA_SEND,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_GET_COUNT: Get number of sessions.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_GET_COUNT,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_SET_PARAMS: Set session parameters.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_SET_PARAMS,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_GET_PARAMS: Get session parameters.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_GET_PARAMS,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_STATUS: Session status notification.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_STATUS,
	/**
	 * @QUWBS_FIRA_CALL_ID_SESSION_INFO: Session information notification.
	 */
	QUWBS_FIRA_CALL_ID_SESSION_INFO,
};


/**
 * struct quwbs_fira_msg_cmd_get_capabilities - Get capabilities information command message.
 */
struct quwbs_fira_msg_cmd_get_capabilities {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
};

/**
 * struct quwbs_fira_returns_get_capabilities - Get capabilities information.
 */
struct quwbs_fira_returns_get_capabilities {
	/**
	 * @capabilities: Device capabilities.
	 */
	union quwbs_fira_capabilities *capabilities;
};

/**
 * struct quwbs_fira_msg_rsp_get_capabilities - Get capabilities information response message.
 */
struct quwbs_fira_msg_rsp_get_capabilities {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_get_capabilities *params;
};

/**
 * struct quwbs_fira_params_session_init - Create a new session.
 *
 * Create a new session and return a session handle to be used for
 * subsequent session commands.
 *
 * On success, the session will be created and a session status
 * notification will be sent to signal the new session state once done.
 * This can still fails, in this case, the session status notification will
 * signal the failure.
 *
 * On failure, the session is not created and an error status is returned.
 */
struct quwbs_fira_params_session_init {
	/**
	 * @session_id: Session identifier.
	 */
	uint32_t session_id;
	/**
	 * @type: Type of the session.
	 */
	enum quwbs_fbs_session_type type;
};

/**
 * struct quwbs_fira_msg_cmd_session_init - Create a new session command message.
 */
struct quwbs_fira_msg_cmd_session_init {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_init *params;
};

/**
 * struct quwbs_fira_returns_session_init - Create a new session.
 */
struct quwbs_fira_returns_session_init {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @session_handle: New session handle.
	 */
	uint32_t session_handle;
};

/**
 * struct quwbs_fira_msg_rsp_session_init - Create a new session response message.
 */
struct quwbs_fira_msg_rsp_session_init {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_init *params;
};

/**
 * struct quwbs_fira_params_session_deinit - Delete a session.
 *
 * Initiate session deletion. Once done, a session status notification is
 * sent.
 */
struct quwbs_fira_params_session_deinit {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
};

/**
 * struct quwbs_fira_msg_cmd_session_deinit - Delete a session command message.
 */
struct quwbs_fira_msg_cmd_session_deinit {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_deinit *params;
};

/**
 * struct quwbs_fira_returns_session_deinit - Delete a session.
 */
struct quwbs_fira_returns_session_deinit {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
};

/**
 * struct quwbs_fira_msg_rsp_session_deinit - Delete a session response message.
 */
struct quwbs_fira_msg_rsp_session_deinit {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_deinit *params;
};

/**
 * struct quwbs_fira_params_session_start - Start a session.
 */
struct quwbs_fira_params_session_start {
	/**
	 * @session_id: Session identifier.
	 */
	uint32_t session_id;
};

/**
 * struct quwbs_fira_msg_cmd_session_start - Start a session command message.
 */
struct quwbs_fira_msg_cmd_session_start {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_start *params;
};

/**
 * struct quwbs_fira_returns_session_start - Start a session.
 */
struct quwbs_fira_returns_session_start {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
};

/**
 * struct quwbs_fira_msg_rsp_session_start - Start a session response message.
 */
struct quwbs_fira_msg_rsp_session_start {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_start *params;
};

/**
 * struct quwbs_fira_params_session_stop - Stop a session.
 */
struct quwbs_fira_params_session_stop {
	/**
	 * @session_id: Session identifier.
	 */
	uint32_t session_id;
};

/**
 * struct quwbs_fira_msg_cmd_session_stop - Stop a session command message.
 */
struct quwbs_fira_msg_cmd_session_stop {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_stop *params;
};

/**
 * struct quwbs_fira_returns_session_stop - Stop a session.
 */
struct quwbs_fira_returns_session_stop {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
};

/**
 * struct quwbs_fira_msg_rsp_session_stop - Stop a session response message.
 */
struct quwbs_fira_msg_rsp_session_stop {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_stop *params;
};

/**
 * struct quwbs_fira_params_session_get_state - Get session current state.
 */
struct quwbs_fira_params_session_get_state {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
};

/**
 * struct quwbs_fira_msg_cmd_session_get_state - Get session current state command message.
 */
struct quwbs_fira_msg_cmd_session_get_state {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_get_state *params;
};

/**
 * struct quwbs_fira_returns_session_get_state - Get session current state.
 */
struct quwbs_fira_returns_session_get_state {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @session_state: Current session state.
	 */
	enum quwbs_fbs_session_state session_state;
};

/**
 * struct quwbs_fira_msg_rsp_session_get_state - Get session current state response message.
 */
struct quwbs_fira_msg_rsp_session_get_state {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_get_state *params;
};

/**
 * struct quwbs_fira_params_session_data_send - Send data message.
 */
struct quwbs_fira_params_session_data_send {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @destination_addr: Address of the recipient.
	 */
	uint64_t destination_addr;
	/**
	 * @sequence_number: Sequence number of the message.
	 */
	uint16_t sequence_number;
	/**
	 * @data: Data payload.
	 */
	struct qnetbuf *data;
};

/**
 * struct quwbs_fira_msg_cmd_session_data_send - Send data message command message.
 */
struct quwbs_fira_msg_cmd_session_data_send {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_data_send *params;
};


/**
 * struct quwbs_fira_msg_rsp_session_data_send - Send data message response message.
 */
struct quwbs_fira_msg_rsp_session_data_send {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
};


/**
 * struct quwbs_fira_msg_cmd_session_get_count - Get number of sessions command message.
 */
struct quwbs_fira_msg_cmd_session_get_count {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
};

/**
 * struct quwbs_fira_returns_session_get_count - Get number of sessions.
 */
struct quwbs_fira_returns_session_get_count {
	/**
	 * @session_count: Number of sessions.
	 */
	uint32_t session_count;
};

/**
 * struct quwbs_fira_msg_rsp_session_get_count - Get number of sessions response message.
 */
struct quwbs_fira_msg_rsp_session_get_count {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_get_count *params;
};

/**
 * struct quwbs_fira_params_session_set_params - Set session parameters.
 *
 * Unless specified, parameters can only be changed when the session is not active.
 */
struct quwbs_fira_params_session_set_params {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @session_params: Session parameters.
	 */
	union quwbs_fira_session_param *session_params;
};

/**
 * struct quwbs_fira_msg_cmd_session_set_params - Set session parameters command message.
 */
struct quwbs_fira_msg_cmd_session_set_params {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_set_params *params;
};

/**
 * struct quwbs_fira_returns_session_set_params - Set session parameters.
 */
struct quwbs_fira_returns_session_set_params {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @failed_session_params: Session parameters which were not applied.
	 */
	struct quwbs_fira_session_param_status *failed_session_params;
};

/**
 * struct quwbs_fira_msg_rsp_session_set_params - Set session parameters response message.
 */
struct quwbs_fira_msg_rsp_session_set_params {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_set_params *params;
};

/**
 * struct quwbs_fira_params_session_get_params - Get session parameters.
 *
 * In case of success, the requested session parameters are returned.
 *
 * If one of the requested parameter is not available, no parameter is
 * returned. Instead the list of unavailable parameters is returned.
 * Parameters which are not listed are assumed to be available.
 */
struct quwbs_fira_params_session_get_params {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @session_params: Session parameters to get.
	 */
	struct quwbs_fira_session_param_tags *session_params;
};

/**
 * struct quwbs_fira_msg_cmd_session_get_params - Get session parameters command message.
 */
struct quwbs_fira_msg_cmd_session_get_params {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_cmd base;
	};
	/**
	 * @params: Call parameters.
	 */
	struct quwbs_fira_params_session_get_params *params;
};

/**
 * struct quwbs_fira_returns_session_get_params - Get session parameters.
 */
struct quwbs_fira_returns_session_get_params {
	/**
	 * @status: Operation status.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @session_params: Session parameters.
	 */
	union quwbs_fira_session_param *session_params;
	/**
	 * @failed_session_params: Unavailable session parameters.
	 */
	struct quwbs_fira_session_param_tags *failed_session_params;
};

/**
 * struct quwbs_fira_msg_rsp_session_get_params - Get session parameters response message.
 */
struct quwbs_fira_msg_rsp_session_get_params {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Call return parameters.
	 */
	struct quwbs_fira_returns_session_get_params *params;
};

/**
 * struct quwbs_fira_params_session_status - Session status notification.
 *
 * This is sent when the session state changed or when it was supposed to
 * change but was not able to do it.
 */
struct quwbs_fira_params_session_status {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @session_state: Current session state.
	 */
	enum quwbs_fbs_session_state session_state;
	/**
	 * @session_state_reason: Reason for current session state.
	 */
	enum quwbs_fira_session_state_reason session_state_reason;
};

/**
 * struct quwbs_fira_msg_ntf_session_status - Session status notification notification message.
 */
struct quwbs_fira_msg_ntf_session_status {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Notification parameters.
	 */
	struct quwbs_fira_params_session_status *params;
};

/**
 * struct quwbs_fira_params_session_info - Session information notification.
 */
struct quwbs_fira_params_session_info {
	/**
	 * @session_id: Session identifier.
	 */
	uint32_t session_id;
	/**
	 * @sequence_number: Notification sequence number.
	 */
	uint32_t sequence_number;
	/**
	 * @ranging_data: Ranging result information.
	 */
	struct quwbs_fira_ranging_data *ranging_data;
};

/**
 * struct quwbs_fira_msg_ntf_session_info - Session information notification notification message.
 */
struct quwbs_fira_msg_ntf_session_info {
	union {
		/**
		 * @msg: Base message structure.
		 */
		struct quwbs_msg msg;
		/**
		 * @base: Base specific message structure.
		 */
		struct quwbs_msg_mac_call_region_rsp_ntf base;
	};
	/**
	 * @params: Notification parameters.
	 */
	struct quwbs_fira_params_session_info *params;
};

#ifdef __cplusplus
}
#endif
