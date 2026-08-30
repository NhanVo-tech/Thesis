#include <Arduino.h>
#include <NimBLEDevice.h>
#include <cstring>
#include <time.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/bignum.h>

#include "ble/ble_attestation.h"
#include "ccc_mailbox.h"

namespace {
  const char* kAttestServiceUUID = "555a0001-00aa-1111-2222-333344445555";
  const char* kAttestRxUUID = "555a0002-00aa-1111-2222-333344445555";
  const char* kAttestTxUUID = "555a0003-00aa-1111-2222-333344445555";

  constexpr size_t kAttestLen = 147;
  constexpr size_t kPayloadLen = 83;

  NimBLECharacteristic* g_cAttestTx = nullptr;

  uint32_t readU32BE(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
  }

  bool getEpochNow(uint32_t& out) {
    time_t now = time(nullptr);
    if (now < 1700000000) return false; // time not set
    out = (uint32_t)now;
    return true;
  }

  void notifyStatus(const char* msg) {
    if (!g_cAttestTx || !msg) return;
    g_cAttestTx->setValue(msg);
    g_cAttestTx->notify();
  }

  bool verifyRawSigP256(const uint8_t* pub65, const uint8_t* hash32,
                        const uint8_t* sigR, const uint8_t* sigS) {
    if (!pub65 || !hash32 || !sigR || !sigS || pub65[0] != 0x04) return false;

    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi r;
    mbedtls_mpi s;

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    int rc = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    if (rc == 0) {
      rc = mbedtls_ecp_point_read_binary(&grp, &Q, pub65, 65);
    }
    if (rc == 0) {
      rc = mbedtls_mpi_read_binary(&r, sigR, 32);
    }
    if (rc == 0) {
      rc = mbedtls_mpi_read_binary(&s, sigS, 32);
    }
    if (rc == 0) {
      rc = mbedtls_ecdsa_verify(&grp, hash32, 32, &Q, &r, &s);
    }

    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);

    return rc == 0;
  }

  class AttestWriteCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
      std::string v = c->getValue();
      if (v.size() != kAttestLen) {
        notifyStatus("ERR_LEN");
        return;
      }

      const uint8_t* data = (const uint8_t*)v.data();
      const uint8_t* vehicleId = data + 0;
      uint8_t slotId = data[8];
      const uint8_t* friendPub = data + 9; // unused for owner-only policy
      (void)friendPub;
      uint32_t validFrom = readU32BE(data + 0x4A);
      uint32_t validUntil = readU32BE(data + 0x4E);
      uint8_t entitlement = data[0x52];
      (void)entitlement;
      const uint8_t* sigR = data + 0x53;
      const uint8_t* sigS = data + 0x73;

      const char* vid = CCCMailbox::vehicleId();
      if (!vid || strlen(vid) != 8 || memcmp(vehicleId, vid, 8) != 0) {
        notifyStatus("ERR_VID");
        return;
      }

      uint8_t ownerPub[65];
      if (!CCCMailbox::getEndpointPub(ownerPub, sizeof(ownerPub))) {
        notifyStatus("ERR_NOT_PROVISIONED");
        return;
      }

      // Hash payload (bytes 0..82)
      uint8_t hash[32];
      mbedtls_sha256_context ctx;
      mbedtls_sha256_init(&ctx);
      mbedtls_sha256_starts(&ctx, 0);
      mbedtls_sha256_update(&ctx, data, kPayloadLen);
      mbedtls_sha256_finish(&ctx, hash);
      mbedtls_sha256_free(&ctx);

      if (!verifyRawSigP256(ownerPub, hash, sigR, sigS)) {
        notifyStatus("ERR_SIG");
        return;
      }

      uint32_t now = 0;
      if (!getEpochNow(now)) {
        notifyStatus("ERR_TIME");
        return;
      }
      if (validFrom != 0 && now < validFrom) {
        notifyStatus("ERR_TIME");
        return;
      }
      if (validUntil != 0 && now > validUntil) {
        notifyStatus("ERR_TIME");
        return;
      }

      if (slotId < 1 || slotId > 7) {
        notifyStatus("ERR_SLOT");
        return;
      }

      // Owner-only policy for now: reject slots 1-7 until sharing is enabled.
      notifyStatus("ERR_SLOT_LOCKED");
    }
  };
}

namespace BLEAttestation {
  void registerService(NimBLEServer* server) {
    NimBLEService* svc = server->getServiceByUUID(kAttestServiceUUID);
    if (!svc) {
      svc = server->createService(kAttestServiceUUID);
    }

    NimBLECharacteristic* cRx = svc->createCharacteristic(
        kAttestRxUUID, NIMBLE_PROPERTY::WRITE
    );
    static AttestWriteCallbacks rxCb;
    cRx->setCallbacks(&rxCb);

    g_cAttestTx = svc->createCharacteristic(
        kAttestTxUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    g_cAttestTx->setValue("READY");

    svc->start();

    Serial.println("[BLE] Digital Key attestation service registered");
    Serial.printf("[BLE] Attest Service UUID: %s\n", kAttestServiceUUID);
  }
}
