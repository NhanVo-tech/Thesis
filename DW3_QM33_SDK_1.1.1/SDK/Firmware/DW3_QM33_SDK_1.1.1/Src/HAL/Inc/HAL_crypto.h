/**
 * @file      HAL_cyrpto.h
 *
 * @brief     Interface for HW-related crypto functions.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */
#pragma once

#include "qerr.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum qerr crypto_init(void);
enum qerr crypto_deinit(void);
enum qerr crypto_get_random_vector(uint8_t *const p_target, size_t size);
uint32_t crypto_get_random_number(void);
enum qerr crypto_cmac_aes_128_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
);
enum qerr crypto_cmac_aes_256_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
);
void crypto_aead_aes_ccm_star_128_destroy(void *ctx);
enum qerr crypto_aead_aes_ccm_star_128_create(void **ccm_star_ctx, const uint8_t *key);
enum qerr crypto_aead_aes_ccm_star_128_encrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned header_len, uint8_t *data, unsigned data_len,
    uint8_t *out, uint8_t *mac, unsigned mac_len
);
enum qerr crypto_aead_aes_ccm_star_128_encrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
);
enum qerr crypto_aead_aes_ccm_star_128_decrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *out, uint8_t *mac, unsigned int mac_len
);
enum qerr crypto_aead_aes_ccm_star_128_decrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
);
void crypto_aes_ecb_128_destroy(void *ctx);
enum qerr crypto_aes_ecb_128_create(void **ecb_ctx, const uint8_t *key, bool encrypt);
enum qerr crypto_aes_ecb_128_encrypt_decrypt(void *ctx, const uint8_t *data, unsigned int data_len, uint8_t *out);
