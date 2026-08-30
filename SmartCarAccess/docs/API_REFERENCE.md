# API_REFERENCE.md

Public functions and interfaces across firmware (C++) and app (Dart/Kotlin). Signatures reflect the source headers. Side effects noted where relevant.

---

## 1. Firmware — C++

### 1.1 `CCCMailbox` — [iot/include/ccc_mailbox.h](../iot/include/ccc_mailbox.h)
| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `bool begin()` | Init mailbox, load from NVS, generate identity on first boot | success | NVS read/write, RNG |
| `const CCC_Mailbox& get()` | Access RAM-mirrored mailbox | struct ref | — |
| `const char* vehicleId()` | Vehicle ID string | C-string | — |
| `bool hasVehiclePub()` | Vehicle public key present | bool | — |
| `bool getVehiclePub(uint8_t* out, size_t max)` | Copy 65-byte vehicle pub | success | writes `out` |
| `bool hasVehiclePriv()` | Vehicle private key present | bool | — |
| `bool signVehicleDataP256(const uint8_t* data, size_t dataLen, uint8_t* sigDerOut, size_t sigDerMax, size_t* sigDerLenOut)` | ECDSA-P256 sign with vehicle key | success | mbedTLS RNG/sign |
| `bool hasEndpointPub(uint8_t slot=0)` | Slot phone key present | bool | — |
| `bool getEndpointPub(uint8_t* out, size_t max, uint8_t slot=0)` | Copy slot phone key | success | writes `out` |
| `bool setEndpointPub(const uint8_t* pub65, uint8_t slot=0)` | Store slot phone key | success | NVS write |
| `bool clearEndpointPub(uint8_t slot=0)` | Clear slot phone key | success | NVS write |
| `bool isSlotActive(uint8_t slot)` / `bool setSlotActive(uint8_t slot, bool)` | Slot bitmap get/set | bool | NVS on set |
| `bool hasToken/getToken/setToken/clearToken/ensureToken(uint8_t slot[, …])` | Immobilizer token ops (32 B) | success | NVS |
| `uint16_t signalingBitmap()` / `bool setSignalingBitmap(uint16_t)` / `bool setSignalingFlag(uint16_t mask, bool)` | Signaling flags | value/success | NVS on set |
| `void clearMailboxes()` | Clear slots/signaling/tokens (keep identity) | void | NVS write |
| `void clearAll()` | Clear everything incl. identity | void | NVS write |

### 1.2 `NfcSession` — [iot/include/nfc_session.h](../iot/include/nfc_session.h)
| Signature | Description | Side effects |
|-----------|-------------|--------------|
| `void begin(HardwareSerial& uart, int rxPin, int txPin, uint32_t baud=115200)` | Init PN532 over HSU | UART init |
| `void tick()` | Run one iteration of the Phase A provisioning loop | APDU I/O, FSM events |
| `void setPersistentForce(bool)` / `bool getPersistentForce()` | Persistent force-provisioning flag | NVS |
| `void armOneShotForce()` / `bool isOneShotArmed()` | One-shot force provisioning | state |

### 1.3 `ProvisioningPhase` — [iot/include/provisioning_phase.h](../iot/include/provisioning_phase.h)
| Signature | Description | Returns |
|-----------|-------------|---------|
| `void begin()` | Init provisioning + CCC bindings | void |
| `bool isProvisioned()` | Phone key already stored | bool |
| `bool storePhonePubRaw(const uint8_t* pub65)` | Persist phone long-term key | success |
| `bool storeCertChain(const uint8_t* cert, size_t len)` | Persist cert blob | success |
| `bool storeFastArtifact(uint8_t version, const uint8_t* key32)` / `bool hasFastArtifact()` / `bool getFastArtifact(uint8_t* ver, uint8_t key32[32])` / `bool clearFastArtifact()` | Fast-path artifact | success |
| `bool setOwnerProvisioned(const uint8_t* pub65, bool force)` | Store phone pub, set slot 0, gen `tok_0` | success |
| `bool verifySignatureP256(const uint8_t* pub65, const uint8_t* data, size_t len, const uint8_t* sigDer, size_t sigLen)` | Verify ECDSA-P256 DER | valid |
| `void clearAll()` / `void clearProvisionedOnly()` / `void clearProvisionedData()` | Clear provisioning state | void |
| `bool setForceProvisioningFlag(bool)` / `bool isForceProvisioning()` / `void setOneShotForce(bool)` | Force-provisioning controls | success |
| `size_t getPhonePubRaw(uint8_t* out, size_t max)` / `size_t getCertChain(uint8_t* out, size_t max)` | Read-back diagnostics | bytes copied |
| `bool validateCertPublicKeyMatchesPub(const uint8_t* cert, size_t len, const uint8_t* pub65)` / `bool validateStoredCertMatchesStoredPub()` | Cert/key consistency | valid |
| *(deprecated)* `bool runOnceWithHce(...)`, `size_t getDevicePrivateKeyPEM(...)` | Legacy | — |

### 1.4 `BLEMod` — [iot/include/ble/ble.h](../iot/include/ble/ble.h)
| Signature | Description |
|-----------|-------------|
| `void begin()` | Init NimBLE, GATT server, register services, advertise |
| `void tick()` | Advertising fast→slow demotion |
| `void restartAdvertising(bool requestFastProfile=false, const char* reason=nullptr)` | Restart advertising |
| `bool isStarted()` / `const char* deviceName()` | Status |
| `AdminMode getAdminMode()` / `void setAdminMode(AdminMode)` | Admin mode (`ADMIN_NORMAL/ENROLL/REMOVE`) |
| `void adminNotify(const char* msg)` | Push admin status |
| `bool isSessionReady()` | Phase B session ready |

### 1.5 `BLEAuth` — [iot/include/ble/ble_auth.h](../iot/include/ble/ble_auth.h)
| Signature | Description | Side effects |
|-----------|-------------|--------------|
| `void registerService(NimBLEServer* server, mbedtls_ctr_drbg_context* drbg)` | Register AUTH service, start crypto worker | GATT |
| `bool isSessionReady()` | Session established | — |
| `const uint8_t* sessionEncKey()` / `size_t sessionEncKeyLen()` | Session enc key | — |
| `const uint8_t* sessionMacKey()` / `size_t sessionMacKeyLen()` | Session MAC key | — |
| `void resetSession()` | Clear keys/buffers on disconnect | state |
| `void setGpsDataCallback(GpsDataCallback cb)` | Register GPS callback `(data,len,verified)` | — |
| `void printStats()` | Debug stats | Serial |

### 1.6 `BLEAdmin` / `BLEAttestation` / `BLEEcho`
- `BLEAdmin::registerService(NimBLEServer*)`, `getAdminMode()/setAdminMode(...)`, `notify(const char*)`.
- `BLEAttestation::registerService(NimBLEServer*)`.
- `BLEEcho::registerService(NimBLEServer*, mbedtls_ctr_drbg_context*)`.

### 1.7 `PKETelemetry` — [iot/include/ble/pke_telemetry.h](../iot/include/ble/pke_telemetry.h)
| Signature | Description |
|-----------|-------------|
| `const char* eventName(Event)` | Event → name |
| `void startAttempt(const char* vehicleIdAscii=nullptr)` | Begin unlock attempt |
| `uint32_t attemptId()` | Current attempt id |
| `void setVehicleId(const char*)` | Set vehicle id context |
| `void emit(Event, int rssiDbm=127, const char* unlockDecision=nullptr, const char* details=nullptr)` | Emit telemetry |

### 1.8 `FSM` — [iot/include/fsm/fsm.h](../iot/include/fsm/fsm.h)
| Signature | Description | Returns |
|-----------|-------------|---------|
| `void begin()` | Init FSM + subsystems | void |
| `void tick()` | Main FSM loop step | void |
| `bool triggerEvent(Event)` | Queue an event | queued |
| `bool triggerEventWithData(Event, void* data, size_t len)` | Queue event + payload | queued |
| `State getCurrentState()` / `StateInfo getStateInfo()` | Introspection | value |
| `bool forceState(State, bool force=false)` | Force transition (testing) | success |
| `void reset(bool clearSession=false)` | Reset to IDLE | void |
| `bool isProvisioning()/isAuthenticating()/isUnlocking()/isInErrorState()` | Group predicates | bool |
| `uint32_t getTimeInCurrentState()` | ms in state | value |
| `const StateContext& getContext()` | Read-only context | ref |
| `void setStateTimeout(uint32_t ms)` / `void clearTimeout()` | Timeout control | void |
| `void onStateEntry(State, StateAction)` / `void onStateExit(State, StateAction)` | Register callbacks | void |
| `size_t getEventQueueSize()` / `bool isEventQueueFull()` / `void clearEventQueue()` | Queue status | value |
| `void setDebugLogging(bool)` | Toggle logging | Serial |
| `void printStatus()` / `void printTransitionTable()` | Debug dumps | Serial |
| `bool validateConfiguration()` | Detect unreachable/missing/duplicate transitions | valid |
| `const char* stateToString/eventToString/errorToString(...)` | Enum → string | C-string |

### 1.9 `FSMIntegration` — [iot/include/fsm/fsm_integration.h](../iot/include/fsm/fsm_integration.h)
Bridge wrappers translating callbacks to FSM events:
- **NFC**: `onCardDetected(const uint8_t uid[4])`, `onCardRemoved()`, `onSelectAIDSuccess/Failed()`, `onKeysExchanged(const uint8_t pub[65])`, `onKeysInvalid()`, `onCredentialsStored()`, `onTimeout()`, `onError(FSM::ErrorCode)`.
- **BLE**: `onClientConnected(const uint8_t addr[6])`, `onClientDisconnected()`, `onAuth0Received/onAuth0ResponseSent/onAuth1Sent/onAuth1ResponseReceived/onExchangeReceived/onExchangeResponseSent/onControlFlowReceived/onControlFlowResponseSent()`, `onClientHelloReceived(const uint8_t pub[65])`, `onServerHelloSent(const uint8_t pub[65])`, `onAuthVerified/onAuthFailed/onSessionExpired/onUnlockRequested()`, `onError(FSM::ErrorCode)`, `onAdminCommand()`.
- **Unlock**: `onProximityOK()`, `onProximityTooFar()`, `onSessionValid()`, … (see header).

### 1.10 `UwbUci::UciUartLink` — [iot/include/uwb/uci_uart_link.h](../iot/include/uwb/uci_uart_link.h)
| Signature | Description |
|-----------|-------------|
| `bool begin(HardwareSerial&, int rx, int tx, uint32_t baud)` | Init UART link |
| `void poll()` | Parse incoming bytes → packets |
| `bool sendPacket(Mt, uint8_t gid, uint8_t oid, const std::vector<uint8_t>& payload, uint8_t pbf=0)` | Send UCI packet |
| `void setPacketCallback(PacketCallback)` | Register `void(const UciPacket&)` |
| `bool isReady()` | Link ready |

### 1.11 `UwbUci::UciSessionManager` — [iot/include/uwb/uci_session_manager.h](../iot/include/uwb/uci_session_manager.h)
| Signature | Description | Side effects |
|-----------|-------------|--------------|
| `explicit UciSessionManager(UciUartLink&)` | Construct, bind callback, `lstm_ai_.begin()` | — |
| `void poll()` | Pump the UART link | UART |
| `bool runOnce(const UciRunConfig&)` | Full session: init→config→start | UART, session state |
| `bool stopActiveSession()` | Stop + deinit | UART |
| `bool isSessionActive()` | Status | — |
| `uint32_t rangingNotificationCount()` | Count | — |
| *(private)* `onPacket(...)` | Parse ranging notif → Kalman → LSTM → door | relay, LSTM |

### 1.12 `UwbUci` OOB helpers — [iot/include/uwb/uci_oob.h](../iot/include/uwb/uci_oob.h)
- `bool parseOobPayloadV1(const uint8_t* raw, size_t len, UciOobPayloadV1* out, const char** err)`
- `bool validateOobPayloadV1(const UciOobPayloadV1&, const char** err)`
- `void mapOobToRunConfig(const UciOobPayloadV1&, UciRunConfig*)`

### 1.13 `UwbUciHost` — [iot/include/uwb/uci_host_bridge.h](../iot/include/uwb/uci_host_bridge.h)
- `void init(UwbUci::UciSessionManager*)`, `void tick()`, `bool submitBleOob(const uint8_t* payload, size_t len, const char** err)`, `bool requestStart(const char** err)`, `bool requestStop(const char** err)`, `bool hasCachedConfig()`, `bool isBusy()`, `bool hasPending()`.

### 1.14 `LstmInference` — [iot/include/uwb/lstm_inference.h](../iot/include/uwb/lstm_inference.h)
| Signature | Description | Returns |
|-----------|-------------|---------|
| `LstmInference()` | Construct | — |
| `bool begin()` | Load TFLM model/interpreter | success |
| `bool predict(float filtered_m, float residual_m, float& p_walk, float& p_loiter, float& p_attack)` | Feed frame, infer when window ready | true if valid (false during warm-up) |
| `int getFrameCount()` | Frames in window | count |

### 1.15 `UwbDoorUnlock` — [iot/include/uwb/uci_door_unlock.h](../iot/include/uwb/uci_door_unlock.h)
| Signature | Description | Side effects |
|-----------|-------------|--------------|
| `void begin()` | Init module + GPIO | GPIO |
| `void handleRangingDistance(double m)` | Hysteresis-only unlock | relay |
| `void handleRangingWithAI(double m, float p_walk, float p_loiter, float p_attack)` | Proximity + AI gate | relay |
| `void tick()` | Background state mgmt | — |
| `bool isDoorUnlocked()` / `int getConsecutiveReadCount()` / `double getLastDistance()/getLastFilteredDistance()/getLastResidual()` | Telemetry | — |
| `void manualUnlock()` | Force relay (admin/test) | relay |
| `void resetDoorState()` | Reset state | — |

### 1.16 `Kalman` — [iot/lib/Kalman/Kalman.h](../iot/lib/Kalman/Kalman.h)
- `Kalman(double q, double r, double p, double initial)` — construct.
- `double getFilteredValue(double measurement)` — predict+update, returns filtered value.
- `void setParameters(double q, double r, double p)` — retune.

---

## 2. Mobile App — Dart (selected public API)

### 2.1 `PkeAuthOrchestrator` — `lib/service/pke_auth_orchestrator.dart`
- `Future<PhaseBResult> authenticate({String deviceAddress, BluetoothDevice? device, Duration? timeout, void Function(String)? onProgress})` — full Phase B handshake (Auth0/1/Exchange/ControlFlow), retries ×3. Returns session keys.
- `Future<PhaseBResult> authenticatePreferredDevice({Duration? timeout, onProgress})`.
- `Future<void> disconnect()`.
- `Future<bool> sendGpsLocation()` / `Future<bool> sendGpsPacket(GpsDataPacket)` — send encrypted GPS over BLE.
- `static Future<void> savePreferredDeviceAddress(String)` / `static Future<String?> loadPreferredDeviceAddress()`.
- Props: `bool hasActiveSession`, `Uint8List? sessionEncKey/sessionMacKey`.

### 2.2 `GpsService` — `lib/service/gps_service.dart`
- `Future<bool> checkAndRequestPermissions()`.
- `Future<Position?> getCurrentPosition({Duration timeout})`.
- `Future<String?> getAddressFromPosition(Position)`.
- `GpsDataPacket? buildEncryptedLocationPacket(Position, Uint8List encKey, Uint8List macKey)`.
- `Uint8List packageLocationData(Position)` — 32-byte packet.
- `Uint8List encryptData(Uint8List plaintext, Uint8List key)` — XOR placeholder.
- `Uint8List computeHMAC(Uint8List data, Uint8List key)` — HMAC-SHA256.
- `Future<Uint8List?> getEncryptedLocationData(Uint8List, Uint8List)` — 64-byte combined.

### 2.3 `CarService` — `lib/service/car_service.dart`
- `Future<String> addCar(Map)`, `Future<void> updateCar(String, Map)`, `Future<void> deleteCar(String)`.
- `registerOwnerProvisioningRecord(...)`, `Future<String> addDigitalKey(Map)`, `updateDigitalKey/deleteDigitalKey(...)`.
- Streams: `Stream<List<Map>> getUserCars()`, `getUserDigitalKeys()`; `Future<Map?> getCarById(String)`, `getDigitalKeyById(String)`.

### 2.4 `AnomalyDetectionService` — `lib/service/anomaly_detection_service.dart`
- `Future<AnomalyAnalysisResult> analyzeAccessEvent(AccessEvent)` — rule-based.
- `Future<AnomalyEnrichedDecision> analyzeAccessEventWithAI(AccessEvent)` — AI-scored + Firestore log.
- `Future<List<AnomalyAnalysisResult>> getUserAnomalyHistory(String userId, {int limit})`.

### 2.5 `AIService` — `lib/service/ai_service.dart`
- `static Future<AnomalyOutput> detectAnomalyWithAI(AnomalyInput)` — Gemini call, rule-based fallback. Thresholds: High ≥0.58 BLOCK, Medium ≥0.28 CONFIRM, else ALLOW.

### 2.6 `UwbService` — `lib/service/uwb_service.dart`
- `Future<void> ensureBluetoothReady()`, `Future<bool> connect(BluetoothDevice)`, `Future<void> disconnect()`, `dispose()`.
- `Future<Map> startSession(...)`, `Future<Map?> prepareSession(...)`, `getSessionStatus(...)`, `Future<bool> stopSession(int sessionId)`.
- Streams: `Stream<String> logs`, `Stream<Uint8List> infoNotifications`, `Stream<UwbOobPayload> oobPayloads`, `Stream<UwbRangingEvent> rangingEvents`.
- Static: `toHex(Uint8List)`, `formatShortMac(int)`.

### 2.7 `MasterCardProvisioningService` — `lib/service/master_card_provisioning.dart`
- `Future<MasterCardPayload> readMasterCard({Duration timeout})`, `cancelReadMasterCard()`.
- `savePendingPayload/loadPendingPayload/clearPendingPayload(String carId[, payload])`.
- `activateHceSession(MasterCardPayload, {Duration ttl})`, `clearHceSession()`, `Future<bool> isHceSessionActive()`.
- `getProvisioningVehicleBinding()`, `clearProvisioningVehicleBinding()`, `validateBindingConsistency(...)`.

### 2.8 Support services
- `AuthMethods`: `getCurrentUser()`, `signInWithGoogle(ctx)`, `signInWithEmailPassword(ctx, email, pw)`.
- `LanguageService` (ChangeNotifier): `setLanguage(String)`, `translate(String key)`, `currentLocale/currentLanguage`.
- `BleRuntimePermissionService`: `ensureReady({bool requestIfNeeded})`, `openSettings()`.
- `DozeExemptionService`: `getStatus()`, `requestExemption()`, `openBatteryOptimizationSettings()`.
- `BackgroundServiceControlService`: `isEnabled()`, `startService()`, `stopService()`, `toggleService()`.
- `PushNotificationService`: `showNotification({title, body, payload})`, `cancelAllNotifications()`.
- `NotificationService`: `showNotification(ctx,…)`, `showBlockingDialog(ctx,…)`, `getNotificationTitle/Body(severity)`.
- `PkeTelemetry`: `startAttempt()`, `emit({event, unlockDecision, details})`.

---

## 3. Android Native — Kotlin (`MethodChannel` interfaces)

| Channel | Method | Returns |
|---------|--------|---------|
| `smartcar/keystore` | `ensurePhaseAKey()` | bool |
| | `getPhaseAPublicKey65()` | ByteArray(65) |
| | `signPhaseA(ByteArray)` / `signPhaseAData(ByteArray)` | ByteArray (DER sig) |
| `smartcar/mastercard` | `setMasterSession(vehicleId, masterSecret, ttlSeconds)` | — |
| | `clearMasterSession()` / `isMasterSessionActive()` | bool |
| | `getProvisioningVehicleBinding()` / `clearProvisioningVehicleBinding()` | Map/bool |
| `smartcar/nfc_reader` | `enableReaderMode()` / `disableReaderMode()` | — |
| `smartcar/device_info` | `getAndroidSdkInt()` | int |
| | `getBatteryOptimizationStatus()` | Map{supported,ignored,sdkInt} |
| | `requestIgnoreBatteryOptimizations()` / `openBatteryOptimizationSettings()` | bool |
| Phase B (`HandshakeChannel`) | `generateEphemeralKeypair()` | {publicKey,privateKey} |
| | `getEphemeralPublicKey()` | ByteArray(65) |
| | `signEphemeralWithIdentity(ByteArray)` / `signChallenge(ByteArray)` | ByteArray sig |
| | `computeECDH({ecuPublicKey})` | ByteArray sharedSecret |
| | `deriveSessionKeys({sharedSecret, phoneEphemeralPub, ecuEphemeralPub})` | {encKey,macKey} |
| | `resetEphemeralKeys()` | true |

**HCE APDU interface** (`ProvisioningHostApduService.kt`, AID `A000000809434343444B467631`):
`0xCA` GET_CHALLENGE (Lc=0 base creds / Lc>0 signature), `0x30` SPAKE2_REQUEST, `0x32` SPAKE2_VERIFY, `0xD4` WRITE_DATA, `0x3C` OP_CONTROL, `0xB0` READ_BINARY, `0xDA` PROVISION_RESULT. SPAKE2 challenge TTL 45 s.

---

## 4. BLE GATT UUID Reference

| Service | Service UUID | Characteristics |
|---------|--------------|-----------------|
| Auth (Phase B / CCC tunnel) | `0000aaaa-1234-5678-9abc-def012345678` | RX `0000aac1-…` (write), TX `0000aac2-…` (read/notify) |
| Admin | `9a9b9c9d-0000-4000-8000-9a9b9c9d0000` | Mode `…0001`, Cmd `…0002`, Info `…0003`, PhoneKey `…0004` |
| Attestation | `555a0001-00aa-1111-2222-333344445555` | RX `555a0002-…`, TX `555a0003-…` |
| Secure Echo | `d0d0d0d0-0000-4000-8000-d0d0d0d00000` | In `…0005` (write), Out `…0006` (read/notify) |
