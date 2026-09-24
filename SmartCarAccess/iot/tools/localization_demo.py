#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""localization_demo.py — self-contained UWB localization demo.

Starts the 3 UWB anchors as FiRa responders, forwards their distances to the
ESP32-S3 over USB-CDC (exactly like run_fira_bridge.py), and live-visualizes the
EKF position of a person walking around the car on a clean 2D localization map.

Display is the Tkinter top-down map from car_position_display.py: a metric-native
car body with the 3 UWB anchors, range circles, the live EKF position and trail,
plus a side panel with anchor ranges, position readout, and access state.

Usage (set PYTHONPATH to the uci + uqt-utils libs first, as for run_fira_bridge.py):

    $env:PYTHONPATH = "<repo>/lib/uwb-uci;<repo>/lib/uqt-utils"
    python localization_demo.py -p COM11 COM19 COM12 --macs 0 1 2 --esp-port COM5

Replay a previously captured log instead of running live:

    python localization_demo.py --log capture.log

Run the geometry + solver + Tk UI simulation without hardware:

    python localization_demo.py --simulate 200

Simulate a lost anchor (default: right-side anchor, index 1) without touching
hardware — its distance is forced to 0 so the ESP32 falls back to its 2-anchor
trilateration path:

    python localization_demo.py ... --drop-anchor 1   # -1 disables

Parsed ESP32 lines (same format as analyze_ekf.py):
    [RANGE3] t=<ms> d0=.. d1=.. d2=.. n=.. mask=..   (n = #fresh anchors, mask bits d2 d1 d0)
    [POS2D]  t=<ms> x=.. y=.. rms=..
    [EKF]    t=<ms> x=.. y=.. vx=.. vy=.. v=..
    [INTENT] p_approach=.. p_seated=.. p_leave=.. p_passing=..
"""

import argparse
import math
import os
import queue
import re
import sys
import threading
import time

import numpy as np

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None

try:
    from uci import *  # noqa: F401,F403
    UCI_AVAILABLE = True
except Exception:  # pragma: no cover
    UCI_AVAILABLE = False

# -----------------------------------------------------------------------------
# Geometry — MUST match iot/include/uwb/uwb_geometry.h and access_controller.h.
# -----------------------------------------------------------------------------
ANCHORS = [
    (0.0, -2.0),    # anchor 0 (d0, COM11) — rear centre
    (0.95, 0.0),    # anchor 1 (d1, COM19) — right side
    (-0.95, 0.0),   # anchor 2 (d2, COM12) — left B-pillar (driver door)
]
CAR_LENGTH = 4.0
CAR_WIDTH = 1.9
UNLOCK_POINT = ANCHORS[2]   # left B-pillar (driver door)
UNLOCK_RADIUS = 1.0         # unlock zone radius (matches Geofence::kDoorRadiusM)
SEAT_POINT = (-0.35, 0.0)   # driver seat (slightly inside the door)
SEAT_RADIUS = 0.45

INTENT_NAMES = ("approach", "seated", "leave", "passing")
INTENT_DECISION = {
    "approach": "Allow unlock",
    "seated": "Lock door & start engine",
    "leave": "Lock door",
    "passing": "Deny unlock",
}

EKF_RE = re.compile(
    r"\[EKF\]\s+(?:t=\d+\s+)?x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+"
    r"vx=(-?[\d.]+)\s+vy=(-?[\d.]+)\s+v=(-?[\d.]+)"
)
STATE_RE = re.compile(r"\[STATE\]\s+(LOCKED|DOOR_UNLOCKED|OCCUPIED)")
IGNITION_RE = re.compile(r"\[IGNITION\]\s+(ON|OFF)")
INTENT_RE = re.compile(
    r"\[INTENT\]\s+p_approach=(-?[\d.]+)\s+p_seated=(-?[\d.]+)\s+"
    r"p_leave=(-?[\d.]+)\s+p_passing=(-?[\d.]+)"
)
RANGE_RE = re.compile(
    r"\[RANGE3\]\s+(?:t=\d+\s+)?d0=(-?[\d.]+)\s+d1=(-?[\d.]+)\s+d2=(-?[\d.]+)\s+n=(\d+)\s+mask=([01]{3})"
)


def intent_text(probabilities):
    winner = max(range(len(INTENT_NAMES)), key=probabilities.__getitem__)
    name = INTENT_NAMES[winner]
    return (f"AI: {name} ({probabilities[winner]:.0%})\n"
            f"DECISION: {INTENT_DECISION[name]}")


# -----------------------------------------------------------------------------
# Simulation: no hardware; verifies geometry + solver + Tk UI.
# -----------------------------------------------------------------------------
def _sim_true_pose(k, dt=0.05):
    """A person-sized path around the fixed car body, in metres."""
    t = k * dt
    theta = t * 0.55
    c = math.cos(theta)
    s = math.sin(theta)
    standoff = 0.32 + 0.10 * math.sin(t * 0.37)
    half_w = CAR_WIDTH / 2.0
    half_l = CAR_LENGTH / 2.0

    # Radial intersection with a rectangle, plus a small standoff. This walks
    # around the car instead of around an arbitrary anchor bounding box.
    sx = half_w / max(abs(c), 1e-6)
    sy = half_l / max(abs(s), 1e-6)
    r = min(sx, sy) + standoff
    x = r * c
    y = r * s

    # Once per lap, step toward the seat and back so the Access panel exercises
    # driver-detected / occupied / ignition states too.
    phase = (theta % (2.0 * math.pi)) / (2.0 * math.pi)
    seat = np.array(SEAT_POINT, dtype=float)
    if 0.46 <= phase <= 0.54:
        edge = np.array([-half_w - standoff, 0.0])
        blend = 1.0 - abs(phase - 0.50) / 0.04
        blend = min(max(blend, 0.0), 1.0)
        x, y = (1.0 - blend) * edge + blend * seat

    return float(x), float(y)


def simulate_ranges(k, rng, noise_m=0.03, drop_anchor=-1, drop_rate=0.0):
    """Synthetic UWB ranges from the SmartCarAccess anchor geometry."""
    x, y = _sim_true_pose(k)
    distances = []
    for i, (ax, ay) in enumerate(ANCHORS):
        if i == drop_anchor or (drop_rate > 0.0 and rng.random() < drop_rate):
            distances.append(0.0)
            continue
        measured = math.hypot(x - ax, y - ay) + float(rng.normal(0.0, noise_m))
        distances.append(max(measured, 0.0))
    return (x, y), distances


def solve_position_from_ranges(distances):
    """Simulation-only trilateration solver used to validate the geometry."""
    valid = [
        (np.array(ANCHORS[i], dtype=float), float(d))
        for i, d in enumerate(distances)
        if d is not None and math.isfinite(d) and d > 0.0
    ]
    if len(valid) < 3:
        return None, float("nan")

    p0, r0 = valid[0]
    rows = []
    rhs = []
    for pi, ri in valid[1:]:
        rows.append(2.0 * (pi - p0))
        rhs.append(r0 * r0 - ri * ri + float(pi @ pi) - float(p0 @ p0))
    a = np.vstack(rows)
    b = np.array(rhs, dtype=float)
    try:
        xy, *_ = np.linalg.lstsq(a, b, rcond=None)
    except np.linalg.LinAlgError:
        return None, float("nan")

    # Refine the range fit. This mirrors the spirit of UI-Demo's Python solver
    # without pulling in csmn_localization or changing the live ESP32 path.
    for _ in range(8):
        h_rows = []
        residuals = []
        for pi, ri in valid:
            diff = xy - pi
            pred = float(np.linalg.norm(diff))
            if pred < 1e-9:
                continue
            h_rows.append(diff / pred)
            residuals.append(ri - pred)
        if len(h_rows) < 2:
            break
        h = np.vstack(h_rows)
        r = np.array(residuals, dtype=float)
        try:
            step, *_ = np.linalg.lstsq(h, r, rcond=None)
        except np.linalg.LinAlgError:
            break
        xy = xy + step
        if float(np.linalg.norm(step)) < 1e-5:
            break

    errors = []
    for pi, ri in valid:
        errors.append(float(np.linalg.norm(xy - pi)) - ri)
    residual_rms = math.sqrt(float(np.mean(np.square(errors)))) if errors else float("nan")
    return (float(xy[0]), float(xy[1])), residual_rms


def access_state_for_position(x, y):
    if math.hypot(x - SEAT_POINT[0], y - SEAT_POINT[1]) <= SEAT_RADIUS:
        return "OCCUPIED", True
    if math.hypot(x - UNLOCK_POINT[0], y - UNLOCK_POINT[1]) <= UNLOCK_RADIUS:
        return "DOOR_UNLOCKED", False
    return "LOCKED", False


def run_simulation(args):
    """Run synthetic ranges through the local solver and Tk display."""
    try:
        from car_position_display import PositionFix, create_position_window
    except Exception as e:  # pragma: no cover
        sys.exit(f"Simulation UI needs Tkinter: {e}")

    total = max(int(args.simulate), 1)
    rng = np.random.default_rng(0)
    win = create_position_window("UWB Localization (simulation)")
    stats = {"k": 0, "prev": None, "errors": [], "residuals": []}
    dt = 0.05

    print(f"SIMULATION MODE - no serial ports opened, {total} synthetic frames.")
    print("Close the window to exit after the simulation finishes.")

    def step():
        if win.closed:
            return
        k = stats["k"]
        true_xy, distances = simulate_ranges(
            k, rng, noise_m=args.sim_noise,
            drop_anchor=args.drop_anchor, drop_rate=args.sim_drop_rate)
        solved_xy, residual = solve_position_from_ranges(distances)

        win.update_ranges(distances)
        if solved_xy is not None:
            if stats["prev"] is None:
                vx = vy = 0.0
            else:
                _pt, px, py = stats["prev"]
                vx = (solved_xy[0] - px) / dt
                vy = (solved_xy[1] - py) / dt
            stats["prev"] = (k * dt, solved_xy[0], solved_xy[1])
            stats["errors"].append(math.hypot(
                solved_xy[0] - true_xy[0], solved_xy[1] - true_xy[1]))
            if math.isfinite(residual):
                stats["residuals"].append(residual)
            win.update_position(PositionFix(
                x=solved_xy[0], y=solved_xy[1], vx=vx, vy=vy, valid=True,
                residual_m=residual, timestamp=time.monotonic()))

            door, ignition = access_state_for_position(solved_xy[0], solved_xy[1])
            win.update_access(door, ignition)

        stats["k"] += 1
        if stats["k"] < total:
            win.after(int(dt * 1000), step)
        else:
            errors = stats["errors"]
            residuals = stats["residuals"]
            if errors:
                print(
                    "simulation: mean position error %.3f m over %d fixes "
                    "(%.0f mm range noise injected)"
                    % (float(np.mean(errors)), len(errors), args.sim_noise * 1000.0)
                )
            if residuals:
                print("simulation: mean range residual %.3f m" %
                      float(np.mean(residuals)))
            print("Simulation complete. Close the window to exit.")

    win.after(50, step)
    try:
        win.run()
    except KeyboardInterrupt:
        pass
    finally:
        print("Stopped.")


# -----------------------------------------------------------------------------
# Anchor bridge (mirrors run_fira_bridge.py)
# -----------------------------------------------------------------------------
class AnchorState:
    def __init__(self):
        self._lock = threading.Lock()
        self._distance = None   # metres; meaningful only when status == "Ok"
        self._status = None     # last Status name (str) or None
        self._nlos = None       # bool or None
        self._fom = None        # AoA azimuth FOM (%) or None
        self._rssi = None       # dBm or None
        self._slot_err = None   # error slot number or None
        self._seq = -1          # ranging-round sequence counter (RangingData.idx)
        self._ts = 0.0

    def update(self, distance_m, status, nlos, fom, rssi, slot_err, seq):
        with self._lock:
            self._distance = distance_m
            self._status = status
            self._nlos = nlos
            self._fom = fom
            self._rssi = rssi
            self._slot_err = slot_err
            self._seq = seq
            self._ts = time.monotonic()

    def snapshot(self):
        with self._lock:
            return (self._distance, self._ts, self._status,
                    self._nlos, self._fom, self._rssi, self._slot_err,
                    self._seq)

    def reset(self):
        with self._lock:
            self._distance = None
            self._status = None
            self._nlos = None
            self._fom = None
            self._rssi = None
            self._slot_err = None
            self._seq = -1
            self._ts = 0.0


def make_range_handler(state):
    def handler(payload):
        try:
            rd = RangingData(payload)
        except Exception:
            return
        if not rd.meas:
            return
        meas = rd.meas[0]
        ok = meas.status == Status.Ok
        status_name = getattr(meas.status, "name", None) or str(meas.status)
        state.update(
            distance_m=meas.distance / 100.0 if ok else None,
            status=status_name,
            nlos=bool(meas.nlos) if meas.nlos is not None else None,
            fom=meas.aoa_tetha_fom,
            rssi=meas.rssi,
            slot_err=meas.slot_in_error,
            seq=rd.idx,
        )
    return handler


def make_session_status_handler(idx, restart_queue):
    """Detect the anchor leaving responder mode.

    When the Android phone stops ranging (app backgrounded/closed or the
    session drops), it sends an in-band termination signal that makes the
    responder exit responder mode. The PC host otherwise never notices, so the
    anchor goes dead until power-cycled. This handler queues a restart request
    that the bridge picks up and re-runs session init + ranging_start.
    """
    def handler(payload):
        try:
            st = SessionStatus(payload)
        except Exception:
            return
        if (st.state == SessionState.Idle and
                st.reason == SessionStateChangeReason.SessionStoppedDueToInbandSignal):
            restart_queue.put(idx)
    return handler


def start_anchor(client, mac, dest_mac, args):
    rts, session_handle = client.session_init(args.session, SessionType.Ranging)
    if rts != Status.Ok:
        raise RuntimeError(f"session_init failed: {rts.name} ({rts})")
    session = session_handle if session_handle is not None else args.session

    app_configs = [
        (App.DeviceType, 0), (App.DeviceRole, 0), (App.MultiNodeMode, 1),
        (App.RangingRoundUsage, 2), (App.DeviceMacAddress, mac),
        (App.ChannelNumber, args.channel), (App.ScheduleMode, 1),
        (App.StsConfig, 0), (App.RframeConfig, 3),
        (App.ResultReportConfig, 11), (App.VendorId, 0x0708),
        (App.StaticStsIv, 0x060504030201), (App.AoaResultReq, 1),
        (App.UwbInitiationTime, 0), (App.PreambleCodeIndex, args.preamble_idx),
        (App.SfdId, 2), (App.SlotDuration, 2400), (App.RangingInterval, 200),
        (App.SlotsPerRr, 25), (App.MaxNumberOfMeasurements, 0),
        (App.HoppingMode, 0), (App.RssiReporting, 1),
        (App.BlockStrideLength, 0), (App.NumberOfControlees, 1),
        (App.DstMacAddress, [dest_mac]), (App.StsLength, 1),
    ]
    rts, rtv = client.session_set_app_config(session, app_configs)
    if rts != Status.Ok:
        raise RuntimeError(f"session_set_app_config failed: {rts.name}\n{rtv}")
    rts = client.ranging_start(session)
    if rts != Status.Ok:
        raise RuntimeError(f"ranging_start failed: {rts.name} ({rts})")
    return session


def stop_anchor(client, session):
    try:
        client.ranging_stop(session)
    except Exception:
        pass
    try:
        client.session_deinit(session)
    except Exception:
        pass


class EspLink:
    def __init__(self, port, baud):
        self._ser = serial.Serial()
        self._ser.port = port
        self._ser.baudrate = baud
        self._ser.timeout = 0.1
        self._ser.dtr = False
        self._ser.rts = False
        self._ser.open()
        self._wlock = threading.Lock()

    def send(self, line):
        data = (line + "\n").encode("utf-8")
        with self._wlock:
            self._ser.write(data)

    def readline(self):
        raw = self._ser.readline()
        if not raw:
            return None
        return raw.decode("utf-8", errors="ignore").strip()

    def close(self):
        try:
            self._ser.close()
        except Exception:
            pass


class DemoBridge:
    """Starts/stops the anchors and forwards distances to the ESP32."""

    def __init__(self, clients, macs, states, dest_mac, esp, args, viz_queue,
                 capture=None, restart_queue=None):
        self._clients = clients
        self._macs = macs
        self._states = states
        self._dest_mac = dest_mac
        self._esp = esp
        self._args = args
        self._viz = viz_queue
        self._capture = capture
        self._restart_queue = restart_queue
        self._sessions = [None] * len(clients)
        self._ranging = False
        self._lock = threading.Lock()
        self._stop = threading.Event()

    def start_ranging(self):
        with self._lock:
            if self._ranging:
                return
            for i, client in enumerate(self._clients):
                try:
                    self._sessions[i] = start_anchor(
                        client, self._macs[i], self._dest_mac, self._args)
                    print(f"[{self._args.ports[i]}] ranging started "
                          f"(mac={self._macs[i]:#x})")
                except Exception as e:
                    self._sessions[i] = None
                    print(f"[{self._args.ports[i]}] START error: {e}")
            self._ranging = True
        self._esp.send("ACK:START_RANGING")

    def stop_ranging(self):
        with self._lock:
            for i, client in enumerate(self._clients):
                if self._sessions[i] is not None:
                    stop_anchor(client, self._sessions[i])
                    self._sessions[i] = None
            for s in self._states:
                s.reset()
            self._ranging = False
        self._esp.send("ACK:STOP_RANGING")

    def _is_ranging(self):
        with self._lock:
            return self._ranging

    def _restart_anchor(self, i):
        """Re-run session init + config + ranging_start for one anchor after it
        dropped out of responder mode (in-band stop from the phone)."""
        if not (0 <= i < len(self._clients)):
            return
        with self._lock:
            client = self._clients[i]
            if self._sessions[i] is not None:
                stop_anchor(client, self._sessions[i])
                self._sessions[i] = None
            try:
                self._sessions[i] = start_anchor(
                    client, self._macs[i], self._dest_mac, self._args)
                print(f"[{self._args.ports[i]}] session restarted "
                      f"(in-band stop recovery)")
            except Exception as e:
                self._sessions[i] = None
                print(f"[{self._args.ports[i]}] RESTART error: {e}")

    def command_loop(self):
        """Read ESP32 lines: handle CMD and forward [POS2D]/[EKF] to the UI."""
        while not self._stop.is_set():
            line = self._esp.readline()
            if not line:
                continue
            if self._args.esp_debug:
                print(f"[ESP] {line}")
            if self._capture is not None:
                self._capture.write(line + "\n")
                self._capture.flush()
            if line == "CMD:START_RANGING":
                print("[ESP] CMD:START_RANGING")
                self.start_ranging()
                continue
            if line == "CMD:STOP_RANGING":
                print("[ESP] CMD:STOP_RANGING")
                self.stop_ranging()
                continue
            self._ingest(line)

    def _ingest(self, line):
        m = STATE_RE.search(line)
        if m:
            self._viz.put(("state", m.group(1)))
            return
        m = IGNITION_RE.search(line)
        if m:
            self._viz.put(("ignition", m.group(1) == "ON"))
            return
        m = INTENT_RE.search(line)
        if m:
            self._viz.put(("intent", float(m.group(1)), float(m.group(2)),
                           float(m.group(3)), float(m.group(4))))
            return
        m = RANGE_RE.search(line)
        if m:
            self._viz.put(("range", float(m.group(1)), float(m.group(2)),
                           float(m.group(3)), int(m.group(4))))
            return
        m = EKF_RE.search(line)
        if m:
            self._viz.put(("ekf", float(m.group(1)), float(m.group(2)),
                           float(m.group(3)), float(m.group(4))))
            return

    def forward_loop(self):
        period = 1.0 / self._args.rate_hz
        fresh_s = self._args.fresh_ms / 1000.0
        dmin, dmax = self._args.dmin, self._args.dmax
        if self._args.autostart:
            self.start_ranging()
        while not self._stop.is_set():
            time.sleep(period)
            if not self._is_ranging():
                continue
            # Recover anchors that dropped out of responder mode (the phone
            # sent an in-band stop) so they keep ranging without a power-cycle.
            if self._restart_queue is not None:
                while True:
                    try:
                        idx = self._restart_queue.get_nowait()
                    except queue.Empty:
                        break
                    self._restart_anchor(idx)
            now = time.monotonic()
            dists = [0.0, 0.0, 0.0]
            n_usable = 0
            for i, s in enumerate(self._states):
                if i == self._args.drop_anchor:
                    continue  # simulated lost anchor: distance stays 0
                d, ts, *_ = s.snapshot()
                fresh = d is not None and (now - ts) <= fresh_s
                in_bounds = fresh and (dmin <= d <= dmax)
                if in_bounds and i < 3:
                    dists[i] = d
                    n_usable += 1
            if self._args.esp_debug:
                parts = []
                for i, s in enumerate(self._states):
                    if i == self._args.drop_anchor:
                        parts.append(f"d{i}=SIM")
                        continue
                    d, ts, status, nlos, fom, rssi, slot_err, seq = s.snapshot()
                    if status is None:
                        parts.append(f"d{i}=no-ntf")
                        continue
                    age_ms = (now - ts) * 1000.0
                    bits = [f"d{i}"]
                    if d is not None:
                        bits.append(f"d={d:.2f}")
                    bits.append(f"seq={seq}")
                    bits.append(f"age={age_ms:.0f}ms")
                    bits.append(status)
                    if nlos:
                        bits.append("nlos")
                    if fom is not None:
                        bits.append(f"fom={fom:.0f}")
                    if rssi is not None:
                        bits.append(f"rssi={rssi:.0f}")
                    if slot_err is not None:
                        bits.append(f"slot={slot_err}")
                    parts.append(" ".join(bits))
                line = "[DIAG] " + " | ".join(parts)
                print(line)
                if self._capture is not None:
                    self._capture.write(line + "\n")
                    self._capture.flush()
            # informational only: the ESP32 derives the per-anchor mask from
            # which distances are > 0, so stale anchors are zeroed above.
            valid = 1 if n_usable >= 2 else 0
            self._esp.send(
                f"RANGE:d0={dists[0]:.3f},d1={dists[1]:.3f},"
                f"d2={dists[2]:.3f},valid={valid}")

    def shutdown(self):
        self._stop.set()
        self.stop_ranging()

    def close(self):
        self.shutdown()
        for c in self._clients:
            c.close()
        self._esp.close()


# -----------------------------------------------------------------------------
# Visualization (Tkinter top-down map from car_position_display.py)
# -----------------------------------------------------------------------------
def run_hmi(args, viz, bridge, capture_fh):
    """Drive the Tkinter top-down localization map from the viz queue.

    The bridge threads keep feeding `viz`; a Tk after() timer drains it at
    roughly 30 fps and forwards only display data to the window.
    """
    try:
        from car_position_display import PositionFix, create_position_window
    except Exception as e:  # pragma: no cover
        sys.exit(f"HMI mode needs Tkinter: {e}")

    win = create_position_window()

    import tkinter as tk
    intent_var = tk.StringVar(value="")
    intent_label = tk.Label(
        win.view, textvariable=intent_var, justify="left", anchor="nw",
        background="#ffffff", foreground="#273671",
        font=("Segoe UI Semibold", 11), padx=7, pady=5)
    intent_label.place(relx=0.5, x=0, y=12, anchor="n")

    dists = [0.0, 0.0, 0.0]

    def drain():
        while True:
            try:
                ev = viz.get_nowait()
            except queue.Empty:
                break
            if ev[0] == "range":
                _, d0, d1, d2, _valid = ev
                dists[0], dists[1], dists[2] = d0, d1, d2
                win.update_ranges(dists)
            elif ev[0] == "state":
                win.update_access(state=ev[1])
            elif ev[0] == "ignition":
                win.update_access(ignition=ev[1])
            elif ev[0] == "intent":
                intent_var.set(intent_text(ev[1:]))
            elif ev[0] == "ekf":
                _, x, y, vx, vy = ev
                win.update_position(PositionFix(
                    x=x, y=y, vx=vx, vy=vy, valid=True,
                    timestamp=time.monotonic()))
        if not win.closed:
            win.after(33, drain)

    try:
        win.after(33, drain)
        win.run()
    except KeyboardInterrupt:
        pass
    finally:
        if bridge:
            bridge.close()
        if capture_fh is not None:
            try:
                capture_fh.close()
            except Exception:
                pass
        print("Stopped.")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--ports", nargs="+", default=None,
                    help="anchor COM ports, e.g. COM11 COM19 COM12")
    ap.add_argument("--macs", nargs="+", default=["0", "1", "2"],
                    help="anchor MAC addresses (default: 0 1 2)")
    ap.add_argument("--dest-mac", default="0x06C1")
    ap.add_argument("-s", "--session", type=int, default=42)
    ap.add_argument("-c", "--channel", type=int, default=9)
    ap.add_argument("--preamble-idx", type=int, default=9)
    ap.add_argument("--esp-port", default=None)
    ap.add_argument("--esp-baud", type=int, default=115200)
    ap.add_argument("--rate-hz", type=float, default=10.0)
    ap.add_argument("--fresh-ms", type=int, default=500)
    ap.add_argument("--dmin", type=float, default=0.1)
    ap.add_argument("--dmax", type=float, default=30.0)
    ap.add_argument("--autostart", action="store_true")
    ap.add_argument("--drop-anchor", type=int, default=-1,
                    help="simulate a lost anchor: force its distance to 0 "
                         "(-1 disables)")
    ap.add_argument("--simulate", type=int, default=0,
                    help="run N synthetic frames with no hardware; verifies "
                         "SmartCarAccess geometry, the simulation solver, and "
                         "the Tk localization UI")
    ap.add_argument("--sim-noise", type=float, default=0.03,
                    help="standard deviation of synthetic range noise in metres "
                         "(default 0.03)")
    ap.add_argument("--sim-drop-rate", type=float, default=0.0,
                    help="per-anchor random dropout probability for simulation "
                         "frames (default 0.0)")
    ap.add_argument("--esp-debug", action="store_true",
                    help="echo every ESP32 line to the console (like run_fira_bridge.py)")
    ap.add_argument("--log", default=None, help="replay a captured log file")
    ap.add_argument("--capture", default=None,
                    help="tee raw ESP32 lines to this file (for collect_traj.py)")
    args = ap.parse_args()

    if args.simulate:
        run_simulation(args)
        return

    if args.log is None and (not args.ports or not args.esp_port):
        ap.error("provide --log (replay) or -p PORTS --esp-port (live)")

    viz = queue.Queue()
    bridge = None
    bridge_threads = []
    capture_fh = None

    if args.log:
        # Replay: parse the log in a background thread at a gentle pace.
        def replay():
            with open(args.log, "r", encoding="utf-8", errors="ignore") as fh:
                for line in fh:
                    m = STATE_RE.search(line)
                    if m:
                        viz.put(("state", m.group(1)))
                        continue
                    m = IGNITION_RE.search(line)
                    if m:
                        viz.put(("ignition", m.group(1) == "ON"))
                        continue
                    m = INTENT_RE.search(line)
                    if m:
                        viz.put(("intent", float(m.group(1)), float(m.group(2)),
                                 float(m.group(3)), float(m.group(4))))
                        continue
                    m = RANGE_RE.search(line)
                    if m:
                        viz.put(("range", float(m.group(1)), float(m.group(2)),
                                 float(m.group(3)), int(m.group(4))))
                        continue
                    m = EKF_RE.search(line)
                    if m:
                        viz.put(("ekf", float(m.group(1)), float(m.group(2)),
                                 float(m.group(3)), float(m.group(4))))
                        time.sleep(0.05)
                        continue
        threading.Thread(target=replay, daemon=True).start()
    else:
        if not UCI_AVAILABLE:
            sys.exit("uci library not importable — set PYTHONPATH to "
                     "lib/uwb-uci;lib/uqt-utils first.")
        if serial is None:
            sys.exit("pyserial not installed — run: pip install pyserial")

        macs = [int(m, 0) for m in args.macs]
        dest_mac = int(args.dest_mac, 0)
        states = [AnchorState() for _ in args.ports]
        restart_queue = queue.Queue()
        clients = []
        for i, port in enumerate(args.ports):
            c = Client(port=port)
            c.notif_handlers = {
                (Gid.Ranging, OidRanging.Start): make_range_handler(states[i]),
                (Gid.Session, OidSession.Status): make_session_status_handler(
                    i, restart_queue),
                ("default", "default"): lambda gid, oid, x: None,
            }
            clients.append(c)

        try:
            esp = EspLink(args.esp_port, args.esp_baud)
        except Exception as e:
            print(f"Cannot open ESP32 port {args.esp_port}: {e}")
            for c in clients:
                c.close()
            sys.exit(1)

        capture_fh = None
        if args.capture:
            capture_path = os.path.abspath(args.capture)
            capture_fh = open(capture_path, "a", encoding="utf-8")
            print(f"Capturing raw ESP32 lines -> {capture_path}")

        bridge = DemoBridge(clients, macs, states, dest_mac, esp, args, viz,
                            capture=capture_fh, restart_queue=restart_queue)
        bridge_threads = [
            threading.Thread(target=bridge.command_loop, daemon=True),
            threading.Thread(target=bridge.forward_loop, daemon=True),
        ]
        for t in bridge_threads:
            t.start()
        print(f"Anchors: {', '.join(args.ports)}   ESP32: {args.esp_port}")

    run_hmi(args, viz, bridge, capture_fh)


if __name__ == "__main__":
    main()
