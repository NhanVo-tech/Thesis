#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

"""PC UART bridge between N UWB anchors and the ESP32-S3.

Derived from run_fira_multianchor.py. Differences:
  * Trilateration is REMOVED from the PC side (the ESP32 computes position).
  * Raw distances are forwarded to the ESP32 over one serial port:
        RANGE:d0=<float>,d1=<float>,d2=<float>,valid=<0|1>\\n
    valid=1 only when all anchors are fresh and in-bounds.
  * Ranging is started/stopped on command from the ESP32:
        CMD:START_RANGING  -> start sessions on all anchors, reply ACK:START_RANGING
        CMD:STOP_RANGING   -> stop  sessions on all anchors, reply ACK:STOP_RANGING
  * Anchor rx runs in each Client's own thread; a reader thread handles ESP32
    commands; the main thread forwards RANGE frames.
"""

import argparse
import logging
import sys
import threading
import time

import serial

from uci import *


# =============================================================================
# Per-anchor state (thread-safe)
# =============================================================================


class AnchorState:
    def __init__(self):
        self._lock = threading.Lock()
        self._distance = None   # metres
        self._seq = -1
        self._ts = 0.0          # time.monotonic() of last update

    def update(self, distance_m, seq):
        with self._lock:
            self._distance = distance_m
            self._seq = seq
            self._ts = time.monotonic()

    def snapshot(self):
        with self._lock:
            return self._distance, self._ts

    def reset(self):
        with self._lock:
            self._distance = None
            self._seq = -1
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


# =============================================================================
# Anchor session lifecycle (responder / controlee)
# =============================================================================


def start_anchor(client, mac, dest_mac, args):
    rts, session_handle = client.session_init(args.session, SessionType.Ranging)
    if rts != Status.Ok:
        raise RuntimeError(f"session_init failed: {rts.name} ({rts})")

    session = session_handle if session_handle is not None else args.session

    app_configs = [
        (App.DeviceType, 0),            # controlee
        (App.DeviceRole, 0),            # responder
        (App.MultiNodeMode, 1),         # onetomany
        (App.RangingRoundUsage, 2),     # ds-deferred
        (App.DeviceMacAddress, mac),
        (App.ChannelNumber, args.channel),
        (App.ScheduleMode, 1),          # time
        (App.StsConfig, 0),             # static
        (App.RframeConfig, 3),          # sp3
        (App.ResultReportConfig, 11),   # tof|azimuth|fom
        (App.VendorId, 0x0708),
        (App.StaticStsIv, 0x060504030201),
        (App.AoaResultReq, 1),          # all-enabled
        (App.UwbInitiationTime, 0),
        (App.PreambleCodeIndex, args.preamble_idx),
        (App.SfdId, 2),
        (App.SlotDuration, 2400),
        (App.RangingInterval, 200),
        (App.SlotsPerRr, 25),
        (App.MaxNumberOfMeasurements, 0),
        (App.HoppingMode, 0),           # disabled
        (App.RssiReporting, 0),
        (App.BlockStrideLength, 0),
        (App.NumberOfControlees, 1),
        (App.DstMacAddress, [dest_mac]),
        (App.StsLength, 1),
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


# =============================================================================
# ESP32 serial helpers
# =============================================================================


class EspLink:
    """Thread-safe line writer/reader for the ESP32 USB-CDC serial port."""

    def __init__(self, port, baud):
        # dtr/rts left low so the native USB-CDC does not reset the ESP32.
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


# =============================================================================
# Bridge controller
# =============================================================================


class Bridge:
    def __init__(self, clients, macs, states, dest_mac, esp, args):
        self._clients = clients
        self._macs = macs
        self._states = states
        self._dest_mac = dest_mac
        self._esp = esp
        self._args = args
        self._sessions = [None] * len(clients)
        self._ranging = False
        self._state_lock = threading.Lock()
        self._stop_event = threading.Event()

    # ---- ranging control -------------------------------------------------

    def start_ranging(self):
        with self._state_lock:
            if self._ranging:
                return
            ok = 0
            for i, client in enumerate(self._clients):
                try:
                    self._sessions[i] = start_anchor(
                        client, self._macs[i], self._dest_mac, self._args
                    )
                    ok += 1
                    print(f"[{self._args.ports[i]}] ranging started "
                          f"(mac={self._macs[i]:#x}, session={self._sessions[i]})")
                except Exception as e:
                    self._sessions[i] = None
                    print(f"[{self._args.ports[i]}] START error: {e}")
            self._ranging = ok > 0
        self._esp.send("ACK:START_RANGING")

    def stop_ranging(self):
        with self._state_lock:
            for i, client in enumerate(self._clients):
                if self._sessions[i] is not None:
                    stop_anchor(client, self._sessions[i])
                    self._sessions[i] = None
            for s in self._states:
                s.reset()
            self._ranging = False
        self._esp.send("ACK:STOP_RANGING")

    def is_ranging(self):
        with self._state_lock:
            return self._ranging

    # ---- ESP32 command reader (background thread) ------------------------

    def command_loop(self):
        while not self._stop_event.is_set():
            line = self._esp.readline()
            if not line:
                continue
            if line == "CMD:START_RANGING":
                print("[ESP] CMD:START_RANGING")
                self.start_ranging()
            elif line == "CMD:STOP_RANGING":
                print("[ESP] CMD:STOP_RANGING")
                self.stop_ranging()

    # ---- RANGE forwarding (main thread) ----------------------------------

    def forward_loop(self):
        period = 1.0 / self._args.rate_hz
        fresh_s = self._args.fresh_ms / 1000.0
        dmin, dmax = self._args.dmin, self._args.dmax

        if self._args.autostart:
            self.start_ranging()

        while not self._stop_event.is_set():
            time.sleep(period)

            if not self.is_ranging():
                continue

            now = time.monotonic()
            dists = [0.0, 0.0, 0.0]
            all_valid = len(self._states) == 3
            for i, s in enumerate(self._states):
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
                f"d2={dists[2]:.3f},valid={valid}"
            )

    def shutdown(self):
        self._stop_event.set()
        self.stop_ranging()


# =============================================================================
# Main
# =============================================================================


def main():
    parser = argparse.ArgumentParser(
        description="Bridge N UWB anchors to the ESP32-S3 over one serial port. "
        "Forwards raw distances (RANGE:) and obeys CMD:START/STOP_RANGING."
    )
    parser.add_argument("-p", "--ports", nargs="+", required=True,
                        help="Anchor COM ports, e.g.: COM11 COM19 COM12")
    parser.add_argument("--macs", nargs="+", default=["0", "1", "2"],
                        help="Anchor MAC addresses (default: 0 1 2)")
    parser.add_argument("--dest-mac", type=str, default="0x06C1",
                        help="Phone/controller address (default: 0x06C1)")
    parser.add_argument("-s", "--session", type=int, default=42,
                        help="Session id (default: 42)")
    parser.add_argument("-c", "--channel", type=int, default=9,
                        help="Channel number (default: 9)")
    parser.add_argument("--preamble-idx", type=int, default=9,
                        help="Preamble code index (default: 9)")
    parser.add_argument("--esp-port", type=str, required=True,
                        help="Serial port of the ESP32-S3 (USB-CDC), e.g.: COM20")
    parser.add_argument("--esp-baud", type=int, default=115200,
                        help="ESP32 serial baud (default: 115200)")
    parser.add_argument("--rate-hz", type=float, default=10.0,
                        help="RANGE forward rate in Hz (default: 10)")
    parser.add_argument("--fresh-ms", type=int, default=500,
                        help="Max age of a reading to count as fresh (default: 500)")
    parser.add_argument("--dmin", type=float, default=0.1,
                        help="Min valid distance in metres (default: 0.1)")
    parser.add_argument("--dmax", type=float, default=30.0,
                        help="Max valid distance in metres (default: 30.0)")
    parser.add_argument("--autostart", action="store_true", default=False,
                        help="Start ranging immediately without waiting for CMD")
    parser.add_argument("-v", "--verbose", action="store_true", default=False)

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    n = len(args.ports)
    macs = [int(m, 0) for m in args.macs]
    dest_mac = int(args.dest_mac, 0)

    if len(macs) != n:
        print("Error: --ports and --macs must have the same length.")
        sys.exit(2)
    if n != 3:
        print(f"Warning: expected 3 anchors, got {n}. RANGE line always carries d0..d2.")

    states = [AnchorState() for _ in range(n)]
    clients = []
    for i in range(n):
        client = Client(port=args.ports[i])
        client.notif_handlers = {
            (Gid.Ranging, OidRanging.Start): make_range_handler(states[i]),
            ("default", "default"): lambda gid, oid, x: None,
        }
        clients.append(client)

    try:
        esp = EspLink(args.esp_port, args.esp_baud)
    except Exception as e:
        print(f"Cannot open ESP32 port {args.esp_port}: {e}")
        for c in clients:
            c.close()
        sys.exit(1)

    print(f"ESP32 bridge on {args.esp_port} @ {args.esp_baud}")
    print(f"Anchors: {', '.join(args.ports)}  (Ctrl+C to quit)\n")

    bridge = Bridge(clients, macs, states, dest_mac, esp, args)

    cmd_thread = threading.Thread(target=bridge.command_loop, daemon=True)
    cmd_thread.start()

    try:
        bridge.forward_loop()
    except KeyboardInterrupt:
        pass
    finally:
        print("\nStopping...")
        bridge.shutdown()
        for c in clients:
            c.close()
        esp.close()
        print("Done.")


if __name__ == "__main__":
    main()
