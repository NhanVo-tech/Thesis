# Beyond Relay Attacks: A Cost-Effective Cross-Layer and Robustness Evaluation of CCC Digital Key Systems

## 1. Purpose And Scope

This document is a comprehensive project review for paper writing. It focuses on what has already been implemented and validated across:

- Vehicle firmware (ESP32 + PN532 + NimBLE)
- Android native layers (HCE, Keystore, cryptographic channels)
- Flutter orchestration and UX
- Firebase data integration

Important scope note:

- UWB implementation details are intentionally deferred.
- This document still records current UWB-related placeholders and hooks (for future integration).

Project title target:

- Beyond Relay Attacks: A Cost-Effective Cross-Layer and Robustness Evaluation of CCC Digital Key Systems


## 2. High-Level System Architecture

The implemented design is cross-layer and protocol-driven:

1. NFC provisioning layer (Phase A)
- Phone emulates CCC applet using Android HCE.
- Vehicle (ESP32 + PN532) acts as reader and verifier.
- Owner key provisioning and vehicle binding are established.

2. BLE authentication layer (Phase B)
- APDU-like tunnel over BLE characteristics.
- Fresh ephemeral ECDH per session.
- Challenge/response signature verification before unlock path.

3. FSM orchestration and task separation
- Dedicated FreeRTOS tasks for FSM and NFC polling.
- BLE callbacks trigger FSM events without blocking-critical NFC loops.

4. App and cloud layer
- Flutter UI drives provisioning and diagnostics.
- Android native services hold sensitive operations.
- Firestore stores owner provisioning records (not immobilizer token secrets).


## 3. Completed Roadmap Progress (Current)

From the active rollout plan, completed items are:

1. Step 1 Add rollout flags (firmware + app): Done
2. Step 2 Shared telemetry format: Done
3. Step 3 BLE privacy + SMP bonding setup: Done
4. Step 4 Progressive bonding mode (warn but allow): Done
5. Step 5 ESP advertising duty cycle: Done
6. Step 6 Runtime diagnostics for ad profile/window: Done
7. Step 7 Android manifest/runtime permissions for background BLE + FGS notification: Done

Next incomplete step:

- Step 8 Android Doze exemption UX: Not started


## 4. Firmware Foundation Accomplishments (ESP32 Side)

### 4.1 CCC Mailbox As Vehicle Root-Of-Trust

Implemented in `include/ccc_mailbox.h` and `src/ccc_mailbox.cpp`.

Key design achievements:

- Persistent mailbox namespace: `ccc_dk` in NVS
- Vehicle identity assets persisted:
	- `vehicle_id` (8 ASCII bytes)
	- `vehicle_pub` (65 bytes)
	- `vehicle_priv` (32 bytes)
- Multi-slot model implemented (`slots[8]`):
	- Slot 0 owner
	- Slots 1-7 friend/share slots
- Per-slot assets:
	- Endpoint public key (65 bytes)
	- Immobilizer token (32 bytes)
- Bitmaps for slot activity and signaling are persisted.

Robustness and integrity logic already implemented:

- Automatic mailbox load and normalization on boot
- Migration from legacy endpoint key storage to slot model
- Vehicle keypair validation routine (`validateVehicleKeypair`)
- Fail-closed re-key when root identity material is missing/inconsistent

Security value:

- Prevents cloud from being a source of truth for unlock secrets
- Gives a local cryptographic trust anchor in the vehicle


### 4.2 FSM-Centered Runtime Model

Implemented in `src/main.cpp`, `include/fsm/fsm_states.h`, `src/fsm/fsm_integration.cpp`, and `src/fsm/fsm.cpp`.

What is achieved:

- Dedicated FreeRTOS task for FSM tick (1 ms cadence)
- Dedicated FreeRTOS task for NFC tick (2 ms cadence)
- Main loop kept lightweight for BLE profile maintenance
- Rich state and event model covering provisioning, auth tunnel, unlock, admin, and errors

Security and reliability impact:

- Reduces risk of BLE callback starvation caused by NFC blocking loops
- Keeps protocol event ordering deterministic through explicit transitions


## 5. NFC Provisioning Layer (Phase A) Accomplishments

Core implementation: `src/nfc_session.cpp`, `src/provisioning_phase.cpp`, Android `ProvisioningHostApduService.kt`.

### 5.1 APDU Flow Implemented End-To-End

Current production flow sequence:

1. SELECT AID
- Vehicle sends SELECT to CCC AID.
- Success requires both `9000` and expected payload bytes.

2. SPAKE2+ shell challenge
- Vehicle sends INS `0x30` with challenge TLV `0x50`.
- Verify uses INS `0x32` expecting MAC TLV `0x58`.
- Local HMAC-SHA256 computed and compared byte-for-byte.

3. Base GET DATA
- Vehicle sends INS `0xCA` with `Lc=0`.
- Payload unwrapping for tag `7F24` supported.
- Phone endpoint public key and optional chain parsed.

4. WRITE DATA
- Vehicle sends INS `0xD4` with TLVs:
	- `0x80` vehicle ID (8)
	- `0x81` vehicle pubkey (65)
- Android stores vehicle binding locally.

5. Signature GET DATA
- Vehicle sends INS `0xCA` with 24-byte challenge.
- Android signs challenge with Keystore private key.
- Vehicle verifies DER ECDSA signature with phone endpoint pubkey.

6. OP CONTROL commit gate
- Vehicle sends INS `0x3C` with `P1=0x11`.
- Provisioning is aborted if this commit gate fails.

7. Persist + notify
- Owner endpoint key stored in mailbox slot 0.
- Slot 0 activated, token ensured, signaling flag updated.
- Provision result sent via INS `0xDA`.


### 5.2 Reliability Hardening Added To NFC

Already implemented in code:

- Adaptive non-blocking card polling
- Stable-read requirement before card present/absent acceptance
- APDU retry with bounded attempts and backoff
- Reselect-based recovery path (`inRelease` + reselect + resend)
- Removal wait cap to avoid post-success lock loops

Observed benefits:

- Better first-tap behavior
- Reduced stale frame acceptance
- Better resilience to PN532 link instability and HCE reselect churn


### 5.3 Security Properties Enforced In Phase A

Implemented safeguards:

- Stale SELECT payload rejection (not status-word-only validation)
- HMAC verification required for SPAKE2+ shell
- Signature verification required before storing owner credential
- Commit-gate requirement before persistence
- Force provisioning mode controls (persistent and one-shot)


## 6. BLE Authentication Layer (Phase B) Accomplishments

Core implementation: `src/ble/ble_auth.cpp`, `src/ble/ble.cpp`, Android `PhaseBCrypto.kt`, `HandshakeChannel.kt`, Flutter `ble_phase_test.dart`.

### 6.1 APDU-Like CCC Tunnel Over BLE

Tunnel instructions implemented:

- AUTH0: `0x80`
- AUTH1: `0x81`
- EXCHANGE: `0x82`
- CONTROL FLOW: `0x83`

Flow already implemented:

1. AUTH0 standard request (`P1=0x11`)
2. ECU ephemeral generation
3. AUTH1 payload from ECU signed by vehicle private key
4. Phone returns AUTH1 response with phone ephemeral + signature
5. ECU verifies phone signature (using provisioned phone key)
6. ECDH shared secret computation
7. HKDF-SHA256 key derivation for ENC and MAC keys
8. EXCHANGE challenge issuance and signature verification
9. CONTROL FLOW ack triggers unlock event path


### 6.2 Session Hygiene And Memory Cleanup

Implemented in `reset_auth_state`:

- Explicit zeroing of ephemeral/public/signature/session buffers
- Cleanup of challenge/nonce/vehicle-id buffers
- State machine reset to idle

Security impact:

- Reduces key material lifetime in RAM
- Limits post-session residue exposure


### 6.3 Advertising Security-Robustness Policy

Implemented in `src/ble/ble.cpp`:

- Security config with bonding + LE Secure Connections
- Progressive rollout policy for unbonded peers
- Fast/slow advertising profile with bounded fast window
- Periodic ad diagnostics with transition counters

Current rollout defaults (from compile flags and app flags):

- background mode: off
- fast transaction: off
- bonding enforce: off (progressive allow mode)
- RSSI monitor-only: on


### 6.4 Observability For BLE Auth Performance

Implemented in firmware:

- Structured telemetry events: connect/auth0/auth1/auth_verified/control_flow_ack/unlock_decision
- `[AUTH-LAT]` timing report per run:
	- AUTH0 receive offset
	- AUTH1 send offset
	- auth verified offset
	- control flow ack offset
	- door-open-path latency

Paper value:

- Enables repeatable latency and stability measurements for robustness evaluation


## 7. BLE Attestation Service Accomplishments

Implementation: `src/ble/ble_attestation.cpp`.

What is implemented now:

- Dedicated BLE service for digital key attestation package writes
- Fixed-size attestation packet parsing
- Vehicle ID match validation
- Owner endpoint key retrieval from mailbox
- Payload hash and raw `(r,s)` P-256 signature verification
- Validity-window checks using device epoch

Policy status currently enforced:

- Slots 1-7 are intentionally locked (`ERR_SLOT_LOCKED`)
- Owner-only policy remains active until sharing enablement phase

Why this matters for paper claims:

- Cross-layer proof that key-sharing authorization can be cryptographically verified at vehicle side
- Demonstrates offline-verifiable trust transfer model independent of cloud trust


## 8. Android Native Layer Accomplishments

### 8.1 HCE Service With Protocol-Accurate APDU Dispatch

Implementation: `android/app/src/main/java/com/example/smart_car_app/ProvisioningHostApduService.kt`.

Implemented command handling:

- SELECT
- GET DATA (`0xCA`)
- SPAKE2 REQUEST (`0x30`)
- SPAKE2 VERIFY (`0x32`)
- WRITE DATA (`0xD4`)
- OP CONTROL (`0x3C`)
- PROVISION RESULT (`0xDA`)

Robustness/security logic:

- SPAKE2 challenge TTL (`15000 ms`)
- Pending challenge survival across link-loss deactivation
- WRITE DATA validation (TLV presence, byte lengths, pubkey format)
- Local provisioning result persistence for Flutter handshake
- Login gating path with explicit debug bypass policy


### 8.2 Android Keystore For Owner Identity Keys

Implementation: `KeystoreBridge.kt`.

Implemented capabilities:

- Ensure phase-A key pair exists (`secp256r1`)
- Export uncompressed public key (65-byte format)
- Sign challenge data via `SHA256withECDSA`

Security impact:

- Private key operations are performed through Android Keystore APIs
- Signature generation path is isolated from Flutter Dart layer


### 8.3 Native Handshake Channel For Phase B Crypto

Implementation: `HandshakeChannel.kt`, `PhaseBCrypto.kt`.

Implemented capabilities:

- Generate ephemeral keypair
- Sign ephemeral key with identity key
- Compute ECDH shared secret
- Derive session keys with HKDF-SHA256
- Sign EXCHANGE challenge

Paper value:

- Strong evidence of layered crypto split:
	- Firmware verifies
	- Android native computes/signs
	- Flutter orchestrates and displays


### 8.4 MainActivity Bridge And NFC Reader Coordination

Implementation: `MainActivity.kt`.

Bridged channels:

- Keystore channel
- Master card session channel
- NFC reader mode channel
- Device info channel (`getAndroidSdkInt`)

Key behavior achieved:

- Keeps HCE service behavior stable across app foreground/background lifecycle
- Toggles reader mode around provisioning sessions
- Exposes provisioning vehicle binding data to Flutter for integrity checks


## 9. Flutter Layer Accomplishments

### 9.1 Provisioning UX And Data Integrity Controls

Key files:

- `lib/service/master_card_provisioning.dart`
- `lib/screen/dashboard.dart`
- `lib/service/car_service.dart`

Implemented achievements:

- Master card scan flow with NFC session handling and timeout control
- HCE master session activation/clear with TTL
- Retrieval of Android-side WRITE DATA binding for verification
- Binding consistency validation before cloud registration
- Vehicle ID mismatch detection flow with user confirmation gate
- Stale binding timestamp guard before registration


### 9.2 Cloud Registration And Best-Effort Dual Write

Implemented in `car_service.dart`:

- Primary owner provisioning record merged into authorized `cars` document
- Secondary mirror write to `Vehicles` collection as best effort
- `permission-denied` on mirror path is tolerated, preserving successful local flow

Security/operational impact:

- Cloud cannot silently invalidate local cryptographic truth
- UX remains reliable when secondary collection rules differ


### 9.3 Shared Rollout Flags And Telemetry Schema

Implemented in:

- `lib/service/pke_rollout_flags.dart`
- `lib/service/pke_telemetry.dart`

Achievements:

- Persistent rollout flag defaults and updates in SharedPreferences
- Unified telemetry event names shared with firmware intent
- Structured app-side telemetry logging for attempt/event timeline alignment


### 9.4 Runtime Permission Hardening (Step 7)

Implemented in:

- `lib/service/ble_runtime_permissions.dart`
- `lib/screen/dashboard.dart`
- `lib/screen/location.dart`
- `lib/screen/test_phase_ab.dart`
- Android manifest + MainActivity bridge

Current behavior:

- API-aware BLE permission requirements checked at runtime
- Missing permissions produce explicit degraded-mode UX
- User actions provided: grant permissions or open settings
- BLE-critical paths are gated when prerequisites are missing


## 10. Android Permission And Manifest Hardening Achieved

Manifest now includes:

- NFC + HCE features
- BLE permissions (`BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT`, `BLUETOOTH_ADVERTISE`)
- Foreground/background prerequisites:
	- `FOREGROUND_SERVICE`
	- `FOREGROUND_SERVICE_CONNECTED_DEVICE`
	- `WAKE_LOCK`
	- `POST_NOTIFICATIONS`

This is a major cross-layer robustness improvement for staged background operation.


## 11. Security Analysis: What Is Already Defended

### 11.1 Replay And Stale Frame Defenses

- SELECT response payload strict-match check blocks stale acceptance
- SPAKE2 challenge MAC verification prevents replayed verify responses
- Challenge TTL on HCE side limits challenge reuse window
- Fresh BLE ephemeral key generation per session blocks key reuse replay


### 11.2 Spoofing And Impersonation Defenses

- Phone must prove possession of phase-A private key via signature
- Vehicle signs AUTH1 payload with mailbox vehicle private key
- ECU verifies phone AUTH1 response signature before deriving trust
- EXCHANGE signature required before session is promoted to ready


### 11.3 Downgrade Resistance (Current State)

- Standard path requires AUTH0 `P1=0x11`
- Fast transaction path (`P1=0x01`) is currently gated and unsupported by default
- Cryptographic algorithms are fixed to strong mainstream primitives:
	- P-256 ECDSA
	- ECDH
	- HKDF-SHA256
	- HMAC-SHA256


### 11.4 Fail-Closed Patterns

- Missing/invalid signature aborts provisioning
- OP CONTROL failure aborts persistence
- Missing WRITE DATA binding validity blocks owner registration path
- Unavailable session keys block secure EXCHANGE/CONTROL behaviors


## 12. Robustness Engineering Achievements

Major robustness work already completed:

- Non-blocking NFC polling and bounded removal waits
- APDU retries with reselection and bounded backoff
- Task separation to keep FSM progression responsive
- Diagnostic logs for ad profile transitions and periodic state
- Structured telemetry for cross-run behavior comparison
- Provisioning flow guarded against stale cloud/binding mismatches

Practical effect:

- Fewer transient provisioning failures
- Better explainability of failure points
- Better recoverability without manual reboot/reset loops


## 13. Current Limitations And Explicit Gaps (Code-Visible)

This section is intentionally honest and paper-ready.

1. UWB integration is not implemented yet
- Only foundational hooks/tokens and policy placeholders exist.

2. Fast transaction path is staged but not active
- AUTH0 `P1=0x01` currently returns unsupported in active path.

3. Time sync through BLE EXCHANGE is marked TODO
- Post-auth EXCHANGE payload handling exists as placeholder.

4. Background service stack is not complete yet
- Doze exemption UX not implemented (next rollout step).
- `flutter_background_service` integration not yet present.

5. Attestation friend slots intentionally locked
- Signature and validity checks run, but slot activation policy is not opened.

6. GPS payload encryption is currently simple XOR + HMAC in this test path
- Appropriate for controlled prototype testing but should be upgraded for production security claims.


## 14. Why This Is Cost-Effective Yet Security-Meaningful

The implemented architecture demonstrates cost-effectiveness through:

- Commodity hardware stack (ESP32 + PN532)
- Open cryptographic libraries and standards
- Android Keystore usage instead of custom secure element integration
- Staged feature-flag rollout reducing migration risk

And still delivers meaningful security improvements:

- Cryptographic proof-of-possession across NFC and BLE
- Vehicle-side trust anchored in local mailbox, not cloud authority
- Replay/stale handling and bounded failure recovery
- Observable, testable security-relevant telemetry


## 15. Suggested Paper Section Mapping (Directly From Code)

Use this mapping to convert implementation facts into paper sections.

1. Introduction and threat model
- Relay, replay, spoofing, downgrade, DoS assumptions.

2. Cross-layer architecture
- NFC provisioning, BLE auth tunnel, app/cloud orchestration.

3. Phase A implementation and hardening
- APDU flow, stale-frame handling, SPAKE2 shell MAC verification.

4. Phase B implementation and hardening
- AUTH0/AUTH1/EXCHANGE/CONTROL, ECDH + HKDF, session cleanup.

5. Robustness engineering
- Retry strategy, task separation, diagnostics, telemetry.

6. Security evaluation
- Attack classes and implemented mitigations.

7. Deployment and staged rollout
- Flags, progressive bonding policy, manifest/runtime permissions.

8. Limitations and future work
- UWB integration, doze exemptions, fast-path enablement, stronger location packet crypto.


## 16. Key Code Evidence Index

Firmware:

- `src/main.cpp`
- `include/ccc_mailbox.h`
- `src/ccc_mailbox.cpp`
- `src/nfc_session.cpp`
- `src/provisioning_phase.cpp`
- `src/ble/ble.cpp`
- `src/ble/ble_auth.cpp`
- `src/ble/ble_attestation.cpp`
- `src/ble/ble_admin.cpp`
- `src/ble/pke_telemetry.cpp`
- `include/fsm/fsm_states.h`
- `src/fsm/fsm_integration.cpp`

Android native:

- `android/app/src/main/AndroidManifest.xml`
- `android/app/src/main/kotlin/com/example/smart_car_app/MainActivity.kt`
- `android/app/src/main/java/com/example/smart_car_app/ProvisioningHostApduService.kt`
- `android/app/src/main/java/com/smartcaraccess/KeystoreBridge.kt`
- `android/app/src/main/java/com/smartcar/phaseb/PhaseBCrypto.kt`
- `android/app/src/main/java/com/smartcar/phaseb/HandshakeChannel.kt`
- `android/app/src/main/java/com/example/smart_car_app/MasterCardSession.kt`

Flutter:

- `lib/main.dart`
- `lib/service/master_card_provisioning.dart`
- `lib/service/car_service.dart`
- `lib/service/pke_rollout_flags.dart`
- `lib/service/pke_telemetry.dart`
- `lib/service/ble_phase_test.dart`
- `lib/service/ble_runtime_permissions.dart`
- `lib/screen/dashboard.dart`
- `lib/screen/location.dart`
- `lib/screen/test_phase_ab.dart`


## 17. UWB Placeholder For Future Paper Revision

When UWB module data/code is ready, append:

1. UWB trust handoff design from BLE-authenticated session
2. Distance bounding / ranging evaluation details
3. Relay-resistant proximity decisions with measured thresholds
4. Final unlock policy composition with BLE + UWB evidence


## 18. Final Summary For Writing

This project has already implemented a serious cross-layer digital key prototype with:

- Real NFC provisioning security controls
- Real BLE authenticated session establishment with modern cryptography
- Vehicle-local trust and key persistence model
- Cross-layer app/native integrity checks
- Structured rollout and diagnostics for empirical robustness analysis

For the paper, you can confidently claim that NFC and BLE layers are not only connected functionally, but coordinated through explicit cryptographic verification, state-machine control, and resilience engineering against common practical attack and failure classes. UWB can be added later as the final proximity-hardening layer on top of this foundation.

