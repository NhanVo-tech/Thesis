/**
 * @file      HAL_crypto.c
 *
 * @brief     HAL implementation of crypto functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */
#include "HAL_crypto.h"

#include "nrf_crypto_aes.h"
#include "nrf_crypto_aead.h"
#include "nrf_crypto_error.h"
#include "nrf_crypto_init.h"
#include "nrf_crypto_rng.h"
#include "qmalloc.h"

#define CCM_STAR_NONCE_LEN 13
#define KEY_SIZE_BYTES     16

struct crypto_ctx
{
    bool is_initialized;
};

struct crypto_aes_128_ctx
{
    nrf_crypto_aes_context_t aes_ctx;
    uint8_t key[KEY_SIZE_BYTES];
    bool encrypt;
};

static struct crypto_ctx crypto_ctx;
static nrf_crypto_aes_context_t cmac_ctx;

static enum qerr nrf_error_to_qerr(int error)
{
    switch (error)
    {
        case 0:
            return QERR_SUCCESS;
        case NRF_ERROR_CRYPTO_FEATURE_UNAVAILABLE:
            return QERR_ENOTSUP;
        case NRF_ERROR_CRYPTO_INPUT_NULL:
        case NRF_ERROR_CRYPTO_INPUT_LENGTH:
        case NRF_ERROR_CRYPTO_INPUT_LOCATION:
        case NRF_ERROR_CRYPTO_OUTPUT_NULL:
        case NRF_ERROR_CRYPTO_OUTPUT_LENGTH:
        case NRF_ERROR_CRYPTO_INVALID_PARAM:
        case NRF_ERROR_CRYPTO_KEY_SIZE:
        case NRF_ERROR_CRYPTO_ECDH_CURVE_MISMATCH:
        case NRF_ERROR_CRYPTO_ECDSA_INVALID_SIGNATURE:
        case NRF_ERROR_CRYPTO_ECC_INVALID_KEY:
        case NRF_ERROR_CRYPTO_AES_INVALID_PADDING:
        case NRF_ERROR_CRYPTO_AEAD_INVALID_MAC:
        case NRF_ERROR_CRYPTO_AEAD_NONCE_SIZE:
        case NRF_ERROR_CRYPTO_AEAD_MAC_SIZE:
            return QERR_EINVAL;
        case NRF_ERROR_CRYPTO_ALLOC_FAILED:
        case NRF_ERROR_CRYPTO_STACK_OVERFLOW:
            return QERR_ENOMEM;
        case NRF_ERROR_CRYPTO_BUSY:
            return QERR_EBUSY;
        case NRF_ERROR_CRYPTO_NOT_INITIALIZED:
        case NRF_ERROR_CRYPTO_CONTEXT_NULL:
        case NRF_ERROR_CRYPTO_CONTEXT_NOT_INITIALIZED:
        case NRF_ERROR_CRYPTO_INTERNAL:
        case NRF_ERROR_CRYPTO_ECC_KEY_NOT_INITIALIZED:
        case NRF_ERROR_CRYPTO_RNG_INIT_FAILED:
        case NRF_ERROR_CRYPTO_RNG_RESEED_REQUIRED:
            return QERR_EBADMSG;
        default:
            return QERR_EBADMSG;
    }
}

enum qerr crypto_init(void)
{
    if (crypto_ctx.is_initialized)
        return QERR_SUCCESS;

    crypto_ctx.is_initialized = true;
    return nrf_error_to_qerr(nrf_crypto_init());
}

enum qerr crypto_deinit(void)
{
    if (crypto_ctx.is_initialized)
        return nrf_error_to_qerr(nrf_crypto_uninit());

    crypto_ctx.is_initialized = false;
    return QERR_SUCCESS;
}

enum qerr crypto_get_random_vector(uint8_t *const p_target, size_t size)
{
    return nrf_error_to_qerr(nrf_crypto_rng_vector_generate(p_target, size));
}

uint32_t crypto_get_random_number(void)
{
    uint8_t random_bytes[4];

    nrf_crypto_rng_vector_generate(random_bytes, sizeof(random_bytes));

    return *(uint32_t *)random_bytes;
}

enum qerr crypto_cmac_aes_128_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
)
{
    size_t out_len = data_len;
    ret_code_t r;

    r = nrf_crypto_aes_crypt(&cmac_ctx, &g_nrf_crypto_aes_cmac_128_info, NRF_CRYPTO_MAC_CALCULATE, (uint8_t *)key, NULL, (uint8_t *)data, data_len, out, &out_len);
    return nrf_error_to_qerr(r);
}

enum qerr crypto_cmac_aes_256_digest(
    const uint8_t *key, const uint8_t *data,
    unsigned int data_len, uint8_t *out
)
{
#if NRF_CRYPTO_BACKEND_CC310_ENABLED
    return QERR_ENOTSUP;
#else
    size_t out_len = data_len;
    ret_code_t r;

    r = nrf_crypto_aes_crypt(&cmac_ctx, &g_nrf_crypto_aes_cmac_256_info, NRF_CRYPTO_MAC_CALCULATE, (uint8_t *)key, NULL, (uint8_t *)data, data_len, out, &out_len);
    return nrf_error_to_qerr(r);
#endif
}

void crypto_aead_aes_ccm_star_128_destroy(void *ctx)
{
    nrf_crypto_aead_uninit(ctx);
    qfree(ctx);
}

enum qerr crypto_aead_aes_ccm_star_128_create(void **ccm_star_ctx, const uint8_t *key)
{
    nrf_crypto_aead_context_t *ccm_ctx;
    ret_code_t r;

    ccm_ctx = (nrf_crypto_aead_context_t *)qmalloc(sizeof(nrf_crypto_aead_context_t));
    if (!ccm_ctx)
        return QERR_ENOMEM;

    r = nrf_crypto_aead_init(ccm_ctx, &g_nrf_crypto_aes_ccm_128_info, (uint8_t *)key);
    if (r)
        return nrf_error_to_qerr(r);
    *ccm_star_ctx = ccm_ctx;

    return QERR_SUCCESS;
}

enum qerr crypto_aead_aes_ccm_star_128_encrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned header_len, uint8_t *data, unsigned data_len,
    uint8_t *out, uint8_t *mac, unsigned mac_len
)
{
    ret_code_t r;
    nrf_crypto_aead_context_t *ccm_ctx = ctx;

    r = nrf_crypto_aead_crypt(ccm_ctx, NRF_CRYPTO_ENCRYPT, (uint8_t *)nonce, CCM_STAR_NONCE_LEN, (uint8_t *)header, header_len, data, data_len, data, mac, mac_len);
    return nrf_error_to_qerr(r);
}

enum qerr crypto_aead_aes_ccm_star_128_encrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
)
{
    return crypto_aead_aes_ccm_star_128_encrypt_inout(
        ctx, nonce, header, header_len, data, data_len, data, mac,
        mac_len
    );
}

enum qerr crypto_aead_aes_ccm_star_128_decrypt_inout(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *out, uint8_t *mac, unsigned int mac_len
)
{
    ret_code_t r;
    nrf_crypto_aead_context_t *ccm_ctx = ctx;

    r = nrf_crypto_aead_crypt(ccm_ctx, NRF_CRYPTO_DECRYPT, (uint8_t *)nonce, CCM_STAR_NONCE_LEN, (uint8_t *)header, header_len, data, data_len, data, mac, mac_len);
    return nrf_error_to_qerr(r);
}

enum qerr crypto_aead_aes_ccm_star_128_decrypt(
    void *ctx, const uint8_t *nonce, const uint8_t *header,
    unsigned int header_len, uint8_t *data, unsigned int data_len,
    uint8_t *mac, unsigned int mac_len
)
{
    return crypto_aead_aes_ccm_star_128_decrypt_inout(
        ctx, nonce, header, header_len, data, data_len, data, mac,
        mac_len
    );
}

enum qerr crypto_aes_ecb_128_create(void **ecb_ctx, const uint8_t *key, bool encrypt)
{
    struct crypto_aes_128_ctx *ctx;

    ctx = qmalloc(sizeof(*ctx));
    if (!ctx)
        return QERR_ENOMEM;

    memcpy(ctx->key, key, sizeof(ctx->key));
    ctx->encrypt = encrypt;
    *ecb_ctx = ctx;

    return QERR_SUCCESS;
}

void crypto_aes_ecb_128_destroy(void *ctx)
{
    struct crypto_aes_128_ctx *ecb_ctx = ctx;

    qfree(ecb_ctx);
}

enum qerr crypto_aes_ecb_128_encrypt_decrypt(void *ctx, const uint8_t *data, unsigned int data_len, uint8_t *out)
{
    struct crypto_aes_128_ctx *ecb_ctx = ctx;
    ret_code_t r;

    r = nrf_crypto_aes_crypt(&ecb_ctx->aes_ctx, &g_nrf_crypto_aes_ecb_128_info, ecb_ctx->encrypt ? NRF_CRYPTO_ENCRYPT : NRF_CRYPTO_DECRYPT, ecb_ctx->key, NULL, (uint8_t *)data, data_len, out, &data_len);
    return nrf_error_to_qerr(r);
}
