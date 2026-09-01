#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>

#include "../../include/ble/ble_admin.h"
#include "provisioning_phase.h"
#include "ccc_mailbox.h"
#include "../../include/nfc_session.h"
#include "../../include/ble/ble_auth.h"


namespace {
  // Admin service (preserved for NFC admin operations)
  const char* kAdminServiceUUID   = "9a9b9c9d-0000-4000-8000-9a9b9c9d0000";
  const char* kAdminModeCharUUID  = "9a9b9c9d-0001-4000-8000-9a9b9c9d0001"; // 0=normal,1=enroll,2=remove
  const char* kAdminCmdCharUUID   = "9a9b9c9d-0002-4000-8000-9a9b9c9d0002"; // write-only small commands
  const char* kAdminInfoCharUUID  = "9a9b9c9d-0003-4000-8000-9a9b9c9d0003"; // read/notify info
  const char* kAdminPhoneKeyUUID  = "9a9b9c9d-0004-4000-8000-9a9b9c9d0004"; // write-only: phone PEM (supports chunking)

  // Admin state
  BLEMod::AdminMode s_adminMode = BLEMod::ADMIN_NORMAL;
  NimBLECharacteristic* g_cAdminMode = nullptr;
  NimBLECharacteristic* g_cAdminInfo = nullptr;

  // Admin mode characteristic: read/write 1 byte
  class AdminModeCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
      std::string v = c->getValue(); if (v.empty()) return;
      uint8_t m = static_cast<uint8_t>(v[0]); if (m > 2) m = 0;
      s_adminMode = static_cast<BLEMod::AdminMode>(m);
      Serial.printf("[BLE-Admin] Mode set to %u\n", (unsigned)m);
      if (g_cAdminMode) g_cAdminMode->setValue(&m, 1);
    }
  };

  // Admin command characteristic
  class AdminCmdCallbacks : public NimBLECharacteristicCallbacks {
    NimBLECharacteristic* _info;
   public:
    explicit AdminCmdCallbacks(NimBLECharacteristic* info): _info(info) {}
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
      std::string v = c->getValue(); if (v.empty()) return;

      // Robust command parsing:
      // - If user writes a single byte, use that as the command.
      // - If user writes ASCII hex ("01","02","10","20","33".."36"), parse to the corresponding byte.
      // - Also accept "0xNN"/"0XNN" form.
      auto isHex = [](char c){ return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'); };
      auto hexVal = [](char c)->uint8_t{
        if (c>='0'&&c<='9') return (uint8_t)(c-'0');
        if (c>='a'&&c<='f') return (uint8_t)(10 + (c-'a'));
        if (c>='A'&&c<='F') return (uint8_t)(10 + (c-'A'));
        return 0;
      };
      uint8_t raw0 = static_cast<uint8_t>(v[0]);
      uint8_t cmd = raw0;
      bool asciiHexParsed = false;
      if (v.size() == 2 && isHex(v[0]) && isHex(v[1])) {
        cmd = (uint8_t)((hexVal(v[0]) << 4) | hexVal(v[1]));
        asciiHexParsed = true;
      } else if (v.size() == 4 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X') && isHex(v[2]) && isHex(v[3])) {
        cmd = (uint8_t)((hexVal(v[2]) << 4) | hexVal(v[3]));
        asciiHexParsed = true;
      }
      Serial.printf("[BLE-Admin] Cmd write: raw0=0x%02X len=%u parsedHex=%s -> cmd=0x%02X\n", raw0, (unsigned)v.size(), asciiHexParsed?"YES":"NO", cmd);

      switch (cmd) {
        case 0x01:
          // Clear phone provisioning only (equivalent to 'r')
          ProvisioningPhase::clearProvisionedOnly();
          _info->setValue("CLEARED_PROVISIONED");
          _info->notify();
          break;
        case 0x02:
          // Clear provisioning data (equivalent to 'C')
          ProvisioningPhase::clearAll();
          _info->setValue("CLEARED_ALL");
          _info->notify();
          break;
        case 0x10:
          // Listing tags not supported in new provisioning module
          _info->setValue("NOT_SUPPORTED");
          _info->notify();
          break;
        case 0x20:
          // Trigger NFC provisioning flow: our NFC loop auto-runs; we just inform the app
          _info->setValue("NFC_TAP_TO_PROVISION");
          _info->notify();
          Serial.println("[BLE Admin] Notify: Tap phone at reader to provision.");
          break;
        case 0x30: // Persistent force ON
          NfcSession::setPersistentForce(true);
          _info->setValue("FORCE_PERSIST_ON");
          _info->notify();
          break;
        case 0x31: // Persistent force OFF
          NfcSession::setPersistentForce(false);
          _info->setValue("FORCE_PERSIST_OFF");
          _info->notify();
          break;
        case 0x32: // Arm one-shot force
          NfcSession::armOneShotForce();
          _info->setValue("FORCE_ONESHOT_ARMED");
          _info->notify();
          break;
        case 0x33: { // Status query
          bool prov = ProvisioningPhase::isProvisioned();
          bool pf = NfcSession::getPersistentForce();
          bool os = NfcSession::isOneShotArmed();
          char buf[48];
          snprintf(buf, sizeof(buf), "STATUS P=%s PF=%s OS=%s", prov?"YES":"NO", pf?"ON":"OFF", os?"ARMED":"NO");
          _info->setValue(buf);
          _info->notify();
          Serial.printf("[BLE-Admin] %s\n", buf);
          break; }
        case 0x34: { // Validate cert vs pub
          bool ok = ProvisioningPhase::validateStoredCertMatchesStoredPub();
          _info->setValue(ok?"CERT_MATCH":"CERT_MISMATCH");
          _info->notify();
          Serial.printf("[BLE-Admin] Cert vs pub: %s\n", ok?"MATCH":"MISMATCH/NA");
          break; }
        case 0x35: { // Pub key presence
          uint8_t pub[65]; size_t len = ProvisioningPhase::getPhonePubRaw(pub, sizeof(pub));
          _info->setValue((len==65 && pub[0]==0x04)?"PUB_PRESENT":"PUB_NONE");
          _info->notify();
          Serial.printf("[BLE-Admin] Pub presence: %s\n", (len==65 && pub[0]==0x04)?"PRESENT":"NONE");
          break; }
        case 0x36: { // CCC mailbox summary
          const CCCMailbox::CCC_Mailbox& box = CCCMailbox::get();
          const char* vid = CCCMailbox::vehicleId();
          const char* vidSafe = (vid && vid[0] != '\0') ? vid : "--------";
          char buf[64];
          snprintf(buf, sizeof(buf), "CCC VID=%s SLOT=0x%02X EP=%s",
                   vidSafe, box.slot_bitmap, CCCMailbox::hasEndpointPub() ? "Y" : "N");
          _info->setValue(buf);
          _info->notify();
          Serial.printf("[BLE-Admin] %s\n", buf);
          break; }
        case 0x40: // Start Phase B authentication test
          _info->setValue("AUTH_TEST_READY");
          _info->notify();
          Serial.println("[BLE-Admin] Phase B authentication test mode activated");
          break;
        case 0x41: { // Check authentication status
          if (BLEAuth::isSessionReady()) {
            _info->setValue("AUTH_SESSION_READY");
          } else {
            _info->setValue("AUTH_SESSION_NOT_READY");
          }
          _info->notify();
          break; }
        case 0x42: // Reset authentication session
          BLEAuth::resetSession();
          _info->setValue("AUTH_SESSION_RESET");
          _info->notify();
          break;
        case 0x43: // Print auth statistics
          BLEAuth::printStats();
          _info->setValue("AUTH_STATS_PRINTED");
          _info->notify();
          break;
        default:
          _info->setValue("UNSUPPORTED");
          _info->notify();
          Serial.printf("[BLE-Admin] Unsupported cmd: 0x%02X (len=%u)\n", cmd, (unsigned)v.size());
          break;
      }
    }
  };

  // Admin phone public key upload (PEM). Supports chunked writes; commit when END line detected.
  class AdminPhoneKeyCallbacks : public NimBLECharacteristicCallbacks {
    NimBLECharacteristic* _info; std::string _buf;
   public:
    explicit AdminPhoneKeyCallbacks(NimBLECharacteristic* info): _info(info) {}
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
      // New provisioning flow does not accept phone pubkey via BLE; instruct to use NFC provisioning.
      (void)c; _buf.clear();
      _info->setValue("UNSUPPORTED_USE_NFC");
      _info->notify();
    }
  };
}

namespace BLEAdmin {
  void registerService(NimBLEServer* server) {
    NimBLEService* pAdmin = server->createService(kAdminServiceUUID);
    // Mode
    NimBLECharacteristic* cAdminMode = pAdmin->createCharacteristic(kAdminModeCharUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    static AdminModeCallbacks adminModeCb; cAdminMode->setCallbacks(&adminModeCb);
    uint8_t initAdmin = (uint8_t)s_adminMode; cAdminMode->setValue(&initAdmin, 1); g_cAdminMode = cAdminMode;
    // Info
    NimBLECharacteristic* cAdminInfo = pAdmin->createCharacteristic(kAdminInfoCharUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    cAdminInfo->setValue("READY"); g_cAdminInfo = cAdminInfo;
    // Commands
    NimBLECharacteristic* cAdminCmd = pAdmin->createCharacteristic(kAdminCmdCharUUID, NIMBLE_PROPERTY::WRITE);
    static AdminCmdCallbacks adminCmdCb(cAdminInfo); cAdminCmd->setCallbacks(&adminCmdCb);
    // Phone public key upload
    NimBLECharacteristic* cPhoneKey = pAdmin->createCharacteristic(kAdminPhoneKeyUUID, NIMBLE_PROPERTY::WRITE);
    static AdminPhoneKeyCallbacks phoneKeyCb(cAdminInfo); cPhoneKey->setCallbacks(&phoneKeyCb);
    pAdmin->start();
  }

  BLEMod::AdminMode getAdminMode() { return s_adminMode; }
  void setAdminMode(BLEMod::AdminMode mode) { s_adminMode = mode; uint8_t m = (uint8_t)mode; if (g_cAdminMode) g_cAdminMode->setValue(&m, 1); }
  void notify(const char* msg) { if (g_cAdminInfo && msg) { g_cAdminInfo->setValue(msg); g_cAdminInfo->notify(); } }
}
