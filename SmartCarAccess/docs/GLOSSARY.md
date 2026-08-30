# GLOSSARY.md

Domain-specific terms, acronyms, and project-specific identifiers used in **this** codebase, with the meaning as used here and where they appear.

---

## Protocols & Standards

| Term | Definition (as used here) | Appears in |
|------|---------------------------|-----------|
| **CCC** | Car Connectivity Consortium — the Digital Key Release 3 spec that inspires this system's architecture and mailbox model | README, `ccc_mailbox.*`, HCE AID |
| **Digital Key (DK)** | Phone-based cryptographic vehicle key replacing a physical fob | `ble_attestation.*`, README |
| **Phase A** | NFC-based owner **provisioning/enrollment** flow (APDU exchange) | `nfc_session.*`, `provisioning_phase.*`, `test_phase_ab.dart` |
| **Phase B** | BLE-based **authentication** flow establishing a session (PKE) | `ble_auth.*`, `pke_auth_orchestrator.dart` |
| **PKE** | Passive Keyless Entry — hands-free unlock via BLE presence/auth | `ble_rollout.h`, `pke_*`, telemetry |
| **UCI** | UWB Command Interface — FiRa/CCC framing used over UART to the UWB radio | `uci_uart_link.*`, `uci_session_manager.*` |
| **FiRa** | UWB industry consortium whose ranging spec the notification parsing follows | `uci_session_manager.cpp` |
| **HCE** | Host Card Emulation — Android emulates an NFC smartcard applet for Phase A | `ProvisioningHostApduService.kt`, `main.dart` |
| **APDU** | Application Protocol Data Unit — NFC command/response frames | `nfc_session.cpp`, HCE service |
| **AID** | Application Identifier for NFC SELECT; here `A000000809434343444B467631` | `ProvisioningHostApduService.kt` |
| **SPAKE2+** | Password-authenticated key exchange (shell/HMAC form) used during provisioning | README, HCE `handleSpake2*` |
| **OOB** | Out-Of-Band — UWB session config (`UciOobPayloadV1`, 37 bytes) delivered over BLE | `uci_oob.*`, `uci_host_bridge.*` |
| **OP CONTROL** | Provisioning "commit gate" APDU; failure aborts persistence | README, HCE `handleOpControl` |

## Cryptography

| Term | Definition | Appears in |
|------|-----------|-----------|
| **ECDH** | Elliptic-Curve Diffie–Hellman key agreement (P-256) → shared secret | `ble_auth.*`, `PhaseBCrypto.kt`, FSM `CRYPTO_ECDH_FAILED` |
| **ECDSA-P256** | Signature scheme over secp256r1; DER-encoded | `signVehicleDataP256`, `verifySignatureP256`, `KeystoreBridge.kt` |
| **HKDF-SHA256** | Key derivation producing `session_enc_key`/`session_mac_key` | `PhaseBCrypto.kt`, Phase B |
| **HMAC** | Keyed hash; used for GPS packet integrity and provisioning auth | `gps_service.dart`, README |
| **DRBG / CTR-DRBG** | mbedTLS deterministic RNG context passed to BLE services | `ble_auth.h`, `ble_echo.h`, `ccc_mailbox.cpp` |
| **AES-GCM** | Authenticated encryption in the secure echo service | `ble_echo.*` |
| **Uncompressed EC point** | 65-byte public key `0x04‖X‖Y` used everywhere for P-256 keys | `CCC_Slot`, `StateContext`, Keystore |
| **Immobilizer token (`tok_n`)** | 32-byte per-slot root-of-trust secret that must never leave the vehicle | `CCC_Slot.immobilizer_token`, README |

## Project-Specific Identifiers

| Term | Definition | Appears in |
|------|-----------|-----------|
| **CCC Mailbox** | Vehicle confidential store in NVS (`ccc_dk`): identity, slots, tokens | `ccc_mailbox.*` |
| **`v_id`** | 8-byte (char[9]) vehicle ID generated on first boot | `CCC_Mailbox.vehicle_id` |
| **`v_pub` / `v_priv`** | Vehicle public/private key (65 B / 32 B) | `CCC_Mailbox` |
| **`ep_pub` / `ep_PK`** | Endpoint (phone) public key stored per slot | `CCC_Slot.endpoint_pub` |
| **`slot_bitmap`** | Active-slot bitmap; bit0 = owner, bits1–7 = friends | `CCC_Mailbox.slot_bitmap` |
| **`signaling_bitmap`** | 16-bit signaling flags | `CCC_Mailbox.signaling_bitmap` |
| **Slot 0 / Owner slot** | The owner's provisioned key slot | `setOwnerProvisioned`, README |
| **Share slots (1–7)** | Friend key slots — verified but currently disabled | README limitations |
| **Fast artifact** | Versioned fast-path key (32 B) generated during provisioning | `storeFastArtifact`, `provisioning_phase.h` |
| **Master card / MasterCardSession** | NFC card carrying `{vehicleId, masterSecret}`; 60 s HCE session | `master_card_provisioning.dart`, `MasterCardSession.kt` |
| **Write Data payload** | 77-byte provisioning binding blob | `car_service.dart`, `DataStoreUtil.kt` |
| **FSM** | Finite State Machine coordinating provisioning/auth/unlock | `fsm/*` |
| **StateContext** | Data carried across FSM transitions | `fsm_states.h` |
| **AUTH0 / AUTH1 / EXCHANGE / CONTROL FLOW** | CCC tunnel handshake steps (BLE instructions 0x80–0x83) | `ble_auth.cpp`, `pke_auth_orchestrator.dart`, FSM events |

## UWB / Signal Processing

| Term | Definition | Appears in |
|------|-----------|-----------|
| **Ranging** | UWB distance measurement between phone and vehicle | `uci_session_manager.*` |
| **AoA** | Angle of Arrival (`aoaReport`/`aoaResultReq`) | `UciRunConfig`, `UwbRangingEvent` |
| **STS** | Scrambled Timestamp Sequence (secure UWB) — `stsConfig`, `staticStsIv` | `UciRunConfig`, `UciOobPayloadV1` |
| **SFD** | Start-of-Frame Delimiter index | `UciRunConfig.sfd` |
| **Preamble index** | UWB preamble code index | `UciRunConfig.preambleIdx` |
| **Slots per RR** | Slots per ranging round (`slotsPerRr`) | `UciRunConfig` |
| **Controller / Controlee** | UWB session roles (initiator vs. responder) | `UciRunConfig.controlee`, `role` |
| **Short MAC** | 2-byte UWB device address (`XX:XX`) | `main.cpp parseMacShort`, `localMac/destMac` |
| **Antenna offset** | +0.24 m constant correction applied to raw distance | `kAntennaOffsetM` |
| **Saturation (status 0x1B)** | Too-close/near-field UWB status; last filtered value reused | `uci_session_manager.cpp` |
| **Residual** | `raw − filtered` distance; anomaly feature for the LSTM | `uci_session_manager.cpp`, `lstm_inference.*` |
| **Kalman filter** | 1-D scalar low-pass filter smoothing distance | `Kalman.h` |
| **Hysteresis** | Dual-threshold (2.0 m / 3.0 m) anti-chatter unlock logic | `uci_door_unlock.*` |

## AI / ML

| Term | Definition | Appears in |
|------|-----------|-----------|
| **LSTM** | Long Short-Term Memory network classifying approach behaviour on-device | `lstm_inference.*` |
| **TFLM / TFLite Micro** | TensorFlow Lite for Microcontrollers runtime | `lstm_inference.*`, `uwb_lstm_model.h` |
| **`p_walk` / `p_loiter` / `p_attack`** | Softmax outputs: normal approach / loitering / relay attack | `lstm_inference.*`, `uci_door_unlock.*` |
| **Relay attack** | Spoofing proximity by relaying/manipulating UWB distance | README, LSTM labels, sim blocks |
| **Label 0 / 1 / 2** | Training-set classes walk / loiter / attack (`uwb_lstm_data_label{0,1,2}.csv`) | dataset files |
| **Z-score scaler** | `(x−μ)/σ` normalization using training-set `scaler_mean`/`scaler_scale` | `lstm_inference.h` |
| **TIME_STEPS / window** | 25-frame sliding window fed to the LSTM | `lstm_inference.h` |
| **Gemini** | Google `gemini-2.5-flash-lite` LLM used for access-pattern anomaly scoring | `ai_service.dart` |
| **Anomaly severity / action** | low/medium/high → ALLOW/CONFIRM/BLOCK decision | `anomaly_scorer.dart`, `ai_service.dart` |
| **Haversine** | Great-circle distance formula for location anomaly | `location_anomaly_detector.dart` |

## Platform / Infrastructure

| Term | Definition | Appears in |
|------|-----------|-----------|
| **NVS** | ESP32 Non-Volatile Storage (namespace `ccc_dk`) | `ccc_mailbox.cpp` |
| **HSU** | High-Speed UART interface to the PN532 NFC chip | `nfc_session.*`, `platformio.ini` |
| **NimBLE** | Lightweight BLE stack (h2zero Arduino port) | `ble.cpp`, `platformio.ini` |
| **DW3000 / nRF52840** | UWB transceiver + companion MCU behind the UCI link | README, `main.cpp` |
| **yolo_uno** | Custom ESP32-S3 board target | `platformio.ini`, `boards/yolo_uno.json` |
| **Doze** | Android battery-optimization mode; exemption requested for reliability | `doze_exemption_service.dart` |
| **MethodChannel** | Flutter↔native (Kotlin) bridge (`smartcar/*` channels) | `MainActivity.kt` |
| **Foreground/Background service** | Persistent BLE/GPS monitoring on Android | `pke_background_service.dart` |
| **Rollout flags** | Compile-time PKE feature toggles (`PKE_ROLLOUT_*`) | `ble_rollout.h`, `platformio.ini` |
