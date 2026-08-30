#include <Arduino.h>
#include <Preferences.h>
#include <PN532.h>
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/x509_crt.h>
#include "provisioning_phase.h"
#include "ccc_mailbox.h"

namespace {
  Preferences prefs;
  const char* kNs = "ccc_dk";
  const char* kForceProvFlag = "force_prov";
  const char* kOneShotForce = "oneshot_force";
  const char* kCertChain = "cert_chain";
  const char* kFastArtifactVersion = "fast_art_ver";
  const char* kFastArtifactKey = "fast_art_key";

  // Authentication stores phone public key and relies on CCC mailbox vehicle keypair.
}

namespace ProvisioningPhase {

void begin() {
  // Ensure CCC mailbox is initialized and loaded at boot.
  CCCMailbox::begin();
  Serial.println("[ProvisioningPhase] CCC mailbox ready; phone pubkey + vehicle keypair available");
}

bool isProvisioned() {
  return CCCMailbox::hasEndpointPub();
}

bool storePhonePubRaw(const uint8_t* pub65) {
  if (!pub65 || pub65[0] != 0x04) return false;
  bool ok = CCCMailbox::setEndpointPub(pub65);
  if (ok) Serial.println("[PhaseA] Stored phone public key raw (65 bytes) in CCC mailbox");
  return ok;
}

bool storeCertChain(const uint8_t* cert, size_t certLen) {
  if (!cert || certLen == 0) return false;
  prefs.begin(kNs, false);
  bool ok = prefs.putBytes(kCertChain, cert, certLen) == certLen;
  prefs.end();
  if (ok) Serial.printf("[PhaseA] Stored phone cert chain (%u bytes) in CCC mailbox\n", (unsigned)certLen);
  return ok;
}

bool storeFastArtifact(uint8_t version, const uint8_t* key32) {
  if (!key32 || version == 0) return false;
  prefs.begin(kNs, false);
  bool ok = prefs.putUChar(kFastArtifactVersion, version) == 1;
  if (ok) {
    ok = prefs.putBytes(kFastArtifactKey, key32, 32) == 32;
  }
  prefs.end();
  if (ok) {
    Serial.printf("[PhaseA] Stored fast artifact version=%u key_len=32\n", (unsigned)version);
  }
  return ok;
}

bool hasFastArtifact() {
  prefs.begin(kNs, true);
  bool hasVersion = prefs.isKey(kFastArtifactVersion);
  bool hasKey = prefs.getBytesLength(kFastArtifactKey) == 32;
  prefs.end();
  return hasVersion && hasKey;
}

bool getFastArtifact(uint8_t* outVersion, uint8_t outKey32[32]) {
  if (!outVersion || !outKey32) return false;
  prefs.begin(kNs, true);
  if (!prefs.isKey(kFastArtifactVersion) || prefs.getBytesLength(kFastArtifactKey) != 32) {
    prefs.end();
    return false;
  }
  *outVersion = prefs.getUChar(kFastArtifactVersion, 0);
  bool ok = prefs.getBytes(kFastArtifactKey, outKey32, 32) == 32;
  prefs.end();
  return ok && (*outVersion != 0);
}

bool clearFastArtifact() {
  prefs.begin(kNs, false);
  bool changed = false;
  if (prefs.isKey(kFastArtifactVersion)) {
    prefs.remove(kFastArtifactVersion);
    changed = true;
  }
  if (prefs.isKey(kFastArtifactKey)) {
    prefs.remove(kFastArtifactKey);
    changed = true;
  }
  prefs.end();
  return changed;
}

bool setOwnerProvisioned(const uint8_t* pub65, bool force) {
  if (!pub65 || pub65[0] != 0x04) return false;
  if (isProvisioned() && !force) return false;
  if (!CCCMailbox::setEndpointPub(pub65)) return false;
  if (force) {
    CCCMailbox::clearToken(0);
  }
  CCCMailbox::setSlotActive(0, true);
  CCCMailbox::ensureToken(0);
  CCCMailbox::setSignalingFlag(0x0001, true);
  return true;
}

static bool importPubKey65(mbedtls_pk_context &pk, const uint8_t* raw65) {
  mbedtls_pk_init(&pk);
  if (mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) != 0) return false;
  mbedtls_ecp_keypair* eckey = mbedtls_pk_ec(pk);
  if (mbedtls_ecp_group_load(&eckey->grp, MBEDTLS_ECP_DP_SECP256R1) != 0) { mbedtls_pk_free(&pk); return false; }
  if (mbedtls_ecp_point_read_binary(&eckey->grp, &eckey->Q, raw65, 65) != 0) { mbedtls_pk_free(&pk); return false; }
  return true;
}

bool verifySignatureP256(const uint8_t* pub65,
                         const uint8_t* data, size_t dataLen,
                         const uint8_t* sigDer, size_t sigLen) {
  if (!pub65 || !data || !sigDer || dataLen == 0 || sigLen == 0) return false;
  mbedtls_pk_context pk; if (!importPubKey65(pk, pub65)) return false;
  // Hash data
  unsigned char hash[32];
  mbedtls_sha256_context c; mbedtls_sha256_init(&c);
  mbedtls_sha256_starts(&c, 0);
  mbedtls_sha256_update(&c, data, dataLen);
  mbedtls_sha256_finish(&c, hash);
  mbedtls_sha256_free(&c);
  // Verify (sigDer assumed to be DER encoded ECDSA signature)
  int rc = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), sigDer, sigLen);
  mbedtls_pk_free(&pk);
  return rc == 0;
}

void clearAll() {
  CCCMailbox::clearMailboxes();
  prefs.begin(kNs, false);
  if (prefs.isKey(kCertChain)) prefs.remove(kCertChain);
  if (prefs.isKey(kFastArtifactVersion)) prefs.remove(kFastArtifactVersion);
  if (prefs.isKey(kFastArtifactKey)) prefs.remove(kFastArtifactKey);
  if (prefs.isKey(kForceProvFlag)) prefs.remove(kForceProvFlag);
  if (prefs.isKey(kOneShotForce)) prefs.remove(kOneShotForce);
  prefs.end();
}

void clearProvisionedOnly() {
  CCCMailbox::clearEndpointPub();
  CCCMailbox::clearToken(0);
  CCCMailbox::setSlotActive(0, false);
  CCCMailbox::setSignalingFlag(0x0001, true);
  prefs.begin(kNs, false);
  if (prefs.isKey(kCertChain)) prefs.remove(kCertChain);
  if (prefs.isKey(kFastArtifactVersion)) prefs.remove(kFastArtifactVersion);
  if (prefs.isKey(kFastArtifactKey)) prefs.remove(kFastArtifactKey);
  prefs.end();
}

void clearProvisionedData() {
  // Alias for clearProvisionedOnly (for FSM compatibility)
  clearProvisionedOnly();
}

bool setForceProvisioningFlag(bool enable) {
  prefs.begin(kNs, false);
  bool ok = prefs.putBool(kForceProvFlag, enable);
  prefs.end();
  if (ok) {
    Serial.printf("[ProvisioningPhase] Force provisioning flag set to: %s\n", enable ? "ENABLED" : "DISABLED");
  }
  return ok;
}

bool isForceProvisioning() {
  prefs.begin(kNs, true);
  bool forced = prefs.getBool(kForceProvFlag, false);
  bool oneShot = prefs.getBool(kOneShotForce, false);
  prefs.end();
  return forced || oneShot;
}

void setOneShotForce(bool enable) {
  prefs.begin(kNs, false);
  if (enable) {
    prefs.putBool(kOneShotForce, true);
    Serial.println("[ProvisioningPhase] One-shot force provisioning ARMED");
  } else {
    if (prefs.isKey(kOneShotForce)) prefs.remove(kOneShotForce);
    Serial.println("[ProvisioningPhase] One-shot force provisioning CLEARED");
  }
  prefs.end();
}

size_t getPhonePubRaw(uint8_t* out, size_t max) {
  if (!out || max < 65) return 0;
  return CCCMailbox::getEndpointPub(out, max) ? 65 : 0;
}

size_t getCertChain(uint8_t* out, size_t max) {
  prefs.begin(kNs, true);
  if (!prefs.isKey(kCertChain)) { prefs.end(); return 0; }
  size_t need = prefs.getBytesLength(kCertChain);
  if (need == 0) { prefs.end(); return 0; }
  if (!out) { prefs.end(); return need; }
  size_t toCopy = (need <= max) ? need : max;
  size_t got = prefs.getBytes(kCertChain, out, toCopy);
  prefs.end();
  return (got > 0) ? got : need; // if truncated, caller sees at least required length via earlier branch
}

bool validateCertPublicKeyMatchesPub(const uint8_t* cert, size_t certLen, const uint8_t* pub65) {
  if (!cert || certLen == 0 || !pub65 || pub65[0] != 0x04) return false;
  mbedtls_x509_crt crt; mbedtls_x509_crt_init(&crt);
  // Try parse (PEM or DER). For PEM, buffer need not be null-terminated if not relying on PEM; but mbedtls can parse DER directly.
  int rc = mbedtls_x509_crt_parse(&crt, cert, certLen);
  if (rc != 0) {
    mbedtls_x509_crt_free(&crt);
    return false;
  }
  bool match = false;
  if (mbedtls_pk_can_do(&crt.pk, MBEDTLS_PK_ECKEY)) {
    mbedtls_ecp_keypair* eckey = mbedtls_pk_ec(crt.pk);
    unsigned char out[65]; size_t olen = 0;
    if (mbedtls_ecp_point_write_binary(&eckey->grp, &eckey->Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, out, sizeof(out)) == 0 && olen == 65) {
      match = (memcmp(out, pub65, 65) == 0);
    }
  }
  mbedtls_x509_crt_free(&crt);
  return match;
}

bool validateStoredCertMatchesStoredPub() {
  uint8_t pub[65]; size_t pubLen = getPhonePubRaw(pub, sizeof(pub));
  if (pubLen != 65 || pub[0] != 0x04) return false;
  size_t certLen = getCertChain(nullptr, 0);
  if (certLen == 0) return false;
  // Allocate temp buffer to read cert (avoid std::unique_ptr for toolchain compatibility)
  uint8_t* buf = (uint8_t*)malloc(certLen);
  if (!buf) return false;
  size_t got = getCertChain(buf, certLen);
  if (got < certLen) {
    // best-effort even if truncated
    certLen = got;
  }
  bool ok = validateCertPublicKeyMatchesPub(buf, certLen, pub);
  free(buf);
  return ok;
}

// Minimal helper wrapping current working flow
bool runOnceWithHce(PN532& nfc, const uint8_t* aid, size_t aidLen, uint32_t waitMs) {
  Serial.println("[PhaseA] runOnceWithHce is not supported under CCC mailbox flow");
  (void)nfc; (void)aid; (void)aidLen; (void)waitMs;
  return false;
}

size_t getDevicePrivateKeyPEM(uint8_t* out, size_t maxLen) {
  // Deprecated: private key is stored as raw scalar in CCC mailbox, not PEM.
  (void)out; (void)maxLen;
  return 0;
}
} // namespace
