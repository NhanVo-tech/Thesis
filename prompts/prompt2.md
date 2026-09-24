# Prompt 2 — Claude (synthesize prose)

You are a senior technical writer specializing in embedded systems and academic
documentation. You will receive `structured_facts.md` — a machine-extracted fact
sheet from a real codebase. Your job is to write polished technical prose for a
thesis system documentation.

## INPUT
Paste the FULL content of `structured_facts.md` below (do not summarize or drop
sections — every fact and every Mermaid block must be preserved):

<PASTE structured_facts.md HERE>

## YOUR JOB
For each of the 11 sections below, write:
1. A technical paragraph (3–6 sentences) explaining WHAT it is, HOW it works,
   and WHY it was designed this way.
2. Keep ALL Mermaid diagram blocks from the input — copy them VERBATIM, do not
   modify or redraw them.
3. Any [hard]-tagged fact must appear in the prose with its EXACT value.

## THESIS CONTEXT
University thesis on UWB-based smart car access (Stage 2, 2D localization).
System: ESP32-S3 master + 3 UWB anchors + Flutter/Kotlin Android app.
Core contributions: multi-anchor 2D UWB localization, trilateration + EKF
tracking, and on-device TFLM Conv1D intent recognition gating a 3-state
access state machine.
Tone: formal, precise, no marketing language. Undergraduate/graduate level.

## OUTPUT — strictly follow this 11-section structure

# Smart Car Access — System Architecture

## 1. Executive Summary
<prose: one paragraph overview of the full system — 4 domains: Phone / Anchors+PC bridge / ESP32 ECU / Firebase cloud>

## 2. NFC Provisioning (Phase A)
<prose: APDU flow, SPAKE2+ handshake, CCC mailbox (NVS), Android Keystore role>
[Mermaid sequence diagram — copy verbatim from structured_facts.md]

## 3. BLE Authentication (Phase B)
<prose: GATT tunnel, AUTH0/AUTH1/EXCHANGE/CONTROL_FLOW, ECDH+HKDF, fast transaction, RANGING_START/STOP>
[Mermaid sequence diagram — copy verbatim]

## 4. UWB Localization Pipeline
<prose: DS-TWR multicast, PC bridge as research transport, trilateration (2-circle + Gauss-Newton), EKF state model and no-coasting rule>
[Mermaid flowchart — copy verbatim]

## 5. Access Control & Geofencing
<prose: 4 zones with exact coordinates/radii, 3-state FSM, each gate (deceleration, debounce, radial-velocity, AI intent). Mention exact pin numbers RELAY=5, IGNITION=7.>
[Mermaid state diagram — copy verbatim]

## 6. Intent Recognition (TinyML)
<prose: data collection -> training -> uwb_traj_model.h -> TFLM inference -> gating. Mention window 25x4, 4 classes, thresholds 0.80/0.70.>
[Mermaid flowchart — copy verbatim]

## 7. Mobile App Architecture
<prose: Flutter service layer, Kotlin native modules, background service, dual-isolate handoff>

## 8. Cloud & Anomaly Detection
<prose: Firebase collections, anomaly scoring pipeline, Gemini enrichment, encrypted GPS sync>

## 9. FreeRTOS Runtime & Data Contracts
<prose: task table (name/priority/stack/role), RangingFrame + EKF state + Zone enum descriptions>

## 10. Key Constants Table
<render the constants table from structured_facts.md as prose-introduced table; do not drop any row or value>

## 11. End-to-End Unlock Sequence
<prose: 2–3 sentences connecting all phases — NFC provision -> BLE auth -> UWB locate -> geofence -> intent gate -> relay actuate>
[Mermaid sequence diagram — copy verbatim from structured_facts.md]

## RULES
- Never invent a value — use only what is in structured_facts.md.
- If a value is tagged [NOT FOUND IN CODE], write "implementation detail not exposed in public interface".
- Preserve every exact number: coordinates, thresholds, pin numbers, timing values.
- Formal academic English. No bullet points inside prose paragraphs (the constants table is allowed to be a table).
- Do not reproduce stale doc values (GPIO26/27, old anchor coords (0,2)/(-0.85,0), predictTo coasting).
- Sections must match this exact 11-section order and numbering.
