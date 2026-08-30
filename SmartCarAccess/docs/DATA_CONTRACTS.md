# DATA_CONTRACTS.md

All major data structures, structs, schemas, and significant types across the codebase, with field names, types, units, valid ranges (where inferable), and how they are passed between modules.

---

## 1. Firmware (C++)

### 1.1 `CCCMailbox::CCC_Slot` — `__attribute__((packed))`
File: [iot/include/ccc_mailbox.h](../iot/include/ccc_mailbox.h)

| Field | Type | Units / Range | Notes |
|-------|------|---------------|-------|
| `endpoint_pub` | `uint8_t[65]` | uncompressed EC point `0x04‖X‖Y` | Phone public key `ep_PK` |
| `immobilizer_token` | `uint8_t[32]` | 256-bit secret | UWB root of trust `tok_n` |

### 1.2 `CCCMailbox::CCC_Mailbox` — `__attribute__((packed))`
| Field | Type | Units / Range | Notes |
|-------|------|---------------|-------|
| `vehicle_id` | `char[9]` | 8 ASCII chars + NUL | Vehicle ID |
| `vehicle_pub` | `uint8_t[65]` | uncompressed EC point | Vehicle public key |
| `vehicle_priv` | `uint8_t[32]` | 256-bit | Vehicle private key (protected) |
| `signaling_bitmap` | `uint16_t` | 16 flags | Signaling state |
| `slot_bitmap` | `uint8_t` | bit0=owner, bits1–7=friends | Active slots |
| `slots` | `CCC_Slot[8]` | index 0=owner | Owner + 7 friends |
| `vehicle_identity_valid` | `bool` | — | Identity established |

**Passing**: obtained by const reference via `CCCMailbox::get()`; persisted to NVS (`ccc_dk`).

### 1.3 `FSM::StateContext`
File: [iot/include/fsm/fsm_states.h](../iot/include/fsm/fsm_states.h)

| Field | Type | Units / Range | Notes |
|-------|------|---------------|-------|
| `nfc_uid` | `uint8_t[4]` | — | NFC card UID |
| `phone_pub_key` | `uint8_t[65]` | uncompressed EC point | Phone long-term key |
| `phone_key_valid` | `bool` | — | Stored & valid |
| `ecu_ephemeral_pub` | `uint8_t[65]` | uncompressed EC point | ECU handshake key |
| `phone_ephemeral_pub` | `uint8_t[65]` | uncompressed EC point | Phone handshake key |
| `shared_secret` | `uint8_t[32]` | ECDH output | |
| `session_enc_key` | `uint8_t[32]` | AES-256 key | |
| `session_mac_key` | `uint8_t[32]` | HMAC key | |
| `session_keys_ready` | `bool` | — | |
| `retry_count` | `uint32_t` | count | |
| `last_activity_ms` | `uint32_t` | ms (millis) | |
| `last_error` | `ErrorCode` | enum | |
| `error_count` | `uint32_t` | count | |

**Passing**: mutable reference into state actions; read-only via `FSM::getContext()`. `reset()` clears validity/counters.

### 1.4 `FSM::StateInfo`
`current`, `previous`: `State`; `lastEvent`: `Event`; `lastError`: `ErrorCode`; `enter_time`: `uint32_t` (ms); `transition_count`: `uint32_t`. Returned by value from `FSM::getStateInfo()`.

### 1.5 `FSM::StateTransition`
`from_state`: `State`; `on_event`: `Event`; `to_state`: `State`; `guard`: `bool(*)(const StateContext&)`. Static transition table in `fsm.cpp`.

### 1.6 Enums — `FSM::State`, `FSM::Event`, `FSM::ErrorCode`
- `State`: INIT, IDLE, PROVISIONING_WAIT_TAP, PROVISIONING_SELECT_AID, PROVISIONING_EXCHANGE_KEYS, PROVISIONING_STORE_CREDS, AUTH_WAIT_AUTH0, AUTH_PROCESSING_AUTH0_STD, AUTH_WAIT_AUTH1_RESP, AUTH_SECURE_CHANNEL_READY, AUTH_WAIT_CONNECT (legacy), AUTH_HANDSHAKE, AUTH_VERIFY_KEYS, AUTH_SESSION_READY, UNLOCKING_CHECK_PROXIMITY, UNLOCKING_VERIFY_AUTH, UNLOCKING_EXECUTE, UNLOCKING_COMPLETE, ADMIN_MODE, ERROR_HANDLER.
- `Event`: SYSTEM_READY, TIMEOUT, ERROR_OCCURRED, RESET_REQUESTED, PROVISION_START, NFC_CARD_DETECTED, NFC_CARD_REMOVED, SELECT_AID_SUCCESS/FAILED, KEYS_EXCHANGED, KEYS_INVALID, CREDENTIALS_STORED, BLE_CLIENT_CONNECTED/DISCONNECTED, BLE_AUTH0_RECEIVED, BLE_AUTH0_RESP_SENT, BLE_AUTH1_SENT, BLE_AUTH1_RESP_RECEIVED, BLE_EXCHANGE_RECEIVED, BLE_EXCHANGE_RESP_SENT, BLE_CONTROL_FLOW_RECEIVED, BLE_CONTROL_FLOW_RESP_SENT, CLIENT_HELLO_RECEIVED, SERVER_HELLO_SENT, CLIENT_CONFIRM_RECEIVED, AUTH_VERIFIED, AUTH_FAILED, UNLOCK_REQUESTED, PROXIMITY_OK, PROXIMITY_TOO_FAR, AUTH_SESSION_VALID, AUTH_SESSION_EXPIRED, UNLOCK_EXECUTED, ADMIN_COMMAND, FORCE_PROVISION_ON/OFF, CLEAR_KEYS, DIAGNOSTICS_REQUEST.
- `ErrorCode`: NONE, NFC_INIT_FAILED, NFC_SAM_CONFIG_FAILED, NFC_TIMEOUT, NFC_APDU_FAILED, NFC_SELECT_AID_FAILED, BLE_INIT_FAILED, BLE_DISCONNECT_UNEXPECTED, BLE_INVALID_DATA, BLE_AUTH_FAILED, CRYPTO_SIGNATURE_INVALID, CRYPTO_ECDH_FAILED, CRYPTO_HMAC_MISMATCH, INVALID_KEY_FORMAT, STORAGE_WRITE_FAILED, STORAGE_READ_FAILED, INVALID_STATE_TRANSITION, NOT_PROVISIONED, SESSION_NOT_READY, SESSION_EXPIRED, AUTH_TIMEOUT.

### 1.7 `UwbUci::UciRunConfig`
File: [iot/include/uwb/uci_session_manager.h](../iot/include/uwb/uci_session_manager.h)

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `sessionId` | `uint32_t` | 42 | UWB session |
| `controlee` | `bool` | false | Role |
| `localMac` | `uint16_t` | 0x0000 | Own short MAC |
| `destMac` | `uint16_t` | 0x0001 | Peer short MAC |
| `channel` | `uint8_t` | 9 | UWB channel |
| `scheduleMode` | `uint8_t` | 1 | |
| `preambleIdx` | `uint8_t` | 9 | |
| `sfd` | `uint8_t` | 2 | |
| `slotDuration` | `uint16_t` | 2400 | |
| `rangingDuration` | `uint32_t` | 120 | ms interval |
| `slotsPerRr` | `uint8_t` | 6 | Slots per ranging round |
| `hoppingMode` | `uint8_t` | 1 | |
| `stsConfig` | `uint8_t` | 0 | |
| `aoaReport` | `uint8_t` | 1 | Angle-of-arrival |
| `vendorId` | `uint16_t` | 0x0708 | |
| `staticStsIv` | `uint8_t[6]` | 01..06 | STS IV |
| `resultReportConfig` | `uint8_t` | 0x0B | |
| `rframeConfig` | `uint8_t` | 0x03 | |

**Passing**: by const reference into `runOnce()`; copied into `activeCfg_`.

### 1.8 `UwbUci::UciOobPayloadV1` (`kVersion=1`, `kSize=37`)
File: [iot/include/uwb/uci_oob.h](../iot/include/uwb/uci_oob.h). Fields: `version:u8`, `role:u8` (0=controlee,1=controller), `sessionId:u32=42`, `phoneMac:u16=0x0001`, `carMac:u16=0x0000`, `channel:u8=9`, `preambleIdx:u8=9`, `sfdId:u8=2`, `stsConfig:u8=0`, `hoppingMode:u8=1`, `rframeConfig:u8=3`, `resultReportConfig:u8=0x0B`, `aoaResultReq:u8=1`, `scheduleMode:u8=1`, `multiNodeMode:u8=0`, `rangingRoundUsage:u8=2`, `rssiReporting:u8=1`, `slotDuration:u16=2400`, `rangingInterval:u32=120`, `slotsPerRr:u8=6`, `vendorId:u16=0x0708`, `staticStsIv:u8[6]`. **Passing**: parsed from raw BLE bytes → mapped into `UciRunConfig`.

### 1.9 `UwbUci::UciPacket` + `Mt`
File: [iot/include/uwb/uci_uart_link.h](../iot/include/uwb/uci_uart_link.h). `Mt { Data=0, Command=1, Response=2, Notification=3 }`; `UciPacket { Mt mt; uint8_t gid; uint8_t oid; uint8_t pbf; std::vector<uint8_t> payload; }`. **Passing**: by const reference to the `PacketCallback`.

### 1.10 `LstmInference` internal state
File: [iot/include/uwb/lstm_inference.h](../iot/include/uwb/lstm_inference.h). `window[25][3]` (float), `frame_count:int`, `last_filtered_m:float`, `scaler_mean[3]={4.87,-0.12,-0.08}`, `scaler_scale[3]={4.33,0.20,0.13}`, TFLM `model/interpreter/input/output`. Features order: `[distance(m), residual(m), velocity(m/s)]`.

### 1.11 `BLERollout::Flags`
File: [iot/include/ble/ble_rollout.h](../iot/include/ble/ble_rollout.h). `backgroundMode:bool`, `fastTransaction:bool`, `bondingEnforce:bool`, `rssiMonitorOnly:bool`, `rssiThresholdDbm:int`, `advFastMinMs/advFastMaxMs/advSlowMinMs/advSlowMaxMs:uint16_t`, `advFastWindowMs:uint32_t`. Built from compile-time `-D` defines.

### 1.12 `Kalman` (class fields)
File: [iot/lib/Kalman/Kalman.h](../iot/lib/Kalman/Kalman.h). `q` (process noise), `r` (measurement noise), `x` (filtered value), `p` (estimation error covariance), `k` (Kalman gain) — all `double`.

---

## 2. Mobile App (Dart)

### 2.1 `PhaseBResult`
`success:bool`, `message:String`, `sharedSecret:Uint8List?`, `sessionEncKey:Uint8List?` (32 B), `sessionMacKey:Uint8List?` (32 B), `challenge:Uint8List?`. Returned by `PkeAuthOrchestrator.authenticate()`.

### 2.2 `GpsDataPacket`
`position:Position`, `encryptedData:Uint8List` (encrypted 32 B + HMAC 32 B = 64 B), `plaintextData:Uint8List` (32 B). Plaintext layout (little-endian):

| Offset | Field | Type | Bytes |
|--------|-------|------|-------|
| 0 | latitude | double LE | 8 |
| 8 | longitude | double LE | 8 |
| 16 | altitude | float LE | 4 |
| 20 | accuracy | float LE | 4 |
| 24 | timestamp | int64 LE (ms epoch) | 8 |

### 2.3 `MasterCardPayload`
`vehicleId:Uint8List(8)`, `masterSecret:Uint8List(32)`; getters `vehicleIdHex`, `masterSecretHex`.

### 2.4 `ProvisioningVehicleBinding`
`vehicleId:Uint8List(8)`, `vehiclePubKey:Uint8List(65)`, `devicePubKey:Uint8List(65)`, `writeDataPayload:Uint8List?(77)`, `updatedAtMs:int?`.

### 2.5 `UwbOobPayload`
Mirrors firmware `UciOobPayloadV1` (version, role, sessionId, phoneMac, carMac, channel, preambleIdx, sfdId, stsConfig, hoppingMode, rframeConfig, resultReportConfig, aoaResultReq, scheduleMode, multiNodeMode, rangingRoundUsage, rssiReporting, slotDuration, rangingInterval, slotsPerRr, vendorId, staticStsIv). Helpers: `carIsController/Controlee`, `phoneMacString`, `carMacString`, `sessionKeyInfo`.

### 2.6 `UwbRangingEvent`
`type:String`, `raw:Map`, `distanceM:double?` (meters), `azimuthDeg:double?`, `elevationDeg:double?`, `elapsedRealtimeNanos:int?`, `elapsedMs:int?`, `positionSeen:bool?`. Factory `fromDynamic(dynamic)`.

### 2.7 `AnomalyInput`
`timestamp:DateTime`, `hour:int` (0–23), `weekday:int` (1–7), `location:Location{lat,lng:double}`, `distanceFromUsual:double` (km), `accessCountLastHour:int`.

### 2.8 `AnomalyOutput`
`isAnomalous:bool`, `confidenceScore:double` (0–1), `reason:String`, `severity:String` (low/medium/high), `action:String` (ALLOW/CONFIRM/BLOCK), `shouldNotify:bool`. `toJson()`, `toJsonString()`.

### 2.9 `AnomalyEnrichedDecision`
`isAnomalous:bool`, `confidenceScore:double`, `reason:String`, `severity:String`, `action:String`, `shouldNotify:bool`, `timestamp:DateTime`; getters `notificationTitle`, `notificationBody`.

### 2.10 `AnomalyAnalysisResult`
`isAnomalous:bool`, `confidence:double`, `severity:AnomalySeverity` (enum low/medium/high), `timeAnalysis:AnomalyResult`, `locationAnalysis:AnomalyResult`, `timestamp:DateTime`.

### 2.11 `AccessEvent`
`userId:String`, `carId:String`, `timestamp:DateTime`, `location:Position?`, `deviceInfo:Map?`.

### 2.12 Status/config classes
- `BleRuntimePermissionStatus`: `androidSdkInt:int`, `ready:bool`, `missing:List<String>`, `toUserMessage()`.
- `DozeExemptionStatus`: `androidSdkInt:int`, `supported:bool`, `ignored:bool`, `toUserMessage()`.
- `PkeRolloutFlags`: `backgroundMode/fastTransaction/bondingEnforce:bool`, `copyWith()`.

---

## 3. Cloud — Firestore Schemas

### `cars`
`ownerId:String`, `name/model/year:String`, `licensePlate:String`, `provisioned:bool`, `ownerProvisioning:Map`, `batteryLevel:int`, `isLocked:bool`, `isOnline:bool`, `createdAt/updatedAt:Timestamp`.

### `digital_keys`
`ownerId:String`, `carId:String`, `keyType:String` (Owner/Guest/Temporary), `expirationDate:Timestamp`, `permissions:List<String>`, `createdAt/updatedAt:Timestamp`.

### `Vehicles` (canonical registry)
`vehicle_id:String(hex)`, `owner_uid:String`, `device_pub_key:String(hex)`, `vehicle_pub_key:String(hex)`, `status:String`, `slots:List<Map>`, `write_data_payload:String(hex)`, `write_data_updated_at_ms:int`, `scanned_vehicle_id:String(hex)`, `car_doc_id:String`.

### `User`
`email:String`, `name:String`, `imgUrl:String`, `id:String`.

### `anomaly_analysis`
`userId/carId:String`, `timestamp:Timestamp` (serverTimestamp), `eventTimestamp:DateTime`, `location:Map{latitude,longitude}`, `deviceInfo:Map`, `timeAnalysis:Map`, `locationAnalysis:Map`, `overallScore/overallSeverity:String`, `isAnomalous:bool`.

---

## 4. Cross-Module Passing Semantics

| Data | From → To | Mechanism |
|------|-----------|-----------|
| `UciPacket` | `UciUartLink` → `UciSessionManager` | const-ref callback |
| Distance/features | `UciSessionManager` → `UwbDoorUnlock` | by value (double/float args) |
| Session keys | `BLEAuth` → callers | const pointer + length accessors |
| `StateContext` | `FSM` → actions | mutable ref (write) / const ref (read) |
| Phase B keys | Kotlin `PhaseBCrypto` → Dart | `MethodChannel` (ByteArray) |
| OOB payload | Dart → firmware | BLE bytes → `parseOobPayloadV1` |
| GPS packet | Dart `GpsService` → firmware `BLEAuth` | BLE write (64 B) |
| Anomaly result | services → Firestore | serialized `Map` |
