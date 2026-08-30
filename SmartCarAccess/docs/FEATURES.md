# FEATURES.md

Implemented-feature checklist for the **ESP32 Smart Car Access** project.
Status legend: ✅ Done · 🔄 Partial · ❌ Not started

---

## Core Features

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| CCC confidential mailbox (identity, slots, tokens) | ✅ | `iot/src/ccc_mailbox.cpp`, `iot/include/ccc_mailbox.h` | NVS namespace `ccc_dk`, RAM mirror |
| First-boot vehicle identity (`v_id`, `v_pub`, `v_priv`) | ✅ | `iot/src/ccc_mailbox.cpp` | Generated once |
| NFC Phase A provisioning (fail-closed APDU) | ✅ | `iot/src/nfc_session.cpp`, `iot/src/provisioning_phase.cpp`, `…/ProvisioningHostApduService.kt` | Validates content, not just status words |
| Owner enrollment (slot 0 + `tok_0`) | ✅ | `iot/src/provisioning_phase.cpp` | `setOwnerProvisioned()` |
| Share slots 1–7 (friends) | 🔄 | `iot/src/ccc_mailbox.cpp` | Verified but intentionally disabled pending policy |
| Token MAC binding for share payloads | ❌ | — | Not implemented (README limitation #2) |
| BLE Phase B authentication (ECDH + session keys) | ✅ | `iot/src/ble/ble_auth.cpp`, `…/pke_auth_orchestrator.dart`, `PhaseBCrypto.kt` | Instructions 0x80–0x83 |
| BLE Admin service (mode / cmd / phone-key upload) | ✅ | `iot/src/ble/ble_admin.cpp` | Chunked key upload |
| BLE Digital-Key attestation service | ✅ | `iot/src/ble/ble_attestation.cpp` | Auth_RX/Auth_TX |
| BLE AES-GCM secure echo | ✅ | `iot/src/ble/ble_echo.cpp` | Uses session key |
| Advertising fast→slow demotion | ✅ | `iot/src/ble/ble.cpp`, `iot/include/ble/ble_rollout.h` | Rollout flags |
| PKE telemetry events | ✅ | `iot/src/ble/pke_telemetry.cpp`, `…/service/pke_telemetry.dart` | Unlock decision logging |
| FSM orchestration + transition validation | ✅ | `iot/src/fsm/*.cpp` | Duplicate/unreachable detection |
| Master card HCE session (60 s TTL) | ✅ | `…/master_card_provisioning.dart`, `MasterCardSession.kt` | In-memory session |
| Firebase auth (email/Google) | ✅ | `…/service/auth.dart`, `login.dart`, `signup.dart` | |
| Cars & digital keys CRUD | ✅ | `…/service/car_service.dart` | Firestore `cars`, `digital_keys`, `Vehicles` |
| Multi-language (EN/VI) | ✅ | `…/service/language_service.dart` | SharedPreferences persisted |
| Background service / doze exemption | ✅ | `…/service/pke_background_service.dart`, `doze_exemption_service.dart` | Android |

## Algorithms

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| 1-D Kalman distance smoothing | ✅ | `iot/lib/Kalman/Kalman.h`, `iot/src/uwb/uci_session_manager.cpp` | q=0.05, r=0.2, p=1.0 |
| Residual + velocity feature extraction | ✅ | `iot/src/uwb/uci_session_manager.cpp`, `lstm_inference.cpp` | 3-feature vector |
| LSTM relay-attack detection (TFLM) | ✅ | `iot/src/uwb/lstm_inference.cpp`, `iot/include/uwb/uwb_lstm_model.h` | 25×3 window, 3-class softmax |
| Z-score feature normalization | ✅ | `iot/include/uwb/lstm_inference.h` | Static scaler from training set |
| AI-gated door unlock (hysteresis) | ✅ | `iot/src/uwb/uci_door_unlock.cpp` | 2.0/3.0 m, 3 hits, p_walk>0.80, p_attack>0.70 |
| UWB saturation/near-field handling | ✅ | `iot/src/uwb/uci_session_manager.cpp` | status 0x1B reuse < 0.5 m |
| Relay-attack simulation (dataset gen) | 🔄 | `iot/src/uwb/uci_session_manager.cpp` | Commented out; needs compile-time guard |
| App anomaly detection (rule-based) | ✅ | `…/service/{anomaly_detection_service,time_anomaly_detector,location_anomaly_detector}.dart` | Time + location + frequency |
| App anomaly detection (Gemini AI) | ✅ | `…/service/ai_service.dart`, `anomaly_scorer.dart` | Fallback to rules |
| Haversine distance | ✅ | `…/service/location_anomaly_detector.dart` | 6371 km radius |

## I/O & Integration

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| UCI/UART link (packet framing) | ✅ | `iot/src/uwb/uci_uart_link.cpp` | Mt/UciPacket |
| UWB session lifecycle (init/config/start/stop/deinit) | ✅ | `iot/src/uwb/uci_session_manager.cpp` | Retry logic |
| OOB session config (37-byte payload) | ✅ | `iot/src/uwb/uci_oob.cpp`, `uci_host_bridge.cpp` | Parse/validate/map |
| PN532 NFC reader (HSU) | ✅ | `iot/src/nfc_session.cpp`, `iot/lib/PN532/` | UART2 RX44/TX43 |
| Relay GPIO control | ✅ | `iot/src/uwb/uci_door_unlock.cpp` | GPIO26, 500 ms pulse |
| Serial structured logging | ✅ | `iot/src/uwb/*` | `[LSTM_DATA]`,`[AI]`,`[DOOR]`,`[UCI]` |
| CSV logger (dataset) | ✅ | `iot/tools/serial_csv_logger.py` | |
| Real-time academic visualizer | ✅ | `iot/tools/realtime_lstm_visualizer.py` | PDF/SVG export |
| App BLE (flutter_blue_plus) | ✅ | `…/service/pke_auth_orchestrator.dart`, `uwb_service.dart` | |
| App NFC (nfc_manager + HCE) | ✅ | `…/service/master_card_provisioning.dart`, Kotlin HCE | |
| App GPS packaging + HMAC | ✅ | `…/service/gps_service.dart` | 32-byte packet + HMAC-SHA256 |
| GPS encryption (production) | 🔄 | `…/service/gps_service.dart` | XOR placeholder only |
| Android Keystore P-256 bridge | ✅ | `KeystoreBridge.kt`, `MainActivity.kt` | `smartcar/keystore` channel |
| Push / in-app notifications | ✅ | `…/service/{push_notification_service,notification_service}.dart` | |
| Firebase Firestore integration | ✅ | `…/service/{car_service,database,anomaly_detection_service}.dart` | |
| Gemini API integration | ✅ | `…/service/ai_service.dart` | Hardcoded key (needs fix) |
| Trusted time source (NTP/RTC/BLE sync) | ❌ | — | Not enforced (README limitation #3) |

## Testing

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| FSM native unit tests | 🔄 | `iot/src/test/test_fsm.cpp`, `test_fsm.h` | Has a TODO at line 95 |
| FSM build-verify checks | ✅ | `iot/src/fsm/fsm_build_verify.cpp` | Config validation |
| BLE Phase B demo/test client (Python) | ✅ | `iot/tools/demo_ble_auth.py`, `phase_b_test.py` | |
| App Phase A/B test screen | ✅ | `…/screen/test_phase_ab.dart` | Live log UI |
| App UWB test screen | ✅ | `…/screen/test_uwb.dart` | Distance/zone display |
| App AI test harness (v1/v2) | ✅ | `…/screen/ai_test_harness.dart`, `ai_test_harness_v2.dart` | 6 predefined cases |
| App anomaly-notification test | ✅ | `…/test_anomaly_notifications.dart` | |
| App GPS test app/screen | ✅ | `…/main_gps_test.dart`, `…/screen/gps_test_screen.dart` | |
| Automated CI test suite | ❌ | — | None found in current codebase |
