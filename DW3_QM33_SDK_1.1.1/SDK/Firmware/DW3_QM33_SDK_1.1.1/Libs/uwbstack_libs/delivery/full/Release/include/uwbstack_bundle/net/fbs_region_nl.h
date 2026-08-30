/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * enum fbs_call - FBS calls identifiers.
 *
 * @FBS_CALL_GET_CAPABILITIES:
 *	Request Fira Based Session capabilities.
 * @FBS_CALL_SESSION_INIT:
 *	Initialize Fira Based Session session.
 * @FBS_CALL_SESSION_START:
 *	Start Fira Based Session session.
 * @FBS_CALL_SESSION_STOP:
 *	Stop Fira Based Session session.
 * @FBS_CALL_SESSION_DEINIT:
 *	Deinit Fira Based Session session.
 * @FBS_CALL_SESSION_SET_PARAMS:
 *	Set session parameters.
 * @FBS_CALL_NEW_CONTROLEE:
 *	Add a new controlee to a session.
 * @FBS_CALL_DEL_CONTROLEE:
 *	Delete controlee from the session.
 * @FBS_CALL_SESSION_NOTIFICATION:
 *	Notify session reports.
 * @FBS_CALL_SESSION_GET_PARAMS:
 *	Get session parameters.
 * @FBS_CALL_SESSION_GET_STATE:
 *	Get session state.
 * @FBS_CALL_SESSION_GET_COUNT:
 *	Get count of active and inactive sessions.
 * @FBS_CALL_GET_CONTROLEES:
 *	Get the list of controlees.
 * @FBS_CALL_UPDATE_DT_ANCHOR_RANGING_ROUNDS:
 *	Configure ranging rounds for DT-Anchor.
 * @FBS_CALL_UPDATE_DT_TAG_RANGING_ROUNDS:
 *	Configure ranging rounds for DT-Tag.
 * @FBS_CALL_DATA_MESSAGE_SND:
 *	Send Data Message to UWBS.
 * @FBS_CALL_DATA_MESSAGE_RCV:
 *	Receive Data Message from UWBS.
 * @FBS_CALL_DATA_SIZE_IN_RANGING:
 *	Get maximum data size in ranging round.
 * @FBS_CALL_DEVICE_STATUS_NOTIFICATION:
 *	Notify device state change.
 * @FBS_CALL_SESSION_STATUS_NOTIFICATION:
 *	Notify session status change.
 * @FBS_CALL_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_NOTIFICATION:
 *	Notify multicast list status change (add or delete).
 * @FBS_CALL_GET_RANGING_COUNT:
 *	Get the ranging count of a session.
 * @FBS_CALL_SESSION_SET_HUS_CONTROLLER_CONFIG:
 *	Configure a HUS controller.
 * @FBS_CALL_SESSION_SET_HUS_CONTROLEE_CONFIG:
 *	Configure a HUS controlee.
 * @FBS_CALL_MAX:
 *	Internal use.
 */
enum fbs_call {
	FBS_CALL_GET_CAPABILITIES,
	FBS_CALL_SESSION_INIT,
	FBS_CALL_SESSION_START,
	FBS_CALL_SESSION_STOP,
	FBS_CALL_SESSION_DEINIT,
	FBS_CALL_SESSION_SET_PARAMS,
	FBS_CALL_NEW_CONTROLEE,
	FBS_CALL_DEL_CONTROLEE,
	FBS_CALL_SESSION_NOTIFICATION,
	FBS_CALL_SESSION_GET_PARAMS,
	FBS_CALL_SESSION_GET_STATE,
	FBS_CALL_SESSION_GET_COUNT,
	FBS_CALL_GET_CONTROLEES,
	FBS_CALL_UPDATE_DT_ANCHOR_RANGING_ROUNDS,
	FBS_CALL_UPDATE_DT_TAG_RANGING_ROUNDS,
	FBS_CALL_DATA_MESSAGE_SND,
	FBS_CALL_DATA_MESSAGE_RCV,
	FBS_CALL_DATA_SIZE_IN_RANGING,
	FBS_CALL_DEVICE_STATUS_NOTIFICATION,
	FBS_CALL_SESSION_STATUS_NOTIFICATION,
	FBS_CALL_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_NOTIFICATION,
	FBS_CALL_GET_RANGING_COUNT,
	FBS_CALL_SESSION_SET_HUS_CONTROLLER_CONFIG,
	FBS_CALL_SESSION_SET_HUS_CONTROLEE_CONFIG,
	FBS_CALL_MAX,
};

/**
 * enum fbs_call_attrs - Fira Based Session call attributes.
 *
 * @FBS_CALL_ATTR_SESSION_ID:
 *	Session identifier.
 * @FBS_CALL_ATTR_SESSION_PARAMS:
 *	Session parameters.
 * @FBS_CALL_ATTR_CONTROLEES:
 *	Controlees information.
 * @FBS_CALL_ATTR_RANGING_DATA:
 *	Ranging data NTF.
 * @FBS_CALL_ATTR_CAPABILITIES:
 *	Capabilities.
 * @FBS_CALL_ATTR_SESSION_STATE:
 *	Session state.
 * @FBS_CALL_ATTR_SESSION_COUNT:
 *	Sessions count.
 * @FBS_CALL_ATTR_SEQUENCE_NUMBER:
 *	Session notification counter.
 * @FBS_CALL_ATTR_RANGING_DIAGNOSTICS:
 *	Diagnostic information.
 * @FBS_CALL_ATTR_DT_ANCHOR_ROUND_CONFIG:
 *	Configuration parameters of the ranging round for DT-Anchor.
 * @FBS_CALL_ATTR_DL_TDOA_ROUND_CONFIG_INDEX:
 *	Index of the ranging round (used for both DT-Anchor and DT-Tag).
 * @FBS_CALL_ATTR_DL_TDOA_ROUND_CONFIG_STATUS:
 *	Status of the reply to ranging round configuration command (used for
 *	both DT-Anchor and DT-Tag).
 * @FBS_CALL_ATTR_DATA_MESSAGE:
 *	Data Message.
 * @FBS_CALL_ATTR_CREDIT:
 *	Data Credit NTF.
 * @FBS_CALL_ATTR_DATA_TRANSFER_STATUS:
 *	Data Transfer Status NTF.
 * @FBS_CALL_ATTR_PSDUS_LOG:
 *	PSDUs logs.
 * @FBS_CALL_ATTR_DATA_SIZE_IN_RANGING:
 *	Data size in ranging query.
 * @FBS_CALL_ATTR_SESSION_STATUS:
 *	Session Status NTF.
 * @FBS_CALL_ATTR_REMAINING_MULTICAST_LIST_SIZE:
 *	Number of controlees that might still be added into the session.
 * @FBS_CALL_ATTR_CONTROLEE_STATUS_LIST:
 *	Status list for session update controller multicast list NTF.
 * @FBS_CALL_ATTR_SESSION_TYPE:
 *	Session type available during the FBS_CALL_SESSION_INIT.
 * @FBS_CALL_ATTR_DEVICE_STATE:
 *	FBS Device status, used with FBS_CALL_DEVICE_STATUS_NOTIFICATION.
 * @FBS_CALL_ATTR_SESSION_HANDLE:
 *	Session handle returned in response to FBS_CALL_SESSION_INIT.
 * @FBS_CALL_ATTR_STATUS_CODE:
 *	FBS status code. See &enum quwbs_fbs_status.
 * @FBS_CALL_ATTR_RANGING_COUNT:
 *	The number of successful ranging attempts during the session.
 * @FBS_CALL_ATTR_HUS_CONTROLLER_UPDATE_TIME:
 *	Time when the new HUS controller configuration applies. Currently not supported.
 * @FBS_CALL_ATTR_HUS_PHASE_LIST:
 *	HUS controller/controlee phase list.
 *
 * @FBS_CALL_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_CALL_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_CALL_ATTR_MAX:
 *	Internal use.
 */
enum fbs_call_attrs {
	FBS_CALL_ATTR_UNSPEC,
	FBS_CALL_ATTR_SESSION_ID,
	FBS_CALL_ATTR_SESSION_PARAMS,
	FBS_CALL_ATTR_CONTROLEES,
	FBS_CALL_ATTR_RANGING_DATA,
	FBS_CALL_ATTR_CAPABILITIES,
	FBS_CALL_ATTR_SESSION_STATE,
	FBS_CALL_ATTR_SESSION_COUNT,
	FBS_CALL_ATTR_SEQUENCE_NUMBER,
	FBS_CALL_ATTR_RANGING_DIAGNOSTICS,
	FBS_CALL_ATTR_DT_ANCHOR_ROUND_CONFIG,
	FBS_CALL_ATTR_DL_TDOA_ROUND_CONFIG_INDEX,
	FBS_CALL_ATTR_DL_TDOA_ROUND_CONFIG_STATUS,
	FBS_CALL_ATTR_DATA_MESSAGE,
	FBS_CALL_ATTR_CREDIT,
	FBS_CALL_ATTR_DATA_TRANSFER_STATUS,
	FBS_CALL_ATTR_PSDUS_LOG,
	FBS_CALL_ATTR_DATA_SIZE_IN_RANGING,
	FBS_CALL_ATTR_SESSION_STATUS,
	FBS_CALL_ATTR_REMAINING_MULTICAST_LIST_SIZE,
	FBS_CALL_ATTR_CONTROLEE_STATUS_LIST,
	FBS_CALL_ATTR_SESSION_TYPE,
	FBS_CALL_ATTR_DEVICE_STATE,
	FBS_CALL_ATTR_SESSION_HANDLE,
	FBS_CALL_ATTR_STATUS_CODE,
	FBS_CALL_ATTR_RANGING_COUNT,
	FBS_CALL_ATTR_HUS_CONTROLLER_UPDATE_TIME,
	FBS_CALL_ATTR_HUS_PHASE_LIST,

	__FBS_CALL_ATTR_AFTER_LAST,
	FBS_CALL_ATTR_MAX = __FBS_CALL_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_capability_attrs - Fira Based Session capabilities.
 *
 * @FBS_CAPABILITY_ATTR_VENDOR1_START:
 *	Begin of the first vendor specific range.
 * @FBS_CAPABILITY_ATTR_VENDOR1_END:
 *	End of the first vendor specific range.
 * @FBS_CAPABILITY_ATTR_VENDOR2_START:
 *	Begin of the second vendor specific range.
 * @FBS_CAPABILITY_ATTR_VENDOR2_END:
 *	End of the second vendor specific range.
 * @__FBS_CAPABILITY_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_CAPABILITY_ATTR_MAX:
 *	Internal use.
 */
enum fbs_capability_attrs {
	FBS_CAPABILITY_ATTR_VENDOR1_START = 0xa0,
	FBS_CAPABILITY_ATTR_VENDOR1_END = 0xdf,
	FBS_CAPABILITY_ATTR_VENDOR2_START = 0xe3,
	FBS_CAPABILITY_ATTR_VENDOR2_END = 0xff,

	__FBS_CAPABILITY_ATTR_AFTER_LAST,
	FBS_CAPABILITY_ATTR_MAX = __FBS_CAPABILITY_ATTR_AFTER_LAST - 1,
};

/**
 * enum fbs_session_param_attrs - Fira Based Session session parameters attributes.
 *
 * @FBS_SESSION_PARAM_ATTR_DEVICE_TYPE:
 *	Controlee (0) or controller (1)
 * @FBS_SESSION_PARAM_ATTR_DEVICE_ROLE:
 *	Responder (0) or initiator (1)
 * @FBS_SESSION_PARAM_ATTR_RANGING_ROUND_USAGE:
 *	OWR-UL-TDOA (0), SS-TWR-DEFERRED (1), DS-TWR-DEFERRED (2, default),
 *	SS-TWR-NON-DEFERRED (3), DS-TWR-NON-DEFERRED (4), OWR-DL-TDOA (5),
 *	OWR-AoA (6), eSS-TWR-NON-DEFERRED-CONTENTION-BASED (7),
 *	aDS-TWR-CONTENTION-BASED (8)
 * @FBS_SESSION_PARAM_ATTR_MULTI_NODE_MODE:
 *	Unicast (0), one to many (1) or many to many (2, unused)
 * @FBS_SESSION_PARAM_ATTR_SHORT_ADDR:
 *	Override device address for this session (UCI: DEVICE_MAC_ADDRESS)
 * @FBS_SESSION_PARAM_ATTR_DESTINATION_SHORT_ADDRS:
 *	Destination short addresses (UCI: DST_MAC_ADDRESS)
 * @FBS_SESSION_PARAM_ATTR_TIME0_NS:
 *	Absolute value of the initiation time in nanoseconds (default 0 - N/A).
 * @FBS_SESSION_PARAM_ATTR_SLOT_DURATION_RSTU:
 *	Duration of a slot in RSTU, default 2400. (2 ms)
 * @FBS_SESSION_PARAM_ATTR_BLOCK_DURATION_MS:
 *	Block size in unit of 1200 RSTU (same as ms), default 200.
 * @FBS_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS:
 *	Number of slots per ranging round, default 30 (UCI: SLOTS_PER_RR)
 * @FBS_SESSION_PARAM_ATTR_SESSION_TIME_BASE:
 *	Binary (byte array) containing the reference time base information
 *	(controlee or DT-Tag only):
 *	Octet 0: time base configuration.
 *	Octet 1-4: Session Handle of the reference session.
 *	Octet 5-8: Session offset time in microseconds.
 * @FBS_SESSION_PARAM_ATTR_BLOCK_STRIDE_LENGTH:
 *	Value of block striding, default 0, can be changed when the
 *	session is active [controller only]
 * @FBS_SESSION_PARAM_ATTR_MAX_NUMBER_OF_MEASUREMENTS:
 *	Unlimited (0, default) or limit of measurements (1-65535)
 * @FBS_SESSION_PARAM_ATTR_MAX_RR_RETRY:
 *	Number of failed ranging round attempts before stopping the session,
 *	or disabled (0, default) [controller only]
 * @FBS_SESSION_PARAM_ATTR_ROUND_HOPPING:
 *	Disabled (0, default) or enabled (1)
 * @FBS_SESSION_PARAM_ATTR_PRIORITY:
 *	Priority value, higher has more priority (1-100, default 50)
 * @FBS_SESSION_PARAM_ATTR_RANGING_ROUND_CONTROL:
 *	b0: ranging result report phase is disabled (0) or enabled (1, default)
 *	    report phase [controller only]
 *	b1: Control Message is sent in band (1, default) or not (0, not supported)
 *	b2: Control Message is sent separately (0, default) or piggybacked to ranging
 *	    initiation message (1) [Only valid for non-deferred mode TWR]
 *	b3-b6: RFUs, must be set to 0.
 *	b7: Measurement report message (MRM) is sent from the initiator (0, default),
 *	    or from the responder (1, not supported) [controller only]
 * @FBS_SESSION_PARAM_ATTR_SCHEDULE_MODE:
 *	Contention-based ranging (0), Time-scheduled ranging (1, default),
 *	Hybrid-based ranging (2, not supported)
 * @FBS_SESSION_PARAM_ATTR_IN_BAND_TERMINATION_ATTEMPT_COUNT:
 *	1-10, default 1 [controller only]
 * @FBS_SESSION_PARAM_ATTR_MIN_FRAMES_PER_RR:
 *	For OWR for AoA only, number of frames in RR (default 4).
 * @FBS_SESSION_PARAM_ATTR_MTU_SIZE:
 *	Maximum Transfer Unit (MTU), max data size allowed to be transmitted in
 *	one frame.
 * @FBS_SESSION_PARAM_ATTR_INTER_FRAME_INTERVAL:
 *	For OWR for AoA only, Interval between RFRAMES (in units of 1200 RSTU),
 *	(default 1)
 * @FBS_SESSION_PARAM_ATTR_OWR_AOA_MEASUREMENT_NTF_PERIOD:
 *	For OWR for AoA only, configures period of OWR for AoA measurement
 *	notifications: after every frame (0, default), after MIN_FRAMES_PER_RR
 *	number measurements (1).
 * @FBS_SESSION_PARAM_ATTR_CHANNEL_NUMBER:
 *	Channel for this session: 5 or 9 (default: 9)
 * @FBS_SESSION_PARAM_ATTR_PREAMBLE_CODE_INDEX:
 *	Preamble code for this session, BPRF (9-24), HPRF (25-32) (default: 10)
 * @FBS_SESSION_PARAM_ATTR_RFRAME_CONFIG:
 *	SP0 (0), SP1 (1) or SP3 (3, default)
 * @FBS_SESSION_PARAM_ATTR_PRF_MODE:
 *	BPRF (0, default), HPRF (1) or HPRF with high data rate (2)
 * @FBS_SESSION_PARAM_ATTR_PREAMBLE_DURATION:
 *	64 (1, default) or 32 (0, only for HPRF)
 * @FBS_SESSION_PARAM_ATTR_SFD_ID:
 *	BPRF (0 or 2), HPRF (1-4), default 2
 * @FBS_SESSION_PARAM_ATTR_NUMBER_OF_STS_SEGMENTS:
 *	0-2, default to 0 for SP0, default to 1 for SP1 & SP3, 2 not supported
 * @FBS_SESSION_PARAM_ATTR_PSDU_DATA_RATE:
 *	6.81 Mbps (0, default), 7.80 Mbps (1), 27.2 Mbps (2), 31.2 Mbps (3)
 * @FBS_SESSION_PARAM_ATTR_BPRF_PHR_DATA_RATE:
 *	850 kbps (0, default) or 6.81 Mbps (1)
 * @FBS_SESSION_PARAM_ATTR_MAC_FCS_TYPE:
 *	CRC16 (0, default) or CRC32 (1, not supported)
 * @FBS_SESSION_PARAM_ATTR_MEASUREMENT_SEQUENCE:
 *	Sequence of measurement steps. Configures antenna flexibility.
 * @FBS_SESSION_PARAM_ATTR_STS_CONFIG:
 *	See &enum fbs_sts_mode.
 *	- Static STS (0, default),
 *	- Dynamic STS (1),
 *	- Dynamic STS for controlee individual keys (2),
 *	- Provisioned STS (3),
 *	- Provisioned STS for controlee individual keys (4).
 * @FBS_SESSION_PARAM_ATTR_SUB_SESSION_ID:
 *	For dynamic STS for controlee individual key, sub session ID [controlee only]
 * @FBS_SESSION_PARAM_ATTR_VENDOR_ID:
 *	Vendor ID used to construct vUpper64 in static STS
 * @FBS_SESSION_PARAM_ATTR_STATIC_STS_IV:
 *	Static STS IV used to construct vUpper64 in static STS
 * @FBS_SESSION_PARAM_ATTR_SESSION_KEY:
 *	For provisioned sts only, session key.
 * @FBS_SESSION_PARAM_ATTR_SUB_SESSION_KEY:
 *	For dynamic or provisioned STS, sub session key [controlee only]
 * @FBS_SESSION_PARAM_ATTR_KEY_ROTATION:
 *	Disable (0, default) or enabled (1)
 * @FBS_SESSION_PARAM_ATTR_KEY_ROTATION_RATE:
 *	Key rotation rate parameter n, key rotated after 2^n rounds
 * @FBS_SESSION_PARAM_ATTR_MAC_PAYLOAD_ENCRYPTION:
 *	Enable or disable encryption of payload data.
 * @FBS_SESSION_PARAM_ATTR_RESULT_REPORT_CONFIG:
 *	See &enum fbs_result_report_config_flags
 * @FBS_SESSION_PARAM_ATTR_REPORT_RSSI:
 *	Report average RSSI of the round in result message, disabled (0,
 *	default) or enabled (1)
 * @FBS_SESSION_PARAM_ATTR_DIAGNOSTICS:
 *	Report diagnostic information on each round, disabled (0, default) or
 *	enabled (1)
 * @FBS_SESSION_PARAM_ATTR_DIAGNOSTICS_FRAME_REPORTS_FIELDS:
 *	Bitfield activating various frame diagnostics in the report (0: no frame
 *	diagnostic report, default).
 *	see &enum fbs_ranging_diagnostics_frame_report_flags
 * @FBS_SESSION_PARAM_ATTR_STS_LENGTH:
 *	Number of symbols in a STS segment. 32 (0x00), 64 (0x01, default) or 128
 *	symbols (0x02)
 * @FBS_SESSION_PARAM_ATTR_CAP_SIZE_MAX:
 *	Maximum for Contention Access Period Size,
 *	default: 29 (FBS_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS - 1) (UCI: CAP_SIZE_RANGE[0])
 * @FBS_SESSION_PARAM_ATTR_CAP_SIZE_MIN:
 *	Minimum for Contention Access Period Size, default 5 (UCI: CAP_SIZE_RANGE[1])
 * @FBS_SESSION_PARAM_ATTR_SESSION_INFO_NTF_CONFIG:
 *	Configure session info notification
 * @FBS_SESSION_PARAM_ATTR_NEAR_PROXIMITY_CONFIG_CM:
 *	Lower bound in mm above which the ranging notifications
 *	should be enabled when SESSION_INFO_NTF_CONFIG is set to "proximity" or
 *	"aoa_proximity"
 * @FBS_SESSION_PARAM_ATTR_FAR_PROXIMITY_CONFIG_CM:
 *	Upper bound in mm above which the ranging notifications
 *	should be disabled when SESSION_INFO_NTF_CONFIG is set to "proximity" or
 *	"aoa_proximity"
 * @FBS_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_AZIMUTH_2PI:
 *	Lower bound in rad_2pi for AOA azimuth above which the ranging
 *	notifications should automatically be enabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 32 bits (rad_2pi).
 *	Allowed values range from -180° to +180°. should be less than or equal to
 *	SESSION_INFO_NTF_UPPER_BOUND_AOA_AZIMUTH value. (default = -180)
 * @FBS_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_AZIMUTH_2PI:
 *	Upper bound in rad_2pi for AOA azimuth above which the ranging
 *	notifications should automatically be disabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 32 bits (rad_2pi).
 *	Allowed values range from -180° to +180°. Should be greater than or equal to
 *	SESSION_INFO_NTF_LOWER_BOUND_AOA_AZIMUTH value. (default = 180)
 * @FBS_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_ELEVATION_2PI:
 *	Lower bound in rad_2pi for AOA elevation above which the ranging
 *	notifications should automatically be enabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 16 bits (rad_2pi).
 *	Allowed values range from -90° to +90°. Should be less than or equal to
 *	SESSION_INFO_NTF_UPPER_BOUND_AOA_ELEVATION value. (default = -90)
 * @FBS_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_ELEVATION_2PI:
 *	Upper bound in rad_2pi for AOA elevation above which the ranging
 *	notifications should automatically be disabled if SESSION_INFO_NTF_CONFIG has
 *	bit is set to "aoa" or "aoa_proximity". It is a signed value on 16 bits
 *	(rad_2pi). Allowed values range from -90° to +90°. Should be greater than or
 *	equal to SESSION_INFO_NTF_LOWER_BOUND_AOA_ELEVATION value. (default = +90)
 * @FBS_SESSION_PARAM_ATTR_LINK_LAYER_MODE:
 *	Used to define link layer behavior. Possible values are:
 *	- 0x00 (link layer in bypass mode - default)
 *	- 0x01 (link layer in connection less mode).
 * @FBS_SESSION_PARAM_ATTR_DATA_REPETITION_COUNT:
 *	Indicates the number of times the data shall be repeated in consecutive ranging rounds.
 *	Available only on OWR sessions. 0x00 means no repetition shall be performed at all and 0xff
 *	means the same MDSDU shall transmitted infinite number of times.
 * @FBS_SESSION_PARAM_ATTR_DATA_TRANSFER_STATUS_NTF_CONFIG:
 *	Used to configure the SESSION_DATA_TRANSFER_STATUS_NTF.
 *	If disabled (0, default), then the UWBS shall not send
 *	SESSION_DATA_TRANSFER_STATUS_NTF for every Application Data transmission
 *	except for last transmission.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_IS_REFERENCE:
 *	Sets or resets the DT-Anchor to be the global time reference of a DL-TDoA network.
 * @FBS_SESSION_PARAM_ATTR_DL_TDOA_RESPONDER_TOF:
 *	Includes (1) or excludes (0, default) the responder_tof field in a response DTM.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_RANGING_METHOD:
 *	Type of the ranging method used in message exchange between DT-Anchors.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_TYPE:
 *	Type of the TX timestamps (local time base vs common time base) included in DTMs.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_LEN:
 *	Length of the TX timestamps (40-bit vs 64-bit) included in DTMs.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_HOP_COUNT_PRESENT:
 *	True when Hop Count field is included in Poll DTMs, false otherwise.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_CFO_PRESENT:
 *	True when DT-Anchor CFO estimate is included in Response DTMs, false otherwise.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_PRESENT:
 *	True when the information about DT-Anchor location is included in DTMs, false otherwise.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_TYPE:
 *	Type of coordinate system of DT-Anchor (0: WGS84, 1: relative).
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION:
 *	Binary (byte array) that encodes coordinates of DT-Anchor location.
 * @FBS_SESSION_PARAM_ATTR_DT_ANCHOR_ACTIVE_RANGING_ROUNDS_INFO_PRESENT:
 *	True when the information about active ranging rounds for DT-Anchor is
 *	included in DTMs, false otherwise.
 * @FBS_SESSION_PARAM_ATTR_DT_TAG_BLOCK_SKIPPING:
 *	Number of blocks to be skipped by the DT-Tag after the current ranging block.
 * @FBS_SESSION_PARAM_ATTR_REPORT_PSDUS:
 *	Report psdu data on each round, disabled (0, default) or enabled (1)
 * @FBS_SESSION_PARAM_ATTR_UT_TX_INTERVAL:
 *	Average transmission interval of Blink UTM or Synchronization UTMs.
 *	32-bit unsigned value, default: 2000ms.
 * @FBS_SESSION_PARAM_ATTR_UT_RANDOM_WINDOW:
 *	Length of randomization window for Blink UTM or Synchronization UTM.
 *	32-bit unsigned value, default: 0 ms.
 * @FBS_SESSION_PARAM_ATTR_UT_TX_TIMESTAMP_LEN:
 *	Presence and length of TX timestamps in UTM, not present (0, default),
 *	40-bit timestamp (1), 64-bit timestamp (2).
 * @FBS_SESSION_PARAM_ATTR_UT_DEVICE_ID_LEN:
 *	Specify length of Device ID included in UTMs.
 *	Device ID not included (0, default), 16-bit Device ID (1),
 *	32-bit Device Id (2), 64-bit Device ID (3)
 * @FBS_SESSION_PARAM_ATTR_UT_DEVICE_ID:
 *	Specify Device ID included in UTMs.
 *
 * @FBS_SESSION_PARAM_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_SESSION_PARAM_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_SESSION_PARAM_ATTR_MAX:
 *	Internal use.
 */
enum fbs_session_param_attrs {
	FBS_SESSION_PARAM_ATTR_UNSPEC,
	/* Main session parameters. */
	FBS_SESSION_PARAM_ATTR_DEVICE_TYPE,
	FBS_SESSION_PARAM_ATTR_DEVICE_ROLE,
	FBS_SESSION_PARAM_ATTR_RANGING_ROUND_USAGE,
	FBS_SESSION_PARAM_ATTR_MULTI_NODE_MODE,
	FBS_SESSION_PARAM_ATTR_SHORT_ADDR,
	FBS_SESSION_PARAM_ATTR_DESTINATION_SHORT_ADDRS,
	/* Timings. */
	FBS_SESSION_PARAM_ATTR_TIME0_NS,
	FBS_SESSION_PARAM_ATTR_SLOT_DURATION_RSTU,
	FBS_SESSION_PARAM_ATTR_BLOCK_DURATION_MS,
	FBS_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS,
	FBS_SESSION_PARAM_ATTR_SESSION_TIME_BASE,
	FBS_SESSION_PARAM_ATTR_BLOCK_STRIDE_LENGTH,
	/* Behaviour. */
	FBS_SESSION_PARAM_ATTR_MAX_NUMBER_OF_MEASUREMENTS,
	FBS_SESSION_PARAM_ATTR_MAX_RR_RETRY,
	FBS_SESSION_PARAM_ATTR_ROUND_HOPPING,
	FBS_SESSION_PARAM_ATTR_PRIORITY,
	FBS_SESSION_PARAM_ATTR_RANGING_ROUND_CONTROL,
	FBS_SESSION_PARAM_ATTR_SCHEDULE_MODE,
	FBS_SESSION_PARAM_ATTR_IN_BAND_TERMINATION_ATTEMPT_COUNT,
	/* Behaviour OWR for AoA ranging. */
	FBS_SESSION_PARAM_ATTR_MIN_FRAMES_PER_RR,
	FBS_SESSION_PARAM_ATTR_MTU_SIZE,
	FBS_SESSION_PARAM_ATTR_INTER_FRAME_INTERVAL,
	FBS_SESSION_PARAM_ATTR_OWR_AOA_MEASUREMENT_NTF_PERIOD,
	/* Radio. */
	FBS_SESSION_PARAM_ATTR_CHANNEL_NUMBER,
	FBS_SESSION_PARAM_ATTR_PREAMBLE_CODE_INDEX,
	FBS_SESSION_PARAM_ATTR_RFRAME_CONFIG,
	FBS_SESSION_PARAM_ATTR_PRF_MODE,
	FBS_SESSION_PARAM_ATTR_PREAMBLE_DURATION,
	FBS_SESSION_PARAM_ATTR_SFD_ID,
	FBS_SESSION_PARAM_ATTR_NUMBER_OF_STS_SEGMENTS,
	FBS_SESSION_PARAM_ATTR_PSDU_DATA_RATE,
	FBS_SESSION_PARAM_ATTR_BPRF_PHR_DATA_RATE,
	FBS_SESSION_PARAM_ATTR_MAC_FCS_TYPE,
	/* Measurement Sequence. */
	FBS_SESSION_PARAM_ATTR_MEASUREMENT_SEQUENCE,
	/* STS and crypto. */
	FBS_SESSION_PARAM_ATTR_STS_CONFIG,
	FBS_SESSION_PARAM_ATTR_SUB_SESSION_ID,
	FBS_SESSION_PARAM_ATTR_VENDOR_ID,
	FBS_SESSION_PARAM_ATTR_STATIC_STS_IV,
	FBS_SESSION_PARAM_ATTR_SESSION_KEY,
	FBS_SESSION_PARAM_ATTR_SUB_SESSION_KEY,
	FBS_SESSION_PARAM_ATTR_KEY_ROTATION,
	FBS_SESSION_PARAM_ATTR_KEY_ROTATION_RATE,
	/* Report. */
	FBS_SESSION_PARAM_ATTR_RESULT_REPORT_CONFIG,
	FBS_SESSION_PARAM_ATTR_REPORT_RSSI,
	/* Diagnostics. */
	FBS_SESSION_PARAM_ATTR_DIAGNOSTICS,
	FBS_SESSION_PARAM_ATTR_DIAGNOSTICS_FRAME_REPORTS_FIELDS,
	/* Misc */
	FBS_SESSION_PARAM_ATTR_STS_LENGTH,
	/* Contention-based ranging. */
	FBS_SESSION_PARAM_ATTR_CAP_SIZE_MAX,
	FBS_SESSION_PARAM_ATTR_CAP_SIZE_MIN,
	/* Range data notification enable. */
	FBS_SESSION_PARAM_ATTR_SESSION_INFO_NTF_CONFIG,
	FBS_SESSION_PARAM_ATTR_NEAR_PROXIMITY_CONFIG_CM,
	FBS_SESSION_PARAM_ATTR_FAR_PROXIMITY_CONFIG_CM,
	FBS_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_AZIMUTH_2PI,
	FBS_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_AZIMUTH_2PI,
	FBS_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_ELEVATION_2PI,
	FBS_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_ELEVATION_2PI,
	/* Data transfer. */
	FBS_SESSION_PARAM_ATTR_LINK_LAYER_MODE,
	FBS_SESSION_PARAM_ATTR_DATA_REPETITION_COUNT,
	FBS_SESSION_PARAM_ATTR_DATA_TRANSFER_STATUS_NTF_CONFIG,
	/* Parameters specific for DL-TDoA. */
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_IS_REFERENCE,
	FBS_SESSION_PARAM_ATTR_DL_TDOA_RESPONDER_TOF,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_RANGING_METHOD,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_TYPE,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_LEN,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_HOP_COUNT_PRESENT,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_CFO_PRESENT,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_PRESENT,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_TYPE,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION,
	FBS_SESSION_PARAM_ATTR_DT_ANCHOR_ACTIVE_RANGING_ROUNDS_INFO_PRESENT,
	FBS_SESSION_PARAM_ATTR_DT_TAG_BLOCK_SKIPPING,
	/* Debug. */
	FBS_SESSION_PARAM_ATTR_REPORT_PSDUS,
	/* Vendor specific parameter. */
	FBS_SESSION_PARAM_ATTR_MAC_PAYLOAD_ENCRYPTION,
	/* Parameters specific for UL-TDoA. */
	FBS_SESSION_PARAM_ATTR_UT_TX_INTERVAL,
	FBS_SESSION_PARAM_ATTR_UT_RANDOM_WINDOW,
	FBS_SESSION_PARAM_ATTR_UT_TX_TIMESTAMP_LEN,
	FBS_SESSION_PARAM_ATTR_UT_DEVICE_ID_LEN,
	FBS_SESSION_PARAM_ATTR_UT_DEVICE_ID,
	__FBS_SESSION_PARAM_ATTR_AFTER_LAST,
	FBS_SESSION_PARAM_ATTR_MAX = __FBS_SESSION_PARAM_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_call_controlee_attrs - Fira Based Session controlee parameters attributes.
 *
 * @FBS_CALL_CONTROLEE_ATTR_SHORT_ADDR:
 *	Controlee short address.
 * @FBS_CALL_CONTROLEE_ATTR_SUB_SESSION_ID:
 *	Controlee sub session identifier
 * @FBS_CALL_CONTROLEE_ATTR_SUB_SESSION_KEY:
 *	Controlee sub session key
 *
 * @FBS_CALL_CONTROLEE_ATTR_UNSPEC: Invalid command.
 * @__FBS_CALL_CONTROLEE_ATTR_AFTER_LAST: Internal use.
 * @FBS_CALL_CONTROLEE_ATTR_MAX: Internal use.
 */
enum fbs_call_controlee_attrs {
	FBS_CALL_CONTROLEE_ATTR_UNSPEC,
	FBS_CALL_CONTROLEE_ATTR_SHORT_ADDR,
	FBS_CALL_CONTROLEE_ATTR_SUB_SESSION_ID,
	FBS_CALL_CONTROLEE_ATTR_SUB_SESSION_KEY,

	__FBS_CALL_CONTROLEE_ATTR_AFTER_LAST,
	FBS_CALL_CONTROLEE_ATTR_MAX = __FBS_CALL_CONTROLEE_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_call_dt_anchor_round_config_attrs - Attributes for ranging round
 * configuration for DT-Anchor.
 *
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_INDEX:
 *	Index of the ranging round.
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_ACTING_ROLE:
 *	DT-Anchor's acting role for this ranging round.
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SHORT_ADDR:
 *	Short address of DT-Anchor Responders (nested parameter) for
 *	this ranging round (applicable if DT-Anchor's acting role is Initiator).
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SLOT:
 *	Slot indexes for Response DTMs for DT-Anchor Responders (nested parameter)
 *	for this ranging round (applicable if DT-Anchor's acting role is *Initiator).
 *
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_UNSPEC: Invalid command.
 * @__FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST: Internal use.
 * @FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_MAX: Internal use.
 */
enum fbs_call_dt_anchor_round_config_attrs {
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_UNSPEC,
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_INDEX,
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_ACTING_ROLE,
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SHORT_ADDR,
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SLOT,

	__FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST,
	FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_MAX =
		__FBS_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_ranging_data_attrs_ranging_measurement_type - Values for
 * FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE attribute.
 *
 * @FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_UL_TDOA:
 *	UL-TDoA Ranging Measurement (OWR UL-TDoA).
 * @FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_TWR:
 *	Two Way Ranging Measurement (SS-TWR, DS-TWR).
 * @FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_DL_TDOA:
 *	DL-TDoA Ranging Measurement (OWR DL-TDoA).
 * @FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_OWR_AOA:
 *	OWR for AoA Measurement.
 */
enum fbs_ranging_data_attrs_ranging_measurement_type {
	FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_UL_TDOA,
	FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_TWR,
	FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_DL_TDOA,
	FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_OWR_AOA,
};

/**
 * enum fbs_ranging_data_attrs - Fira Based Session ranging data parameter attributes.
 *
 * @FBS_RANGING_DATA_ATTR_BLOCK_INDEX:
 *	Current block index.
 * @FBS_RANGING_DATA_ATTR_TIMESTAMP_NS:
 *	Timestamp in nanoseconds in the CLOCK_MONOTONIC time reference.
 * @FBS_RANGING_DATA_ATTR_RANGING_INTERVAL_MS:
 *	Current ranging interval (block size * (stride + 1)) in unit of
 *	1200 RSTU (same as ms).
 * @FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE:
 *	Type of ranging measurement
 * @FBS_RANGING_DATA_ATTR_MEASUREMENTS:
 *	Measurements, see fbs_ranging_data_measurements.
 *
 * @FBS_RANGING_DATA_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_RANGING_DATA_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_RANGING_DATA_ATTR_MAX:
 *	Internal use.
 */
enum fbs_ranging_data_attrs {
	FBS_RANGING_DATA_ATTR_UNSPEC,
	FBS_RANGING_DATA_ATTR_BLOCK_INDEX,
	FBS_RANGING_DATA_ATTR_TIMESTAMP_NS,
	FBS_RANGING_DATA_ATTR_RANGING_INTERVAL_MS,
	FBS_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE,
	FBS_RANGING_DATA_ATTR_MEASUREMENTS,

	__FBS_RANGING_DATA_ATTR_AFTER_LAST,
	FBS_RANGING_DATA_ATTR_MAX = __FBS_RANGING_DATA_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_ranging_data_measurements_attrs - Fira Based Session ranging data
 *	measurements attributes.
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_SHORT_ADDR:
 *	Address of the participing device.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_STOPPED:
 *	If present, ranging was stopped as requested [controller only].
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_STATUS:
 *	Status of the measurement (0: success, 1: tx failed, 2: rx timeout,
 *	3: rx phy dec error, 4: rx phy toa error, 5: rx phy sts error,
 *	6: rx mac dec error, 7: rx mac ie dec error, 8: rx mac ie missing)
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_SLOT_INDEX:
 *	In case of failure slot index where the error was detected.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_NLOS:
 *	Set if in non line of sight.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOS:
 *	Set if in line of sight.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_FRAME_SEQUENCE_NUMBER:
 *	Sequence number as received in MHR.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_BLOCK_INDEX:
 *	Block Index number as received in the OWR message from the Advertiser
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_DISTANCE_CM:
 *	Distance in cm.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA:
 *	Local AoA measurement,
 *	cf. fbs_ranging_data_measurements_aoa_attrs.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_AZIMUTH:
 *	Local AoA measurement for azimuth,
 *	cf. fbs_ranging_data_measurements_aoa_attrs.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_ELEVATION:
 *	Local AoA measurement for elevation,
 *	cf. fbs_ranging_data_measurements_aoa_attrs.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_2PI:
 *	Estimation of reception angle in the azimuth of the participing device.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_PI:
 *	Estimation of reception angle in elevation of the participing device.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_FOM_100:
 *	Estimation of azimuth reliability of the participing device.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_FOM_100:
 *	Estimation of elevation reliability of the participing device.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_RSSI:
 *	RSSI for received frames during the ranging round,
 *	reported as Q7.1.
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_RANGING_DATA_MEASUREMENTS_ATTR_MAX:
 *	Internal use.
 */
enum fbs_ranging_data_measurements_attrs {
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_UNSPEC,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_SHORT_ADDR,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_STOPPED,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_STATUS,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_SLOT_INDEX,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_NLOS,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOS,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_FRAME_SEQUENCE_NUMBER,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_BLOCK_INDEX,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_DISTANCE_CM,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_AZIMUTH,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_ELEVATION,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_2PI,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_PI,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_FOM_100,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_FOM_100,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_RSSI,

	__FBS_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST,
	FBS_RANGING_DATA_MEASUREMENTS_ATTR_MAX = __FBS_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_ranging_data_measurements_aoa_attrs - Fira Based Session ranging AoA
 *	measurements attributes.
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_RX_ANTENNA_SET:
 *	Antenna set index.
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_2PI:
 *	Estimation of reception angle.
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_FOM_100:
 *	Estimation of local AoA reliability.
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_PDOA_2PI:
 *	Estimation of reception phase difference.
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_MAX:
 *	Internal use.
 */
enum fbs_ranging_data_measurements_aoa_attrs {
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_UNSPEC,
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_RX_ANTENNA_SET,
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_2PI,
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_FOM_100,
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_PDOA_2PI,

	__FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST,
	FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_MAX =
		__FBS_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_ranging_data_measurements_dt_attrs - Fira Based Session ranging
 *	DL-TDoA measurement attributes (per each message received, used only for DT-Tag).
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_SHORT_ADDR:
 *	Short MAC address of the device sending the message.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_STATUS:
 *	Status of the message reception (see &enum quwbs_fbs_status).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_MESSAGE_TYPE:
 *	Type of the message received.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_TYPE:
 *	Type of the TX timestamp (local time base vs common time base) included
 *	in the received message.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_LEN:
 *	Length of the TX timestamp (40-bit vs 64-bit) included
 *	in the received message.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_LEN:
 *	Length of the RX timestamp calculated during the reception
 *	of the received message.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_PRESENT:
 *	True when the information about DT-Anchor location is included
 *	in the measurement, false otherwise.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_TYPE:
 *	Type of the coordinate system of DT-Anchor location (0: WGS84, 1:
 *	relative) (if included).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES_LEN:
 *	Number of active ranging round indexes included in the measurement.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_BLOCK_INDEX:
 *	Index of the current ranging block.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ROUND_INDEX:
 *	Index of the current ranging round.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_NLOS:
 *	Indicates if the reception of the message was in Line of Sight or not.
 *	Value 0xff means that it was unable to be determined (or not supported
 *	at all).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_2PI:
 *	AoA Azimuth in degrees measured by the DT-Tag during the reception
 *	(encoded as Q9.7).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_FOM_100:
 *	Reliability of the estimated AoA Azimuth measured by the DT-Tag
 *	during the reception (range: 0-100).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_2PI:
 *	AoA Elevation in degrees measured by the DT-Tag during the reception
 *	(encoded as Q9.7).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_FOM_100:
 *	Reliability of the estimated AoA Elevation measured by the DT-Tag
 *	during the reception (range: 0-100).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RSSI:
 *	RSSI measured by the DT-Tag during the reception (encoded as Q7.1).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_RCTU:
 *	TX timestamp included in the received message (unit: RCTU).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_RCTU:
 *	RX timestamp calculated during the reception of the received message
 *	(unit: RCTU).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_REMOTE_CFO:
 *	Clock frequency offset of a Responder DT-Anchor with respect to the
 *	Initiator DT-Anchor of the ranging round as included in the received
 *	message (encoded as Q6.10).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_LOCAL_CFO:
 *	Clock frequency offset measured locally with respect to the DT-Anchor
 *	that sent the message received (encoded as Q6.10).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_REPLY_TIME_RCTU:
 *	Reply time of the Initiator DT-Anchor measured between the reception of
 *	Response DTM and the transmission of Final DTM (unit: RCTU,
 *	used only in DS-TWR).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RESPONDER_REPLY_TIME_RCTU:
 *	Reply time of the Responder DT-Anchor measured between the reception of
 *	Poll DTM and the transmission of Response DTM (unit: RCTU).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_RESPONDER_TOF_RCTU:
 *	Time of Flight measured between the Initiator DT-Anchor and the
 *	Responder DT-Anchor (for SS-TWR it's calculated by Initiator DT-Anchor
 *	and included in Poll DTM and for DS-TWR it's calculated by Responder
 *	DT-Anchor and included in Response DTM, unit: RCTU).
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION:
 *	Location coordinates of DT-Anchor that sent the message received.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES:
 *	List of active ranging round indexes in which the DT-Anchor that sent
 *	the message received participates.
 *
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_MAX:
 *	Internal use.
 */
enum fbs_ranging_data_measurements_dt_attrs {
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_UNSPEC,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_SHORT_ADDR,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_STATUS,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_MESSAGE_TYPE,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_TYPE,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_LEN,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_LEN,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_PRESENT,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_TYPE,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES_LEN,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_BLOCK_INDEX,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ROUND_INDEX,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_NLOS,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_2PI,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_FOM_100,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_2PI,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_FOM_100,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RSSI,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_RCTU,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_RCTU,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_REMOTE_CFO,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_LOCAL_CFO,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_REPLY_TIME_RCTU,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_RESPONDER_REPLY_TIME_RCTU,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_RESPONDER_TOF_RCTU,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES,

	__FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST,
	FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_MAX =
		__FBS_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_session_param_meas_seq_step_attrs - Fira Based Session measurement
 *	sequence step attributes.
 *
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MEASUREMENT_TYPE:
 *	The type of measurement to perform during the step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_N_MEASUREMENTS:
 *	The number of times this type of measurement shall be performed
 *	during the step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SET_NONRANGING:
 *	The antenna set to use to receive the non-rfames during the step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SETS_RANGING:
 *	The antenna set to use to receive the rframes frame during the step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_NONRANGING:
 *	The antenna set to use to transmit the non-rframes during the step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_RANGING:
 *	The antenna set to use to transmit the rframes during the step.
 *
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MAX:
 *	Internal use.
 */
enum fbs_session_param_meas_seq_step_attrs {
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_UNSPEC,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MEASUREMENT_TYPE,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_N_MEASUREMENTS,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SET_NONRANGING,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SETS_RANGING,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_NONRANGING,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_RANGING,

	__FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MAX =
		__FBS_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_session_params_meas_seq_step_sets_attrs - Attributes of the
 * Fira Based Session RX antenna sets to use during a step.
 *
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_0:
 *	Antenna set used to receive all rframes for range, azimuth and elevation
 *	steps or initial rframe for azimuth_elevation step.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_1:
 *	Antenna set used to receive final rframes for azimuth_elevation step.
 *
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_UNSPEC:
 *	Invalid command.
 * @FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_MAX:
 *	Internal use.
 * @__FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST:
 *	Internal use.
 */
enum fbs_session_params_meas_seq_step_sets_attrs {
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_UNSPEC,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_0,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_1,

	__FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST,
	FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_MAX =
		__FBS_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_ranging_diagnostics_attrs - Fira Based Session ranging diagnostic attributes.
 *
 * @FBS_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS_NB:
 *	Number of diagnostic reports.
 * @FBS_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS:
 *	Pointer to diagnostic reports.
 *
 * @FBS_RANGING_DIAGNOSTICS_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_RANGING_DIAGNOSTICS_ATTR_MAX:
 *	Internal use.
 */
enum fbs_ranging_diagnostics_attrs {
	FBS_RANGING_DIAGNOSTICS_ATTR_UNSPEC,
	FBS_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS_NB,
	FBS_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS,

	__FBS_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST,
	FBS_RANGING_DIAGNOSTICS_ATTR_MAX = __FBS_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_data_message_attrs - Fira Based Session data_message parameter attributes.
 *
 * @FBS_DATA_MESSAGE_ATTR_SESSION_ID:
 *	Session id.
 * @FBS_DATA_MESSAGE_ATTR_MAC_ADDRESS:
 *	MAC short address, depending on the data message (SND or RCV) it can be
 *	sender or receiver address.
 * @FBS_DATA_MESSAGE_ATTR_UCI_SEQ_NR:
 *	Sequence number from UCI Data Message.
 * @FBS_DATA_MESSAGE_ATTR_STATUS:
 *	Status of the received message.
 * @FBS_DATA_MESSAGE_ATTR_SEGMENT_INFO:
 *	Information about segment of Application Data Payload (first, last).
 * @FBS_DATA_MESSAGE_ATTR_APP_DATA_LEN:
 *	Application Data length.
 * @FBS_DATA_MESSAGE_ATTR_APP_DATA:
 *	Application Data.
 *
 * @FBS_DATA_MESSAGE_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_DATA_MESSAGE_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_DATA_MESSAGE_ATTR_MAX:
 *	Internal use.
 */
enum fbs_data_message_attrs {
	FBS_DATA_MESSAGE_ATTR_UNSPEC,
	FBS_DATA_MESSAGE_ATTR_SESSION_ID,
	FBS_DATA_MESSAGE_ATTR_MAC_ADDRESS,
	FBS_DATA_MESSAGE_ATTR_UCI_SEQ_NR,
	FBS_DATA_MESSAGE_ATTR_STATUS,
	FBS_DATA_MESSAGE_ATTR_SEGMENT_INFO,
	FBS_DATA_MESSAGE_ATTR_APP_DATA_LEN,
	FBS_DATA_MESSAGE_ATTR_APP_DATA,

	__FBS_DATA_MESSAGE_ATTR_AFTER_LAST,
	FBS_DATA_MESSAGE_ATTR_MAX = __FBS_DATA_MESSAGE_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_credit_attrs - Fira Based Session credit parameter attributes.
 *
 * @FBS_CREDIT_ATTR_AVAILABLE:
 *	Credit availability.
 *
 * @FBS_CREDIT_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_CREDIT_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_CREDIT_ATTR_MAX:
 *	Internal use.
 */
enum fbs_credit_attrs {
	FBS_CREDIT_ATTR_UNSPEC,
	FBS_CREDIT_ATTR_AVAILABLE,

	__FBS_CREDIT_ATTR_AFTER_LAST,
	FBS_CREDIT_ATTR_MAX = __FBS_CREDIT_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_data_transfer_status_attrs - Fira Based Session data_transfer_status
 * 	parameter attributes.
 *
 * @FBS_DATA_TRANSFER_STATUS_ATTR_UCI_SEQ_NR:
 *	The Sequence Number identifying the UCI Data Message.
 * @FBS_DATA_TRANSFER_STATUS_ATTR_CODE:
 *	Status code.
 * @FBS_DATA_TRANSFER_STATUS_ATTR_TX_COUNT:
 *	Indicates the number of times Application Data with the same
 *	UCI Sequence Number has been transmitted.
 *
 * @FBS_DATA_TRANSFER_STATUS_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_DATA_TRANSFER_STATUS_ATTR_MAX:
 *	Internal use.
 */
enum fbs_data_transfer_status_attrs {
	FBS_DATA_TRANSFER_STATUS_ATTR_UNSPEC,
	FBS_DATA_TRANSFER_STATUS_ATTR_UCI_SEQ_NR,
	FBS_DATA_TRANSFER_STATUS_ATTR_CODE,
	FBS_DATA_TRANSFER_STATUS_ATTR_TX_COUNT,

	__FBS_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST,
	FBS_DATA_TRANSFER_STATUS_ATTR_MAX = __FBS_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST - 1
};

/**
 * enum fbs_session_status_attrs - Fira Based Session session status attributes.
 *
 * @FBS_SESSION_STATUS_ATTR_STATE:
 *	Session state see &enum quwbs_fbs_session_state.
 * @FBS_SESSION_STATUS_ATTR_REASON_CODE:
 *	Reason of the state change see &enum quwbs_fbs_reason_code.
 *
 * @FBS_SESSION_STATUS_ATTR_UNSPEC:
 *	Invalid command.
 * @__FBS_SESSION_STATUS_ATTR_AFTER_LAST:
 *	Internal use.
 * @FBS_SESSION_STATUS_ATTR_MAX:
 *	Internal use.
 */
enum fbs_session_status_attrs {
	FBS_SESSION_STATUS_ATTR_UNSPEC,
	FBS_SESSION_STATUS_ATTR_STATE,
	FBS_SESSION_STATUS_ATTR_REASON_CODE,

	__FBS_SESSION_STATUS_ATTR_AFTER_LAST,
	FBS_SESSION_STATUS_ATTR_MAX = __FBS_SESSION_STATUS_ATTR_AFTER_LAST - 1
};

#ifdef __cplusplus
}
#endif
