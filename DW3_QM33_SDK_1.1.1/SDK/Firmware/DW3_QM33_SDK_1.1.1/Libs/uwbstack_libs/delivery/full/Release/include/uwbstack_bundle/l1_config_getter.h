/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

/*
 * This file is generated automatically, edit with care.
 */

#pragma once

#include "l1_config_types.h"

#include <qerr.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * l1_config_read_restricted_channels - Read the Restricted channels.
 * @restricted_channels: the returned Restricted channels.
 *
 * The value is a bitfield indicating the restricted channel(s).
 * Setting to 1 a bit will restrict its corresponding channel.
 * For example, to restrict channel 5, value must be (1 << 5).
 * It is used for regulatory concerns.
 */
void l1_config_read_restricted_channels(uint16_t *restricted_channels);

/**
 * l1_config_read_wifi_coex_mode - Read the Wi-Fi coexistence mode.
 * @wifi_coex_mode: the returned Wi-Fi coexistence mode.
 */
void l1_config_read_wifi_coex_mode(uint8_t *wifi_coex_mode);

/**
 * l1_config_read_wifi_coex_time_gap - Read the Delay (in milliseconds) to inform the Wi-Fi stack that there will be a UWB transmission.
 * @wifi_coex_time_gap: the returned Delay (in milliseconds) to inform the Wi-Fi stack that there will be a UWB transmission.
 */
void l1_config_read_wifi_coex_time_gap(uint8_t *wifi_coex_time_gap);

/**
 * l1_config_read_wifi_coex_enabled - Read the WiFi coexistence enable status.
 * @ch_idx: Index of the Channel: 5 or 9.
 * @wifi_coex_enabled: the returned WiFi coexistence enable status.
 *
 * Defines if WiFi coexistence is enabled for the specific channel.
 * Note: if wifi_coex_mode is WIFI_COEX_DISABLED, WiFi coexistence will be disabled for all
 * channels regardless of the ch<>.wifi_coex_enabled value:
 *  - 0: WiFi coexistence disabled for the specific channel.
 *  - 1: WiFi coexistence enabled for the specific channel.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_wifi_coex_enabled(uint8_t ch_idx,bool *wifi_coex_enabled);

/**
 * l1_config_read_pll_locking_code - Read the PLL Locking code. Legacy, shall be removed in the future..
 * @ch_idx: Index of the Channel: 5 or 9.
 * @pll_locking_code: the returned PLL Locking code. Legacy, shall be removed in the future..
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pll_locking_code(uint8_t ch_idx,uint8_t *pll_locking_code);

/**
 * l1_config_read_wifi_coex_max_grant_duration - Read the Max grant duration in ms.
 * @wifi_coex_max_grant_duration: the returned Max grant duration in ms.
 */
void l1_config_read_wifi_coex_max_grant_duration(uint8_t *wifi_coex_max_grant_duration);

/**
 * l1_config_read_wifi_coex_min_inactive_duration - Read the Minimum inactive duration in ms.
 * @wifi_coex_min_inactive_duration: the returned Minimum inactive duration in ms.
 */
void l1_config_read_wifi_coex_min_inactive_duration(uint8_t *wifi_coex_min_inactive_duration);

/**
 * l1_config_read_wifi_sw_cfg - Read the WiFi SPDT (Single pole, double throw switch) configuration.
 * @wifi_sw_cfg: the returned WiFi SPDT (Single pole, double throw switch) configuration.
 */
void l1_config_read_wifi_sw_cfg(struct wifi_sw_config *wifi_sw_cfg);

/**
 * l1_config_read_alternate_pulse_shape - Read the Alternative pulse shape.
 * @alternate_pulse_shape: the returned Alternative pulse shape.
 */
void l1_config_read_alternate_pulse_shape(uint8_t *alternate_pulse_shape);

/**
 * l1_config_read_ref_frame_phy_cfg - Read the PHY configuration.
 * @ref_frame_idx: Index of the Reference Frame, from 0 to 7.
 * @phy_cfg: the returned PHY configuration.
 *
 * The PHY configuration, combined to Payload size, defines a Reference Frame.
 * Reference frames are used to itemize parameters used for TX power index selection.
 * 
 * Only the first Reference Frame is defined by default, and corresponds to **BPRF SET#3**,
 * according to FiRa UWB PHY Technical specification.
 * 
 * Its PHY config is ``{ 0x44, 0x21, 0x07 }``, i.e.:
 * 
 * - ``prf``: BPRF
 * - ``sfd_type``: IEEE_4Z_8
 * - ``psr``: PSR_64
 * - ``payload_rate``: 6M8
 * - ``phr_rate``: Standard rate (0.85)
 * - ``sts_seg_num``: 1 segment
 * - ``sts_seg_len``: STS_LEN_64
 * 
 * **Note** : the first reference frame is mandatory. It can be overwritten. However, it
 * must always remain defined as BPRF, i.e bit[0] ``prf`` must be 0.
 * 
 * Other reference frames are undefined by default.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ref_frame_phy_cfg(uint8_t ref_frame_idx,struct ref_frame_phy_cfg *phy_cfg);

/**
 * l1_config_read_ref_frame_payload_size - Read the Payload data size.
 * @ref_frame_idx: Index of the Reference Frame, from 0 to 7.
 * @payload_size: the returned Payload data size.
 *
 * The Payload data size, combined to PHY configuration, defines a Reference Frame.
 * Reference frames are used to itemize parameters used for TX power index selection.
 * 
 * Only the first Reference Frame is defined by default, and corresponds to **BPRF SET#3**,
 * according to FiRa UWB PHY Technical specification.
 * Its payload size is ``127``.
 * 
 * Other reference frames are undefined by default, i.e. use specific unset value
 * ``REF_FRAME_PAYLOAD_SZ_INVALID``.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ref_frame_payload_size(uint8_t ref_frame_idx,uint16_t *payload_size);

/**
 * l1_config_read_rx_diag_config_cir_n_taps - Read the Number of taps, i.e. size of the CIR window, in CIR samples.
 * @cir_n_taps: the returned Number of taps, i.e. size of the CIR window, in CIR samples.
 */
void l1_config_read_rx_diag_config_cir_n_taps(uint16_t *cir_n_taps);

/**
 * l1_config_read_rx_diag_config_cir_fp_tap_offset - Read the Offset relative to first path tap index at which the CIR window starts, in CIR samples.
 * @cir_fp_tap_offset: the returned Offset relative to first path tap index at which the CIR window starts, in CIR samples.
 */
void l1_config_read_rx_diag_config_cir_fp_tap_offset(uint16_t *cir_fp_tap_offset);

/**
 * l1_config_read_xtal_trim - Read the Crystal trim value.
 * @xtal_trim: the returned Crystal trim value.
 *
 * Valid values should be in range 0x0 to 0x7F (127 steps, ~0.8ppm per step).
 * Initialized to 0xFF to use value from OTP.
 */
void l1_config_read_xtal_trim(uint8_t *xtal_trim);

/**
 * l1_config_read_rf_noise_offset - Read the Noise offset, in dB unit, to apply when calculating the RX SNR.
 * @rf_noise_offset: the returned Noise offset, in dB unit, to apply when calculating the RX SNR.
 */
void l1_config_read_rf_noise_offset(int8_t *rf_noise_offset);

/**
 * l1_config_read_pdoa_lut_data - Read the PDoA LUT data.
 * @pdoa_lut_idx: Index of the PDoA Look-Up Table.
 * @data: the returned PDoA LUT data.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pdoa_lut_data(uint8_t pdoa_lut_idx,const pdoa_lut_entry **data);

/**
 * l1_config_read_debug_tx_power - Read the TX Power used in debug mode.
 * @tx_power: the returned TX Power used in debug mode.
 *
 * Only used when tx_power_control is in debug mode.
 * In debug mode, it indicates the TX power to apply, as four bytes, one for each part of the
 * frame:
 *  - Byte 0: Data
 *  - Byte 1: PHR
 *  - Byte 2: SHR
 *  - Byte 3: STS
 */
void l1_config_read_debug_tx_power(uint32_t *tx_power);

/**
 * l1_config_read_debug_pa_enabled - Read the PA value used in debug mode.
 * @pa_enabled: the returned PA value used in debug mode.
 *
 * Only used when tx_power_control is in debug mode.
 * In debug mode, it indicates if PA must be forced or bypassed
 */
void l1_config_read_debug_pa_enabled(bool *pa_enabled);

/**
 * l1_config_read_debug_pll_bias_trim - Read the PLL bias trim in debug mode.
 * @pll_bias_trim: the returned PLL bias trim in debug mode.
 *
 * Only used when tx_power_control is in debug mode.
 * In debug mode, it indicates the PLL bias trim value configured
 */
void l1_config_read_debug_pll_bias_trim(uint8_t *pll_bias_trim);

/**
 * l1_config_read_debug_rx_segment - Read the RX segment used for ToA measurement in debug mode.
 * @rx_segment: the returned RX segment used for ToA measurement in debug mode.
 *
 * By default, the RX segment used for ToA measurement is automatically chosen by the ToA
 * earliest first path algorithm.
 * In order to add more flexibility, especially needed for the RF engineers during tests, the
 * segment used can be overriden by current parameter.
 * If the received frame does not contain a STS segment while this parameter is set to a STS
 * segment, then the Ipatov segment of the same receiver is used.
 * This is the case for instance if a SP0 frame is sent
 */
void l1_config_read_debug_rx_segment(uint8_t *rx_segment);

/**
 * l1_config_read_dual_rx_auto_accum_period - Read the Accumulation duration, in 0.1s unit.
 * @accum_period: the returned Accumulation duration, in 0.1s unit.
 */
void l1_config_read_dual_rx_auto_accum_period(uint8_t *accum_period);

/**
 * l1_config_read_dual_rx_auto_rssi_diff_thres - Read the RSSI difference threshold, in dBm.
 * @rssi_diff_thres: the returned RSSI difference threshold, in dBm.
 *
 * When (Secondary Receiver RSSI) - (Main Receiver RSSI) is greater than the configured
 * threshold, Main and Secondary receivers are automatically switched
 */
void l1_config_read_dual_rx_auto_rssi_diff_thres(uint8_t *rssi_diff_thres);

/**
 * l1_config_read_dual_rx_auto_error_rate_thres - Read the RX error rate threshold, in %.
 * @error_rate_thres: the returned RX error rate threshold, in %.
 *
 * When the current error rate is greater than that threshold, Master and Slave receivers
 * must be switched
 */
void l1_config_read_dual_rx_auto_error_rate_thres(uint8_t *error_rate_thres);

/**
 * l1_config_read_ip_sts_sanity_thres_q2 - Read the Ipatov-STS time difference threshold.
 * @ip_sts_sanity_thres_q2: the returned Ipatov-STS time difference threshold.
 *
 * If the difference between Ipatov and STS timestamps is above the configured threshold,
 * then an error is returned. Special value 0 allows to disable the check.
 */
void l1_config_read_ip_sts_sanity_thres_q2(uint8_t *ip_sts_sanity_thres_q2);

/**
 * l1_config_read_tx_power_index - Read the TX power indexes, in -0.25 dB unit.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @ref_frame_idx: Index of the Reference Frames.
 * @tx_power_index: the returned TX power indexes, in -0.25 dB unit.
 *
 * TX power indexes are only used when tx_power_control is in normal mode (not debug mode).
 * 
 * The array returned contains 4 TX power indexes, one for each section of the frame:
 * - Byte 0: Power index on the STS section
 * - Byte 1: Power index on the SHR section
 * - Byte 2: Power index on the PHR section
 * - Byte 3: Power index on the DATA section
 * 
 * For each TX power index:
 * - One unit means an attenuation of 0.25 dB compared to maximum emitted power.
 * - An index of zero means no attenuation.
 * - Valid values are from 0 to TX_POWER_INDEX_MAX.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_tx_power_index(uint8_t ant_idx,uint8_t ch_idx,uint8_t ref_frame_idx,const uint8_t **tx_power_index);

/**
 * l1_config_read_max_gating_gain - Read the Maximum Gating Gains.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @ref_frame_idx: Index of the Reference Frames.
 * @max_gating_gain: the returned Maximum Gating Gains.
 *
 * Max gating gains are only used when tx_power_control is in normal mode (not debug mode).
 * 
 * The table returned contains 4 max gating gains, one for each section of the frame:
 * - Byte 0: Power index on the STS section
 * - Byte 1: Power index on the SHR section
 * - Byte 2: Power index on the PHR section
 * - Byte 3: Power index on the DATA section
 * 
 * The max Gating Gain defines the limit applied on the compensation/boosting gain calculated
 * from Reference Frame properties, when Adaptive TX Power feature is enabled. It prevents
 * exceeding peak power (0 dBm in 50 MHz) for shortest SP0/SP1 frame types or to prevent from
 * PSD failure issues due to ePA compression.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_max_gating_gain(uint8_t ant_idx,uint8_t ch_idx,uint8_t ref_frame_idx,const uint8_t **max_gating_gain);

/**
 * l1_config_read_ant_delay - Read the Antenna delay.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @ant_delay: the returned Antenna delay.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_delay(uint8_t ant_idx,uint8_t ch_idx,uint32_t *ant_delay);

/**
 * l1_config_read_tx_bypass_delay_offset - Read the Offset used to compensate TX antenna delay when PA is bypassed.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @tx_bypass_delay_offset: the returned Offset used to compensate TX antenna delay when PA is bypassed.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_tx_bypass_delay_offset(uint8_t ant_idx,uint8_t ch_idx,int8_t *tx_bypass_delay_offset);

/**
 * l1_config_read_rx_bypass_delay_offset - Read the Offset used to compensate TX antenna delay when LNA is bypassed.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @rx_bypass_delay_offset: the returned Offset used to compensate TX antenna delay when LNA is bypassed.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_rx_bypass_delay_offset(uint8_t ant_idx,uint8_t ch_idx,int8_t *rx_bypass_delay_offset);

/**
 * l1_config_read_pa_gain_offset - Read the PA Gain Offset, in -0.25 dB unit.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @pa_gain_offset: the returned PA Gain Offset, in -0.25 dB unit.
 *
 * Offset corresponding to a correction to apply to ensure the TX power linearity at PA
 * switching point. Valid range is -127 to 127, in 0.25 dB steps.
 * Special value INT8_MIN allows to force usage of the OTP value instead of current value.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pa_gain_offset(uint8_t ant_idx,uint8_t ch_idx,int8_t *pa_gain_offset);

/**
 * l1_config_read_pg_count - Read the Pulse Generator count.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @pg_count: the returned Pulse Generator count.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pg_count(uint8_t ant_idx,uint8_t ch_idx,uint8_t *pg_count);

/**
 * l1_config_read_pg_delay - Read the Pulse Generator delay.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @pg_delay: the returned Pulse Generator delay.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pg_delay(uint8_t ant_idx,uint8_t ch_idx,uint8_t *pg_delay);

/**
 * l1_config_read_rssi_offset_q3 - Read the Rssi Offset using Q3 format.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Path - Channel} dependent parameters.
 * @rssi_offset_q3: the returned Rssi Offset using Q3 format.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_rssi_offset_q3(uint8_t ant_idx,uint8_t ch_idx,int8_t *rssi_offset_q3);

/**
 * l1_config_read_ant_path_transceiver - Read the Transceiver used.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @transceiver: the returned Transceiver used.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_path_transceiver(uint8_t ant_idx,uint8_t *transceiver);

/**
 * l1_config_read_ant_path_port - Read the Antenna Port used.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @port: the returned Antenna Port used.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_path_port(uint8_t ant_idx,uint8_t *port);

/**
 * l1_config_read_ant_path_ext_sw_cfg - Read the State of External Switch(es).
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @ext_sw_cfg: the returned State of External Switch(es).
 *
 * Bitfield indicating the status of each external switch.
 *   - A bit set to 1 indicates that the corresponding external switch is ON.
 *   - A bit set to 0 indicates that the corresponding external switch is OFF.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_path_ext_sw_cfg(uint8_t ant_idx,uint8_t *ext_sw_cfg);

/**
 * l1_config_read_ant_path_lna - Read the LNA (Low Noise Amplifier) state.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @lna: the returned LNA (Low Noise Amplifier) state.
 *
 * Boolean value indicating if the LNA is active or bypassed:
 *   - False: Bypass
 *   - True: Active
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_path_lna(uint8_t ant_idx,bool *lna);

/**
 * l1_config_read_ant_path_pa - Read the PA (Power Amplifier) state.
 * @ant_idx: Index of the Antenna Path, from 0 to CONFIG_L1_CONFIG_ANT_NUM - 1.
 * @pa: the returned PA (Power Amplifier) state.
 *
 * Boolean value indicating if the PA is active or bypassed:
 *   - False: Bypass
 *   - True: Active
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_path_pa(uint8_t ant_idx,bool *pa);

/**
 * l1_config_read_ant_pair_axis - Read the PDoA axis measured.
 * @ant_pair_idx: Index of the Antenna Pair, from 0 to CONFIG_L1_CONFIG_ANT_PAIR_NUM - 1.
 * @axis: the returned PDoA axis measured.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_pair_axis(uint8_t ant_pair_idx,uint8_t *axis);

/**
 * l1_config_read_ant_pair_ant_paths - Read the Index of the two Antenna Paths constituting the pair.
 * @ant_pair_idx: Index of the Antenna Pair, from 0 to CONFIG_L1_CONFIG_ANT_PAIR_NUM - 1.
 * @ant_paths: the returned Index of the two Antenna Paths constituting the pair.
 *
 * A negative value indicates that value is undefined
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_pair_ant_paths(uint8_t ant_pair_idx,const int8_t **ant_paths);

/**
 * l1_config_read_pdoa_offset - Read the PDoA offset. Value is in radian, in fixed-point Q11 format..
 * @ant_pair_idx: Index of the Antenna Pair, from 0 to CONFIG_L1_CONFIG_ANT_PAIR_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Pair - Channel} dependent parameters.
 * @offset: the returned PDoA offset. Value is in radian, in fixed-point Q11 format..
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pdoa_offset(uint8_t ant_pair_idx,uint8_t ch_idx,int16_t *offset);

/**
 * l1_config_read_pdoa_interval_shift - Read the PDoA interval shift. Value is in radian, in fixed-point Q11 format..
 * @ant_pair_idx: Index of the Antenna Pair, from 0 to CONFIG_L1_CONFIG_ANT_PAIR_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Pair - Channel} dependent parameters.
 * @interval_shift: the returned PDoA interval shift. Value is in radian, in fixed-point Q11 format..
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pdoa_interval_shift(uint8_t ant_pair_idx,uint8_t ch_idx,int16_t *interval_shift);

/**
 * l1_config_read_pdoa_lut_id - Read the PDOA Look-Up table index.
 * @ant_pair_idx: Index of the Antenna Pair, from 0 to CONFIG_L1_CONFIG_ANT_PAIR_NUM - 1.
 * @ch_idx: Index of the Per {Antenna Pair - Channel} dependent parameters.
 * @lut_id: the returned PDOA Look-Up table index.
 *
 * When negative value is used, default PDoA LUT table is used.
 * That default table depends on the channel.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_pdoa_lut_id(uint8_t ant_pair_idx,uint8_t ch_idx,int8_t *lut_id);

/**
 * l1_config_read_ant_set_rx_ants - Read the List of RX Antennas indexes (Antenna Path or Antenna Pairs) used in the Antenna Set.
 * @ant_set_idx: Index of the Antenna Set, from 0 to CONFIG_L1_CONFIG_ANT_SET_NUM - 1.
 * @rx_ants: the returned List of RX Antennas indexes (Antenna Path or Antenna Pairs) used in the Antenna Set.
 *
 * A negative index indicates that value is undefined
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_set_rx_ants(uint8_t ant_set_idx,const int8_t **rx_ants);

/**
 * l1_config_read_ant_set_tx_ant_path - Read the Index of the Antenna Path used for TX.
 * @ant_set_idx: Index of the Antenna Set, from 0 to CONFIG_L1_CONFIG_ANT_SET_NUM - 1.
 * @tx_ant_path: the returned Index of the Antenna Path used for TX.
 *
 * A negative value indicates that value is undefined
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_set_tx_ant_path(uint8_t ant_set_idx,int8_t *tx_ant_path);

/**
 * l1_config_read_ant_set_nb_rx_ants - Read the Number of RX Antennas (Antenna Path or Antenna Pairs) contained in the Antenna Set.
 * @ant_set_idx: Index of the Antenna Set, from 0 to CONFIG_L1_CONFIG_ANT_SET_NUM - 1.
 * @nb_rx_ants: the returned Number of RX Antennas (Antenna Path or Antenna Pairs) contained in the Antenna Set.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_set_nb_rx_ants(uint8_t ant_set_idx,uint8_t *nb_rx_ants);

/**
 * l1_config_read_ant_set_rx_ants_are_pairs - Read the Indicates if the RX Antennas are Antenna Paths or Antenna Pairs.
 * @ant_set_idx: Index of the Antenna Set, from 0 to CONFIG_L1_CONFIG_ANT_SET_NUM - 1.
 * @rx_ants_are_pairs: the returned Indicates if the RX Antennas are Antenna Paths or Antenna Pairs.
 *
 * If value is True:
 *   - PDoA/AoA will be measured.
 *   - Parameter `rx_ants` contains index(es) of Antenna Pair(s).
 * If value is False:
 *   - PDoA/AoA will NOT be measured.
 *   - Parameter `rx_ants` contains index(es) of Antenna Path(s).
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_ant_set_rx_ants_are_pairs(uint8_t ant_set_idx,bool *rx_ants_are_pairs);

/**
 * l1_config_read_tx_power_control - Read the TX power control mode.
 * @ant_set_idx: Index of the Antenna Set, from 0 to CONFIG_L1_CONFIG_ANT_SET_NUM - 1.
 * @tx_power_control: the returned TX power control mode.
 *
 * Return: QERR_EINVAL if an index given as argument is invalid, else QERR_SUCCESS.
 */
enum qerr l1_config_read_tx_power_control(uint8_t ant_set_idx,struct tx_power_control *tx_power_control);

/**
 * l1_config_read_mac_session_scheduler_id - Read the Scheduler version exercized by FBS region.
 * @id: the returned Scheduler version exercized by FBS region.
 */
void l1_config_read_mac_session_scheduler_id(uint8_t *id);

#ifdef __cplusplus
}
#endif
