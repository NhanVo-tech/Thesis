/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "quwbs/fbs/defs.h"

#define FIRA_VENDOR_ID_SIZE 2
#define FIRA_STATIC_STS_IV_SIZE 6
#define FIRA_VUPPER64_SIZE (FIRA_VENDOR_ID_SIZE + FIRA_STATIC_STS_IV_SIZE)
#define FIRA_KEY_SIZE_MAX 32
#define FIRA_KEY_SIZE_MIN 16
#define FIRA_VERSION_RANGE_SIZE 4
#define FIRA_TIME_BASE_SIZE 9
/*
 * Maximum number of responders: applicable for TWR (Controlees)
 * and for DL-TDoA (DT-Anchor Responders).
 *
 * Note: this number is coded in 8-bit for TWR and in 4-bit
 * for DL-TDoA - keep in mind these technical boundaries.
 */
#define FIRA_RESPONDERS_MAX 8

/*
 * Maximum number of measurements in one ranging block.
 * This value is determined by MIN_FRAMES_PER_RR max value.
 * From UCI spec v2.0.0_0.9r11::Table 46.
 */
#define FIRA_OWR_AOA_MEASUREMENTS_MAX 15

/*
 * This is a technical limitation caused by the maximum size of PSDUs
 * in BPRF (calculated for Poll DTM). More details in UWB-10852.
 */
#define FIRA_DT_TAG_MAX_ACTIVE_RR 33
#define FIRA_DT_ANCHOR_MAX_ACTIVE_RR 33

#define FIRA_RX_ANTENNA_PAIR_INVALID 0xff

/* From UCI spec v1.1.0. */
#define FIRA_FAR_PROXIMITY_CONFIG_CM_DEFAULT 20000

/*
 * FIRA_SESSION_DATA_NTF_LOWER_/UPPER_BOUND_AOA default values :
 * Azimuth in rad_2pi : -32768 / 32768 (equal to -180 / +180 degrees)
 * Elevation in rad_2pi : -16384 / 16384 (equal to -90 / +90 degrees)
 */
#define FIRA_SESSION_LOWER_AOA_BOUND_CONFIG_AZIMUTH_2PI_DEFAULT -32768
#define FIRA_SESSION_UPPER_AOA_BOUND_CONFIG_AZIMUTH_2PI_DEFAULT 32768
#define FIRA_SESSION_LOWER_AOA_BOUND_CONFIG_ELEVATION_2PI_DEFAULT -16384
#define FIRA_SESSION_UPPER_AOA_BOUND_CONFIG_ELEVATION_2PI_DEFAULT 16384

/* Default channel and preamble code in UCI spec v2.0.0. */
#define FIRA_DEFAULT_CHANNEL_NUMBER 9
#define FIRA_DEFAULT_PREAMBLE_CODE_INDEX 10

/*
 * Default value for maximum number of ranging sessions.
 * This value is used to set the corresponding capability message.
 */
#define FIRA_MAX_RANGING_SESSION_NUMBER 8

/*
 * Default value for minimum ranging interval in ms.
 * We consider the Non-deferred SS-TWR Ranging Round without RCP case, with a
 * slot duration of 1 ms.
 * This value is used to set the corresponding capability message.
 */
#define FIRA_MIN_RANGING_INTERVAL_MS 3

/*
 * Default value for minimum slot duration RSTU.
 * This value is used to set the corresponding capability message.
 */
#define FIRA_MIN_SLOT_DURATION_RSTU 1200

#define FIRA_NLOS_NOT_SUPPORTED 0xff

/* Default value UL-TDoA TX interval between blinks (in milliseconds). */
#define FIRA_UT_DEFAULT_TX_INTERVAL_MS 2000

/* Invalid value for ToF in DL-TDoA frames. */
#define FIRA_OWR_DT_TOF_INVALID 0

/* Invalid value for distance_cm in TWR notification. */
#define FIRA_TWR_NTF_DISTANCE_INVALID -1

/*
 * FiRa Consortium UCI Generic Technical Specification v2.0.0_0.9r2
 * Section: 8.2 Device Configuration Parameters
 * See preamble code range of value accepted in description column.
 */
#define FIRA_PCODE_BPRF_MIN 9 /* Match with MCPS802154_LLHW_BPRF_MIN. */
#define FIRA_PCODE_BPRF_MAX 12 /* Doesn't match with MCPS802154_LLHW_BPRF_MAX. */
#define FIRA_PCODE_HPRF_MIN 25 /* Match with MCPS802154_LLHW_HPRF_MIN. */
#define FIRA_PCODE_HPRF_MAX 32 /* Match with MCPS802154_LLHW_HPRF_MAX. */

/**
 * enum fira_dt_anchor_acting_role - |NSQM33| Internal role played by a DT-Anchor
 * during the particular ranging round.
 * @FIRA_DT_ANCHOR_ACTING_RESPONDER: The DT-Anchor acts as a responder.
 * @FIRA_DT_ANCHOR_ACTING_INITIATOR: The DT-Anchor acts as an initiator.
 */
enum fira_dt_anchor_acting_role {
	FIRA_DT_ANCHOR_ACTING_RESPONDER,
	FIRA_DT_ANCHOR_ACTING_INITIATOR,
};

/**
 * enum fira_ranging_round_usage - Ranging mode.
 * @FIRA_RANGING_ROUND_USAGE_OWR_UL_TDOA: |NSQM33|
 *	One Way Ranging UL-TDoA.
 * @FIRA_RANGING_ROUND_USAGE_SSTWR_DEFERRED:
 *	Single-Sided Two Way Ranging with Deferred Mode.
 * @FIRA_RANGING_ROUND_USAGE_DSTWR_DEFERRED:
 *	Dual-Sided Two Way Ranging with Deferred Mode.
 * @FIRA_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED:
 *	Single-Sided Two Way Ranging with Non-Deferred Mode.
 * @FIRA_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED:
 *	Dual-Sided Two Way Ranging with Non-Deferred Mode.
 * @FIRA_RANGING_ROUND_USAGE_OWR_DL_TDOA: |NSQM33|
 *	One Way Ranging for DownLink Time Difference of Arrival measurement.
 * @FIRA_RANGING_ROUND_USAGE_OWR_AOA: |NSQM33|
 *	One Way Ranging for Angle of Arrival measurement
 * @FIRA_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED: |NSQM33|
 *	Enhanced SingleSided Two Way Ranging with Non-deferred Mode for
 *	Contention-based ranging.
 * @FIRA_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED: |NSQM33|
 *	Alternative Dual-Sided Two Way Ranging for Contention-based ranging.
 * @FIRA_RANGING_ROUND_USAGE_ASSIGNED:
 *	RFU.
 * @FIRA_RANGING_ROUND_USAGE_HYBRID_RANGING: |NSQM33|
 *	Hybrid Ranging Mode.
 * @FIRA_RANGING_ROUND_USAGE_MAX: Max value for the range check.
 * @FIRA_RANGING_ROUND_USAGE_UNDEFINED: Ranging round usage is undefined.
 */
enum fira_ranging_round_usage {
	FIRA_RANGING_ROUND_USAGE_OWR_UL_TDOA,
	FIRA_RANGING_ROUND_USAGE_SSTWR_DEFERRED,
	FIRA_RANGING_ROUND_USAGE_DSTWR_DEFERRED,
	FIRA_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED,
	FIRA_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED,
	FIRA_RANGING_ROUND_USAGE_OWR_DL_TDOA,
	FIRA_RANGING_ROUND_USAGE_OWR_AOA,
	FIRA_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED,
	FIRA_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED,
	FIRA_RANGING_ROUND_USAGE_ASSIGNED,
	FIRA_RANGING_ROUND_USAGE_HYBRID_RANGING,
	FIRA_RANGING_ROUND_USAGE_MAX = FIRA_RANGING_ROUND_USAGE_HYBRID_RANGING,
	FIRA_RANGING_ROUND_USAGE_UNDEFINED,
};

/**
 * enum fira_dl_tdoa_ranging_method - |NSQM33| Ranging method used by DT-Anchors in
 * DL-TDoA.
 * @FIRA_DL_TDOA_RANGING_METHOD_SSTWR: Single-Sided Two Way Ranging.
 * @FIRA_DL_TDOA_RANGING_METHOD_DSTWR: Dual-Sided Two Way Ranging.
 */
enum fira_dl_tdoa_ranging_method {
	FIRA_DL_TDOA_RANGING_METHOD_SSTWR,
	FIRA_DL_TDOA_RANGING_METHOD_DSTWR,
};

/**
 * enum fira_multi_node_mode - Multi-node mode.
 * @FIRA_MULTI_NODE_MODE_UNICAST: Ranging between one initiator and one
 * responder.
 * @FIRA_MULTI_NODE_MODE_ONE_TO_MANY: Ranging between one initiator and
 * multiple responders.
 * @FIRA_MULTI_NODE_MODE_MANY_TO_MANY: |NSQM33| Ranging between multiple initiators
 * and multiple responders.
 * @FIRA_MULTI_NODE_MODE_UNDEFINED: Ranging mode is undefined.
 */
enum fira_multi_node_mode {
	FIRA_MULTI_NODE_MODE_UNICAST,
	FIRA_MULTI_NODE_MODE_ONE_TO_MANY,
	FIRA_MULTI_NODE_MODE_MANY_TO_MANY,
	FIRA_MULTI_NODE_MODE_UNDEFINED,
};

/**
 * enum fira_measurement_report_originator - Originator (author) of the
 * Ranging Measurement Report Message (MRM).
 * @FIRA_MEASUREMENT_REPORT_FROM_INITIATOR:
 *    The initiator sends a measurement report message.
 * @FIRA_MEASUREMENT_REPORT_FROM_RESPONDER:
 *    The responder sends a measurement report message.
 */
enum fira_measurement_report_originator {
	FIRA_MEASUREMENT_REPORT_FROM_INITIATOR,
	FIRA_MEASUREMENT_REPORT_FROM_RESPONDER,
};

/**
 * enum fira_measurement_report_type - Type of the current
 * Ranging Measurement Report Message (MRM).
 * @FIRA_MEASUREMENT_REPORT_TYPE_1: Measurement report message type 1.
 * @FIRA_MEASUREMENT_REPORT_TYPE_2: Measurement report message type 2.
 * @FIRA_MEASUREMENT_REPORT_TYPE_3: |NSQM33| Measurement report message type 3.
 */
enum fira_measurement_report_type {
	FIRA_MEASUREMENT_REPORT_TYPE_1,
	FIRA_MEASUREMENT_REPORT_TYPE_2,
	FIRA_MEASUREMENT_REPORT_TYPE_3,
};

/**
 * enum fira_owr_message_type - |NSQM33| Type of the current
 * One Way Ranging (OWR) Message (FiRa v2.0 5.9.13 OWR Message).
 * @FIRA_OWR_MESSAGE_TYPE_UT_BLINK: UL-TDOA Blink Message.
 * @FIRA_OWR_MESSAGE_TYPE_UT_SYNC: UL-TDOA Synchronization Message.
 * @FIRA_OWR_MESSAGE_TYPE_DT_POLL: DL-TDOA Poll Message.
 * @FIRA_OWR_MESSAGE_TYPE_DT_RESPONSE: DL-TDOA Response Message.
 * @FIRA_OWR_MESSAGE_TYPE_DT_FINAL: DL-TDOA Final Message.
 * @FIRA_OWR_MESSAGE_TYPE_AOA_MEASUREMENT: AoA Measurement Message.
 */
enum fira_owr_message_type {
	FIRA_OWR_MESSAGE_TYPE_UT_BLINK,
	FIRA_OWR_MESSAGE_TYPE_UT_SYNC,
	FIRA_OWR_MESSAGE_TYPE_DT_POLL,
	FIRA_OWR_MESSAGE_TYPE_DT_RESPONSE,
	FIRA_OWR_MESSAGE_TYPE_DT_FINAL,
	FIRA_OWR_MESSAGE_TYPE_AOA_MEASUREMENT,
};

/**
 * enum fira_schedule_mode - Slot scheduling mode used
 * during the ranging session.
 * @FIRA_SCHEDULE_MODE_CONTENTION_BASED: |NSQM33| Contention-based ranging.
 * @FIRA_SCHEDULE_MODE_TIME_SCHEDULED: Time-scheduled ranging.
 * @FIRA_SCHEDULE_MODE_HYBRID_BASED: |NSQM33| Hybrid-based ranging.
 * @FIRA_SCHEDULE_MODE_MAX: Max value defined
 * @FIRA_SCHEDULE_MODE_UNDEFINED: Scheduled mode is undefined.
 */
enum fira_schedule_mode {
	FIRA_SCHEDULE_MODE_CONTENTION_BASED,
	FIRA_SCHEDULE_MODE_TIME_SCHEDULED,
	FIRA_SCHEDULE_MODE_HYBRID_BASED,
	FIRA_SCHEDULE_MODE_MAX = FIRA_SCHEDULE_MODE_HYBRID_BASED,
	FIRA_SCHEDULE_MODE_UNDEFINED,
};

/**
 * enum fira_rframe_config - Rframe configuration used to transmit/receive
 * ranging messages.
 * @FIRA_RFRAME_CONFIG_SP0: Use SP0 mode. (Applicable only for PCTT)
 * @FIRA_RFRAME_CONFIG_SP1: Use SP1 mode.
 * @FIRA_RFRAME_CONFIG_SP2: RFU
 * @FIRA_RFRAME_CONFIG_SP3: Use SP3 mode.
 */
enum fira_rframe_config {
	FIRA_RFRAME_CONFIG_SP0,
	FIRA_RFRAME_CONFIG_SP1,
	FIRA_RFRAME_CONFIG_SP2,
	FIRA_RFRAME_CONFIG_SP3,
};

/**
 * enum fira_prf_mode - Pulse Repetition Frequency mode
 * @FIRA_PRF_MODE_BPRF: Base Pulse Repetition Frequency.
 * @FIRA_PRF_MODE_HPRF: |NSQM33| Higher Pulse Repetition Frequency.
 * @FIRA_PRF_MODE_HPRF_HIGH_RATE: |NSQM33| Higher Pulse Repetition Frequency allows
 * high data rate (27.2 Mbps and 31.2 Mbps).
 *
 * This enum is not used in the current implementation.
 */
enum fira_prf_mode {
	FIRA_PRF_MODE_BPRF,
	FIRA_PRF_MODE_HPRF,
	FIRA_PRF_MODE_HPRF_HIGH_RATE,
};

/**
 * enum fira_preamble_duration - Duration of preamble in symbols.
 * @FIRA_PREAMBLE_DURATION_32: |NSQM33| 32 symbols duration.
 * @FIRA_PREAMBLE_DURATION_64: 64 symbols duration.
 */
enum fira_preamble_duration {
	FIRA_PREAMBLE_DURATION_32,
	FIRA_PREAMBLE_DURATION_64,
};

/**
 * enum fira_sfd_id - Start-of-frame delimiter.
 * @FIRA_SFD_ID_0: Delimiter is [0 +1 0 –1 +1 0 0 –1]
 * @FIRA_SFD_ID_1: |NSQM33| Delimiter is [ –1 –1 +1 –1 ]
 * @FIRA_SFD_ID_2: Delimiter is [ –1 –1 –1 +1 –1 –1 +1 –1 ]
 * @FIRA_SFD_ID_3: |NSQM33| Delimiter is
 * [ –1 –1 –1 –1 –1 +1 +1 –1 –1 +1 –1 +1 –1 –1 +1 –1 ]
 * @FIRA_SFD_ID_4: |NSQM33| Delimiter is
 * [ –1 –1 –1 –1 –1 –1 –1 +1 –1 –1 +1 –1 –1 +1 –1 +1 –1 +1
 * –1 –1 –1 +1 +1 –1 –1 –1 +1 –1 +1 +1 –1 –1 ]
 */
enum fira_sfd_id {
	FIRA_SFD_ID_0,
	FIRA_SFD_ID_1,
	FIRA_SFD_ID_2,
	FIRA_SFD_ID_3,
	FIRA_SFD_ID_4,
};

/**
 * enum fira_sts_segments - Number of STS segments.
 * @FIRA_STS_SEGMENTS_0: No STS Segment (Rframe config SP0).
 * @FIRA_STS_SEGMENTS_1: 1 STS Segment.
 * @FIRA_STS_SEGMENTS_2: |NSQM33| 2 STS Segments.
 * @FIRA_STS_SEGMENTS_3: |NSQM33| 3 STS Segments.
 * @FIRA_STS_SEGMENTS_4: |NSQM33| 4 STS Segments.
 */
enum fira_sts_segments {
	FIRA_STS_SEGMENTS_0,
	FIRA_STS_SEGMENTS_1,
	FIRA_STS_SEGMENTS_2,
	FIRA_STS_SEGMENTS_3,
	FIRA_STS_SEGMENTS_4,
};

/**
 * enum fira_psdu_data_rate - Data rate used to exchange PSDUs.
 * @FIRA_PSDU_DATA_RATE_6M81: 6.8Mb/s rate.
 * @FIRA_PSDU_DATA_RATE_7M80: |NSQM33| 7.8Mb/s rate.
 * @FIRA_PSDU_DATA_RATE_27M2: |NSQM33| 27.2Mb/s rate.
 * @FIRA_PSDU_DATA_RATE_31M2: |NSQM33| 31.2Mb/s rate.
 */
enum fira_psdu_data_rate {
	FIRA_PSDU_DATA_RATE_6M81,
	FIRA_PSDU_DATA_RATE_7M80,
	FIRA_PSDU_DATA_RATE_27M2,
	FIRA_PSDU_DATA_RATE_31M2,
};

/**
 * enum fira_phr_data_rate - Data rate used to exchange PHR.
 * @FIRA_PHR_DATA_RATE_850K: 850kb/s rate.
 * @FIRA_PHR_DATA_RATE_6M81: 6.8Mb/s rate.
 */
enum fira_phr_data_rate {
	FIRA_PHR_DATA_RATE_850K,
	FIRA_PHR_DATA_RATE_6M81,
};

/**
 * enum fira_mac_fcs_type - Length of Frame Check Sequence.
 * @FIRA_MAC_FCS_TYPE_CRC_16: 2 bytes sequence.
 * @FIRA_MAC_FCS_TYPE_CRC_32: 4 bytes sequence.
 */
enum fira_mac_fcs_type {
	FIRA_MAC_FCS_TYPE_CRC_16,
	FIRA_MAC_FCS_TYPE_CRC_32,
};

/**
 * enum fira_data_transfer_status - |NSQM33| Data transfer status.
 * @FIRA_DATA_TRANSFER_STATUS_REPETITION_OK: If DATA_REPETITION_COUNT>0 and if
 * SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG = Enable; it indicates that one
 * Data transmission is completed in a RR.
 * @FIRA_DATA_TRANSFER_STATUS_OK: For TWR - it indicates that the Application
 * Data transmission is completed. For OWR - it indicates that the Application
 * Data transmission and its repetitions of DATA_REPETITION_COUNT is completed.
 * @FIRA_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER: Application Data couldn’t be
 * sent due to an unrecoverable error.
 * @FIRA_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE: DATA_MESSAGE_SND is
 * not accepted as no credit is available.
 * @FIRA_DATA_TRANSFER_STATUS_ERROR_REJECTED: DATA_MESSAGE_SND packet sent in
 * wrong state or Application Data Size exceeds the maximum size that can be
 * sent in one Ranging Round.
 * @FIRA_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED: Data transfer is
 * not supported for given session type.
 * @FIRA_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING: Application Data
 * is being transmitted and the number of configured DATA_REPETITION_COUNT
 * transmissions is not yet completed.
 * @FIRA_DATA_TRANSFER_STATUS_INVALID_FORMAT: The format of the command
 * DATA_MESSAGE_SND associated with this notification is incorrect (e.g,
 * a parameter is missing, a parameter value is invalid).
 */
enum fira_data_transfer_status {
	FIRA_DATA_TRANSFER_STATUS_REPETITION_OK = 0x00,
	FIRA_DATA_TRANSFER_STATUS_OK = 0x01,
	FIRA_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER = 0x02,
	FIRA_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE = 0x03,
	FIRA_DATA_TRANSFER_STATUS_ERROR_REJECTED = 0x04,
	FIRA_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED = 0x05,
	FIRA_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING = 0x06,
	FIRA_DATA_TRANSFER_STATUS_INVALID_FORMAT = 0x07,
};

/*
 * The maximum number of steps a measurement sequence can contain.
 */
#define FIRA_MEASUREMENT_SEQUENCE_STEP_MAX 10

/**
 * enum fira_measurement_type - The different type of available measurements.
 * @FIRA_MEASUREMENT_TYPE_RANGE: Measure only range.
 * @FIRA_MEASUREMENT_TYPE_AOA: Measure range + unspecified AoA.
 * @FIRA_MEASUREMENT_TYPE_AOA_AZIMUTH: Measure range + azimuth.
 * @FIRA_MEASUREMENT_TYPE_AOA_ELEVATION: Measure range + elevation.
 * @FIRA_MEASUREMENT_TYPE_AOA_AZIMUTH_ELEVATION: Measure
 * range+azimuth+elevation.
 * @__FIRA_MEASUREMENT_TYPE_AFTER_LAST: Internal use.
 */
enum fira_measurement_type {
	FIRA_MEASUREMENT_TYPE_RANGE = 0,
	FIRA_MEASUREMENT_TYPE_AOA,
	FIRA_MEASUREMENT_TYPE_AOA_AZIMUTH,
	FIRA_MEASUREMENT_TYPE_AOA_ELEVATION,
	FIRA_MEASUREMENT_TYPE_AOA_AZIMUTH_ELEVATION,
	__FIRA_MEASUREMENT_TYPE_AFTER_LAST,
};

/**
 * struct fira_session_time_base - |NSQM33| Session time base information.
 * @config: Time base configuration bitfield.
 * 	b0: 1:enable - 0:disable
 * 	b1: 1:continue - 0:stop
 * 	b2: 1:resync - 0:no resync
 * @session_handle: Session handle of the reference session.
 * @time_offset_us: Time offset in microseconds.
 */
struct fira_session_time_base {
	uint8_t config;
	uint32_t session_handle;
	uint32_t time_offset_us;
};

/**
 * struct fira_measurement_sequence_step - One measurement step configuration.
 * @type: The type of measurement.
 * @n_measurements: The number of measurement to do.
 * @rx_ant_set_nonranging: RX antenna set for non-ranging frames.
 * @rx_ant_sets_ranging: RX antenna sets for ranging frames.
 * @tx_ant_set_nonranging: TX antenna set for non-ranging frames.
 * @tx_ant_set_ranging: TX antenna set for ranging frames.
 */
struct fira_measurement_sequence_step {
	enum fira_measurement_type type;
	uint8_t n_measurements;
	int8_t rx_ant_set_nonranging;
	int8_t rx_ant_sets_ranging[2];
	int8_t tx_ant_set_nonranging;
	int8_t tx_ant_set_ranging;
};

/**
 * struct fira_measurement_sequence - Measurement sequence configuration.
 * @n_steps: Number of steps in the sequence.
 * @steps: Array of step configuration.
 */
struct fira_measurement_sequence {
	uint8_t n_steps;
	struct fira_measurement_sequence_step steps[];
};

/**
 * enum fira_ranging_diagnostics_frame_report_flags - Activation flags for
 * different frame diagnostics information.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_NONE: No specific frame diagnostic
 *	report requested.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_AOAS: Report AOA in frame diagnostics.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_CFO: Report Clock Frequency Offset in
 *	report, as measured on the first Rx RFRAME in the round.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_EMITTER_SHORT_ADDR: Report the MAC
 *	emitter short address in frame diagnostics.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_SEGMENT_METRICS: Report Segment
 *	Metrics in frame diagnostics.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_CIRS: Report CIR in frame diagnostics.
 * @__FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_AFTER_LAST: Internal use.
 */
enum fira_ranging_diagnostics_frame_report_flags {
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_NONE = 0,
	/* Bit 1 is deprecated., please do not use. */
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_AOAS = 1 << 1,
	/* Bit 3 is deprecated., please do not use. */
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_CFO = 1 << 3,
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_EMITTER_SHORT_ADDR = 1 << 4,
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_SEGMENT_METRICS = 1 << 5,
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_CIRS = 1 << 6,
	/* Last enum value to keep 8-bit enum. */
	__FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_AFTER_LAST = 1 << 7,
};

/**
 * enum fira_ranging_diagnostics_frame_reports_status_flags - Flags for the
 * invidual frame report's status bitfield.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_SUCCESS:
 *	False when then frame Rx has failed for some reason. Always true for Tx.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_WIFI_COEX:
 *	True if the Wifi Coex GPIO was on while transmitting the frame.
 * @FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_GRANT_DURATION_EXCEEDED:
 *	True if the MAX_GRANT_DURATION has been exceeded.
 */
enum fira_ranging_diagnostics_frame_reports_status_flags {
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_SUCCESS = 1 << 0,
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_WIFI_COEX = 1 << 1,
	FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_GRANT_DURATION_EXCEEDED = 1 << 2,
};

/**
 * enum fira_sts_length - Number of symbols in a STS segment.
 * @FIRA_STS_LENGTH_32: The STS length is 32 symbols.
 * @FIRA_STS_LENGTH_64: The STS length is 64 symbols.
 * @FIRA_STS_LENGTH_128: The STS length is 128 symbols.
 */
enum fira_sts_length {
	FIRA_STS_LENGTH_32 = 0,
	FIRA_STS_LENGTH_64 = 1,
	FIRA_STS_LENGTH_128 = 2,
};

/**
 * enum fira_session_info_ntf_config - |NSQM33| Configure session info notification.
 * @FIRA_SESSION_INFO_NTF_CONFIG_DISABLED: Do not report range data.
 * @FIRA_SESSION_INFO_NTF_CONFIG_ALWAYS: Report range data.
 * @FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY: Report range data if it is within
 * proximity range defined by proximity parameters
 * (NEAR_PROXIMITY_CONFIG/FAR).
 * @FIRA_SESSION_INFO_NTF_CONFIG_AOA: Report range data in AoA upper and lower
 * bound. defined by AOA parameters
 * (FIRA_SESSION_PARAM_ATTR_SESSION_INFO_NTF_UPPER/
 * LOWER_BOUND_AOA_AZIMUTH/ELEVATION)
 * @FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA: Report range data in AoA
 * upper and lower bound as well as in proximity range.
 * @FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_CROSSING: Same as
 * FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY, but issues notification on crossing
 * of boundaries. As for now, same notif is sent for "enter" and "exit" events.
 * @FIRA_SESSION_INFO_NTF_CONFIG_AOA_CROSSING:  Same as
 * FIRA_SESSION_INFO_NTF_CONFIG_AOA, but issues notification on crossing of
 * boundaries. As for now, same notif is sent for "enter" and "exit" events.
 * @FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA_CROSSING:  Same as
 * FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA, but issues notification on
 * crossing of boundaries. As for now, same notif is sent for "enter" and "exit"
 * events.
 */
enum fira_session_info_ntf_config {
	FIRA_SESSION_INFO_NTF_CONFIG_DISABLED,
	FIRA_SESSION_INFO_NTF_CONFIG_ALWAYS,
	FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY,
	FIRA_SESSION_INFO_NTF_CONFIG_AOA,
	FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA,
	FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_CROSSING,
	FIRA_SESSION_INFO_NTF_CONFIG_AOA_CROSSING,
	FIRA_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA_CROSSING,
};

/**
 * enum fira_link_layer_mode - Link layer behavior.
 * @FIRA_LINK_LAYER_MODE_BYPASS: No link layer overhead is added to the MDSDU.
 * @FIRA_LINK_LAYER_MODE_CONNECTION_LESS: |NSQM33| Link layer header is needed to provide
 * addressing capabilities for data transmission.
 */
enum fira_link_layer_mode {
	FIRA_LINK_LAYER_MODE_BYPASS = 0,
	FIRA_LINK_LAYER_MODE_CONNECTION_LESS = 1,
};

/**
 * enum fira_message_id - Message identifiers, used in report and in messages.
 * @FIRA_MESSAGE_ID_RANGING_INITIATION: Ranging Initiation Message.
 * @FIRA_MESSAGE_ID_RANGING_RESPONSE: Ranging Response Message.
 * @FIRA_MESSAGE_ID_RANGING_FINAL: Ranging Final Message, only for DS-TWR.
 * @FIRA_MESSAGE_ID_CONTROL: Control Message, sent by the controller.
 * @FIRA_MESSAGE_ID_MEASUREMENT_REPORT: Measurement Report Message.
 * @FIRA_MESSAGE_ID_RESULT_REPORT: Ranging Result Report Message.
 * @FIRA_MESSAGE_ID_CONTROL_UPDATE: Control Update Message.
 * @FIRA_MESSAGE_ID_ONE_WAY_RANGING: |NSQM33| One Way Ranging Message (see internal
 * types).
 * @FIRA_MESSAGE_ID_DATA: |NSQM33| Data Message.
 * @FIRA_MESSAGE_ID_RFRAME_MAX: Maximum identifier of messages transmitted as
 * an RFRAME (without a payload).
 * @FIRA_MESSAGE_ID_MAX: Maximum identifier of all messages.
 */
enum fira_message_id {
	FIRA_MESSAGE_ID_RANGING_INITIATION,
	FIRA_MESSAGE_ID_RANGING_RESPONSE,
	FIRA_MESSAGE_ID_RANGING_FINAL,
	FIRA_MESSAGE_ID_CONTROL,
	FIRA_MESSAGE_ID_MEASUREMENT_REPORT,
	FIRA_MESSAGE_ID_RESULT_REPORT,
	FIRA_MESSAGE_ID_CONTROL_UPDATE,
	FIRA_MESSAGE_ID_ONE_WAY_RANGING,
	FIRA_MESSAGE_ID_DATA,
	FIRA_MESSAGE_ID_RFRAME_MAX = FIRA_MESSAGE_ID_RANGING_FINAL,
	FIRA_MESSAGE_ID_MAX = FIRA_MESSAGE_ID_CONTROL_UPDATE,
};

/**
 * enum fira_result_report_config_flags - result report config flags.
 * @FIRA_RESULT_REPORT_CONFIG_REPORT_TOF: Report Time of flight.
 * @FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION: Report elevation angle.
 * @FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH: Report azimuth angle.
 * @FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_FOM: report AoA figure of merit.
 * @FIRA_RESULT_REPORT_CONFIG_REPORT_ALL: Maximum value of the parameter.
 */
enum fira_result_report_config_flags {
	FIRA_RESULT_REPORT_CONFIG_REPORT_TOF = 1u << 0,
	FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH = 1u << 1,
	FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION = 1u << 2,
	FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_FOM = 1u << 3,
	FIRA_RESULT_REPORT_CONFIG_REPORT_ALL = FIRA_RESULT_REPORT_CONFIG_REPORT_TOF +
					       FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH +
					       FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION +
					       FIRA_RESULT_REPORT_CONFIG_REPORT_AOA_FOM,
};

/**
 * enum fira_data_message_status - |NSQM33| status of data message receive notification.
 * @FIRA_DATA_MESSAGE_STATUS_SUCCESS: All data segments in the round were
 * successfully received.
 * @FIRA_DATA_MESSAGE_STATUS_FAILED: Intended operation failed to complete,
 * e.g. incomplete ranging round with piggyback data.
 * @FIRA_DATA_MESSAGE_STATUS_UNKNOWN: Failure due to unknown reason.
 */
enum fira_data_message_status {
	FIRA_DATA_MESSAGE_STATUS_SUCCESS = 0x00,
	FIRA_DATA_MESSAGE_STATUS_FAILED = 0x02,
	FIRA_DATA_MESSAGE_STATUS_UNKNOWN = 0x0b,
};

/**
 * enum fira_dt_location_coord_system_type - |NSQM33| Coordinate System Type
 * of a DT-Anchor.
 * @FIRA_DT_LOCATION_COORD_WGS84: The location is given in WGS84 coordinate
 * system (longitude, latitude, altitude,(see &struct fira_wgs84_location).
 * @FIRA_DT_LOCATION_COORD_RELATIVE: The location is given in relative
 * coordinates system (see &struct fira_relative_location).
 * @FIRA_DT_LOCATION_COORD_INVALID: is a value in RSU range for test.
 */
enum fira_dt_location_coord_system_type {
	FIRA_DT_LOCATION_COORD_WGS84 = 0,
	FIRA_DT_LOCATION_COORD_RELATIVE = 1,
	FIRA_DT_LOCATION_COORD_INVALID = 0xff,
};

/**
 * enum fira_owr_dtm_timestamp_type - |NSQM33| DTM TX Timestamp type.
 * @FIRA_OWR_DTM_TIMESTAMP_LOCAL_TIME_BASE: timestamp in local time base.
 * @FIRA_OWR_DTM_TIMESTAMP_COMMON_TIME_BASE: timestamp in common time base
 * of the Initiator DT-Anchor.
 */
enum fira_owr_dtm_timestamp_type {
	FIRA_OWR_DTM_TIMESTAMP_LOCAL_TIME_BASE = 0,
	FIRA_OWR_DTM_TIMESTAMP_COMMON_TIME_BASE = 1,
};

/**
 * enum fira_owr_dtm_timestamp_len - |NSQM33| DTM TX Timestamp length.
 * @FIRA_OWR_DTM_TIMESTAMP_40BITS: 40 bits timestamp.
 * @FIRA_OWR_DTM_TIMESTAMP_64BITS: 64 bits timestamp.
 */
enum fira_owr_dtm_timestamp_len {
	FIRA_OWR_DTM_TIMESTAMP_40BITS = 0,
	FIRA_OWR_DTM_TIMESTAMP_64BITS = 1,
};

/**
 * enum fira_owr_utm_timestamp_len - |NSQM33| UTM TX Timestamp length.
 * @FIRA_OWR_UTM_TIMESTAMP_NOT_PRESENT:   0 bits timestamp.
 * @FIRA_OWR_UTM_TIMESTAMP_40BITS: 40 bits timestamp.
 * @FIRA_OWR_UTM_TIMESTAMP_64BITS: 64 bits timestamp.
 */
enum fira_owr_utm_timestamp_len {
	FIRA_OWR_UTM_TIMESTAMP_NOT_PRESENT,
	FIRA_OWR_UTM_TIMESTAMP_40BITS,
	FIRA_OWR_UTM_TIMESTAMP_64BITS,
};

/**
 * enum fira_ranging_round_control_flags - Ranging round control flags.
 * Below bits make sense when SCHEDULE_MODE is set to Time scheduled.
 *
 * @FIRA_RANGING_ROUND_CONTROL_RRRM_EXPECTED:
 *   If set to 1, a Controller shall schedule an RRRM in the Ranging
 *   Device Management List (RDML).
 *   If set to 0, a Controller shall not schedule an RRRM in the RDML.
 * @FIRA_RANGING_ROUND_CONTROL_CM_EXPECTED:
 *   If set to 1, a Controller shall send a CM in-band and a Controlee
 *   shall expect a CM in-band.
 *   If set to 0, a Controller shall not send a CM in-band and a Controlee
 *   shall not expect a CM in-band.
 * @FIRA_RANGING_ROUND_CONTROL_RCP_EXCLUDED:
 *   If set to 1, RCP is excluded in Ranging Round, means CM is piggybacked
 *   with the RIM.
 *   If set to 0, RCP is included in Ranging Round.
 * @FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE: UWBS shall ignore this
 *   bit.
 * @FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE:
 *   If set to 1, the controller shall schedule the MRM to be sent from the
 *   responder(s) to the initiator in the RDML.
 *   If set to 0, the controller shall schedule the MRM to be sent from
 *   the initiator to the Responder(s) in the RDML.
 * @FIRA_RANGING_ROUND_CONTROL_ALL: Maximum value of the parameter.
 */
enum fira_ranging_round_control_flags {
	FIRA_RANGING_ROUND_CONTROL_RRRM_EXPECTED = 1u << 0,
	FIRA_RANGING_ROUND_CONTROL_CM_EXPECTED = 1u << 1,
	FIRA_RANGING_ROUND_CONTROL_RCP_EXCLUDED = 1u << 2,
	/* Bit 3 to 5 are reserved for a future usage (RFU). */
	FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE = 1u << 6,
	FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE = 1u << 7,
	FIRA_RANGING_ROUND_CONTROL_ALL = FIRA_RANGING_ROUND_CONTROL_RRRM_EXPECTED +
					 FIRA_RANGING_ROUND_CONTROL_CM_EXPECTED +
					 FIRA_RANGING_ROUND_CONTROL_RCP_EXCLUDED +
					 FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE +
					 FIRA_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE,
};

/**
 * enum fira_owr_utm_device_id_len - |NSQM33| UTM Device ID length.
 * @FIRA_OWR_UTM_DEVICE_ID_NOT_PRESENT: 0 bits device id.
 * @FIRA_OWR_UTM_DEVICE_ID_16BITS: 16 bits device id.
 * @FIRA_OWR_UTM_DEVICE_ID_32BITS: 32 bits device id.
 * @FIRA_OWR_UTM_DEVICE_ID_64BITS: 64 bits device id.
 */
enum fira_owr_utm_device_id_len {
	FIRA_OWR_UTM_DEVICE_ID_NOT_PRESENT,
	FIRA_OWR_UTM_DEVICE_ID_16BITS,
	FIRA_OWR_UTM_DEVICE_ID_32BITS,
	FIRA_OWR_UTM_DEVICE_ID_64BITS,
};

enum fira_owr_ut_report_config_event {
	FIRA_OWR_UT_REPORT_CONFIG_EVENT_ONLY_OK,
	FIRA_OWR_UT_REPORT_CONFIG_EVENT_ALL,
};

/**
 * enum fira_multicast_update_status - controlee change status after update
 * controller multicast list command.
 * @FIRA_MULTICAST_UPDATE_STATUS_OK_MULTICAST_LIST_UPDATE: it shall be reported
 * if the multicast list is updated (Add/Delete) successfully for the given
 * Controlee.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_MULTICAST_LIST_FULL: it shall be reported
 * for a Controlee if the multicast is full.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_KEY_FETCH_FAIL: it shall be reported for
 * a Controlee if Session Key fetch from Secure Component is failed.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_ID_NOT_FOUND: it shall be
 * reported for a Controlee if Sub-Session ID is not found in Secure Component.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_FOUND: it shall be
 * reported for a Controlee if Sub-Session Key is not found in Secure Component.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_APPLICABLE: it shall
 * be reported for a Controlee if Sub-Session Key is configured with STS config
 * is other than 0x04 (Provisioned STS for Responder specific Sub-session Key).
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_SESSION_KEY_NOT_FOUND: it shall be
 * reported for a Controlee if Sub-Session Key is configured but SESSION_KEY App
 * configuration parameter is not programmed.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_NOT_FOUND: it shall be reported
 * for a Controlee if its short address is not found while deleting its entry
 * from multicast list.
 * @FIRA_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_ALREADY_PRESENT: it shall be
 * reported for a Controlee if its short address is already present in the
 * multicast list.
 *
 */
enum fira_multicast_update_status {
	FIRA_MULTICAST_UPDATE_STATUS_OK_MULTICAST_LIST_UPDATE = 0x00,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_MULTICAST_LIST_FULL,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_KEY_FETCH_FAIL,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_ID_NOT_FOUND,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_FOUND,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_APPLICABLE,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_SESSION_KEY_NOT_FOUND,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_NOT_FOUND,
	FIRA_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_ALREADY_PRESENT = 0x08,
	/* From 0x09 to 0xFF, Reserved for Future Use. */
};

/**
 * enum fira_data_segment_info - |NSQM33| information about data packet received.
 * @FIRA_DATA_SEGMENT_FIRST: packet is the first one in data message.
 * @FIRA_DATA_SEGMENT_LAST: packet is the last one in data message.
 *
 * @__FIRA_DATA_SEGMENT_AFTER_LAST: Internal use.
 */
enum fira_data_segment_info {
	FIRA_DATA_SEGMENT_FIRST = 1u << 0,
	FIRA_DATA_SEGMENT_LAST = 1u << 1,
	__FIRA_DATA_SEGMENT_AFTER_LAST = 1u << 7, /* to keep 8-bit enum */
};

/**
 * enum fira_owr_aoa_measurement_ntf_period - |NSQM33| period of sending
 * SESSION_INFO_NTF.
 * @FIRA_OWR_AOA_MEASUREMENT_NTF_PERIOD_SINGLE: notification sent after every
 * received frames.
 * @FIRA_OWR_AOA_MEASUREMENT_NTF_PERIOD_AGGREGATED: notification sent after
 * MIN_FRAMES_PER_RR aggregated frames.
 */
enum fira_owr_aoa_measurement_ntf_period {
	FIRA_OWR_AOA_MEASUREMENT_NTF_PERIOD_SINGLE = 0,
	FIRA_OWR_AOA_MEASUREMENT_NTF_PERIOD_AGGREGATED = 1
};

struct cir {
	struct cir *next;
	uint8_t receiver_segment;
	int16_t fpath_tap_offset;
	uint16_t n_taps;
	uint8_t tap_size;
	const uint8_t *taps;
};

struct aoa_measurement {
	struct aoa_measurement *next;
	int16_t tdoa;
	int16_t pdoa;
	int16_t aoa;
	uint8_t fom;
	uint8_t type;
};

struct segment_metrics {
	struct segment_metrics *next;
	int16_t noise_value;
	uint16_t rsl_q8;
	uint16_t fp_index;
	uint16_t fp_rsl_q8;
	uint16_t fp_ns_q6;
	uint16_t pp_index;
	uint16_t pp_rsl_q8;
	uint16_t pp_ns_q6;
	uint8_t receiver_segment;
};

struct frame_report {
	struct frame_report *next;
	struct segment_metrics *seg_metrics;
	struct aoa_measurement *aoas;
	struct cir *cirs;
	int32_t cfo_q26;
	uint16_t emitter_short_addr;
	uint16_t extra_status;
	uint8_t nb_seg_metrics;
	uint8_t nb_aoa;
	uint8_t nb_cir;
	bool cfo_present;
	bool emitter_short_addr_present;
	uint8_t msg_id;
	uint8_t action;
	uint8_t antenna_set;
};

struct diagnostic_info {
	uint32_t nb_reports;
	struct frame_report *reports;
};

/**
 * fira_free_frame_report() - Free the frame report created by diagnostic.
 * @head: Head of the frame report.
 */
void fira_free_frame_report(struct frame_report *head);

#ifdef __cplusplus
}
#endif
