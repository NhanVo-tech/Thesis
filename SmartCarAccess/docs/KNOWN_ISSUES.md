# KNOWN_ISSUES.md

Annotated list of in-code markers (TODO / FIXME / HACK / NOTE / WARNING), placeholder logic, and inferred structural limitations.
Severity is a best-effort guess: **High** (security/correctness), **Medium** (robustness/tech-debt), **Low** (cosmetic/informational).

---

## 1. In-Code Markers & Placeholders

| Location | Type | Content / Meaning | Severity |
|----------|------|-------------------|----------|
| [software/smart_car_app/lib/service/ai_service.dart#L6](../software/smart_car_app/lib/service/ai_service.dart#L6) | HACK/SECURITY | Hardcoded Google Gemini API key `_apiKey = 'AIza…'` committed in source | **High** |
| [software/smart_car_app/lib/service/gps_service.dart#L175](../software/smart_car_app/lib/service/gps_service.dart#L175) | Placeholder | "we'll use XOR-based encryption" — `encryptData()` is a non-cryptographic placeholder matching ESP32's simple decryption | **High** |
| [software/smart_car_app/lib/service/gps_service.dart#L177](../software/smart_car_app/lib/service/gps_service.dart#L177) | NOTE | "This is a placeholder that works with ESP32's simple decryption" | **High** |
| [iot/src/uwb/uci_session_manager.cpp#L292](../iot/src/uwb/uci_session_manager.cpp#L292) | HACK/experimental | Large commented-out **relay-attack simulation** blocks (Classic Jitter / Creeping / Step&Hold). Must be guarded behind a compile-time flag; must never ship enabled | **High** |
| [iot/src/test/test_fsm.cpp#L95](../iot/src/test/test_fsm.cpp#L95) | TODO | "Provision device, then try again - should succeed" — incomplete test path | Medium |
| [iot/src/provisioning_phase.cpp#L266](../iot/src/provisioning_phase.cpp#L266) | Deprecated | "private key is stored as raw scalar in CCC mailbox, not PEM" — `getDevicePrivateKeyPEM` legacy stub | Low |
| [iot/src/ccc_mailbox.cpp#L326](../iot/src/ccc_mailbox.cpp#L326) | WARNING (runtime) | Logs "vehicle signing RNG seed failed" — signing can fail if DRBG seed fails | Medium |
| [iot/src/nfc_session.cpp#L262](../iot/src/nfc_session.cpp#L262) | WARNING (runtime) | "CCC vehicleId missing; using fallback for WRITE DATA" | Medium |
| [iot/src/fsm/fsm.cpp#L641](../iot/src/fsm/fsm.cpp#L641) | WARNING (runtime) | Duplicate transition detection warning in transition table | Medium |
| [software/smart_car_app/lib/service/pke_auth_orchestrator.dart (~L752)](../software/smart_car_app/lib/service/pke_auth_orchestrator.dart) | WARNING (runtime) | "Failed to reset ephemeral keys" error log path | Low |
| `…/android/.../ProvisioningHostApduService.kt#L75` | NOTE | "We now only enforce login for the signature step. Allow SELECT and base (Lc=0) for testing." — relaxed HCE gating for testing | **High** |
| `…/android/.../MainActivity.kt#L204` | NOTE | "Do NOT use setPreferredService for HCE" | Low |
| [iot/src/ble/ble_auth.cpp#L208](../iot/src/ble/ble_auth.cpp#L208) | NOTE | `(void)label; (void)data; (void)len;` — suppressed-unused stub, indicates unfinished path | Medium |

> **Security callout for the user:** A live-looking Gemini API key is committed at [ai_service.dart#L6](../software/smart_car_app/lib/service/ai_service.dart#L6). Treat it as compromised: rotate/revoke it and move it to secure configuration (env, remote config, or a backend proxy). Also review the HCE test-bypass note and force-provisioning flags before any production build.

---

## 2. Documented Limitations (from README / PAPER)

| Item | Detail | Severity |
|------|--------|----------|
| Share slots 1–7 disabled | Friend key sharing is verified but intentionally disabled pending final policy | Medium |
| Token MAC binding missing | Share payload token MAC binding not implemented | Medium |
| No trusted time source | NTP/RTC/BLE time sync not enforced for time-bound attestations | Medium |
| Attack simulation guard | Simulation code should be behind a compile-time flag for production | High |
| Debug/test bypass | Debug/test bypass features must be disabled in production (`isTestBypassEnabled`, force-provisioning) | High |
| LSTM warm-up mismatch | Header comment says window full = 25 frames; runtime prints "warm-up: N/15" — inconsistent readiness threshold | Low |

---

## 3. Structural Limitations

Inferred architectural constraints visible from the code:

1. **Single-peer UWB session.** `UciSessionManager` (legacy path) tracks one `activeCfg_`/`activeSessionId_`; concurrent multi-device ranging is not modeled. The ESP32-C3 Bridge path (`thesis253_workspace`) also runs one session per C3 node.
2. **ESP-NOW + BLE coexistence.** Master ESP32-S3 uses WiFi radio for both ESP-NOW (bridge communication) and BLE (phone connection). Both protocols share the same 2.4 GHz PHY. Current testing shows no conflicts, but heavy BLE activity could add latency to ESP-NOW ranging reports.
3. **Static LSTM scalers baked into firmware.** `scaler_mean`/`scaler_scale` and `TIME_STEPS`/`NUM_FEATURES` are compile-time constants; retraining requires reflashing and regenerating `uwb_lstm_model.h`.
4. **Hardware-coupled constants.** `kAntennaOffsetM`, relay pin, and UART pins (master side and C3 bridge side) are hardcoded; porting to different hardware requires source edits.
5. **Relay fired directly from `UwbDoorUnlock`.** The unlock decision is local to the UWB path; the FSM's `UNLOCKING_*` states and the AI-gated relay are two partially parallel unlock routes (legacy vs. new) — integration is not fully unified.
6. **Cloud is untrusted-by-design but app-side crypto is incomplete.** GPS confidentiality currently relies on an XOR placeholder, so the encrypted-channel guarantee is not yet real end-to-end.
7. **No automated CI test suite.** Only native FSM tests and manual on-device/Python test harnesses exist.
8. **App secrets in client.** The Gemini key and AI calls run client-side; there is no backend proxy, exposing keys and prompt logic on-device.
9. **Firmware/app protocol coupling.** BLE UUIDs, APDU instruction codes, and OOB byte layouts are duplicated on both sides and must be kept in sync manually.
10. **ESP-NOW StartSession struct must stay synchronized** between Master (`uci_host_bridge.cpp`) and ESP32-C3 Bridge (`espnow_link.cpp`). Both use `#pragma pack(1)` with identical field order. Any field addition/removal requires updating both sides simultaneously.
