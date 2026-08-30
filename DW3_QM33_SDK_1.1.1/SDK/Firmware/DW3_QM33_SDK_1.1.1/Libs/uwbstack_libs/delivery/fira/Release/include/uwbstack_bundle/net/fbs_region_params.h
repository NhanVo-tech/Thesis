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

#define FBS_VENDOR_ID_SIZE 2
#define FBS_STATIC_STS_IV_SIZE 6
#define FBS_VUPPER64_SIZE (FBS_VENDOR_ID_SIZE + FBS_STATIC_STS_IV_SIZE)
#define FBS_KEY_SIZE_MAX 32
#define FBS_KEY_SIZE_MIN 16

/*
 * Maximum number of responders: applicable for TWR (Controlees)
 * and for DL-TDoA (DT-Anchor Responders).
 *
 * Note: this number is coded in 8-bit for TWR and in 4-bit
 * for DL-TDoA - keep in mind these technical boundaries.
 */
#define FBS_RESPONDERS_MAX 8

/*
 * Maximum number of measurements in one ranging block.
 * This value is determined by MIN_FRAMES_PER_RR max value.
 * From FiRa UCI spec v2.0.0_0.9r11::Table 46.
 */
#define FBS_OWR_AOA_MEASUREMENTS_MAX 15

/* Greater value (FBS_RESPONDERS_MAX or FBS_OWR_AOA_MEASUREMENTS_MAX). */
#define FBS_MEASUREMENTS_MAX FBS_OWR_AOA_MEASUREMENTS_MAX

/*
 * This is a technical limitation caused by the maximum size of PSDUs
 * in BPRF (calculated for Poll DTM). More details in UWB-10852.
 */
#define FBS_DT_TAG_MAX_ACTIVE_RR 33
#define FBS_DT_ANCHOR_MAX_ACTIVE_RR 33

#define FBS_RX_ANTENNA_PAIR_INVALID 0xff

/* From FiRa UCI spec v1.1.0. */
#define FBS_FAR_PROXIMITY_CONFIG_CM_DEFAULT 20000

/*
 * FBS_SESSION_DATA_NTF_LOWER_/UPPER_BOUND_AOA default values :
 * Azimuth in rad_2pi : -32768 / 32768 (equal to -180 / +180 degrees)
 * Elevation in rad_2pi : -16384 / 16384 (equal to -90 / +90 degrees)
 */
#define FBS_SESSION_DATA_NTF_LOWER_BOUND_AOA_AZIMUTH_2PI_DEFAULT -32768
#define FBS_SESSION_DATA_NTF_UPPER_BOUND_AOA_AZIMUTH_2PI_DEFAULT 32768
#define FBS_SESSION_DATA_NTF_LOWER_BOUND_AOA_ELEVATION_2PI_DEFAULT -16384
#define FBS_SESSION_DATA_NTF_UPPER_BOUND_AOA_ELEVATION_2PI_DEFAULT 16384

/* Default channel and preamble code in FiRa UCI spec v2.0.0. */
#define FBS_DEFAULT_CHANNEL_NUMBER 9
#define FBS_DEFAULT_PREAMBLE_CODE_INDEX 10

/*
 * Default value for maximum number of ranging sessions.
 * This value is used to set the corresponding capability message.
 */
#define FBS_MAX_RANGING_SESSION_NUMBER 8

/*
 * Default value for minimum ranging interval in ms.
 * We consider the Non-deferred SS-TWR Ranging Round without RCP case, with a
 * slot duration of 1 ms.
 * This value is used to set the corresponding capability message.
 */
#define FBS_MIN_RANGING_INTERVAL_MS 3

/*
 * Default value for minimum slot duration RSTU.
 * This value is used to set the corresponding capability message.
 */
#define FBS_MIN_SLOT_DURATION_RSTU 1200

#define FBS_NLOS_NOT_SUPPORTED 0xff

/* Default value UL-TDoA TX interval between blinks (in milliseconds). */
#define FBS_UT_DEFAULT_TX_INTERVAL_MS 2000

/* Invalid value for ToF in DL-TDoA frames. */
#define FBS_OWR_DT_TOF_INVALID 0

/* Invalid value for distance_cm in TWR notification. */
#define FBS_TWR_NTF_DISTANCE_INVALID -1

/*
 * FiRa Consortium UCI Generic Technical Specification v2.0.0_0.9r2
 * Section: 8.2 Device Configuration Parameters
 * See preamble code range of value accepted in description column.
 */
#define FBS_PCODE_BPRF_MIN 9 /* Match with MCPS802154_LLHW_BPRF_MIN. */
#define FBS_PCODE_BPRF_MAX                                 \
	12 /* Doesn't match with MCPS802154_LLHW_BPRF_MAX. \
	    */
#define FBS_PCODE_HPRF_MIN 25 /* Match with MCPS802154_LLHW_HPRF_MIN. */
#define FBS_PCODE_HPRF_MAX 32 /* Match with MCPS802154_LLHW_HPRF_MAX. */

/*
 * The maximum number of steps a measurement sequence can contain.
 */
#define FBS_MEASUREMENT_SEQUENCE_STEP_MAX 10

/*
 * Special value of the TIME0_NS to start session as it can.
 */
#define FBS_TIME0_NS_NOT_APPLICABLE 0

/**
 * enum fbs_device_state - Device state expose by FBS region.
 *
 * @FBS_DEVICE_STATE_READY: Device is ready, no sessions are active.
 * @FBS_DEVICE_STATE_ACTIVE: Device is active, at least one session is active.
 * @FBS_DEVICE_STATE_ERROR: Device is in error, reset is mandatory.
 */
enum fbs_device_state {
	FBS_DEVICE_STATE_READY = 0x01,
	FBS_DEVICE_STATE_ACTIVE = 0x02,
	FBS_DEVICE_STATE_ERROR = 0xff,
};

/**
 * enum fbs_dt_anchor_acting_role - Internal role played by a DT-Anchor
 * during the particular ranging round.
 * @FBS_DT_ANCHOR_ACTING_RESPONDER: The DT-Anchor acts as a responder.
 * @FBS_DT_ANCHOR_ACTING_INITIATOR: The DT-Anchor acts as an initiator.
 */
enum fbs_dt_anchor_acting_role {
	FBS_DT_ANCHOR_ACTING_RESPONDER,
	FBS_DT_ANCHOR_ACTING_INITIATOR,
};

/**
 * enum fbs_ranging_round_usage - Ranging mode.
 * @FBS_RANGING_ROUND_USAGE_OWR_UL_TDOA:
 *	One Way Ranging UL-TDoA.
 * @FBS_RANGING_ROUND_USAGE_SSTWR_DEFERRED:
 *	Single-Sided Two Way Ranging with Deferred Mode.
 * @FBS_RANGING_ROUND_USAGE_DSTWR_DEFERRED:
 *	Dual-Sided Two Way Ranging with Deferred Mode.
 * @FBS_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED:
 *	Single-Sided Two Way Ranging with Non-Deferred Mode.
 * @FBS_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED:
 *	Dual-Sided Two Way Ranging with Non-Deferred Mode.
 * @FBS_RANGING_ROUND_USAGE_OWR_DL_TDOA:
 *	One Way Ranging for DownLink Time Difference of Arrival measurement.
 * @FBS_RANGING_ROUND_USAGE_OWR_AOA:
 *	One Way Ranging for Angle of Arrival measurement
 * @FBS_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED:
 *	Enhanced SingleSided Two Way Ranging with Non-deferred Mode for
 *	Contention-based ranging.
 * @FBS_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED:
 *	Alternative Dual-Sided Two Way Ranging for Contention-based ranging.
 * @FBS_RANGING_ROUND_USAGE_MAX: Max value for the range check.
 * @FBS_RANGING_ROUND_USAGE_UNDEFINED: Ranging round usage is undefined.
 */
enum fbs_ranging_round_usage {
	FBS_RANGING_ROUND_USAGE_OWR_UL_TDOA,
	FBS_RANGING_ROUND_USAGE_SSTWR_DEFERRED,
	FBS_RANGING_ROUND_USAGE_DSTWR_DEFERRED,
	FBS_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED,
	FBS_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED,
	FBS_RANGING_ROUND_USAGE_OWR_DL_TDOA,
	FBS_RANGING_ROUND_USAGE_OWR_AOA,
	FBS_RANGING_ROUND_USAGE_ESS_TWR_NON_DEFERRED_CONTENTION_BASED,
	FBS_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED,
	FBS_RANGING_ROUND_USAGE_MAX = FBS_RANGING_ROUND_USAGE_ADS_TWR_CONTENTION_BASED,
	FBS_RANGING_ROUND_USAGE_UNDEFINED,
};

/**
 * enum fbs_dl_tdoa_ranging_method - Ranging method used by DT-Anchors in
 * DL-TDoA.
 * @FBS_DL_TDOA_RANGING_METHOD_SSTWR: Single-Sided Two Way Ranging.
 * @FBS_DL_TDOA_RANGING_METHOD_DSTWR: Dual-Sided Two Way Ranging.
 */
enum fbs_dl_tdoa_ranging_method {
	FBS_DL_TDOA_RANGING_METHOD_SSTWR,
	FBS_DL_TDOA_RANGING_METHOD_DSTWR,
};

/**
 * enum fbs_multi_node_mode - Multi-node mode.
 * @FBS_MULTI_NODE_MODE_UNICAST:
 *	Ranging between one initiator and one responder.
 * @FBS_MULTI_NODE_MODE_ONE_TO_MANY:
 *	Ranging between one initiator and multiple responders.
 * @FBS_MULTI_NODE_MODE_MANY_TO_MANY:
 *	Ranging between multiple initiators and multiple responders.
 * @FBS_MULTI_NODE_MODE_UNDEFINED:
 *	Ranging mode is undefined.
 */
enum fbs_multi_node_mode {
	FBS_MULTI_NODE_MODE_UNICAST,
	FBS_MULTI_NODE_MODE_ONE_TO_MANY,
	FBS_MULTI_NODE_MODE_MANY_TO_MANY,
	FBS_MULTI_NODE_MODE_UNDEFINED,
};

/**
 * enum fbs_measurement_report_originator - Originator (author) of the
 * Ranging Measurement Report Message (MRM).
 * @FBS_MEASUREMENT_REPORT_FROM_INITIATOR:
 *	The initiator sends a measurement report message.
 * @FBS_MEASUREMENT_REPORT_FROM_RESPONDER:
 *	The responder sends a measurement report message.
 */
enum fbs_measurement_report_originator {
	FBS_MEASUREMENT_REPORT_FROM_INITIATOR,
	FBS_MEASUREMENT_REPORT_FROM_RESPONDER,
};

/**
 * enum fbs_measurement_report_type - Type of the current
 * Ranging Measurement Report Message (MRM).
 * @FBS_MEASUREMENT_REPORT_TYPE_1: Measurement report message type 1.
 * @FBS_MEASUREMENT_REPORT_TYPE_2: Measurement report message type 2.
 * @FBS_MEASUREMENT_REPORT_TYPE_3: Measurement report message type 3.
 */
enum fbs_measurement_report_type {
	FBS_MEASUREMENT_REPORT_TYPE_1,
	FBS_MEASUREMENT_REPORT_TYPE_2,
	FBS_MEASUREMENT_REPORT_TYPE_3,
};

/**
 * enum fbs_owr_message_type - Type of the current
 * One Way Ranging (OWR) Message (Fira Based Session v2.0 5.9.13 OWR Message).
 * @FBS_OWR_MESSAGE_TYPE_UT_BLINK: UL-TDoA Blink Message.
 * @FBS_OWR_MESSAGE_TYPE_UT_SYNC: UL-TDoA Synchronization Message.
 * @FBS_OWR_MESSAGE_TYPE_DT_POLL: DL-TDoA Poll Message.
 * @FBS_OWR_MESSAGE_TYPE_DT_RESPONSE: DL-TDoA Response Message.
 * @FBS_OWR_MESSAGE_TYPE_DT_FINAL: DL-TDoA Final Message.
 * @FBS_OWR_MESSAGE_TYPE_AOA_MEASUREMENT: AoA Measurement Message.
 */
enum fbs_owr_message_type {
	FBS_OWR_MESSAGE_TYPE_UT_BLINK,
	FBS_OWR_MESSAGE_TYPE_UT_SYNC,
	FBS_OWR_MESSAGE_TYPE_DT_POLL,
	FBS_OWR_MESSAGE_TYPE_DT_RESPONSE,
	FBS_OWR_MESSAGE_TYPE_DT_FINAL,
	FBS_OWR_MESSAGE_TYPE_AOA_MEASUREMENT,
};

/**
 * enum fbs_schedule_mode - Slot scheduling mode used
 * during the ranging session.
 * @FBS_SCHEDULE_MODE_CONTENTION_BASED: Contention-based ranging.
 * @FBS_SCHEDULE_MODE_TIME_SCHEDULED: Time-scheduled ranging.
 * @FBS_SCHEDULE_MODE_HYBRID_BASED: Hybrid-based ranging (not supported).
 * @FBS_SCHEDULE_MODE_MAX: Max value defined
 * @FBS_SCHEDULE_MODE_UNDEFINED: Scheduled mode is undefined.
 */
enum fbs_schedule_mode {
	FBS_SCHEDULE_MODE_CONTENTION_BASED,
	FBS_SCHEDULE_MODE_TIME_SCHEDULED,
	FBS_SCHEDULE_MODE_HYBRID_BASED,
	FBS_SCHEDULE_MODE_MAX = FBS_SCHEDULE_MODE_HYBRID_BASED,
	FBS_SCHEDULE_MODE_UNDEFINED,
};

/**
 * enum fbs_rframe_config - Rframe configuration used to transmit/receive
 * ranging messages.
 * @FBS_RFRAME_CONFIG_SP0: Use SP0 mode.
 * @FBS_RFRAME_CONFIG_SP1: Use SP1 mode.
 * @FBS_RFRAME_CONFIG_SP2: RFU
 * @FBS_RFRAME_CONFIG_SP3: Use SP3 mode.
 */
enum fbs_rframe_config {
	FBS_RFRAME_CONFIG_SP0,
	FBS_RFRAME_CONFIG_SP1,
	FBS_RFRAME_CONFIG_SP2,
	FBS_RFRAME_CONFIG_SP3,
};

/**
 * enum fbs_prf_mode - Pulse Repetition Frequency mode
 * @FBS_PRF_MODE_BPRF: Base Pulse Repetition Frequency.
 * @FBS_PRF_MODE_HPRF: Higher Pulse Repetition Frequency.
 * @FBS_PRF_MODE_HPRF_HIGH_RATE: Higher Pulse Repetition Frequency allows
 * high data rate (27.2 Mbps and 31.2 Mbps).
 *
 * This enum is not used in the current implementation.
 */
enum fbs_prf_mode {
	FBS_PRF_MODE_BPRF,
	FBS_PRF_MODE_HPRF,
	FBS_PRF_MODE_HPRF_HIGH_RATE,
};

/**
 * enum fbs_preamble_duration - Duration of preamble in symbols.
 * @FBS_PREAMBLE_DURATION_32: 32 symbols duration.
 * @FBS_PREAMBLE_DURATION_64: 64 symbols duration.
 */
enum fbs_preamble_duration {
	FBS_PREAMBLE_DURATION_32,
	FBS_PREAMBLE_DURATION_64,
};

/**
 * enum fbs_sfd_id - Start-of-frame delimiter.
 * @FBS_SFD_ID_0: Delimiter is [0 +1 0 –1 +1 0 0 –1]
 * @FBS_SFD_ID_1: Delimiter is [ –1 –1 +1 –1 ]
 * @FBS_SFD_ID_2: Delimiter is [ –1 –1 –1 +1 –1 –1 +1 –1 ]
 * @FBS_SFD_ID_3: Delimiter is
 * [ –1 –1 –1 –1 –1 +1 +1 –1 –1 +1 –1 +1 –1 –1 +1 –1 ]
 * @FBS_SFD_ID_4: Delimiter is
 * [ –1 –1 –1 –1 –1 –1 –1 +1 –1 –1 +1 –1 –1 +1 –1 +1 –1 +1
 * –1 –1 –1 +1 +1 –1 –1 –1 +1 –1 +1 +1 –1 –1 ]
 */
enum fbs_sfd_id {
	FBS_SFD_ID_0,
	FBS_SFD_ID_1,
	FBS_SFD_ID_2,
	FBS_SFD_ID_3,
	FBS_SFD_ID_4,
};

/**
 * enum fbs_sts_segments - Number of STS segments.
 * @FBS_STS_SEGMENTS_0: No STS Segment (Rframe config SP0).
 * @FBS_STS_SEGMENTS_1: 1 STS Segment.
 * @FBS_STS_SEGMENTS_2: 2 STS Segments.
 * @FBS_STS_SEGMENTS_3: 3 STS Segments.
 * @FBS_STS_SEGMENTS_4: 4 STS Segments.
 */
enum fbs_sts_segments {
	FBS_STS_SEGMENTS_0,
	FBS_STS_SEGMENTS_1,
	FBS_STS_SEGMENTS_2,
	FBS_STS_SEGMENTS_3,
	FBS_STS_SEGMENTS_4,
};

/**
 * enum fbs_psdu_data_rate - Data rate used to exchange PSDUs.
 * @FBS_PSDU_DATA_RATE_6M81: 6.8Mb/s rate.
 * @FBS_PSDU_DATA_RATE_7M80: 7.8Mb/s rate.
 * @FBS_PSDU_DATA_RATE_27M2: 27.2Mb/s rate.
 * @FBS_PSDU_DATA_RATE_31M2: 31.2Mb/s rate.
 */
enum fbs_psdu_data_rate {
	FBS_PSDU_DATA_RATE_6M81,
	FBS_PSDU_DATA_RATE_7M80,
	FBS_PSDU_DATA_RATE_27M2,
	FBS_PSDU_DATA_RATE_31M2,
};

/**
 * enum fbs_phr_data_rate - Data rate used to exchange PHR.
 * @FBS_PHR_DATA_RATE_850K: 850kb/s rate.
 * @FBS_PHR_DATA_RATE_6M81: 6.8Mb/s rate.
 *
 * This enum is not used in the current implementation.
 */
enum fbs_phr_data_rate {
	FBS_PHR_DATA_RATE_850K,
	FBS_PHR_DATA_RATE_6M81,
};

/**
 * enum fbs_mac_fcs_type - Length of Frame Check Sequence.
 * @FBS_MAC_FCS_TYPE_CRC_16: 2 bytes sequence.
 * @FBS_MAC_FCS_TYPE_CRC_32: 4 bytes sequence.
 */
enum fbs_mac_fcs_type {
	FBS_MAC_FCS_TYPE_CRC_16,
	FBS_MAC_FCS_TYPE_CRC_32,
};

/**
 * enum fbs_sts_mode - Scrambled Timestamp Sequence modes.
 *
 * @FBS_STS_MODE_STATIC: Static STS mode.
 * @FBS_STS_MODE_DYNAMIC: Use a dynamic STS mode.
 * @FBS_STS_MODE_DYNAMIC_INDIVIDUAL_KEY:
 *	Use a dynamic STS mode with individual controlee key.
 * @FBS_STS_MODE_PROVISIONED: Use a provisioned STS mode.
 * @FBS_STS_MODE_PROVISIONED_INDIVIDUAL_KEY: Use a provisioned STS
 * mode with individual controlee key.
 */
enum fbs_sts_mode {
	FBS_STS_MODE_STATIC,
	FBS_STS_MODE_DYNAMIC,
	FBS_STS_MODE_DYNAMIC_INDIVIDUAL_KEY,
	FBS_STS_MODE_PROVISIONED,
	FBS_STS_MODE_PROVISIONED_INDIVIDUAL_KEY,
};

/**
 * enum fbs_data_transfer_status - Data transfer status.
 * @FBS_DATA_TRANSFER_STATUS_REPETITION_OK:
 *	If DATA_REPETITION_COUNT>0 and if
 *	SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG = Enable; it indicates that one
 *	Data transmission is completed in a RR.
 * @FBS_DATA_TRANSFER_STATUS_OK:
 *	For:
 *	- TWR: it indicates that the Application Data transmission is completed.
 *	- OWR: it indicates that the Application Data transmission and its
 *	  repetitions of DATA_REPETITION_COUNT is completed.
 * @FBS_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER:
 *	Application Data couldn’t be sent due to an unrecoverable error.
 * @FBS_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE:
 *	DATA_MESSAGE_SND is not accepted as no credit is available.
 * @FBS_DATA_TRANSFER_STATUS_ERROR_REJECTED:
 *	DATA_MESSAGE_SND packet sent in wrong state or Application Data Size
 *	exceeds the maximum size that can be sent in one Ranging Round.
 * @FBS_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED:
 *	Data transfer is not supported for given session type.
 * @FBS_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING:
 *	Application Data is being transmitted and the number of configured
 *	DATA_REPETITION_COUNT transmissions is not yet completed.
 * @FBS_DATA_TRANSFER_STATUS_INVALID_FORMAT:
 *	The format of the command DATA_MESSAGE_SND associated with this
 *	notification is incorrect (e.g, a parameter is missing, a parameter
 *	value is invalid).
 */
enum fbs_data_transfer_status {
	FBS_DATA_TRANSFER_STATUS_REPETITION_OK,
	FBS_DATA_TRANSFER_STATUS_OK,
	FBS_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER,
	FBS_DATA_TRANSFER_STATUS_ERROR_NO_CREDIT_AVAILABLE,
	FBS_DATA_TRANSFER_STATUS_ERROR_REJECTED,
	FBS_DATA_TRANSFER_STATUS_SESSION_TYPE_NOT_SUPPORTED,
	FBS_DATA_TRANSFER_STATUS_ERROR_DATA_TRANSFER_IS_ONGOING,
	FBS_DATA_TRANSFER_STATUS_INVALID_FORMAT,
};

/**
 * enum fbs_measurement_type - The different type of available measurements.
 * @FBS_MEASUREMENT_TYPE_RANGE: Measure only range.
 * @FBS_MEASUREMENT_TYPE_AOA: Measure range + unspecified AoA.
 * @FBS_MEASUREMENT_TYPE_AOA_AZIMUTH: Measure range + azimuth.
 * @FBS_MEASUREMENT_TYPE_AOA_ELEVATION: Measure range + elevation.
 * @FBS_MEASUREMENT_TYPE_AOA_AZIMUTH_ELEVATION: Measure range+azimuth+elevation.
 * @__FBS_MEASUREMENT_TYPE_AFTER_LAST: Internal use.
 */
enum fbs_measurement_type {
	FBS_MEASUREMENT_TYPE_RANGE,
	FBS_MEASUREMENT_TYPE_AOA,
	FBS_MEASUREMENT_TYPE_AOA_AZIMUTH,
	FBS_MEASUREMENT_TYPE_AOA_ELEVATION,
	FBS_MEASUREMENT_TYPE_AOA_AZIMUTH_ELEVATION,
	__FBS_MEASUREMENT_TYPE_AFTER_LAST,
};

/**
 * enum fbs_ranging_diagnostics_frame_report_flags - Activation flags for
 * different frame diagnostics information.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_NONE:
 *	No specific frame diagnostic report requested.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_AOAS:
 *	Report AOA in frame diagnostics.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_CFO: Report Clock Frequency Offset in
 *	report, as measured on the first Rx RFRAME in the round.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_EMITTER_SHORT_ADDR: Report the MAC
 *	emitter short address in frame diagnostics.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_SEGMENT_METRICS:
 *	Report Segment Metrics in frame diagnostics.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_CIRS:
 *	Report CIR in frame diagnostics.
 * @__FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_AFTER_LAST:
 *	Internal use.
 */
enum fbs_ranging_diagnostics_frame_report_flags {
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_NONE = 0,
	/* Bit 1 is deprecated., please do not use. */
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_AOAS = 1 << 1,
	/* Bit 3 is deprecated., please do not use. */
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_CFO = 1 << 3,
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_EMITTER_SHORT_ADDR = 1 << 4,
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_SEGMENT_METRICS = 1 << 5,
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_CIRS = 1 << 6,
	/* Last enum is to keep 8-bit enum. */
	__FBS_RANGING_DIAGNOSTICS_FRAME_REPORT_AFTER_LAST = 1 << 7,
};

/**
 * enum fbs_ranging_diagnostics_frame_reports_status_flags - Flags for the
 * invidual frame report's status bitfield.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_SUCCESS:
 *	False when then frame Rx has failed for some reason. Always true for Tx.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_WIFI_COEX:
 *	True if the Wifi Coex GPIO was on while transmitting the frame.
 * @FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_GRANT_DURATION_EXCEEDED:
 *	True if the MAX_GRANT_DURATION has been exceeded.
 */
enum fbs_ranging_diagnostics_frame_reports_status_flags {
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_SUCCESS = 1 << 0,
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_WIFI_COEX = 1 << 1,
	FBS_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_GRANT_DURATION_EXCEEDED = 1 << 2,
};

/**
 * enum fbs_sts_length - Number of symbols in a STS segment.
 * @FBS_STS_LENGTH_32: The STS length is 32 symbols.
 * @FBS_STS_LENGTH_64: The STS length is 64 symbols.
 * @FBS_STS_LENGTH_128: The STS length is 128 symbols.
 */
enum fbs_sts_length {
	FBS_STS_LENGTH_32,
	FBS_STS_LENGTH_64,
	FBS_STS_LENGTH_128,
};

/**
 * enum fbs_session_info_ntf_config - Configure session info notification.
 * @FBS_SESSION_INFO_NTF_CONFIG_DISABLED:
 *	Do not report range data.
 * @FBS_SESSION_INFO_NTF_CONFIG_ALWAYS:
 *	Report range data.
 * @FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY:
 *	Report range data if it is within proximity range defined by proximity
 *	parameters (NEAR_PROXIMITY_CONFIG/FAR).
 * @FBS_SESSION_INFO_NTF_CONFIG_AOA:
 *	Report range data in AoA upper and lower bound.  defined by AOA
 *	parameters (FBS_SESSION_PARAM_ATTR_SESSION_INFO_NTF_UPPER/
 *	LOWER_BOUND_AOA_AZIMUTH/ELEVATION)
 * @FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA:
 *	Report range data in AoA upper and lower bound as well as in proximity
 *	range.
 * @FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_CROSSING:
 *	Same as FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY, but issues notification
 *	on crossing of boundaries. As for now, same notif is sent for "enter"
 *	and "exit" events.
 * @FBS_SESSION_INFO_NTF_CONFIG_AOA_CROSSING:
 *	Same as FBS_SESSION_INFO_NTF_CONFIG_AOA, but issues notification on
 *	crossing of boundaries.  As for now, same notif is sent for "enter" and
 *	"exit" events.
 * @FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA_CROSSING:
 *	Same as FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA, but issues
 *	notification on crossing of boundaries. As for now, same notif is sent
 *	for "enter" and "exit" events.
 */
enum fbs_session_info_ntf_config {
	FBS_SESSION_INFO_NTF_CONFIG_DISABLED,
	FBS_SESSION_INFO_NTF_CONFIG_ALWAYS,
	FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY,
	FBS_SESSION_INFO_NTF_CONFIG_AOA,
	FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA,
	FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_CROSSING,
	FBS_SESSION_INFO_NTF_CONFIG_AOA_CROSSING,
	FBS_SESSION_INFO_NTF_CONFIG_PROXIMITY_AND_AOA_CROSSING,
};

/**
 * enum fbs_link_layer_mode - Link layer behavior.
 * @FBS_LINK_LAYER_MODE_BYPASS:
 *	No link layer overhead is added to the MDSDU.
 * @FBS_LINK_LAYER_MODE_CONNECTION_LESS:
 *	Link layer header is needed to provide addressing capabilities for data
 *	transmission.
 */
enum fbs_link_layer_mode {
	FBS_LINK_LAYER_MODE_BYPASS,
	FBS_LINK_LAYER_MODE_CONNECTION_LESS,
};

/**
 * enum fbs_message_id - Message identifiers, used in report and in messages.
 * @FBS_MESSAGE_ID_RANGING_INITIATION: Ranging Initiation Message.
 * @FBS_MESSAGE_ID_RANGING_RESPONSE: Ranging Response Message.
 * @FBS_MESSAGE_ID_RANGING_FINAL: Ranging Final Message, only for DS-TWR.
 * @FBS_MESSAGE_ID_CONTROL: Control Message, sent by the controller.
 * @FBS_MESSAGE_ID_MEASUREMENT_REPORT: Measurement Report Message.
 * @FBS_MESSAGE_ID_RESULT_REPORT: Ranging Result Report Message.
 * @FBS_MESSAGE_ID_CONTROL_UPDATE: Control Update Message.
 * @FBS_MESSAGE_ID_ONE_WAY_RANGING:
 *	One Way Ranging Message (see internal types).
 * @FBS_MESSAGE_ID_DATA: Data Message.
 * @FBS_MESSAGE_ID_RFRAME_MAX:
 *	Maximum identifier of messages transmitted as an RFRAME (without a
 *	payload).
 * @FBS_MESSAGE_ID_MAX: Maximum identifier of all messages.
 */
enum fbs_message_id {
	FBS_MESSAGE_ID_RANGING_INITIATION,
	FBS_MESSAGE_ID_RANGING_RESPONSE,
	FBS_MESSAGE_ID_RANGING_FINAL,
	FBS_MESSAGE_ID_CONTROL,
	FBS_MESSAGE_ID_MEASUREMENT_REPORT,
	FBS_MESSAGE_ID_RESULT_REPORT,
	FBS_MESSAGE_ID_CONTROL_UPDATE,
	FBS_MESSAGE_ID_ONE_WAY_RANGING,
	FBS_MESSAGE_ID_DATA,
	FBS_MESSAGE_ID_RFRAME_MAX = FBS_MESSAGE_ID_RANGING_FINAL,
	FBS_MESSAGE_ID_MAX = FBS_MESSAGE_ID_CONTROL_UPDATE,
};

/**
 * enum fbs_result_report_config_flags - result report config flags.
 * @FBS_RESULT_REPORT_CONFIG_REPORT_TOF: Report Time of flight.
 * @FBS_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION: Report elevation angle.
 * @FBS_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH: Report azimuth angle.
 * @FBS_RESULT_REPORT_CONFIG_REPORT_AOA_FOM: report AoA figure of merit.
 * @FBS_RESULT_REPORT_CONFIG_REPORT_ALL: Maximum value of the parameter.
 */
enum fbs_result_report_config_flags {
	FBS_RESULT_REPORT_CONFIG_REPORT_TOF = 1u << 0,
	FBS_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH = 1u << 1,
	FBS_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION = 1u << 2,
	FBS_RESULT_REPORT_CONFIG_REPORT_AOA_FOM = 1u << 3,
	FBS_RESULT_REPORT_CONFIG_REPORT_ALL = FBS_RESULT_REPORT_CONFIG_REPORT_TOF +
					      FBS_RESULT_REPORT_CONFIG_REPORT_AOA_AZIMUTH +
					      FBS_RESULT_REPORT_CONFIG_REPORT_AOA_ELEVATION +
					      FBS_RESULT_REPORT_CONFIG_REPORT_AOA_FOM,
};

/**
 * enum fbs_dt_location_coord_system_type -- Coordinate System Type
 * of a DT-Anchor.
 * @FBS_DT_LOCATION_COORD_WGS84: The location is given in WGS84 coordinate
 * system (longitude, latitude, altitude,(see &struct fbs_wgs84_location).
 * @FBS_DT_LOCATION_COORD_RELATIVE: The location is given in relative
 * coordinates system (see &struct fbs_relative_location).
 * @FBS_DT_LOCATION_COORD_INVALID: is a value in RSU range for test.
 */
enum fbs_dt_location_coord_system_type {
	FBS_DT_LOCATION_COORD_WGS84,
	FBS_DT_LOCATION_COORD_RELATIVE,
	FBS_DT_LOCATION_COORD_INVALID = 0xff,
};

/**
 * enum fbs_owr_dtm_timestamp_type -- DTM TX Timestamp type.
 * @FBS_OWR_DTM_TIMESTAMP_LOCAL_TIME_BASE: timestamp in local time base.
 * @FBS_OWR_DTM_TIMESTAMP_COMMON_TIME_BASE: timestamp in common time base
 * of the Initiator DT-Anchor.
 */
enum fbs_owr_dtm_timestamp_type {
	FBS_OWR_DTM_TIMESTAMP_LOCAL_TIME_BASE,
	FBS_OWR_DTM_TIMESTAMP_COMMON_TIME_BASE,
};

/**
 * enum fbs_owr_dtm_timestamp_len -- DTM TX Timestamp length.
 * @FBS_OWR_DTM_TIMESTAMP_40BITS: 40 bits timestamp.
 * @FBS_OWR_DTM_TIMESTAMP_64BITS: 64 bits timestamp.
 */
enum fbs_owr_dtm_timestamp_len {
	FBS_OWR_DTM_TIMESTAMP_40BITS,
	FBS_OWR_DTM_TIMESTAMP_64BITS,
};

/**
 * enum fbs_owr_utm_timestamp_len -- UTM TX Timestamp length.
 * @FBS_OWR_UTM_TIMESTAMP_NOT_PRESENT:   0 bits timestamp.
 * @FBS_OWR_UTM_TIMESTAMP_40BITS: 40 bits timestamp.
 * @FBS_OWR_UTM_TIMESTAMP_64BITS: 64 bits timestamp.
 */
enum fbs_owr_utm_timestamp_len {
	FBS_OWR_UTM_TIMESTAMP_NOT_PRESENT,
	FBS_OWR_UTM_TIMESTAMP_40BITS,
	FBS_OWR_UTM_TIMESTAMP_64BITS,
};

/**
 * enum fbs_ranging_round_control_flags - Ranging round control flags.
 * Below bits make sense when SCHEDULE_MODE is set to Time scheduled.
 *
 * @FBS_RANGING_ROUND_CONTROL_RRRM_EXPECTED:
 *	If set to 1, a Controller shall schedule an RRRM in the Ranging
 *	Device Management List (RDML).
 *	If set to 0, a Controller shall not schedule an RRRM in the RDML.
 * @FBS_RANGING_ROUND_CONTROL_CM_EXPECTED:
 *	If set to 1, a Controller shall send a CM in-band and a Controlee
 *	shall expect a CM in-band.
 *	If set to 0, a Controller shall not send a CM in-band and a Controlee
 *	shall not expect a CM in-band.
 * @FBS_RANGING_ROUND_CONTROL_RCP_EXCLUDED:
 *	If set to 1, RCP is excluded in Ranging Round, means CM is piggybacked
 *	with the RIM.
 *	If set to 0, RCP is included in Ranging Round.
 * @FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE:
 *	UWBS shall ignore this bit.
 * @FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE:
 *	If set to 1, the controller shall schedule the MRM to be sent from the
 *	responder(s) to the initiator in the RDML.
 *	If set to 0, the controller shall schedule the MRM to be sent from
 *	the initiator to the Responder(s) in the RDML.
 * @FBS_RANGING_ROUND_CONTROL_ALL: Maximum value of the parameter.
 */
enum fbs_ranging_round_control_flags {
	FBS_RANGING_ROUND_CONTROL_RRRM_EXPECTED = 1u << 0,
	FBS_RANGING_ROUND_CONTROL_CM_EXPECTED = 1u << 1,
	FBS_RANGING_ROUND_CONTROL_RCP_EXCLUDED = 1u << 2,
	/* Bit 3 to 5 are reserved for a future usage (RFU). */
	FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE = 1u << 6,
	FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE = 1u << 7,
	FBS_RANGING_ROUND_CONTROL_ALL = FBS_RANGING_ROUND_CONTROL_RRRM_EXPECTED +
					FBS_RANGING_ROUND_CONTROL_CM_EXPECTED +
					FBS_RANGING_ROUND_CONTROL_RCP_EXCLUDED +
					FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_PHASE +
					FBS_RANGING_ROUND_CONTROL_MEASUREMENT_REPORT_MESSAGE,
};

/**
 * enum fbs_owr_utm_device_id_len -- UTM Device Id length.
 * @FBS_OWR_UTM_DEVICE_ID_NOT_PRESENT: 0 bits device id.
 * @FBS_OWR_UTM_DEVICE_ID_16BITS: 16 bits device id.
 * @FBS_OWR_UTM_DEVICE_ID_32BITS: 32 bits device id.
 * @FBS_OWR_UTM_DEVICE_ID_64BITS: 64 bits device id.
 */
enum fbs_owr_utm_device_id_len {
	FBS_OWR_UTM_DEVICE_ID_NOT_PRESENT,
	FBS_OWR_UTM_DEVICE_ID_16BITS,
	FBS_OWR_UTM_DEVICE_ID_32BITS,
	FBS_OWR_UTM_DEVICE_ID_64BITS,
};

/**
 * enum fbs_multicast_update_status -- controlee change status after update
 * controller multicast list command.
 * @FBS_MULTICAST_UPDATE_STATUS_OK_MULTICAST_LIST_UPDATE:
 *	It shall be reported if the multicast list is updated (Add/Delete)
 *	successfully for the given Controlee.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_MULTICAST_LIST_FULL:
 *	It shall be reported for a Controlee if the multicast is full.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_KEY_FETCH_FAIL:
 *	It shall be reported for a Controlee if Session Key fetch from Secure
 *	Component is failed.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_ID_NOT_FOUND:
 *	It shall be reported for a Controlee if Sub-Session ID is not found in
 *	Secure Component.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_FOUND:
 *	It shall be reported for a Controlee if Sub-Session Key is not found in
 *	Secure Component.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_APPLICABLE:
 *	It shall be reported for a Controlee if Sub-Session Key is configured
 *	with STS config is other than 0x04 (Provisioned STS for Responder
 *	specific Sub-session Key).
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_SESSION_KEY_NOT_FOUND:
 *	It shall be reported for a Controlee if Sub-Session Key is configured
 *	but SESSION_KEY App configuration parameter is not programmed.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_NOT_FOUND:
 *	It shall be reported for a Controlee if its short address is not found
 *	while deleting its entry from multicast list.
 * @FBS_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_ALREADY_PRESENT:
 *	It shall be reported for a Controlee if its short address is already
 *	present in the multicast list.
 */
enum fbs_multicast_update_status {
	FBS_MULTICAST_UPDATE_STATUS_OK_MULTICAST_LIST_UPDATE,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_MULTICAST_LIST_FULL,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_KEY_FETCH_FAIL,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_ID_NOT_FOUND,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_FOUND,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_SUB_SESSION_KEY_NOT_APPLICABLE,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_SESSION_KEY_NOT_FOUND,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_NOT_FOUND,
	FBS_MULTICAST_UPDATE_STATUS_ERROR_ADDRESS_ALREADY_PRESENT,
	/* From 0x09 to 0xff, Reserved for Future Use. */
};

/**
 * enum fbs_data_segment_info -- information about data packet received.
 * @FBS_DATA_SEGMENT_FIRST: packet is the first one in data message.
 * @FBS_DATA_SEGMENT_LAST: packet is the last one in data message.
 * @__FBS_DATA_SEGMENT_AFTER_LAST: Internal use.
 */
enum fbs_data_segment_info {
	FBS_DATA_SEGMENT_FIRST = 1u << 0,
	FBS_DATA_SEGMENT_LAST = 1u << 1,
	__FBS_DATA_SEGMENT_AFTER_LAST = 1u << 7, /* to keep 8-bit enum */
};

/**
 * enum fbs_owr_aoa_measurement_ntf_period -- Period of sending SESSION_INFO_NTF.
 * @FBS_OWR_AOA_MEASUREMENT_NTF_PERIOD_SINGLE:
 *	Notification sent after every received frames.
 * @FBS_OWR_AOA_MEASUREMENT_NTF_PERIOD_AGGREGATED:
 *	Notification sent after MIN_FRAMES_PER_RR aggregated frames.
 */
enum fbs_owr_aoa_measurement_ntf_period {
	FBS_OWR_AOA_MEASUREMENT_NTF_PERIOD_SINGLE,
	FBS_OWR_AOA_MEASUREMENT_NTF_PERIOD_AGGREGATED,
};

/**
 * enum fbs_mac_address_mode - MAC addressing mode.
 * For more details in "8.3 Application Configuration Parameters" section of FiRa Std:
 * FiRa UCI Generic Technical Specification v2.0.0
 *
 * @FBS_MAC_ADDRESS_MODE_SHORT: MAC address is 2 bytes and 2 bytes to be used
 *	in MAC header.
 * @FBS_MAC_ADDRESS_MODE_MIXED: MAC address is 8 bytes and 2 bytes to be used
 *	in MAC header (Not supported).
 * @FBS_MAC_ADDRESS_MODE_EXTENDED: MAC address is 8 bytes and 8 bytes to be used
 *	in MAC header.
 */
enum fbs_mac_address_mode {
	FBS_MAC_ADDRESS_MODE_SHORT,
	FBS_MAC_ADDRESS_MODE_MIXED,
	FBS_MAC_ADDRESS_MODE_EXTENDED,
};

#ifdef __cplusplus
}
#endif
