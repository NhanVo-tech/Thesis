/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "net/fbs_region_nl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * enum fira_capability_attrs - FBS FiRa capabilities.
 *
 * @FIRA_CAPABILITY_ATTR_FIRA_PHY_VERSION_RANGE:
 *	FiRa PHY version range supported, ex: 0x01010202 -> support from 1.1
 *	to 2.2.
 * @FIRA_CAPABILITY_ATTR_FIRA_MAC_VERSION_RANGE:
 *	FiRa MAC version range supported, ex: @0x01010202 -> support from 1.1
 *	to 2.2.
 * @FIRA_CAPABILITY_ATTR_DEVICE_CLASS:
 *	Class of FiRa device.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE_RESPONDER:
 *	Acting as a controlee/responder supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE_INITIATOR:
 *	Acting as a controlee/initiator supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER_RESPONDER:
 *	Acting as a controller/responder supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER_INITIATOR:
 *	Acting as a controller/initiator supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER:
 *	Acting as a controller supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE:
 *	Acting as a controlee supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_RESPONDER:
 *	Acting as a responder supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_INITIATOR:
 *	Acting as an initiator supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_SYNC_ANCHOR:
 *	Acting as an uplink TDOA synchronization anchor supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_ANCHOR:
 *	Acting as an uplink TDOA anchor supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_TAG:
 *	Acting as an uplink TDOA tag supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_ADVERTISER:
 *	Acting as an advertiser supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_OBSERVER:
 *	Acting as an observer supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_DT_ANCHOR:
 *	Acting as a downlink TDOA anchor supported.
 * @FIRA_CAPABILITY_ATTR_DEVICE_ROLE_DT_TAG:
 *	Acting as a downlink TDOA tag supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_TIME_STRUCT_BLOCK_BASED_SCHEDULING:
 *	Ranging time structure block based scheduling supported.
 * @FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_UNICAST:
 *	Unicast supported.
 * @FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_ONE_TO_MANY:
 *	One to many supported.
 * @FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_MANY_TO_MANY:
 *	Many to many supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_ROUND_USAGE_SS_TWR:
 *	SS-TWR supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_ROUND_USAGE_DS_TWR:
 *	DS-TWR supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_SS_TWR_DEFERRED:
 *	SS-TWR in deferred mode supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_SS_TWR_NON_DEFERRED:
 *	SS-TWR in non-deferred mode supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_DS_TWR_DEFERRED:
 *	DS-TWR in deferred mode supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_DS_TWR_NON_DEFERRED:
 *	DS-TWR in non-deferred mode supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_UL_TDOA:
 *	OWR UL-TDOA supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_DL_TDOA:
 *	OWR DL-TDOA supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_AOA:
 *	OWR for AOA supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_ESS_TWR_NON_DEFERRED_FOR_CBR:
 *	eSS for Contention-Based ranging supported.
 * @FIRA_CAPABILITY_ATTR_RANGING_METHOD_ADS_TWR_NON_DEFERRED_FOR_CBR:
 *	aDS for Contention-Based ranging supported.
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_CONTROLEES_MAX:
 *	Maximum number of controlee, no limit if not present (but the size of
 *	the control message is a limit).
 * @FIRA_CAPABILITY_ATTR_ROUND_HOPPING:
 *	Round hopping supported.
 * @FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_CONTENTION_BASED:
 *	Contention based schedule mode supported.
 * @FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_TIME_SCHEDULED:
 *	Time scheduled mode supported.
 * @FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_HYBRID:
 *	Hybrid schedule mode supported.
 * @FIRA_CAPABILITY_ATTR_BLOCK_STRIDING:
 *	Block striding supported.
 * @FIRA_CAPABILITY_ATTR_UWB_INITIATION_TIME:
 *	UWB initiation time supported.
 * @FIRA_CAPABILITY_ATTR_EXTENDED_MAC_ADDRESS:
 *	Extended MAC address supported.
 * @FIRA_CAPABILITY_ATTR_SUSPEND_RANGING_SUPPORT:
 *	Suspend ranging supported.
 * @FIRA_CAPABILITY_ATTR_CHANNEL_NUMBER:
 *	Bitmask, with bits 5, 6, 8, 9, 10, 12, 13 and 14 representing
 *	the corresponding channel support.
 * @FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP0:
 *	SP0 supported.
 * @FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP1:
 *	SP1 supported.
 * @FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP3:
 *	SP3 supported.
 * @FIRA_CAPABILITY_ATTR_CC_CONSTRAINT_LENGTH_SYSTEMATIC_CE:
 *	Systematic convolutional encoding supported.
 * @FIRA_CAPABILITY_ATTR_CC_CONSTRAINT_LENGTH_NON_SYSTEMATIC_CE:
 *	Non-systematic convolutional encoding supported.
 * @FIRA_CAPABILITY_ATTR_PRF_MODE_BPRF:
 *	BPRF supported.
 * @FIRA_CAPABILITY_ATTR_PRF_MODE_HPRF:
 *	HPRF supported.
 * @FIRA_CAPABILITY_ATTR_PREAMBLE_DURATION_64:
 *	Preamble duration 64 symbols supported.
 * @FIRA_CAPABILITY_ATTR_PREAMBLE_DURATION_32:
 *	Preamble duration 32 symbols supported.
 * @FIRA_CAPABILITY_ATTR_SFD_ID_0:
 *	SFD 0 supported.
 * @FIRA_CAPABILITY_ATTR_SFD_ID_1:
 *	SFD 1 supported.
 * @FIRA_CAPABILITY_ATTR_SFD_ID_2:
 *	SFD 2 supported.
 * @FIRA_CAPABILITY_ATTR_SFD_ID_3:
 *	SFD 3 supported.
 * @FIRA_CAPABILITY_ATTR_SFD_ID_4:
 *	SFD 4 supported.
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_0:
 *	0 segment for STS supported (SP0).
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_1:
 *	1 segment for STS supported.
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_2:
 *	2 segments for STS supported.
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_3:
 *	3 segments for STS supported.
 * @FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_4:
 *	4 segments for STS supported.
 * @FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_6M81:
 *	6.81 Mbps support.
 * @FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_7M80:
 *	7.80 Mbps support.
 * @FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_27M2:
 *	27.2 Mbps support.
 * @FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_31M2:
 *	31.2 Mbps support.
 * @FIRA_CAPABILITY_ATTR_BPRF_PHR_DATA_RATE_850K:
 *	850 kbps for PHR in BPRF supported.
 * @FIRA_CAPABILITY_ATTR_BPRF_PHR_DATA_RATE_6M81:
 *	6.81 Mbps for PHR in BPRF supported.
 * @FIRA_CAPABILITY_ATTR_MAC_FCS_TYPE_CRC32:
 *	CRC32 supported.
 * @FIRA_CAPABILITY_ATTR_RX_ANTENNA_PAIRS:
 *	Number of antenna pairs for RX.
 * @FIRA_CAPABILITY_ATTR_TX_ANTENNAS:
 *	Number of antennas for TX.
 * @FIRA_CAPABILITY_ATTR_STS_STATIC:
 *	Static STS supported.
 * @FIRA_CAPABILITY_ATTR_STS_DYNAMIC:
 *	Dynamic STS supported.
 * @FIRA_CAPABILITY_ATTR_STS_DYNAMIC_INDIVIDUAL_KEY:
 *	Dynamic STS for controlee individual keys supported.
 * @FIRA_CAPABILITY_ATTR_STS_PROVISIONED:
 *	Provisioned STS supported.
 * @FIRA_CAPABILITY_ATTR_STS_PROVISIONED_INDIVIDUAL_KEY:
 *	Provisioned STS for controlee individual keys supported.
 * @FIRA_CAPABILITY_ATTR_SESSION_DYNAMIC_KEY_LENGTHS:
 *	Dynamic STS supported key lengths, with: b0: 128 bits b1: 256 bits
 *	b2-7: RFU
 * @FIRA_CAPABILITY_ATTR_SESSION_PROVISIONED_KEY_LENGTHS:
 *	Provisioned STS supported key lengths, with: b0: 128 bits b1: 256 bits
 *	b2-7: RFU
 * @FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP0:
 *	STS Packet Configuration 0 supported.
 * @FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP1:
 *	STS Packet Configuration 1 supported.
 * @FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP3:
 *	STS Packet Configuration 3 supported.
 * @FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENGTH_32:
 *	32 symbols STS segment length supported.
 * @FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENGTH_64:
 *	64 symbols STS segment length supported.
 * @FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENTGH_128:
 *	128 symbols STS segment length supported.
 * @FIRA_CAPABILITY_ATTR_AOA_AZIMUTH:
 *	AoA in azimuth supported.
 * @FIRA_CAPABILITY_ATTR_AOA_AZIMUTH_FULL:
 *	AoA in azimuth supported on full circle (front/back difference).
 * @FIRA_CAPABILITY_ATTR_AOA_ELEVATION:
 *	AoA in elevation supported.
 * @FIRA_CAPABILITY_ATTR_AOA_FOM:
 *	AoA FOM supported.
 * @FIRA_CAPABILITY_ATTR_DT_ANCHOR_MAX_ACTIVE_RR:
 *	Maximum number of active ranging rounds
 *	supported by the device when acting as DT-Anchor.
 * @FIRA_CAPABILITY_ATTR_DT_TAG_MAX_ACTIVE_RR:
 *	Maximum number of active ranging rounds
 *	supported by the device when acting as DT-Tag.
 * @FIRA_CAPABILITY_ATTR_DT_TAG_BLOCK_SKIPPING:
 *	Support for block skipping feature by DT-Tag.
 *
 * @FIRA_CAPABILITY_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CAPABILITY_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CAPABILITY_ATTR_MAX: Internal use.
 */
enum fira_capability_attrs {
	FIRA_CAPABILITY_ATTR_UNSPEC,
	/* Main session capabilities. */
	FIRA_CAPABILITY_ATTR_FIRA_PHY_VERSION_RANGE,
	FIRA_CAPABILITY_ATTR_FIRA_MAC_VERSION_RANGE,
	FIRA_CAPABILITY_ATTR_DEVICE_CLASS,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE_RESPONDER,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE_INITIATOR,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER_RESPONDER,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER_INITIATOR,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLLER,
	FIRA_CAPABILITY_ATTR_DEVICE_TYPE_CONTROLEE,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_RESPONDER,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_INITIATOR,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_SYNC_ANCHOR,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_ANCHOR,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_UT_TAG,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_ADVERTISER,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_OBSERVER,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_DT_ANCHOR,
	FIRA_CAPABILITY_ATTR_DEVICE_ROLE_DT_TAG,
	FIRA_CAPABILITY_ATTR_RANGING_TIME_STRUCT_BLOCK_BASED_SCHEDULING,
	FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_UNICAST,
	FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_ONE_TO_MANY,
	FIRA_CAPABILITY_ATTR_MULTI_NODE_MODE_MANY_TO_MANY,
	/* Ranging Round structure. */
	FIRA_CAPABILITY_ATTR_RANGING_ROUND_USAGE_SS_TWR,
	FIRA_CAPABILITY_ATTR_RANGING_ROUND_USAGE_DS_TWR,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_SS_TWR_DEFERRED,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_SS_TWR_NON_DEFERRED,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_DS_TWR_DEFERRED,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_DS_TWR_NON_DEFERRED,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_UL_TDOA,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_DL_TDOA,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_OWR_AOA,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_ESS_TWR_NON_DEFERRED_FOR_CBR,
	FIRA_CAPABILITY_ATTR_RANGING_METHOD_ADS_TWR_NON_DEFERRED_FOR_CBR,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_CONTROLEES_MAX,
	/* Behaviour. */
	FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_CONTENTION_BASED,
	FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_TIME_SCHEDULED,
	FIRA_CAPABILITY_ATTR_SCHEDULE_MODE_HYBRID,
	FIRA_CAPABILITY_ATTR_ROUND_HOPPING,
	FIRA_CAPABILITY_ATTR_BLOCK_STRIDING,
	FIRA_CAPABILITY_ATTR_UWB_INITIATION_TIME,
	FIRA_CAPABILITY_ATTR_EXTENDED_MAC_ADDRESS,
	FIRA_CAPABILITY_ATTR_SUSPEND_RANGING_SUPPORT,
	/* Radio. */
	FIRA_CAPABILITY_ATTR_CHANNEL_NUMBER,
	FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP0,
	FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP1,
	FIRA_CAPABILITY_ATTR_RFRAME_CONFIG_SP3,
	FIRA_CAPABILITY_ATTR_CC_CONSTRAINT_LENGTH_SYSTEMATIC_CE,
	FIRA_CAPABILITY_ATTR_CC_CONSTRAINT_LENGTH_NON_SYSTEMATIC_CE,
	FIRA_CAPABILITY_ATTR_PRF_MODE_BPRF,
	FIRA_CAPABILITY_ATTR_PRF_MODE_HPRF,
	FIRA_CAPABILITY_ATTR_PREAMBLE_DURATION_64,
	FIRA_CAPABILITY_ATTR_PREAMBLE_DURATION_32,
	FIRA_CAPABILITY_ATTR_SFD_ID_0,
	FIRA_CAPABILITY_ATTR_SFD_ID_1,
	FIRA_CAPABILITY_ATTR_SFD_ID_2,
	FIRA_CAPABILITY_ATTR_SFD_ID_3,
	FIRA_CAPABILITY_ATTR_SFD_ID_4,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_0,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_1,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_2,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_3,
	FIRA_CAPABILITY_ATTR_NUMBER_OF_STS_SEGMENTS_4,
	FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_6M81,
	FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_7M80,
	FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_27M2,
	FIRA_CAPABILITY_ATTR_PSDU_DATA_RATE_31M2,
	FIRA_CAPABILITY_ATTR_BPRF_PHR_DATA_RATE_850K,
	FIRA_CAPABILITY_ATTR_BPRF_PHR_DATA_RATE_6M81,
	FIRA_CAPABILITY_ATTR_MAC_FCS_TYPE_CRC32,
	/* Antenna. */
	FIRA_CAPABILITY_ATTR_RX_ANTENNA_PAIRS,
	FIRA_CAPABILITY_ATTR_TX_ANTENNAS,
	/* STS and crypto capabilities. */
	FIRA_CAPABILITY_ATTR_STS_STATIC,
	FIRA_CAPABILITY_ATTR_STS_DYNAMIC,
	FIRA_CAPABILITY_ATTR_STS_DYNAMIC_INDIVIDUAL_KEY,
	FIRA_CAPABILITY_ATTR_STS_PROVISIONED,
	FIRA_CAPABILITY_ATTR_STS_PROVISIONED_INDIVIDUAL_KEY,
	FIRA_CAPABILITY_ATTR_SESSION_DYNAMIC_KEY_LENGTHS,
	FIRA_CAPABILITY_ATTR_SESSION_PROVISIONED_KEY_LENGTHS,
	FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP0,
	FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP1,
	FIRA_CAPABILITY_ATTR_PPDU_FORMAT_SP3,
	FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENGTH_32,
	FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENGTH_64,
	FIRA_CAPABILITY_ATTR_STS_SEGMENT_LENTGH_128,
	/* Report. */
	FIRA_CAPABILITY_ATTR_AOA_AZIMUTH,
	FIRA_CAPABILITY_ATTR_AOA_AZIMUTH_FULL,
	FIRA_CAPABILITY_ATTR_AOA_ELEVATION,
	FIRA_CAPABILITY_ATTR_AOA_FOM,
	/* Specific for DL-TDoA. */
	FIRA_CAPABILITY_ATTR_DT_ANCHOR_MAX_ACTIVE_RR,
	FIRA_CAPABILITY_ATTR_DT_TAG_MAX_ACTIVE_RR,
	FIRA_CAPABILITY_ATTR_DT_TAG_BLOCK_SKIPPING,

	__FIRA_CAPABILITY_ATTR_AFTER_LAST,
	FIRA_CAPABILITY_ATTR_MAX = __FIRA_CAPABILITY_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_call_attrs - FiRa call attributes.
 *
 * @FIRA_CALL_ATTR_SESSION_ID:
 *	Session identifier.
 * @FIRA_CALL_ATTR_SESSION_PARAMS:
 *	Session parameters.
 * @FIRA_CALL_ATTR_CONTROLEES:
 *	Controlees information.
 * @FIRA_CALL_ATTR_RANGING_DATA:
 *	Ranging data NTF.
 * @FIRA_CALL_ATTR_CAPABILITIES:
 *	Capabilities.
 * @FIRA_CALL_ATTR_SESSION_STATE:
 *	Session state.
 * @FIRA_CALL_ATTR_SESSION_COUNT:
 *	Sessions count.
 * @FIRA_CALL_ATTR_SEQUENCE_NUMBER:
 *	Session notification counter.
 * @FIRA_CALL_ATTR_RANGING_DIAGNOSTICS:
 *	Diagnostic information.
 * @FIRA_CALL_ATTR_DT_ANCHOR_ROUND_CONFIG:
 *	Configuration parameters of the ranging round for DT-Anchor.
 * @FIRA_CALL_ATTR_DL_TDOA_ROUND_CONFIG_INDEX:
 *	Index of the ranging round (used for both DT-Anchor and DT-Tag).
 * @FIRA_CALL_ATTR_DL_TDOA_ROUND_CONFIG_STATUS:
 *	Status of the reply to ranging round configuration command (used for
 *	both DT-Anchor and DT-Tag).
 * @FIRA_CALL_ATTR_DATA_MESSAGE:
 *	Data Message.
 * @FIRA_CALL_ATTR_CREDIT:
 *	Data Credit NTF.
 * @FIRA_CALL_ATTR_DATA_TRANSFER_STATUS:
 *	Data Transfer Status NTF.
 * @FIRA_CALL_ATTR_PSDUS_LOG:
 *	PSDUs logs.
 * @FIRA_CALL_ATTR_DATA_SIZE_IN_RANGING:
 *	Data size in ranging query.
 * @FIRA_CALL_ATTR_SESSION_STATUS:
 *	Session Status NTF.
 * @FIRA_CALL_ATTR_REMAINING_MULTICAST_LIST_SIZE:
 *	Number of controlees that might still be added into the session.
 * @FIRA_CALL_ATTR_CONTROLEE_STATUS_LIST:
 *	Status list for session update controller multicast list NTF.
 * @FIRA_CALL_ATTR_SESSION_HANDLE:
 *	Session handle returned in response to FBS_CALL_SESSION_INIT.
 * @FIRA_CALL_ATTR_RANGING_COUNT:
 *	The number of successful ranging attempts in a session.
 *
 * @FIRA_CALL_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CALL_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CALL_ATTR_MAX: Internal use.
 */
enum fira_call_attrs {
	FIRA_CALL_ATTR_UNSPEC,
	FIRA_CALL_ATTR_SESSION_ID,
	FIRA_CALL_ATTR_SESSION_PARAMS,
	FIRA_CALL_ATTR_CONTROLEES,
	FIRA_CALL_ATTR_RANGING_DATA,
	FIRA_CALL_ATTR_CAPABILITIES,
	FIRA_CALL_ATTR_SESSION_STATE,
	FIRA_CALL_ATTR_SESSION_COUNT,
	FIRA_CALL_ATTR_SEQUENCE_NUMBER,
	FIRA_CALL_ATTR_RANGING_DIAGNOSTICS,
	FIRA_CALL_ATTR_DT_ANCHOR_ROUND_CONFIG,
	FIRA_CALL_ATTR_DL_TDOA_ROUND_CONFIG_INDEX,
	FIRA_CALL_ATTR_DL_TDOA_ROUND_CONFIG_STATUS,
	FIRA_CALL_ATTR_DATA_MESSAGE,
	FIRA_CALL_ATTR_CREDIT,
	FIRA_CALL_ATTR_DATA_TRANSFER_STATUS,
	FIRA_CALL_ATTR_PSDUS_LOG,
	FIRA_CALL_ATTR_DATA_SIZE_IN_RANGING,
	FIRA_CALL_ATTR_SESSION_STATUS,
	FIRA_CALL_ATTR_REMAINING_MULTICAST_LIST_SIZE,
	FIRA_CALL_ATTR_CONTROLEE_STATUS_LIST,
	FIRA_CALL_ATTR_SESSION_HANDLE = FBS_CALL_ATTR_SESSION_HANDLE,
	FIRA_CALL_ATTR_RANGING_COUNT = FBS_CALL_ATTR_RANGING_COUNT,

	__FIRA_CALL_ATTR_AFTER_LAST,
	FIRA_CALL_ATTR_MAX = __FIRA_CALL_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_session_param_attrs - FiRa session parameters attributes.
 *
 * @FIRA_SESSION_PARAM_ATTR_DEVICE_TYPE:
 *	Controlee (0) or controller (1)
 * @FIRA_SESSION_PARAM_ATTR_DEVICE_ROLE:
 *	Responder (0) or initiator (1)
 * @FIRA_SESSION_PARAM_ATTR_RANGING_ROUND_USAGE:
 *	OWR-UL-TDOA (0), SS-TWR-DEFERRED (1), DS-TWR-DEFERRED (2, default),
 *	SS-TWR-NON-DEFERRED (3), DS-TWR-NON-DEFERRED (4), OWR-DL-TDOA (5),
 *	OWR-AoA (6), eSS-TWR-NON-DEFERRED-CONTENTION-BASED (7),
 *	aDS-TWR-CONTENTION-BASED (8)
 * @FIRA_SESSION_PARAM_ATTR_MULTI_NODE_MODE:
 *	Unicast (0), one to many (1) or many to many (2, unused)
 * @FIRA_SESSION_PARAM_ATTR_SHORT_ADDR:
 *	Override device address for this session (UCI: DEVICE_MAC_ADDRESS)
 * @FIRA_SESSION_PARAM_ATTR_DESTINATION_SHORT_ADDRS:
 *	Destination short addresses (UCI: DST_MAC_ADDRESS)
 * @FIRA_SESSION_PARAM_ATTR_MAC_ADDRESS_MODE:
 *	MAC addressing mode: short address(0), extended address(2).
 * @FIRA_SESSION_PARAM_ATTR_TIME0_NS:
 *	Absolute value of the initiation time in nanoseconds (default 0 - N/A).
 * @FIRA_SESSION_PARAM_ATTR_SLOT_DURATION_RSTU:
 *	Duration of a slot in RSTU, default 2400. (2 ms)
 * @FIRA_SESSION_PARAM_ATTR_BLOCK_DURATION_MS:
 *	Block size in unit of 1200 RSTU (same as ms), default 200.
 * @FIRA_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS:
 *	Number of slots per ranging round, default 30 (UCI: SLOTS_PER_RR)
 * @FIRA_SESSION_PARAM_ATTR_SESSION_TIME_BASE:
 *	Binary (byte array) containing the reference time base information
 *	(controlee or DT-Tag only):
 *	Octet 0: time base configuration.
 *	Octet 1-4: Session Handle of the reference session.
 *	Octet 5-8: Session offset time in microseconds.
 * @FIRA_SESSION_PARAM_ATTR_BLOCK_STRIDE_LENGTH:
 *	Value of block striding, default 0, can be changed when the
 *	session is active [controller only]
 * @FIRA_SESSION_PARAM_ATTR_MAX_NUMBER_OF_MEASUREMENTS:
 *	Unlimited (0, default) or limit of measurements (1-65535)
 * @FIRA_SESSION_PARAM_ATTR_MAX_RR_RETRY:
 *	Number of failed ranging round attempts before stopping the session,
 *	or disabled (0, default) [controller only]
 * @FIRA_SESSION_PARAM_ATTR_ROUND_HOPPING:
 *	Disabled (0, default) or enabled (1)
 * @FIRA_SESSION_PARAM_ATTR_PRIORITY:
 *	Priority value, higher has more priority (1-100, default 50)
 * @FIRA_SESSION_PARAM_ATTR_RANGING_ROUND_CONTROL:
 *	b0: ranging result report phase is disabled (0) or enabled (1, default)
 *	    report phase [controller only]
 *	b1: Control Message is sent in band (1, default) or not (0, not supported)
 *	b2: Control Message is sent separately (0, default) or piggybacked
 *	    to ranging initiation message (1) [Only valid for non-deferred mode TWR]
 *	b3-b6: RFUs, must be set to 0.
 *	b7: Measurement report message (MRM) is sent from the initiator (0, default),
 *	    or from the responder (1, not supported) [controller only]
 * @FIRA_SESSION_PARAM_ATTR_SCHEDULE_MODE:
 *	Contention-based ranging (0), Time-scheduled ranging (1, default),
 *	Hybrid-based ranging (2, not supported)
 * @FIRA_SESSION_PARAM_ATTR_IN_BAND_TERMINATION_ATTEMPT_COUNT:
 *	1-10, default 1 [controller only]
 * @FIRA_SESSION_PARAM_ATTR_MIN_FRAMES_PER_RR:
 *	For OWR for AoA only, number of frames in RR (default 4).
 * @FIRA_SESSION_PARAM_ATTR_MTU_SIZE:
 *	Maximum Transfer Unit (MTU), max data size allowed to be transmitted in one frame.
 * @FIRA_SESSION_PARAM_ATTR_INTER_FRAME_INTERVAL:
 *	For OWR for AoA only, Interval between RFRAMES (in units of 1200 RSTU),
 *	(default 1)
 * @FIRA_SESSION_PARAM_ATTR_OWR_AOA_MEASUREMENT_NTF_PERIOD:
 *	For OWR for AoA only, configures period of OWR for AoA measurement
 *	notifications: after every frame (0, default), after MIN_FRAMES_PER_RR
 *	number measurements (1).
 * @FIRA_SESSION_PARAM_ATTR_CHANNEL_NUMBER:
 *	Channel for this session: 5 or 9 (default: 9)
 * @FIRA_SESSION_PARAM_ATTR_PREAMBLE_CODE_INDEX:
 *	Preamble code for this session, BPRF (9-24), HPRF (25-32) (default: 10)
 * @FIRA_SESSION_PARAM_ATTR_RFRAME_CONFIG:
 *	SP0 (0), SP1 (1) or SP3 (3, default)
 * @FIRA_SESSION_PARAM_ATTR_PRF_MODE:
 *	BPRF (0, default), HPRF (1) or HPRF with high data rate (2)
 * @FIRA_SESSION_PARAM_ATTR_PREAMBLE_DURATION:
 *	64 (1, default) or 32 (0, only for HPRF)
 * @FIRA_SESSION_PARAM_ATTR_SFD_ID:
 *	BPRF (0 or 2), HPRF (1-4), default 2
 * @FIRA_SESSION_PARAM_ATTR_NUMBER_OF_STS_SEGMENTS:
 *	0-2, default to 0 for SP0, default to 1 for SP1 & SP3, 2 not supported
 * @FIRA_SESSION_PARAM_ATTR_PSDU_DATA_RATE:
 *	6.81 Mbps (0, default), 7.80 Mbps (1), 27.2 Mbps (2), 31.2 Mbps (3)
 * @FIRA_SESSION_PARAM_ATTR_BPRF_PHR_DATA_RATE:
 *	850 kbps (0, default) or 6.81 Mbps (1)
 * @FIRA_SESSION_PARAM_ATTR_MAC_FCS_TYPE:
 *	CRC16 (0, default) or CRC32 (1, not supported)
 * @FIRA_SESSION_PARAM_ATTR_MEASUREMENT_SEQUENCE:
 *	Sequence of measurement steps. Configures antenna flexibility.
 * @FIRA_SESSION_PARAM_ATTR_STS_CONFIG:
 *	Static STS (0, default), Dynamic STS (1), Dynamic STS for controlee
 *	individual keys (2), Provisioned STS (3), Provisioned STS for controlee
 *	individual keys (4). See &enum fbs_sts_mode.
 * @FIRA_SESSION_PARAM_ATTR_SUB_SESSION_ID:
 *	For dynamic STS for controlee individual key, sub session ID [controlee only]
 * @FIRA_SESSION_PARAM_ATTR_VENDOR_ID:
 *	Vendor ID used to construct vUpper64 in static STS
 * @FIRA_SESSION_PARAM_ATTR_STATIC_STS_IV:
 *	Static STS IV used to construct vUpper64 in static STS
 * @FIRA_SESSION_PARAM_ATTR_SESSION_KEY:
 *	For provisioned sts only, session key.
 * @FIRA_SESSION_PARAM_ATTR_SUB_SESSION_KEY:
 *	For dynamic or provisioned STS, sub session key [controlee only]
 * @FIRA_SESSION_PARAM_ATTR_KEY_ROTATION:
 *	Disable (0, default) or enabled (1)
 * @FIRA_SESSION_PARAM_ATTR_KEY_ROTATION_RATE:
 *	Key rotation rate parameter n, key rotated after 2^n rounds
 * @FIRA_SESSION_PARAM_ATTR_MAC_PAYLOAD_ENCRYPTION:
 *	Enable or disable encryption of payload data.
 * @FIRA_SESSION_PARAM_ATTR_RESULT_REPORT_CONFIG:
 *	See &enum fira_result_report_config_flags
 * @FIRA_SESSION_PARAM_ATTR_REPORT_RSSI:
 *	Report average RSSI of the round in result message, disabled (0,
 *	default) or enabled (1)
 * @FIRA_SESSION_PARAM_ATTR_DIAGNOSTICS:
 *	Report diagnostic information on each round, disabled (0, default) or
 *	enabled (1)
 * @FIRA_SESSION_PARAM_ATTR_DIAGNOSTICS_FRAME_REPORTS_FIELDS:
 *	Bitfield activating various frame diagnostics in the report (0: no frame
 *	diagnostic report, default).
 *	see &enum fira_ranging_diagnostics_frame_report_flags
 * @FIRA_SESSION_PARAM_ATTR_STS_LENGTH:
 *	Number of symbols in a STS segment. 32 (0x00), 64 (0x01, default) or 128
 *	symbols (0x02)
 * @FIRA_SESSION_PARAM_ATTR_CAP_SIZE_MAX:
 *	Maximum for Contention Access Period Size,
 *	default: 29 (FIRA_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS - 1) (UCI: CAP_SIZE_RANGE[0])
 * @FIRA_SESSION_PARAM_ATTR_CAP_SIZE_MIN:
 *	Minimum for Contention Access Period Size, default 5 (UCI: CAP_SIZE_RANGE[1])
 * @FIRA_SESSION_PARAM_ATTR_SESSION_INFO_NTF_CONFIG:
 *	Configure session info notification
 * @FIRA_SESSION_PARAM_ATTR_NEAR_PROXIMITY_CONFIG_CM:
 *	Lower bound in mm above which the ranging notifications
 *	should be enabled when SESSION_INFO_NTF_CONFIG is set to "proximity" or
 *	"aoa_proximity"
 * @FIRA_SESSION_PARAM_ATTR_FAR_PROXIMITY_CONFIG_CM:
 *	Upper bound in mm above which the ranging notifications
 *	should be disabled when SESSION_INFO_NTF_CONFIG is set to "proximity" or
 *	"aoa_proximity"
 * @FIRA_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_AZIMUTH_2PI:
 *	Lower bound in rad_2pi for AOA azimuth above which the ranging
 *	notifications should automatically be enabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 32 bits (rad_2pi).
 *	Allowed values range from -180° to +180°. should be less than or equal to
 *	SESSION_INFO_NTF_UPPER_BOUND_AOA_AZIMUTH value. (default = -180)
 * @FIRA_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_AZIMUTH_2PI:
 *	Upper bound in rad_2pi for AOA azimuth above which the ranging
 *	notifications should automatically be disabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 32 bits (rad_2pi).
 *	Allowed values range from -180° to +180°. Should be greater than or equal to
 *	SESSION_INFO_NTF_LOWER_BOUND_AOA_AZIMUTH value. (default = 180)
 * @FIRA_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_ELEVATION_2PI:
 *	Lower bound in rad_2pi for AOA elevation above which the ranging
 *	notifications should automatically be enabled if SESSION_INFO_NTF_CONFIG is
 *	set to "aoa" or "aoa_proximity". It is a signed value on 16 bits (rad_2pi).
 *	Allowed values range from -90° to +90°. Should be less than or equal to
 *	SESSION_INFO_NTF_UPPER_BOUND_AOA_ELEVATION value. (default = -90)
 * @FIRA_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_ELEVATION_2PI:
 *	Upper bound in rad_2pi for AOA elevation above which the ranging
 *	notifications should automatically be disabled if SESSION_INFO_NTF_CONFIG has
 *	bit is set to "aoa" or "aoa_proximity". It is a signed value on 16 bits
 *	(rad_2pi). Allowed values range from -90° to +90°. Should be greater than or
 *	equal to SESSION_INFO_NTF_LOWER_BOUND_AOA_ELEVATION value. (default = +90)
 * @FIRA_SESSION_PARAM_ATTR_LINK_LAYER_MODE:
 *	Used to define link layer behavior. Possible values are 0x00 (link layer
 *	in bypass mode - default) or 0x01 (link layer in connection less mode).
 * @FIRA_SESSION_PARAM_ATTR_DATA_REPETITION_COUNT:
 *	Indicates the number of times the data shall be repeated in consecutive
 *	ranging rounds. Available only on OWR sessions. 0x00 means no repetition shall
 *	be performed at all and 0xff means the same MDSDU shall transmitted infinite
 *	number of times.
 * @FIRA_SESSION_PARAM_ATTR_DATA_TRANSFER_STATUS_NTF_CONFIG:
 *	Used to configure the SESSION_DATA_TRANSFER_STATUS_NTF.
 *	If disabled (0, default), then the UWBS shall not send
 *	SESSION_DATA_TRANSFER_STATUS_NTF for every Application Data transmission
 *	except for last transmission.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_IS_REFERENCE:
 *	Sets or resets the DT-Anchor to be the global time reference of a DL-TDoA network.
 * @FIRA_SESSION_PARAM_ATTR_DL_TDOA_RESPONDER_TOF:
 *	Includes (1) or excludes (0, default) the responder_tof field in a response DTM.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_RANGING_METHOD:
 *	Type of the ranging method used in message exchange between DT-Anchors.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_TYPE:
 *	Type of the TX timestamps (local time base vs common time base) included in DTMs.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_LEN:
 *	Length of the TX timestamps (40-bit vs 64-bit) included in DTMs.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_HOP_COUNT_PRESENT:
 *	True when Hop Count field is included in Poll DTMs, false otherwise.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_CFO_PRESENT:
 *	True when DT-Anchor CFO estimate is included in Response DTMs, false otherwise.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_PRESENT:
 *	True when the information about DT-Anchor location is included in DTMs, false otherwise.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_TYPE:
 *	Type of coordinate system of DT-Anchor (0: WGS84, 1: relative).
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION:
 *	Binary (byte array) that encodes coordinates of DT-Anchor location.
 * @FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_ACTIVE_RANGING_ROUNDS_INFO_PRESENT:
 *	True when the information about active ranging rounds for DT-Anchor is
 *	included in DTMs, false otherwise.
 * @FIRA_SESSION_PARAM_ATTR_DT_TAG_BLOCK_SKIPPING:
 *	Number of blocks to be skipped by the DT-Tag after the current ranging block.
 * @FIRA_SESSION_PARAM_ATTR_APPLICATION_DATA_ENDPOINT:
 *	Local endpoint configuration for the session,
 *	b3-b0: Non-secure endpoint configuration:
 *	   0x0: Host
 *	   0x1: Secure Component
 *	b7-b4: Secure endpoint configuration:
 *	   0x0: Host
 *	   0x1: Secure Component
 * @FIRA_SESSION_PARAM_ATTR_REPORT_PSDUS:
 *	Report psdu data on each round, disabled (0, default) or enabled (1)
 * @FIRA_SESSION_PARAM_ATTR_UT_TX_INTERVAL:
 *	Average transmission interval of Blink UTM or Synchronization UTMs.
 *	32-bit unsigned value, default: 2000ms.
 * @FIRA_SESSION_PARAM_ATTR_UT_RANDOM_WINDOW:
 *	Length of randomization window for Blink UTM or Synchronization UTM.
 *	32-bit unsigned value, default: 0 ms.
 * @FIRA_SESSION_PARAM_ATTR_UT_TX_TIMESTAMP_LEN:
 *	Presence and length of TX timestamps in UTM, not present (0, default),
 *	40-bit timestamp (1), 64-bit timestamp (2).
 * @FIRA_SESSION_PARAM_ATTR_UT_DEVICE_ID_LEN:
 *	Specify length of Device ID included in UTMs.
 *	Device ID not included (0, default), 16-bit Device ID (1),
 *	32-bit Device Id (2), 64-bit Device ID (3)
 * @FIRA_SESSION_PARAM_ATTR_UT_DEVICE_ID:
 *	Specify Device ID included in UTMs.
 * @FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_INTERVAL:
 *	Time interval between SESSION_INFO_NTFs (in ms).
 * @FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_COUNT:
 *	Number of received blink UTM count between SESSION_INFO_NTFs.
 * @FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_EVENT:
 *	Rx event that shall be reported as measurement result, only STATUS_OK (0, default), all RX
 *	events (1).
 *
 * @FIRA_SESSION_PARAM_ATTR_UNSPEC: Invalid command.
 * @__FIRA_SESSION_PARAM_ATTR_AFTER_LAST: Internal use.
 * @FIRA_SESSION_PARAM_ATTR_MAX: Internal use.
 */

enum fira_session_param_attrs {
	FIRA_SESSION_PARAM_ATTR_UNSPEC,
	/* Main session parameters. */
	FIRA_SESSION_PARAM_ATTR_DEVICE_TYPE,
	FIRA_SESSION_PARAM_ATTR_DEVICE_ROLE,
	FIRA_SESSION_PARAM_ATTR_RANGING_ROUND_USAGE,
	FIRA_SESSION_PARAM_ATTR_MULTI_NODE_MODE,
	FIRA_SESSION_PARAM_ATTR_SHORT_ADDR,
	FIRA_SESSION_PARAM_ATTR_DESTINATION_SHORT_ADDRS,
	FIRA_SESSION_PARAM_ATTR_MAC_ADDRESS_MODE,
	/* Timings. */
	FIRA_SESSION_PARAM_ATTR_TIME0_NS,
	FIRA_SESSION_PARAM_ATTR_SLOT_DURATION_RSTU,
	FIRA_SESSION_PARAM_ATTR_BLOCK_DURATION_MS,
	FIRA_SESSION_PARAM_ATTR_ROUND_DURATION_SLOTS,
	FIRA_SESSION_PARAM_ATTR_SESSION_TIME_BASE,
	FIRA_SESSION_PARAM_ATTR_BLOCK_STRIDE_LENGTH,
	/* Behaviour. */
	FIRA_SESSION_PARAM_ATTR_MAX_NUMBER_OF_MEASUREMENTS,
	FIRA_SESSION_PARAM_ATTR_MAX_RR_RETRY,
	FIRA_SESSION_PARAM_ATTR_ROUND_HOPPING,
	FIRA_SESSION_PARAM_ATTR_PRIORITY,
	FIRA_SESSION_PARAM_ATTR_RANGING_ROUND_CONTROL,
	FIRA_SESSION_PARAM_ATTR_SCHEDULE_MODE,
	FIRA_SESSION_PARAM_ATTR_IN_BAND_TERMINATION_ATTEMPT_COUNT,
	/* Behaviour OWR for AoA ranging. */
	FIRA_SESSION_PARAM_ATTR_MIN_FRAMES_PER_RR,
	FIRA_SESSION_PARAM_ATTR_MTU_SIZE,
	FIRA_SESSION_PARAM_ATTR_INTER_FRAME_INTERVAL,
	FIRA_SESSION_PARAM_ATTR_OWR_AOA_MEASUREMENT_NTF_PERIOD,
	/* Radio. */
	FIRA_SESSION_PARAM_ATTR_CHANNEL_NUMBER,
	FIRA_SESSION_PARAM_ATTR_PREAMBLE_CODE_INDEX,
	FIRA_SESSION_PARAM_ATTR_RFRAME_CONFIG,
	FIRA_SESSION_PARAM_ATTR_PRF_MODE,
	FIRA_SESSION_PARAM_ATTR_PREAMBLE_DURATION,
	FIRA_SESSION_PARAM_ATTR_SFD_ID,
	FIRA_SESSION_PARAM_ATTR_NUMBER_OF_STS_SEGMENTS,
	FIRA_SESSION_PARAM_ATTR_PSDU_DATA_RATE,
	FIRA_SESSION_PARAM_ATTR_BPRF_PHR_DATA_RATE,
	FIRA_SESSION_PARAM_ATTR_MAC_FCS_TYPE,
	/* Measurement Sequence. */
	FIRA_SESSION_PARAM_ATTR_MEASUREMENT_SEQUENCE,
	/* STS and crypto. */
	FIRA_SESSION_PARAM_ATTR_STS_CONFIG,
	FIRA_SESSION_PARAM_ATTR_SUB_SESSION_ID,
	FIRA_SESSION_PARAM_ATTR_VENDOR_ID,
	FIRA_SESSION_PARAM_ATTR_STATIC_STS_IV,
	FIRA_SESSION_PARAM_ATTR_SESSION_KEY,
	FIRA_SESSION_PARAM_ATTR_SUB_SESSION_KEY,
	FIRA_SESSION_PARAM_ATTR_KEY_ROTATION,
	FIRA_SESSION_PARAM_ATTR_KEY_ROTATION_RATE,
	/* Report. */
	FIRA_SESSION_PARAM_ATTR_RESULT_REPORT_CONFIG,
	FIRA_SESSION_PARAM_ATTR_REPORT_RSSI,
	/* Diagnostics. */
	FIRA_SESSION_PARAM_ATTR_DIAGNOSTICS,
	FIRA_SESSION_PARAM_ATTR_DIAGNOSTICS_FRAME_REPORTS_FIELDS,
	/* Misc */
	FIRA_SESSION_PARAM_ATTR_STS_LENGTH,
	/* Contention-based ranging. */
	FIRA_SESSION_PARAM_ATTR_CAP_SIZE_MAX,
	FIRA_SESSION_PARAM_ATTR_CAP_SIZE_MIN,
	/* Range data notification enable. */
	FIRA_SESSION_PARAM_ATTR_SESSION_INFO_NTF_CONFIG,
	FIRA_SESSION_PARAM_ATTR_NEAR_PROXIMITY_CONFIG_CM,
	FIRA_SESSION_PARAM_ATTR_FAR_PROXIMITY_CONFIG_CM,
	FIRA_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_AZIMUTH_2PI,
	FIRA_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_AZIMUTH_2PI,
	FIRA_SESSION_PARAM_ATTR_LOWER_AOA_BOUND_CONFIG_ELEVATION_2PI,
	FIRA_SESSION_PARAM_ATTR_UPPER_AOA_BOUND_CONFIG_ELEVATION_2PI,
	/* Data transfer. */
	FIRA_SESSION_PARAM_ATTR_LINK_LAYER_MODE,
	FIRA_SESSION_PARAM_ATTR_DATA_REPETITION_COUNT,
	FIRA_SESSION_PARAM_ATTR_DATA_TRANSFER_STATUS_NTF_CONFIG,
	/* Parameters specific for DL-TDoA. */
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_IS_REFERENCE,
	FIRA_SESSION_PARAM_ATTR_DL_TDOA_RESPONDER_TOF,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_RANGING_METHOD,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_TYPE,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_TX_TIMESTAMP_LEN,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_HOP_COUNT_PRESENT,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_CFO_PRESENT,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_PRESENT,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION_TYPE,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_LOCATION,
	FIRA_SESSION_PARAM_ATTR_DT_ANCHOR_ACTIVE_RANGING_ROUNDS_INFO_PRESENT,
	FIRA_SESSION_PARAM_ATTR_DT_TAG_BLOCK_SKIPPING,
	/* Not Used but defined */
	FIRA_SESSION_PARAM_ATTR_APPLICATION_DATA_ENDPOINT,
	/* Debug. */
	FIRA_SESSION_PARAM_ATTR_REPORT_PSDUS,
	/* Vendor specific parameter. */
	FIRA_SESSION_PARAM_ATTR_MAC_PAYLOAD_ENCRYPTION,
	/* Parameters specific for UL-TDoA. */
	FIRA_SESSION_PARAM_ATTR_UT_TX_INTERVAL,
	FIRA_SESSION_PARAM_ATTR_UT_RANDOM_WINDOW,
	FIRA_SESSION_PARAM_ATTR_UT_TX_TIMESTAMP_LEN,
	FIRA_SESSION_PARAM_ATTR_UT_DEVICE_ID_LEN,
	FIRA_SESSION_PARAM_ATTR_UT_DEVICE_ID,
	FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_INTERVAL,
	FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_COUNT,
	FIRA_SESSION_PARAM_ATTR_UT_REPORT_CONFIG_EVENT,
	__FIRA_SESSION_PARAM_ATTR_AFTER_LAST,
	FIRA_SESSION_PARAM_ATTR_MAX = __FIRA_SESSION_PARAM_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_call_controlee_attrs - FiRa controlee parameters attributes.
 *
 * @FIRA_CALL_CONTROLEE_ATTR_SHORT_ADDR:
 *	Controlee short address.
 * @FIRA_CALL_CONTROLEE_ATTR_SUB_SESSION_ID:
 *	Controlee sub session identifier
 * @FIRA_CALL_CONTROLEE_ATTR_SUB_SESSION_KEY:
 *	Controlee sub session key
 *
 * @FIRA_CALL_CONTROLEE_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CALL_CONTROLEE_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CALL_CONTROLEE_ATTR_MAX: Internal use.
 */
enum fira_call_controlee_attrs {
	FIRA_CALL_CONTROLEE_ATTR_UNSPEC,
	FIRA_CALL_CONTROLEE_ATTR_SHORT_ADDR,
	FIRA_CALL_CONTROLEE_ATTR_SUB_SESSION_ID,
	FIRA_CALL_CONTROLEE_ATTR_SUB_SESSION_KEY,

	__FIRA_CALL_CONTROLEE_ATTR_AFTER_LAST,
	FIRA_CALL_CONTROLEE_ATTR_MAX = __FIRA_CALL_CONTROLEE_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_call_dt_anchor_round_config_attrs - Attributes for ranging round
 * configuration for DT-Anchor.
 *
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_INDEX:
 *	Index of the ranging round.
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_ACTING_ROLE:
 *	DT-Anchor's acting role for this ranging round.
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SHORT_ADDR:
 *	Short address of DT-Anchor Responders (nested parameter) for
 *	this ranging round (applicable if DT-Anchor's acting role is Initiator).
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SLOT:
 *	Slot indexes for Response DTMs for DT-Anchor Responders (nested
 *	parameter) for this ranging round (applicable if DT-Anchor's acting role is
 *	Initiator).
 *
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_MAX: Internal use.
 */
enum fira_call_dt_anchor_round_config_attrs {
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_UNSPEC,
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_INDEX,
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_ACTING_ROLE,
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SHORT_ADDR,
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_RESPONDER_SLOT,

	__FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST,
	FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_MAX =
		__FIRA_CALL_DT_ANCHOR_ROUND_CONFIG_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_ranging_data_attrs_ranging_measurement_type - Values for
 * FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE attribute.
 *
 * @FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_UL_TDOA:
 *	UL-TDoA Ranging Measurement (OWR UL-TDoA).
 * @FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_TWR:
 *	Two Way Ranging Measurement (SS-TWR, DS-TWR).
 * @FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_DL_TDOA:
 *	DL-TDoA Ranging Measurement (OWR DL-TDoA).
 * @FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_OWR_AOA:
 *	OWR for AoA Measurement.
 */
enum fira_ranging_data_attrs_ranging_measurement_type {
	FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_UL_TDOA,
	FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_TWR,
	FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_DL_TDOA,
	FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE_OWR_AOA,
};

/**
 * enum fira_ranging_data_attrs - FiRa ranging data parameter attributes.
 *
 * @FIRA_RANGING_DATA_ATTR_BLOCK_INDEX:
 *	Current block index.
 * @FIRA_RANGING_DATA_ATTR_TIMESTAMP_NS:
 *	Timestamp in nanoseconds in the CLOCK_MONOTONIC time reference.
 * @FIRA_RANGING_DATA_ATTR_RANGING_INTERVAL_MS:
 *	Current ranging interval (block size * (stride + 1)) in unit of
 *	1200 RSTU (same as ms).
 * @FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE:
 *	Type of ranging measurement
 * @FIRA_RANGING_DATA_ATTR_MEASUREMENTS:
 *	Measurements, see fira_ranging_data_measurements.
 *
 * @FIRA_RANGING_DATA_ATTR_UNSPEC: Invalid command.
 * @__FIRA_RANGING_DATA_ATTR_AFTER_LAST: Internal use.
 * @FIRA_RANGING_DATA_ATTR_MAX: Internal use.
 */
enum fira_ranging_data_attrs {
	FIRA_RANGING_DATA_ATTR_UNSPEC,
	FIRA_RANGING_DATA_ATTR_BLOCK_INDEX,
	FIRA_RANGING_DATA_ATTR_TIMESTAMP_NS,
	FIRA_RANGING_DATA_ATTR_RANGING_INTERVAL_MS,
	FIRA_RANGING_DATA_ATTR_RANGING_MEASUREMENT_TYPE,
	FIRA_RANGING_DATA_ATTR_MEASUREMENTS,

	__FIRA_RANGING_DATA_ATTR_AFTER_LAST,
	FIRA_RANGING_DATA_ATTR_MAX = __FIRA_RANGING_DATA_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_ranging_data_measurements_attrs - FiRa ranging data measurements
 * attributes.
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_SHORT_ADDR:
 *	Address of the participing device.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_STOPPED:
 *	If present, ranging was stopped as requested [controller only].
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_STATUS:
 *	Status of the measurement (0: success, 1: tx failed, 2: rx timeout,
 *	3: rx phy dec error, 4: rx phy toa error, 5: rx phy sts error,
 *	6: rx mac dec error, 7: rx mac ie dec error, 8: rx mac ie missing)
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_SLOT_INDEX:
 *	In case of failure slot index where the error was detected.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_NLOS:
 *	Indicates if the ranging measurement was in Line of Sight (LoS) or Non-Line of Sight (NLoS):
 *	0x00 = LoS, 0x01 = NLoS, 0xFF = Unable to determine.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_FRAME_SEQUENCE_NUMBER:
 *	Sequence number as received in MHR.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_BLOCK_INDEX:
 *	Block Index number as received in the OWR message from the Advertiser
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_DISTANCE_CM:
 *	Distance in cm.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA:
 *	Local AoA measurement,
 *	cf. fira_ranging_data_measurements_aoa_attrs.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_AZIMUTH:
 *	Local AoA measurement for azimuth,
 *	cf. fira_ranging_data_measurements_aoa_attrs.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_ELEVATION:
 *	Local AoA measurement for elevation,
 *	cf. fira_ranging_data_measurements_aoa_attrs.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_2PI:
 *	Estimation of reception angle in the azimuth of the participing device.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_PI:
 *	Estimation of reception angle in elevation of the participing device.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_FOM_100:
 *	Estimation of azimuth reliability of the participing device.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_FOM_100:
 *	Estimation of elevation reliability of the participing device.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_RSSI:
 *	RSSI for received frames during the ranging round,
 *	reported as Q7.1.
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_UNSPEC: Invalid command.
 * @__FIRA_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST: Internal use.
 * @FIRA_RANGING_DATA_MEASUREMENTS_ATTR_MAX: Internal use.
 */
enum fira_ranging_data_measurements_attrs {
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_UNSPEC,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_SHORT_ADDR,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_STOPPED,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_STATUS,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_SLOT_INDEX,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_NLOS,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_FRAME_SEQUENCE_NUMBER,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_BLOCK_INDEX,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_DISTANCE_CM,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_AZIMUTH,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_LOCAL_AOA_ELEVATION,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_2PI,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_PI,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_AZIMUTH_FOM_100,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_REMOTE_AOA_ELEVATION_FOM_100,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_RSSI,

	__FIRA_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST,
	FIRA_RANGING_DATA_MEASUREMENTS_ATTR_MAX =
		__FIRA_RANGING_DATA_MEASUREMENTS_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_ranging_data_measurements_aoa_attrs - FiRa ranging AoA measurements
 * attributes.
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_RX_ANTENNA_SET:
 *	Antenna set index.
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_2PI:
 *	Estimation of reception angle.
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_FOM_100:
 *	Estimation of local AoA reliability.
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_PDOA_2PI:
 *	Estimation of reception phase difference.
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_UNSPEC: Invalid command.
 * @__FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST: Internal use.
 * @FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_MAX: Internal use.
 */
enum fira_ranging_data_measurements_aoa_attrs {
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_UNSPEC,
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_RX_ANTENNA_SET,
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_2PI,
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AOA_FOM_100,
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_PDOA_2PI,

	__FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST,
	FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_MAX =
		__FIRA_RANGING_DATA_MEASUREMENTS_AOA_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_ranging_data_measurements_dt_attrs - FiRa ranging DL-TDoA
 * measurement attributes (per each message received, used only for DT-Tag).
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_SHORT_ADDR:
 *	Short MAC address of the device sending the message.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_STATUS:
 *	Status of the message reception (see &enum quwbs_fbs_status).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_MESSAGE_TYPE:
 *	Type of the message received.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_TYPE:
 *	Type of the TX timestamp (local time base vs common time base) included
 *	in the received message.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_LEN:
 *	Length of the TX timestamp (40-bit vs 64-bit) included
 *	in the received message.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_LEN:
 *	Length of the RX timestamp calculated during the reception
 *	of the received message.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_PRESENT:
 *	True when the information about DT-Anchor location is included
 *	in the measurement, false otherwise.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_TYPE:
 *	Type of the coordinate system of DT-Anchor location (0: WGS84, 1:
 *	relative) (if included).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES_LEN:
 *	Number of active ranging round indexes included in the measurement.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_BLOCK_INDEX:
 *	Index of the current ranging block.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ROUND_INDEX:
 *	Index of the current ranging round.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_NLOS:
 *	Indicates if the reception of the message was in Line of Sight or not.
 *	Value 0xff means that it was unable to be determined (or not supported
 *	at all).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_2PI:
 *	AoA Azimuth in degrees measured by the DT-Tag during the reception
 *	(encoded as Q9.7).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_FOM_100:
 *	Reliability of the estimated AoA Azimuth measured by the DT-Tag
 *	during the reception (range: 0-100).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_2PI:
 *	AoA Elevation in degrees measured by the DT-Tag during the reception
 *	(encoded as Q9.7).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_FOM_100:
 *	Reliability of the estimated AoA Elevation measured by the DT-Tag
 *	during the reception (range: 0-100).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RSSI:
 *	RSSI measured by the DT-Tag during the reception (encoded as Q7.1).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_RCTU:
 *	TX timestamp included in the received message (unit: RCTU).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_RCTU:
 *	RX timestamp calculated during the reception of the received message
 *	(unit: RCTU).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_REMOTE_CFO:
 *	Clock frequency offset of a Responder DT-Anchor with respect to the
 *	Initiator DT-Anchor of the ranging round as included in the received
 *	message (encoded as Q6.10).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_LOCAL_CFO:
 *	Clock frequency offset measured locally with respect to the DT-Anchor
 *	that sent the message received (encoded as Q6.10).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_REPLY_TIME_RCTU:
 *	Reply time of the Initiator DT-Anchor measured between the reception of
 *	Response DTM and the transmission of Final DTM (unit: RCTU,
 *	used only in DS-TWR).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RESPONDER_REPLY_TIME_RCTU:
 *	Reply time of the Responder DT-Anchor measured between the reception of
 *	Poll DTM and the transmission of Response DTM (unit: RCTU).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_RESPONDER_TOF_RCTU:
 *	Time of Flight measured between the Initiator DT-Anchor and the
 *	Responder DT-Anchor (for SS-TWR it's calculated by Initiator DT-Anchor
 *	and included in Poll DTM and for DS-TWR it's calculated by Responder
 *	DT-Anchor and included in Response DTM, unit: RCTU).
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION:
 *	Location coordinates of DT-Anchor that sent the message received.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES:
 *	List of active ranging round indexes in which the DT-Anchor that sent
 *	the message received participates.
 *
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_UNSPEC: Invalid command.
 * @__FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST: Internal use.
 * @FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_MAX: Internal use.
 */
enum fira_ranging_data_measurements_dt_attrs {
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_UNSPEC,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_SHORT_ADDR,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_STATUS,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_MESSAGE_TYPE,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_TYPE,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_LEN,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_LEN,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_PRESENT,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION_TYPE,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES_LEN,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_BLOCK_INDEX,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ROUND_INDEX,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_NLOS,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_2PI,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_AZIMUTH_FOM_100,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_2PI,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AOA_ELEVATION_FOM_100,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RSSI,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_TX_TIMESTAMP_RCTU,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RX_TIMESTAMP_RCTU,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_REMOTE_CFO,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_LOCAL_CFO,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_REPLY_TIME_RCTU,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_RESPONDER_REPLY_TIME_RCTU,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_INITIATOR_RESPONDER_TOF_RCTU,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ANCHOR_LOCATION,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_ACTIVE_RANGING_ROUND_INDEXES,

	__FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST,
	FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_MAX =
		__FIRA_RANGING_DATA_MEASUREMENTS_DT_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_session_param_meas_seq_step_attrs - FiRa measurement sequence
 * step attributes.
 *
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MEASUREMENT_TYPE:
 *	The type of measurement to perform during the step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_N_MEASUREMENTS:
 *	The number of times this type of measurement shall be performed
 *	during the step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SET_NONRANGING:
 *	The antenna set to use to receive the non-rfames during the step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SETS_RANGING:
 *	The antenna set to use to receive the rframes frame during the step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_NONRANGING:
 *	The antenna set to use to transmit the non-rframes during the step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_RANGING:
 *	The antenna set to use to transmit the rframes during the step.
 *
 *
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_UNSPEC: Invalid command.
 * @__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST: Internal use.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MAX: Internal use.
 */
enum fira_session_param_meas_seq_step_attrs {
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_UNSPEC,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MEASUREMENT_TYPE,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_N_MEASUREMENTS,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SET_NONRANGING,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_RX_ANT_SETS_RANGING,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_NONRANGING,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_TX_ANT_SET_RANGING,

	__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_MAX =
		__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_session_params_meas_seq_step_sets_attrs - Attributes of the
 * FiRa RX antenna sets to use during a step.
 *
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_0:
 *	Antenna set used to receive all rframes for range, azimuth and elevation
 *	steps or initial rframe for azimuth_elevation step.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_1:
 *	Antenna set used to receive final rframes for azimuth_elevation step.
 *
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_UNSPEC:
 *	Invalid command.
 * @FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_MAX:
 *	Internal use.
 * @__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST:
 *	Internal use.
 */
enum fira_session_params_meas_seq_step_sets_attrs {
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_UNSPEC,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_0,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_1,

	__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST,
	FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_MAX =
		__FIRA_SESSION_PARAM_MEAS_SEQ_STEP_RX_ANT_SETS_RANGING_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_ranging_diagnostics_attrs - FiRa ranging diagnostic attributes.
 *
 * @FIRA_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS_NB:
 *	Number of diagnostic reports.
 * @FIRA_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS:
 *	Pointer to diagnostic reports.
 *
 * @FIRA_RANGING_DIAGNOSTICS_ATTR_UNSPEC: Invalid command.
 * @__FIRA_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST : Internal use.
 * @FIRA_RANGING_DIAGNOSTICS_ATTR_MAX : Internal use.
 */
enum fira_ranging_diagnostics_attrs {
	FIRA_RANGING_DIAGNOSTICS_ATTR_UNSPEC,
	FIRA_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS_NB,
	FIRA_RANGING_DIAGNOSTICS_ATTR_FRAMES_REPORTS,

	__FIRA_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST,
	FIRA_RANGING_DIAGNOSTICS_ATTR_MAX = __FIRA_RANGING_DIAGNOSTICS_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_data_message_attrs - FiRa data_message parameter attributes.
 *
 * @FIRA_DATA_MESSAGE_ATTR_SESSION_HANDLE:
 *	Session handle.
 * @FIRA_DATA_MESSAGE_ATTR_MAC_ADDRESS:
 *	MAC short address, depending on the data message (SND or RCV) it can be
 *	sender or receiver address.
 * @FIRA_DATA_MESSAGE_ATTR_UCI_SEQ_NR:
 *	Sequence number from UCI Data Message.
 * @FIRA_DATA_MESSAGE_ATTR_STATUS:
 *	Status of the received message.
 * @FIRA_DATA_MESSAGE_ATTR_SEGMENT_INFO:
 *	Information about segment of Application Data Payload (first, last).
 * @FIRA_DATA_MESSAGE_ATTR_APP_DATA_LEN:
 *	Application Data length.
 * @FIRA_DATA_MESSAGE_ATTR_APP_DATA:
 *	Application Data.
 *
 * @FIRA_DATA_MESSAGE_ATTR_UNSPEC: Invalid command.
 * @__FIRA_DATA_MESSAGE_ATTR_AFTER_LAST: Internal use.
 * @FIRA_DATA_MESSAGE_ATTR_MAX: Internal use.
 */
enum fira_data_message_attrs {
	FIRA_DATA_MESSAGE_ATTR_UNSPEC,
	FIRA_DATA_MESSAGE_ATTR_SESSION_HANDLE,
	FIRA_DATA_MESSAGE_ATTR_MAC_ADDRESS,
	FIRA_DATA_MESSAGE_ATTR_UCI_SEQ_NR,
	FIRA_DATA_MESSAGE_ATTR_STATUS,
	FIRA_DATA_MESSAGE_ATTR_SEGMENT_INFO,
	FIRA_DATA_MESSAGE_ATTR_APP_DATA_LEN,
	FIRA_DATA_MESSAGE_ATTR_APP_DATA,

	__FIRA_DATA_MESSAGE_ATTR_AFTER_LAST,
	FIRA_DATA_MESSAGE_ATTR_MAX = __FIRA_DATA_MESSAGE_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_credit_attrs - FiRa credit parameter attributes.
 *
 *  @FIRA_CREDIT_ATTR_AVAILABLE:
 *	 Credit availability.
 *
 * @FIRA_CREDIT_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CREDIT_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CREDIT_ATTR_MAX: Internal use.
 */
enum fira_credit_attrs {
	FIRA_CREDIT_ATTR_UNSPEC,
	FIRA_CREDIT_ATTR_AVAILABLE,

	__FIRA_CREDIT_ATTR_AFTER_LAST,
	FIRA_CREDIT_ATTR_MAX = __FIRA_CREDIT_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_data_transfer_status_attrs - FiRa data_transfer_status parameter
 * attributes.
 *
 * @FIRA_DATA_TRANSFER_STATUS_ATTR_UCI_SEQ_NR:
 *	The Sequence Number identifying the UCI Data Message.
 * @FIRA_DATA_TRANSFER_STATUS_ATTR_CODE:
 *	Status code.
 * @FIRA_DATA_TRANSFER_STATUS_ATTR_TX_COUNT:
 *	Indicates the number of times Application Data with the same
 *	UCI Sequence Number has been transmitted.
 *
 * @FIRA_DATA_TRANSFER_STATUS_ATTR_UNSPEC: Invalid command.
 * @__FIRA_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST: Internal use.
 * @FIRA_DATA_TRANSFER_STATUS_ATTR_MAX: Internal use.
 */
enum fira_data_transfer_status_attrs {
	FIRA_DATA_TRANSFER_STATUS_ATTR_UNSPEC,
	FIRA_DATA_TRANSFER_STATUS_ATTR_UCI_SEQ_NR,
	FIRA_DATA_TRANSFER_STATUS_ATTR_CODE,
	FIRA_DATA_TRANSFER_STATUS_ATTR_TX_COUNT,

	__FIRA_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST,
	FIRA_DATA_TRANSFER_STATUS_ATTR_MAX = __FIRA_DATA_TRANSFER_STATUS_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_session_status_attrs - FiRa session status attributes.
 *
 * @FIRA_SESSION_STATUS_ATTR_STATE:
 *	Session state see &enum quwbs_fbs_session_state.
 * @FIRA_SESSION_STATUS_ATTR_REASON_CODE:
 *	Reason of the state change see &enum quwbs_fbs_reason_code.
 *
 * @FIRA_SESSION_STATUS_ATTR_UNSPEC: Invalid command.
 * @__FIRA_SESSION_STATUS_ATTR_AFTER_LAST: Internal use.
 * @FIRA_SESSION_STATUS_ATTR_MAX: Internal use.
 */
enum fira_session_status_attrs {
	FIRA_SESSION_STATUS_ATTR_UNSPEC,
	FIRA_SESSION_STATUS_ATTR_STATE,
	FIRA_SESSION_STATUS_ATTR_REASON_CODE,

	__FIRA_SESSION_STATUS_ATTR_AFTER_LAST,
	FIRA_SESSION_STATUS_ATTR_MAX = __FIRA_SESSION_STATUS_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_controlee_status_list_attrs - FiRa controlee status list attributes.
 *
 * @FIRA_CONTROLEE_STATUS_LIST_ATTR_SHORT_ADDR:
 *	Short MAC address of the device added/deleted from the multicast list.
 * @FIRA_CONTROLEE_STATUS_LIST_ATTR_SUB_SESSION_ID:
 *	Sub-session Id of the device added/deleted from the multicast list.
 * @FIRA_CONTROLEE_STATUS_LIST_ATTR_STATUS:
 *	Multicast update status code, see &enum fira_multicast_update_status.
 *
 * @FIRA_CONTROLEE_STATUS_LIST_ATTR_UNSPEC: Invalid command.
 * @__FIRA_CONTROLEE_STATUS_LIST_ATTR_AFTER_LAST: Internal use.
 * @FIRA_CONTROLEE_STATUS_LIST_ATTR_MAX: Internal use.
 */
enum fira_controlee_status_list_attrs {
	FIRA_CONTROLEE_STATUS_LIST_ATTR_UNSPEC,
	FIRA_CONTROLEE_STATUS_LIST_ATTR_SHORT_ADDR,
	FIRA_CONTROLEE_STATUS_LIST_ATTR_SUB_SESSION_ID,
	FIRA_CONTROLEE_STATUS_LIST_ATTR_STATUS,

	__FIRA_CONTROLEE_STATUS_LIST_ATTR_AFTER_LAST,
	FIRA_CONTROLEE_STATUS_LIST_ATTR_MAX = __FIRA_CONTROLEE_STATUS_LIST_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_hus_controller_phase_list_attrs - FiRa HUS controller phase list
 * attributes.
 *
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_SESSION_ID:
 *	Session ID of the phase.
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_START_SLOT_INDEX:
 *	Slot index of the first slot of this phase.
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_END_SLOT_INDEX:
 *	Slot index of the last slot of this phase.
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_CONTROL:
 *	MAC addressing mode and phase type. Currently not supported.
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_MAC_ADDRESS:
 *	Mac address of the controller of this phase.
 *
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_UNSPEC: Invalid command.
 * @__FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_AFTER_LAST: Internal use.
 * @FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_MAX: Internal use.
 */
enum fira_hus_controller_phase_list_attrs {
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_UNSPEC,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_SESSION_ID,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_START_SLOT_INDEX,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_END_SLOT_INDEX,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_CONTROL,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_MAC_ADDRESS,

	__FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_AFTER_LAST,
	FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_MAX =
		__FIRA_HUS_CONTROLLER_PHASE_LIST_ATTR_AFTER_LAST - 1
};

/**
 * enum fira_hus_controlee_phase_list_attrs - FiRa HUS controlee phase list
 * attributes.
 *
 * @FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_SESSION_HANDLE:
 *	Session handle of the phase.
 *
 * @FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_UNSPEC: Invalid command.
 * @__FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_AFTER_LAST: Internal use.
 * @FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_MAX: Internal use.
 */
enum fira_hus_controlee_phase_list_attrs {
	FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_UNSPEC,
	FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_SESSION_HANDLE,

	__FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_AFTER_LAST,
	FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_MAX = __FIRA_HUS_CONTROLEE_PHASE_LIST_ATTR_AFTER_LAST - 1
};

#ifdef __cplusplus
}
#endif
