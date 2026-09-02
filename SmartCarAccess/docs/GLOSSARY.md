# GLOSSARY.md

Domain terms and acronyms as used in the **Smart Car Access** codebase.

> Last updated: 2026-09-02.

## Protocols & Standards

- **CCC (Car Connectivity Consortium) Digital Key** — industry standard for phone-as-key. This
  project is *inspired by* Release 3 (NFC provisioning, BLE authentication, UWB proximity).
- **APDU** — Application Protocol Data Unit; command/response framing used for NFC (Phase A) and
  tunneled over BLE (Phase B CCC tunnel).
- **HCE (Host Card Emulation)** — Android capability letting the phone emulate an NFC card
  (`ProvisioningHostApduService`).
- **NDEF** — NFC Data Exchange Format; the master card stores `{vid, msk}` as an NDEF text record.
- **DS-TWR (Double-Sided Two-Way Ranging)** — UWB ranging method used here in **multicast /
  one-to-many** mode (one controller, three responders).
- **FiRa / UCI** — FiRa Consortium UWB profile / UWB Command Interface; the anchors are driven via
  UCI by the PC bridge (Qorvo `uci` package). Historically UCI ran directly on the ESP32; it is
  now handled entirely on the PC.

## Cryptography

- **ECDSA / ECDH (P-256, secp256r1)** — signatures / key agreement used throughout.
- **HKDF-SHA256** — derives session ENC/MAC keys from the ECDH shared secret.
- **HMAC-SHA256** — SPAKE2+ verification (Phase A) and GPS payload integrity.
- **SPAKE2+** — password-authenticated key exchange shell used during Phase A (HMAC over the
  master-card shared secret).
- **Fast artifact** — a pre-shared, versioned key that lets an already-provisioned phone skip ECDH
  during Phase B (fast transaction).

## Project Identifiers

- **CCC Mailbox** — the vehicle's confidential store (NVS namespace `ccc_dk`).
- **Immobilizer token** — per-slot 32-byte secret that stays in the vehicle; root of trust for
  key sharing. Never leaves the ESP32.
- **Endpoint key (`ep_pub`)** — a phone's public identity key stored in a slot.
- **Vehicle ID (`v_id`)** — 8-char identifier (`"VN" + 6`).
- **Slot** — one of 8 key holders: slot 0 = owner, slots 1–7 = friends.
- **Controller `0x06C1`** — the phone's pinned UWB address; anchors are responders `mac 0/1/2`.

## UWB / Signal Processing

- **Anchor** — a fixed UWB node (DWM3001CDK). Three anchors at A0 (0, 2), A1 (−0.85, 0),
  A2 (0, −2) m in the car frame.
- **RANGE frame** — a PC→ESP line `RANGE:d0,d1,d2,valid` with the three anchor distances.
- **Validity mask** — bitfield; `0x07` when all three readings are fresh and in-bounds.
- **Trilateration** — computing planar `(x, y)` from three distances; 2-circle geometry for two
  anchors, Gauss-Newton least-squares for three.
- **RMS residual** — trilateration fix-quality metric; feeds the EKF measurement noise.
- **EKF (Extended/linear Kalman filter)** — 2-D constant-velocity filter with state `[x, y, vx, vy]`;
  smooths NLOS/multipath spikes and estimates velocity.
- **Radial velocity (`v_r`)** — velocity component toward/away from the unlock point; `>0` = moving
  away. Drives the approach gate.
- **Hysteresis** — separate unlock (2 m) and re-arm (3 m) radii to avoid relay chatter.
- **GDOP** — Geometric Dilution of Precision; anchor-placement quality metric (relevant to
  future geometry tuning).

## AI / ML

- **CNN-LSTM (roadmap)** — planned hybrid trajectory classifier over `[x, y, vx, vy]` windows
  (approaching / passing-by / anomaly).
- **1-D LSTM (dormant)** — the retained Stage-1 model on `[distance, filtered, residual]` with
  softmax `[p_walk, p_loiter, p_attack]`; not wired into the current pipeline.
- **TFLM (TensorFlow Lite for Microcontrollers)** — on-device inference runtime.
- **Access-pattern anomaly** — app-side scoring of *when/where/how often* keys are used (distinct
  from UWB relay-attack detection).
- **Gemini 2.5 Flash Lite** — cloud LLM used to enrich anomaly decisions.

## Platform / Infrastructure

- **ESP32-S3 (`yolo_uno`)** — the vehicle ECU (BLE peripheral, NFC reader, fusion + relay).
- **USB-CDC** — native USB serial link carrying `RANGE`/`CMD`/`ACK` between PC and ESP32.
- **PN532** — NFC reader IC (UART HSU) used for Phase A.
- **NVS** — ESP32 non-volatile storage.
- **FreeRTOS** — RTOS running the FSM/NFC/UWB tasks.
- **Firebase / Firestore / FCM** — cloud auth, database, push notifications.
