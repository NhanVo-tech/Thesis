#include "ccc_mailbox.h"
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <cstring>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha256.h>

namespace {
  Preferences prefs;
  const char* kNs = "ccc_dk";
  constexpr uint8_t kMaxSlots = 8;

  CCCMailbox::CCC_Mailbox g_mailbox{};
  bool g_loaded = false;
  mbedtls_entropy_context g_vsign_entropy;
  mbedtls_ctr_drbg_context g_vsign_drbg;
  bool g_vsign_rng_ready = false;
  bool g_vehicle_pub_loaded = false;
  bool g_vehicle_priv_loaded = false;

  bool isValidPub(const uint8_t* pub65) {
    return pub65 && pub65[0] == 0x04;
  }

  bool isAllZero(const uint8_t* data, size_t len) {
    if (!data) return true;
    for (size_t i = 0; i < len; ++i) {
      if (data[i] != 0) return false;
    }
    return true;
  }

  bool slotIndexValid(uint8_t slotIndex) {
    return slotIndex < kMaxSlots;
  }

  uint8_t slotMask(uint8_t slotIndex) {
    return static_cast<uint8_t>(1u << slotIndex);
  }

  void endpointKey(uint8_t slotIndex, char* out, size_t outLen) {
    snprintf(out, outLen, "ep_%u", (unsigned)slotIndex);
  }

  void tokenKey(uint8_t slotIndex, char* out, size_t outLen) {
    snprintf(out, outLen, "tok_%u", (unsigned)slotIndex);
  }

  bool slotHasEndpoint(uint8_t slotIndex) {
    if (!slotIndexValid(slotIndex)) return false;
    if ((g_mailbox.slot_bitmap & slotMask(slotIndex)) == 0) return false;
    return isValidPub(g_mailbox.slots[slotIndex].endpoint_pub);
  }

  bool slotHasToken(uint8_t slotIndex) {
    if (!slotIndexValid(slotIndex)) return false;
    if ((g_mailbox.slot_bitmap & slotMask(slotIndex)) == 0) return false;
    return !isAllZero(g_mailbox.slots[slotIndex].immobilizer_token,
                      sizeof(g_mailbox.slots[slotIndex].immobilizer_token));
  }

  void clearLegacyProvisioning() {
    prefs.begin("prov", false);
    prefs.clear();
    prefs.end();
  }

  void generateVehicleId(char out[9]) {
    static const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    out[0] = 'V';
    out[1] = 'N';
    for (size_t i = 2; i < 8; ++i) {
      uint32_t r = esp_random();
      out[i] = kAlphabet[r % (sizeof(kAlphabet) - 1)];
    }
    out[8] = '\0';
  }

  bool generateVehicleKeypair(uint8_t outPub[65], uint8_t outPriv[32]) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point q;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&q);

    const char* pers = "ccc_vpub";
    int rc = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                                   (const unsigned char*)pers, strlen(pers));
    if (rc != 0) {
      mbedtls_ecp_point_free(&q);
      mbedtls_mpi_free(&d);
      mbedtls_ecp_group_free(&grp);
      mbedtls_ctr_drbg_free(&drbg);
      mbedtls_entropy_free(&entropy);
      return false;
    }

    rc = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (rc != 0) {
      mbedtls_ecp_point_free(&q);
      mbedtls_mpi_free(&d);
      mbedtls_ecp_group_free(&grp);
      mbedtls_ctr_drbg_free(&drbg);
      mbedtls_entropy_free(&entropy);
      return false;
    }

    rc = mbedtls_ecp_gen_keypair(&grp, &d, &q, mbedtls_ctr_drbg_random, &drbg);
    if (rc != 0) {
      mbedtls_ecp_point_free(&q);
      mbedtls_mpi_free(&d);
      mbedtls_ecp_group_free(&grp);
      mbedtls_ctr_drbg_free(&drbg);
      mbedtls_entropy_free(&entropy);
      return false;
    }

    size_t olen = 0;
    rc = mbedtls_ecp_point_write_binary(&grp, &q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                        &olen, outPub, 65);
    if (rc == 0) {
      rc = mbedtls_mpi_write_binary(&d, outPriv, 32);
    }

    mbedtls_ecp_point_free(&q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);

    return rc == 0 && olen == 65;
  }

  bool validateVehicleKeypair(const uint8_t pub[65], const uint8_t priv[32]) {
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point q;
    mbedtls_ecp_point qCalc;

    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&q);
    mbedtls_ecp_point_init(&qCalc);

    int rc = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (rc == 0) rc = mbedtls_mpi_read_binary(&d, priv, 32);
    if (rc == 0) rc = mbedtls_ecp_point_read_binary(&grp, &q, pub, 65);
    if (rc == 0) {
      rc = mbedtls_ecp_mul(&grp, &qCalc, &d, &grp.G, nullptr, nullptr);
    }

    unsigned char qBuf[65];
    size_t qLen = 0;
    if (rc == 0) {
      rc = mbedtls_ecp_point_write_binary(&grp, &qCalc, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                          &qLen, qBuf, sizeof(qBuf));
    }

    bool ok = (rc == 0 && qLen == 65 && memcmp(qBuf, pub, 65) == 0);

    mbedtls_ecp_point_free(&qCalc);
    mbedtls_ecp_point_free(&q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    return ok;
  }

  void loadMailbox() {
    memset(&g_mailbox, 0, sizeof(g_mailbox));
    g_vehicle_pub_loaded = false;
    g_vehicle_priv_loaded = false;
    prefs.begin(kNs, true);

    String vId = prefs.getString("v_id", "");
    if (vId.length() == 8) {
      memcpy(g_mailbox.vehicle_id, vId.c_str(), 8);
      g_mailbox.vehicle_id[8] = '\0';
    }

    size_t vPubLen = prefs.getBytesLength("v_pub");
    if (vPubLen == 65) {
      prefs.getBytes("v_pub", g_mailbox.vehicle_pub, 65);
      g_vehicle_pub_loaded = isValidPub(g_mailbox.vehicle_pub);
    }

    size_t vPrivLen = prefs.getBytesLength("v_priv");
    if (vPrivLen == 32) {
      prefs.getBytes("v_priv", g_mailbox.vehicle_priv, 32);
      g_vehicle_priv_loaded = true;
    }

    g_mailbox.signaling_bitmap = prefs.getUShort("sig_bmp", 0);
    g_mailbox.slot_bitmap = prefs.getUChar("slot_bmp", 0);

    bool normalizeSlotBitmap = false;

    for (uint8_t i = 0; i < kMaxSlots; ++i) {
      char epKey[8];
      endpointKey(i, epKey, sizeof(epKey));
      size_t epLen = prefs.getBytesLength(epKey);
      if (epLen == 65) {
        prefs.getBytes(epKey, g_mailbox.slots[i].endpoint_pub, 65);
        if (isValidPub(g_mailbox.slots[i].endpoint_pub)) {
          if ((g_mailbox.slot_bitmap & slotMask(i)) == 0) {
            g_mailbox.slot_bitmap |= slotMask(i);
            normalizeSlotBitmap = true;
          }
        } else {
          memset(g_mailbox.slots[i].endpoint_pub, 0, 65);
        }
      }
    }

    // Migrate legacy owner endpoint key storage (ep_pub) into slot 0.
    bool migratedLegacyEndpoint = false;
    size_t legacyEpLen = prefs.getBytesLength("ep_pub");
    if (legacyEpLen == 65 && !slotHasEndpoint(0)) {
      uint8_t legacyPub[65];
      prefs.getBytes("ep_pub", legacyPub, 65);
      if (isValidPub(legacyPub)) {
        memcpy(g_mailbox.slots[0].endpoint_pub, legacyPub, 65);
        g_mailbox.slot_bitmap |= slotMask(0);
        normalizeSlotBitmap = true;
        migratedLegacyEndpoint = true;
      }
    }

    for (uint8_t i = 0; i < kMaxSlots; ++i) {
      char key[8];
      tokenKey(i, key, sizeof(key));
      size_t tokLen = prefs.getBytesLength(key);
      if (tokLen == 32) {
        prefs.getBytes(key, g_mailbox.slots[i].immobilizer_token, 32);
        if ((g_mailbox.slot_bitmap & slotMask(i)) == 0) {
          g_mailbox.slot_bitmap |= slotMask(i);
          normalizeSlotBitmap = true;
        }
      }
    }

    prefs.end();

    if (normalizeSlotBitmap || migratedLegacyEndpoint) {
      prefs.begin(kNs, false);
      prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
      if (migratedLegacyEndpoint) {
        char ep0Key[8];
        endpointKey(0, ep0Key, sizeof(ep0Key));
        prefs.putBytes(ep0Key, g_mailbox.slots[0].endpoint_pub, 65);
        if (prefs.isKey("ep_pub")) {
          prefs.remove("ep_pub");
        }
      }
      prefs.end();
    }
  }

  void persistRoot() {
    prefs.begin(kNs, false);
    prefs.putString("v_id", g_mailbox.vehicle_id);
    prefs.putBytes("v_pub", g_mailbox.vehicle_pub, 65);
    prefs.putBytes("v_priv", g_mailbox.vehicle_priv, 32);
    prefs.putUShort("sig_bmp", g_mailbox.signaling_bitmap);
    prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
    prefs.end();
  }
}

namespace CCCMailbox {

bool begin() {
  if (g_loaded) return true;

  loadMailbox();

  bool invalidVehiclePair = g_vehicle_pub_loaded && g_vehicle_priv_loaded
      ? !validateVehicleKeypair(g_mailbox.vehicle_pub, g_mailbox.vehicle_priv)
      : false;
  bool missingRoot = (strlen(g_mailbox.vehicle_id) != 8)
      || !g_vehicle_pub_loaded
      || !g_vehicle_priv_loaded
      || invalidVehiclePair;
  if (missingRoot) {
    if (g_vehicle_pub_loaded && !g_vehicle_priv_loaded) {
      Serial.println("[CCC] Vehicle private key missing; rekeying vehicle identity and clearing provisioned data");
    }
    clearLegacyProvisioning();
    clearAll();
    generateVehicleId(g_mailbox.vehicle_id);
    if (!generateVehicleKeypair(g_mailbox.vehicle_pub, g_mailbox.vehicle_priv)) {
      Serial.println("[CCC] ERROR: Failed to generate vehicle keypair");
      return false;
    }
    g_vehicle_pub_loaded = true;
    g_vehicle_priv_loaded = true;
    g_mailbox.vehicle_identity_valid = true;
    g_mailbox.signaling_bitmap = 0;
    g_mailbox.slot_bitmap = 0;
    persistRoot();
    Serial.printf("[CCC] Initialized mailbox (v_id=%s)\n", g_mailbox.vehicle_id);
  } else {
    g_mailbox.vehicle_identity_valid = true;
    Serial.printf("[CCC] Loaded mailbox (v_id=%s slot_bmp=0x%02X)\n",
                  g_mailbox.vehicle_id, g_mailbox.slot_bitmap);
  }

  g_loaded = true;

  mbedtls_entropy_init(&g_vsign_entropy);
  mbedtls_ctr_drbg_init(&g_vsign_drbg);
  const char* pers = "ccc_vsig";
  int seedRc = mbedtls_ctr_drbg_seed(&g_vsign_drbg, mbedtls_entropy_func, &g_vsign_entropy,
                                     (const unsigned char*)pers, strlen(pers));
  g_vsign_rng_ready = (seedRc == 0);
  if (!g_vsign_rng_ready) {
    Serial.printf("[CCC] WARNING: vehicle signing RNG seed failed rc=%d\n", seedRc);
  }

  return true;
}

const CCC_Mailbox& get() {
  return g_mailbox;
}

const char* vehicleId() {
  return g_mailbox.vehicle_id;
}

bool hasVehiclePub() {
  return g_mailbox.vehicle_identity_valid;
}

bool hasVehiclePriv() {
  return g_mailbox.vehicle_identity_valid;
}

bool getVehiclePub(uint8_t* out, size_t max) {
  if (!out || max < 65 || !g_mailbox.vehicle_identity_valid) return false;
  memcpy(out, g_mailbox.vehicle_pub, 65);
  return true;
}

bool signVehicleDataP256(const uint8_t* data, size_t dataLen,
                        uint8_t* sigDerOut, size_t sigDerMax,
                        size_t* sigDerLenOut) {
  if (!data || dataLen == 0 || !sigDerOut || sigDerMax == 0 || !sigDerLenOut) return false;
  if (!g_mailbox.vehicle_identity_valid) return false;
  if (!g_vsign_rng_ready) return false;

  unsigned char hash[32];
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  mbedtls_sha256_update(&sha, data, dataLen);
  mbedtls_sha256_finish(&sha, hash);
  mbedtls_sha256_free(&sha);

  mbedtls_ecdsa_context ecdsa;
  mbedtls_ecdsa_init(&ecdsa);
  int rc = 0;
  if (rc == 0) rc = mbedtls_ecp_group_load(&ecdsa.grp, MBEDTLS_ECP_DP_SECP256R1);
  if (rc == 0) rc = mbedtls_mpi_read_binary(&ecdsa.d, g_mailbox.vehicle_priv, 32);
  if (rc == 0) {
    rc = mbedtls_ecp_mul(&ecdsa.grp, &ecdsa.Q, &ecdsa.d, &ecdsa.grp.G,
                         mbedtls_ctr_drbg_random, &g_vsign_drbg);
  }

  size_t sigLen = 0;
  if (rc == 0) {
    rc = mbedtls_ecdsa_write_signature(&ecdsa, MBEDTLS_MD_SHA256,
                                       hash, sizeof(hash),
                                       sigDerOut, &sigLen,
                                       mbedtls_ctr_drbg_random, &g_vsign_drbg);
  }

  mbedtls_ecdsa_free(&ecdsa);

  if (rc != 0 || sigLen == 0 || sigLen > sigDerMax) {
    return false;
  }

  *sigDerLenOut = sigLen;
  return true;
}

bool hasEndpointPub(uint8_t slotIndex) {
  return slotHasEndpoint(slotIndex);
}

bool getEndpointPub(uint8_t* out, size_t max, uint8_t slotIndex) {
  if (!slotHasEndpoint(slotIndex) || !out || max < 65) return false;
  memcpy(out, g_mailbox.slots[slotIndex].endpoint_pub, 65);
  return true;
}

bool setEndpointPub(const uint8_t* pub65, uint8_t slotIndex) {
  if (!slotIndexValid(slotIndex) || !pub65 || !isValidPub(pub65)) return false;
  memcpy(g_mailbox.slots[slotIndex].endpoint_pub, pub65, 65);
  g_mailbox.slot_bitmap |= slotMask(slotIndex);
  char key[8];
  endpointKey(slotIndex, key, sizeof(key));
  prefs.begin(kNs, false);
  bool ok = prefs.putBytes(key, pub65, 65) == 65;
  if (ok) ok = prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  if (slotIndex == 0 && prefs.isKey("ep_pub")) {
    prefs.remove("ep_pub");
  }
  prefs.end();
  return ok;
}

bool clearEndpointPub(uint8_t slotIndex) {
  if (!slotIndexValid(slotIndex)) return false;
  memset(g_mailbox.slots[slotIndex].endpoint_pub, 0,
         sizeof(g_mailbox.slots[slotIndex].endpoint_pub));
  g_mailbox.slot_bitmap &= static_cast<uint8_t>(~slotMask(slotIndex));
  char key[8];
  endpointKey(slotIndex, key, sizeof(key));
  prefs.begin(kNs, false);
  bool ok = true;
  if (prefs.isKey(key)) ok = prefs.remove(key);
  if (slotIndex == 0 && prefs.isKey("ep_pub")) ok = ok && prefs.remove("ep_pub");
  ok = ok && prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  prefs.end();
  return ok;
}

bool isSlotActive(uint8_t slotIndex) {
  if (!slotIndexValid(slotIndex)) return false;
  return (g_mailbox.slot_bitmap & slotMask(slotIndex)) != 0;
}

bool setSlotActive(uint8_t slotIndex, bool active) {
  if (!slotIndexValid(slotIndex)) return false;
  if (active) {
    g_mailbox.slot_bitmap |= slotMask(slotIndex);
  } else {
    g_mailbox.slot_bitmap &= static_cast<uint8_t>(~slotMask(slotIndex));
  }
  prefs.begin(kNs, false);
  bool ok = prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  prefs.end();
  return ok;
}

bool hasToken(uint8_t slotIndex) {
  return slotHasToken(slotIndex);
}

bool getToken(uint8_t slotIndex, uint8_t out[32]) {
  if (!slotHasToken(slotIndex) || !out) return false;
  memcpy(out, g_mailbox.slots[slotIndex].immobilizer_token, 32);
  return true;
}

bool setToken(uint8_t slotIndex, const uint8_t tok[32]) {
  if (!slotIndexValid(slotIndex) || !tok) return false;
  memcpy(g_mailbox.slots[slotIndex].immobilizer_token, tok, 32);
  g_mailbox.slot_bitmap |= slotMask(slotIndex);
  char key[8];
  tokenKey(slotIndex, key, sizeof(key));
  prefs.begin(kNs, false);
  bool ok = prefs.putBytes(key, tok, 32) == 32;
  if (ok) ok = prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  prefs.end();
  return ok;
}

bool clearToken(uint8_t slotIndex) {
  if (!slotIndexValid(slotIndex)) return false;
  memset(g_mailbox.slots[slotIndex].immobilizer_token, 0, 32);
  g_mailbox.slot_bitmap &= static_cast<uint8_t>(~slotMask(slotIndex));
  char key[8];
  tokenKey(slotIndex, key, sizeof(key));
  prefs.begin(kNs, false);
  bool ok = true;
  if (prefs.isKey(key)) ok = prefs.remove(key);
  ok = ok && prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  prefs.end();
  return ok;
}

bool ensureToken(uint8_t slotIndex) {
  if (!slotIndexValid(slotIndex)) return false;
  if (slotHasToken(slotIndex)) return true;
  uint8_t tok[32];
  for (size_t i = 0; i < sizeof(tok); ++i) {
    tok[i] = (uint8_t)(esp_random() & 0xFF);
  }
  return setToken(slotIndex, tok);
}

uint16_t signalingBitmap() {
  return g_mailbox.signaling_bitmap;
}

bool setSignalingBitmap(uint16_t bitmap) {
  g_mailbox.signaling_bitmap = bitmap;
  prefs.begin(kNs, false);
  bool ok = prefs.putUShort("sig_bmp", bitmap);
  prefs.end();
  return ok;
}

bool setSignalingFlag(uint16_t mask, bool enabled) {
  uint16_t bitmap = g_mailbox.signaling_bitmap;
  if (enabled) bitmap |= mask;
  else bitmap &= ~mask;
  return setSignalingBitmap(bitmap);
}

void clearMailboxes() {
  for (uint8_t i = 0; i < kMaxSlots; ++i) {
    clearEndpointPub(i);
  }
  g_mailbox.signaling_bitmap = 0;
  g_mailbox.slot_bitmap = 0;
  prefs.begin(kNs, false);
  prefs.putUShort("sig_bmp", g_mailbox.signaling_bitmap);
  prefs.putUChar("slot_bmp", g_mailbox.slot_bitmap);
  prefs.end();

  for (uint8_t i = 0; i < kMaxSlots; ++i) {
    clearToken(i);
  }
}

void clearAll() {
  prefs.begin(kNs, false);
  prefs.clear();
  prefs.end();
  memset(&g_mailbox, 0, sizeof(g_mailbox));
  g_vehicle_pub_loaded = false;
  g_vehicle_priv_loaded = false;
  g_loaded = false;
}

} // namespace CCCMailbox
