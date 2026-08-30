#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include <mbedtls/ctr_drbg.h>

#include "ble/utils/crypto_utils.h"
#include "ble/ble_auth.h"
#include "ble/ble_echo.h"

namespace {
  const char* kCharSecEchoInUUID  = "d0d0d0d0-0005-4000-8000-d0d0d0d00005"; // Write: encrypted echo frame
  const char* kCharSecEchoOutUUID = "d0d0d0d0-0006-4000-8000-d0d0d0d00006"; // Read/Notify: encrypted echo response

  NimBLECharacteristic* g_cSecEchoOut = nullptr;
  mbedtls_ctr_drbg_context* s_drbg = nullptr;

  // AUTH: secure echo (AES-256-GCM using K_enc, AAD="ECHO1")
  class SecureEchoInCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
      if (!BLEAuth::isSessionReady()) return;
      std::string v = c->getValue();
      const uint8_t* frame = (const uint8_t*)v.data(); size_t flen = v.size();
      if (flen < 12 + 16) return; // nonce(12) + tag(16) minimal
      const uint8_t* nonce = frame; size_t nlen = 12;
      const uint8_t* ct = frame + nlen; size_t ctlen = flen - nlen - 16;
      const uint8_t* tag = frame + nlen + ctlen; size_t tlen = 16;

      uint8_t aad[] = {'E','C','H','O','1'}; size_t aadLen = sizeof(aad);

      // Decrypt
      std::string pt; pt.resize(ctlen);
      const uint8_t* K = BLEAuth::sessionEncKey();
      if (!aes_gcm_decrypt(K, nonce, aad, aadLen, ct, ctlen, tag, (uint8_t*)pt.data())) { return; }

      // Build response plaintext: "ECHO:" + pt
      std::string respPt = "ECHO:"; respPt.append(pt);

      // Encrypt response with fresh nonce
      uint8_t nonceOut[12];
      mbedtls_ctr_drbg_random(s_drbg, nonceOut, sizeof(nonceOut));
      std::string ctOut; ctOut.resize(respPt.size());
      uint8_t tagOut[16];
      if (!aes_gcm_encrypt(K, nonceOut, aad, aadLen, (const uint8_t*)respPt.data(), respPt.size(), (uint8_t*)ctOut.data(), tagOut)) { return; }

      // Frame = nonce || ciphertext || tag
      std::string frameOut; frameOut.reserve(sizeof(nonceOut) + ctOut.size() + sizeof(tagOut));
      frameOut.append((const char*)nonceOut, sizeof(nonceOut));
      frameOut.append(ctOut);
      frameOut.append((const char*)tagOut, sizeof(tagOut));
      if (g_cSecEchoOut) { g_cSecEchoOut->setValue(frameOut); g_cSecEchoOut->notify(); }
    }
  };
}

namespace BLEEcho {
  void registerService(NimBLEServer* server, mbedtls_ctr_drbg_context* drbg) {
    s_drbg = drbg;
    NimBLEService* pAuth = server->getServiceByUUID("d0d0d0d0-0000-4000-8000-d0d0d0d00000");
    if (!pAuth) { pAuth = server->createService("d0d0d0d0-0000-4000-8000-d0d0d0d00000"); }

    NimBLECharacteristic* cEchoIn = pAuth->createCharacteristic(kCharSecEchoInUUID, NIMBLE_PROPERTY::WRITE);
    static SecureEchoInCallbacks secEchoCb; cEchoIn->setCallbacks(&secEchoCb);
  g_cSecEchoOut = pAuth->createCharacteristic(kCharSecEchoOutUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  }
}
