/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

/*
 * This file is generated automatically, edit with care.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L1_CONFIG_NB_KEYS 259

/* Maximum number of Reference Frames. */
#define REF_FRAME_NUM 8

/* Invalid/unset reference Frame payload data size. */
#define REF_FRAME_PAYLOAD_SZ_INVALID 65535

/* Size of the Reference Frame PHY Configuration. */
#define REF_FRAME_PHY_CFG_SIZE 3

/* Invalid/unset reference Frame PHY Configuration. */
#define REF_FRAME_PHY_CFG_INVALID 16777215

/* Size of one PDoA LUT entry. */
#define PDOA_LUT_ENTRY_SIZE 2

/* Maximum size of a PDoA LUT. */
#define L1_CONFIG_PDOA_LUT_MAX 31

/* Obvious number of fields in a pair. */
#define PAIR_NUM 2

/* Max number of RX Antenna Paths or Pairs per Antenna Set. */
#define ANTENNA_SET_MAX_NB_RX_ANTS 3

/* Maximum antenna port. */
#define ANT_PORT_MAX 4

/* Invalid PA Gain Offset. */
#define PA_GAIN_OFFSET_INVALID -128

/* Value indicating that XTAL Trim is not calibrated. In that case, value from OTP must be used.. */
#define XTAL_TRIM_UNSET 255

/* Maximum TX Power index, for each section of the frame. */
#define TX_POWER_INDEX_MAX 254

/* Number of TX Power index sections per frame. */
#define TX_POWER_INDEX_NB_SECTIONS_PER_FRAME 4

/* Maximum CIR window size is (512 for STS, 1024 for Ipatov). */
#define CIR_WINDOW_MAX_SIZE 1024

/* Invalid/unset Antenna Path. */
#define ANT_PATH_IDX_INVALID -1

/**
 * enum channels - Channel numbers.
 * @CHANNELS_5: Channel 5.
 * @CHANNELS_9: Channel 9.
 * @CHANNELS_NUM: Maximum number of Channel numbers.
 */
enum channels
{
	CHANNELS_5 = 0,
	CHANNELS_9 = 1,
	CHANNELS_NUM
};

/**
 * enum pdoa_axis - PDoA axis.
 * @PDOA_AXIS_X: Axis X.
 * @PDOA_AXIS_Y: Axis Y.
 * @PDOA_AXIS_Z: Axis Z.
 * @PDOA_AXIS_NUM: Maximum number of PDoA axis.
 */
enum pdoa_axis
{
	PDOA_AXIS_X = 0,
	PDOA_AXIS_Y = 1,
	PDOA_AXIS_Z = 2,
	PDOA_AXIS_NUM
};

/**
 * enum wifi_coex_mode - Wi-Fi coexistence modes.
 * @WIFI_COEX_MODE_DISABLED: WiFi coex disabled.
 * @WIFI_COEX_MODE_BLOCK_BASED_ONE_DIR: Block-based mode.
 * @WIFI_COEX_MODE_BLOCK_BASED_TWO_DIR: Block-based mode with additional diagnostics information.
 * @WIFI_COEX_MODE_FRAME_BASED_ONE_DIR: Frame based mode.
 * @WIFI_COEX_MODE_NUM: Maximum number of Wi-Fi coexistence modes.
 */
enum wifi_coex_mode
{
	WIFI_COEX_MODE_DISABLED = 0,
	WIFI_COEX_MODE_BLOCK_BASED_ONE_DIR = 1,
	WIFI_COEX_MODE_BLOCK_BASED_TWO_DIR = 2,
	WIFI_COEX_MODE_FRAME_BASED_ONE_DIR = 3,
	WIFI_COEX_MODE_NUM
};

/**
 * enum wifi_sw_mode - WiFi switch configuration modes.
 * @WIFI_SW_MODE_MANUAL: Activate the wifi switch GPIO via wifi_sw_cfg key.
 * @WIFI_SW_MODE_AUTO: The WIFI switch GPIO will be activated at each tx/rx.
 * @WIFI_SW_MODE_NUM: Maximum number of WiFi switch configuration modes.
 */
enum wifi_sw_mode
{
	WIFI_SW_MODE_MANUAL = 0,
	WIFI_SW_MODE_AUTO = 1,
	WIFI_SW_MODE_NUM
};

/**
 * struct wifi_sw_config - WiFi SPDT (Single pole, double throw switch) settings.
 * @level: GPIO level used in MANUAL mode:
 *  - 0: Low level
 *  - 1: High Level
 * @mode: Control mode:
 *    - 0: "MANUAL mode: the GPIO will be set based on bit `spdt`"
 *    - 1: "AUTOMATIC mode: the GPIO will be set to `high` level at each rx/tx"
 * @time_gap_us: Time gap duration, in µs unit, from 0 to 63 µs, used in AUTO mode.
 */
struct wifi_sw_config
{
	uint8_t level: 1;
	uint8_t mode: 1;
	uint8_t time_gap_us: 6;
};

/**
 * enum alternate_pulse_shape - Alternative pulse shape.
 * @ALTERNATE_PULSE_SHAPE_NONE: No alternate pulse shape, use standard one.
 * @ALTERNATE_PULSE_SHAPE_JAPAN: Block-based mode.
 * @ALTERNATE_PULSE_SHAPE_NUM: Maximum number of Alternative pulse shape.
 */
enum alternate_pulse_shape
{
	ALTERNATE_PULSE_SHAPE_NONE = 0,
	ALTERNATE_PULSE_SHAPE_JAPAN = 1,
	ALTERNATE_PULSE_SHAPE_NUM
};

/**
 * struct channel - Per Channel dependent parameters.
 * @wifi_coex_enabled: WiFi coexistence enable status.
 * @pll_locking_code: PLL Locking code. Legacy, shall be removed in the future..
 */
struct channel
{
	bool wifi_coex_enabled;
	uint8_t pll_locking_code;
};

/**
 * enum ref_frame_prf - Reference Frame Pulse Repetition Frequency.
 * @REF_FRAME_PRF_BPRF: Base pulse repetition frequency.
 * @REF_FRAME_PRF_HPRF: Higher pulse repetition frequency.
 * @REF_FRAME_PRF_NUM: Maximum number of Reference Frame Pulse Repetition Frequency.
 */
enum ref_frame_prf
{
	REF_FRAME_PRF_BPRF = 0,
	REF_FRAME_PRF_HPRF = 1,
	REF_FRAME_PRF_NUM
};

/**
 * enum ref_frame_sfd - Reference Frame Start of Frame Delimiter.
 * @REF_FRAME_SFD_IEEE_4A: IEEE 8-bit ternary.
 * @REF_FRAME_SFD_IEEE_4Z_4: IEEE 4-bit binary (4z).
 * @REF_FRAME_SFD_IEEE_4Z_8: IEEE 8-bit binary (4z).
 * @REF_FRAME_SFD_IEEE_4Z_16: IEEE 16-bit binary (4z).
 * @REF_FRAME_SFD_IEEE_4Z_32: IEEE 32-bit binary (4z).
 * @REF_FRAME_SFD_NUM: Maximum number of Reference Frame Start of Frame Delimiter.
 */
enum ref_frame_sfd
{
	REF_FRAME_SFD_IEEE_4A = 0,
	REF_FRAME_SFD_IEEE_4Z_4 = 1,
	REF_FRAME_SFD_IEEE_4Z_8 = 2,
	REF_FRAME_SFD_IEEE_4Z_16 = 3,
	REF_FRAME_SFD_IEEE_4Z_32 = 4,
	REF_FRAME_SFD_NUM
};

/**
 * enum ref_frame_psr - Reference Frame Preamble symbol repetitions.
 * @REF_FRAME_PSR_16: value for 16 repetitions.
 * @REF_FRAME_PSR_24: value for 24 repetitions.
 * @REF_FRAME_PSR_32: value for 32 repetitions.
 * @REF_FRAME_PSR_48: value for 48 repetitions.
 * @REF_FRAME_PSR_64: value for 64 repetitions.
 * @REF_FRAME_PSR_96: value for 96 repetitions.
 * @REF_FRAME_PSR_128: value for 128 repetitions.
 * @REF_FRAME_PSR_256: value for 256 repetitions.
 * @REF_FRAME_PSR_512: value for 512 repetitions.
 * @REF_FRAME_PSR_1024: value for 1024 repetitions.
 * @REF_FRAME_PSR_2048: value for 2048 repetitions.
 * @REF_FRAME_PSR_4096: value for 4096 repetitions.
 * @REF_FRAME_PSR_NUM: Maximum number of Reference Frame Preamble symbol repetitions.
 */
enum ref_frame_psr
{
	REF_FRAME_PSR_16 = 0,
	REF_FRAME_PSR_24 = 1,
	REF_FRAME_PSR_32 = 2,
	REF_FRAME_PSR_48 = 3,
	REF_FRAME_PSR_64 = 4,
	REF_FRAME_PSR_96 = 5,
	REF_FRAME_PSR_128 = 6,
	REF_FRAME_PSR_256 = 7,
	REF_FRAME_PSR_512 = 8,
	REF_FRAME_PSR_1024 = 9,
	REF_FRAME_PSR_2048 = 10,
	REF_FRAME_PSR_4096 = 11,
	REF_FRAME_PSR_NUM
};

/**
 * enum ref_frame_data_rate - Reference Frame Payload data rate.
 * @REF_FRAME_DATA_RATE_850K: UWB bit rate 850 kbits/s.
 * @REF_FRAME_DATA_RATE_6M8: UWB bit rate 6.8 Mbits/s.
 * @REF_FRAME_DATA_RATE_NO_DATA: No data (SP3 packet format).
 * @REF_FRAME_DATA_RATE_6M8_128: UWB bit rate 6.8 Mbits/s (128 MHz PRF).
 * @REF_FRAME_DATA_RATE_27M_256: UWB bit rate 27 Mbits/s (256 MHz PRF).
 * @REF_FRAME_DATA_RATE_6M8_128_K7: UWB bit rate 6.8 Mbits/s (128 MHz PRF) K=7.
 * @REF_FRAME_DATA_RATE_27M_256_K7: UWB bit rate 27 Mbits/s (256 MHz PRF) K=7.
 * @REF_FRAME_DATA_RATE_54M_256: UWB bit rate 54 Mbits/s (256 MHz PRF).
 * @REF_FRAME_DATA_RATE_108M_256: UWB bit rate 108 Mbits/s (256 MHz PRF).
 * @REF_FRAME_DATA_RATE_NUM: Maximum number of Reference Frame Payload data rate.
 */
enum ref_frame_data_rate
{
	REF_FRAME_DATA_RATE_850K = 0,
	REF_FRAME_DATA_RATE_6M8 = 1,
	REF_FRAME_DATA_RATE_NO_DATA = 2,
	REF_FRAME_DATA_RATE_6M8_128 = 4,
	REF_FRAME_DATA_RATE_27M_256 = 5,
	REF_FRAME_DATA_RATE_6M8_128_K7 = 12,
	REF_FRAME_DATA_RATE_27M_256_K7 = 13,
	REF_FRAME_DATA_RATE_54M_256 = 14,
	REF_FRAME_DATA_RATE_108M_256 = 15,
	REF_FRAME_DATA_RATE_NUM
};

/**
 * enum ref_frame_bprf_phr_rate - Reference Frame BPRF PHR data rate.
 * @REF_FRAME_BPRF_PHR_RATE_STD: Standard PHR data rate.
 * @REF_FRAME_BPRF_PHR_RATE_6M81: 6M81 PHR data rate.
 * @REF_FRAME_BPRF_PHR_RATE_NUM: Maximum number of Reference Frame BPRF PHR data rate.
 */
enum ref_frame_bprf_phr_rate
{
	REF_FRAME_BPRF_PHR_RATE_STD = 0,
	REF_FRAME_BPRF_PHR_RATE_6M81 = 1,
	REF_FRAME_BPRF_PHR_RATE_NUM
};

/**
 * enum ref_frame_seg_len - Reference Frame STS segment length.
 * @REF_FRAME_SEG_LEN_0: No STS segment.
 * @REF_FRAME_SEG_LEN_16: STS length 16 bits.
 * @REF_FRAME_SEG_LEN_32: STS length 32 bits.
 * @REF_FRAME_SEG_LEN_64: STS length 64 bits.
 * @REF_FRAME_SEG_LEN_128: STS length 128 bits.
 * @REF_FRAME_SEG_LEN_256: STS length 256 bits.
 * @REF_FRAME_SEG_LEN_512: STS length 512 bits.
 * @REF_FRAME_SEG_LEN_1024: STS length 1024 bits.
 * @REF_FRAME_SEG_LEN_2048: STS length 2048 bits.
 * @REF_FRAME_SEG_LEN_NUM: Maximum number of Reference Frame STS segment length.
 */
enum ref_frame_seg_len
{
	REF_FRAME_SEG_LEN_0 = 0,
	REF_FRAME_SEG_LEN_16 = 1,
	REF_FRAME_SEG_LEN_32 = 3,
	REF_FRAME_SEG_LEN_64 = 7,
	REF_FRAME_SEG_LEN_128 = 15,
	REF_FRAME_SEG_LEN_256 = 31,
	REF_FRAME_SEG_LEN_512 = 63,
	REF_FRAME_SEG_LEN_1024 = 127,
	REF_FRAME_SEG_LEN_2048 = 255,
	REF_FRAME_SEG_LEN_NUM
};

/**
 * struct ref_frame_phy_cfg - PHY Configuration defining a reference frame used for TX power index selection.
 * @prf: PRF mode, as defined by `enum ref_frame_prf`.
 * @sfd_type: SFD types, as defined by `enum ref_frame_sfd`.
 * @psr: Number of preamble symbol repetitions, as defined by `enum ref_frame_psr`.
 * @data_rate: Payload data rate, as defined by `enum ref_frame_data_rate`.
 * @phr_rate: BPRF PHR rate, as defined by `enum ref_frame_bprf_phr_rate`.
 * @seg_num: Number of STS segments, from 0 to 4.
 * @seg_len: Length of STS segments, as defined by `enum ref_frame_seg_len`.
 */
struct ref_frame_phy_cfg
{
	uint8_t prf: 1;
	uint8_t sfd_type: 3;
	uint8_t psr: 4;
	uint8_t data_rate: 4;
	uint8_t phr_rate: 1;
	uint8_t seg_num: 3;
	uint8_t seg_len: 8;
};

/**
 * struct ref_frame - Reference frame definition.
 * @phy_cfg: PHY configuration.
 * @payload_size: Payload data size.
 */
struct ref_frame
{
	struct ref_frame_phy_cfg phy_cfg;
	uint16_t payload_size;
};

/**
 * struct rx_diag_config - RX Diag parameters.
 * @cir_n_taps: Number of taps, i.e. size of the CIR window, in CIR samples.
 * @cir_fp_tap_offset: Offset relative to first path tap index at which the CIR window starts, in CIR samples.
 */
struct rx_diag_config
{
	uint16_t cir_n_taps;
	uint16_t cir_fp_tap_offset;
};

/**
 * typedef pdoa_lut_entry - PDoA LUT entry.
 */
typedef int16_t pdoa_lut_entry[PDOA_LUT_ENTRY_SIZE];

/**
 * struct pdoa_lut - PDoA Look-Up Table.
 * @data: PDoA LUT data.
 */
struct pdoa_lut
{
	pdoa_lut_entry data[L1_CONFIG_PDOA_LUT_MAX];
};

/**
 * enum rx_segment - RX frame segment used for ToA measurement in debug mode.
 * @RX_SEGMENT_IP_M: Use Ipatov from main receiver.
 * @RX_SEGMENT_STS0_M: Use STS0 from main receiver.
 * @RX_SEGMENT_STS1_M: Use STS1 from main receiver.
 * @RX_SEGMENT_STS2_M: Use STS2 from main receiver.
 * @RX_SEGMENT_STS3_M: Use STS3 from main receiver.
 * @RX_SEGMENT_IP_S: Use Ipatov from secondary receiver.
 * @RX_SEGMENT_STS0_S: Use STS0 from secondary receiver.
 * @RX_SEGMENT_STS1_S: Use STS1 from secondary receiver.
 * @RX_SEGMENT_STS2_S: Use STS2 from secondary receiver.
 * @RX_SEGMENT_STS3_S: Use STS3 from secondary receiver.
 * @RX_SEGMENT_FORCED_DISABLED: Do not force RX segment: the one used is defined by the ToA earliest first path algorithm.
 * @RX_SEGMENT_NUM: Maximum number of RX frame segment used for ToA measurement in debug mode.
 */
enum rx_segment
{
	RX_SEGMENT_IP_M = 0,
	RX_SEGMENT_STS0_M = 8,
	RX_SEGMENT_STS1_M = 16,
	RX_SEGMENT_STS2_M = 24,
	RX_SEGMENT_STS3_M = 32,
	RX_SEGMENT_IP_S = 40,
	RX_SEGMENT_STS0_S = 48,
	RX_SEGMENT_STS1_S = 56,
	RX_SEGMENT_STS2_S = 64,
	RX_SEGMENT_STS3_S = 72,
	RX_SEGMENT_FORCED_DISABLED = 255,
	RX_SEGMENT_NUM
};

/**
 * struct debug_config - Debug specific parameters.
 * @tx_power: TX Power used in debug mode.
 * @pa_enabled: PA value used in debug mode.
 * @pll_bias_trim: PLL bias trim in debug mode.
 * @rx_segment: RX segment used for ToA measurement in debug mode.
 */
struct debug_config
{
	uint32_t tx_power;
	bool pa_enabled;
	uint8_t pll_bias_trim;
	uint8_t rx_segment;
};

/**
 * struct dual_rx_auto_config - Dual RX Main receiver automatic selection parameters.
 * @accum_period: Accumulation duration, in 0.1s unit.
 * @rssi_diff_thres: RSSI difference threshold, in dBm.
 * @error_rate_thres: RX error rate threshold, in %.
 */
struct dual_rx_auto_config
{
	uint8_t accum_period;
	uint8_t rssi_diff_thres;
	uint8_t error_rate_thres;
};

/**
 * struct antenna_channel_ref_frame - Per {Antenna - Channel - Ref Frame} dependent parameters.
 * @tx_power_index: TX power indexes, in -0.25 dB unit.
 * @max_gating_gain: Maximum Gating Gains.
 */
struct antenna_channel_ref_frame
{
	uint8_t tx_power_index[TX_POWER_INDEX_NB_SECTIONS_PER_FRAME];
	uint8_t max_gating_gain[TX_POWER_INDEX_NB_SECTIONS_PER_FRAME];
};

/**
 * struct antenna_channel - Per {Antenna - Channel} dependent parameters.
 * @ref_frame: Reference Frames.
 * @ant_delay: Antenna delay.
 * @tx_bypass_delay_offset: Offset used to compensate TX antenna delay when PA is bypassed.
 * @rx_bypass_delay_offset: Offset used to compensate TX antenna delay when LNA is bypassed.
 * @pa_gain_offset: PA Gain Offset, in -0.25 dB unit.
 * @pg_count: Pulse Generator count.
 * @pg_delay: Pulse Generator delay.
 * @rssi_offset_q3: Rssi Offset using Q3 format.
 */
struct antenna_channel
{
	struct antenna_channel_ref_frame ref_frame[REF_FRAME_NUM];
	uint32_t ant_delay;
	int8_t tx_bypass_delay_offset;
	int8_t rx_bypass_delay_offset;
	int8_t pa_gain_offset;
	uint8_t pg_count;
	uint8_t pg_delay;
	int8_t rssi_offset_q3;
};

/**
 * enum transceiver - Transceiver types.
 * @TRANSCEIVER_TX: Transmitter.
 * @TRANSCEIVER_RXA: Receiver A.
 * @TRANSCEIVER_RXB: Receiver B.
 * @TRANSCEIVER_NUM: Maximum number of Transceiver types.
 */
enum transceiver
{
	TRANSCEIVER_TX = 0,
	TRANSCEIVER_RXA = 1,
	TRANSCEIVER_RXB = 2,
	TRANSCEIVER_NUM
};

/**
 * enum ant_port - Antenna Port.
 * @ANT_PORT_1: Antenna Port 1.
 * @ANT_PORT_2: Antenna Port 2.
 * @ANT_PORT_3: Antenna Port 3.
 * @ANT_PORT_4: Antenna Port 4.
 * @ANT_PORT_NA: Invalid/Undefined Antenna Port.
 * @ANT_PORT_NUM: Maximum number of Antenna Port.
 */
enum ant_port
{
	ANT_PORT_1 = 1,
	ANT_PORT_2 = 2,
	ANT_PORT_3 = 3,
	ANT_PORT_4 = 4,
	ANT_PORT_NA = 15,
	ANT_PORT_NUM
};

/**
 * struct antenna_path - Antenna Path configuration.
 * @ch: Per {Antenna Path - Channel} dependent parameters.
 * @transceiver: Transceiver used.
 * @port: Antenna Port used.
 * @ext_sw_cfg: State of External Switch(es).
 * @lna: LNA (Low Noise Amplifier) state.
 * @pa: PA (Power Amplifier) state.
 */
struct antenna_path
{
	struct antenna_channel ch[CHANNELS_NUM];
	uint8_t transceiver;
	uint8_t port;
	uint8_t ext_sw_cfg;
	bool lna;
	bool pa;
};

/**
 * struct pdoa_config - PDoA measurement configuration.
 * @offset: PDoA offset. Value is in radian, in fixed-point Q11 format..
 * @interval_shift: PDoA interval shift. Value is in radian, in fixed-point Q11 format..
 * @lut_id: PDOA Look-Up table index.
 */
struct pdoa_config
{
	int16_t offset;
	int16_t interval_shift;
	int8_t lut_id;
};

/**
 * struct antenna_pair_channel - Per {Antenna Pair - Channel} dependent parameters.
 * @pdoa: PDoA measurement configuration.
 */
struct antenna_pair_channel
{
	struct pdoa_config pdoa;
};

/**
 * struct antenna_pair - Antenna Pair configuration.
 * @axis: PDoA axis measured.
 * @ant_paths: Index of the two Antenna Paths constituting the pair.
 * @ch: Per {Antenna Pair - Channel} dependent parameters.
 */
struct antenna_pair
{
	uint8_t axis;
	int8_t ant_paths[PAIR_NUM];
	struct antenna_pair_channel ch[CHANNELS_NUM];
};

/**
 * struct tx_power_control - TX power control mode.
 * @frame: Adaptive Power control for frame:
 *   - 0 to disable
 *   - 1 to enable
 * @post_tx: Adaptive Power control for Post TX Tone:
 *   - 0 to disable
 *   - 1 to enable
 * @rfu: Reserved for future usage.
 * @debug: Use debug mode: TX Power and PA state are forced using dedicated debug parameters.
 */
struct tx_power_control
{
	uint8_t frame: 1;
	uint8_t post_tx: 1;
	uint8_t rfu: 5;
	uint8_t debug: 1;
};

/**
 * struct antenna_set - Antenna Set configuration.
 * @rx_ants: List of RX Antennas indexes (Antenna Path or Antenna Pairs) used in the Antenna Set.
 * @tx_ant_path: Index of the Antenna Path used for TX.
 * @nb_rx_ants: Number of RX Antennas (Antenna Path or Antenna Pairs) contained in the Antenna Set.
 * @rx_ants_are_pairs: Indicates if the RX Antennas are Antenna Paths or Antenna Pairs.
 * @tx_power_control: TX power control mode.
 */
struct antenna_set
{
	int8_t rx_ants[ANTENNA_SET_MAX_NB_RX_ANTS];
	int8_t tx_ant_path;
	uint8_t nb_rx_ants;
	bool rx_ants_are_pairs;
	struct tx_power_control tx_power_control;
};

/**
 * enum mac_session_scheduler_id - Session scheduler type identifier in the MAC.
 * @MAC_SESSION_SCHEDULER_ID_STRICT: Strict priority scheduler.
 * @MAC_SESSION_SCHEDULER_ID_FAIR: Weight priority scheduler (also called fair).
 * @MAC_SESSION_SCHEDULER_ID_NUM: Maximum number of Session scheduler type identifier in the MAC.
 */
enum mac_session_scheduler_id
{
	MAC_SESSION_SCHEDULER_ID_STRICT = 0,
	MAC_SESSION_SCHEDULER_ID_FAIR = 1,
	MAC_SESSION_SCHEDULER_ID_NUM
};

/**
 * struct mac_session_scheduler_params - Session scheduler configuration parameters.
 * @id: Scheduler version exercized by FBS region.
 */
struct mac_session_scheduler_params
{
	uint8_t id;
};

/**
 * struct mac_params - MAC configuration parameters.
 * @session_scheduler: Session scheduler configuration parameters.
 */
struct mac_params
{
	struct mac_session_scheduler_params session_scheduler;
};

/**
 * struct experimental - Experimental parameters, to be used in development phase only.
 * @mac: MAC configuration parameters.
 */
struct experimental
{
	struct mac_params mac;
};

#ifdef __cplusplus
}
#endif
