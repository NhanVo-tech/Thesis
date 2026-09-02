# CODEBASE_REFERENCE.md

Onboarding reference for the **Smart Car Access** repository: directory map, module breakdown,
core data structures, algorithms, configuration constants, and current capabilities.

> Last updated: 2026-09-02. Reflects the multi-anchor UWB pipeline (PC bridge → trilateration →
> EKF → geometric unlock). Legacy single-anchor UCI/OOB/ESP-NOW modules and the 1-D Kalman
> library have been removed. The 1-D LSTM is retained but **not wired** into the pipeline.

---

## 1. Overview & Tech Stack

| Layer | Technology |
|-------|------------|
| Vehicle firmware | ESP32-S3 (`yolo_uno`), Arduino framework, PlatformIO, FreeRTOS, NimBLE, mbedTLS, PN532 |
| Ranging bridge | Python 3.8+, `pyserial`, Qorvo `uci` package, 3× DWM3001CDK anchors |
| Phone app | Flutter/Dart, Kotlin/Java native (HCE, Keystore, `android.ranging`) |
| Cloud | Firebase (Auth, Firestore, Storage, FCM), Gemini 2.5 Flash Lite |
| Crypto | ECDSA/ECDH P-256, HKDF-SHA256, HMAC-SHA256, AES |

---

## 2. Directory Map

```
SmartCarAccess/
├─ README.md                     ← detailed thesis overview (top-level)
├─ PHASE_A_AND_PHASE_B.md        ← NFC + BLE protocol deep dive
├─ UX_UI_IMPROVEMENTS.md         ← app UI changelog
├─ docs/                         ← this documentation set
│  ├─ ARCHITECTURE.md
│  ├─ CODEBASE_REFERENCE.md
│  ├─ API_REFERENCE.md
│  ├─ DATA_CONTRACTS.md
│  ├─ FEATURES.md
│  ├─ GLOSSARY.md
│  ├─ KNOWN_ISSUES.md
│  └─ STAGE2_PLAN.md
├─ iot/                          ← ESP32-S3 firmware (PlatformIO)
│  ├─ platformio.ini
│  ├─ include/
│  │  ├─ ccc_mailbox.h  nfc_session.h  provisioning_phase.h
│  │  ├─ ble/           (ble, ble_auth, ble_attestation, ble_admin, ble_echo, rollout, telemetry)
│  │  ├─ fsm/           (fsm, fsm_states, fsm_integration)
│  │  └─ uwb/           (uwb_bridge, ranging_frame, uwb_geometry, trilateration,
│  │                     ekf_stub, access_controller, lstm_inference, uwb_lstm_model)
│  ├─ src/              (mirrors include/, plus main.cpp, ccc_mailbox.cpp, nfc_session.cpp,
│  │                     provisioning_phase.cpp)
│  ├─ tools/            (Python: serial_csv_logger, realtime_lstm_visualizer, analyze_ekf,
│  │                     demo_ble_auth, phase_b_test, setup.sh, requirements.txt)
│  ├─ uwb_lstm_data_label{0,1,2}.csv   ← labelled datasets (normal / loiter / relay)
│  ├─ IMPLEMENTATION_SUMMARY.md  PAPER.md
│  └─ boards/yolo_uno.json
└─ software/smart_car_app/       ← Flutter Android app
   ├─ pubspec.yaml
   ├─ lib/  (main.dart, service/, screen/, widgets/, theme/)
   ├─ android/  (Kotlin/Java native: HCE, Keystore, Phase B, UWB bridge)
   └─ packages/phaseb_handshake_bridge/
```

The multi-anchor UWB **ranging bridge** lives outside this repo, in the Qorvo SDK tools:
`DW3_QM33_SDK_1.1.1/SDK/Tools/uwb-qorvo-tools/scripts/fira/run_fira_twr/run_fira_bridge.py`.

---

## 3. Module Breakdown

### 3.1 Firmware entry — `iot/src/main.cpp`
Boots subsystems (`CCCMailbox`, `FSM`, `BLEMod`, `NfcSession`, `UwbBridge`, `AccessController`)
and spawns the FSM/NFC/UWB FreeRTOS tasks. The UWB task reads USB-CDC lines and routes
`RANGE:`/`ACK:` frames into `UwbBridge::feedLine()`.

### 3.2 CCC Mailbox — `ccc_mailbox.{h,cpp}`
Confidential vehicle store in NVS namespace `ccc_dk`. First boot generates the vehicle ID
(`"VN"` + 6 random chars) and a P-256 keypair. Exposes signing (`signVehicleDataP256`), endpoint
key/token slot management, and the fast-transaction artifact. See [DATA_CONTRACTS.md](DATA_CONTRACTS.md) §1.1.

### 3.3 NFC / Provisioning — `nfc_session.{h,cpp}`, `provisioning_phase.{h,cpp}`
PN532 reader (HSU) implementing Phase A. Drives SELECT AID → SPAKE2+ HMAC → GET/WRITE DATA →
OP CONTROL commit, with reselect recovery and fail-closed TLV validation. `ProvisioningPhase`
persists the phone endpoint public key, cert chain, and fast artifact; sets owner slot 0.

### 3.4 BLE — `ble/*.cpp`
- `ble.cpp` — device setup (`ESP-Smart-Car-ECU`), bonding/secure connections, advertising.
- `ble_auth.cpp` — Phase B CCC tunnel (AUTH0/1, EXCHANGE, CONTROL_FLOW, RANGING_START/STOP),
  ECDH + HKDF session keys, fast path, epoch time-sync, latency telemetry.
- `ble_attestation.cpp` — 147-byte owner-signed digital-key attestation (ECDSA P-256).
- `ble_admin.cpp` — maintenance commands (`0x01`–`0x43`).
- `ble_echo.cpp` — AES secure-channel echo (session-key smoke test).
- `pke_telemetry.cpp` / `ble_rollout.*` — event telemetry + feature flags (fast tx, bonding, RSSI).

### 3.5 FSM — `fsm/*.cpp`
Central state machine (INIT/IDLE, provisioning, auth, unlock, admin, error). `fsm_integration.h`
exposes `FSMIntegration::{NFC,BLE,Unlock}` hooks so subsystems post events without tight coupling
(e.g. `BLE::onAuth0Received()`, `BLE::onUnlockRequested()`).

### 3.6 UWB pipeline — `uwb/*.cpp` (the reworked path)

| File | Role |
|------|------|
| `uwb_bridge.cpp` | Parses `RANGE:d0,d1,d2,valid`; queues frames; at 10 Hz runs trilateration → EKF update → `AccessController::handlePosition`; emits `CMD:START/STOP_RANGING`. |
| `ranging_frame.h` | `RangingFrame { t_ms, d[3], valid_mask }`. |
| `uwb_geometry.h` | Fixed anchor coords A0 (0, 2), A1 (−0.85, 0), A2 (0, −2). |
| `trilateration.cpp` | 2-circle geometric solve (n = 2) or weighted Gauss-Newton least-squares (n = 3); returns `(x, y, rms, valid)`. |
| `ekf_stub.cpp` | 2-D constant-velocity Kalman filter, state `[x, y, vx, vy]`; measurement noise derived from trilateration RMS; `predictTo()` coasts over dropped frames. |
| `access_controller.cpp` | Geometric zone unlock: Euclidean distance to the unlock point, hysteresis, consecutive-hit debounce, radial-velocity approach gate, relay pulse. |
| `lstm_inference.cpp`, `uwb_lstm_model.h` | **Dormant** TFLM 1-D model (retained for future CNN-LSTM milestone; not called). |

### 3.7 Flutter app — `software/smart_car_app/lib/service/`
Key services: `pke_auth_orchestrator.dart` (BLE Phase B), `uwb_multi_service.dart`
(phone-side `android.ranging` controller), `gps_service.dart` (encrypted location + HMAC),
`anomaly_detection_service.dart` + `ai_service.dart` (time/location/frequency + Gemini),
`pke_background_service.dart` (foreground scanning), `master_card_provisioning.dart` (NDEF
`{vid, msk}` read → HCE session), `car_service.dart` (Firestore keys/cars).

### 3.8 Android native — `software/smart_car_app/android/.../`
`ProvisioningHostApduService.kt` (HCE Phase A applet, AID `A000000809434343444B467631`),
`KeystoreBridge.kt` (identity key alias `smart_car_phone_identity_p256`),
`PhaseBCrypto.kt` + `HandshakeChannel.kt` (ephemeral ECDH/HKDF/ECDSA bridge),
`UwbMulticastBridge.kt` (multicast DS-TWR, phone pinned to `0x06C1`), `DataStoreUtil.kt`.

---

## 4. Core Data Structures (summary)

Full field tables in [DATA_CONTRACTS.md](DATA_CONTRACTS.md). Highlights:

- `CCC_Mailbox` / `CCC_Slot` — vehicle identity + 8 key/token slots.
- `RangingFrame { uint32 t_ms; double d[3]; uint8 valid_mask }` — one forwarded ranging round.
- `Trilateration::Result { double x, y, rms; bool valid }`.
- EKF state `double x[4] = {px, py, vx, vy}`, covariance `P[4][4]`.
- AccessController trackers: `consecutive_close_reads`, `is_door_unlocked`, `last_distance_m`,
  `last_radial_mps`, `relay_active`.

---

## 5. Key Algorithms

### 5.1 Distance forwarding (PC bridge)
Per anchor, a thread-safe `AnchorState` stores `(distance, timestamp)`. The forward loop samples
all anchors at `rate_hz` (default 10 Hz) and emits `RANGE:d0,d1,d2,valid`, where `valid=1` iff all
three are fresh (`≤ fresh_ms`, default 500) and within `[dmin, dmax]` (default 0.1–30 m).

### 5.2 Trilateration
- **n = 2:** closed-form two-circle intersection (with fallback to the line between centers when
  circles don't intersect).
- **n = 3:** weighted centroid seed, then up to 10 Gauss-Newton iterations minimizing
  $\sum_i (\lVert p - a_i \rVert - d_i)^2$. Residual RMS is returned as a fix-quality metric.

### 5.3 EKF (2-D constant velocity)
Predict: $x \mathrel{+}= v\,\Delta t$ with process (manoeuvre) noise $\sigma_a = 1.2\ \text{m/s}^2$.
Correct: measurement $z = (x, y)$ from trilateration with variance $R = \sigma^2$, where
$\sigma$ is the clamped trilateration RMS (0.05–1.0 m). Long gaps (> 2 s) or clock glitches
reinitialize the track. Runs/predicts at 10 Hz.

### 5.4 Geometric door unlock
Let $d = \lVert (x,y) - (x_u, y_u) \rVert$ to the unlock point $(-0.85, 0)$, and radial velocity
$v_r = (v\cdot(p - p_u))/d$ ($>0$ moving away). Logic:
1. `d > RESET_RADIUS (3 m)` → re-arm, reset counter.
2. Already unlocked & near → ignore.
3. Approach gate: if `v_r > 0.10 m/s` (moving away) → reset counter.
4. `d ≤ UNLOCK_RADIUS (2 m)` → increment hits; after `REQUIRED_CONSECUTIVE_HITS (3)` → pulse
   relay (GPIO26, 500 ms) and latch unlocked.

---

## 6. Configuration Constants

| Constant | Value | Where |
|----------|-------|-------|
| Anchor coords A0/A1/A2 | (0, 2) / (−0.85, 0) / (0, −2) m | `uwb_geometry.h` |
| Unlock point | (−0.85, 0) m | `access_controller.h` |
| `UNLOCK_RADIUS_M` / `RESET_RADIUS_M` | 2.0 / 3.0 m | `access_controller.h` |
| `REQUIRED_CONSECUTIVE_HITS` | 3 | `access_controller.h` |
| `APPROACH_SPEED_MIN_MPS` | 0.10 m/s | `access_controller.h` |
| `RELAY_PIN` / `RELAY_PULSE_MS` | GPIO26 / 500 ms | `access_controller.h` |
| EKF `kAccelStd` | 1.2 m/s² | `ekf_stub.cpp` |
| EKF meas-noise clamp | 0.05–1.0 m | `ekf_stub.cpp` |
| EKF `kMaxGapS` | 2.0 s | `ekf_stub.cpp` |
| Drive cadence `kDrivePeriodMs` | 100 ms (10 Hz) | `uwb_bridge.cpp` |
| Bridge defaults | rate 10 Hz, fresh 500 ms, dmin 0.1 / dmax 30 m | `run_fira_bridge.py` |
| Anchor session | ch 9, preamble 9, DS-TWR deferred, slot 2400, interval 200, slots/rr 25 | `run_fira_bridge.py` |
| BLE rollout | fastTx ON, bonding OFF, RSSI monitor-only, threshold −70 dBm | `platformio.ini` |

---

## 7. Hardware / Platform Dependencies

- **Vehicle:** ESP32-S3 dev board (`yolo_uno`), native USB-CDC (`ARDUINO_USB_CDC_ON_BOOT=1`),
  PN532 over UART HSU, door relay on GPIO26.
- **Anchors:** 3× DWM3001CDK (Qorvo DW3xxx/QM33), each on its own COM port to the PC.
- **PC:** Python 3.8+, `pyserial`, Qorvo `uci` package; USB/serial drivers for anchors + ESP32.
- **Phone:** Android 16+ (API 36) for `android.ranging`; NFC + BLE; Keystore-backed EC keys.

---

## 8. Current Capabilities

| Capability | Status |
|------------|--------|
| NFC Phase A provisioning (fail-closed) | ✅ |
| BLE Phase B auth (ECDH/HKDF, fast path, time-sync) | ✅ |
| Digital-key attestation (owner) | ✅ |
| Multi-anchor DS-TWR ranging (phone ↔ 3 anchors) | ✅ |
| PC bridge distance forwarding + CMD control | ✅ |
| 2-D trilateration (2/3-anchor) on ESP32 | ✅ |
| 2-D EKF position/velocity tracking on ESP32 | ✅ |
| Geometric zone unlock + approach gate | ✅ |
| Ranging start/stop gated by BLE session (CCC 0x84/0x85) | ✅ |
| App: keys/cars/logs, background scanning, GPS encryption | ✅ |
| App: access-pattern anomaly (rules + Gemini) | ✅ |
| On-device intent recognition (CNN-LSTM) | ❌ (dormant LSTM present; see STAGE2_PLAN) |
| Geofenced multi-zone actuation (trunk/cabin) | ❌ (roadmap) |
| Key sharing to friend slots 1–7 | 🔄 (verified, disabled by policy) |

See [FEATURES.md](FEATURES.md) for the full checklist, [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for
limitations, and [STAGE2_PLAN.md](STAGE2_PLAN.md) for extension milestones.
