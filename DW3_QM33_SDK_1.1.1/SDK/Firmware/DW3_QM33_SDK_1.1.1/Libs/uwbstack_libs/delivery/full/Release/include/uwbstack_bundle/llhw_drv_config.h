/*
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "deca_device_api.h"
#include "deca_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L1_CONFIG_PDOA_LUT_MAX 31

#define CIA_RX_NUM 2
#define CIA_CIR_NUM 5

/* Chip capabilities. */
#define UWB_LLHW_PRF_MODE_SUPPORTED (MCPS802154_LLHW_BPRF)
#define UWB_LLHW_DATA_RATE_SUPPORTED \
	(MCPS802154_LLHW_DATA_RATE_850K | MCPS802154_LLHW_DATA_RATE_6M81)
#define UWB_LLHW_PRF_SUPPORTED (MCPS802154_LLHW_PRF_16 | MCPS802154_LLHW_PRF_64)
#define UWB_LLHW_PHR_DATA_RATE_SUPPORTED \
	(MCPS802154_LLHW_PHR_DATA_RATE_850K | MCPS802154_LLHW_PHR_DATA_RATE_6M81)
#define UWB_LLHW_PSR_SUPPORTED                                                       \
	(MCPS802154_LLHW_PSR_32 | MCPS802154_LLHW_PSR_64 | MCPS802154_LLHW_PSR_128 | \
	 MCPS802154_LLHW_PSR_256 | MCPS802154_LLHW_PSR_1024 | MCPS802154_LLHW_PSR_4096)
#define UWB_LLHW_SFD_SUPPORTED (MCPS802154_LLHW_SFD_4A | MCPS802154_LLHW_SFD_4Z_8)
#define UWB_LLHW_STS_SEG_SUPPORTED (MCPS802154_LLHW_STS_SEGMENT_1)
#define UWB_LLHW_AOA_SUPPORTED \
	(MCPS802154_LLHW_AOA_AZIMUTH | MCPS802154_LLHW_AOA_ELEVATION | MCPS802154_LLHW_AOA_FOM)

/* Check whether the accumulator index is for Ipatov (master or slave). */
#define DWT_ACC_IDX_IS_IPATOV(acc_idx) ((acc_idx) == DWT_ACC_IDX_IP_M)

/* Check whether the accumulator index is for STS (any segment, master or slave). */
#define DWT_ACC_IDX_IS_STS(acc_idx) (!DWT_ACC_IDX_IS_IPATOV(acc_idx))

/* Enable zero PSDU support. */
#define UWB_LLHW_SUPPORT_ZERO_PSDU

/* Anticip DTU in unit of 100us: 2ms. */
#define ANTICIP_DTU_100US 20

#ifdef __cplusplus
}
#endif
