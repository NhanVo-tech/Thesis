#pragma once
#include <cstddef>
#include <cstdint>

// HKDF-SHA256: outLen bytes derived from ikm with optional salt and info
void hkdf_sha256(const uint8_t* salt, size_t saltLen,
                 const uint8_t* ikm, size_t ikmLen,
                 const uint8_t* info, size_t infoLen,
                 uint8_t* out, size_t outLen);

// HMAC-SHA256 over label || a || b using the given key; writes 32-byte tag
void hmac_sha256_label(const uint8_t* key, size_t keyLen,
                       const char* label,
                       const uint8_t* a, size_t aLen,
                       const uint8_t* b, size_t bLen,
                       uint8_t out[32]);

// HMAC-SHA256 over raw data buffer; writes 32-byte tag
bool hmac_sha256(const uint8_t* key, size_t keyLen,
                 const uint8_t* data, size_t dataLen,
                 uint8_t out[32]);

// AES-256-GCM primitives (key must be 32 bytes, nonce 12 bytes, tag 16 bytes)
bool aes_gcm_encrypt(const uint8_t key[32],
                     const uint8_t nonce[12],
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* plaintext, size_t ptLen,
                     uint8_t* ciphertextOut /* size=ptLen */,
                     uint8_t tagOut[16]);

bool aes_gcm_decrypt(const uint8_t key[32],
                     const uint8_t nonce[12],
                     const uint8_t* aad, size_t aadLen,
                     const uint8_t* ciphertext, size_t ctLen,
                     const uint8_t tag[16],
                     uint8_t* plaintextOut /* size=ctLen */);
