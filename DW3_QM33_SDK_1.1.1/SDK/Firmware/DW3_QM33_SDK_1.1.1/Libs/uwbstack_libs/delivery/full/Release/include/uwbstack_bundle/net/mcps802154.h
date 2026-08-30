/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "crypto/aes.h"
#include "net/mac802154.h"
#include "qerr.h"

struct mcps802154_ops;

#ifdef __cplusplus
extern "C" {
#endif

/* Antennas set id to use for transmission by default. */
#define TX_ANT_SET_ID_DEFAULT 0
/* Antennas set id to use for reception by default. */
#define RX_ANT_SET_ID_DEFAULT 0

/** Maximum number of STS segments. */
#define MCPS802154_STS_N_SEGS_MAX 4

/** Maximum number of PDOA segments. */
#define MCPS802154_PDOA_N_SEGS_MAX 6

/** Maximum number of RSSI values. */
#define MCPS802154_RSSIS_N_MAX 2

/** Maximum number of angle of arrival measurements. */
#define MCPS802154_RX_AOA_MEASUREMENTS_MAX 3

/** Speed of light in the air. */
#define SPEED_OF_LIGHT_MM_PER_S (int64_t)299702547000

/** Max value of preamble code index for HPRF. */
#define MCPS802154_LLHW_HPRF_MAX 32
/** Min value of preamble code index for HPRF. */
#define MCPS802154_LLHW_HPRF_MIN 25
/** Max value of preamble code index for BPRF. */
#define MCPS802154_LLHW_BPRF_MAX 24
/** Min value of preamble code index for BPRF. Below is a PRF at 16Mhz from 802.15.4a. */
#define MCPS802154_LLHW_BPRF_MIN 9
/** Nlos value of a good sts reception. */
#define MCPS802154_STS_QUALITY_GOOD 0xff
/** Nlos value of a bad sts reception. */
#define MCPS802154_STS_QUALITY_BAD 0x1
/** Nlos value of an invalid sts reception. */
#define MCPS802154_STS_QUALITY_INVALID 0

/**
 * typedef mcps802154_device_address - IEEE802154 Device Address.
 */
typedef union {
	/* TODO (UWB-7822): Generalise use of this type in the stack. */
	uint16_t ieee802154_short;
	uint64_t ieee802154_extended;
} mcps802154_device_address;

/**
 * struct mcps802154_channel - Channel parameters.
 */
struct mcps802154_channel {
	/**
	 * @page: Channel page used in conjunction with channel to uniquely
	 * identify the channel.
	 */
	int page;
	/**
	 * @number: RF channel number to use for all transmissions and receptions.
	 */
	int number;
	/**
	 * @preamble_code: Preamble code index for HRP UWB. Must be zero for
	 * other PHYs.
	 */
	int preamble_code;
};

/**
 * enum mcps802154_llhw_flags - Low-level hardware without MCPS flags.
 * @MCPS802154_LLHW_RDEV:
 *	Support for ranging (RDEV). TODO: move to &ieee802154_hw.
 * @MCPS802154_LLHW_ERDEV:
 *	Support for enhanced ranging (ERDEV). TODO: move to &ieee802154_hw.
 * @MCPS802154_LLHW_BPRF:
 *	Support for BPRF.
 * @MCPS802154_LLHW_HPRF:
 *	Support for HPRF.
 * @MCPS802154_LLHW_DATA_RATE_850K:
 *	Support for data rate 110 kpbs.
 * @MCPS802154_LLHW_DATA_RATE_6M81:
 *	Support for data rate 6.81 Mpbs.
 * @MCPS802154_LLHW_DATA_RATE_7M80:
 *	Support for data rate 7.8 Mpbs.
 * @MCPS802154_LLHW_DATA_RATE_27M2:
 *	Support for data rate 27.2 Mpbs.
 * @MCPS802154_LLHW_DATA_RATE_31M2:
 *	Support for data rate 31.2 Mpbs.
 * @MCPS802154_LLHW_DATA_RATE_CUSTOM:
 *	Support for custom data rate, When presents extra data rate are
 *	possible to set.
 * @MCPS802154_LLHW_PHR_DATA_RATE_850K:
 *	Support PHR data rate 850 kpbs.
 * @MCPS802154_LLHW_PHR_DATA_RATE_6M81:
 *	Support PHR data rate 6.81 Mpbs.
 * @MCPS802154_LLHW_PRF_4:
 *	Support Pulse Repetition Frequency 4 MHz.
 * @MCPS802154_LLHW_PRF_16:
 *	Support Pulse Repetition Frequency 16 MHz.
 * @MCPS802154_LLHW_PRF_64:
 *	Support Pulse Repetition Frequency 64 MHz.
 * @MCPS802154_LLHW_PRF_125:
 *	Support Pulse Repetition Frequency 125 MHz.
 * @MCPS802154_LLHW_PRF_250:
 *	Support Pulse Repetition Frequency 250 MHz.
 * @MCPS802154_LLHW_PSR_16:
 *	Support 16 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_24:
 *	Support 24 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_32:
 *	Support 32 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_48:
 *	Support 48 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_64:
 *	Support 64 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_96:
 *	Support 96 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_128:
 *	Support 128 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_256:
 *	Support 256 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_512:
 *	Support 512 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_1024:
 *	Support 1024 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_2048:
 *	Support 2048 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_PSR_4096:
 *	Support 4096 symbols in preamble symbol repetitions.
 * @MCPS802154_LLHW_SFD_4A:
 *	Support SFD defined in 4a.
 * @MCPS802154_LLHW_SFD_4Z_4:
 *	Support SFD defined in 4z with length of 4 symbols.
 * @MCPS802154_LLHW_SFD_4Z_8:
 *	Support SFD defined in 4z with length of 8 symbols.
 * @MCPS802154_LLHW_SFD_4Z_16:
 *	Support SFD defined in 4z with length of 16 symbols.
 * @MCPS802154_LLHW_SFD_4Z_32:
 *	Support SFD defined in 4z with length of 32 symbols.
 * @MCPS802154_LLHW_STS_SEGMENT_1:
 *	Support one STS segment.
 * @MCPS802154_LLHW_STS_SEGMENT_2:
 *	Support two STS segments.
 * @MCPS802154_LLHW_STS_SEGMENT_3:
 *	Support three STS segments.
 * @MCPS802154_LLHW_STS_SEGMENT_4:
 *	Support four STS segments.
 * @MCPS802154_LLHW_AOA_AZIMUTH:
 *	Support AOA azimuth [-90°,+90°].
 * @MCPS802154_LLHW_AOA_AZIMUTH_FULL:
 *	Support AOA full azimuth [-180°,+180°].
 * @MCPS802154_LLHW_AOA_ELEVATION:
 *	Support AOA elevation [-90°,+90°].
 * @MCPS802154_LLHW_AOA_FOM:
 *	Support AOA figure of merit.
 */
enum mcps802154_llhw_flags {
	MCPS802154_LLHW_RDEV = BIT(0),
	MCPS802154_LLHW_ERDEV = BIT(1),
	MCPS802154_LLHW_BPRF = BIT(2),
	MCPS802154_LLHW_HPRF = BIT(3),
	MCPS802154_LLHW_DATA_RATE_850K = BIT(4),
	MCPS802154_LLHW_DATA_RATE_6M81 = BIT(5),
	MCPS802154_LLHW_DATA_RATE_7M80 = BIT(6),
	MCPS802154_LLHW_DATA_RATE_27M2 = BIT(7),
	MCPS802154_LLHW_DATA_RATE_31M2 = BIT(8),
	MCPS802154_LLHW_DATA_RATE_CUSTOM = BIT(9),
	MCPS802154_LLHW_PHR_DATA_RATE_850K = BIT(10),
	MCPS802154_LLHW_PHR_DATA_RATE_6M81 = BIT(11),
	MCPS802154_LLHW_PRF_4 = BIT(12),
	MCPS802154_LLHW_PRF_16 = BIT(13),
	MCPS802154_LLHW_PRF_64 = BIT(14),
	MCPS802154_LLHW_PRF_125 = BIT(15),
	MCPS802154_LLHW_PRF_250 = BIT(16),
	MCPS802154_LLHW_PSR_16 = BIT(17),
	MCPS802154_LLHW_PSR_24 = BIT(18),
	MCPS802154_LLHW_PSR_32 = BIT(19),
	MCPS802154_LLHW_PSR_48 = BIT(20),
	MCPS802154_LLHW_PSR_64 = BIT(21),
	MCPS802154_LLHW_PSR_96 = BIT(22),
	MCPS802154_LLHW_PSR_128 = BIT(23),
	MCPS802154_LLHW_PSR_256 = BIT(24),
	MCPS802154_LLHW_PSR_512 = BIT(25),
	MCPS802154_LLHW_PSR_1024 = BIT(26),
	MCPS802154_LLHW_PSR_2048 = BIT(27),
	MCPS802154_LLHW_PSR_4096 = BIT(28),
	MCPS802154_LLHW_SFD_4A = BIT(29),
	MCPS802154_LLHW_SFD_4Z_4 = BIT(30),
	MCPS802154_LLHW_SFD_4Z_8 = BIT(31),
	MCPS802154_LLHW_SFD_4Z_16 = BIT_ULL(32),
	MCPS802154_LLHW_SFD_4Z_32 = BIT_ULL(33),
	MCPS802154_LLHW_STS_SEGMENT_1 = BIT_ULL(34),
	MCPS802154_LLHW_STS_SEGMENT_2 = BIT_ULL(35),
	MCPS802154_LLHW_STS_SEGMENT_3 = BIT_ULL(36),
	MCPS802154_LLHW_STS_SEGMENT_4 = BIT_ULL(37),
	MCPS802154_LLHW_AOA_AZIMUTH = BIT_ULL(38),
	MCPS802154_LLHW_AOA_AZIMUTH_FULL = BIT_ULL(39),
	MCPS802154_LLHW_AOA_ELEVATION = BIT_ULL(40),
	MCPS802154_LLHW_AOA_FOM = BIT_ULL(41),
};

/**
 * struct mcps802154_llhw - Low-level hardware without MCPS.
 *
 * This must be allocated with mcps802154_alloc_llhw(). The low-level driver
 * should then initialize it.
 */
struct mcps802154_llhw {
	/**
	 * @dtu_freq_hz: Inverse of device time unit duration, in Hz.
	 */
	int dtu_freq_hz;
	/**
	 * @symbol_dtu: Symbol duration in device time unit, can change if radio
	 * parameters are changed. Can be set to 1 if device time unit is the
	 * symbol.
	 */
	int symbol_dtu;
	/**
	 * @cca_dtu: CCA duration in device time unit, can change if radio
	 * parameters or CCA modes are changed.
	 */
	int cca_dtu;
	/**
	 * @shr_dtu: Synchronisation header duration in device time unit, can
	 * change if radio parameters are changed. If ranging is supported, this
	 * is the difference between the RMARKER and the first frame symbol.
	 */
	int shr_dtu;
	/**
	 * @dtu_rctu: Duration of one device time unit in ranging counter time
	 * unit (RDEV only).
	 */
	int dtu_rctu;
	/**
	 * @rstu_dtu: Duration of ranging slot time unit in device time unit
	 * (ERDEV only).
	 */
	int rstu_dtu;
	/**
	 * @anticip_dtu: Reasonable delay between reading the current timestamp
	 * and doing an operation in device time unit.
	 */
	int anticip_dtu;
	/**
	 * @idle_dtu: Duration long enough to prefer entering the idle mode
	 * rather than trying to find a valid access.
	 */
	int idle_dtu;
#ifndef CONFIG_HAVE_IEEE802154_PREAMBLE_CODES
	/**
	 * @current_preamble_code: Current value of preamble code index for HRP
	 * UWB. Must be zero for other PHYs.
	 */
	int current_preamble_code;
#endif
	/**
	 * @rx_antenna_pairs: Number of antenna pairs for RX.
	 */
	uint32_t rx_antenna_pairs;
	/**
	 * @tx_antennas: Number of antennas for TX.
	 */
	uint32_t tx_antennas;
	/**
	 * @cir_default_sizeof_tap: Default size of one tap in CIR array.
	 * Needed to get &struct mcps802154_rx_segment_cir, when this value
	 * wasn't specified by higher layers.
	 */
	uint8_t cir_default_sizeof_tap;
	/**
	 * @cir_default_n_taps: Default value for number of taps in one CIR.
	 * Needed to get &struct mcps802154_rx_segment_cir, when this value
	 * wasn't specified by higher layers.
	 */
	uint16_t cir_default_n_taps;
	/**
	 * @cir_default_fp_tap_offset: Default value for first path offset in
	 * CIR array measured in number of taps. Needed to get
	 * &struct mcps802154_rx_segment_cir, when this value wasn't specified
	 * by higher layers.
	 */
	int16_t cir_default_fp_tap_offset;
	/**
	 * @flags: Low-level hardware flags, see &enum mcps802154_llhw_flags.
	 */
	uint64_t flags;
	/**
	 * @hw: Pointer to IEEE802154 hardware exposed by MCPS. The low-level
	 * driver needs to update this and hw->phy according to supported
	 * hardware features and radio parameters. More specifically:
	 *
	 * * &ieee802154_hw.extra_tx_headroom
	 * * in &ieee802154_hw.flags:
	 *
	 *     * IEEE802154_HW_TX_OMIT_CKSUM
	 *     * IEEE802154_HW_RX_OMIT_CKSUM
	 *     * IEEE802154_HW_RX_DROP_BAD_CKSUM
	 *
	 * * &wpan_phy.flags
	 * * &wpan_phy_supported
	 * * &wpan_phy.symbol_duration
	 */
	struct ieee802154_hw *hw;
	/**
	 * @priv: Driver private data.
	 */
	void *priv;
	/**
	 * @rx_ctx_size: size of the context.
	 */
	uint32_t rx_ctx_size;
};

/**
 * enum mcps802154_tx_frame_config_flags - Flags for transmitting a frame.
 * @MCPS802154_TX_FRAME_CONFIG_TIMESTAMP_DTU:
 *	Start transmission at given timestamp in device time unit.
 * @MCPS802154_TX_FRAME_CONFIG_CCA:
 *	Use CCA before transmission using the programmed mode.
 * @MCPS802154_TX_FRAME_CONFIG_RANGING:
 *	Enable precise timestamping for the transmitted frame and its response
 *	(RDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_KEEP_RANGING_CLOCK:
 *	Request that the ranging clock be kept valid after the transmission of
 *	this frame (RDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_RANGING_PDOA:
 *	Enable phase difference of arrival measurement for the response frame
 *	(RDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_SP1:
 *	Enable STS for the transmitted frame and its response, mode 1 (STS after
 *	SFD and before PHR, ERDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_SP2:
 *	Enable STS for the transmitted frame and its response, mode 2 (STS after
 *	the payload, ERDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_SP3:
 *	Enable STS for the transmitted frame and its response, mode 3 (STS after
 *	SFD, no PHR, no payload, ERDEV only).
 * @MCPS802154_TX_FRAME_CONFIG_STS_MODE_MASK:
 *	Mask covering all the STS mode configuration values.
 *
 * If no timestamp flag is given, transmit as soon as possible.
 */
enum mcps802154_tx_frame_config_flags {
	MCPS802154_TX_FRAME_CONFIG_TIMESTAMP_DTU = BIT(0),
	MCPS802154_TX_FRAME_CONFIG_CCA = BIT(1),
	MCPS802154_TX_FRAME_CONFIG_RANGING = BIT(2),
	MCPS802154_TX_FRAME_CONFIG_KEEP_RANGING_CLOCK = BIT(3),
	MCPS802154_TX_FRAME_CONFIG_RANGING_PDOA = BIT(4),
	MCPS802154_TX_FRAME_CONFIG_SP1 = BIT(5),
	MCPS802154_TX_FRAME_CONFIG_SP2 = BIT(6),
	MCPS802154_TX_FRAME_CONFIG_SP3 = BIT(5) | BIT(6),
	MCPS802154_TX_FRAME_CONFIG_STS_MODE_MASK = BIT(5) | BIT(6),
};

/**
 * struct mcps802154_tx_frame_config - Information for transmitting a frame.
 */
struct mcps802154_tx_frame_config {
	/**
	 * @timestamp_dtu: If timestamped, date of transmission start.
	 */
	uint32_t timestamp_dtu;
	/**
	 * @rx_enable_after_tx_dtu: If positive, enable receiver this number of
	 * device time unit after the end of the transmitted frame.
	 */
	int32_t rx_enable_after_tx_dtu;
	/**
	 * @rx_enable_after_tx_timeout_dtu: When receiver is enabled after the
	 * end of the transmitted frame: if negative, no timeout, if zero, use
	 * a default timeout value, else this is the timeout value in device
	 * time unit.
	 */
	int32_t rx_enable_after_tx_timeout_dtu;
	/**
	 * @flags: See &enum mcps802154_tx_frame_config_flags.
	 */
	uint8_t flags;
	/**
	 * @ant_set_id : antenna set index to use for transmit.
	 */
	int8_t ant_set_id;
};

/**
 * enum mcps802154_rx_frame_config_flags - Flags for enabling the receiver.
 * @MCPS802154_RX_FRAME_CONFIG_TIMESTAMP_DTU:
 *	Enable receiver at given timestamp in device time unit.
 * @MCPS802154_RX_FRAME_CONFIG_AACK:
 *	Enable automatic acknowledgment.
 * @MCPS802154_RX_FRAME_CONFIG_RANGING:
 *	Enable precise timestamping for the received frame (RDEV only).
 * @MCPS802154_RX_FRAME_CONFIG_KEEP_RANGING_CLOCK:
 *	Request that the ranging clock be kept valid after the reception of the
 *	frame (RDEV only).
 * @MCPS802154_RX_FRAME_CONFIG_RANGING_PDOA:
 *	Enable phase difference of arrival measurement (RDEV only).
 * @MCPS802154_RX_FRAME_CONFIG_SP1:
 *	Enable STS for the received frame, mode 1 (STS after SFD and before PHR,
 *	ERDEV only).
 * @MCPS802154_RX_FRAME_CONFIG_SP2:
 *	Enable STS for the received frame, mode 2 (STS after the payload, ERDEV
 *	only).
 * @MCPS802154_RX_FRAME_CONFIG_SP3:
 *	Enable STS for the received frame, mode 3 (STS after SFD, no PHR, no
 *	payload, ERDEV only).
 * @MCPS802154_RX_FRAME_CONFIG_STS_MODE_MASK:
 *	Mask covering all the STS mode configuration values.
 * @MCPS802154_RX_FRAME_CONFIG_NO_DATA:
 *	Accept receiving frame without payload (SP3 is by default without data).
 * @MCPS802154_RX_FRAME_CONFIG_SP1_NO_DATA:
 *	Enable STS for the received frame, mode 1 and accept receiving frames
 *	without payload.
 *
 * If no timestamp flag is given, enable receiver as soon as possible.
 */
enum mcps802154_rx_frame_config_flags {
	MCPS802154_RX_FRAME_CONFIG_TIMESTAMP_DTU = BIT(0),
	MCPS802154_RX_FRAME_CONFIG_AACK = BIT(1),
	MCPS802154_RX_FRAME_CONFIG_RANGING = BIT(2),
	MCPS802154_RX_FRAME_CONFIG_KEEP_RANGING_CLOCK = BIT(3),
	MCPS802154_RX_FRAME_CONFIG_RANGING_PDOA = BIT(4),
	MCPS802154_RX_FRAME_CONFIG_SP1 = BIT(5),
	MCPS802154_RX_FRAME_CONFIG_SP2 = BIT(6),
	MCPS802154_RX_FRAME_CONFIG_SP3 = BIT(5) | BIT(6),
	MCPS802154_RX_FRAME_CONFIG_STS_MODE_MASK = BIT(5) | BIT(6),
	MCPS802154_RX_FRAME_CONFIG_NO_DATA = BIT(7),
	MCPS802154_RX_FRAME_CONFIG_SP1_NO_DATA = MCPS802154_RX_FRAME_CONFIG_SP1 |
						 MCPS802154_RX_FRAME_CONFIG_NO_DATA,
};

/**
 * struct mcps802154_rx_frame_config - Information for enabling the receiver.
 */
struct mcps802154_rx_frame_config {
	/**
	 * @timestamp_dtu: If timestamped, date to enable the receiver.
	 */
	uint32_t timestamp_dtu;
	/**
	 * @timeout_dtu: If negative, no timeout, if zero, use a default timeout
	 * value, else this is the timeout value in device time unit.
	 */
	int32_t timeout_dtu;
	/**
	 * @frame_timeout_dtu: If no zero, timeout value for the full frame
	 * reception. This allow limiting the length of accepted frame. The
	 * timeout starts after &mcps802154_rx_frame_config.timeout_dtu value.
	 */
	int32_t frame_timeout_dtu;
	/**
	 * @flags: See &enum mcps802154_rx_frame_config_flags.
	 */
	uint8_t flags;
	/**
	 * @ant_set_id: Antenna set index to use for reception.
	 */
	int8_t ant_set_id;
};

/**
 * enum mcps802154_rx_frame_info_flags - Flags for a received frame.
 * @MCPS802154_RX_FRAME_INFO_TIMESTAMP_DTU:
 *	Set by MCPS to request timestamp in device time unit.
 * @MCPS802154_RX_FRAME_INFO_TIMESTAMP_RCTU:
 *	Set by MCPS to request RMARKER timestamp in ranging counter time unit
 *	(RDEV only).
 * @MCPS802154_RX_FRAME_INFO_LQI:
 *	Set by MCPS to request link quality indicator (LQI).
 * @MCPS802154_RX_FRAME_INFO_RANGING_FOM:
 *	Set by MCPS to request ranging figure of merit (FoM, RDEV only).
 * @MCPS802154_RX_FRAME_INFO_RANGING_OFFSET:
 *	Set by MCPS to request clock characterization data (RDEV only).
 * @MCPS802154_RX_FRAME_INFO_RANGING_PDOA:
 *	Set by MCPS to request phase difference of arrival (RDEV only).
 * @MCPS802154_RX_FRAME_INFO_RANGING_PDOA_FOM:
 *	Set by MCPS to request phase difference of arrival figure of merit (FoM,
 *	RDEV only).
 * @MCPS802154_RX_FRAME_INFO_RANGING_STS_TIMESTAMP_RCTU:
 *	Set by MCPS to request SRMARKERx timestamps for each STS segments in
 *	ranging counter time unit (ERDEV only).
 * @MCPS802154_RX_FRAME_INFO_RANGING_STS_FOM:
 *	Set by MCPS to request STS segments figure of merit measuring the
 *	correlation strength between the received STS segment and the expected
 *	one (FoM, ERDEV only).
 * @MCPS802154_RX_FRAME_INFO_AACK:
 *	Set by low-level driver if an automatic acknowledgment was sent or is
 *	being sent.
 * @MCPS802154_RX_FRAME_INFO_WIFI_ACTIVE:
 *	Set by low-level driver to notify the fact the WiFi was active during
 *	the reception of the frame.
 * @MCPS802154_RX_FRAME_INFO_GRANT_DURATION_EXCEEDED:
 *	Set by low-level driver to notify that maximum grant duration has
 *	been exceeded.
 * @MCPS802154_RX_FRAME_INFO_RANGING:
 *	Set by low-level driver if frame received with ranging bit set.
 *
 * The low-level driver must clear the corresponding flag if the information is
 * not available.
 */
enum mcps802154_rx_frame_info_flags {
	MCPS802154_RX_FRAME_INFO_TIMESTAMP_DTU = BIT(0),
	MCPS802154_RX_FRAME_INFO_TIMESTAMP_RCTU = BIT(1),
	MCPS802154_RX_FRAME_INFO_LQI = BIT(2),
	MCPS802154_RX_FRAME_INFO_RANGING_FOM = BIT(4),
	MCPS802154_RX_FRAME_INFO_RANGING_OFFSET = BIT(5),
	MCPS802154_RX_FRAME_INFO_RANGING_PDOA = BIT(6),
	MCPS802154_RX_FRAME_INFO_RANGING_PDOA_FOM = BIT(7),
	MCPS802154_RX_FRAME_INFO_RANGING_STS_TIMESTAMP_RCTU = BIT(8),
	MCPS802154_RX_FRAME_INFO_RANGING_STS_FOM = BIT(9),
	MCPS802154_RX_FRAME_INFO_AACK = BIT(10),
	MCPS802154_RX_FRAME_INFO_WIFI_ACTIVE = BIT(11),
	MCPS802154_RX_FRAME_INFO_GRANT_DURATION_EXCEEDED = BIT(12),
	MCPS802154_RX_FRAME_INFO_RANGING = BIT(13),
};

/**
 * struct mcps802154_rx_frame_info - Information on a received frame.
 */
struct mcps802154_rx_frame_info {
	/**
	 * @timestamp_dtu: Timestamp of start of frame in device time unit.
	 */
	uint32_t timestamp_dtu;
	/**
	 * @timestamp_rctu: Timestamp of RMARKER in ranging count time unit
	 * (RDEV only).
	 */
	uint64_t timestamp_rctu;
	/**
	 * @frame_duration_dtu: Duration of the whole frame in device time unit
	 * or 0 if unknown.
	 */
	int frame_duration_dtu;
	/**
	 * @ranging_tracking_interval_rctu: Interval on which tracking offset
	 * was measured (RDEV only).
	 */
	int ranging_tracking_interval_rctu;
	/**
	 * @ranging_offset_rctu: Difference between the transmitter and the
	 * receiver clock measure over the tracking interval, if positive, the
	 * transmitter operates at a higher frequency (RDEV only).
	 */
	int ranging_offset_rctu;
	/**
	 * @ranging_sts_timestamp_diffs_rctu: For each SRMARKERx, difference
	 * between the measured timestamp and the expected timestamp relative to
	 * RMARKER in ranging count time unit (ERDEV only). When STS mode is
	 * 1 or 3, SRMARKER0 is the same as RMARKER and difference is always 0.
	 */
	int16_t ranging_sts_timestamp_diffs_rctu[MCPS802154_STS_N_SEGS_MAX + 1];
	/**
	 * @lqi: Link quality indicator (LQI).
	 */
	uint8_t lqi;
	/**
	 * @ranging_sts_fom: Table of figures of merit measuring the correlation
	 * strength between the received STS segment and the expected one (FoM,
	 * ERDEV only). Range is 0 to 255, with 0 being an invalid measure and
	 * 255 being a 100% confidence.
	 */
	uint8_t ranging_sts_fom[MCPS802154_STS_N_SEGS_MAX];
	/**
	 * @n_segment_meas: Available number of segments.
	 *
	 * Usually, low-level driver sets it based on calibration of the antenna
	 * set used for reception.
	 */
	int n_segment_meas;
	/**
	 * @flags: See &enum mcps802154_rx_frame_info_flags.
	 */
	uint16_t flags;
	/**
	 * @sts_quality: Sts quality indicator.
	 * This field is not relevant if the ranging is not successful.
	 * Range is from 0 to 255, with 0 being an invalid measure and 255
	 * being a 100% confidence.
	 */
	uint8_t sts_quality;
};

/**
 * enum mcps802154_rx_measurement_info_flags - Flags for measurements on a
 *	received frame.
 * @MCPS802154_RX_MEASUREMENTS_TIMESTAMP:
 *	Set by MCPS to request time of arrival measurement and associated figure
 *	of merit (RDEV only).
 * @MCPS802154_RX_MEASUREMENTS_CLOCK_OFFSET:
 *	Set by MCPS to request clock characterization data (RDEV only).
 * @MCPS802154_RX_MEASUREMENTS_STS_SEGS_TIMESTAMPS:
 *	Set by MCPS to request time of arrival measurement on STS segments and
 *	associated figure of merit (ERDEV only).
 * @MCPS802154_RX_MEASUREMENTS_SEGMENT_METRICS:
 *	Set by MCPS to request segment metrics values.
 * @MCPS802154_RX_MEASUREMENTS_AOAS:
 *	Set by MCPS to request angle of arrival measurements, time difference of
 *	arrival, phase difference of arrival and associated figure of merit
 *	(RDEV only).
 * @MCPS802154_RX_MEASUREMENTS_CIRS:
 *	Set by MCPS to request CIR taps (RDEV only).
 * @MCPS802154_RX_MEASUREMENTS_CIRS_FIXED_FP_INDEX:
 *	Set by MCPS to request CIR with the fixed fp-index instead of hardware
 *	calculated value.
 * @MCPS802154_RX_MEASUREMENTS_VENDOR0:
 *	Set by MCPS to request first set of vendor specific measurements.
 * @MCPS802154_RX_MEASUREMENTS_VENDOR1:
 *	Set by MCPS to request second set of vendor specific measurements.
 * @MCPS802154_RX_MEASUREMENTS_VENDOR2:
 *	Set by MCPS to request third set of vendor specific measurements.
 * @MCPS802154_RX_MEASUREMENTS_VENDOR3:
 *	Set by MCPS to request fourth set of vendor specific measurements.
 *
 * The low-level driver must clear the corresponding flag if the information is
 * not available.
 */
enum mcps802154_rx_measurement_info_flags {
	/* TODO: fix inconsistency between flag name and enum name. */
	MCPS802154_RX_MEASUREMENTS_TIMESTAMP = BIT(0),
	MCPS802154_RX_MEASUREMENTS_CLOCK_OFFSET = BIT(1),
	MCPS802154_RX_MEASUREMENTS_STS_SEGS_TIMESTAMPS = BIT(2),
	MCPS802154_RX_MEASUREMENTS_SEGMENT_METRICS = BIT(3),
	MCPS802154_RX_MEASUREMENTS_AOAS = BIT(4),
	MCPS802154_RX_MEASUREMENTS_CIRS = BIT(5),
	MCPS802154_RX_MEASUREMENTS_CIRS_FIXED_FP_INDEX = BIT(6),
	MCPS802154_RX_MEASUREMENTS_VENDOR0 = BIT(12),
	MCPS802154_RX_MEASUREMENTS_VENDOR1 = BIT(13),
	MCPS802154_RX_MEASUREMENTS_VENDOR2 = BIT(14),
	MCPS802154_RX_MEASUREMENTS_VENDOR3 = BIT(15),
};

/**
 * enum mcps802154_aoa_type - Type of Angle of Arrival measurement (RDEV only).
 *
 * @MCPS802154_AOA_TYPE_X_AXIS:
 *	AoA measured on X axis.
 * @MCPS802154_AOA_TYPE_Y_AXIS:
 *	AoA measured on Y axis.
 * @MCPS802154_AOA_TYPE_Z_AXIS:
 *	AoA measured on Z axis.
 *
 * The axes are defined on a right hand orthonormal base.
 */
enum mcps802154_aoa_type {
	MCPS802154_AOA_TYPE_X_AXIS = 0,
	MCPS802154_AOA_TYPE_Y_AXIS = 1,
	MCPS802154_AOA_TYPE_Z_AXIS = 2,
} __attribute__((__packed__));

/**
 * struct mcps802154_rx_aoa_measurements - Angle of arrival measurements on a
 * received frame (RDEV only).
 */
struct mcps802154_rx_aoa_measurements {
	/**
	 * @tdoa_rctu: Time difference of arrival, in ranging count time unit.
	 */
	int16_t tdoa_rctu;
	/**
	 * @pdoa_rad_q11: Phase difference of arrival, unit is radian multiplied
	 * by 2048.
	 */
	int16_t pdoa_rad_q11;
	/**
	 * @aoa_rad_q11: Angle of arrival, unit is radian multiplied by 2048.
	 */
	int16_t aoa_rad_q11;
	/**
	 * @fom: Measurements figure of merit (FoM). Range is 0 to 255, with 0
	 * being an invalid measure and 255 being a 100% confidence.
	 */
	uint8_t fom;
	/**
	 * @type: Measurement type.
	 */
	enum mcps802154_aoa_type type;
};

/**
 * struct mcps802154_rx_segment_cir - CIR measurements.
 */
struct mcps802154_rx_segment_cir {
	/**
	 * @receiver_segment: The receiver and the frame segment the CIR has
	 * been computed formated as:
	 *
	 * - b2-b0: Id of the segment with:
	 *
	 *   - b000: IPATOV
	 *   - b001: STS0
	 *   - b010: STS1
	 *   - b011: STS2
	 *   - b100: STS3
	 *   - b101-b111: RFU
	 *
	 * - b3: Master/slave indicator: 1 if receiver is master, 0 if receiver
	 *   is slave.
	 * - b7-b4: Id of the receiver from 0x0 to 0xF
	 */
	uint8_t receiver_segment;
	/**
	 * @fp_tap_offset: The offset of the first path in the CIR.
	 *
	 * Set by MCPS. Updated by low-level driver if value set by MCPS is too
	 * large and hardware is not able to respect the request.
	 *
	 * Note, in case when the value is unknown for MCPS, MCPS should use
	 * default value stored in &mcps802154_llhw.cir_default_fp_tap_offset.
	 */
	int16_t fp_tap_offset;
	/**
	 * @n_taps: The number of taps contained in the CIR.
	 *
	 * Set by MCPS. Updated by low-level driver if value set by MCPS is too
	 * large and hardware is not able to respect the request.
	 *
	 * Note, in case when the value is unknown for MCPS, MCPS should use
	 * default value stored in &mcps802154_llhw.cir_default_n_taps.
	 */
	uint16_t n_taps;
	/**
	 * @sizeof_tap: Size of each tap. The sizes of real and imaginary parts
	 * are equal to each other and are equal to the half of this field.
	 * Should have one of the following value: 4, 6, 8.
	 *
	 * Note, in case when the value is unknown for MCPS, MCPS should use
	 * default value stored in &mcps802154_llhw.cir_default_sizeof_tap.
	 */
	uint8_t sizeof_tap;
	/**
	 * @taps: CIR taps values.
	 *
	 * Each tap is composed of the real and imaginary part which are
	 * signed numbers. Each tap is encoded using the platform endianness
	 * with &mcps802154_rx_segment_cir.sizeof_tap bytes, first half
	 * is the real part, second half is the imaginary part.
	 *
	 * Memory must be allocated by MCPS. Pointer must be set and send to
	 * low-level driver. Low-level driver should fill the memory with CIR
	 * taps. Size of allocated memory must be equal or bigger than
	 * &mcps802154_rx_segment_cir.n_taps * &mcps802154_rx_segment_cir.sizeof_tap.
	 */
	void *taps;
};

/**
 * struct mcps802154_rx_segment_metrics - Segments metrics on a received frame.
 */
struct mcps802154_rx_segment_metrics {
	/**
	 * @rsl_q8: General Received Signal Level as an absolute value,
	 * Q8.8 fixed point format, unit is dBm.
	 *
	 * This field is not attached to a specific Rx path (First path or Peak
	 * path), it is applicable to the entire received segment.
	 */
	uint16_t rsl_q8;
	/**
	 * @noise_value: General noise value
	 *
	 * This field is not attached to a specific Rx path (First path or Peak
	 * path), it is applicable to the entire received segment.
	 */
	int16_t noise_value;
	/**
	 * @fp_rsl_q8: The Received Signal Level of the first path of the
	 * segment, as an absolute value, Q8.8 fixed point format, unit is dBm.
	 *
	 * Set by low-level driver.
	 */
	uint16_t fp_rsl_q8;
	/**
	 * @fp_ns_q6: (Q10.6) Time in nanosecond of the first path index
	 *
	 * Set by low-level driver. Set to zero when
	 * MCPS802154_RX_MEASUREMENTS_CIRS_FIXED_FP_INDEX in
	 * &mcps802154_rx_measurement_info.flags is set.
	 */
	uint16_t fp_ns_q6;
	/**
	 * @pp_rsl_q8: The Received Signal Level of the peak path of the
	 * segment, as an absolute value, Q8.8 fixed point format, unit is dBm.
	 *
	 * Note: On some chips this value may not be available.
	 *
	 * Set by low-level driver.
	 */
	uint16_t pp_rsl_q8;
	/**
	 * @pp_ns_q6: (Q10.6) Time in nanosecond of the peak path index
	 *
	 * Set by low-level driver.
	 */
	uint16_t pp_ns_q6;
	/**
	 * @receiver_segment: The receiver and the frame segment the CIR has
	 * been computed formated as:
	 *
	 * - b2-b0: Id of the segment with:
	 *
	 *   - b000: IPATOV
	 *   - b001: STS0
	 *   - b010: STS1
	 *   - b011: STS2
	 *   - b100: STS3
	 *   - b101-b111: RFU
	 *
	 * - b3: Master/slave indicator: 1 if receiver is master, 0 if receiver
	 *   is slave.
	 * - b7-b4: Id of the receiver from 0x0 to 0xFi
	 *
	 */
	uint8_t receiver_segment;
};

/**
 * struct mcps802154_rx_measurement_info - Measurements on a received frame.
 */
struct mcps802154_rx_measurement_info {
	/**
	 * @n_aoas: Number of angle of arrival measurements.
	 *
	 * Set by low-level driver.
	 */
	int n_aoas;
	/**
	 * @aoas: Angle of arrival measurements, ordered by increasing
	 * measurement type.
	 */
	struct mcps802154_rx_aoa_measurements aoas[MCPS802154_RX_AOA_MEASUREMENTS_MAX];
	/**
	 * @n_segment_meas: Available number of segments.
	 *
	 * Set by MCPS. Updated by low-level driver, when it is not able to
	 * satisfy the request. Note, it is suggested to set the value based on
	 * received &mcps802154_rx_frame_info.n_segment_meas. Otherwise,
	 * low-level driver may not satisfy the request and return default
	 * value.
	 */
	int n_segment_meas;
	/**
	 * @rx_seg_metrics: RX segments metrics for different parts of the
	 * frame. Must be dynamically allocated in MCPS and sent to low-level
	 * driver. Size of allocated memory must be equal or larger than
	 * sizeof(&struct mcps802154_rx_segment_metrics) *
	 * &mcps802154_rx_measurement_info.n_segment_meas.
	 *
	 */
	struct mcps802154_rx_segment_metrics *rx_seg_metrics;
	/**
	 * @cirs: CIR measurements for different parts of the frame.
	 * Must be dynamically allocated in MCPS and sent to low-level driver.
	 * Size of allocated memory must be equal or larger than
	 * sizeof(&struct mcps802154_rx_segment_cir) *
	 * &mcps802154_rx_measurement_info.n_segment_meas.
	 *
	 */
	struct mcps802154_rx_segment_cir *cirs;
	/**
	 * @flags: See &enum mcps802154_rx_measurement_info_flags.
	 */
	int flags;
};

/**
 * enum mcps802154_tx_frame_info_flags - Bitfield values giving additional info
 * 	attached to tx frames handled by the mcps802154.
 *
 * @MCPS802154_TX_FRAME_INFO_WIFI_ACTIVE:
 *	Set by low-level driver to notify the fact the WIFI was active during
 *	the transmission of the frame.
 * @MCPS802154_TX_FRAME_INFO_GRANT_DURATION_EXCEEDED:
 *	Set by low-level driver to notify that maximum grant duration has
 *	been exceeded.
 */
enum mcps802154_tx_frame_info_flags {
	MCPS802154_TX_FRAME_INFO_WIFI_ACTIVE = BIT(0),
	MCPS802154_TX_FRAME_INFO_GRANT_DURATION_EXCEEDED = BIT(1),
};

/**
 * struct mcps802154_tx_frame_info - Information on a received frame.
 */
struct mcps802154_tx_frame_info {
	/**
	 * @flags: See &enum mcps802154_tx_frame_info_flags.
	 *
	 * Set by low-level driver.
	 */
	uint16_t flags;
};

/**
 * struct mcps802154_sts_params - STS parameters for HRP UWB.
 */
struct mcps802154_sts_params {
	/**
	 * @v: Value V used in DRBG for generating the STS. The 32 LSB are the
	 * VCounter which is incremented every 128 generated pulse.
	 */
	uint8_t v[AES_BLOCK_SIZE];
	/**
	 * @key: STS AES key used in DRBG for generating the STS.
	 */
	uint8_t key[AES_KEYSIZE_128];
	/**
	 * @seg_len: Length of STS segments.
	 */
	uint16_t seg_len;
	/**
	 * @n_segs: Number of STS segments.
	 */
	uint8_t n_segs;
};

/**
 * enum mcps802154_prf - Pulse repetition frequency.
 * @MCPS802154_PRF_16:
 *	16 MHz, only used in 4a.
 * @MCPS802154_PRF_64:
 *	64 MHz, used for 4a and 4z BPRF.
 * @MCPS802154_PRF_125:
 *	125 MHz, used for 4z HPRF.
 * @MCPS802154_PRF_250:
 *	250 MHz, used for 4z HPRF.
 */
enum mcps802154_prf {
	MCPS802154_PRF_16 = 16,
	MCPS802154_PRF_64 = 64,
	MCPS802154_PRF_125 = 125,
	MCPS802154_PRF_250 = 250,
};

/**
 * enum mcps802154_psr - Number of preamble symbol repetitions in the SYNC
 * sequence.
 * @MCPS802154_PSR_16:
 *	16 symbols, used in 4a and 4z HPRF.
 * @MCPS802154_PSR_24:
 *	24 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_32:
 *	32 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_48:
 *	48 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_64:
 *	64 symbols, used 4a and 4z BPRF and HPRF.
 * @MCPS802154_PSR_96:
 *	96 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_128:
 *	128 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_256:
 *	256 symbols, used only in 4z HPRF.
 * @MCPS802154_PSR_512:
 *	512 symbols, Non-standard PSR. Requested for radar.
 * @MCPS802154_PSR_1024:
 *	1024 symbols, used only in 4a.
 * @MCPS802154_PSR_2048:
 *	2048 symbols, Non-standard PSR. Requested for radar.
 * @MCPS802154_PSR_4096:
 *	4096 symbols, used only in 4a.
 */
enum mcps802154_psr {
	MCPS802154_PSR_16 = 16,
	MCPS802154_PSR_24 = 24,
	MCPS802154_PSR_32 = 32,
	MCPS802154_PSR_48 = 48,
	MCPS802154_PSR_64 = 64,
	MCPS802154_PSR_96 = 96,
	MCPS802154_PSR_128 = 128,
	MCPS802154_PSR_256 = 256,
	MCPS802154_PSR_512 = 512,
	MCPS802154_PSR_1024 = 1024,
	MCPS802154_PSR_2048 = 2048,
	MCPS802154_PSR_4096 = 4096,
};

/**
 * enum mcps802154_sfd - sfd type selector.
 * @MCPS802154_SFD_4A:
 *	SFD defined in 4a, length of 8 symbols.
 * @MCPS802154_SFD_4Z_4:
 *	SFD defined in 4z, length of 4 symbols.
 * @MCPS802154_SFD_4Z_8:
 *	SFD defined in 4z, length of 8 symbols.
 * @MCPS802154_SFD_4Z_16:
 *	SFD defined in 4z, length of 16 symbols.
 * @MCPS802154_SFD_4Z_32:
 *	SFD defined in 4z, length of 32 symbols.
 */
enum mcps802154_sfd {
	MCPS802154_SFD_4A,
	MCPS802154_SFD_4Z_4,
	MCPS802154_SFD_4Z_8,
	MCPS802154_SFD_4Z_16,
	MCPS802154_SFD_4Z_32,
};

/**
 * enum mcps802154_data_rate - Data rate.
 * @MCPS802154_DATA_RATE_850K:
 *	850 kbps, used only for 4a.
 * @MCPS802154_DATA_RATE_6M81:
 *	6.81 Mbps, used for 4a and 4z (PRF must be 125MHz).
 * @MCPS802154_DATA_RATE_7M80:
 *	7.80 Mbps, only used for 4z (PRF must be 125MHz).
 * @MCPS802154_DATA_RATE_27M2:
 *	27.2 Mbps, used for 4a and 4z (PRF must be 250MHz).
 * @MCPS802154_DATA_RATE_31M2:
 *	31.2 Mbps, used for 4z (PRF must be 250MHz).
 * NOTE: device specific values can be set to use a custom data rate.
 */
enum mcps802154_data_rate {
	MCPS802154_DATA_RATE_850K = 0,
	MCPS802154_DATA_RATE_6M81 = 6,
	MCPS802154_DATA_RATE_7M80 = 7,
	MCPS802154_DATA_RATE_27M2 = 27,
	MCPS802154_DATA_RATE_31M2 = 31,
};

/**
 * enum mcps802154_hrp_uwb_psdu_size - PSDU size in HPRF.
 * @MCPS802154_HRP_UWB_PSDU_SIZE_1023:
 *	1023-bytes PSDU.
 * @MCPS802154_HRP_UWB_PSDU_SIZE_2047:
 *	2047-bytes PSDU.
 * @MCPS802154_HRP_UWB_PSDU_SIZE_4095:
 *	4095-bytes PSDU.
 */
enum mcps802154_hrp_uwb_psdu_size {
	MCPS802154_HRP_UWB_PSDU_SIZE_1023 = 0,
	MCPS802154_HRP_UWB_PSDU_SIZE_2047 = 1,
	MCPS802154_HRP_UWB_PSDU_SIZE_4095 = 2,
};

/**
 * enum mcps802154_fcs - Frame Check Sequence.
 * @MCPS802154_FCS_16:
 *	CRC-16 to add as footer to the payload.
 * @MCPS802154_FCS_32:
 *	CRC-32 to add as footer to the payload.
 */
enum mcps802154_fcs {
	MCPS802154_FCS_16 = 2,
	MCPS802154_FCS_32 = 4,
};

/**
 * struct mcps802154_hrp_uwb_params - Parameters for HRP UWB.
 *
 * Parameters are given directly to driver without checking. The driver needs to
 * check the parameters for supported values, but it can accept non-standard
 * values.
 */
struct mcps802154_hrp_uwb_params {
	/**
	 * @prf: Nominal mean Pulse Repetition Frequency.
	 *
	 * For 4a, one of MCPS802154_PRF_16 or MCPS802154_PRF_64.
	 *
	 * For 4z BPRF, must be MCPS802154_PRF_64.
	 *
	 * For 4z HPRF, one of MCPS802154_PRF_125 or MCPS802154_PRF_250.
	 */
	enum mcps802154_prf prf;
	/**
	 * @psr: Number of preamble symbol repetitions in the SYNC sequence, or
	 * preamble length.
	 *
	 * For 4a, one of 16, 64, 1024 or 4096.
	 *
	 * For 4z BPRF, must be 64.
	 *
	 * For 4z HPRF, one of 16, 24, 32, 48, 64, 96, 128 or 256.
	 */
	enum mcps802154_psr psr;
	/**
	 * @sfd_selector: SFD type selector.
	 *
	 * When MCPS802154_SFD_4A, use short SFD defined in 802.15.4a.
	 *
	 * When MCPS802154_SFD_4Z_*, use SFD defined in 802.15.4z, with length
	 * 4, 8, 16 or 32.
	 *
	 * For 4a, must be MCPS802154_SFD_4A.
	 *
	 * For 4z BPRF, one of MCPS802154_SFD_4A or MCPS802154_SFD_4Z_8.
	 *
	 * For 4z HPRF, one of MCPS802154_SFD_4Z_{4,8,16,32}.
	 */
	enum mcps802154_sfd sfd_selector;
	/**
	 * @data_rate: Data rate.
	 *
	 * For 4a, one of 850 kbps, 6.81 Mbps or 27.2 Mbps.
	 *
	 * For 4z BPRF, must be 6.81 Mbps.
	 *
	 * For 4z HPRF at 125 MHz, use 6.81 Mbps or 7.8 Mbps.
	 *
	 * For 4z HPRF at 250 MHz, use 27.2 Mbps or 31.2 Mbps.
	 */
	uint8_t data_rate;
	/**
	 * @phr_hi_rate: Use high PHR data rate, for 4z BPRF only.
	 *
	 * For 4a and 4z HPRF, this parameter is ignored.
	 *
	 * For 4z BPRF, when enabled use 6.81 Mbps, otherwise use 850 kbps.
	 */
	bool phr_hi_rate;
	/**
	 * @psdu_size: PSDU size in HPRF.
	 */
	enum mcps802154_hrp_uwb_psdu_size psdu_size;
};

/**
 * struct mcps802154_rmarker_info - Parameters for RMARKER computation.
 */
struct mcps802154_rmarker_info {
	/**
	 * @hrp_uwb_params: HRP UWB parameters.
	 */
	const struct mcps802154_hrp_uwb_params *hrp_uwb_params;
	/**
	 * @channel_params: Channel parameters.
	 */
	const struct mcps802154_channel *channel_params;
	/**
	 * @tx_timestamp_dtu: Tx timestamp in device time unit.
	 */
	uint32_t tx_timestamp_dtu;
	/**
	 * @tx_buffer_len: Length of the buffer to be transmitted, 0 if none.
	 */
	uint16_t tx_buffer_len;
	/**
	 * @ant_set_id: Antenna set index for tx.
	 */
	int8_t ant_set_id;
	/**
	 * @compensate_rffe_delay: True if the delay introduce by the RFFE needs to be compensated.
	 */
	bool compensate_rffe_delay;
};

/**
 * enum mcps802154_antenna_caps - Antenna set capabilities
 * @MCPS802154_AOA_X_AXIS:
 *	Antenna can report azimuth
 * @MCPS802154_AOA_Y_AXIS:
 *	Antenna can report elevation
 */
enum mcps802154_antenna_caps {
	MCPS802154_AOA_X_AXIS = BIT(0),
	MCPS802154_AOA_Y_AXIS = BIT(1),
};

/**
 * enum mcps802154_power_state - Power states
 * @MCPS802154_PWR_STATE_OFF:
 *	Power off state.
 * @MCPS802154_PWR_STATE_SLEEP:
 *	Deep sleep state.
 * @MCPS802154_PWR_STATE_IDLE:
 *	Idle state, ready to transmit or receive.
 * @MCPS802154_PWR_STATE_RX:
 *	Receive state.
 * @MCPS802154_PWR_STATE_TX:
 *	Transmit state.
 * @MCPS802154_PWR_STATE_MAX:
 *	Total power states count.
 */
enum mcps802154_power_state {
	MCPS802154_PWR_STATE_OFF,
	MCPS802154_PWR_STATE_SLEEP,
	MCPS802154_PWR_STATE_IDLE,
	MCPS802154_PWR_STATE_RX,
	MCPS802154_PWR_STATE_TX,
	MCPS802154_PWR_STATE_MAX
};

/**
 * struct mcps802154_power_state_stats - Statistics for a power state.
 * @dur_ms: Duration in this power state in milliseconds.
 * @count: Count of transitions in this power state.
 */
struct mcps802154_power_state_stats {
	uint32_t dur_ms;
	uint32_t count;
};

/**
 * struct mcps802154_power_stats - Global power statistics.
 * @power_state_stats: Array of power statistics for each power state.
 * @interrupts: Hardware interrupts count on the device.
 */
struct mcps802154_power_stats {
	struct mcps802154_power_state_stats power_state_stats[MCPS802154_PWR_STATE_MAX];
	uint64_t interrupts;
};

/**
 * struct mcps802154_uwb_device_stats - Chip statistics.
 * @temperature_hundredth_celsius: Temperature in hundredth of degree Celsius.
 */
struct mcps802154_uwb_device_stats {
	int16_t temperature_hundredth_celsius;
};

/**
 * enum mcps802154_rx_error_type - Type of reception errors.
 * @MCPS802154_RX_ERROR_NONE:
 *	RX successful.
 * @MCPS802154_RX_ERROR_TIMEOUT:
 *	RX timeout.
 * @MCPS802154_RX_ERROR_BAD_CKSUM:
 *	Checksum is not correct.
 * @MCPS802154_RX_ERROR_UNCORRECTABLE:
 *	During reception, the error correction code detected an uncorrectable
 *	error.
 * @MCPS802154_RX_ERROR_FILTERED:
 *	A received frame was rejected due to frame filter.
 * @MCPS802154_RX_ERROR_SFD_TIMEOUT:
 *	A preamble has been detected but without SFD.
 * @MCPS802154_RX_ERROR_OTHER:
 *	Other error, frame reception is aborted.
 * @MCPS802154_RX_ERROR_PHR_DECODE:
 *	the preamble and SFD have been detected but without PHR.
 */
enum mcps802154_rx_error_type {
	MCPS802154_RX_ERROR_NONE = 0,
	MCPS802154_RX_ERROR_TIMEOUT = 1,
	MCPS802154_RX_ERROR_BAD_CKSUM = 2,
	MCPS802154_RX_ERROR_UNCORRECTABLE = 3,
	MCPS802154_RX_ERROR_FILTERED = 4,
	MCPS802154_RX_ERROR_SFD_TIMEOUT = 5,
	MCPS802154_RX_ERROR_OTHER = 6,
	MCPS802154_RX_ERROR_PHR_DECODE = 7,
};

/**
 * struct mcps802154_device_info - Device information.
 * @lot_id: Lot ID.
 * @dev_id: Device ID.
 * @part_id: Part ID.
 */
struct mcps802154_device_info {
	uint64_t lot_id;
	uint32_t dev_id;
	uint32_t part_id;
};

/**
 * mcps802154_alloc_llhw() - Allocate a new low-level hardware device.
 * @priv_data_len: Length of private data.
 * @ops: Callbacks for this device.
 *
 * Return: A pointer to the new low-level hardware device, or %NULL on error.
 */
struct mcps802154_llhw *mcps802154_alloc_llhw(size_t priv_data_len,
					      const struct mcps802154_ops *ops);

/**
 * mcps802154_free_llhw() - Free low-level hardware descriptor.
 * @llhw: Low-level device pointer.
 *
 * You must call mcps802154_unregister_hw() before calling this function.
 */
void mcps802154_free_llhw(struct mcps802154_llhw *llhw);

/**
 * mcps802154_register_llhw() - Register low-level hardware device.
 * @llhw: Low-level device pointer.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_register_llhw(struct mcps802154_llhw *llhw);

/**
 * mcps802154_unregister_llhw() - Unregister low-level hardware device.
 * @llhw: Low-level device pointer.
 */
void mcps802154_unregister_llhw(struct mcps802154_llhw *llhw);

/**
 * mcps802154_is_driver_started() - Check the driver state.
 * @llhw: Low-level device pointer.
 *
 * Return: True is the driver is started or false.
 */
bool mcps802154_is_driver_started(struct mcps802154_llhw *llhw);

/**
 * mcps802154_rx_frame() - Signal a frame reception.
 * @llhw: Low-level device this frame came in on.
 *
 * The MCPS will call the &mcps802154_ops.rx_get_frame() handler to retrieve
 * frame.
 */
void mcps802154_rx_frame(struct mcps802154_llhw *llhw);

/**
 * mcps802154_rx_timeout() - Signal a reception timeout.
 * @llhw: Low-level device pointer.
 */
void mcps802154_rx_timeout(struct mcps802154_llhw *llhw);

/**
 * mcps802154_rx_error() - Signal a reception error.
 * @llhw: Low-level device pointer.
 * @error: Type of detected error.
 *
 * In case of filtered frame, the MCPS can call the
 * &mcps802154_ops.rx_get_error_frame() handler to retrieve frame information.
 */
void mcps802154_rx_error(struct mcps802154_llhw *llhw, enum mcps802154_rx_error_type error);

/**
 * mcps802154_tx_done() - Signal the end of an MCPS transmission.
 * @llhw: Low-level device pointer.
 */
void mcps802154_tx_done(struct mcps802154_llhw *llhw);

/**
 * mcps802154_broken() - Signal an unrecoverable error, device needs to be
 * reset.
 * @llhw: Low-level device pointer.
 */
void mcps802154_broken(struct mcps802154_llhw *llhw);

/**
 * mcps802154_timer_expired() - Signal that a programmed timer expired.
 * @llhw: Low-level device pointer.
 *
 * To be called before the timestamp given to &mcps802154_ops.idle() callback.
 */
void mcps802154_timer_expired(struct mcps802154_llhw *llhw);

/**
 * mcps802154_se_done() - Signals that key manager request is finished.
 * @llhw: Low-level device pointer.
 * @id: Identifier of the requested key.
 * @status: The status code of the request.
 */
void mcps802154_se_done(struct mcps802154_llhw *llhw, uint32_t id, enum qerr status);

/**
 * mcps802154_check_hrp_uwb_params() - Check that the HRP UWB params are
 * supported.
 * @llhw: Low-level device pointer.
 * @hrp_uwb_params: HRP UWB parameters.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_hrp_uwb_params(struct mcps802154_llhw *llhw,
					  const struct mcps802154_hrp_uwb_params *hrp_uwb_params);

/**
 * mcps802154_check_channel_number() - Check that the channel is supported by
 * the hardware.
 * @llhw: Low-level device pointer.
 * @number: Channel number.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_channel_number(struct mcps802154_llhw *llhw, int number);

/**
 * mcps802154_check_preamble_code_index() - Check that the preamble code index
 * is supported by the hardware.
 * @llhw: Low-level device pointer.
 * @preamble_code_index: preamble code index.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_preamble_code_index(struct mcps802154_llhw *llhw,
					       int preamble_code_index);

/**
 * mcps802154_check_prf_mode() - Check that the prf mode is supported by the
 * hardware.
 * @llhw: Low-level device pointer.
 * @prf_mode: PRF mode.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_prf_mode(struct mcps802154_llhw *llhw, int prf_mode);

/**
 * mcps802154_check_sfd_id() - Check that the Start of Frame Delimiter is
 * supported by the hardware as described in IEEE-802.11.4z 15.2.6.3.
 * @llhw: Low-level device pointer.
 * @sfd_id: SFD Identifier.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_sfd_id(struct mcps802154_llhw *llhw, int sfd_id);

/**
 * mcps802154_check_phr_data_rate() - Check that the phr data rate is supported by the hardware.
 * @llhw: Low-level device pointer.
 * @phr_data_rate: PHR data rate.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_check_phr_data_rate(struct mcps802154_llhw *llhw, int phr_data_rate);

/**
 * is_before_dtu() - Check if timestamp A is before timestamp B.
 * @a_dtu: A timestamp in device time unit.
 * @b_dtu: B timestamp in device time unit.
 *
 * Return: true if A timestamp is before B timestamp.
 */
static inline bool is_before_dtu(uint32_t a_dtu, uint32_t b_dtu)
{
	return (int32_t)(a_dtu - b_dtu) < 0;
}

/**
 * mcps802154_rctu_to_mm() - Convert time (rctu) to distance (mm).
 * @llhw: Low-level device pointer.
 * @time_rctu: duration to be converted into length.
 *
 * Return: distance travelled by light in time_rctu (mm).
 */
static inline int32_t mcps802154_rctu_to_mm(const struct mcps802154_llhw *llhw, int64_t time_rctu)
{
	const int64_t rctu_freq_hz = (int64_t)llhw->dtu_freq_hz * llhw->dtu_rctu;
	int64_t temp = SPEED_OF_LIGHT_MM_PER_S * time_rctu;

	temp += ((temp > 0) ? rctu_freq_hz : -rctu_freq_hz) / 2;
	return temp / rctu_freq_hz;
}

/**
 * mcps802154_mm_to_rctu() - Convert distance (mm) to time (rctu).
 * @llhw: Low-level device pointer.
 * @distance_mm: length to be converted into duration.
 *
 * Return: time light takes to travel distance_mm (rctu).
 */
static inline int64_t mcps802154_mm_to_rctu(const struct mcps802154_llhw *llhw, int32_t distance_mm)
{
	int64_t temp = (int64_t)distance_mm * llhw->dtu_freq_hz * llhw->dtu_rctu;

	temp += ((temp > 0) ? SPEED_OF_LIGHT_MM_PER_S : -SPEED_OF_LIGHT_MM_PER_S) / 2;
	return temp / SPEED_OF_LIGHT_MM_PER_S;
}

/**
 * mcps802154_psdu_size_to_bytes - Return the PSDU size in bytes.
 * @psdu_size: PSDU size.
 *
 * Return: PSDU size in bytes.
 */
static inline int mcps802154_psdu_size_to_bytes(enum mcps802154_hrp_uwb_psdu_size psdu_size)
{
	switch (psdu_size) {
	case MCPS802154_HRP_UWB_PSDU_SIZE_1023:
		return 1023;
	case MCPS802154_HRP_UWB_PSDU_SIZE_2047:
		return 2047;
	case MCPS802154_HRP_UWB_PSDU_SIZE_4095:
	default:
		return 4095;
	}
}

/**
 * mcps802154_init() - Initialize MCPS 802.15.4 layer.
 *
 * Return: QERR_SUCCESS or error
 */
enum qerr mcps802154_init(void);

/**
 * mcps802154_exit() - Exit MCPS 802.15.4 layer.
 */
void mcps802154_exit(void);

/**
 * mcps802154_restricted_channel_updated() - Restricted channel updated.
 * @llhw: Low-level hardware context.
 */
void mcps802154_restricted_channel_updated(struct mcps802154_llhw *llhw);

#ifdef __cplusplus
}
#endif
