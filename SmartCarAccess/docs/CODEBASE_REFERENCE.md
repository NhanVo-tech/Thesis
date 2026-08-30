# CODEBASE_REFERENCE.md

> Complete onboarding and context document for the **ESP32 Smart Car Access** project.
> Written for future AI-assisted development sessions. Read this first before touching source.

---

## 1. Project Overview

### 1.1 Purpose / Domain
This project implements the **in-vehicle side of a digital car key system** inspired by the **CCC (Car Connectivity Consortium) Digital Key Release 3** specification, together with a companion **Android/Flutter application** for provisioning, authentication, and user-facing controls.

The system solves **secure, multi-factor, multi-transport vehicle access**:

1. **NFC** performs initial owner provisioning (enrollment of the owner's key).
2. **BLE** performs authenticated session establishment ("Passive Keyless Entry" / PKE).
3. **UWB** performs physical proximity verification (secure ranging).
4. An on-device **LSTM (TinyML)** model classifies user behaviour (normal walk-in, loitering, relay attack) to defend against **relay/distance-spoofing attacks**.

Two invariant security rules drive the design:
1. Immobilizer secrets never leave the vehicle.
2. The vehicle trusts only cryptographically verifiable data.

### 1.2 Tech Stack Summary

| Layer | Language / Framework | Key Libraries / Tools |
|-------|----------------------|------------------------|
| Vehicle firmware | C++17, Arduino framework on ESP32-S3 | PlatformIO, NimBLE-Arduino, PN532, Kalman, TensorFlowLite_ESP32 (TFLM), mbedTLS |
| Mobile app | Dart / Flutter | flutter_blue_plus, nfc_manager, firebase (core/auth/firestore), geolocator, pointycastle, cryptography, flutter_local_notifications |
| Mobile native | Kotlin (Android) | Android Keystore, HostApduService (HCE), MethodChannel bridges |
| AI / tooling | Python | matplotlib, pyserial (real-time visualizer, CSV logger) |
| Cloud | Firebase | Firebase Auth, Cloud Firestore (metadata / anomaly logs only — never immobilizer tokens) |
| AI (cloud, app-side) | Google Gemini API | `gemini-2.5-flash-lite` for access-pattern anomaly scoring |

### 1.3 High-Level Architecture

```
   Phone (Flutter + Kotlin)                    Vehicle (ESP32-S3 firmware)
 ┌───────────────────────────┐              ┌────────────────────────────────┐
 │ HCE Applet (NFC)           │──Phase A────▶│ NfcSession (PN532 reader)      │
 │ Android Keystore (P-256)   │   NFC APDU   │ ProvisioningPhase / CCCMailbox │
 │ PkeAuthOrchestrator (BLE)  │──Phase B────▶│ BLEMod (NimBLE GATT services)  │
 │ GpsService / UwbService    │   BLE GATT   │ FSM (state machine)            │
 └───────────────────────────┘              │ UciSessionManager (UWB ranging)│
             │                              │  → Kalman → LstmInference       │
             │ Firebase / Gemini            │  → UwbDoorUnlock (relay)        │
             ▼                              └────────────────────────────────┘
   Cloud (Firestore, Gemini)                         nRF52840 + DW3000 (UWB radio)
```

Data flow (high level):
1. NFC provisioning establishes owner keys.
2. BLE authentication establishes a trusted session (ECDH + signature).
3. UWB ranging measures proximity.
4. Kalman + LSTM pipeline classifies user behaviour.
5. The door relay fires only when **both** cryptographic and physical criteria are met.

---

## 2. Directory & File Map

```
SmartCarAccess-main/
├── README.md                     # Top-level academic README (system overview, build)
├── PHASE_A_AND_PHASE_B.md        # Provisioning (A) + BLE auth (B) protocol notes
├── UX_UI_IMPROVEMENTS.md         # App UX/UI change log
├── docs/                         # ← THIS documentation set
│
├── iot/                          # ===== ESP32-S3 VEHICLE FIRMWARE (PlatformIO) =====
│   ├── platformio.ini            # Build config: env=yolo_uno, build flags, lib_deps  [CONFIG]
│   ├── README.md                 # Firmware-specific README
│   ├── PAPER.md                  # Academic paper draft
│   ├── IMPLEMENTATION_SUMMARY.md # UWB/AI implementation notes
│   ├── uwb_lstm_data_label{0,1,2}.csv  # Training datasets (0=walk,1=loiter,2=attack)
│   ├── boards/yolo_uno.json      # Custom board definition
│   │
│   ├── include/                  # Public headers (API surface)
│   │   ├── ccc_mailbox.h         # CCC confidential storage structs + API      [KEY MODULE]
│   │   ├── nfc_session.h         # NFC (PN532) provisioning loop API
│   │   ├── provisioning_phase.h  # Phase A provisioning + key storage API      [KEY MODULE]
│   │   ├── ble/
│   │   │   ├── ble.h             # BLE entrypoint (begin/tick, admin mode enum)
│   │   │   ├── ble_auth.h        # Phase B auth service + session key accessors [KEY MODULE]
│   │   │   ├── ble_admin.h       # Admin GATT service (mode, cmd, phone key upload)
│   │   │   ├── ble_attestation.h # Digital Key attestation service
│   │   │   ├── ble_echo.h        # AES-GCM secure echo service
│   │   │   ├── ble_rollout.h     # Compile-time PKE rollout flags (structs)     [CONFIG]
│   │   │   └── pke_telemetry.h   # PKE unlock telemetry event API
│   │   ├── fsm/
│   │   │   ├── fsm_states.h      # State/Event/ErrorCode enums + StateContext   [KEY MODULE]
│   │   │   ├── fsm.h             # FSM public API (begin/tick/triggerEvent)     [KEY MODULE]
│   │   │   └── fsm_integration.h # NFC/BLE/Unlock→FSM event bridge wrappers
│   │   └── uwb/
│   │       ├── uci_uart_link.h        # UCI packet framing over UART (Mt/UciPacket)
│   │       ├── uci_session_manager.h  # UWB session lifecycle + UciRunConfig    [KEY MODULE]
│   │       ├── uci_oob.h              # OOB payload (UciOobPayloadV1) parse/map
│   │       ├── uci_host_bridge.h      # Host-facing UWB control (start/stop/OOB)
│   │       ├── uci_door_unlock.h      # Hysteresis + AI-gated relay logic        [KEY MODULE]
│   │       ├── lstm_inference.h       # TFLM LSTM inference class                [KEY MODULE]
│   │       └── uwb_lstm_model.h       # Flatbuffer model bytes (generated)
│   │
│   ├── src/                       # Implementations mirroring include/
│   │   ├── main.cpp              # ← FIRMWARE ENTRY POINT (setup/loop, FreeRTOS tasks) [ENTRY]
│   │   ├── ccc_mailbox.cpp
│   │   ├── nfc_session.cpp
│   │   ├── provisioning_phase.cpp
│   │   ├── ble/{ble,ble_auth,ble_admin,ble_attestation,ble_echo,pke_telemetry}.cpp
│   │   ├── fsm/{fsm,fsm_states,fsm_integration,fsm_build_verify}.cpp
│   │   ├── uwb/{uci_uart_link,uci_session_manager,uci_oob,uci_host_bridge,
│   │   │        uci_door_unlock,lstm_inference}.cpp
│   │   └── test/{test_fsm.cpp,test_fsm.h}    # Native FSM unit tests
│   │
│   ├── lib/                       # Vendored libraries
│   │   ├── Kalman/Kalman.h        # 1-D Kalman (low-pass) filter                 [KEY MODULE]
│   │   └── PN532/                 # PN532 NFC driver (Seeed fork)
│   │
│   └── tools/                     # Python tooling
│       ├── serial_csv_logger.py       # Capture [LSTM_DATA] lines → CSV for ML
│       ├── realtime_lstm_visualizer.py# Live 3-subplot academic plotter (PDF/SVG export)
│       ├── demo_ble_auth.py           # BLE Phase B demo/test client
│       ├── phase_b_test.py            # Phase B test harness
│       ├── setup.sh                   # Tooling setup
│       └── requirements.txt           # Python deps
│
├── thesis253_workspace/          # ===== ESP32-C3 UCI BRIDGE FIRMWARE (Stage 1.5) =====
│   ├── platformio.ini            # Build: esp32-c3-devkitm-1, ANCHOR_ID, MASTER_MAC
│   ├── include/
│   │   ├── espnow_link.h         # ESP-NOW: receive StartMsg, send RangingMsg
│   │   ├── uci_session.h         # UCI session: Config struct + run()
│   │   └── uci_uart.h            # UART transport: sendCommand, waitResponse, poll
│   └── src/
│       ├── main.cpp              # setup(): UART+ESP-NOW init, uciTask + loop poll
│       ├── espnow_link.cpp       # StartMsg(34B) ↔ RangingMsg structs
│       ├── uci_session.cpp       # buildAppConfig (21 TLVs), run (init→config→start)
│       └── uci_uart.cpp          # Raw UCI framing, RANGE_DATA_NTF parse
│
└── software/
    ├── android_software.txt
    └── smart_car_app/            # ===== FLUTTER MOBILE APP =====
        ├── pubspec.yaml          # Dart deps + assets                          [CONFIG]
        ├── analysis_options.yaml # Lint config
        ├── lib/
        │   ├── main.dart              # ← APP ENTRY POINT (Firebase, HCE, notif) [ENTRY]
        │   ├── main_gps_test.dart     # Standalone GPS test entry point
        │   ├── test_anomaly_notifications.dart  # Anomaly notification test harness
        │   ├── screen/           # 15 UI screens (dashboard, login, test harnesses…)
        │   ├── service/          # 26 services (BLE, GPS, NFC, anomaly, Firebase…)
        │   ├── theme/app_colors.dart  # Color palette + Material 3 theme
        │   └── widgets/          # Reusable UI components
        ├── android/app/src/main/{java,kotlin}/…  # Kotlin native (HCE, Keystore, bridges)
        ├── packages/
        │   ├── phaseb_handshake_bridge/  # Local plugin: Phase B BLE crypto
        │   └── nfc_manager/              # Local nfc_manager override
        └── {ios,linux,macos,windows,web}/ # Other Flutter platform folders
```

**Entry points**
- Firmware: [iot/src/main.cpp](../iot/src/main.cpp) — `setup()` / `loop()`.
- App: [software/smart_car_app/lib/main.dart](../software/smart_car_app/lib/main.dart) — `main()` + `hceMain()` background isolate.
- GPS test app: `lib/main_gps_test.dart` (run with `flutter run -t lib/main_gps_test.dart`).

**Config files**
- [iot/platformio.ini](../iot/platformio.ini) — firmware build environment & rollout flags.
- [iot/include/ble/ble_rollout.h](../iot/include/ble/ble_rollout.h) — compile-time PKE defaults.
- [software/smart_car_app/pubspec.yaml](../software/smart_car_app/pubspec.yaml) — app dependencies.

---

## 3. Module / Component Breakdown

### 3.1 Firmware — Application Entry (`iot/src/main.cpp`)
- **Purpose**: Boots all subsystems and runs them under FreeRTOS tasks pinned to core 1.
- **Key functions**:
  - `setup()` — Initializes `CCCMailbox`, `FSM`, `BLEMod`, `NfcSession` (Serial2, RX=44 TX=43), `UciUartLink` (Serial1, RX=17 TX=18 @ 115200), `UciSessionManager`, `UwbUciHost`, `UwbDoorUnlock`. Spawns tasks `FSMTask` (prio 6), `NFCTask` (prio 4), `UWBTask` (prio 5, 20 KB stack).
  - `loop()` — Calls `BLEMod::tick()` (advertising demotion), `handleConsole()`, idles 50 ms.
  - `handleConsole()` — Serial console: `uci_run XX:XX`, `uci_help`.
  - `parseMacShort(String, uint16_t*)` — Parse `XX:XX` short MAC.
  - `runUciDemo(const String&)` — Build `UciRunConfig` and call `runOnce`.
  - Tasks: `fsmTask`, `nfcTask`, `uwbTask` (polls UCI, host bridge, door tick).
- **Dependencies**: all firmware modules.
- **Side effects**: Serial I/O, GPIO/UART init, FreeRTOS task creation, hardware bring-up.

### 3.2 Firmware — CCC Mailbox (`ccc_mailbox.h/.cpp`)
- **Purpose**: Vehicle confidential storage (root of trust), persisted in ESP32 NVS namespace `ccc_dk`, mirrored in RAM.
- **Key structs**: `CCC_Slot`, `CCC_Mailbox` (see §4).
- **Key functions**: `begin()`, `get()`, `vehicleId()`, `hasVehiclePub()/getVehiclePub()`, `hasVehiclePriv()`, `signVehicleDataP256(...)` (ECDSA-P256 DER sign with vehicle private key), `hasEndpointPub/getEndpointPub/setEndpointPub/clearEndpointPub(slot)`, `isSlotActive/setSlotActive`, `hasToken/getToken/setToken/clearToken/ensureToken(slot)`, `signalingBitmap/setSignalingBitmap/setSignalingFlag`, `clearMailboxes()`, `clearAll()`.
- **Inputs/Outputs**: 65-byte public keys, 32-byte tokens, 8-slot model (slot 0 = owner, 1–7 = friends).
- **Side effects**: NVS reads/writes, mbedTLS RNG seeding for vehicle signing. On first boot generates `v_id` (8 bytes) + `v_pub` (65 bytes).

### 3.3 Firmware — NFC Session (`nfc_session.h/.cpp`)
- **Purpose**: Drives the PN532 as an NFC reader and runs the Phase A APDU provisioning loop against the phone's HCE applet.
- **Key functions**: `begin(HardwareSerial&, rxPin, txPin, baud)`, `tick()`, `setPersistentForce(bool)`, `armOneShotForce()`, `getPersistentForce()`, `isOneShotArmed()`.
- **Side effects**: UART to PN532 (HSU), APDU exchange, calls into `ProvisioningPhase`/`CCCMailbox`, emits FSM events.

### 3.4 Firmware — Provisioning Phase (`provisioning_phase.h/.cpp`)
- **Purpose**: Phase A owner enrollment logic and key persistence.
- **Key functions**: `begin()`, `isProvisioned()`, `storePhonePubRaw(pub65)`, `storeCertChain(cert,len)`, `storeFastArtifact/hasFastArtifact/getFastArtifact/clearFastArtifact`, `setOwnerProvisioned(pub65, force)`, `verifySignatureP256(pub65,data,len,sigDer,sigLen)`, `clearAll/clearProvisionedOnly/clearProvisionedData`, force-provisioning controls, read-back helpers, `validateCertPublicKeyMatchesPub(...)`. Deprecated: `runOnceWithHce`, `getDevicePrivateKeyPEM`.
- **Side effects**: NVS writes, ECDSA verification via mbedTLS.

### 3.5 Firmware — BLE Stack (`ble/`)
- **`ble` (BLEMod)**: NimBLE entrypoint. `begin()`, `tick()` (fast→slow advertising demotion), `restartAdvertising(fast,reason)`, `isStarted()`, `deviceName()`, `AdminMode` enum + get/set, `adminNotify(msg)`, `isSessionReady()`. Registers all four GATT services.
- **`ble_auth` (BLEAuth)**: Phase B / CCC tunnel. `registerService(server, drbg)`, session-key accessors (`sessionEncKey/Len`, `sessionMacKey/Len`), `isSessionReady()`, `resetSession()`, `setGpsDataCallback(cb)`, `printStats()`. Handles APDU-like AUTH0/AUTH1/EXCHANGE/CONTROL_FLOW; does ECDH + HKDF; decrypts GPS.
- **`ble_admin` (BLEAdmin)**: `registerService(server)`, `getAdminMode/setAdminMode`, `notify(msg)`. Phone-key upload with chunking.
- **`ble_attestation` (BLEAttestation)**: `registerService(server)` — Auth_RX/Auth_TX attestation channel.
- **`ble_echo` (BLEEcho)**: `registerService(server, drbg)` — AES-GCM secure echo using BLEAuth session key.
- **`pke_telemetry` (PKETelemetry)**: `startAttempt`, `attemptId`, `setVehicleId`, `emit(event, rssiDbm, decision, details)`, `eventName(event)`; `Event` enum (ScanWake…UnlockDecision).
- **Side effects**: BLE radio, GATT notifications, mbedTLS CTR-DRBG.

### 3.6 Firmware — FSM (`fsm/`)
- **Purpose**: Central state machine coordinating provisioning, auth, unlock, admin, error handling.
- **`fsm_states.h`**: `State`, `Event`, `ErrorCode` enums; `StateContext`, `StateInfo`, `StateTransition` structs; `StateAction` callback typedef; `stateToString/eventToString/errorToString`.
- **`fsm.h` (FSM)**: `begin()`, `tick()`, `triggerEvent(e)`, `triggerEventWithData(e,data,len)`, `getCurrentState()`, `getStateInfo()`, `forceState(s,force)`, `reset(clearSession)`, group predicates (`isProvisioning/isAuthenticating/isUnlocking/isInErrorState`), timeout controls, entry/exit callback registration, event-queue status, debug/logging, `printStatus/printTransitionTable/validateConfiguration`.
- **`fsm_integration.h` (FSMIntegration)**: Namespaced bridges `NFC::…`, `BLE::…`, `Unlock::…` that translate subsystem callbacks into FSM events.
- **Side effects**: Event queue mutation, Serial logging, invokes registered actions.

### 3.7 Firmware — UWB Pipeline (`uwb/` on Master; `thesis253_workspace/` on C3 Bridge)

**Master (ESP32-S3):**
- **`uci_uart_link` (UciUartLink)**: UCI packet framing over UART. Used for legacy direct-UART path; not active in Stage 1.5 bridge mode.
- **`uci_session_manager` (UciSessionManager)**: Owns UWB session lifecycle for **legacy direct-UART path**. Still functional as fallback. Not used when ESP32-C3 bridge is active.
- **`uci_oob` (UciOobPayloadV1)**: 37-byte OOB config packet. `parseOobPayloadV1/validateOobPayloadV1/mapOobToRunConfig`.
- **`uci_host_bridge` (UwbUciHost)**: **Active in Stage 1.5.** `submitBleOob()`, `requestStart()`, `tick()`. Sends `StartSession` over ESP-NOW to C3 bridges; receives `RangingReport` back. Feeds distance into Kalman/LSTM pipeline.
- **`lstm_inference` (LstmInference)**: TFLM sliding-window classifier.
- **`uci_door_unlock` (UwbDoorUnlock)**: Hysteresis + AI-gated relay.

**ESP32-C3 Bridge (`thesis253_workspace/`):**
- **`espnow_link` (EspNowLink)**: Receives `StartMsg` (34 bytes) from Master via ESP-NOW; sends `RangingMsg` back with anchor_id, seq, distance, status, nlos, rssi.
- **`uci_session` (UciSession)**: Simplified UCI session manager. `run(cfg)` executes SESSION_INIT → SET_APP_CONFIG → RANGING_START with 3 retries each. `buildAppConfig(cfg)` builds 21 TLVs from config, respecting `controlee` flag for DEVICE_TYPE/DEVICE_ROLE.
- **`uci_uart` (UciUart)**: Raw UCI framing over Serial1 (GPIO20/21). `sendCommand()`, `waitResponse()`, `poll(callback)` parses RANGE_DATA_NTF and fires callback with distance/status.

### 3.8 Firmware — Kalman (`lib/Kalman/Kalman.h`)
- **Purpose**: 1-D Kalman (scalar low-pass) filter for distance smoothing.
- **Key API**: `Kalman(process_noise q, sensor_noise r, estimated_error p, initial_value x)`, `getFilteredValue(measurement)`, `setParameters(q,r,p)`.
- **Firmware config**: instantiated as `Kalman(0.05, 0.2, 1.0, rawDistance)` on first reading.

### 3.9 Mobile App — see the detailed breakdown in `API_REFERENCE.md` and `DATA_CONTRACTS.md`.
Highlights:
- **Entry** (`lib/main.dart`): Firebase init, `NfcProvisioningService.initialize`, `PkeBackgroundService.ensureForRollout`, `PushNotificationService`, HCE background isolate `hceMain()`.
- **BLE Phase B** (`service/pke_auth_orchestrator.dart`): full handshake (Auth0/1/Exchange/ControlFlow instructions 0x80–0x83), ECDH, session-key derivation, GPS send.
- **GPS** (`service/gps_service.dart`): builds 32-byte location packet + HMAC-SHA256.
- **Anomaly detection** (`service/anomaly_detection_service.dart`, `ai_service.dart`, `time_/location_anomaly_detector.dart`): rule-based + Gemini AI scoring.
- **Native Kotlin**: `ProvisioningHostApduService` (HCE), `KeystoreBridge` (P-256 keystore), `HandshakeChannel`/`PhaseBCrypto` (BLE crypto), `MasterCardSession`, `UwbRangingBridge`.

---

## 4. Data Structures & Interfaces

### 4.1 Firmware — `CCCMailbox::CCC_Slot` (packed)
| Field | Type | Notes |
|-------|------|-------|
| `endpoint_pub` | `uint8_t[65]` | Phone public key (`ep_PK`), uncompressed `0x04‖X‖Y` |
| `immobilizer_token` | `uint8_t[32]` | Root-of-trust token for UWB (`tok_n`) |

### 4.2 Firmware — `CCCMailbox::CCC_Mailbox` (packed)
| Field | Type | Notes |
|-------|------|-------|
| `vehicle_id` | `char[9]` | 8-char vehicle ID + NUL |
| `vehicle_pub` | `uint8_t[65]` | Vehicle public key |
| `vehicle_priv` | `uint8_t[32]` | Vehicle private key (highly protected) |
| `signaling_bitmap` | `uint16_t` | Signaling flags |
| `slot_bitmap` | `uint8_t` | Bit 0 = owner, bits 1–7 = friends |
| `slots` | `CCC_Slot[8]` | Owner + 7 friends |
| `vehicle_identity_valid` | `bool` | Identity established flag |

### 4.3 Firmware — `FSM::StateContext`
Carries NFC (Phase A) and BLE (Phase B) data across transitions: `nfc_uid[4]`, `phone_pub_key[65]`, `phone_key_valid`, `ecu_ephemeral_pub[65]`, `phone_ephemeral_pub[65]`, `shared_secret[32]`, `session_enc_key[32]`, `session_mac_key[32]`, `session_keys_ready`, `retry_count`, `last_activity_ms`, `last_error`, `error_count`, plus `reset()`.

### 4.4 Firmware — `UwbUci::UciRunConfig`
Ranging parameters (defaults): `sessionId=42`, `controlee=false`, `localMac=0x0000`, `destMac=0x0001`, `channel=9`, `scheduleMode=1`, `preambleIdx=9`, `sfd=2`, `slotDuration=2400`, `rangingDuration=120`, `slotsPerRr=6`, `hoppingMode=1`, `stsConfig=0`, `aoaReport=1`, `vendorId=0x0708`, `staticStsIv[6]={01..06}`, `resultReportConfig=0x0B`, `rframeConfig=0x03`.

### 4.5 Firmware — `UwbUci::UciOobPayloadV1` (`kSize=37`, `kVersion=1`)
Session config transported over BLE OOB (version, role, sessionId, phoneMac, carMac, channel, preambleIdx, sfdId, stsConfig, hoppingMode, rframeConfig, resultReportConfig, aoaResultReq, scheduleMode, multiNodeMode, rangingRoundUsage, rssiReporting, slotDuration, rangingInterval, slotsPerRr, vendorId, staticStsIv[6]).

### 4.6 Firmware — `UwbUci::UciPacket` / `Mt`
`Mt { Data=0, Command=1, Response=2, Notification=3 }`; `UciPacket { Mt mt; uint8_t gid, oid, pbf; vector<uint8_t> payload; }`.

### 4.7 App — key data classes
`PhaseBResult`, `GpsDataPacket`, `MasterCardPayload`, `ProvisioningVehicleBinding`, `UwbOobPayload`, `UwbRangingEvent`, `AnomalyInput`, `AnomalyOutput`, `AnomalyEnrichedDecision`, `AccessEvent`. Full field lists in `DATA_CONTRACTS.md`.

### 4.8 Data-flow diagram
```
UWB radio ─UCI/UART─▶ UciSessionManager.onPacket()
   parse dist(cm) ─▶ +antenna offset(0.24m) ─▶ sanity[-1..30m]
   ─▶ Kalman.getFilteredValue() ─▶ residual = raw - filtered
   ─▶ [LSTM_DATA] serial log
   ─▶ LstmInference.predict(filtered, residual) ─▶ p_walk/p_loiter/p_attack
   ─▶ UwbDoorUnlock.handleRangingWithAI(dist, p...) ─▶ relay GPIO26
```

---

## 5. Algorithms & Core Logic

### 5.1 1-D Kalman filter (distance smoothing)
Scalar low-pass Kalman. Predict: `p = p + q`. Update: `k = p/(p+r)`, `x = x + k·(z − x)`, `p = (1−k)·p`. Firmware params `q=0.05, r=0.2, p=1.0`. Residual `r_t = d_raw − d_filt` feeds the LSTM as an anomaly feature.

### 5.2 UWB measurement parsing (`uci_session_manager.cpp::onPacket`)
- Ranging notification GID `0x02`, OID `0x00`; requires `payload.size() ≥ 31`.
- `num_meas = payload[24]`; `status = payload[27]`; distance `uint16 LE` at `payload[29:30]` in **cm**, reinterpreted as `int16`, divided by 100 → meters.
- **Saturation handling**: `status 0x1B` (too-close) reuses last filtered distance when within near-field window `< 0.5 m` (`kNearFieldSaturationReuseThresholdM`).
- **Antenna offset**: `+0.24 m` (`kAntennaOffsetM`).
- **Sanity bounds**: drop `< −1.0 m` or `> 30.0 m`.

### 5.3 LSTM relay-attack detection (`lstm_inference.cpp`, TFLM)
- Sliding window `window[TIME_STEPS=25][NUM_FEATURES=3]`, features `[distance, residual, velocity]` where `velocity = (d_filt_t − d_filt_{t−1})/Δt`.
- Z-score normalization per feature: `x' = (x − μ)/σ`; `scaler_mean = {4.87, −0.12, −0.08}`, `scaler_scale = {4.33, 0.20, 0.13}`.
- Runs TFLM `MicroInterpreter` → softmax `p_walk`, `p_loiter`, `p_attack`.
- **Warm-up**: returns `false` until the window has enough frames (header says window full = 25; runtime logs "warm-up: N/15"). *Discrepancy noted in `KNOWN_ISSUES.md`.*

### 5.4 AI-gated door unlock (`uci_door_unlock.cpp`)
Conjunction of physical proximity and AI confidence with hysteresis:
- `UNLOCK_THRESHOLD_M = 2.0`, `RESET_THRESHOLD_M = 3.0`, `REQUIRED_CONSECUTIVE_HITS = 3`.
- **Accept**: distance ≤ 2.0 m AND `p_walk > 0.80` for 3 consecutive hits → pulse relay (`RELAY_PIN=26`, `RELAY_PULSE_MS=500`).
- **Reject**: `p_attack > 0.70` disables the relay (attack protection).
- Cooldown reset requires distance > 3.0 m.

### 5.5 App-side anomaly scoring (`ai_service.dart`)
Gemini prompt with time/location/frequency risk sub-scores; deterministic (temperature 0.0). Thresholds: High ≥ 0.58 → BLOCK; Medium ≥ 0.28 → CONFIRM; else ALLOW. Falls back to rule-based scoring if the API fails.

### 5.6 Phase B key agreement
ECDH over P-256 between phone and ECU ephemeral keys → shared secret → HKDF-SHA256 → `session_enc_key[32]` + `session_mac_key[32]`; challenge-response bound to `v_id`.

---

## 6. Configuration & Constants

| Constant | Value | Location | Hardware/Env dependent |
|----------|-------|----------|------------------------|
| `UNLOCK_THRESHOLD_M` | 2.0 m | `uci_door_unlock.h` | Env (deployment) |
| `RESET_THRESHOLD_M` | 3.0 m | `uci_door_unlock.h` | Env |
| `REQUIRED_CONSECUTIVE_HITS` | 3 | `uci_door_unlock.h` | Env |
| `RELAY_PIN` | 26 | `uci_door_unlock.h` | **Hardware** |
| `RELAY_PULSE_MS` | 500 | `uci_door_unlock.h` | Hardware |
| `kAntennaOffsetM` | 0.24 m | `uci_session_manager.cpp` | **Hardware** (antenna) |
| `kNearFieldSaturationReuseThresholdM` | 0.5 m | `uci_session_manager.cpp` | Hardware |
| Kalman `q,r,p` | 0.05, 0.2, 1.0 | `uci_session_manager.cpp` | Env (tuning) |
| `TIME_STEPS` | 25 | `lstm_inference.h` | Model |
| `NUM_FEATURES` | 3 | `lstm_inference.h` | Model |
| `scaler_mean/scale` | see §5.3 | `lstm_inference.h` | Model (training set) |
| `p_walk` accept | > 0.80 | `uci_door_unlock.cpp` | Tuning |
| `p_attack` reject | > 0.70 | `uci_door_unlock.cpp` | Tuning |
| UCI UART pins (master legacy) | RX=17, TX=18 @115200 | `main.cpp` | **Hardware** |
| UCI UART pins (C3 bridge) | RX=20, TX=21 @115200 | `thesis253_workspace/platformio.ini` | **Hardware** |
| ESP-NOW Master MAC | 10:20:BA:73:6E:D8 | `thesis253_workspace/platformio.ini` | **Hardware** |
| PN532 UART pins | RX=44, TX=43 | `main.cpp` | **Hardware** |
| PKE rollout flags | see `ble_rollout.h` | `platformio.ini` / `ble_rollout.h` | Compile-time |
| BLE Auth service UUID | `0000aaaa-1234-5678-9abc-def012345678` | `ble_auth.cpp` | Protocol |
| CCC NVS namespace | `ccc_dk` | `ccc_mailbox.cpp` | Firmware |
| CCC MasterCard AID | `A000000809434343444B467631` | `ProvisioningHostApduService.kt` | Protocol |
| Gemini endpoint/model | `gemini-2.5-flash-lite` | `ai_service.dart` | Env (API key) |

PKE rollout defaults (`ble_rollout.h`): `BACKGROUND_MODE=0`, `FAST_TRANSACTION=1`, `BONDING_ENFORCE=0`, `RSSI_MONITOR_ONLY=1`, `RSSI_THRESHOLD_DBM=-70`, adv fast 30–60 ms, adv slow 250–500 ms, fast window 15000 ms.

---

## 7. Hardware / Platform Dependencies

- **MCU**: ESP32-S3 (`board = yolo_uno`, custom `boards/yolo_uno.json`), Arduino framework via PlatformIO (`platform = espressif32`). USB CDC on boot.
- **NFC**: PN532 over HSU (UART2, RX=44/TX=43) — Phase A reader.
- **UWB radio**: nRF52840 + DW3000 over UART1 (RX=17/TX=18 @115200) using UCI (FiRa/CCC) framing.
- **Relay**: GPIO 26, 500 ms active pulse.
- **Crypto**: mbedTLS (ECDSA P-256, ECDH, HKDF, CTR-DRBG, AES-GCM).
- **Storage**: ESP32 NVS namespace `ccc_dk`.
- **AI runtime**: TensorFlow Lite Micro (`tanakamasayuki/TensorFlowLite_ESP32`).
- **BLE**: NimBLE-Arduino (h2zero) 2.3.6+.
- **App/Android**: Android Keystore (`smart_car_phone_identity_p256`), HCE (`FEATURE_NFC_HOST_CARD_EMULATION`), foreground/background service, doze exemption. Min iOS 12.0+; multi-platform Flutter targets present.

---

## 8. Current Capabilities (Stage 1 Completed Features)

| Feature | What it does | Files | Status |
|---------|--------------|-------|--------|
| CCC confidential mailbox | Vehicle identity + owner/friend slots + tokens in NVS | `ccc_mailbox.*` | Working |
| First-boot identity | Generates `v_id`, `v_pub`/`v_priv` | `ccc_mailbox.cpp` | Working |
| NFC Phase A provisioning | Fail-closed APDU flow, owner enrollment | `nfc_session.*`, `provisioning_phase.*`, `ProvisioningHostApduService.kt` | Working |
| ECDSA-P256 verify/sign | Vehicle + phone signature ops | `ccc_mailbox.cpp`, `provisioning_phase.cpp` | Working |
| BLE Phase B auth | Ephemeral exchange, ECDH, session keys, challenge | `ble_auth.*`, `pke_auth_orchestrator.dart`, `PhaseBCrypto.kt` | Working |
| BLE admin/attestation/echo | Mode control, DK attestation, AES-GCM echo | `ble_admin/attestation/echo.*` | Working |
| Advertising profile mgmt | Fast→slow demotion window | `ble.cpp`, `ble_rollout.h` | Working |
| UWB session lifecycle | UCI init/config/start/stop/deinit with retry | `uci_session_manager.*`, `uci_uart_link.*` | Working |
| OOB session config | 37-byte payload parse/validate/map | `uci_oob.*`, `uci_host_bridge.*` | Working |
| Kalman smoothing | Distance filtering + residual | `Kalman.h`, `uci_session_manager.cpp` | Working |
| LSTM relay detection | On-device 3-class TinyML | `lstm_inference.*`, `uwb_lstm_model.h` | Working |
| AI-gated door unlock | Proximity + AI + hysteresis | `uci_door_unlock.*` | Working |
| FSM orchestration | State machine + transition table validation | `fsm/*` | Working |
| Serial logging + tools | `[LSTM_DATA]`/`[AI]`/`[DOOR]` logs; CSV + live plot | `iot/tools/*` | Working |
| App: Firebase auth/data | Login/signup, cars, keys, anomaly logs | `service/{auth,car_service,database}.dart` | Working |
| App: GPS packaging | 32-byte packet + HMAC send over BLE | `gps_service.dart`, `pke_auth_orchestrator.dart` | Working |
| App: anomaly detection | Rule + Gemini AI, notifications | `service/anomaly_*`, `ai_service.dart` | Working |
| App: master card HCE | Read master card, 60 s HCE session | `master_card_provisioning.dart`, `MasterCardSession.kt` | Working |
| FSM native tests | Unit tests for transitions | `src/test/test_fsm.cpp`, `fsm_build_verify.cpp` | Partial |

---

## 9. Known Limitations & TODOs
See `KNOWN_ISSUES.md` for the full annotated list. Summary:
1. Share slots 1–7 are verified but intentionally **disabled** pending final policy.
2. Token MAC binding for share payloads is **not implemented**.
3. Trusted time source (NTP/RTC/BLE sync) not enforced for time-bound attestations.
4. **Relay-attack simulation blocks** in `uci_session_manager.cpp` are commented out and must be guarded behind a compile-time flag before production.
5. GPS encryption in `gps_service.dart` uses an **XOR placeholder** (not production-grade).
6. `ai_service.dart` contains a **hardcoded Gemini API key** — must be moved to secure config.
7. LSTM warm-up threshold inconsistency (header says window full = 25; runtime prints "N/15").
8. `test_fsm.cpp:95` — TODO: provision device then retry (should succeed).

---

## 10. How to Extend (Stage 2)

### 10.1 Where to add features
- **New BLE service**: create `iot/include/ble/ble_x.h` + `iot/src/ble/ble_x.cpp`, add `BLEX::registerService(server[, drbg])`, and register it in `ble.cpp` (`init` block near the other `registerService` calls). Add its UUID to the advertising/verification block if needed.
- **New FSM state/event**: add enum values in `fsm_states.h`, extend `stateToString/eventToString`, add rows to the transition table in `fsm.cpp`, and bridge via `fsm_integration.h`. Run `validateConfiguration()` / `printTransitionTable()` to check for duplicates and unreachable states.
- **New UWB/AI feature**: distance parsing lives in `UciSessionManager::onPacket`; features and window in `lstm_inference.*`; unlock policy in `uci_door_unlock.*`. Retrain the model, regenerate `uwb_lstm_model.h`, and update `scaler_mean/scale` + `TIME_STEPS`.
- **New app screen**: add under `lib/screen/`, wire navigation from `dashboard.dart`; put logic in a `lib/service/` class; reuse `widgets/app_components.dart` and `theme/app_colors.dart`.
- **New app service**: add under `lib/service/`; if it needs native access, add a `MethodChannel` in `MainActivity.kt` and a Kotlin bridge object.

### 10.2 Conventions & patterns
- Firmware modules are **namespaces** (e.g., `CCCMailbox`, `BLEAuth`, `UwbDoorUnlock`) or classes (`UciSessionManager`, `LstmInference`); one header + one source per module under mirrored `include/`/`src/` trees.
- Public keys are **65-byte uncompressed** (`0x04‖X‖Y`); tokens/keys are **32 bytes**; vehicle ID is **8 bytes**.
- Serial log tags are structured (`[UCI]`, `[LSTM_DATA]`, `[AI]`, `[DOOR]`, `[BLE]`, `[FSM]`, `[CCC]`, `[PhaseA]`) — keep this convention so the Python tools keep parsing.
- Config knobs are `constexpr` in headers or `-D` build flags in `platformio.ini`.
- App services are typically singletons / static helpers returning `Future<…>`; data classes are immutable-ish with `copyWith`/`toJson`.

### 10.3 Constraints & pitfalls
- **Do not** move immobilizer tokens off-device or into the cloud (core security rule 1).
- **Do not** ship with the relay-attack simulation enabled or debug/test bypass flags on (`isTestBypassEnabled`, force-provisioning).
- BLE callbacks run on the NimBLE stack — keep them short; heavy work is offloaded to FreeRTOS tasks (see `main.cpp`).
- The relay is fired directly from `UwbDoorUnlock`; ensure any new gate keeps the **AND of crypto + physical** invariant.
- Changing UWB antenna/hardware requires re-tuning `kAntennaOffsetM` and possibly Kalman params and LSTM scalers.
- Match ESP32↔phone UUIDs and APDU instruction codes on both sides when changing the protocol.
```
