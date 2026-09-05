#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""localization_demo.py — self-contained UWB localization demo.

Starts the 3 UWB anchors as FiRa responders, forwards their distances to the
ESP32-S3 over USB-CDC (exactly like run_fira_bridge.py), and live-visualizes the
EKF position of a person walking around the car on a clean 2D localization map.

Layout (portrait):
  * top ~70% — top-down map: car (4 m x 2 m, front up), 3 green UWB anchors
    (with ranging rings), a dashed localization boundary, the raw trilateration
    fixes (dim), the EKF estimate (blue) with its trajectory and a translucent
    direction cone.
  * bottom ~30% — 2x2 metrics panel (live x / y / speed / heading).

Usage (set PYTHONPATH to the uci + uqt-utils libs first, as for run_fira_bridge.py):

    $env:PYTHONPATH = "<repo>/lib/uwb-uci;<repo>/lib/uqt-utils"
    python localization_demo.py -p COM11 COM19 COM12 --macs 0 1 2 --esp-port COM5

Replay a previously captured log instead of running live:

    python localization_demo.py --log capture.log

Simulate a lost anchor (default: right-side anchor, index 1) without touching
hardware — its distance is forced to 0 so the ESP32 falls back to its 2-anchor
trilateration path:

    python localization_demo.py ... --drop-anchor 1   # -1 disables

Parsed ESP32 lines (same format as analyze_ekf.py):
    [POS2D] t=<ms> x=.. y=.. rms=..
    [EKF]   t=<ms> x=.. y=.. vx=.. vy=.. v=..
"""

import argparse
import collections
import math
import queue
import re
import sys
import threading
import time

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.patches import Rectangle, Circle, Wedge

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
CAR_WIDTH = 2.0
UNLOCK_POINT = ANCHORS[2]   # left B-pillar (driver door)
UNLOCK_RADIUS = 2.0
SEAT_POINT = (-0.35, 0.0)   # driver seat (slightly inside the door)
SEAT_RADIUS = 0.45

RAW_KEEP = 50      # dim raw dots kept on screen
EKF_KEEP = 120     # blue EKF trail length
CONE_HALF_ANGLE = 28.0   # degrees, half-width of the direction cone
CONE_LENGTH = 0.9        # metres, direction cone length
MIN_SPEED_FOR_CONE = 0.05  # m/s, below this the cone is hidden

METRIC_LABELS = [["x (m)", "y (m)"], ["speed (m/s)", "heading (\u00b0)"]]

POS_RE = re.compile(
    r"\[POS2D\]\s+(?:t=\d+\s+)?x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+rms=(-?[\d.]+)"
)
EKF_RE = re.compile(
    r"\[EKF\]\s+(?:t=\d+\s+)?x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+"
    r"vx=(-?[\d.]+)\s+vy=(-?[\d.]+)\s+v=(-?[\d.]+)"
)
STATE_RE = re.compile(r"\[STATE\]\s+(LOCKED|DOOR_UNLOCKED|OCCUPIED)")
IGNITION_RE = re.compile(r"\[IGNITION\]\s+(ON|OFF)")


# -----------------------------------------------------------------------------
# Anchor bridge (mirrors run_fira_bridge.py)
# -----------------------------------------------------------------------------
class AnchorState:
    def __init__(self):
        self._lock = threading.Lock()
        self._distance = None
        self._ts = 0.0

    def update(self, distance_m, _seq):
        with self._lock:
            self._distance = distance_m
            self._ts = time.monotonic()

    def snapshot(self):
        with self._lock:
            return self._distance, self._ts

    def reset(self):
        with self._lock:
            self._distance = None
            self._ts = 0.0


def make_range_handler(state):
    def handler(payload):
        try:
            rd = RangingData(payload)
        except Exception:
            return
        for meas in rd.meas:
            if meas.status == Status.Ok:
                state.update(meas.distance / 100.0, rd.idx)
                break
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
        (App.HoppingMode, 0), (App.RssiReporting, 0),
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
                 capture=None):
        self._clients = clients
        self._macs = macs
        self._states = states
        self._dest_mac = dest_mac
        self._esp = esp
        self._args = args
        self._viz = viz_queue
        self._capture = capture
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
        m = EKF_RE.search(line)
        if m:
            self._viz.put(("ekf", float(m.group(1)), float(m.group(2)),
                           float(m.group(3)), float(m.group(4))))
            return
        m = POS_RE.search(line)
        if m:
            self._viz.put(("raw", float(m.group(1)), float(m.group(2))))

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
            now = time.monotonic()
            dists = [0.0, 0.0, 0.0]
            all_valid = len(self._states) == 3
            for i, s in enumerate(self._states):
                if i == self._args.drop_anchor:
                    continue  # simulated lost anchor: keep d=0, skip validity
                d, ts = s.snapshot()
                fresh = d is not None and (now - ts) <= fresh_s
                in_bounds = fresh and (dmin <= d <= dmax)
                if not in_bounds:
                    all_valid = False
                if i < 3:
                    dists[i] = d if d is not None else 0.0
            valid = 1 if all_valid else 0
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
# Visualization
# -----------------------------------------------------------------------------
def build_figure(drop_anchor=-1):
    fig = plt.figure(figsize=(7.2, 10), facecolor="#faf7f0")
    gs = fig.add_gridspec(2, 1, height_ratios=[7, 3], hspace=0.12,
                          left=0.06, right=0.94, top=0.98, bottom=0.03)

    ax = fig.add_subplot(gs[0])
    ax.set_facecolor("#f6efdd")
    ax.set_aspect("equal")
    ax.set_xlim(-4.2, 4.2)
    ax.set_ylim(-4.2, 4.2)
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_axis_off()

    # Dashed localization boundary (orange).
    ax.add_patch(Rectangle((-3.6, -3.6), 7.2, 7.2, fill=False,
                           ec="#e8a33d", ls="--", lw=1.6))

    # Car: 4 m x 2 m, front pointing up.
    ax.add_patch(Rectangle((-CAR_WIDTH / 2, -CAR_LENGTH / 2),
                           CAR_WIDTH, CAR_LENGTH,
                           facecolor="#e7dcc3", edgecolor="#4a4a4a", lw=2.2))
    ax.plot([-CAR_WIDTH / 2, CAR_WIDTH / 2],
            [CAR_LENGTH / 2 - 0.45, CAR_LENGTH / 2 - 0.45],
            color="#4a4a4a", lw=1.6)

    # Anchors: green circle + outline + concentric ranging rings.
    for i, (axx, ayy) in enumerate(ANCHORS):
        ax.add_patch(Circle((axx, ayy), 0.30, fill=False,
                            ec="#2e7d32", lw=1.0, alpha=0.55))
        ax.add_patch(Circle((axx, ayy), 0.18, fill=False,
                            ec="#2e7d32", lw=1.2, alpha=0.8))
        ax.plot(axx, ayy, marker="o", ms=9, color="#43a047",
                mec="#1b5e20", mew=1.5, zorder=6)

    # Mark a simulated dropped anchor with a red X.
    if drop_anchor is not None and 0 <= drop_anchor < len(ANCHORS):
        axx, ayy = ANCHORS[drop_anchor]
        ax.plot([axx - 0.24, axx + 0.24], [ayy - 0.24, ayy + 0.24],
                color="#d32f2f", lw=2.2, zorder=7)
        ax.plot([axx - 0.24, axx + 0.24], [ayy + 0.24, ayy - 0.24],
                color="#d32f2f", lw=2.2, zorder=7)

    # Unlock zone around the driver door.
    ax.add_patch(Circle(UNLOCK_POINT, UNLOCK_RADIUS, fill=False,
                        ec="#27ae60", ls=":", lw=1.1, alpha=0.7))

    # Driver seat point + zone (target for the "seated" state).
    ax.add_patch(Circle(SEAT_POINT, SEAT_RADIUS, fill=True,
                        facecolor="#ffb74d", alpha=0.18, ec="#e65100",
                        ls="--", lw=1.0))
    ax.plot(SEAT_POINT[0], SEAT_POINT[1], marker="s", ms=8,
            color="#ef6c00", mec="#bf360c", mew=1.2, zorder=6)

    # Dynamic artists.
    raw_sc = ax.scatter([], [], s=14, color="#9aa3ad", alpha=0.30, zorder=3)
    ekf_line, = ax.plot([], [], color="#1e88e5", lw=1.8, zorder=4)
    ekf_pt, = ax.plot([], [], marker="o", ms=8, color="#1565c0",
                      mfc="#42a5f5", mec="#0d47a1", mew=1.2, zorder=5)
    cone = Wedge((0, 0), CONE_LENGTH, 0, 360, color="#1e88e5", alpha=0.0,
                 zorder=3)
    ax.add_patch(cone)

    # Access-state status banner (top-left of the map).
    status_text = ax.text(-4.0, 3.9, "", fontsize=11, weight="bold",
                          va="top", ha="left", color="#273671",
                          family="monospace")

    # Legend at bottom edge of the map.
    ax.plot(0.5, -3.95, marker="o", ms=7, color="#43a047", mec="#1b5e20",
            mew=1.2)
    ax.text(0.62, -3.95, "uwb anchor", fontsize=8, va="center")
    ax.plot(1.6, -3.95, marker="o", ms=7, color="#42a5f5", mec="#0d47a1",
            mew=1.2)
    ax.text(1.72, -3.95, "estimated position", fontsize=8, va="center")
    ax.plot(3.0, -3.95, marker="o", ms=6, color="#9aa3ad", alpha=0.5)
    ax.text(3.12, -3.95, "raw", fontsize=8, va="center")

    # Metrics panel: 2x2.
    gsm = gs[1].subgridspec(2, 2, hspace=0.5, wspace=0.15)
    metric_axes = [[fig.add_subplot(gsm[i, j]) for j in range(2)]
                   for i in range(2)]
    for i in range(2):
        for j in range(2):
            a = metric_axes[i][j]
            a.set_axis_off()
            a.set_xlim(0, 1)
            a.set_ylim(0, 1)
            a.text(0.5, 0.62, "--", ha="center", va="center",
                   fontsize=26, weight="bold", color="#273671")
            a.text(0.5, 0.22, METRIC_LABELS[i][j], ha="center", va="center",
                   fontsize=11, color="#8a8f98")

    return fig, ax, raw_sc, ekf_line, ekf_pt, cone, metric_axes, status_text


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
    ap.add_argument("--drop-anchor", type=int, default=1,
                    help="simulate a lost anchor: force its distance to 0 "
                         "(default 1 = right side; -1 to disable)")
    ap.add_argument("--esp-debug", action="store_true",
                    help="echo every ESP32 line to the console (like run_fira_bridge.py)")
    ap.add_argument("--log", default=None, help="replay a captured log file")
    ap.add_argument("--capture", default=None,
                    help="tee raw ESP32 lines to this file (for collect_traj.py)")
    args = ap.parse_args()

    if args.log is None and (not args.ports or not args.esp_port):
        ap.error("provide --log (replay) or -p PORTS --esp-port (live)")

    fig, ax, raw_sc, ekf_line, ekf_pt, cone, metric_axes, status_text = build_figure(args.drop_anchor)

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
                    m = EKF_RE.search(line)
                    if m:
                        viz.put(("ekf", float(m.group(1)), float(m.group(2)),
                                 float(m.group(3)), float(m.group(4))))
                        time.sleep(0.05)
                        continue
                    m = POS_RE.search(line)
                    if m:
                        viz.put(("raw", float(m.group(1)), float(m.group(2))))
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
        clients = []
        for i, port in enumerate(args.ports):
            c = Client(port=port)
            c.notif_handlers = {
                (Gid.Ranging, OidRanging.Start): make_range_handler(states[i]),
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
            capture_fh = open(args.capture, "a", encoding="utf-8")
            print(f"Capturing raw ESP32 lines -> {args.capture}")

        bridge = DemoBridge(clients, macs, states, dest_mac, esp, args, viz,
                            capture=capture_fh)
        bridge_threads = [
            threading.Thread(target=bridge.command_loop, daemon=True),
            threading.Thread(target=bridge.forward_loop, daemon=True),
        ]
        for t in bridge_threads:
            t.start()
        print(f"Anchors: {', '.join(args.ports)}   ESP32: {args.esp_port}")

    raw_hist = collections.deque(maxlen=RAW_KEEP)
    ekf_hist = collections.deque(maxlen=EKF_KEEP)
    state = {"x": None, "y": None, "vx": 0.0, "vy": 0.0, "n": 0}
    access = {"door": "LOCKED", "ignition": False}

    def update(_frame):
        while True:
            try:
                ev = viz.get_nowait()
            except queue.Empty:
                break
            if ev[0] == "raw":
                raw_hist.append((ev[1], ev[2]))
            elif ev[0] == "state":
                access["door"] = ev[1]
            elif ev[0] == "ignition":
                access["ignition"] = ev[1]
            else:
                _, x, y, vx, vy = ev
                state["x"], state["y"] = x, y
                state["vx"], state["vy"] = vx, vy
                state["n"] += 1
                ekf_hist.append((x, y))

        if raw_hist:
            raw_sc.set_offsets(np.array(raw_hist))
        if ekf_hist:
            eh = np.array(ekf_hist)
            ekf_line.set_data(eh[:, 0], eh[:, 1])
            ekf_pt.set_data([eh[-1, 0]], [eh[-1, 1]])

        # Direction cone from EKF velocity.
        speed = math.hypot(state["vx"], state["vy"])
        if state["x"] is not None and speed >= MIN_SPEED_FOR_CONE:
            heading = math.degrees(math.atan2(state["vy"], state["vx"]))
            cone.set_center((state["x"], state["y"]))
            cone.set_theta1(heading - CONE_HALF_ANGLE)
            cone.set_theta2(heading + CONE_HALF_ANGLE)
            cone.set_alpha(0.18)
        else:
            cone.set_alpha(0.0)

        # Metrics.
        if state["x"] is not None:
            heading = math.degrees(math.atan2(state["vy"], state["vx"])) % 360
            values = [
                f"{state['x']:+.2f}",
                f"{state['y']:+.2f}",
                f"{speed:.2f}",
                f"{heading:.0f}",
            ]
        else:
            values = ["--", "--", "--", "--"]
        for i in range(2):
            for j in range(2):
                metric_axes[i][j].texts[0].set_text(values[i * 2 + j])
                metric_axes[i][j].texts[1].set_text(METRIC_LABELS[i][j])

        # Access-state banner.
        door = access["door"].replace("_", " ")
        ign = "AUTHORIZED" if access["ignition"] else "OFF"
        banner = f"DOOR: {door}\nIGNITION: {ign}"
        if 0 <= args.drop_anchor < len(ANCHORS):
            banner += f"\nDROPPED: anchor {args.drop_anchor} (sim)"
        status_text.set_text(banner)

        return [raw_sc, ekf_line, ekf_pt, cone, status_text]

    anim = FuncAnimation(fig, update, interval=50, blit=False,
                         cache_frame_data=False)

    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        if bridge:
            bridge.close()
        if args.capture and capture_fh is not None:
            try:
                capture_fh.close()
            except Exception:
                pass
        print("Stopped.")


if __name__ == "__main__":
    main()
