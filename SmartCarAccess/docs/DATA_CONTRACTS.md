# DATA_CONTRACTS.md

Schemas for every structure that crosses a module boundary: firmware C++ types, the PC-bridge
serial protocol, mobile Dart classes, cloud Firestore documents, and cross-module passing
semantics.

> Last updated: 2026-09-02. The UWB section now describes the multi-anchor pipeline
> (`RangingFrame`, `Trilateration::Result`, EKF state) and the USB-CDC line protocol. The legacy
> `UciRunConfig`, `UciOobPayloadV1`, and 1-D `Kalman` contracts have been removed.

---

## 1. Firmware C++ Structures

### 1.1 `CCC_Mailbox` / `CCC_Slot` — `include/ccc_mailbox.h`

| Field | Type | Size | Notes |
|-------|------|------|-------|
| `vehicle_id` | `char[9]` | 8 + NUL | `"VN"` + 6 random alphanumerics |
| `vehicle_pub` | `uint8_t[65]` | 65 | P-256 uncompressed (`0x04 ‖ X ‖ Y`) |
| `vehicle_priv` | `uint8_t[32]` | 32 | **Protected**, never leaves NVS |
| `signaling_bitmap` | `uint16_t` | 2 | Signaling flags |
| `slot_bitmap` | `uint8_t` | 1 | bit 0 = owner, bits 1–7 = friends |
| `slots[8]` | `CCC_Slot[8]` | — | per-slot endpoint key + token |
| `vehicle_identity_valid` | `bool` | 1 | keypair present and validated |

`CCC_Slot { uint8_t endpoint_pub[65]; uint8_t immobilizer_token[32]; }`

**NVS keys** (namespace `ccc_dk`): `v_id`, `v_pub`, `v_priv`, `sig_bmp`, `slot_bmp`,
`ep_0`…`ep_7`, `tok_0`…`tok_7`, `cert_chain`, `fast_art_ver`, `fast_art_key`,
`force_prov`, `oneshot_force`.

### 1.2 FSM `StateContext` — `include/fsm/fsm_states.h`
NFC phase: `nfc_uid[4]`, `phone_pub_key[65]`, `phone_key_valid`.
BLE phase: `ecu_ephemeral_pub[65]`, `phone_ephemeral_pub[65]`, `shared_secret[32]`,
`session_enc_key[32]`, `session_mac_key[32]`, `session_keys_ready`.
Bookkeeping: `last_error`, `error_count`, `retry_count`, `last_activity_ms`.

### 1.3 `RangingFrame` — `include/uwb/ranging_frame.h`

| Field | Type | Unit | Meaning |
|-------|------|------|---------|
| `t_ms` | `uint32_t` | ms | ESP32 `millis()` at parse time |
| `d[3]` | `double[3]` | m | Distances to anchors 0/1/2 |
| `valid_mask` | `uint8_t` | bitfield | `0x07` if all fresh & in-bounds, else `0x00` |

### 1.4 `Trilateration::Result` — `include/uwb/trilateration.h`

| Field | Type | Unit | Meaning |
|-------|------|------|---------|
| `x`, `y` | `double` | m | Fused planar position (car frame) |
| `rms` | `double` | m | Residual RMS (fix quality; feeds EKF R) |
| `valid` | `bool` | — | true when ≥ 2 anchors contributed |

### 1.5 EKF state — `src/uwb/ekf_stub.cpp`

| Symbol | Type | Unit | Meaning |
|--------|------|------|---------|
| `g_x[4]` | `double[4]` | m, m/s | State `[px, py, vx, vy]` |
| `g_P[4][4]` | `double[4][4]` | — | State covariance |
| `g_lastMs` / `g_lastMeasMs` | `uint32_t` | ms | Last propagate / last correction time |

Public accessors: `x() y() vx() vy() speed()`; `update(x, y, t_ms, measNoiseStd)`,
`predictTo(t_ms)`, `reset()`.

### 1.6 AccessController trackers — `src/uwb/access_controller.cpp`
`consecutive_close_reads (int)`, `is_door_unlocked (bool)`, `last_x_m/last_y_m/last_distance_m`,
`last_radial_mps`, `relay_active`, `relay_deactivate_time_ms`.

### 1.7 Digital-key attestation payload (147 bytes) — `src/ble/ble_attestation.cpp`

| Offset | Size | Field |
|--------|------|-------|
| 0 | 8 | Vehicle ID |
| 8 | 1 | Slot ID |
| 9 | 65 | Friend public key (unused for owner policy) |
| 74 | 4 | `validFrom` (Unix, big-endian) |
| 78 | 4 | `validUntil` (Unix, big-endian) |
| 82 | 1 | Entitlement |
| 83 | 32 | Signature R |
| 115 | 32 | Signature S |

### 1.8 LSTM window (dormant) — `include/uwb/lstm_inference.h`
Retained but not driven: 25-step window over 1-D features `[distance, filtered, residual]`,
3-class softmax `[p_walk, p_loiter, p_attack]`. Superseded by the geometric unlock; slated to be
replaced by a CNN-LSTM on `[x, y, vx, vy]` (see STAGE2_PLAN M5/M6).

---

## 2. PC-Bridge ↔ ESP32 Serial Protocol (USB-CDC, newline-terminated ASCII)

**PC → ESP32**

| Line | Meaning |
|------|---------|
| `RANGE:d0=<f>,d1=<f>,d2=<f>,valid=<0\|1>` | One ranging round. Distances in metres; `valid=1` iff all three fresh & in-bounds. |
| `ACK:START_RANGING` / `ACK:STOP_RANGING` | Acknowledge a control command. |

**ESP32 → PC**

| Line | Meaning |
|------|---------|
| `CMD:START_RANGING` | Start all anchor sessions (issued when phone sends CCC `0x84`). |
| `CMD:STOP_RANGING` | Stop all anchor sessions (CCC `0x85`). |

Parsing: `UwbBridge::parseRange()` uses `sscanf("d0=%lf,d1=%lf,d2=%lf,valid=%d")`; malformed
lines are dropped. Lines > 128 chars are discarded by the console reader in `main.cpp`.

---

## 3. Phase B CCC Tunnel Frame (BLE GATT)

Write to `CCC_RX` (`0000aac1`), notify on `CCC_TX` (`0000aac2`).

**Request (phone → ECU):** `CLA ‖ INS ‖ P1 ‖ P2 ‖ Lc ‖ data[Lc]`
**Response (ECU → phone):** `INS ‖ SW1 ‖ SW2 ‖ len ‖ payload[len]`

| INS | Name | Notable P1 / payload |
|-----|------|----------------------|
| `0x80` | AUTH0 | P1 `0x11` standard / `0x01` fast; ECU returns ephemeral pub (65 B) |
| `0x81` | AUTH1 | data = phone ephemeral pub(65) ‖ sig_len(2 LE) ‖ DER sig |
| `0x82` | EXCHANGE | challenge signature; P1 `0x10` = post-auth epoch time-sync |
| `0x83` | CONTROL_FLOW | unlock/secure command (requires session) |
| `0x84` | RANGING_START | starts UWB (requires session) |
| `0x85` | RANGING_STOP | stops UWB (requires session) |

Status words: `90 00` OK, `6A 80` wrong data, `6A 86` wrong P1/P2, `69 85` conditions not met,
`6A 81` unsupported, `6F 00` internal.

**Session-key derivation:** `HKDF-SHA256(sharedSecret, info)` with
`info = "SmartCarv1|ENC"/"SmartCarv1|MAC" ‖ ecu_pub65 ‖ phone_pub65` (standard) or
`"SmartCarFast|ENC"/"SmartCarFast|MAC" ‖ vehicleId8 ‖ artifactVersion` (fast path).

---

## 4. Mobile App Data Classes (Dart)

| Class | Source | Key fields |
|-------|--------|-----------|
| `MasterCardPayload` | `master_card_provisioning.dart` | `vehicleId (8B)`, `masterSecret (32B)` from NDEF `{vid, msk}` |
| `UwbMultiRange` | `uwb_multi_service.dart` | `d0, d1, d2 (double)`, `mask (int)` |
| `UwbMultiStatus` | `uwb_multi_service.dart` | `ACTIVE \| STOPPED \| ERROR` |
| Phase B session | `pke_auth_orchestrator.dart` | `sessionEncKey(32)`, `sessionMacKey(32)`, `sharedSecret`, `challenge` |
| GPS packet | `gps_service.dart` | 32-byte binary: lon/lat/alt (le32 ×3) + accuracy(u16) + ts(u32), then `‖ HMAC-SHA256(32)` = 64 B |
| `AccessEvent` | `anomaly_detection_service.dart` | `timestamp`, `hour`, `weekday`, `lat/lon`, `distanceFromUsual`, `accessCountLastHour` |
| `AnomalyAnalysisResult` / `AnomalyEnrichedDecision` | anomaly services | `isAnomalous`, `confidence`, `severity (low/med/high)`, `action (ALLOW/CONFIRM/BLOCK)`, `reason` |

---

## 5. Cloud Firestore Schemas

| Collection | Key fields |
|------------|-----------|
| `users/{uid}` | email, name, photoURL |
| `cars/{carId}` | `ownerId`, make/model, plate, photoURL |
| `digital_keys/{keyId}` | `carId`, `ownerId`, slot, validity window, status |
| `access_logs/{id}` | timestamp, location, decision, severity, reason |
| provisioning record | vehicleId, ownerUid, device pub key, vehicle pub key, write-data payload, slot info |

Cloud stores metadata only — **never** immobilizer tokens or private keys.

---

## 6. Cross-Module Passing Semantics

```
PC bridge  ──"RANGE:…" (USB-CDC)──▶  UwbBridge.parseRange  ──RangingFrame──▶  queue
queue      ──RangingFrame──▶  Trilateration.solve  ──Result(x,y,rms)──▶  Ekf.update
Ekf (10 Hz predictTo)  ──(x,y,vx,vy)──▶  AccessController.handlePosition  ──▶  Relay(GPIO26)

Phone(BLE CCC 0x84/0x85)  ──▶  ble_auth  ──▶  UwbBridge.sendStart/sendStop  ──"CMD:…"──▶  PC bridge
Phone(NFC APDU)           ──▶  NfcSession/ProvisioningPhase  ──▶  CCCMailbox (NVS)
Phone(BLE Phase B)        ──▶  ble_auth  ──session keys──▶  FSMIntegration::BLE hooks  ──▶  FSM
```

All UWB numeric values are IEEE-754 `double` in metres (position) / metres·s⁻¹ (velocity);
timestamps are unsigned milliseconds from `millis()`.
