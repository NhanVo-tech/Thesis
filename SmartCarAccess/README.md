## ESP32 Smart Car Access (IoT) - Academic Project README

This repository contains the in-vehicle side of a digital key system inspired by CCC Release 3, plus a companion Android application for provisioning, authentication, and user-facing controls. The system is built to study secure access control with NFC, BLE, and UWB, and to evaluate edge AI for relay-attack detection.

Core security rules:
1. Immobilizer secrets stay in the vehicle.
2. The vehicle trusts only cryptographically verifiable data.

---

## Abstract

This project implements a multi-factor, multi-transport digital key system on an ESP32 platform. The vehicle uses NFC for initial owner provisioning, BLE for authenticated session establishment, and UWB for proximity verification. A Kalman-filtered ranging pipeline produces stable distance measurements and feature residuals, which are fed into an on-device LSTM model (TensorFlow Lite Micro) to classify normal approach, loitering, and relay attacks. The result is a defense-in-depth architecture that couples cryptographic authentication with physical proximity validation and lightweight anomaly detection.

---

## Repository Structure

- Firmware (ESP32, PlatformIO): [iot](iot)
- Android app (Flutter + Kotlin): [software/smart_car_app](software/smart_car_app)
- UWB/AI tools (Python): [iot/tools](iot/tools)

Key firmware modules:
- UWB session manager: [iot/src/uwb/uci_session_manager.cpp](iot/src/uwb/uci_session_manager.cpp)
- Door unlock logic: [iot/src/uwb/uci_door_unlock.cpp](iot/src/uwb/uci_door_unlock.cpp)
- LSTM inference: [iot/src/uwb/lstm_inference.cpp](iot/src/uwb/lstm_inference.cpp)
- LSTM configuration: [iot/include/uwb/lstm_inference.h](iot/include/uwb/lstm_inference.h)

---

## System Architecture

### Components

1. Vehicle (ESP32 firmware)
   - NFC reader (PN532) for owner provisioning
   - BLE services for authentication and attestation
   - UWB session manager for ranging and anomaly detection
   - Secure mailbox in NVS for confidential state

2. Phone (Android + Flutter)
   - HCE applet for NFC APDU exchange
   - Android Keystore for private key operations
   - UI for provisioning, diagnostics, and test harnesses

3. Cloud (Firebase)
   - Stores app metadata and share records
   - Does not store immobilizer tokens

### Data Flow (High Level)

1. NFC provisioning establishes owner keys
2. BLE authentication establishes a trusted session
3. UWB ranging measures proximity
4. Kalman + LSTM pipeline classifies user behavior
5. Door relay fires only when both cryptographic and physical criteria are met

---

## Security Model and Threat Scope

### Security Goals

- Prevent unauthorized unlocks without a valid cryptographic identity
- Prevent relay attacks that spoof proximity
- Keep immobilizer tokens local to the vehicle
- Enable offline verification for attestation packages

### Threats Considered

- Relay attacks (UWB distance manipulation)
- Stale APDU responses
- BLE replay or timing anomalies

### Trust Boundaries

- Vehicle: root of trust for immobilizer secrets
- Phone: holds private keys in Android Keystore
- Cloud: untrusted for unlock decisions

---

## CCC Mailbox (Vehicle Confidential Storage)

Stored in ESP32 NVS (`ccc_dk`) and mirrored in RAM:
- `v_id` (8 bytes): Vehicle ID generated once
- `v_pub` (65 bytes): Vehicle public key generated once
- `ep_pub` (65 bytes): Owner public key from successful provisioning
- `slot_bmp` (8 bits): Active slots bitmap
- `sig_bmp` (16 bits): Signaling flags bitmap
- `tok_0..tok_7` (32 bytes each): Immobilizer tokens

Current behavior:
1. First boot creates `v_id` and `v_pub`.
2. Owner provisioning fills slot 0 and ensures `tok_0` exists.
3. Share slots (1..7) remain inactive until key sharing activation is enabled.

---

## Provisioning (Phase A - NFC)

Provisioning is implemented as a strict, fail-closed APDU flow. The vehicle validates content, not only status words, to prevent stale-frame acceptance.

High-level steps:
1. SELECT AID: Validate AID and response payload tags
2. Authentication (SPAKE2+ shell): HMAC verification
3. GET DATA: Pull phone endpoint key and optional chain
4. WRITE DATA: Push vehicle public key and bind
5. Signature proof: Phone signs challenge, ESP32 verifies with `ep_pub`
6. Commit gate (OP CONTROL): Failure aborts persistence
7. Persist owner slot and tokens
8. Final result notify and session close

---

## BLE Authentication (Phase B)

Purpose: prove the phone owns the enrolled key and establish a session for secure commands.

High-level steps:
1. Exchange ephemeral public keys
2. Verify phone signature with stored owner key
3. Derive shared session keys via ECDH
4. Perform challenge-response bound to `v_id`
5. Notify FSM and open encrypted command path

---

## UWB Ranging and AI Pipeline

### Measurement Processing

The UWB session manager parses FiRa/CCC UCI ranging notifications and applies robustness logic, including:

- Saturation handling: status `0x1B` reuses last filtered distance if in the near-field window
- Antenna offset correction: constant offset applied to distance
- Sanity bounds: out-of-range distances are dropped

Key constants (current values):
- Antenna offset: 0.24 m
- Near-field reuse threshold: 0.5 m

### Kalman Smoothing

Each valid measurement is filtered with a 1D Kalman filter. Residuals are computed to capture timing anomalies:

$$r_t = d^{raw}_t - d^{filt}_t$$

The velocity feature is computed from filtered distance:

$$v_t = \frac{d^{filt}_t - d^{filt}_{t-1}}{\Delta t}$$

### Feature Normalization

Each feature is normalized using z-score parameters from the training dataset:

$$x' = \frac{x - \mu}{\sigma}$$

### LSTM Inference

The LSTM model runs on-device using TensorFlow Lite Micro. It consumes a sliding window of features:

- Features: [distance, residual, velocity]
- Window length: configured by `TIME_STEPS`
- Output: softmax probabilities for `p_walk`, `p_loiter`, `p_attack`

Implementation references:
- LSTM configuration: [iot/include/uwb/lstm_inference.h](iot/include/uwb/lstm_inference.h)
- LSTM runtime: [iot/src/uwb/lstm_inference.cpp](iot/src/uwb/lstm_inference.cpp)

### Door Unlock Logic (AI-Gated)

Unlock is a conjunction of physical proximity and AI confidence, with hysteresis to prevent chattering:

- Distance threshold: 2.0 m
- Reset threshold: 3.0 m
- Required consecutive hits: 3
- Attack reject: if `p_attack > 0.70`, the relay is disabled
- Positive accept: if distance <= 2.0 m and `p_walk > 0.80`

Door control reference: [iot/include/uwb/uci_door_unlock.h](iot/include/uwb/uci_door_unlock.h)

---

## Experimental Controls and Attack Simulation

The UWB pipeline includes an optional relay-attack simulation block for dataset collection. It injects synthetic latency spikes into the distance signal at a fixed cadence. This block is useful for controlled experiments but must be disabled for real-world evaluation.

Attack simulation location: [iot/src/uwb/uci_session_manager.cpp](iot/src/uwb/uci_session_manager.cpp)

---

## Data Logging and Visualization

### Serial Log Formats

The firmware emits structured logs for real-time visualization and dataset collection:

- Distance and residual:
  - `[LSTM_DATA],<timestamp_ms>,<raw_m>,<filtered_m>,<residual_m>`
- AI output:
  - `[AI] Walk: <p> | Loiter: <p> | Attack: <p>`
- Door action:
  - `[DOOR] *** FIRING UNLOCK RELAY ***`

### Tools

1. CSV logger for ML dataset collection:
   - [iot/tools/serial_csv_logger.py](iot/tools/serial_csv_logger.py)

2. Real-time academic plotter (3 stacked subplots, shared time axis, vector export):
   - [iot/tools/realtime_lstm_visualizer.py](iot/tools/realtime_lstm_visualizer.py)

Tool documentation:
- [iot/tools/README_VISUALIZATION.md](iot/tools/README_VISUALIZATION.md)

---

## Build and Run

### Firmware (PlatformIO)

From repository root:

```
cd iot
platformio run
platformio run --target upload
platformio device monitor
```

Configuration file:
- [iot/platformio.ini](iot/platformio.ini)

### Python Tools (Visualization)

```
cd iot/tools
pip install -r requirements.txt
python realtime_lstm_visualizer.py --port COM6
```

Requirements:
- [iot/tools/requirements.txt](iot/tools/requirements.txt)

Export vector plots:

```
python realtime_lstm_visualizer.py --port COM6 --export-pdf scenario_walk.pdf
python realtime_lstm_visualizer.py --port COM6 --export-svg scenario_walk.svg
```

---

## Evaluation Protocol (Suggested)

To demonstrate cause-effect relationships between physical signals and AI decisions, capture three representative scenarios and export each to PDF/SVG:

1. Normal walk-in
   - Distance decreases smoothly
   - Residual remains low
   - `p_walk` increases toward 1.0
   - Door opens and a shaded band appears in the plot

2. Relay attack
   - Distance shows abrupt drops
   - Residual spikes
   - `p_attack` dominates
   - No door-open shading

3. Loitering
   - Distance oscillates around the threshold
   - Velocity fluctuates near zero
   - `p_loiter` dominates
   - No door-open shading

These plots provide publication-quality evidence of the system's decision logic and are designed for academic figures and posters.

---

## Limitations and Future Work

1. Share slots 1..7 are verified but intentionally disabled pending final policy decisions.
2. Token MAC binding for share payloads is not yet implemented.
3. Trusted time sources (NTP/RTC/BLE sync) should be enforced for time-bound attestations.
4. Attack simulation code should be guarded behind a compile-time flag for production.

---

## Security Notes

1. Owner private key remains in Android Keystore.
2. ESP32 stores owner public key and verifies locally.
3. Cloud must never store immobilizer tokens.
4. Debug/test bypass features must be disabled in production.

---

## Related Documentation

- Detailed visualization guide: [iot/tools/README_VISUALIZATION.md](iot/tools/README_VISUALIZATION.md)
- UWB AI implementation details: [iot/IMPLEMENTATION_SUMMARY.md](iot/IMPLEMENTATION_SUMMARY.md)
