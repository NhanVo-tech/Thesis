/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "net/fbs_region_params.h"
#include "quwbs/fbs/defs.h"

#include <net/fira_region_params.h>
#include <qerr.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uwbmac/fbs_helper.h>

struct uwbmac_context;

#ifdef __cplusplus
extern "C" {
#endif

/* Compatibility name for Polaris repository. */
#define fira_session_status_ntf_content fbs_helper_session_status_ntf

/*
 * struct fira_capabilities - FiRa capabilities.
 *
 * This structure contains the FiRa capabilities as reported by the MAC.
 */
struct fira_capabilities {
#define F(name) bool has_##name

#define P(name, type) \
	F(name);      \
	type name

	bool has_fira_mac_version_range;
	uint8_t fira_mac_version_range[FIRA_VERSION_RANGE_SIZE];
	bool has_fira_phy_version_range;
	uint8_t fira_phy_version_range[FIRA_VERSION_RANGE_SIZE];
	P(device_class, uint8_t);
	F(device_type_controlee_responder);
	F(device_type_controlee_initiator);
	F(device_type_controller_responder);
	F(device_type_controller_initiator);
	F(device_type_controller);
	F(device_type_controlee);
	F(device_role_responder);
	F(device_role_initiator);
	F(device_role_ut_sync_anchor);
	F(device_role_ut_anchor);
	F(device_role_ut_tag);
	F(device_role_advertiser);
	F(device_role_observer);
	F(device_role_dt_anchor);
	F(device_role_dt_tag);
	F(ranging_time_struct_block_based_scheduling);
	F(multi_node_mode_unicast);
	F(multi_node_mode_one_to_many);
	F(multi_node_mode_many_to_many);
	F(ranging_round_usage_ss_twr);
	F(ranging_round_usage_ds_twr);
	F(ranging_method_ss_twr_deferred);
	F(ranging_method_ss_twr_non_deferred);
	F(ranging_method_ds_twr_deferred);
	F(ranging_method_ds_twr_non_deferred);
	F(ranging_method_owr_ul_tdoa);
	F(ranging_method_owr_dl_tdoa);
	F(ranging_method_owr_aoa);
	F(ranging_method_ess_twr_non_deferred_for_cbr);
	P(number_of_controlees_max, uint32_t);
	/* Behaviour. */
	F(schedule_mode_contention_based);
	F(schedule_mode_time_scheduled);
	F(schedule_mode_hybrid);
	F(round_hopping);
	F(block_striding);
	F(uwb_initiation_time);
	F(extended_mac_address);
	F(suspend_ranging);
	/* Radio. */
	P(channel_number, uint16_t);
	F(rframe_config_sp0);
	F(rframe_config_sp1);
	F(rframe_config_sp3);
	F(convolutional_encoding_systematic);
	F(convolutional_encoding_non_systematic);
	F(prf_mode_bprf);
	F(prf_mode_hprf);
	F(preamble_duration_64);
	F(preamble_duration_32);
	F(sfd_id_0);
	F(sfd_id_1);
	F(sfd_id_2);
	F(sfd_id_3);
	F(sfd_id_4);
	F(number_of_sts_segments_0);
	F(number_of_sts_segments_1);
	F(number_of_sts_segments_2);
	F(number_of_sts_segments_3);
	F(number_of_sts_segments_4);
	F(psdu_data_rate_6m81);
	F(psdu_data_rate_7m80);
	F(psdu_data_rate_27m2);
	F(psdu_data_rate_31m2);
	F(bprf_phr_data_rate_850k);
	F(bprf_phr_data_rate_6m81);
	F(mac_fcs_type_crc32);
	/* Antenna. */
	P(rx_antenna_pairs, uint32_t);
	P(tx_antennas, uint32_t);
	/* STS and crypto capabilities. */
	F(sts_static);
	F(sts_dynamic);
	F(sts_dynamic_individual_key);
	F(sts_provisioned);
	F(sts_provisioned_individual_key);
	P(session_dynamic_key_lengths, uint8_t);
	P(session_provisioned_key_lengths, uint8_t);
	F(ppdu_format_sp0);
	F(ppdu_format_sp1);
	F(ppdu_format_sp3);
	F(sts_segment_length_32);
	F(sts_segment_length_64);
	F(sts_segment_length_128);
	/* Report. */
	F(aoa_azimuth);
	F(aoa_azimuth_full);
	F(aoa_elevation);
	F(aoa_fom);
	/* Specific for DL-TDoA. */
	P(dt_anchor_max_active_rr, uint8_t);
	P(dt_tag_max_active_rr, uint8_t);
	F(dt_tag_block_skipping);

#undef P
#undef F
};

/**
 * struct measurement_sequence - Fira measurement sequence.
 *
 * This structure contains the measurement sequence executed by the region.
 */
struct measurement_sequence {
	/** @n_steps: Number of steps in the schedule. */
	size_t n_steps;
	/** @steps: Steps of the schedule. */
	struct fira_measurement_sequence_step steps[FIRA_MEASUREMENT_SEQUENCE_STEP_MAX];
};

/**
 * struct session_parameters - Fira session parameters.
 *
 * This structure contains the session parameters sent to the Fira region.
 * Current implementation does not use all the parameters defined below.
 */
struct session_parameters {
	/**
	 * @device_type:
	 * Type of the device.
	 *
	 * Possible values:
	 *
	 * - 0x00: Controlee.
	 * - 0x01: Controller.
	 *
	 * See &enum quwbs_fbs_device_type.
	 */
	uint8_t device_type;
	/**
	 * @device_role:
	 * Role played by the device.
	 *
	 * Current implementation does not support decorrelation between the
	 * device's role and the device's type. The controller can only behave
	 * as the initiator and the controlee can only behave as responder.
	 *
	 * Possible values:
	 *
	 * - 0x00: Responder.
	 * - 0x01: Initiator.
	 * - 0x02: UT-Synchronization Anchor. |NSQM33|
	 * - 0x03: UT-Anchor. |NSQM33|
	 * - 0x04: UT-Tag. |NSQM33|
	 * - 0x05: Advertiser. |NSQM33|
	 * - 0x06: Observer. |NSQM33|
	 * - 0x07: DT-Anchor. |NSQM33|
	 * - 0x08: DT-Tag. |NSQM33|
	 *
	 * See &enum quwbs_fbs_device_role.
	 */
	uint8_t device_role;
	/**
	 * @ranging_round_usage:
	 * The ranging mode used during a round.
	 *
	 * Possible values:
	 *
	 * - 0x00: OWR UL-TDoA. |NSQM33|
	 * - 0x01: SS-TWR with Deferred Mode.
	 * - 0x02: DS-TWR with Deferred Mode.
	 * - 0x03: SS-TWR with Non-deferred Mode.
	 * - 0x04: DS-TWR with Non-deferred Mode.
	 * - 0x05: OWR DL-TDoA. |NSQM33|
	 * - 0x06: OWR for AoA. |NSQM33|
	 * - 0x07: eSS-TWR with Non-deferred Mode for Contention-based ranging. |NSQM33|
	 * - 0x08: aDS-TWR with Non-deferred Mode for Contention-based ranging. |NSQM33|
	 *
	 * See &enum fira_ranging_round_usage.
	 */
	uint8_t ranging_round_usage;
	/**
	 * @sts_config: It configures how system shall generate the STS.
	 *
	 * Possible values:
	 *
	 * - 0x00: Static STS (default).
	 * - 0x01: Dynamic STS. |NSQM33|
	 * - 0x02: Dynamic STS - Responder Specific Sub-session Key. |NSQM33|
	 * - 0x03: Provisioned STS.
	 * - 0x04: Provisioned STS - Responder Specific Sub-session Key.
	 *
	 * See &enum fbs_sts_mode.
	 */
	uint8_t sts_config;
	/**
	 * @multi_node_mode:
	 * The multi-node mode used during a round.
	 *
	 * Possible values:
	 *
	 * - 0x00: One-to-One.
	 * - 0x01: One-to-Many.
	 *
	 * See enum &fira_multi_node_mode.
	 */
	uint8_t multi_node_mode;
	/**
	 * @short_addr: Short address of the local device.
	 */
	uint16_t short_addr;
	/**
	 * @destination_short_address: Array of destination short addresses.
	 */
	uint16_t destination_short_address[FIRA_RESPONDERS_MAX];
	/**
	 * @n_destination_short_address: Number of destination short addresses.
	 */
	int n_destination_short_address;
	/**
	 * @time0_ns: Absolute value of the initiation time in nanoseconds.
	 */
	uint64_t time0_ns;
	/**
	 * @slot_duration_rstu: Duration of a slot in RSTU (1200RSTU=1ms).
	 */
	uint32_t slot_duration_rstu;
	/**
	 * @round_duration_slots: Number of slots per ranging round.
	 */
	uint32_t round_duration_slots;
	/**
	 * @block_duration_ms: Block size in unit of 1200 RSTU (same as ms).
	 */
	uint32_t block_duration_ms;
	/**
	 * @block_stride_length: Number of blocks to stride.
	 */
	uint32_t block_stride_length;
	/**
	 * @round_hopping: Enable FiRa round hopping.
	 */
	bool round_hopping;
	/**
	 * @priority: Priority of the session.
	 */
	uint8_t priority;
	/**
	 * @mac_address_mode: MAC addressing mode.
	 */
	uint8_t mac_address_mode;
	/**
	 * @ranging_round_control: Bit map of the following.
	 *
	 * - b0: ranging result report phase is disabled(0) or enabled(1).
	 * - b1: Control Message is sent in band(1) or not (0, not supported).
	 * - b2: Control Message is sent separately(0) or piggybacked to RIM(1).
	 */
	uint8_t ranging_round_control;
	/**
	 * @schedule_mode: Scheduling mode for the ranging session.
	 *
	 * Possible values:
	 *
	 * - 0x00 - Contention-based ranging. |NSQM33|
	 * - 0x01 - Time-scheduled ranging.
	 * - 0x02 - Hybrid-based ranging. |NSQM33|
	 */
	uint8_t schedule_mode;
	/**
	 * @max_number_of_measurements: Max number of measurements
	 */
	uint16_t max_number_of_measurements;
	/**
	 * @max_rr_retry:
	 * Number of failed ranging round attempts before stopping the session.
	 *
	 * The value zero disable the feature.
	 */
	uint32_t max_rr_retry;
	/**
	 * @channel_number: UWB channel for this session.
	 */
	uint8_t channel_number;
	/**
	 * @preamble_code_index:
	 * UWB preamble code index.
	 *
	 * Possible values:
	 *
	 * - 9-24: BPRF
	 * - 25-32: HPRF |NSQM33|
	 */
	uint8_t preamble_code_index;
	/**
	 * @rframe_config:
	 * The configuration of the frame.
	 *
	 * see enum &fira_rframe_config.
	 */
	uint8_t rframe_config;
	/**
	 * @preamble_duration:
	 *
	 * Possible values:
	 *
	 * - 0x00: 32 symbols |NSQM33|
	 * - 0x01: 64 symbols (default)
	 *
	 * See &enum fira_preamble_duration.
	 */
	uint8_t preamble_duration;
	/**
	 * @sfd_id:
	 *
	 * Possible values:
	 *
	 * - 0 or 2 in BPRF
	 * - 1-4 in HPRF |NSQM33|
	 *
	 * See &enum fira_sfd_id.
	 */
	uint8_t sfd_id;
	/**
	 * @psdu_data_rate:
	 *
	 * Possible values:
	 *
	 * - 0: 6.81Mbps (default)
	 * - 1: 7.80 Mbps |NSQM33|
	 * - 2: 27.2 Mbps |NSQM33|
	 * - 3: 31.2 Mbps |NSQM33|
	 *
	 * See &enum fira_psdu_data_rate.
	 */
	uint8_t psdu_data_rate;
	/**
	 * @phr_data_rate:
	 *
	 * Possible values:
	 *
	 * - 0: 850 kbit/s.
	 * - 1: 6.81 Mbit/s.
	 *
	 * See &enum fira_phr_data_rate.
	 */
	uint8_t phr_data_rate;
	union {
		struct {
			/**
			 * @static_sts_iv: Static STS IV used in vUpper64.
			 */
			uint8_t static_sts_iv[FIRA_STATIC_STS_IV_SIZE];
			/**
			 * @vendor_id: Vendor ID used in vUpper64.
			 */
			uint8_t vendor_id[FIRA_VENDOR_ID_SIZE];
		};
		/**
		 * @vupper64: vUpper64 used during Static STS ranging.
		 */
		uint8_t vupper64[FIRA_VUPPER64_SIZE];
	};
	/**
	 * @key_rotation: Enable/disable key rotation feature during
	 * Dynamic |NSQM33| or Provisioned STS ranging.
	 *
	 * Possible values:
	 *
	 * - false: No key rotation.
	 * - true: Key rotation enabled and period set by key_rotation_rate.
	 */
	uint8_t key_rotation;
	/**
	 * @key_rotation_rate: Defines n, with 2^n being the rotation rate of
	 * some keys used during Dynamic |NSQM33| or Provisioned STS Ranging, n shall be
	 * in the range of 0<=n<=15.
	 */
	uint8_t key_rotation_rate;
	/**
	 * @sub_session_id: Sub-session id for the controlee device. This
	 * configuration is applicable if STS_CONFIG is set to 0x02 or 0x04.
	 */
	uint32_t sub_session_id;
	/**
	 * @report_rssi: Activate rssi report
	 *
	 * Possible values:
	 *
	 * - 0: no rssi report
	 * - 1: activate rssi report
	 */
	uint8_t report_rssi;
	/**
	 * @result_report_config: Configure report information.
	 *
	 * - b0: report ToF in result message, disabled(0) or enabled(1,
	 *   default)
	 * - b1: report AoA azimuth in result message, disabled (0, default) or
	 *   enabled (1)
	 * - b2: report AoA elevation in result message, disabled (0, default)
	 *   or enabled (1)
	 * - b3: report AoA FOM in result message, disabled (0, default) or
	 *   enabled (1)
	 */
	uint8_t result_report_config;
	/**
	 * @link_layer_mode: Used to define link layer behavior.
	 *
	 * Possible values:
	 *
	 * - 0x00: Bypass mode (default).
	 * - 0x01: Connection less. |NSQM33|
	 * - Values 0x02 to 0xFF: RFU.
	 */
	uint8_t link_layer_mode;
	/**
	 * @mac_fcs_type:
	 * **[NOT IMPLEMENTED]** The length of the Frame Check Sequence in the
	 * session.
	 *
	 * Possible values:
	 *
	 * - 0x00: CRC 16 (default)
	 * - 0x01: CRC 32
	 * - Values 0x02 to 0xFF: RFU
	 *
	 * This parameter is not used in the current implementation.
	 *
	 * See &enum fira_mac_fcs_type.
	 */
	uint8_t mac_fcs_type;
	/**
	 * @prf_mode:
	 *
	 * Possible values:
	 *
	 * - 0x00: 62.4 MHz PRF. BPRF mode (default)
	 * - 0x01: 124.8 MHz PRF. HPRF mode. |NSQM33|
	 * - 0x02: 249.6 MHz PRF. HPRF mode with data rate 27.2 and 31.2 Mbps. |NSQM33|
	 *
	 * See &enum fira_prf_mode.
	 */
	uint8_t prf_mode;
	/**
	 * @cap_size_min: |NSQM33| Contention access period minimum value.
	 *
	 * Default: 5
	 */
	uint8_t cap_size_min;
	/**
	 * @cap_size_max: |NSQM33| Contention access period maximum value.
	 *
	 * Default: round_duration_slots - 1
	 */
	uint8_t cap_size_max;
	/**
	 * @number_of_sts_segments:
	 * **[NOT IMPLEMENTED]** Number of STS segments.
	 *
	 * Possible values:
	 *
	 * - 0x01: 1 STS Segment (default)
	 * - 0x02: 2 STS Segments (HPRF only) |NSQM33|
	 * - 0x03: 3 STS Segments (HPRF only) |NSQM33|
	 * - 0x04: 4 STS Segments (HPRF only) |NSQM33|
	 * - Values 0x05 to 0xFF: RFU
	 *
	 * This parameter is not used in the current implementation.
	 */
	uint8_t number_of_sts_segments;
	/**
	 * @meas_seq: |NSQM33| Sequence of measurement sequence steps,
	 * configures the Antenna Flexibility features.
	 */
	struct measurement_sequence meas_seq;
	/**
	 * @enable_diagnostics: Activate the diagnostics for each round.
	 */
	bool enable_diagnostics;
	/**
	 * @diags_frame_reports_fields: Select the fields to activate in the
	 * frame reports stored in the diagnostics. Applicable only when @enable_diagnostics
	 * is set to true.
	 */
	uint32_t diags_frame_reports_fields;
	/**
	 * @sts_length: Number of symbols in a STS segment.
	 *
	 * Possible values:
	 *
	 * - 0x00: 32 symbols
	 * - 0x01: 64 symbols (default)
	 * - 0x02: 128 symbols
	 * - Values 0x03 to 0xFF: RFU
	 */
	uint8_t sts_length;
	/**
	 * @min_frames_per_rr: |NSQM33| Minimal number of frames to be transmitted in OWR
	 * for AoA ranging round (block).
	 *
	 * This parameter is only used in OWR for AoA Mode, see
	 * ranging_round_usage paramater
	 */
	uint8_t min_frames_per_rr;
	/**
	 * @mtu_size: |NSQM33| Maximum Transfer Unit, max size allowed to be transmitted
	 * in frame. The value shall be restricted to the maximum possible MTU
	 * size of the given frame which includes MHR, Variable IE size and FCS
	 * size.
	 */
	uint16_t mtu_size;
	/**
	 * @inter_frame_interval_ms: |NSQM33| Interval between RFRAMES transmitted in
	 * OWR for AoA (in units of 1200 RSTU)
	 *
	 * This parameter is only used in OWR for AoA Mode, see
	 * ranging_round_usage paramater
	 */
	uint8_t inter_frame_interval_ms;
	/**
	 * @owr_aoa_measurement_ntf_period: |NSQM33| Configure period of OWR for AoA
	 * measurement notifications.
	 *
	 * Possible values:
	 *
	 * - 0x00 = SESSION_INFO_NTF sent for every received OWR Advertisement
	 *   frame (default)
	 * - 0x01 = SESSION_INFO_NTF sent once after MIN_FRAMES_PER_RR number of
	 *   AoA measurements are aggregated
	 */
	uint8_t owr_aoa_measurement_ntf_period;
	/**
	 * @session_info_ntf_config: |NSQM33| Configure session info notification.
	 *
	 * Possible values:
	 *
	 * - 0x00 = Disable session info notification (ntf)
	 * - 0x01 = Enable session info notification (default)
	 * - 0x02 = Enable session info ntf while inside proximity range
	 * - 0x03 = Enable session info ntf while inside AoA upper and lower
	 *   bounds
	 * - 0x04 = Enable session info ntf while inside AoA upper and lower
	 *   bounds as well as inside proximity range
	 * - 0x05 = Enable session info ntf only when entering or leaving
	 *   proximity range
	 * - 0x06 = Enable session info ntf only when entering or leaving AoA
	 *   upper and lower bounds
	 * - 0x07 = Enable session info ntf only when entering or leaving AoA
	 *   upper and lower bounds as well as entering or leaving proximity
	 *   range
	 */
	uint8_t session_info_ntf_config;
	/**
	 * @near_proximity_config_cm: |NSQM33| Lower bound in cm above which the
	 * ranging notifications should be enabled.
	 *
	 * Applicable when session_info_ntf_config is set to 0x02, 0x04, 0x05 or
	 * 0x07.  Should be less than or equal to far_proximity_config
	 * value.
	 */
	uint32_t near_proximity_config_cm;
	/**
	 * @far_proximity_config_cm: |NSQM33| Upper bound in cm above which the
	 * ranging notifications should be disabled.
	 *
	 * Applicable when session_info_ntf_config is set to 0x02, 0x04, 0x05 or
	 * 0x07.  Should be greater than or equal to
	 * near_proximity_config value.
	 */
	uint32_t far_proximity_config_cm;
	/**
	 * @lower_aoa_bound_config_azimuth_2pi: |NSQM33| Represent degrees.
	 *
	 * Applicable when session_info_ntf_config is set to 0x03, 0x04, 0x06 or
	 * 0x07.
	 */
	int32_t lower_aoa_bound_config_azimuth_2pi;
	/**
	 * @upper_aoa_bound_config_azimuth_2pi: |NSQM33| Represent degrees.
	 *
	 * Applicable when session_info_ntf_config is set to 0x03, 0x04, 0x06 or
	 * 0x07.
	 */
	int32_t upper_aoa_bound_config_azimuth_2pi;
	/**
	 * @lower_aoa_bound_config_elevation_2pi: |NSQM33| Represent degrees
	 *
	 * Applicable when session_info_ntf_config is set to 0x03, 0x04, 0x06 or
	 * 0x07.
	 */
	int16_t lower_aoa_bound_config_elevation_2pi;
	/**
	 * @upper_aoa_bound_config_elevation_2pi: |NSQM33| Represent degrees.
	 *
	 * Applicable when session_info_ntf_config is set to 0x03, 0x04, 0x06 or
	 * 0x07.
	 */
	int16_t upper_aoa_bound_config_elevation_2pi;
	/**
	 * @termination_count:
	 *
	 * in band termination attempt count.
	 */
	uint8_t termination_count;
};

/**
 * struct controlee_parameters - Controlee parameters.
 */
struct controlee_parameters {
	/**
	 * @sub_session_id: Sub-session id for the controlee device.
	 */
	uint32_t sub_session_id;
	/**
	 * @address: Controlee short address.
	 */
	uint16_t address;
	/**
	 * @sub_session: To indicate whether or not the controlee has a
	 * sub-session.
	 */
	bool sub_session;
	/**
	 * @sub_session_key_len: Size of the sub-session key, either 16 or 32
	 * bytes.
	 */
	uint8_t sub_session_key_len;
	/**
	 * @sub_session_key: Key used for sub-session's crypto calculations.
	 */
	uint8_t sub_session_key[FIRA_KEY_SIZE_MAX];
};

/**
 * struct controlees_parameters - Controlees list parameters.
 */
struct controlees_parameters {
	/**
	 * @controlees: List of controlees.
	 */
	struct controlee_parameters controlees[FIRA_RESPONDERS_MAX];
	/**
	 * @n_controlees: Number of controlees in the list.
	 */
	int n_controlees;
};

/**
 * struct dt_anchor_ranging_round_config - |NSQM33| Configuration parameters of
 * the ranging round for DT-Anchor.
 */
struct dt_anchor_ranging_round_config {
	/**
	 * @round_index: Round index.
	 */
	uint8_t round_index;
	/**
	 * @acting_role: Acting role (Initiator or Responder).
	 */
	uint8_t acting_role;
	/**
	 * @n_responders: Total number of DT-Anchor Responders for this
	 * ranging round (applicable when the acting role is Initiator).
	 */
	uint8_t n_responders;
	/**
	 * @are_slots_present: Flag indicating if explicit slot scheduling
	 * will follow (applicable when the acting role is Initiator).
	 */
	bool are_slots_present;
	/**
	 * @responders: Short addresses of DT-Anchor Responders for this
	 * ranging round (applicable when the acting role is Initiator).
	 */
	uint16_t responders[FIRA_RESPONDERS_MAX];
	/**
	 * @slots: Slots for Response DTMs for consecutive DT-Anchor Responders
	 * (explicit slot scheduling, applicable when the acting role is
	 * Initiator).
	 */
	uint8_t slots[FIRA_RESPONDERS_MAX];
};

/**
 * struct update_dt_anchor_ranging_rounds_cmd - |NSQM33| Request to MAC with
 * configuration of the ranging rounds for DT-Anchor.
 */
struct update_dt_anchor_ranging_rounds_cmd {
	/**
	 * @n_ranging_rounds: Total number of round configurations.
	 */
	int n_ranging_rounds;
	/**
	 * @ranging_rounds: Configuration parameters per ranging round.
	 */
	struct dt_anchor_ranging_round_config *ranging_rounds;
};

/**
 * struct update_dt_anchor_ranging_rounds_rsp - |NSQM33| Response from MAC
 * including indexes of ranging rounds which failed to be
 * configured for DT-Anchor.
 */
struct update_dt_anchor_ranging_rounds_rsp {
	/**
	 * @status: Status of the config_rsp.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @n_round_indexes: Number of failing rounds.
	 */
	int n_round_indexes;
	/**
	 * @round_indexes: Failing round indexes.
	 */
	uint8_t round_indexes[FIRA_DT_ANCHOR_MAX_ACTIVE_RR];
};

/**
 * struct dt_tag_ranging_rounds_config - |NSQM33| Configuration parameters of
 * the ranging rounds for DT-Tag.
 */
struct dt_tag_ranging_rounds_config {
	/**
	 * @round_indexes: Round indexes.
	 */
	uint8_t *round_indexes;
	/**
	 * @n_round_indexes: Total number of round indexes.
	 */
	int n_round_indexes;
};

/**
 * struct dt_tag_round_indexes_rsp - |NSQM33| Indexes of ranging rounds
 * which failed to be configured for DT-Tag.
 */
struct dt_tag_round_indexes_rsp {
	/**
	 * @status: Status of the config_rsp.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @n_round_indexes: Total number of round indexes.
	 */
	int n_round_indexes;
	/**
	 * @round_indexes: Round indexes.
	 */
	uint8_t round_indexes[FBS_DT_TAG_MAX_ACTIVE_RR];
};
/**
 * enum aoa_measurements_index - AOA measurements.
 * @FIRA_HELPER_AOA_AZIMUTH: Retrieve AOA azimuth.
 * @FIRA_HELPER_AOA: Retrieve AOA (same as azimuth).
 * @FIRA_HELPER_AOA_ELEVATION: Retrieve AOA elevation. |NSQM33|
 * @FIRA_HELPER_AOA_NB: Enum members number.
 */
enum aoa_measurements_index {
	FIRA_HELPER_AOA_AZIMUTH,
	FIRA_HELPER_AOA = FIRA_HELPER_AOA_AZIMUTH,
	FIRA_HELPER_AOA_ELEVATION,
	FIRA_HELPER_AOA_NB
};

/**
 * struct aoa_measurements - Fira Angle of Arrival measurements.
 *
 * Contains the different results of the AOA measurements.
 */
struct aoa_measurements {
	/**
	 * @rx_antenna_pair: Antenna pair index.
	 */
	uint8_t rx_antenna_pair;
	/**
	 * @aoa_fom_100: Estimation of local AoA reliability.
	 */
	uint8_t aoa_fom_100;
	/**
	 * @aoa_2pi: Estimation of reception angle.
	 */
	int16_t aoa_2pi;
	/**
	 * @pdoa_2pi: Estimation of reception phase difference.
	 */
	int16_t pdoa_2pi;
};

#define FIRA_TWR_MEASUREMENT_DISTANCE_INVALID INT32_MAX
/**
 * struct fira_twr_measurements - Fira ranging measurements.
 */
struct fira_twr_measurements {
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @status: Zero if ok, or error reason.
	 */
	uint8_t status;
	/**
	 * @slot_index: In case of error, slot index where the error was
	 * detected.
	 */
	uint8_t slot_index;
	/**
	 * @stopped: Ranging was stopped as requested [controller only].
	 */
	bool stopped;
	/**
	 * @nlos: Indicates if the ranging measurement was in Line of Sight (LoS)
	 * or Non-Line of Sight (NLoS): 0x00 = LoS, 0x01 = NLoS, 0xFF = Unable to determine.
	 */
	uint8_t nlos;
	/**
	 * @distance_cm: Distance in cm.
	 */
	int32_t distance_cm;
	/**
	 * @remote_aoa_azimuth_2pi: Estimation of reception angle in the azimuth
	 * of the participating device.
	 */
	int16_t remote_aoa_azimuth_2pi;
	/**
	 * @remote_aoa_elevation_pi: Estimation of reception angle in the
	 * elevation of the participating device.
	 */
	int16_t remote_aoa_elevation_pi;
	/**
	 * @remote_aoa_azimuth_fom_100: Estimation of azimuth reliability of the
	 * participating device.
	 */
	uint8_t remote_aoa_azimuth_fom_100;
	/**
	 * @remote_aoa_elevation_fom_100: Estimation of elevation of the
	 * participating device.
	 */
	uint8_t remote_aoa_elevation_fom_100;
	/**
	 * @local_aoa_measurements: Table of estimations of local measurements.
	 */
	struct aoa_measurements local_aoa_measurements[FIRA_HELPER_AOA_NB];
	/**
	 * @rssi: Computed rssi
	 */
	uint8_t rssi;
};

/**
 * struct fira_ranging_info - Common information on the ranging result.
 */
struct fira_ranging_info {
	/**
	 * @session_handle: Session handle of the ranging result.
	 */
	uint32_t session_handle;
	/**
	 * @sequence_number: Session notification counter.
	 */
	uint32_t sequence_number;
	/**
	 * @block_index: Current block index.
	 */
	uint32_t block_index;
	/**
	 * @ranging_interval_ms: Current ranging interval in unit of ms.
	 * formula: (block size * (stride + 1))
	 */
	uint32_t ranging_interval_ms;
	/**
	 * @timestamp_ns:
	 * **[NOT IMPLEMENTED]** Timestamp in nanoseconds in
	 * the CLOCK_MONOTONIC time reference.
	 *
	 * The current implementation does not provide any timestamp.
	 */
	uint64_t timestamp_ns;
	/**
	 * @diagnostic: Debug informations
	 */
	struct diagnostic_info *diagnostic;
	/**
	 * @psdus_report: Report containing all the psdus.
	 */
	struct uwbmac_buf *psdus_report;
};

/**
 * struct fira_twr_ranging_results - Ranging results for Fira SS-TWR/DS-TWR.
 */
struct fira_twr_ranging_results {
	/**
	 * @info: Common information on this ranging.
	 */
	struct fira_ranging_info *info;
	/**
	 * @n_measurements:
	 * Number of measurements stored in the measurements
	 * table.
	 */
	int n_measurements;
	/**
	 * @measurements: Ranging measurements information.
	 */
	struct fira_twr_measurements measurements[FIRA_RESPONDERS_MAX];
};

/**
 * struct fira_owr_aoa_measurements - |NSQM33| Ranging measurement for Fira OWR AoA.
 */
struct fira_owr_aoa_measurements {
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @status: Zero if ok, or error reason.
	 */
	uint8_t status;
	/**
	 * @nlos: Indicates if the reception of the message was in Line of Sight (LoS)
	 * or Non-Line of Sight (NLoS): 0x00 = LoS, 0x01 = NLoS, 0xFF = Unable to determine.
	 */
	uint8_t nlos;
	/**
	 * @frame_sequence_number: Sequence number as received in MHR.
	 */
	uint8_t frame_sequence_number;
	/**
	 * @block_index:
	 * Block Index number as received in the OWR message from the
	 * Advertiser.
	 */
	uint16_t block_index;
	/**
	 * @local_aoa_measurements: Table of estimations of local measurements.
	 */
	struct aoa_measurements local_aoa_measurements[FIRA_HELPER_AOA_NB];
};

/**
 * struct fira_owr_aoa_ranging_results - |NSQM33| Ranging results for Fira OWR AOA.
 */
struct fira_owr_aoa_ranging_results {
	/**
	 * @info: Common information on this ranging.
	 */
	struct fira_ranging_info *info;
	/**
	 * @n_measurements:
	 * Number of measurements stored in the measurements
	 * table.
	 */
	int n_measurements;
	/**
	 * @measurements: Ranging measurements information.
	 */
	struct fira_owr_aoa_measurements measurements[FIRA_OWR_AOA_MEASUREMENTS_MAX];
};

/**
 * struct fira_ul_tdoa_ranging_results - |NSQM33| Ranging results for FiRa UL-TDoA.
 * Will be extended with implementation of UT-Anchor.
 */
struct fira_ul_tdoa_ranging_results {
	/**
	 * @info: Common information on this ranging.
	 */
	struct fira_ranging_info *info;
};

#define FIRA_DL_TDOA_ANCHOR_LOCATION_SIZE_MAX 12
#define FIRA_DL_TDOA_MAX_ROUNDS_PER_BLOCK 6

/**
 * struct fira_dl_tdoa_measurements - |NSQM33| DL-TDOA ranging measurements.
 */
struct fira_dl_tdoa_measurements {
	/**
	 * @next: Pointer on next measurements if there is one, or NULL.
	 */
	struct fira_dl_tdoa_measurements *next;
	/**
	 * @short_addr: Address of the participating device.
	 */
	uint16_t short_addr;
	/**
	 * @status: Zero if ok, or error reason.
	 * See enum quwbs_fbs_status for all error codes.
	 */
	enum quwbs_fbs_status status;
	/**
	 * @message_type: Type of the message which has been received.
	 */
	enum fira_owr_message_type message_type;
	/**
	 * @tx_timestamp_type: Type of the TX timestamp (local time base vs
	 * common time base) included in the received message.
	 */
	enum fira_owr_dtm_timestamp_type tx_timestamp_type;
	/**
	 * @tx_timestamp_len: Length of the TX timestamp (40-bit vs 64-bit)
	 * included in the received message.
	 */
	enum fira_owr_dtm_timestamp_len tx_timestamp_len;
	/**
	 * @rx_timestamp_len: Length of the TX timestamp (40-bit vs 64-bit)
	 * calculated during the reception of the received message.
	 */
	enum fira_owr_dtm_timestamp_len rx_timestamp_len;
	/**
	 * @anchor_location_type: Type of the coordinate system of DT-Anchor
	 * location (0: WGS84, 1: relative) (if included).
	 */
	enum fira_dt_location_coord_system_type anchor_location_type;
	/**
	 * @anchor_location_present: True when the information about DT-Anchor
	 * location is included in the measurement, false otherwise.
	 */
	bool anchor_location_present;
	/**
	 * @active_ranging_round_indexes_len: Number of active ranging round
	 * indexes included in the measurement.
	 */
	uint8_t active_ranging_round_indexes_len;
	/**
	 * @round_index: Index of the current ranging round.
	 */
	uint8_t round_index;
	/**
	 * @block_index: Index of the current ranging block.
	 */
	uint16_t block_index;
	/**
	 * @local_aoa_azimuth_2pi: AoA Azimuth in degrees measured by the DT-Tag
	 * during the reception (encoded as Q9.7).
	 */
	int16_t local_aoa_azimuth_2pi;
	/**
	 * @local_aoa_elevation_2pi: AoA Elevation in degrees measured by the
	 * DT-Tag during the reception (encoded as Q9.7).
	 */
	int16_t local_aoa_elevation_2pi;
	/**
	 * @local_aoa_azimuth_fom: Reliability of the estimated AoA Azimuth
	 * measured by the DT-Tag during the reception (range: 0-100).
	 */
	uint8_t local_aoa_azimuth_fom;
	/**
	 * @local_aoa_elevation_fom: Reliability of the estimated AoA Elevation
	 * measured by the DT-Tag during the reception (range: 0-100).
	 */
	uint8_t local_aoa_elevation_fom;
	/**
	 * @rx_rssi: RSSI measured by the DT-Tag during the reception (encoded
	 * as Q7.1).
	 */
	uint8_t rx_rssi;
	/**
	 * @nlos: Indicates if the reception of the message was in Line of Sight (LoS)
	 * or Non-Line of Sight (NLoS): 0x00 = LoS, 0x01 = NLoS, 0xFF = Unable to determine.
	 */
	uint8_t nlos;
	/**
	 * @local_cfo: Clock frequency offset measured locally with respect to
	 * the DT-Anchor that sent the message received (encoded as Q6.10).
	 */
	uint16_t local_cfo;
	/**
	 * @remote_cfo: Clock frequency offset of a Responder DT-Anchor with
	 * respect to the Initiator DT-Anchor of the ranging round as included
	 * in the received message (encoded as Q6.10).
	 */
	uint16_t remote_cfo;
	/**
	 * @tx_timestamp_rctu: TX timestamp included in the received message
	 * (unit: RCTU).
	 */
	uint64_t tx_timestamp_rctu;
	/**
	 * @rx_timestamp_rctu: RX timestamp calculated during the reception of
	 * the received message (unit: RCTU).
	 */
	uint64_t rx_timestamp_rctu;
	/**
	 * @initiator_reply_time_rctu: Reply time of the Initiator DT-Anchor
	 * measured between the reception of Response DTM and the transmission
	 * of Final DTM (used only in DS-TWR, unit: RCTU).
	 */
	uint32_t initiator_reply_time_rctu;
	/**
	 * @responder_reply_time_rctu: Reply time of the Responder DT-Anchor
	 * measured between the reception of Poll DTM and the transmission of
	 * Response DTM (unit: RCTU).
	 */
	uint32_t responder_reply_time_rctu;
	/**
	 * @anchor_location: Location coordinates of DT-Anchor that sent the
	 * message received.
	 */
	uint8_t anchor_location[FIRA_DL_TDOA_ANCHOR_LOCATION_SIZE_MAX];
	/**
	 * @active_ranging_round_indexes: List of active ranging round indexes
	 * in which the DT-Anchor that sent the message received participates.
	 */
	uint8_t active_ranging_round_indexes[FIRA_DL_TDOA_MAX_ROUNDS_PER_BLOCK];
	/**
	 * @initiator_responder_tof_rctu: Time of Flight measured between the
	 * Initiator DT-Anchor and the Responder DT-Anchor (for SS-TWR it's
	 * calculated by Initiator DT-Anchor and included in Poll DTM and for
	 * DS-TWR it's calculated by Responder DT-Anchor and included in
	 * Response DTM, unit: RCTU)
	 */
	uint16_t initiator_responder_tof_rctu;
};

/**
 * struct fira_dl_tdoa_ranging_results - |NSQM33| Ranging results for Fira DL-TDOA.
 */
struct fira_dl_tdoa_ranging_results {
	/**
	 * @info: Common information on this ranging.
	 */
	struct fira_ranging_info *info;
	/**
	 * @n_measurements:
	 * Number of measurements stored in the measurements
	 * table.
	 */
	int n_measurements;
	/**
	 * @measurements: Linked list of the DL-TDOA measurements or NULL.
	 */
	struct fira_dl_tdoa_measurements *measurements;
};

/**
 * struct controlee_status - Controlee addition/deletion notification status.
 */
struct controlee_status {
	/**
	 * @short_address: Controlee short address.
	 */
	uint16_t short_address;
	/**
	 * @sub_session_id: Sub-session id of the current controlee.
	 */
	uint32_t sub_session_id;
	/**
	 * @status_code: See &enum fira_multicast_update_status.
	 */
	uint8_t status_code;
};

/**
 * struct fira_session_multicast_list_ntf_content - Necessary content to fill
 * a session update controller multicast list notification.
 */
struct fira_session_multicast_list_ntf_content {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @remaining_multicast_list_size: New available size in the
	 * multicast list. Maximum size is defined by FIRA_RESPONDERS_MAX.
	 */
	uint8_t remaining_multicast_list_size;
	/**
	 * @controlees: List of controlees with their corresponding multicast
	 * list update status.
	 */
	struct controlee_status controlees[FIRA_RESPONDERS_MAX];
	/**
	 * @n_controlees: Number of controlees in the previous list.
	 */
	uint8_t n_controlees;
};

/**
 * struct data_credit_ntf_content - |NSQM33| Fira DATA_CREDIT_NFT content.
 */
struct data_credit_ntf_content {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @credit_avail: Credit availability
	 * 0x00 Credit is not available
	 * 0x01 Credit is available
	 */
	uint8_t credit_avail;
};

/**
 * struct data_transfer_status_ntf_content - |NSQM33| Fira
 * SESSION_DATA_TRANSFER_STATUS_NTF content.
 */
struct data_transfer_status_ntf_content {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @uci_seq_nr: The Sequence Number identifying the UCI Data Message
	 * this NTF is for.
	 */
	uint16_t uci_seq_nr;
	/**
	 * @status: Status Code. See &enum uci_data_transfer_status_code.
	 */
	uint8_t status;
	/**
	 * @tx_count: Indicates the number of times Application Data with the
	 * same UCI Sequence Number has been transmitted.
	 */
	uint8_t tx_count;
};

/**
 * struct data_message_content - |NSQM33| Fira DATA_MESSAGE_SND and DATA_MESSAGE_RCV
 * content.
 */
struct data_message_content {
	/**
	 * @session_handle: Session handle.
	 */
	uint32_t session_handle;
	/**
	 * @short_addr: Short_addr.
	 * For DATA_MESSAGE_SND: short_addr of the Application Data recipient.
	 * For DATA_MESSAGE_RCV: short_addr of the sender of the Application
	 * Data.
	 */
	uint16_t short_addr;
	/**
	 * @uci_seq_nr: Sequence Number for the UCI Data Message.
	 */
	uint16_t uci_seq_nr;
	/**
	 * @status: Status. Applicable only in case of DATA_MESSAGE_RCV.
	 * 0x00 STATUS_SUCCESS
	 * 0x01 STATUS_ERROR
	 * 0x02 STATUS_UNKNOWN
	 */
	uint8_t status;
	/**
	 * @data_segment_info: See &enum fira_data_segment_info.
	 */
	uint8_t data_segment_info;
	/**
	 * @data_len: Length of the data.
	 */
	uint16_t data_len;
	/**
	 * @data: Application Data.
	 */
	uint8_t *data;
};

/**
 * struct fira_hus_controller_phase_config - |NSQM33| Phase configuration parameters used by a HUS
 * controller device to bind a secondary session to a primary session.
 */
struct fira_hus_controller_phase_config {
	/**
	 * @session_id: Session id of the targeted phase.
	 */
	uint32_t session_id;
	/**
	 * @start_slot_index: Slot index of the first slot of the phase.
	 */
	uint16_t start_slot_index;
	/**
	 * @end_slot_index: Slot index of the last slot of the phase.
	 */
	uint16_t end_slot_index;
	/**
	 * @controller_short_addr: MAC short address of the controller of the phase.
	 */
	uint16_t controller_short_addr;
	/**
	 * @control: Information about the current phase.
	 * b0: 0 = Short addressing mode, 1 = Extended addressing mode.
	 * b1: 0 = CAP phase, 1 = CFP phase.
	 */
	uint8_t control;
};

/**
 * struct fira_hus_controller_config_cmd - |NSQM33| List of secondary sessions to bind to a primary
 * session. Only applicable to a HUS controller device.
 */
struct fira_hus_controller_config_cmd {
	/**
	 * @update_time_us: Time in microseconds when this configuration shall be applied.
	 */
	uint64_t update_time_us;
	/**
	 * @session_handle: Handle of the targeted session.
	 */
	uint32_t session_handle;
	/**
	 * @phase_list: List of CAP or CFP phases.
	 */
	struct fira_hus_controller_phase_config *phase_list;
	/**
	 * @number_of_phases: Number of CAP or CFP phases in the HUS ranging round.
	 */
	uint8_t number_of_phases;
};

/**
 * struct fira_hus_controlee_phase_config - |NSQM33| Phase configuration parameters used by a HUS
 * controlee device to bind a secondary session to a primary session.
 */
struct fira_hus_controlee_phase_config {
	/**
	 * @session_handle: Session handle of the targeted phase.
	 */
	uint32_t session_handle;
};

/**
 * struct fira_hus_controlee_config_cmd - |NSQM33| Status of the configuration command binding
 * secondary sessions to a primary session.
 */
struct fira_hus_controlee_config_cmd {
	/**
	 * @session_handle: Handle of the targeted session.
	 */
	uint32_t session_handle;
	/**
	 * @phase_list: List of CAP or CFP phases.
	 */
	struct fira_hus_controlee_phase_config *phase_list;
	/**
	 * @number_of_phases: Number of CAP or CFP phases in the HUS ranging round.
	 */
	uint8_t number_of_phases;
};

/**
 * enum fira_helper_cb_type - Callback type. See &fira_helper_notification_cb_t
 * @FIRA_HELPER_CB_TYPE_UNSPEC: unspecified callback type
 * @FIRA_HELPER_CB_TYPE_TWR_RANGE_NTF:
 * 		Callback content is struct fira_twr_ranging_results*.
 * @FIRA_HELPER_CB_TYPE_OWR_AOA_NTF: |NSQM33|
 * 		Callback content is struct fira_owr_aoa_ranging_results*.
 * @FIRA_HELPER_CB_TYPE_UL_TDOA_NTF: |NSQM33|
 * 		Callback content is struct fira_ul_tdoa_ranging_results*.
 * @FIRA_HELPER_CB_TYPE_DL_TDOA_NTF: |NSQM33|
 * 		Callback content is struct fira_dl_tdoa_ranging_results*.
 * @FIRA_HELPER_CB_TYPE_SESSION_DATA_CREDIT_NTF: |NSQM33|
 * 		Callback content is struct data_credit_ntf_content*.
 * @FIRA_HELPER_CB_TYPE_SESSION_DATA_TRANSFER_STATUS_NTF: |NSQM33|
 * 		Callback content is struct data_transfer_status_ntf_content*.
 * @FIRA_HELPER_CB_TYPE_DATA_MESSAGE_RCV: |NSQM33|
 * 		Callback content is struct data_message_content*.
 * @FIRA_HELPER_CB_TYPE_SESSION_STATUS_NTF:
 * 		Callback content is struct fbs_helper_session_status_ntf*.
 * @FIRA_HELPER_CB_TYPE_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_NTF:
 * 		Callback content is struct
 * fira_session_multicast_list_ntf_content*.
 */
enum fira_helper_cb_type {
	FIRA_HELPER_CB_TYPE_UNSPEC,
	FIRA_HELPER_CB_TYPE_TWR_RANGE_NTF,
	FIRA_HELPER_CB_TYPE_OWR_AOA_NTF,
	FIRA_HELPER_CB_TYPE_UL_TDOA_NTF,
	FIRA_HELPER_CB_TYPE_DL_TDOA_NTF,
	FIRA_HELPER_CB_TYPE_SESSION_DATA_CREDIT_NTF,
	FIRA_HELPER_CB_TYPE_SESSION_DATA_TRANSFER_STATUS_NTF,
	FIRA_HELPER_CB_TYPE_DATA_MESSAGE_RCV,
	FIRA_HELPER_CB_TYPE_SESSION_STATUS_NTF,
	FIRA_HELPER_CB_TYPE_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_NTF,
};

/**
 * typedef fira_helper_notification_cb_t - Notification callback type.
 * @cb_type: Type of callback depending on exact message to be sent.
 * @content: Generic content with results depending on the cb_type.
 * @user_data: User data pointer given to fira_helper_open.
 *
 * See enum fira_helper_cb_type documentation for more information on the
 * content.
 */
typedef void (*fira_helper_notification_cb_t)(enum fira_helper_cb_type cb_type, const void *content,
					      void *user_data);

/*
 * struct fira_context - Fira helper context.
 */
struct fira_context {
	fira_helper_notification_cb_t notification_cb;
	void *user_data;
	struct fbs_helper fbs_helper;
};

/**
 * fira_helper_open() - Initialize the fira helper context.
 * @ctx: Fira helper context.
 * @uwbmac: UWB MAC context.
 * @notification_cb: Callback function for notifications feedback.
 * @scheduler: Scheduler name to use with the region.
 * @region_id: Region identifier to associate with the region.
 * @user_data: User data pointer to give back in callback.
 *
 * NOTE: This function must be called first. @fira_helper_close must be called
 * at the end of the application to ensure resources are freed.
 * The channel will be managed by the helper, this means you should neither use
 * uwbmac_channel_create nor uwbmac_channel_release.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_open(struct fira_context *ctx, struct uwbmac_context *uwbmac,
			   fira_helper_notification_cb_t notification_cb, const char *scheduler,
			   int region_id, void *user_data);

/**
 * fira_helper_close() - De-initialize the fira helper context.
 * @ctx: Fira helper context.
 */
void fira_helper_close(struct fira_context *ctx);

/**
 * fira_helper_set_device_status_cb() - Set the device status callback.
 * @ctx: Fira helper context.
 * @cb: Callback for all device status notifications.
 *
 * NOTE: Temporary api before we inverse dependancy with fbs_helper.
 * Once this is done client will have to directly use
 * fbs_helper_set_device_status_ntf_cb.
 *
 * Return: QERR_SUCCESS on success, on error otherwise.
 */
enum qerr fira_helper_set_device_status_cb(struct fira_context *ctx,
					   fbs_helper_device_status_ntf_cb cb);

/**
 * fira_helper_set_scheduler() - Set the scheduler and open the MAC region.
 * @ctx: Fira helper context.
 *
 * NOTE: This function must be called while the UWB MAC is stopped.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_scheduler(struct fira_context *ctx);

/**
 * fira_helper_get_capabilities() - Get the FiRa region capabilities.
 * @ctx: Fira helper context.
 * @capabilites: Fira capabilites.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_capabilities(struct fira_context *ctx,
				       struct fira_capabilities *capabilites);

/**
 * fira_helper_init_session() - Initialize a fira session.
 * @ctx: Fira helper context.
 * @session_id: Session identifier.
 * @session_type: Session type value.
 * @rsp: Session init response message information.
 *
 * This function must be called first to create and initialize the fira session.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_init_session(struct fira_context *ctx, uint32_t session_id,
				   enum quwbs_fbs_session_type session_type,
				   struct fbs_session_init_rsp *rsp);

/**
 * fira_helper_start_session() - Start a fira session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 *
 * This function must be called after fira session was initialized.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_start_session(struct fira_context *ctx, uint32_t session_handle);

/**
 * fira_helper_stop_session() - Stop a fira session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 *
 * This function stop the session ranging.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_stop_session(struct fira_context *ctx, uint32_t session_handle);

/**
 * fira_helper_deinit_session() - Deinitialize a fira session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 *
 * This function is called to free all memory allocated by the session.
 *
 * Return: QERR_SUCCESS or QERR_EBUSY on success, an error otherwise.
 *   The QERR_EBUSY is used to indicate that an active session has been deinit.
 */
enum qerr fira_helper_deinit_session(struct fira_context *ctx, uint32_t session_handle);

/**
 * fira_helper_get_session_parameters() - Get session parameters.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @session_params: Session parameters.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_parameters(struct fira_context *ctx, uint32_t session_handle,
					     struct session_parameters *session_params);

/**
 * fira_helper_session_get_count() - Get sessions count, the number of active
 * and inactive sessions.
 * @ctx: Fira helper context.
 * @count: Session count.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_session_get_count(struct fira_context *ctx, int *count);

/**
 * fira_helper_session_get_state() - Get session state.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @state: Session state.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_session_get_state(struct fira_context *ctx, uint32_t session_handle,
					int *state);

/**
 * fira_helper_get_ranging_count() - Get ranging count, the number of times
 * ranging has been attempted during the session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @rsp: Ranging count response message information.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_ranging_count(struct fira_context *ctx, uint32_t session_handle,
					struct fbs_ranging_count_rsp *rsp);

/**
 * fira_helper_add_controlee() - Add one controlee to a specific session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @controlee: Controlee to add.
 *
 * Return: 0 or positive value on success, negative value on error.
 */
int fira_helper_add_controlee(struct fira_context *ctx, uint32_t session_handle,
			      const struct controlee_parameters *controlee);

/**
 * fira_helper_delete_controlee() - Delete one controlee from a specific
 *   session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @controlee: Controlee to delete.
 *
 * Return: 0 or positive value on success, negative value on error.
 */
int fira_helper_delete_controlee(struct fira_context *ctx, uint32_t session_handle,
				 const struct controlee_parameters *controlee);

/**
 * fira_helper_get_controlees() - Get controlees list.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @controlees: List of controlees to write.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_controlees(struct fira_context *ctx, uint32_t session_handle,
				     struct controlees_parameters *controlees);

/**
 * fira_helper_get_controlees_count() - Get number of currently known
 *   controlees.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @count: Number of controlees known.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_controlees_count(struct fira_context *ctx, uint32_t session_handle,
					   int *count);

/**
 * fira_helper_data_message_send() - |NSQM33| Send data message.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_content: Data message.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_data_message_send(struct fira_context *ctx, uint32_t session_handle,
					const struct data_message_content *data_content);

/**
 * fira_helper_set_session_device_type() - Sets the device type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @device_type: 0 - CONTROLEE, 1 - CONTROLLER.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_device_type(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t device_type);

/**
 * fira_helper_set_session_dl_tdoa_time_reference_anchor() - |NSQM33| Set or reset the
 *   time reference anchor.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @global_time: 0 - DISABLE, 1 - Set DT-ANCHOR as global time reference and
 *   sets its cost metric to zero.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_time_reference_anchor(struct fira_context *ctx,
								uint32_t session_handle,
								uint8_t global_time);

/**
 * fira_helper_set_session_dl_tdoa_responder_tof() - |NSQM33| Include or not the
 *   responder tof.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @responder_tof: 0 - Do not include, 1 - include the estimated Responder ToF
 *   Result in a Response DTM.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_responder_tof(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t responder_tof);

/**
 * fira_helper_set_session_dl_tdoa_ranging_method() - |NSQM33| Set dl-tdoa ranging
 *   method.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @method: 0 - SS-TWR, 1 - DS-TWR.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_ranging_method(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t method);

/**
 * fira_helper_set_session_dl_tdoa_tx_timestamp_type() - |NSQM33| Configure tx timestamp
 *   type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @type: Timestamp type.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_tx_timestamp_type(struct fira_context *ctx,
							    uint32_t session_handle, uint8_t type);

/**
 * fira_helper_set_session_dl_tdoa_tx_timestamp_len() - |NSQM33| Configure tx timestamp
 *   length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Timestamp length.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_tx_timestamp_len(struct fira_context *ctx,
							   uint32_t session_handle, uint8_t len);

/**
 * fira_helper_set_session_dl_tdoa_hop_count() - |NSQM33| Set dl-tdoa hop count presence.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @count: 0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_hop_count(struct fira_context *ctx,
						    uint32_t session_handle, uint8_t count);

/**
 * fira_helper_set_session_dl_tdoa_anchor_cfo() - |NSQM33| Set dl-tdoa presence of cfo.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cfo: 0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_anchor_cfo(struct fira_context *ctx,
						     uint32_t session_handle, uint8_t cfo);

/**
 * fira_helper_set_session_dl_tdoa_anchor_location_presence() - |NSQM33| Set dl-tdoa
 *   presence of dt-anchor location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @presence:presence of the information about DT-Anchor location in DTMs
 *   0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_anchor_location_presence(struct fira_context *ctx,
								   uint32_t session_handle,
								   uint8_t presence);

/**
 * fira_helper_set_session_dl_tdoa_anchor_location() - |NSQM33| Set dl-tdoa dt-anchor
 *   location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of the array according to the location type.
 * @data:data array that represents location.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_anchor_location(struct fira_context *ctx,
							  uint32_t session_handle, uint8_t len,
							  uint8_t *data);

/**
 * fira_helper_set_session_dl_tdoa_anchor_location_type() - |NSQM33| Set dl-tdoa type of
 *   dt-anchor location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @type: Type of the DT-Anchor location format: 0 - WGS84, 1 - relative.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_anchor_location_type(struct fira_context *ctx,
							       uint32_t session_handle,
							       uint8_t type);

/**
 * fira_helper_set_session_dl_tdoa_active_ranging_rounds() - |NSQM33| Set dl-tdoa
 *   presence of ranging rounds.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @rrounds: 0 - deactivated, 1 - activated.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_active_ranging_rounds(struct fira_context *ctx,
								uint32_t session_handle,
								uint8_t rrounds);

/**
 * fira_helper_set_session_dl_tdoa_block_skipping() - |NSQM33| Set dl-tdoa number of
 *   blocks that shall be skipped between 2 active ranging blocks.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @number: Number of blocks to be skipped by the dt-tag.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_dl_tdoa_block_skipping(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t number);

/**
 * fira_helper_set_session_report_psdus() - |NSQM33| Enable/disable psdus report.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @active: True to enable psdus are reported, false otherwise.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_report_psdus(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t active);

/**
 * fira_helper_get_session_in_band_termination_attempt_count() - Get the in band
 *   termination attempt count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @termination_count: Termination_count.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_in_band_termination_attempt_count(struct fira_context *ctx,
								    uint32_t session_handle,
								    uint8_t *termination_count);

/**
 * fira_helper_get_session_dl_tdoa_time_reference_anchor() - |NSQM33| Get the time
 *   reference anchor.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @global_time: Time reference anchor.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_time_reference_anchor(struct fira_context *ctx,
								uint32_t session_handle,
								uint8_t *global_time);

/**
 * fira_helper_get_session_dl_tdoa_responder_tof() - |NSQM33| Get the responder tof
 *   config.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @responder_tof: Responder time of flight.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_responder_tof(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t *responder_tof);

/**
 * fira_helper_get_session_dl_tdoa_ranging_method() - |NSQM33| Get dl-tdoa ranging
 *   method.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @method: 0 - SS-TWR, 1 - DS-TWR.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_ranging_method(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t *method);

/**
 * fira_helper_get_session_dl_tdoa_tx_timestamp_type() - |NSQM33| Get tx timestamp type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @type: Timestamp type.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_tx_timestamp_type(struct fira_context *ctx,
							    uint32_t session_handle, uint8_t *type);

/**
 * fira_helper_get_session_dl_tdoa_tx_timestamp_len() - |NSQM33| Get tx timestamp length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Timestamp length.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_tx_timestamp_len(struct fira_context *ctx,
							   uint32_t session_handle, uint8_t *len);

/**
 * fira_helper_get_session_dl_tdoa_hop_count() - |NSQM33| Get dl-tdoa hop count presence.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @count: 0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_hop_count(struct fira_context *ctx,
						    uint32_t session_handle, uint8_t *count);

/**
 * fira_helper_get_session_dl_tdoa_anchor_cfo() - |NSQM33| Get dl-tdoa presence of cfo.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cfo: 0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_anchor_cfo(struct fira_context *ctx,
						     uint32_t session_handle, uint8_t *cfo);

/**
 * fira_helper_get_session_dl_tdoa_anchor_location_presence() - |NSQM33| Get dl-tdoa
 *   presence of dt-anchor location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @presence:presence of the information about DT-Anchor location in DTMs
 *   0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_anchor_location_presence(struct fira_context *ctx,
								   uint32_t session_handle,
								   uint8_t *presence);

/**
 * fira_helper_get_session_dl_tdoa_anchor_location_type() - |NSQM33| Get dl-tdoa type of
 *   dt-anchor location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @type: Type of the DT-Anchor location format: 0 - WGS84, 1 - RELATIVE.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_anchor_location_type(struct fira_context *ctx,
							       uint32_t session_handle,
							       uint8_t *type);

/**
 * fira_helper_get_session_dl_tdoa_anchor_location() - |NSQM33| Get dl-tdoa dt-anchor
 *   location.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of the array according to the location type.
 * @data:data array that represents location.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_anchor_location(struct fira_context *ctx,
							  uint32_t session_handle, uint8_t len,
							  uint8_t *data);

/**
 * fira_helper_get_session_dl_tdoa_active_ranging_rounds() - |NSQM33| Get dl-tdoa
 *   presence of ranging rounds.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @rrounds: 0 - DEACTIVATED, 1 - ACTIVATED.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_active_ranging_rounds(struct fira_context *ctx,
								uint32_t session_handle,
								uint8_t *rrounds);

/**
 * fira_helper_get_session_dl_tdoa_block_skipping() - |NSQM33| Get dl-tdoa number of
 *   blocks that shall be skipped between 2 active ranging blocks.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @number: Number of blocks to be skipped by the dt-tag.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_dl_tdoa_block_skipping(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t *number);

/**
 * fira_helper_get_session_report_psdus() - |NSQM33| Get activation of psdus report.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @active: True if psdus are reported, false otherwise
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_report_psdus(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t *active);

/**
 * fira_helper_bool_to_ranging_round_control() - get the ranging round
 *   control bitfield format.
 * @result_report_phase: True if result report phase present.
 * @skip_ranging_control_phase: True if ranging control phase is skipped.
 *
 * Return: ranging round control bitfield format.
 */
static inline uint8_t fira_helper_bool_to_ranging_round_control(bool result_report_phase,
								bool skip_ranging_control_phase)
{
#define __BIT(n) (1ul << (n))
	return (result_report_phase ? __BIT(0) : 0) | __BIT(1) |
	       (skip_ranging_control_phase ? __BIT(2) : 0);
#undef __BIT
}
/**
 * fira_helper_set_session_ranging_round_usage() - Sets ranging round usage.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @ranging_round_usage: See &enum fira_ranging_round_usage.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ranging_round_usage(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint8_t ranging_round_usage);

/**
 * fira_helper_set_session_device_role()- Sets the device role
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @device_role: Role played by the device, accepted value are initiator for
 *   controller and responder for controlee.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_device_role(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t device_role);

/**
 * fira_helper_set_session_sts_config() - scrambled timestamp sequence
 *   configuration.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sts_config: Possible values:
 *   0x01: Static STS (default).
 *   0x02: Dynamic STS. |NSQM33|
 *   0x04: RFU (Dynamic STS - Individual Key). |NSQM33|
 *   0x08: Provisioned STS.
 *   0x10: RFU (Provisioned STS - Individual Key).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_sts_config(struct fira_context *ctx, uint32_t session_handle,
					     uint8_t sts_config);

/**
 * fira_helper_set_session_multi_node_mode() - The multi-node mode used during a
 *  round.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @multi_node_mode:
 *  - FIRA_MULTI_NODE_MODE_UNICAST,
 *  - FIRA_MULTI_NODE_MODE_ONE_TO_MANY,
 *  - **[NOT IMPLEMENTED]** FIRA_MULTI_NODE_MODE_MANY_TO_MANY,
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_multi_node_mode(struct fira_context *ctx, uint32_t session_handle,
						  uint8_t multi_node_mode);

/**
 * fira_helper_set_session_short_address() - Sets short address.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @short_addr: Short_addr.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_short_address(struct fira_context *ctx, uint32_t session_handle,
						uint16_t short_addr);

/**
 * fira_helper_set_session_destination_short_addresses() - Sets destination
 *   short addresses.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @n_dest_short_addr: Number of destination short addresses.
 * @dest_short_addr: Array of destination short addresses.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_destination_short_addresses(struct fira_context *ctx,
							      uint32_t session_handle,
							      uint32_t n_dest_short_addr,
							      uint16_t *dest_short_addr);

/**
 * fira_helper_set_session_time0_ns() - Sets an absolute value of the initiation
 *   time [ns].
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @time0_ns: time0_ns.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_time0_ns(struct fira_context *ctx, uint32_t session_handle,
					   uint64_t time0_ns);

/**
 * fira_helper_set_session_slot_duration_rstu() - Sets slot duration rstu.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @slot_duration_rstu: Slot_duration_rstu. - Duration of a slot in RSTU
 *   (1200RSTU=1ms)
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_slot_duration_rstu(struct fira_context *ctx,
						     uint32_t session_handle,
						     uint32_t slot_duration_rstu);

/**
 * fira_helper_set_session_round_duration_slots() - Sets round duration slots.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @round_duration_slots: Number of slots per ranging round.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_round_duration_slots(struct fira_context *ctx,
						       uint32_t session_handle,
						       uint32_t round_duration_slots);

/**
 * fira_helper_set_session_block_duration_ms() - Sets block duration.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @block_duration_ms: Block size in unit of 1200 RSTU (same as ms).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_block_duration_ms(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint32_t block_duration_ms);

/**
 * fira_helper_set_session_time_base() - |NSQM33| Set session time base configuration.
 *
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @time_base_param: Session time base parameter array.
 * Expected array size equals to FIRA_TIME_BASE_SIZE.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_time_base(struct fira_context *ctx, uint32_t session_handle,
					    const uint8_t *time_base_param);

/**
 * fira_helper_set_session_block_stride_length() - Sets block stride length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @block_stride_length: Number of blocks to stride.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_block_stride_length(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint32_t block_stride_length);

/**
 * fira_helper_set_session_round_hopping() - Enables or disable round hopping
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @round_hopping: False - disabled, true - enabled
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_round_hopping(struct fira_context *ctx, uint32_t session_handle,
						uint8_t round_hopping);

/**
 * fira_helper_set_session_priority() - sets the priority.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @priority: Priority of the session.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_priority(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t priority);

/**
 * fira_helper_set_session_mac_address_mode() - sets the MAC addressing mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_address_mode: MAC addressing mode.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_mac_address_mode(struct fira_context *ctx,
						   uint32_t session_handle,
						   uint8_t mac_address_mode);

/**
 * fira_helper_set_session_ranging_round_control() - Set the ranging round
 *   control bitfield.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @ranging_round_control: Bitfield:
 *   - b0: ranging result report phase is disabled (0) or enabled (1)
 *   - b1: Control Message is sent in band (1) or not (0, not supported)
 *   - b2: Control Message is sent separately (0) or piggybacked to RIM (1)
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ranging_round_control(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t ranging_round_control);

/**
 * fira_helper_set_session_schedule_mode() - Sets schedule mode parameter.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @schedule_mode:
 *   - 0x00 - Contention-based ranging. |NSQM33|
 *   - 0x01 - Time-scheduled ranging.
 *   - 0x02 - Hybrid-based ranging. |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_schedule_mode(struct fira_context *ctx, uint32_t session_handle,
						uint8_t schedule_mode);

/**
 * fira_helper_set_session_max_number_of_measurements() - Sets the max number of
 *   measurements.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @max_number_of_measurements: Max_number_of_measurements.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_max_number_of_measurements(struct fira_context *ctx,
							     uint32_t session_handle,
							     uint32_t max_number_of_measurements);

/**
 * fira_helper_set_session_max_rr_retry() - Sets the max rr retry.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @max_rr_retry: Max_rr_retry.
 *   Number of failed ranging round attempts before stopping the session.
 *   The value zero disables the feature.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_max_rr_retry(struct fira_context *ctx, uint32_t session_handle,
					       uint32_t max_rr_retry);

/**
 * fira_helper_set_session_channel_number() - Sets the channel number.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @channel_number: Channel_number.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_channel_number(struct fira_context *ctx, uint32_t session_handle,
						 uint8_t channel_number);

/**
 * fira_helper_set_session_preamble_code_index() - Sets preamble code index.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @preamble_code_index: Preamble code index.
 *   Possible values:
 *   - 9-24: BPRF
 *   - 25-32: HPRF
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_preamble_code_index(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint8_t preamble_code_index);

/**
 * fira_helper_set_session_rframe_config() - Sets rframe_config.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @rframe_config: Ranging frame config.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_rframe_config(struct fira_context *ctx, uint32_t session_handle,
						uint8_t rframe_config);

/**
 * fira_helper_set_session_preamble_duration() - Sets preamble duration.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @preamble_duration: 0x00: 32 symbols |NSQM33| or 0x01: 64 symbols (default)
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_preamble_duration(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t preamble_duration);

/**
 * fira_helper_set_session_sfd_id() - Sets sfd_id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sfd_id: 0 or 2 in BPRF, 1-4 in HPRF |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_sfd_id(struct fira_context *ctx, uint32_t session_handle,
					 uint8_t sfd_id);

/**
 * fira_helper_set_session_psdu_data_rate() - Sets psdu data rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @psdu_data_rate: Possible values:
 *   - 0: 6.81Mbps (default)
 *   - 1: 7.80 Mbps |NSQM33|
 *   - 2: 27.2 Mbps |NSQM33|
 *   - 3: 31.2 Mbps |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_psdu_data_rate(struct fira_context *ctx, uint32_t session_handle,
						 uint8_t psdu_data_rate);

/**
 * fira_helper_set_session_sub_session_id() - Sets controlee' sub-session id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sub_session_id: Controlee' sub-session id used during Dynamic or Provisioned
 * STS for Responder Specific Sub-session Key.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_sub_session_id(struct fira_context *ctx, uint32_t session_handle,
						 uint32_t sub_session_id);

/**
 * fira_helper_set_session_vendor_id() - Sets Vendor ID.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @vendor_id: Vendor ID used for vUpper64.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_vendor_id(struct fira_context *ctx, uint32_t session_handle,
					    uint8_t vendor_id[FIRA_VENDOR_ID_SIZE]);

/**
 * fira_helper_set_session_static_sts_iv() - Sets Static STS IV.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @static_sts_iv: Static STS IV used for vUpper64.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_static_sts_iv(struct fira_context *ctx, uint32_t session_handle,
						uint8_t static_sts_iv[FIRA_STATIC_STS_IV_SIZE]);

/**
 * fira_helper_set_session_vupper64() - Sets vupper64.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @vupper64: vUpper64 used during Static STS ranging.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_vupper64(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t vupper64[FIRA_VUPPER64_SIZE]);

/**
 * fira_helper_set_session_key_rotation() - Enable/disable key rotation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @key_rotation: 0 to disable key rotation, 1 to enable it.
 *   Enable/disable key rotation during Dynamic |NSQM33| or Provisioned STS ranging. If
 *   enable the period will be set with
 *   fira_helper_set_session_key_rotation_rate.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_key_rotation(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t key_rotation);

/**
 * fira_helper_set_session_key_rotation_rate() - Sets key rotation rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @key_rotation_rate: Defines n, with 2^n being the rotation rate of some
 *   keys used during Dynamic |NSQM33| or Provisioned STS Ranging,
 *   n shall be in the range of 0<=n<=15.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_key_rotation_rate(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t key_rotation_rate);

/**
 * fira_helper_set_session_mac_payload_encryption() - Enable or disable
 *   encryption of payload data.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_payload_encryption: Status of mac payload encryption.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_mac_payload_encryption(struct fira_context *ctx,
							 uint32_t session_handle,
							 uint8_t mac_payload_encryption);

/**
 * fira_helper_set_session_report_rssi() - Sets the report rssi.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @report_rssi: Report_rssi false - no report, true report
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_report_rssi(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t report_rssi);

/**
 * fira_helper_bool_to_result_report_config() - get the result report config
 *   bitfield format.
 * @report_tof: True if time of flight must be reported.
 * @report_aoa_azimuth: True if azimuth's angle of arrival must be reported.
 * @report_aoa_elevation: True if elevation's angle of arrival must be reported.
 * @report_aoa_fom: True if aoa figure of merit must be reported.
 *
 * Return: result report config bitfield format.
 */
static inline uint8_t fira_helper_bool_to_result_report_config(bool report_tof,
							       bool report_aoa_azimuth,
							       bool report_aoa_elevation,
							       bool report_aoa_fom)
{
#define __BIT(n) (1ul << (n))
	return (report_tof ? __BIT(0) : 0) | (report_aoa_azimuth ? __BIT(1) : 0) |
	       (report_aoa_elevation ? __BIT(2) : 0) | (report_aoa_fom ? __BIT(3) : 0);
#undef __BIT
}
/**
 * fira_helper_set_session_result_report_config() - Enable/disable time of
 *   flight.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @result_report_config: See &enum fira_result_report_config.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_result_report_config(struct fira_context *ctx,
						       uint32_t session_handle,
						       uint8_t result_report_config);

/**
 * fira_helper_set_session_link_layer_mode() - Sets link layer mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @link_layer_mode: Link layer configuration:
 *   0x00: Bypass mode.
 *   0x01: Connection less. |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_link_layer_mode(struct fira_context *ctx, uint32_t session_handle,
						  uint8_t link_layer_mode);

/**
 * fira_helper_set_session_data_repetition_count() - |NSQM33| Sets data repetition count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_repetition_count: Number of times the current MDSDU shall be sent.
 *   0x00: No repetition.
 *   0x01 - 0xFE: Number of repetitions.
 *  0xFF: Infinite number of times.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_data_repetition_count(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t data_repetition_count);

/**
 * fira_helper_set_session_data_transfer_status_ntf_config() - |NSQM33| Sets config
 *   value for SESSION_DATA_TRANSFER_STATUS_NTF.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @config: Value to set.
 *   0x00: Disable
 *   0x01: Enable
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_data_transfer_status_ntf_config(struct fira_context *ctx,
								  uint32_t session_handle,
								  uint8_t config);

/**
 * fira_helper_set_session_mac_fcs_type() - Sets the CRC type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_fcs_type: CRC type:
 *   0x00: CRC 16.
 *   0x01: CRC 32.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_mac_fcs_type(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t mac_fcs_type);

/**
 * fira_helper_set_session_number_of_sts_segments() - Sets the number of STS
 *   segments.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @number_of_sts_segments: Number of STS segments:
 *   0x00: No STS Segments.
 *   0x01: 1 STS Segment.
 *   0x02: 2 STS Segments (HPRF only). |NSQM33|
 *   0x03: 3 STS Segments (HPRF only). |NSQM33|
 *   0x04: 4 STS Segments (HPRF only). |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_number_of_sts_segments(struct fira_context *ctx,
							 uint32_t session_handle,
							 uint8_t number_of_sts_segments);

/**
 * fira_helper_set_session_phr_data_rate() - Sets the PHR data rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @phr_data_rate: PHR data rate:
 *   0x00: 850 kbps.
 *   0x01: 6.81 Mbps.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_phr_data_rate(struct fira_context *ctx, uint32_t session_handle,
						uint8_t phr_data_rate);

/**
 * fira_helper_set_session_prf_mode() - Sets prf mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @prf_mode: Prf_mode pulse repetition frequency.
 *   0x00: 62.4 MHz PRF. BPRF mode (default)
 *   0x01: 124.8 MHz PRF. HPRF mode. |NSQM33|
 *   0x02: 249.6 MHz PRF. HPRF mode with data rate 27.2 and 31.2 Mbps. |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_prf_mode(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t prf_mode);

/**
 * fira_helper_set_session_cap_size_min() - |NSQM33| Sets the cap size min.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cap_size_min: Cap_size_min - default 5.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_cap_size_min(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t cap_size_min);

/**
 * fira_helper_set_session_cap_size_max() - |NSQM33| Sets the cap size max.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cap_size_max: Cap_size_max.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_cap_size_max(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t cap_size_max);

/**
 * fira_helper_set_session_measurement_sequence() - |NSQM33| Sets the measurement
 *   sequence.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @meas_seq: Sequence of measurement sequence steps,
 *   configures the Antenna Flexibility features.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_measurement_sequence(struct fira_context *ctx,
						       uint32_t session_handle,
						       const struct measurement_sequence *meas_seq);

/**
 * fira_helper_set_session_enable_diagnostics() - Enables diagnostics.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @enable_diagnostics: Enable_diagnostics  0 - no, 1 - yes.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_enable_diagnostics(struct fira_context *ctx,
						     uint32_t session_handle,
						     uint8_t enable_diagnostics);

/**
 * fira_helper_set_session_diags_frame_reports_fields() - Sets the diag frame
 *   fields.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @diags_frame_reports_fields: Select the fields to activate in the frame
 *   reports stored in the diags. Applicable only when @enable_diagnostics
 *   is set to true.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_diags_frame_reports_fields(struct fira_context *ctx,
							     uint32_t session_handle,
							     uint32_t diags_frame_reports_fields);

/**
 * fira_helper_set_session_sts_length() - Sets sts length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sts_length: Values
 *   0x00: 32 symbols
 *   0x01: 64 symbols (default)
 *   0x02: 128 symbols
 *   Values 0x03 to 0xFF: RFU
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_sts_length(struct fira_context *ctx, uint32_t session_handle,
					     uint8_t sts_length);

/**
 * fira_helper_set_session_min_frames_per_rr() - |NSQM33| Sets min_frames_per_rr
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @min_frames_per_rr: Min_frames_per_rr
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_min_frames_per_rr(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t min_frames_per_rr);

/**
 * fira_helper_set_session_mtu_size() - |NSQM33| Sets mtu_size
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mtu_size: Mtu_size, the value shall be restricted to the maximum possible
 *   MTU size of the given frame which includes MHR, Variable IE size and FCS
 *   size.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_mtu_size(struct fira_context *ctx, uint32_t session_handle,
					   uint16_t mtu_size);

/**
 * fira_helper_set_session_inter_frame_interval_ms() - |NSQM33| Sets
 *   inter_frame_interval_ms
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @inter_frame_interval_ms: Inter_frame_interval_ms
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_inter_frame_interval_ms(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint8_t inter_frame_interval_ms);

/**
 * fira_helper_set_session_owr_aoa_measurement_ntf_period() - |NSQM33| Sets OWR for AoA
 *   measurement notification period.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @owr_aoa_measurement_ntf_period: 0 - send on every frame, 1 - send once after
 *   MIN_FRAMES_PER_RR number of AoA measurements are aggregated.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_owr_aoa_measurement_ntf_period(
	struct fira_context *ctx, uint32_t session_handle, uint8_t owr_aoa_measurement_ntf_period);

/**
 * fira_helper_set_session_session_info_ntf_config() - |NSQM33| Sets ntf config.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @session_info_ntf_config: Values;
 *   0x00 = Disable session info notification (ntf)
 *   0x01 = Enable session info notification (default)
 *   0x02 = Enable session info ntf while inside proximity range
 *   0x03 = Enable session info ntf while inside AoA upper and lower bounds
 *   0x04 = Enable session info ntf while inside AoA upper and lower bounds as
 * well as inside proximity range 0x05 = Enable session info ntf only when
 * entering or leaving proximity range 0x06 = Enable session info ntf only when
 * entering or leaving AoA upper and lower bounds 0x07 = Enable session info ntf
 * only when entering or leaving AoA upper and lower bounds as well as entering
 * or leaving proximity range.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_session_info_ntf_config(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint8_t session_info_ntf_config);

/**
 * fira_helper_set_session_near_proximity_config_cm() - |NSQM33| set proximity
 *   near cm.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @near_proximity_config_cm: Range_data_ntf_proximity_near_cm.
 *   prerequisites: Applicable when session_info_ntf_config is set to 0x02, 0x04,
 *   0x05 or 0x07. Should be less than or equal to far_proximity_config
 *   value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_near_proximity_config_cm(struct fira_context *ctx,
							   uint32_t session_handle,
							   uint32_t near_proximity_config_cm);

/**
 * fira_helper_set_session_far_proximity_config_cm() - |NSQM33| Sets
 *   far_proximity_config_cm. prerequisites, Applicable when
 *   session_info_ntf_config is set to 0x02, 0x04, 0x05 or 0x07. Should be greater
 *   than or equal to near_proximity_config value.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @far_proximity_config_cm: Range_data_ntf_proximity_far_cm.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_far_proximity_config_cm(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint32_t far_proximity_config_cm);

/**
 * fira_helper_get_session_device_type() - Gets the device type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @device_type: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_device_type(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t *device_type);

/**
 * fira_helper_get_session_ranging_round_usage() - Gets the ranging round usage.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @ranging_round_usage: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ranging_round_usage(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint8_t *ranging_round_usage);

/**
 * fira_helper_get_session_device_role() - Gets device role.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @device_role: Device_role.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_device_role(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t *device_role);

/**
 * fira_helper_get_session_sts_config() - Gets the sts config.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sts_config: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_sts_config(struct fira_context *ctx, uint32_t session_handle,
					     uint8_t *sts_config);

/**
 * fira_helper_get_session_multi_node_mode() - Gets the multi node mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @multi_node_mode: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_multi_node_mode(struct fira_context *ctx, uint32_t session_handle,
						  uint8_t *multi_node_mode);

/**
 * fira_helper_get_session_short_address() - Gets the short address.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @short_addr: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_short_address(struct fira_context *ctx, uint32_t session_handle,
						uint16_t *short_addr);

/**
 * fira_helper_get_session_destination_short_addresses() - Gets the destination
 *   short addresses.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @n_dest_short_addr: Number of destination short addresses.
 * @dest_short_addr: Array of destination short addresses.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_destination_short_addresses(struct fira_context *ctx,
							      uint32_t session_handle,
							      uint32_t *n_dest_short_addr,
							      uint16_t *dest_short_addr);

/**
 * fira_helper_get_session_time0_ns() - Gets an absolute value of the initiation
 *   time [ns].
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @time0_ns: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_time0_ns(struct fira_context *ctx, uint32_t session_handle,
					   uint64_t *time0_ns);

/**
 * fira_helper_get_session_slot_duration_rstu() - Gets the slot duration rstu.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @slot_duration_rstu: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_slot_duration_rstu(struct fira_context *ctx,
						     uint32_t session_handle,
						     uint32_t *slot_duration_rstu);

/**
 * fira_helper_get_session_round_duration_slots() - Gets the number of round
 *   duration slots
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @round_duration_slots: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_round_duration_slots(struct fira_context *ctx,
						       uint32_t session_handle,
						       uint32_t *round_duration_slots);

/**
 * fira_helper_get_session_block_duration_ms() - Gets the block duration.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @block_duration_ms: Variable to store the value. (Block size in unit of 1200
 *   RSTU (same as ms))
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_block_duration_ms(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint32_t *block_duration_ms);

/**
 * fira_helper_get_session_block_stride_length() - Gets the block stride length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @block_stride_length: Variable to store the number of blocks to stride.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_block_stride_length(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint32_t *block_stride_length);

/**
 * fira_helper_get_session_round_hopping() - Gets the round hopping 0 -
 *   disabled, 1 enabled.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @round_hopping: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_round_hopping(struct fira_context *ctx, uint32_t session_handle,
						uint8_t *round_hopping);

/**
 * fira_helper_get_session_priority() - Gets the priority.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @priority: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_priority(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t *priority);

/**
 * fira_helper_get_session_mac_address_mode() - Gets the MAC addressing mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_address_mode: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_mac_address_mode(struct fira_context *ctx,
						   uint32_t session_handle,
						   uint8_t *mac_address_mode);

/**
 * fira_helper_get_session_ranging_round_control() - Gets the ranging round
 *   control.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @ranging_round_control: Storage variable, where ranging round control is:
 *   - b0: ranging result report phase is disabled (0) or enabled (1)
 *   - b1: Control Message is sent in band (1) or not (0)
 *   - b2: Control Message is sent separately (0) or piggybacked to RIM (1)
 *   - b3-b6: RFUs, must be set to 0.
 *   - b7: MRM is sent from the initiator (0) or from the responder (1)
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ranging_round_control(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t *ranging_round_control);

/**
 * fira_helper_get_session_schedule_mode() - Gets schedule mode parameter.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @schedule_mode: Variable to store the value.
 *   - 0x00 - Contention-based ranging. |NSQM33|
 *   - 0x01 - Time-scheduled ranging.
 *   - 0x02 - Hybrid-based ranging. |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_schedule_mode(struct fira_context *ctx, uint32_t session_handle,
						uint8_t *schedule_mode);

/**
 * fira_helper_get_session_max_number_of_measurements() - Gets the number of
 *   measurements.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @max_number_of_measurements: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_max_number_of_measurements(struct fira_context *ctx,
							     uint32_t session_handle,
							     uint32_t *max_number_of_measurements);

/**
 * fira_helper_get_session_max_rr_retry() - Gets the maximum ranging rounds
 *   retry.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @max_rr_retry: Max_rr_retry. Variable to store the value.
 *   Number of failed ranging round attempts before stopping the session.
 *   The value zero disables the feature.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_max_rr_retry(struct fira_context *ctx, uint32_t session_handle,
					       uint32_t *max_rr_retry);

/**
 * fira_helper_get_session_channel_number() - Gets the channel used in this
 *   session.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @channel_number: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_channel_number(struct fira_context *ctx, uint32_t session_handle,
						 uint8_t *channel_number);

/**
 * fira_helper_get_session_preamble_code_index() - Gets preamble code index.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @preamble_code_index: Variable to store the value, possible values:
 *   - 9-24: BPRF
 *   - 25-32: HPRF
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_preamble_code_index(struct fira_context *ctx,
						      uint32_t session_handle,
						      uint8_t *preamble_code_index);

/**
 * fira_helper_get_session_rframe_config() - Gets the rframe_config.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @rframe_config: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_rframe_config(struct fira_context *ctx, uint32_t session_handle,
						uint8_t *rframe_config);

/**
 * fira_helper_get_session_preamble_duration() - Gets the preamble duration.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @preamble_duration: Variable to store the value. 0x00: 32 symbols |NSQM33| or 0x01: 64
 *   symbols (default)a.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_preamble_duration(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t *preamble_duration);

/**
 * fira_helper_get_session_sfd_id() - Gets sfd_id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sfd_id: Sfd_id. Variable to store the value.
 *   possible values 0 or 2 in BPRF, 1-4 in HPRF |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_sfd_id(struct fira_context *ctx, uint32_t session_handle,
					 uint8_t *sfd_id);

/**
 * fira_helper_get_session_psdu_data_rate() - Gets the psdu data rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @psdu_data_rate: Psdu_data_rate.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_psdu_data_rate(struct fira_context *ctx, uint32_t session_handle,
						 uint8_t *psdu_data_rate);

/**
 * fira_helper_get_session_sub_session_id() - Gets controlee' sub-session id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sub_session_id: Controlee' sub-session id used during Dynamic or Provisioned
 *   STS for Responder Specific Sub-session Key.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_sub_session_id(struct fira_context *ctx, uint32_t session_handle,
						 uint32_t *sub_session_id);

/**
 * fira_helper_get_session_vendor_id() - Gets the Vendor ID.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @vendor_id: Vendor ID.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_vendor_id(struct fira_context *ctx, uint32_t session_handle,
					    uint8_t vendor_id[FIRA_VENDOR_ID_SIZE]);

/**
 * fira_helper_get_session_static_sts_iv() - Gets the Static STS IV.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @static_sts_iv: Static STS IV.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_static_sts_iv(struct fira_context *ctx, uint32_t session_handle,
						uint8_t static_sts_iv[FIRA_STATIC_STS_IV_SIZE]);

/**
 * fira_helper_get_session_vupper64() - Gets the vupper.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @vupper64: vupper64.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_vupper64(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t vupper64[FIRA_VUPPER64_SIZE]);

/**
 * fira_helper_get_session_key_rotation() - Gets the key rotation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @key_rotation: False - no rotation, true rotation
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_key_rotation(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t *key_rotation);

/**
 * fira_helper_get_session_key_rotation_rate() - Gets the key rotation rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @key_rotation_rate: Value to store the variable which.
 *   defines n, with 2^n being the rotation rate of some
 *   keys used during Dynamic |NSQM33| or Provisioned STS Ranging,
 *   n shall be in the range of 0<=n<=15.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_key_rotation_rate(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t *key_rotation_rate);

/**
 * fira_helper_get_session_mac_payload_encryption() - Gets the status of
 *   encryption of payload data.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_payload_encryption: Status of mac payload encryption.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_mac_payload_encryption(struct fira_context *ctx,
							 uint32_t session_handle,
							 uint8_t *mac_payload_encryption);

/**
 * fira_helper_get_session_report_rssi() - Gets rssi report.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @report_rssi: Variable to store: false no report, true report.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_report_rssi(struct fira_context *ctx, uint32_t session_handle,
					      uint8_t *report_rssi);

/**
 * fira_helper_get_session_result_report_config() - Gets the report tof
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @result_report_config: Variable to store: false no report, true report.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_result_report_config(struct fira_context *ctx,
						       uint32_t session_handle,
						       uint8_t *result_report_config);

/**
 * fira_helper_get_session_data_vendor_oui() - gets the data vendor own unique
 *   id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_vendor_oui: Variable to store: false no report, true report.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_data_vendor_oui(struct fira_context *ctx, uint32_t session_handle,
						  uint32_t *data_vendor_oui);

/**
 * fira_helper_get_session_link_layer_mode() - Gets link layer mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @link_layer_mode: Link layer mode.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_link_layer_mode(struct fira_context *ctx, uint32_t session_handle,
						  uint8_t *link_layer_mode);

/**
 * fira_helper_get_session_data_repetition_count() - |NSQM33| Gets data repetition count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_repetition_count: Gets number of times each MDSDU shall be repeated.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_data_repetition_count(struct fira_context *ctx,
							uint32_t session_handle,
							uint8_t *data_repetition_count);

/**
 * fira_helper_get_session_data_transfer_status_ntf_config() - |NSQM33| Gets config
 *   value for SESSION_DATA_TRANSFER_STATUS_NTF.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @config: Gets config value for SESSION_DATA_TRANSFER_STATUS_NTF
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_data_transfer_status_ntf_config(struct fira_context *ctx,
								  uint32_t session_handle,
								  uint8_t *config);

/**
 * fira_helper_get_session_time_base() - |NSQM33| Get session time base configuration.
 *
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @time_base_param: Gets session time base parameter array.
 * Array size equals to FIRA_TIME_BASE_SIZE.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_time_base(struct fira_context *ctx, uint32_t session_handle,
					    uint8_t *time_base_param);

/**
 * fira_helper_get_session_mac_fcs_type() - Gets CRC type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mac_fcs_type: CRC type:
 *  0x00: CRC 16.
 *  0x01: CRC 32.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_mac_fcs_type(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t *mac_fcs_type);

/**
 * fira_helper_get_session_number_of_sts_segments() - Gets the number of STS
 *   segments.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @number_of_sts_segments: Number of STS segments:
 *  0x00: No STS Segments.
 *  0x01: 1 STS Segment.
 *  0x02: 2 STS Segments (HPRF only). |NSQM33|
 *  0x03: 3 STS Segments (HPRF only). |NSQM33|
 *  0x04: 4 STS Segments (HPRF only). |NSQM33|
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_number_of_sts_segments(struct fira_context *ctx,
							 uint32_t session_handle,
							 uint8_t *number_of_sts_segments);

/**
 * fira_helper_get_session_phr_data_rate() - Gets the PHR data rate.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @phr_data_rate: PHR data rate:
 *  0x00: 850 kbps.
 *  0x01: 6.81 Mbps.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_phr_data_rate(struct fira_context *ctx, uint32_t session_handle,
						uint8_t *phr_data_rate);

/**
 * fira_helper_get_session_prf_mode() - gets the prf mode.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @prf_mode: Prf_mode. pulse repetition frequency
 * Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_prf_mode(struct fira_context *ctx, uint32_t session_handle,
					   uint8_t *prf_mode);

/**
 * fira_helper_get_session_cap_size_min() - |NSQM33| Gets cap size min.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cap_size_min: Variable to sore the value
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_cap_size_min(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t *cap_size_min);

/**
 * fira_helper_get_session_cap_size_max() - |NSQM33| Get cap size max.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cap_size_max: Variable to sore the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_cap_size_max(struct fira_context *ctx, uint32_t session_handle,
					       uint8_t *cap_size_max);

/**
 * fira_helper_get_session_enable_diagnostics() - Enables diagnostics.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @enable_diagnostics: Enable_diagnostics  0 - no, 1 - yes.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_enable_diagnostics(struct fira_context *ctx,
						     uint32_t session_handle,
						     uint8_t *enable_diagnostics);

/**
 * fira_helper_get_session_diags_frame_reports_fields() - Gets the diag frame
 *   fields.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @diags_frame_reports_fields: Select the fields to activate in the frame
 *   reports stored in the diags. Applicable only when @enable_diagnostics
 *   is set to true.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_diags_frame_reports_fields(struct fira_context *ctx,
							     uint32_t session_handle,
							     uint32_t *diags_frame_reports_fields);

/**
 * fira_helper_get_session_measurement_sequence() - |NSQM33| Gets the measurement
 *   sequence.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @meas_seq: Variable to store the measurement sequence
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_measurement_sequence(struct fira_context *ctx,
						       uint32_t session_handle,
						       struct measurement_sequence *meas_seq);

/**
 * fira_helper_get_session_sts_length() - gets sts length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @sts_length: Variable to store the value.
 *   0x00: 32 symbols
 *   0x01: 64 symbols (default)
 *   0x02: 128 symbols
 *   Values 0x03 to 0xFF: RFU
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_sts_length(struct fira_context *ctx, uint32_t session_handle,
					     uint8_t *sts_length);

/**
 * fira_helper_get_session_min_frames_per_rr() - |NSQM33| Gets min_frames_per_rr
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @min_frames_per_rr: Min_frames_per_rr
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_min_frames_per_rr(struct fira_context *ctx,
						    uint32_t session_handle,
						    uint8_t *min_frames_per_rr);

/**
 * fira_helper_get_session_mtu_size() - |NSQM33| Gets mtu_size
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @mtu_size: Mtu_size, the value shall be restricted to the maximum possible
 *   MTU size of the given frame which includes MHR, Variable IE size and FCS
 *   size.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_mtu_size(struct fira_context *ctx, uint32_t session_handle,
					   uint16_t *mtu_size);

/**
 * fira_helper_get_session_inter_frame_interval_ms() - |NSQM33| Gets
 *   inter_frame_interval_ms
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @inter_frame_interval_ms: Inter_frame_interval_ms
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_inter_frame_interval_ms(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint8_t *inter_frame_interval_ms);

/**
 * fira_helper_get_session_owr_aoa_measurement_ntf_period() - |NSQM33| Gets OWR for AoA
 *   measurement notification period.
 *
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @owr_aoa_measurement_ntf_period: 0 - send on every frame, 1 - send once after
 *   MIN_FRAMES_PER_RR number of AoA measurements are aggregated.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_owr_aoa_measurement_ntf_period(
	struct fira_context *ctx, uint32_t session_handle, uint8_t *owr_aoa_measurement_ntf_period);

/**
 * fira_helper_get_session_session_info_ntf_config() - |NSQM33| Gets range notification.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @session_info_ntf_config: Variable to store the value:
 *   0x00 = Disable session info notification (ntf).
 *   0x01 = Enable session info notification (default).
 *   0x02 = Enable session info ntf while inside proximity range.
 *   0x03 = Enable session info ntf while inside AoA upper and lower bounds.
 *   0x04 = Enable session info ntf while inside AoA upper and lower bounds as
 *     well as inside proximity range.
 *   0x05 = Enable session info ntf only when entering or leaving proximity range
 *   0x06 = Enable session info ntf only when entering or leaving AoA upper and
 *     lower bounds.
 *   0x07 = Enable session info ntf only when entering or leaving AoA upper and
 *     lower bounds as well as entering or leaving proximity range.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_session_info_ntf_config(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint8_t *session_info_ntf_config);

/**
 * fira_helper_get_session_near_proximity_config_cm() - |NSQM33| Gets ntf
 *   proximity near.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @near_proximity_config_cm: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_near_proximity_config_cm(struct fira_context *ctx,
							   uint32_t session_handle,
							   uint32_t *near_proximity_config_cm);

/**
 * fira_helper_get_session_far_proximity_config_cm() - |NSQM33| Gets ntf
 *   proximity far.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @far_proximity_config_cm: Variable to store the value.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_far_proximity_config_cm(struct fira_context *ctx,
							  uint32_t session_handle,
							  uint32_t *far_proximity_config_cm);

/**
 * fira_helper_set_session_lower_aoa_bound_config_azimuth_2pi() - |NSQM33| Sets
 *   ntf lower bound aoa azimuth.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @lower_aoa_bound_config_azimuth_2pi: Lower bound in rad_2pi
 *   for AOA azimuth, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -180° to +180°. should be less than or equal to
 *   SESSION_INFO_NTF_UPPER_BOUND_AOA_AZIMUTH value. (default = -180).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_lower_aoa_bound_config_azimuth_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	const int32_t lower_aoa_bound_config_azimuth_2pi);

/**
 * fira_helper_get_session_lower_aoa_bound_config_azimuth_2pi() - |NSQM33| Gets
 *   ntf lower bound aoa azimuth.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @lower_aoa_bound_config_azimuth_2pi: Lower bound in rad_2pi
 *   for AOA azimuth, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -180° to +180°. should be less than or equal to
 *   SESSION_INFO_NTF_UPPER_BOUND_AOA_AZIMUTH value. (default = -180).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_lower_aoa_bound_config_azimuth_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	int32_t *lower_aoa_bound_config_azimuth_2pi);

/**
 * fira_helper_set_session_upper_aoa_bound_config_azimuth_2pi() - |NSQM33| Sets
 *   ntf upper bound aoa azimuth.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_upper_bound_aoa_azimuth_2pi: Upper bound in rad_2pi
 *   for AOA azimuth, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -180° to +180°. Should be greater than or equal to
 *   SESSION_INFO_NTF_LOWER_BOUND_AOA_AZIMUTH value. (default = +180).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_upper_aoa_bound_config_azimuth_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	const int32_t data_ntf_upper_bound_aoa_azimuth_2pi);

/**
 * fira_helper_get_session_upper_aoa_bound_config_azimuth_2pi() - |NSQM33| Gets
 *   ntf upper bound aoa azimuth.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_upper_bound_aoa_azimuth_2pi: Upper bound in rad_2pi
 *   for AOA azimuth, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -180° to +180°. Should be greater than or equal to
 *   SESSION_INFO_NTF_LOWER_BOUND_AOA_AZIMUTH value. (default = +180).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_upper_aoa_bound_config_azimuth_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	int32_t *data_ntf_upper_bound_aoa_azimuth_2pi);

/**
 * fira_helper_set_session_lower_aoa_bound_config_elevation_2pi() - |NSQM33| Sets
 *   ntf lower bound aoa elevation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_lower_bound_aoa_elevation_2pi: Lower bound in rad_2pi
 *   for AOA elevation, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -90° to +90°. Should be less than or equal to
 *   SESSION_INFO_NTF_UPPER_BOUND_AOA_ELEVATION value. (default = -90).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_lower_aoa_bound_config_elevation_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	const int16_t data_ntf_lower_bound_aoa_elevation_2pi);

/**
 * fira_helper_get_session_lower_aoa_bound_config_elevation_2pi() - |NSQM33| Gets
 *   ntf lower bound aoa elevation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_lower_bound_aoa_elevation_2pi: Lower bound in rad_2pi
 *   for AOA elevation, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -90° to +90°. Should be less than or equal to
 *   SESSION_INFO_NTF_UPPER_BOUND_AOA_ELEVATION value. (default = -90).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_lower_aoa_bound_config_elevation_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	int16_t *data_ntf_lower_bound_aoa_elevation_2pi);

/**
 * fira_helper_set_session_upper_aoa_bound_config_elevation_2pi() - |NSQM33| Sets
 *   ntf upper bound aoa elevation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_upper_bound_aoa_elevation_2pi: Upper bound in rad_2pi
 *   for AOA elevation, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -90° to +90°. Should be greater than or equal to
 *   SESSION_INFO_NTF_LOWER_BOUND_AOA_ELEVATION value. (default = +90).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_upper_aoa_bound_config_elevation_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	const int16_t data_ntf_upper_bound_aoa_elevation_2pi);

/**
 * fira_helper_get_session_upper_aoa_bound_config_elevation_2pi() - |NSQM33| Gets
 *   ntf upper bound aoa elevation.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @data_ntf_upper_bound_aoa_elevation_2pi: Upper bound in rad_2pi
 *   for AOA elevation, applicable when session_info_ntf_config is set to 0x03,
 *   0x04, 0x06 or 0x07. It is a signed value (rad_2pi). Allowed values range
 *   from -90° to +90°. Should be greater than or equal to
 *   SESSION_INFO_NTF_LOWER_BOUND_AOA_ELEVATION value. (default = +90).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_upper_aoa_bound_config_elevation_2pi(
	struct fira_context *ctx, uint32_t session_handle,
	int16_t *data_ntf_upper_bound_aoa_elevation_2pi);

/**
 * fira_helper_set_session_key() - Sets this key for the session.
 * @ctx: FiRa helper context.
 * @session_handle: Handle of the session to modify.
 * @key: Pointer to the session key
 * @size: Length of the session key, can be 128 or 256 bits.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_key(struct fira_context *ctx, uint32_t session_handle,
				      const void *key, uint8_t size);

/**
 * fira_helper_set_sub_session_key() - Sets key for the current controlee
 *   sub-session.
 * @ctx: FiRa helper context.
 * @session_handle: Handle of the session to modify.
 * @key: Pointer to the sub-session key
 * @size: Length of the sub-session key, can be 128 or 256 bits.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_sub_session_key(struct fira_context *ctx, uint32_t session_handle,
					  const void *key, uint8_t size);

/**
 * fira_helper_set_session_in_band_termination_attempt_count() - Sets in band
 *   termination attemp count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @termination_count: In band termination attempt count to set
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_in_band_termination_attempt_count(struct fira_context *ctx,
								    uint32_t session_handle,
								    uint8_t termination_count);

/**
 * fira_helper_update_dt_anchor_ranging_rounds() - |NSQM33| Configure ranging rounds
 *      for DT-Anchor.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @cmd: Command containing configuration parameters of the ranging rounds.
 * @rsp: Response containing indexes of ranging rounds which failed to be
 *      configured.
 *
 * Return: Error code if the command cannot be executed.
 *
 * The return code will be QERR_SUCCESS if the command is valid, then the
 * result of the command execution will be in the status field of the
 * corresponding response.
 */
enum qerr
fira_helper_update_dt_anchor_ranging_rounds(struct fira_context *ctx, uint32_t session_handle,
					    struct update_dt_anchor_ranging_rounds_cmd *cmd,
					    struct update_dt_anchor_ranging_rounds_rsp *rsp);

/**
 * fira_helper_dt_tag_configure_ranging_rounds() - |NSQM33| Configure ranging rounds
 *   for DT-Tag.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @ranging_rounds_config: Configuration parameters of the ranging rounds.
 * @round_indexes_rsp: Indexes of ranging rounds which failed to be configured.
 *
 * Return: QERR_SUCCESS or error if the command cannot be executed.
 *
 * The return code will be QERR_SUCCESS if the command is valid, then the
 * result of the command execution will be in the status field of the
 * corresponding response.
 */
enum qerr fira_helper_dt_tag_configure_ranging_rounds(
	struct fira_context *ctx, uint32_t session_handle,
	struct dt_tag_ranging_rounds_config *ranging_rounds_config,
	struct dt_tag_round_indexes_rsp *round_indexes_rsp);

/**
 * fira_helper_set_session_ut_tx_interval_ms() - |NSQM33| Set time interval between UTMs.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @interval: Time interval between UTMs (in ms).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_tx_interval_ms(struct fira_context *ctx,
						    uint32_t session_handle, uint32_t interval);

/**
 * fira_helper_set_session_ut_random_window() - |NSQM33| Set duration of random window
 *   in which UTMs can be send.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @window: Random window for UTMs (in ms).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_random_window(struct fira_context *ctx,
						   uint32_t session_handle, uint32_t window);

/**
 * fira_helper_set_session_ut_tx_timestamp_len() - |NSQM33| Set length of timestamp in
 *   UTMs.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of timestamps included in UTMs
 *   not present (0, default), 40-bit timestamp (1), 64-bit timestamp (2).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_tx_timestamp_len(struct fira_context *ctx,
						      uint32_t session_handle, uint8_t len);

/**
 * fira_helper_set_session_ut_device_id_len() - |NSQM33| Set UL-TDoA device id length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of device id
 *   not present (0, default), 16-bit (1), 32-bit timestamp (2), 64-bit
 *   timestamp (3).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_device_id_len(struct fira_context *ctx,
						   uint32_t session_handle, uint8_t len);

/**
 * fira_helper_set_session_ut_device_id() - |NSQM33| Set value of UL-TDoA device id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @id: Device id
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_device_id(struct fira_context *ctx, uint32_t session_handle,
					       uint64_t id);

/**
 * fira_helper_set_session_ut_report_config_interval() - |NSQM33| Set value of UT-Anchor report
 * config time interval.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @interval: Time interval between reports.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_report_config_interval(struct fira_context *ctx,
							    uint32_t session_handle,
							    uint8_t interval);

/**
 * fira_helper_set_session_ut_report_config_count() - |NSQM33| Set value of UT-Anchor report config
 * count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @count: Measurement count between reports.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_report_config_count(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t count);

/**
 * fira_helper_set_session_ut_report_config_event() - |NSQM33| Set value of UT-Anchor report config
 * events type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @event: Rx event to be reported.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_set_session_ut_report_config_event(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t event);

/**
 * fira_helper_get_session_ut_tx_interval_ms() - |NSQM33| Get time interval between UTMs.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @interval: Time interval between UTMs (in ms).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_tx_interval_ms(struct fira_context *ctx,
						    uint32_t session_handle, uint32_t *interval);

/**
 * fira_helper_get_session_ut_random_window() - |NSQM33| Set duration of random window
 *   in which UTMs can be send.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @window: Random window for UTMs (in ms).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_random_window(struct fira_context *ctx,
						   uint32_t session_handle, uint32_t *window);

/**
 * fira_helper_get_session_ut_tx_timestamp_len() - |NSQM33| Get length of timestamp in
 *   UTMs.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of timestamps included in UTMs
 *   not present (0, default), 40-bit timestamp (1), 64-bit timestamp (2).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_tx_timestamp_len(struct fira_context *ctx,
						      uint32_t session_handle, uint8_t *len);

/**
 * fira_helper_get_session_ut_device_id_len() - |NSQM33| Get UL-TDoA device id length.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @len: Length of device id
 *   not present (0, default), 16-bit (1), 32-bit timestamp (2), 64-bit
 *   timestamp (3).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_device_id_len(struct fira_context *ctx,
						   uint32_t session_handle, uint8_t *len);

/**
 * fira_helper_get_session_ut_device_id() - |NSQM33| Get value of UL-TDoA device id.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @id: Device id
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_device_id(struct fira_context *ctx, uint32_t session_handle,
					       uint64_t *id);

/**
 * fira_helper_get_session_ut_report_config_interval() - |NSQM33| Get value of UT-Anchor report
 * config time interval.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @interval: Time interval between reports.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_report_config_interval(struct fira_context *ctx,
							    uint32_t session_handle,
							    uint8_t *interval);

/**
 * fira_helper_get_session_ut_report_config_count() - |NSQM33| Get value of UT-Anchor report config
 * count.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @count: Measurement count between reports.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_report_config_count(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t *count);

/**
 * fira_helper_get_session_ut_report_config_event() - |NSQM33| Get value of UT-Anchor report config
 * events type.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @event: Rx event to be reported.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_get_session_ut_report_config_event(struct fira_context *ctx,
							 uint32_t session_handle, uint8_t *event);

/**
 * fira_helper_session_get_data_size_in_ranging() - |NSQM33| Get maximum data size in
 *   ranging round.
 * @ctx: Fira helper context.
 * @session_handle: Session handle.
 * @size_in_ranging: Data size in ranging.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fira_helper_session_get_data_size_in_ranging(struct fira_context *ctx,
						       uint32_t session_handle,
						       uint16_t *size_in_ranging);

/**
 * fira_helper_set_hus_controller_config() - |NSQM33| Configure phases of a HUS ranging round.
 * @ctx: Fira helper context.
 * @session_handle: Session handle of the targeted HUS controller.
 * @cmd: Command containing configuration parameters of each phase to bind.
 *
 * Return: QERR_SUCCESS or an error otherwise.
 * The corresponding FiRa status code is present if return value > 0.
 */
enum qerr fira_helper_set_hus_controller_config(struct fira_context *ctx, uint32_t session_handle,
						struct fira_hus_controller_config_cmd *cmd);

/**
 * fira_helper_set_hus_controlee_config() - |NSQM33| Configure phases of a HUS ranging round.
 * @ctx: Fira helper context.
 * @session_handle: Session handle of the targeted HUS controller.
 * @cmd: Command containing configuration parameters of each phase to bind.
 *
 * Return: QERR_SUCCESS or an error otherwise.
 * The corresponding FiRa status code is present if return value > 0.
 */
enum qerr fira_helper_set_hus_controlee_config(struct fira_context *ctx, uint32_t session_handle,
					       struct fira_hus_controlee_config_cmd *cmd);

/**
 * fira_helper_set_session_application_data_endpoint() - |NSQM33| Application data endpoint setter.
 * @context: Fira context.
 * @session_handle: Session handle.
 * @application_data_endpoint: Endpoint for non-secure/secure data message exchange.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr fira_helper_set_session_application_data_endpoint(struct fira_context *context,
							    uint32_t session_handle,
							    uint8_t application_data_endpoint);

/**
 * fira_helper_get_session_application_data_endpoint() - |NSQM33| Application data endpoint getter.
 * @context: Fira context.
 * @session_handle: Session handle.
 * @application_data_endpoint: Endpoint for non-secure/secure data message exchange.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr fira_helper_get_session_application_data_endpoint(struct fira_context *context,
							    uint32_t session_handle,
							    uint8_t *application_data_endpoint);

#ifdef __cplusplus
}
#endif
