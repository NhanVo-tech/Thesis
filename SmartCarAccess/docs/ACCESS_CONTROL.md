# ACCESS_CONTROL.md

Deterministic access-control layer (Stage 2, milestones M-B + M-C): geofencing plus the
3-state access state machine that unlocks/locks the car and authorizes engine start from the
fused UWB position/velocity — **without AI**. This document is the reference for the report's
Phase C/D description.

> Last updated: 2026-09-05.

---

## 1. Scope

Sits on top of trilateration + EKF. It answers the *geometry/kinematics* questions — where is
the user, how fast, in which zone — and drives the actuators. The AI intent classifier (later
milestone) will *gate* these transitions; this module is the deterministic core.

**Three demo scenarios** (always line-of-sight, phone held in hand):

1. Walk up to the driver door and nearly stop → **unlock**.
2. Sit (stand still) at the driver seat for 3 s → **lock + ignition authorized**.
3. Walk away from the car → **lock** (revoke ignition).

---

## 2. Data flow

```
d0,d1,d2 ──> Trilateration::solve ──> (x,y) + RMS
          ──> Ekf::update ──> (x,y,vx,vy)
          ──> Geofence::Hysteresis ──> stable Zone
          ──> AccessController::handlePosition(x,y,vx,vy) ──> relays / GPIO
```

`UwbBridge::tick()` calls `AccessController::handlePosition` at a fixed 100 ms cadence with the
EKF estimate; the EKF `predictTo` bridges dropped RANGE frames, so the access logic sees a
steady 10 Hz stream.

---

## 3. Geometry (car frame, metres)

Origin at vehicle centre; +X → right, +Y → forward.

| Point | (x, y) | Zone radius | Notes |
|-------|--------|-------------|-------|
| Driver door (unlock) | (−0.95, 0) | 2.0 m | coincides with anchor A2 (left B-pillar) |
| Driver seat | (−0.35, 0) | 0.45 m | 0.6 m "inside" the door |
| Welcome (car) | (0, 0) | 4.0 m | from centre |

Anchors (`uwb_geometry.h`): A0 (0, −2) rear centre, A1 (0.95, 0) right side,
A2 (−0.95, 0) left B-pillar.

---

## 4. Geofence (`geofence.h/.cpp`)

```
Zone { OUTSIDE, WELCOME, DRIVER_DOOR, DRIVER_SEAT }
```

`classify(x, y)` checks `DRIVER_SEAT` first (it is a subset of `DRIVER_DOOR`), then
`DRIVER_DOOR`, then `WELCOME`, else `OUTSIDE`. A `Hysteresis` wrapper (debounce = 3 frames =
300 ms @ 10 Hz) returns the **stable** zone to prevent boundary chatter; the state machine
consumes the stable zone.

---

## 5. Access state machine (`access_controller.h/.cpp`)

States:

| State | Door | Ignition | Meaning |
|-------|------|----------|---------|
| `LOCKED` | locked | off | initial / after leaving |
| `DOOR_UNLOCKED` | unlocked | off | user admitted, not yet seated |
| `OCCUPIED` | locked | authorized | user seated; engine may start |

Transitions (all gates deterministic):

```
LOCKED        ──(stable zone ∈ {DOOR,SEAT} AND speed < 0.4 m/s, 3 hits)──▶ DOOR_UNLOCKED
DOOR_UNLOCKED ──(stable zone = SEAT AND speed < 0.4 m/s for 3 s)────────▶ OCCUPIED
DOOR_UNLOCKED ──(zone = OUTSIDE AND moving away)────────────────────────▶ LOCKED
OCCUPIED      ──(zone = OUTSIDE AND moving away, 3 hits)────────────────▶ LOCKED
```

Gates:

- **Approach gate** — radial velocity `v_r > 0.10 m/s` (moving away) resets unlock counting.
- **Deceleration gate** — unlock only when `speed < 0.4 m/s` near the door, so a person who
  merely walks past (a tangential pass never stops) does **not** unlock.
- **Seat settle** — 3 s stationary inside `DRIVER_SEAT` before `OCCUPIED`.
- **Leave debounce** — 3 consecutive `OUTSIDE` + moving-away frames before `LOCKED`.

---

## 6. Actuators

| GPIO | Function | Level |
|------|----------|-------|
| 26 | Door lock relay | LOW = locked; 500 ms HIGH pulse = unlock |
| 27 | Engine-start authorization | HIGH = authorized |

---

## 7. Tunables (`access_controller.h`)

| Constant | Value | Meaning |
|----------|-------|---------|
| `STILL_SPEED_MPS` | 0.4 | below this = standing still |
| `REQUIRED_CONSECUTIVE_HITS` | 3 | unlock debounce (300 ms @ 10 Hz) |
| `SEAT_SETTLE_MS` | 3000 | sit-still duration → `OCCUPIED` |
| `LEAVE_CONSECUTIVE_HITS` | 3 | leave debounce |
| `APPROACH_SPEED_MIN_MPS` | 0.10 | radial-velocity approach gate |
| `RELAY_PULSE_MS` | 500 | unlock pulse width |

---

## 8. Serial log tags

| Tag | Example | Meaning |
|-----|---------|---------|
| `[POS2D]` | `x= y= rms=` | trilateration fix |
| `[EKF]` | `x= y= vx= vy= v=` | fused state |
| `[ZONE]` | `DRIVER_DOOR` | stable zone (on change) |
| `[STATE]` | `LOCKED / DOOR_UNLOCKED / OCCUPIED` | access state (on change) |
| `[IGNITION]` | `ON / OFF` | ignition authorization (on change) |
| `[DOOR]` | relay events | debug |

`localization_demo.py` parses `[STATE]` / `[IGNITION]` and shows a status banner
(`DOOR: … / IGNITION: …`), plus the driver-seat marker and zone on the map.

---

## 9. Scenario walkthrough (expected log progression)

| Scenario | Zone / log sequence |
|----------|---------------------|
| 1. Approach → unlock | `OUTSIDE → WELCOME → DRIVER_DOOR`, slow down, `[DOOR] FIRING UNLOCK RELAY`, `[STATE] DOOR_UNLOCKED` |
| 2. Sit → lock + ignition | `DRIVER_SEAT`, `[STATE] OCCUPIED`, `[IGNITION] ON` |
| 3. Leave → lock | `DRIVER_SEAT → DRIVER_DOOR → WELCOME → OUTSIDE` + moving away, `[IGNITION] OFF`, `[STATE] LOCKED` |

---

## 10. Division of responsibility (deterministic vs AI)

| Concern | Owner |
|---------|-------|
| Position `(x, y)` from `d0,d1,d2` | Trilateration |
| Smoothing, velocity `(vx, vy)`, NLOS rejection | EKF |
| Zone classification | Geofence (geometry) |
| Approach / leave velocity gate | AccessController (kinematics) |
| Unlock / seat-settle / leave decisions | AccessController (deterministic) |
| Intent (approach vs passing vs seated vs leave) | **AI (later milestone)** — gates the above |
