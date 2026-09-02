# API_REFERENCE.md

Function- and interface-level reference for the firmware, the PC bridge, the mobile app, and the
BLE GATT surface.

> Last updated: 2026-09-02. The UWB APIs now cover the multi-anchor pipeline
> (`UwbBridge`, `Trilateration`, `Ekf`, `AccessController`). The legacy `UciUartLink`,
> `UciSessionManager`, `UciOobPayload`, and 1-D `UwbDoorUnlock` APIs have been removed.

---

## 1. Firmware C++ APIs

### 1.1 `CCCMailbox` — `include/ccc_mailbox.h`

| Function | Returns | Notes |
|----------|---------|-------|
| `begin()` | `bool` | Load/create vehicle identity from NVS `ccc_dk` |
| `vehicleId()` | `const char*` | 8-char ID (`"VN……"`) |
| `hasVehiclePub()` / `getVehiclePub(out, max)` | `bool` | Vehicle public key (65 B) |
| `hasVehiclePriv()` | `bool` | Private key present |
| `signVehicleDataP256(data, len, sigOut, max, *outLen)` | `bool` | ECDSA-P256 DER signature |
| `setEndpointPub(pub65, slot=0)` / `getEndpointPub(out, max, slot)` | `bool` | Endpoint (phone) key per slot |
| `hasToken(slot)` / `getToken(slot, out)` / `setToken(slot, tok)` / `ensureToken(slot)` | `bool` | Immobilizer tokens |
| `setSlotActive(slot, active)` | `bool` | Slot bitmap management |
| `clearMailboxes()` / `clearAll()` | `void` | Wipe endpoints+tokens / everything |

### 1.2 `ProvisioningPhase` — `include/provisioning_phase.h`

| Function | Returns | Notes |
|----------|---------|-------|
| `isProvisioned()` | `bool` | Owner endpoint key stored |
| `storePhonePubRaw(pub65)` | `bool` | Persist phone public key |
| `getPhonePubRaw(out, max)` | `size_t` | Bytes copied (0 if none) |
| `storeCertChain(cert, len)` | `bool` | Persist cert chain |
| `storeFastArtifact(ver, key32)` / `getFastArtifact(*ver, key32)` | `bool` | Fast-path artifact |
| `setOwnerProvisioned(pub65, force)` | `bool` | Activate slot 0 + ensure token |
| `verifySignatureP256(pub65, data, len, sigDer, sigLen)` | `bool` | Verify phone signature |
| `clearAll()` / `clearProvisionedOnly()` | `void` | Reset (keep/discard vehicle identity) |

### 1.3 `NfcSession` — `include/nfc_session.h`
`begin(Stream&, txPin, rxPin, baud)`, `tick()`. Runs the Phase A reader FSM (SELECT AID → SPAKE2+
→ GET/WRITE DATA → OP CONTROL) with reselect recovery and fail-closed TLV checks.

### 1.4 `BLEAuth` (Phase B) — `include/ble/ble_auth.h`
Registers the CCC tunnel service and handles AUTH0/1, EXCHANGE, CONTROL_FLOW, RANGING_START/STOP.
Internally: `generate_ephemeral_keypair()`, `verify_phone_signature()`,
`compute_shared_secret_and_session_keys()`, `publish_auth_challenge()`,
`derive_fast_session_keys()`. Session gated by `s_session_keys_ready`.

### 1.5 `UwbBridge` — `include/uwb/uwb_bridge.h`

| Function | Returns | Notes |
|----------|---------|-------|
| `begin()` | `void` | Create frame queue, mark not-ranging |
| `feedLine(const char* line)` | `void` | Route `RANGE:`/`ACK:` lines from USB-CDC |
| `tick()` | `void` | Drain frames → trilaterate → EKF update; drive AccessController @10 Hz |
| `sendStart()` / `sendStop()` | `void` | Emit `CMD:START/STOP_RANGING`; reset EKF on start |

### 1.6 `Trilateration` — `include/uwb/trilateration.h`
`solve(const double anchorX[3], const double anchorY[3], const double d[3], uint8_t validMask)`
→ `Result { x, y, rms, valid }`. Uses 2-circle geometry for 2 anchors, Gauss-Newton LS for 3.

### 1.7 `Ekf` — `include/uwb/ekf_stub.h`

| Function | Returns | Notes |
|----------|---------|-------|
| `reset()` | `void` | Uninitialize (call on ranging (re)start) |
| `update(x, y, t_ms, measNoiseStd)` | `void` | Fuse a fix; pass trilateration RMS as noise |
| `update(x, y)` | `void` | Convenience (synthesized dt, default noise) |
| `predictTo(t_ms)` | `bool` | Predict-only to time; false when stale (> 2 s gap) |
| `x() y() vx() vy() speed()` | `double` | Latest estimate |
| `initialized()` | `bool` | Track valid |

### 1.8 `AccessController` — `include/uwb/access_controller.h`

| Function | Returns | Notes |
|----------|---------|-------|
| `begin()` | `void` | Init relay GPIO, reset state |
| `handlePosition(x, y)` | `void` | 2-D unlock without velocity gate |
| `handlePosition(x, y, vx, vy)` | `void` | Adds radial-velocity approach gate |
| `tick()` | `void` | Relay pulse timing |
| `isDoorUnlocked()` / `getConsecutiveReadCount()` | `bool` / `int` | Telemetry |
| `getLastDistance()/getLastX()/getLastY()/getLastRadialSpeed()` | `double` | Telemetry |
| `manualUnlock()` / `resetDoorState()` | `void` | Admin/test |

### 1.9 FSM — `include/fsm/fsm.h`, `fsm_integration.h`
`FSM::begin()`, `FSM::tick()`. Integration hooks:
`FSMIntegration::NFC::{onCardDetected, onSelectAIDSuccess, onKeysExchanged, onCredentialsStored, …}`,
`FSMIntegration::BLE::{onAuth0Received, onAuth1ResponseReceived, onExchangeReceived, onControlFlowReceived, onUnlockRequested, …}`,
`FSMIntegration::Unlock::{onProximityOK, onSessionValid, onUnlockSuccess, …}`.

### 1.10 `LstmInference` (dormant) — `include/uwb/lstm_inference.h`
Retained TFLM API (`begin()`, feed features, read `[p_walk, p_loiter, p_attack]`). **Not called**
by the current pipeline; kept for the future CNN-LSTM milestone.

---

## 2. PC Bridge — `run_fira_bridge.py`

CLI:

| Argument | Default | Purpose |
|----------|---------|---------|
| `-p/--ports` | (required) | Anchor COM ports (e.g. `COM11 COM19 COM12`) |
| `--macs` | `0 1 2` | Anchor MAC addresses |
| `--dest-mac` | `0x06C1` | Phone/controller address |
| `-s/--session` | `42` | UCI session id |
| `-c/--channel` | `9` | UWB channel |
| `--preamble-idx` | `9` | Preamble code index |
| `--esp-port` | (required) | ESP32-S3 USB-CDC port |
| `--esp-baud` | `115200` | ESP32 serial baud |
| `--rate-hz` | `10` | RANGE forward rate |
| `--fresh-ms` | `500` | Max reading age to count as fresh |
| `--dmin` / `--dmax` | `0.1` / `30.0` | In-bounds distance window (m) |
| `--autostart` | off | Start ranging without waiting for CMD |
| `--esp-debug` | off | Log every ESP line (FSM/BLE/AUTH diagnostics) |

Key classes: `AnchorState` (thread-safe last reading), `EspLink` (line I/O; DTR/RTS held low to
avoid resetting the native USB-CDC), `Bridge` (`start_ranging/stop_ranging`, `command_loop`,
`forward_loop`). Each anchor runs `start_anchor()` as controlee/responder, DS-TWR deferred.

---

## 3. Android Native (Kotlin, MethodChannel)

| Channel / Class | Methods |
|-----------------|---------|
| `smartcar/phaseb/handshake` (`HandshakeChannel`) | `generateEphemeralKeypair`, `getEphemeralPublicKey`, `signEphemeralWithIdentity`, `computeECDH`, `deriveSessionKeys`, `signChallenge` |
| `KeystoreBridge` | `ensurePhaseAKey`, `getPhaseAPublicKey65`, `signPhaseA(data)` (alias `smart_car_phone_identity_p256`) |
| `smartcar/uwb_multi` (`UwbMulticastBridge`) | `isSupported`, `ensurePermission`, `start`, `stop`, `isActive`; EventChannel `smartcar/uwb_multi/events` emits `{type:"range", d0,d1,d2,mask}` |
| `smartcar/mastercard` | `activateHceSession(payload)` |
| HCE (`ProvisioningHostApduService`) | APDU handlers: SELECT `0xA4`, GET DATA `0xCA`, SPAKE2 `0x30/0x32`, WRITE `0xD4`, OP CONTROL `0x3C`, PROVISION RESULT `0xDA` |

---

## 4. Mobile Dart Services (selected)

| Service | Key methods |
|---------|-------------|
| `PkeAuthOrchestrator` | scan/connect/authenticate, `authenticatePreferredDevice`, `save/loadPreferredDeviceAddress` |
| `UwbMultiService` | `isSupported`, `ensurePermission`, `start`, `stop`; streams `ranges/status/logs` |
| `GpsService` | `getCurrentPosition`, package+encrypt (session keys) + HMAC, BLE send, 30 s auto-sync |
| `AnomalyDetectionService` | `analyzeAccessEvent`, `analyzeAccessEventWithAI` (Gemini) → ALLOW/CONFIRM/BLOCK |
| `CarService` | `addCar/updateCar/deleteCar`, `addDigitalKey`, `getUserCars/getUserDigitalKeys`, `registerOwnerProvisioningRecord` |
| `MasterCardProvisioning` | `readMasterCard`, `activateHceSession` |

---

## 5. BLE GATT UUID Reference

| Service | UUID | Characteristics |
|---------|------|-----------------|
| Phase B Auth (CCC tunnel) | `0000aaaa-1234-5678-9abc-def012345678` | RX `0000aac1` (write), TX `0000aac2` (notify) |
| Digital-key Attestation | `555a0001-00aa-1111-2222-333344445555` | Rx `555a0002` (write), Tx `555a0003` (notify) |
| Admin | `9a9b9c9d-0000-4000-8000-9a9b9c9d0000` | Mode `…0001`, Cmd `…0002`, Info `…0003`, PhoneKey `…0004` |

Admin commands (`ble_admin.cpp`): `0x01` clear provisioned, `0x02` clear all, `0x20` trigger
provisioning, `0x30/0x31/0x32` force-provision on/off/one-shot, `0x33` status, `0x36` mailbox
summary, `0x40`–`0x43` Phase B test/status/reset/stats.
