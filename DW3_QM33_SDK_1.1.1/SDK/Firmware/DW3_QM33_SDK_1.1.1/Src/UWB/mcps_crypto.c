/**
 * @file      mcps_crypto.c
 *
 * @brief     Implementaion of mcps crypto functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */
#include "HAL_crypto.h"
#include "mcps_crypto_platform.h"
#include "qerr.h"

enum qerr mcps_crypto_init(void)
{
    return crypto_init();
}

void mcps_crypto_deinit(void)
{
    crypto_deinit();
}

enum qerr mcps_crypto_reinit(void)
{
    /* Nothing to reinit for now. */
    return QERR_SUCCESS;
}

uint32_t mcps_crypto_get_random(void)
{
    return crypto_get_random_number();
}

enum qerr mcps_crypto_cmac_aes_128_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
)
{
    return crypto_cmac_aes_128_digest(key, data, data_len, out);
}

enum qerr mcps_crypto_cmac_aes_256_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
)
{
    return crypto_cmac_aes_256_digest(key, data, data_len, out);
}

void mcps_crypto_aead_aes_ccm_star_128_destroy(void *ctx)
{
    crypto_aead_aes_ccm_star_128_destroy(ctx);
}

int mcps_crypto_aead_aes_ccm_star_128_create(void **ccm_star_ctx, const uint8_t *key)
{
    return crypto_aead_aes_ccm_star_128_create(ccm_star_ctx, key);
}

int mcps_crypto_aead_aes_ccm_star_128_encrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned header_len, uint8_t *data, unsigned data_len,
    uint8_t *out, uint8_t *mac, unsigned mac_len
)
{
    return crypto_aead_aes_ccm_star_128_encrypt_inout(ctx, nonce, header, header_len, data, data_len, out, mac, mac_len);
}

int mcps_crypto_aead_aes_ccm_star_128_encrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
)
{
    return crypto_aead_aes_ccm_star_128_encrypt(
        ctx, nonce, header, header_len, data, data_len, mac,
        mac_len
    );
}

int mcps_crypto_aead_aes_ccm_star_128_decrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *out, uint8_t *mac, unsigned int mac_len
)
{
    return crypto_aead_aes_ccm_star_128_decrypt_inout(ctx, nonce, header, header_len, data, data_len, out, mac, mac_len);
}

int mcps_crypto_aead_aes_ccm_star_128_decrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
)
{
    return crypto_aead_aes_ccm_star_128_decrypt(
        ctx, nonce, header, header_len, data, data_len, mac,
        mac_len
    );
}

void mcps_crypto_aes_ecb_128_destroy(void *ctx)
{
    crypto_aes_ecb_128_destroy(ctx);
}

int mcps_crypto_aes_ecb_128_create_decrypt(void **ecb_ctx, const uint8_t *key)
{
    return crypto_aes_ecb_128_create(ecb_ctx, key, false);
}

int mcps_crypto_aes_ecb_128_create_encrypt(void **ecb_ctx, const uint8_t *key)
{
    return crypto_aes_ecb_128_create(ecb_ctx, key, true);
}

int mcps_crypto_aes_ecb_128_encrypt_decrypt(void *ctx, const uint8_t *data, unsigned int data_len, uint8_t *out)
{
    return crypto_aes_ecb_128_encrypt_decrypt(ctx, data, data_len, out);
}
