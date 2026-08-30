/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define M_PI_Q11 (6434)

#if defined(CONFIG_DWT_UWB)
#define UWB_SPIRDY_BIT_MASK DWT_INT_SPIRDY_BIT_MASK
#else
#define UWB_SPIRDY_BIT_MASK 0
#endif

/* UWB config */
/* clang-format off */
#define DEFAULT_CHANNEL			9
#define DEFAULT_TXPREAMBLENGTH 		MCPS802154_PSR_64
#define DEFAULT_PRF			MCPS802154_PRF_64
#define DEFAULT_PCODE 			9
#define DEFAULT_SFD 			MCPS802154_SFD_4Z_8
#define DEFAULT_DATARATE 		MCPS802154_DATA_RATE_6M81
#define DEFAULT_PSDUSIZE 		MCPS802154_HRP_UWB_PSDU_SIZE_1023
#define DEFAULT_PHRHIGHRATE 		false
#define DEFAULT_STS_MODE 		DWT_STS_MODE_OFF /* (DWT_STS_MODE_1 | DWT_STS_MODE_SDC) */
#define DEFAULT_N_SEGS			0 /* 0 segment since DEFAULT_STS_MODE = OFF. */
#define DEFAULT_STS_LENGTH 		64
#ifdef CONFIG_DWT_UWB
#define DEFAULT_PDOA_MODE 		DWT_PDOA_M0
#else
#define DEFAULT_PDOA_MODE 		DWT_PDOA_M0
#endif


#define DEFAULT_FF 				DWT_FF_ENABLE_802_15_4
#define DEFAULT_FF_MODE 		(DWT_FF_BEACON_EN | DWT_FF_DATA_EN \
								| DWT_FF_ACK_EN | DWT_FF_MAC_EN \
								| DWT_FF_MULTI_EN)
#define DEFAULT_DO_TX_CONFIG 		true
#ifdef CONFIG_DWT_UWB
#define DEFAULT_LED_MODE 		(DWT_LEDS_DISABLE)
#else
#define DEFAULT_LED_MODE 		(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK)
#endif
#define DEFAULT_ANT_SET_ID		0

#ifdef CONFIG_DWT_UWB
#define DEFAULT_RFFE_MODE		DWT_LNA_PA_DISABLE
#define DEFAULT_RFFE_RFPATH1		0 /* Unused for DW3000. */
#define DEFAULT_RFFE_RFPATH2		0 /* Unused for DW3000. */
#define DEFAULT_RF_PORT_CTRL		DWT_RF_PORT_AUTO_1_2
#else
/* Default mode must be synchronized with default states */
#define DEFAULT_RFFE_MODE		DWT_RFFE_LUT_MODE
#define DEFAULT_RFFE_RFPATH1		DWT_RFPATH_LNA
#define DEFAULT_RFFE_RFPATH2		DWT_RFPATH_BYPASS
#define DEFAULT_RF_PORT_CTRL		0 /* Unused for QM35. */
#endif
#define DEFAULT_INT_CONFIG_BITMASK_LO 	(UWB_SPIRDY_BIT_MASK    | DWT_INT_TXFRS_BIT_MASK |  \
					 DWT_INT_RXFCG_BIT_MASK | DWT_INT_ARFE_BIT_MASK  | \
					 DWT_INT_RXFSL_BIT_MASK | DWT_INT_RXSTO_BIT_MASK | \
					 DWT_INT_RXPHE_BIT_MASK | DWT_INT_RXFCE_BIT_MASK | \
					 DWT_INT_RXFTO_BIT_MASK | DWT_INT_RXPTO_BIT_MASK | \
					 DWT_INT_TIMER0_BIT_MASK | DWT_INT_TIMER1_BIT_MASK)
#define DEFAULT_INT_CONFIG_BITMASK_HI	0
#define DEFAULT_INT_CONFIG_OPTIONS 	DWT_ENABLE_INT_ONLY
#define DEFAULT_CIA_DIAG_LOG		DW_CIA_DIAG_LOG_ALL
#ifdef CONFIG_DWT_UWB
#define DEFAULT_RX_CONFIG_DRV 		0
#else
#define DEFAULT_RX_CONFIG_DRV 		(DWT_RX_A_ON | DWT_RX_B_ON)
#endif

/* run-time config */

#define DTU_PER_SYS_POWER	4

#define CHIP_FREQ		499200000
#define CHIP_PER_SYS 		2
#define CHIP_PER_DTU 		(CHIP_PER_SYS * (1 << DTU_PER_SYS_POWER))

#define CHIP_PER_SYMB_HPRF 364
#define CHIP_PER_SYMB_PRF64 508
#define CHIP_PER_SYMB_PRF16 496

#define CHIP_PER_DLY		512
#define RCTU_PER_CHIP		128
#define RCTU_PER_DTU 		(RCTU_PER_CHIP * CHIP_PER_DTU)
#define RCTU_PER_SYS 		(RCTU_PER_CHIP * CHIP_PER_SYS)
#define RCTU_PER_DLY 		(CHIP_PER_DLY / RCTU_PER_CHIP)

#define DTU_FREQ		(CHIP_FREQ / CHIP_PER_DTU)

#define LLHW_DTU_TO_US(x) ((int)((uint64_t)(x)*1000000 / DTU_FREQ))
#define LLHW_US_TO_DTU(x) ((int)((uint64_t)(x)*DTU_FREQ / 1000000))

/* 6.9.1.5 in 4z, for HRP UWB PHY:
   416 chips = 416 / (499.2 * 10^6) ~= 833.33 ns */
#define DTU_PER_RSTU		(416 / CHIP_PER_DTU)
#define DTU_PER_DLY		(CHIP_PER_DLY / CHIP_PER_DTU)
#define SYS_PER_DLY		(CHIP_PER_DLY / CHIP_PER_SYS)

#define ANTICIP_DTU		(16 * (DTU_FREQ / 1000))

/* UWB High band 802.15.4a-2007. Only channels 5 & 9 for chips we support. */
#define UWB_SUPPORTED_CHANNELS ((1 << 5) | (1 << 9))

#if defined(CONFIG_DWT_UWB)
#define RX_ENABLE_STARTUP_DLY 16
#else
#define RX_ENABLE_STARTUP_DLY 20
#endif
#define RX_ENABLE_STARTUP_DTU    (RX_ENABLE_STARTUP_DLY * CHIP_PER_DLY / \
					 CHIP_PER_DTU)

#define PREAMBLE_INDEX_HPRF_START 25
#define PREAMBLE_INDEX_PRF64_START 9

/* SOI variant for D2QM3582505 with dynamic LNA, radar mode and 5x2 mux (2 antennas). */
#define QM35_SOI_VARIANT_3 3
/* clang-format on */

#ifdef __cplusplus
}
#endif
