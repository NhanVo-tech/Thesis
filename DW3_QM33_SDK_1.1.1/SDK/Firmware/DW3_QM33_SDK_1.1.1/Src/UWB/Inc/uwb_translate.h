/**
 * @file      uwb_translate.h
 *
 * @brief     Interface for parameters translation
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Channel */
int chan_to_deca(int i);
int deca_to_chan(int i);

/* Bitrate */
int bitrate_to_deca(int i);
int deca_to_bitrate(int i);

/* PRF */
int prf_to_deca(int i);
int deca_to_prf(int i);

/* PAC */
int pac_to_deca(int i);
int deca_to_pac(int i);

/* PLEN */
int plen_to_deca(int i);
int deca_to_plen(int i);

/* Preamble */
int preamble_code_to_deca(int i);
int deca_to_preamble_code(int i);

/* STS Length */
int sts_length_to_deca(int i);
int deca_to_sts_length(int i);

/* STS Mode */
int sts_mode_to_deca(int i);
int deca_to_sts_mode(int i);

/* SFD Type */
int sfd_type_to_deca(int i);
int deca_to_sfd_type(int i);

/* PHR Mode */
int phr_mode_to_deca(int i);
int deca_to_phr_mode(int i);

/* PHR Rate */
int phr_rate_to_deca(int i);
int deca_to_phr_rate(int i);

/* PDOA Mode */
int pdoa_mode_to_deca(int i);
int deca_to_pdoa_mode(int i);

#ifdef __cplusplus
}
#endif
