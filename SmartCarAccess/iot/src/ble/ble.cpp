// BLE entrypoint: initializes NimBLE and registers services.
// Heavy service logic is split into modules under src/ble_*.cpp for readability.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <cstring>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

#include "ble/ble.h"
#include "ble/ble_rollout.h"
#include "ble/pke_telemetry.h"
#include "ble/ble_admin.h"
#include "ble/ble_auth.h"
#include "ble/ble_attestation.h"
#include "ble/ble_echo.h"
#include "ccc_mailbox.h"

namespace {
  // Device name and services
  const char* kDeviceName = "ESP-Smart-Car-ECU";
  constexpr uint8_t kOwnAddrTypeRpaPublicDefault = 0x02;
  constexpr uint8_t kSmIoCapNoIo = 0x03;
  constexpr uint8_t kSmKeyDistEnc = 0x01;
  constexpr uint8_t kSmKeyDistId = 0x02;
  constexpr uint32_t kAdvDiagPeriodMs = 5000;

  bool s_bleStarted = false;

  enum class AdvProfile : uint8_t {
    Fast,
    Slow,
  };

  struct AdvState {
    AdvProfile profile = AdvProfile::Slow;
    uint32_t fastWindowDeadlineMs = 0;
    uint32_t profileSinceMs = 0;
    uint32_t lastDiagMs = 0;
    uint32_t transitionCount = 0;
    bool profileInitialized = false;
    char lastReason[40] = "unset";
  };

  AdvState s_advState;

  uint16_t advMsToUnits(uint32_t intervalMs) {
    const uint32_t units = (intervalMs * 8U + 2U) / 5U;  // Round to 0.625ms units.
    if (units < 0x0020) {
      return 0x0020;
    }
    if (units > 0x4000) {
      return 0x4000;
    }
    return static_cast<uint16_t>(units);
  }

  bool fastWindowActive(uint32_t nowMs) {
    if (s_advState.fastWindowDeadlineMs == 0) {
      return false;
    }
    return static_cast<int32_t>(s_advState.fastWindowDeadlineMs - nowMs) > 0;
  }

  void logAdvertisingDiagnostics(const BLERollout::Flags& flags, uint32_t nowMs, const char* trigger) {
    if (!s_advState.profileInitialized) {
      return;
    }

    const bool useFast = s_advState.profile == AdvProfile::Fast;
    const uint16_t minMs = useFast ? flags.advFastMinMs : flags.advSlowMinMs;
    const uint16_t maxMs = useFast ? flags.advFastMaxMs : flags.advSlowMaxMs;
    const uint32_t elapsedMs = nowMs - s_advState.profileSinceMs;
    const bool fastWindowIsActive = fastWindowActive(nowMs);
    const uint32_t fastWindowRemainingMs = fastWindowIsActive
      ? (s_advState.fastWindowDeadlineMs - nowMs)
      : 0;
    const char* safeTrigger = (trigger != nullptr) ? trigger : "unspecified";

    Serial.printf(
      "[BLE][ADV][DIAG] trigger=%s profile=%s interval_ms=%u-%u elapsed_ms=%lu fast_window_active=%u fast_window_remaining_ms=%lu transitions=%lu last_reason=%s\n",
      safeTrigger,
      useFast ? "fast" : "slow",
      minMs,
      maxMs,
      static_cast<unsigned long>(elapsedMs),
      fastWindowIsActive ? 1 : 0,
      static_cast<unsigned long>(fastWindowRemainingMs),
      static_cast<unsigned long>(s_advState.transitionCount),
      s_advState.lastReason
    );
  }

  void applyAdvertisingProfile(
    NimBLEAdvertising* advertising,
    const BLERollout::Flags& flags,
    AdvProfile profile,
    const char* reason,
    uint32_t nowMs
  ) {
    const bool useFast = profile == AdvProfile::Fast;
    const uint16_t minMs = useFast ? flags.advFastMinMs : flags.advSlowMinMs;
    const uint16_t maxMs = useFast ? flags.advFastMaxMs : flags.advSlowMaxMs;
    const bool profileChanged = !s_advState.profileInitialized || s_advState.profile != profile;
    const char* safeReason = (reason != nullptr) ? reason : "unspecified";

    advertising->setMinInterval(advMsToUnits(minMs));
    advertising->setMaxInterval(advMsToUnits(maxMs));

    if (profileChanged) {
      Serial.printf(
        "[BLE][ADV] profile=%s reason=%s interval_ms=%u-%u\n",
        useFast ? "fast" : "slow",
        safeReason,
        minMs,
        maxMs
      );

      s_advState.profileSinceMs = nowMs;
      s_advState.transitionCount++;
      strncpy(s_advState.lastReason, safeReason, sizeof(s_advState.lastReason) - 1);
      s_advState.lastReason[sizeof(s_advState.lastReason) - 1] = '\0';

      logAdvertisingDiagnostics(flags, nowMs, "transition");
    }

    s_advState.profile = profile;
    s_advState.profileInitialized = true;
  }

  void applyAdvertisingPolicy(bool requestFastProfile, const char* reason) {
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (!advertising) {
      return;
    }

    BLERollout::Flags flags = BLERollout::flags();
    const uint32_t nowMs = millis();
    const char* safeReason = (reason != nullptr) ? reason : "unspecified";

    if (requestFastProfile) {
      if (!fastWindowActive(nowMs)) {
        s_advState.fastWindowDeadlineMs = nowMs + flags.advFastWindowMs;
        Serial.printf(
          "[BLE][ADV] fast_window_started reason=%s window_ms=%lu deadline_ms=%lu\n",
          safeReason,
          static_cast<unsigned long>(flags.advFastWindowMs),
          static_cast<unsigned long>(s_advState.fastWindowDeadlineMs)
        );
      } else {
        Serial.printf(
          "[BLE][ADV] fast_window_guard reason=%s remaining_ms=%lu (not extended)\n",
          safeReason,
          static_cast<unsigned long>(s_advState.fastWindowDeadlineMs - nowMs)
        );
      }
    }

    const AdvProfile targetProfile = fastWindowActive(nowMs) ? AdvProfile::Fast : AdvProfile::Slow;
    applyAdvertisingProfile(advertising, flags, targetProfile, safeReason, nowMs);
    NimBLEDevice::startAdvertising();

    if (s_advState.lastDiagMs == 0) {
      s_advState.lastDiagMs = nowMs;
    }
  }

  void logRolloutFlags() {
    BLERollout::Flags f = BLERollout::flags();
    Serial.printf(
      "[BLE][ROLL] flags background=%u fast_tx=%u bonding_enforce=%u rssi_monitor_only=%u rssi_threshold_dbm=%d adv_fast_ms=%u-%u adv_slow_ms=%u-%u adv_fast_window_ms=%lu\n",
      f.backgroundMode ? 1 : 0,
      f.fastTransaction ? 1 : 0,
      f.bondingEnforce ? 1 : 0,
      f.rssiMonitorOnly ? 1 : 0,
      f.rssiThresholdDbm,
      f.advFastMinMs,
      f.advFastMaxMs,
      f.advSlowMinMs,
      f.advSlowMaxMs,
      static_cast<unsigned long>(f.advFastWindowMs)
    );
  }

  void configureSecurity() {
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(kSmIoCapNoIo);
    NimBLEDevice::setSecurityInitKey(kSmKeyDistEnc | kSmKeyDistId);
    NimBLEDevice::setSecurityRespKey(kSmKeyDistEnc | kSmKeyDistId);

    bool rpaOk = NimBLEDevice::setOwnAddrType(kOwnAddrTypeRpaPublicDefault);
    Serial.printf("[BLE][SEC] own_addr_type=RPA_PUBLIC_DEFAULT set=%s\n", rpaOk ? "OK" : "FAIL");
    Serial.println("[BLE][SEC] pairing config: BOND=1 MITM=0 SC=1 IO=NO_INPUT_OUTPUT KEY_DIST=ENC|ID");
  }

  void logBondTable() {
    int count = NimBLEDevice::getNumBonds();
    Serial.printf("[BLE][SEC] restored_bonds=%d\n", count);
    for (int i = 0; i < count; ++i) {
      NimBLEAddress addr = NimBLEDevice::getBondedAddress(i);
      Serial.printf("[BLE][SEC] restored_bond[%d]=%s\n", i, addr.toString().c_str());
    }
  }
}

namespace BLEMod {
  void begin() {
    NimBLEDevice::init(kDeviceName);
    logRolloutFlags();
    configureSecurity();
    logBondTable();

    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setMTU(512);  // Increased from 185 to reduce packet count for certificate chain

    // Init RNG for ephemeral keys and nonces
    static mbedtls_entropy_context s_entropy;
    static mbedtls_ctr_drbg_context s_drbg;
    mbedtls_entropy_init(&s_entropy);
    mbedtls_ctr_drbg_init(&s_drbg);
    const char* pers = "ble-auth";
    mbedtls_ctr_drbg_seed(&s_drbg, mbedtls_entropy_func, &s_entropy,
                          (const unsigned char*)pers, strlen(pers));

    NimBLEServer *pServer = NimBLEDevice::createServer();
    // Keep advertising after connects and restart after disconnects
    class ServerCallbacks : public NimBLEServerCallbacks {
      void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.println("[BLE] Central connected. Keeping advertising active.");
        PKETelemetry::startAttempt(CCCMailbox::vehicleId());
        PKETelemetry::emit(PKETelemetry::Event::Connect);
        Serial.printf(
          "[BLE][SEC] onConnect peer=%s id=%s bonded=%u enc=%u auth=%u\n",
          connInfo.getAddress().toString().c_str(),
          connInfo.getIdAddress().toString().c_str(),
          connInfo.isBonded() ? 1 : 0,
          connInfo.isEncrypted() ? 1 : 0,
          connInfo.isAuthenticated() ? 1 : 0
        );
        if (!connInfo.isBonded()) {
          Serial.println("[BLE][SEC] POLICY progressive: unbonded peer allowed during rollout");
        }
        bool secStarted = NimBLEDevice::startSecurity(connInfo.getConnHandle());
        Serial.printf("[BLE][SEC] startSecurity requested=%s conn_handle=%u\n",
                      secStarted ? "YES" : "NO",
                      connInfo.getConnHandle());
        
        // Update connection parameters for lower latency
        // Interval: 15ms (12 * 1.25ms), Latency: 0, Timeout: 2000ms (200 * 10ms)
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 12, 0, 200);
        Serial.println("[BLE] Requested connection params: interval=15ms, latency=0, timeout=2000ms");
        
        BLEMod::restartAdvertising(false, "connect_keepalive");
      }
      void onDisconnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& /*connInfo*/, int reason) override {
        Serial.printf("[BLE] Central disconnected (reason=%d). Restarting advertising and resetting session.\n", reason);
        BLEAuth::resetSession();
        BLEMod::restartAdvertising(true, "server_disconnect");
      }
      
      void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE] MTU negotiated: %u bytes (conn_handle=%u)\n", MTU, connInfo.getConnHandle());
      }

      void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        Serial.printf(
          "[BLE][SEC] auth_complete peer=%s bonded=%u enc=%u auth=%u key_size=%u\n",
          connInfo.getAddress().toString().c_str(),
          connInfo.isBonded() ? 1 : 0,
          connInfo.isEncrypted() ? 1 : 0,
          connInfo.isAuthenticated() ? 1 : 0,
          connInfo.getSecKeySize()
        );
      }

      void onIdentity(NimBLEConnInfo& connInfo) override {
        Serial.printf(
          "[BLE][SEC] identity_resolved ota=%s id=%s\n",
          connInfo.getAddress().toString().c_str(),
          connInfo.getIdAddress().toString().c_str()
        );
      }
    };
    static ServerCallbacks s_serverCbs;
    pServer->setCallbacks(&s_serverCbs);

    // Register services (Auth/Echo share the same service UUID)
    BLEAdmin::registerService(pServer);
    BLEEcho::registerService(pServer, &s_drbg);
    BLEAuth::registerService(pServer, &s_drbg);
    BLEAttestation::registerService(pServer);

    if (pServer->getServiceByUUID("9a9b9c9d-0000-4000-8000-9a9b9c9d0000")) Serial.println("[BLE] Admin service registered");
    if (pServer->getServiceByUUID("0000aaaa-1234-5678-9abc-def012345678")) Serial.println("[BLE] Auth service registered");
    if (pServer->getServiceByUUID("555a0001-00aa-1111-2222-333344445555")) Serial.println("[BLE] Attestation service registered");

    // Start the Auth service once after all characteristics are added
    if (auto pAuth = pServer->getServiceByUUID("0000aaaa-1234-5678-9abc-def012345678")) {
      pAuth->start();
    }

    // Advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    
    // 1. Build the primary packet (This MUST contain the UUID for background wake-up)
    NimBLEAdvertisementData advData; 
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    
    // Add the UUID directly into the manual payload
    advData.setCompleteServices(NimBLEUUID("0000aaaa-1234-5678-9abc-def012345678")); 
    pAdvertising->setAdvertisementData(advData);

    // 2. Build the scan response (Contains the name to save space)
    NimBLEAdvertisementData scanResp; 
    scanResp.setName(kDeviceName); 
    pAdvertising->setScanResponseData(scanResp);

    // Start advertising
    restartAdvertising(true, "boot");

    Serial.printf("[BLE] Advertising started: %s\n", kDeviceName);
    s_bleStarted = true;
  }

  void tick() {
    if (!s_bleStarted) {
      return;
    }

    BLERollout::Flags flags = BLERollout::flags();
    const uint32_t nowMs = millis();

    if (s_advState.profileInitialized && (nowMs - s_advState.lastDiagMs) >= kAdvDiagPeriodMs) {
      logAdvertisingDiagnostics(flags, nowMs, "periodic");
      s_advState.lastDiagMs = nowMs;
    }

    if (!fastWindowActive(nowMs) && s_advState.profileInitialized && s_advState.profile == AdvProfile::Fast) {
      applyAdvertisingPolicy(false, "fast_window_elapsed");
    }
  }

  void restartAdvertising(bool requestFastProfile, const char* reason) {
    applyAdvertisingPolicy(requestFastProfile, reason);
  }

  bool isStarted() { return s_bleStarted; }
  const char* deviceName() { return kDeviceName; }
  bool isSessionReady() { return BLEAuth::isSessionReady(); }

  BLEMod::AdminMode getAdminMode() { return BLEAdmin::getAdminMode(); }
  void setAdminMode(BLEMod::AdminMode mode) { BLEAdmin::setAdminMode(mode); }
  void adminNotify(const char* msg) { BLEAdmin::notify(msg); }
}
