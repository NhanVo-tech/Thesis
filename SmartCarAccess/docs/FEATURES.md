# FEATURES.md

Feature checklist. Status legend: ✅ done · 🔄 partial / gated · ❌ not started · ⏸ dormant
(implemented but not wired).

> Last updated: 2026-09-02.

---

## Security & Identity

| Feature | Status | Notes |
|---------|--------|-------|
| CCC-style confidential mailbox (NVS `ccc_dk`) | ✅ | vehicle ID/keys, 8 slots, tokens, fast artifact |
| Vehicle P-256 keypair (first-boot gen + validate) | ✅ | `ccc_mailbox.cpp` |
| NFC Phase A provisioning (SPAKE2+ HMAC, fail-closed) | ✅ | PN532 reader ↔ Android HCE |
| Owner endpoint key + immobilizer token (slot 0) | ✅ | tokens stay in vehicle |
| Fast-transaction artifact (pre-shared, versioned) | ✅ | skips ECDH on re-auth |
| BLE Phase B auth (ephemeral ECDH + HKDF session keys) | ✅ | CCC tunnel APDU |
| Challenge-response bound to vehicle ID | ✅ | `vehicleId ‖ nonce` |
| Epoch time-sync over EXCHANGE | ✅ | bounded 2020–2100 |
| Digital-key attestation (owner, ECDSA, time-bounded) | ✅ | 147-byte payload |
| Friend key sharing (slots 1–7) | 🔄 | verified, disabled by policy; token MAC binding pending |
| AES secure-channel echo | ✅ | session-key smoke test |
| Android Keystore identity key (non-exportable) | ✅ | alias `smart_car_phone_identity_p256` |

## UWB Proximity Pipeline

| Feature | Status | Notes |
|---------|--------|-------|
| Multi-anchor DS-TWR ranging (phone ↔ 3 anchors) | ✅ | `android.ranging` controller `0x06C1` |
| PC bridge distance aggregation + forwarding | ✅ | `run_fira_bridge.py`, `RANGE:` over USB-CDC |
| Ranging start/stop control (CMD/ACK) | ✅ | gated by BLE session (CCC `0x84/0x85`) |
| Freshness + in-bounds validity mask | ✅ | `valid=1` only when all 3 fresh & in range |
| 2-D trilateration (2-circle / Gauss-Newton) | ✅ | `trilateration.cpp`, returns RMS |
| 2-D EKF position + velocity tracking | ✅ | `ekf_stub.cpp`, R from RMS, predict bridges drops |
| Geometric zone unlock + hysteresis | ✅ | unlock/reset radius, consecutive hits |
| Radial-velocity approach gate | ✅ | rejects users moving away/passing by |
| Relay actuation (pulse) | ✅ | GPIO26, 500 ms |
| On-device intent classifier (CNN-LSTM) | ❌ | roadmap M5/M6 |
| 1-D LSTM relay-attack model (TFLM) | ⏸ | retained, not wired |
| Geofenced multi-zone actuation (trunk/cabin) | ❌ | roadmap M7/M8 |

## Mobile App

| Feature | Status | Notes |
|---------|--------|-------|
| Firebase auth (email + Google) | ✅ | |
| Cars / digital keys / access logs (Firestore streams) | ✅ | `car_service.dart` |
| Master-card provisioning (NDEF `{vid, msk}`) | ✅ | → HCE session |
| BLE Phase B orchestration (retry/backoff, fast tx) | ✅ | `pke_auth_orchestrator.dart` |
| Phone-side UWB ranging (multicast DS-TWR) | ✅ | `uwb_multi_service.dart` (API 36+) |
| GPS capture + encryption + HMAC + auto-sync | ✅ | 30 s cadence |
| Background foreground service (auto Phase B) | ✅ | Doze exemption helper |
| Access-pattern anomaly (time/location/frequency) | ✅ | rule-based scorer |
| AI-enriched anomaly (Gemini 2.5 Flash Lite) | ✅ | ALLOW / CONFIRM / BLOCK |
| Push + local notifications (EN/VI) | ✅ | FCM + local |
| Multi-language UI (EN/VI) | ✅ | `language_service.dart` |

## Tooling & Data

| Feature | Status | Notes |
|---------|--------|-------|
| PC bridge CLI (`run_fira_bridge.py`) | ✅ | anchors + ESP port, tunable rate/bounds |
| Serial CSV logger | ✅ | `[LSTM_DATA]` → labelled CSV |
| EKF analysis / tuning tool | ✅ | `analyze_ekf.py` (metrics/noise/trajectory/sweep) |
| Real-time visualizer (PDF/SVG export) | ✅ | `realtime_lstm_visualizer.py` |
| Phase B simulators | ✅ | `demo_ble_auth.py`, `phase_b_test.py` |
| Labelled datasets (normal/loiter/relay) | ✅ | `uwb_lstm_data_label{0,1,2}.csv` |

See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) and [STAGE2_PLAN.md](STAGE2_PLAN.md).
