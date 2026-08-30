#include "../../../include/ble/utils/crypto_utils.h"
#include <cstring>
#include <mbedtls/md.h>
#include <mbedtls/gcm.h>

void hkdf_sha256(const uint8_t* salt, size_t saltLen,
                 const uint8_t* ikm, size_t ikmLen,
                 const uint8_t* info, size_t infoLen,
                 uint8_t* out, size_t outLen) {
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  uint8_t prk[32];
  // Extract
  uint8_t zeroSalt[32];
  if (!salt || saltLen == 0) {
    memset(zeroSalt, 0, sizeof(zeroSalt));
    mbedtls_md_hmac(md, zeroSalt, sizeof(zeroSalt), ikm, ikmLen, prk);
  } else {
    mbedtls_md_hmac(md, salt, saltLen, ikm, ikmLen, prk);
  }
  // Expand
  uint8_t T[32]; size_t Tlen = 0; size_t offset = 0; uint8_t counter = 1;
  while (offset < outLen) {
    mbedtls_md_context_t ctx; mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md, 1);
    mbedtls_md_hmac_starts(&ctx, prk, sizeof(prk));
    if (Tlen) mbedtls_md_hmac_update(&ctx, T, Tlen);
    if (info && infoLen) mbedtls_md_hmac_update(&ctx, info, infoLen);
    mbedtls_md_hmac_update(&ctx, &counter, 1);
    mbedtls_md_hmac_finish(&ctx, T);
    mbedtls_md_free(&ctx);
    size_t toCopy = (outLen - offset) < sizeof(T) ? (outLen - offset) : sizeof(T);
    memcpy(out + offset, T, toCopy);
    offset += toCopy; Tlen = sizeof(T); counter++;
  }
}

void hmac_sha256_label(const uint8_t* key, size_t keyLen,
                       const char* label,
                       const uint8_t* a, size_t aLen,
                       const uint8_t* b, size_t bLen,
                       uint8_t out[32]) {
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  size_t labelLen = label ? strlen(label) : 0;
  mbedtls_md_context_t ctx; mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, md, 1);
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  if (labelLen) mbedtls_md_hmac_update(&ctx, (const unsigned char*)label, labelLen);
  if (a && aLen) mbedtls_md_hmac_update(&ctx, a, aLen);
  if (b && bLen) mbedtls_md_hmac_update(&ctx, b, bLen);
  mbedtls_md_hmac_finish(&ctx, out);
  mbedtls_md_free(&ctx);
}

bool hmac_sha256(const uint8_t* key, size_t keyLen,
                 const uint8_t* data, size_t dataLen,
                 uint8_t out[32]) {
  if (!key || !data || !out) return false;
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md) return false;
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, md, 1) != 0) {
    mbedtls_md_free(&ctx);
    return false;
  }
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  mbedtls_md_hmac_update(&ctx, data, dataLen);
  mbedtls_md_hmac_finish(&ctx, out);
  mbedtls_md_free(&ctx);
  return true;
}

bool aes_gcm_encrypt(const uint8_t key[32],
                     const uint8_t nonce[12],
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* plaintext, size_t ptLen,
                     uint8_t* ciphertextOut,
                     uint8_t tagOut[16]) {
  mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&gcm); return false;
  }
  int rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                     ptLen, nonce, 12,
                                     aad, aadLen,
                                     plaintext, ciphertextOut,
                                     16, tagOut);
  mbedtls_gcm_free(&gcm);
  return rc == 0;
}

bool aes_gcm_decrypt(const uint8_t key[32],
                     const uint8_t nonce[12],
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* ciphertext, size_t ctLen,
                     const uint8_t tag[16],
                     uint8_t* plaintextOut) {
  mbedtls_gcm_context gcm; mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&gcm); return false;
  }
  int rc = mbedtls_gcm_auth_decrypt(&gcm, ctLen,
                                    nonce, 12,
                                    aad, aadLen,
                                    tag, 16,
                                    ciphertext, plaintextOut);
  mbedtls_gcm_free(&gcm);
  return rc == 0;
}
