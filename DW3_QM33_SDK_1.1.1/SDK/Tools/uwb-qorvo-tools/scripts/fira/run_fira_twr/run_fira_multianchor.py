#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

import argparse
import logging
import math
import sys
import threading
import time

from uci import *

# Below hack sometimes required when operating on windows git-bash/msys2
sys.stdin.reconfigure(encoding="utf-8")
sys.stdout.reconfigure(encoding="utf-8")


# =============================================================================
# Trilateration (port từ thuật toán trong app Android - Trilateration.kt)
# =============================================================================


def trilaterate(anchor_positions, distances):
    """Tính vị trí (x, y) từ danh sách vị trí anchor và khoảng cách.

    anchor_positions: list các (x, y) theo mét.
    distances:        list các khoảng cách (mét) hoặc None nếu chưa có.
    Trả về (x, y) hoặc None nếu không đủ dữ liệu.
    """
    samples = [
        (x, y, d)
        for (x, y), d in zip(anchor_positions, distances)
        if d is not None and d > 0
    ]

    if len(samples) < 2:
        return None
    if len(samples) == 2:
        return _trilaterate_2(samples[0], samples[1])
    return _gauss_newton(samples)


def _trilaterate_2(s1, s2):
    x1, y1, d1 = s1
    x2, y2, d2 = s2
    dx = x2 - x1
    dy = y2 - y1
    d = math.hypot(dx, dy)

    if d > d1 + d2 or d < abs(d1 - d2):
        t = d1 / (d1 + d2)
        return (x1 + t * dx, y1 + t * dy)

    a = (d1 * d1 - d2 * d2 + d * d) / (2 * d)
    h = math.sqrt(max(0.0, d1 * d1 - a * a))

    px = x1 + a * dx / d
    py = y1 + a * dy / d

    return (px + h * (-dy) / d, py + h * dx / d)


def _gauss_newton(samples):
    x = 0.0
    y = 0.0
    total_weight = 0.0
    for (sx, sy, sd) in samples:
        w = 1.0 / (sd + 1e-6)
        x += sx * w
        y += sy * w
        total_weight += w
    x /= total_weight
    y /= total_weight

    for _ in range(10):
        sxx = sxy = syy = sx_e = sy_e = 0.0
        for (sx, sy, sd) in samples:
            dx = x - sx
            dy = y - sy
            cur = math.hypot(dx, dy)
            if cur < 1e-6:
                continue
            err = cur - sd
            inv = 1.0 / cur
            jx = dx * inv
            jy = dy * inv
            sxx += jx * jx
            sxy += jx * jy
            syy += jy * jy
            sx_e += jx * err
            sy_e += jy * err

        det = sxx * syy - sxy * sxy
        if abs(det) < 1e-12:
            break

        delta_x = -(sx_e * syy - sy_e * sxy) / det
        delta_y = -(sy_e * sxx - sx_e * sxy) / det
        x += delta_x
        y += delta_y

        if abs(delta_x) < 1e-6 and abs(delta_y) < 1e-6:
            break

    return (x, y)


# =============================================================================
# Trạng thái từng anchor (thread-safe)
# =============================================================================


class AnchorState:
    def __init__(self):
        self._lock = threading.Lock()
        self._distance = None  # mét
        self._seq = -1

    def update(self, distance_m, seq):
        with self._lock:
            self._distance = distance_m
            self._seq = seq

    def get(self):
        with self._lock:
            return self._distance

    def get_seq(self):
        with self._lock:
            return self._seq


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
# Cấu hình + khởi động 1 anchor (responder / controlee)
# =============================================================================


def start_anchor(client, mac, dest_mac, args):
    rts, session_handle = client.session_init(args.session, SessionType.Ranging)
    if rts != Status.Ok:
        raise RuntimeError(f"session_init failed: {rts.name} ({rts})")

    session = session_handle if session_handle is not None else args.session

    app_configs = [
        (App.DeviceType, 0),  # controlee
        (App.DeviceRole, 0),  # responder
        (App.MultiNodeMode, 1),  # onetomany
        (App.RangingRoundUsage, 2),  # ds-deferred
        (App.DeviceMacAddress, mac),
        (App.ChannelNumber, args.channel),
        (App.ScheduleMode, 1),  # time
        (App.StsConfig, 0),  # static
        (App.RframeConfig, 3),  # sp3
        (App.ResultReportConfig, 11),  # tof|azimuth|fom
        (App.VendorId, 0x0708),
        (App.StaticStsIv, 0x060504030201),
        (App.AoaResultReq, 1),  # all-enabled
        (App.UwbInitiationTime, 0),
        (App.PreambleCodeIndex, args.preamble_idx),
        (App.SfdId, 2),
        (App.SlotDuration, 2400),
        (App.RangingInterval, 200),
        (App.SlotsPerRr, 25),
        (App.MaxNumberOfMeasurements, 0),
        (App.HoppingMode, 0),  # disabled
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


# =============================================================================
# Main
# =============================================================================


def main():
    parser = argparse.ArgumentParser(
        description="Chạy N anchor (responder) trên N cổng COM trong 1 tiến trình, "
        "thu thập khoảng cách và tính vị trí (x, y) bằng trilateration."
    )
    parser.add_argument(
        "-p", "--ports", nargs="+", required=True,
        help="Danh sách cổng COM của các anchor, ví dụ: COM11 COM19 COM12",
    )
    parser.add_argument(
        "--macs", nargs="+", default=["0", "1", "2"],
        help="Danh sách MAC (địa chỉ) của từng anchor, ví dụ: 0 1 2 (default: 0 1 2)",
    )
    parser.add_argument(
        "--anchor", nargs="+", default=["0,0", "5,0", "0,5"],
        help="Vị trí (x,y) mét của từng anchor, ví dụ: 0,0 5,0 0,5",
    )
    parser.add_argument("-s", "--session", type=int, default=42, help="Session id (default: 42)")
    parser.add_argument("-c", "--channel", type=int, default=9, help="Channel number (default: 9)")
    parser.add_argument("--preamble-idx", type=int, default=9, help="Preamble code index (default: 9)")
    parser.add_argument("--dest-mac", type=str, default="0x06C1", help="Địa chỉ phone/controller (default: 0x06C1)")
    parser.add_argument("-t", "--time", type=int, default=-1, help="Thời gian ranging (giây). -1: vô hạn. (default: -1)")
    parser.add_argument("-v", "--verbose", action="store_true", default=False)

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    n = len(args.ports)
    macs = [int(m, 0) for m in args.macs]
    anchors = [tuple(float(v) for v in a.split(",")) for a in args.anchor]
    dest_mac = int(args.dest_mac, 0)

    if len(macs) != n or len(anchors) != n:
        print("Lỗi: số lượng --ports, --macs, --anchor phải bằng nhau.")
        sys.exit(2)

    states = [AnchorState() for _ in range(n)]
    clients = []
    sessions = []

    for i in range(n):
        client = Client(port=args.ports[i])
        client.notif_handlers = {
            (Gid.Ranging, OidRanging.Start): make_range_handler(states[i]),
            ("default", "default"): lambda gid, oid, x: None,
        }
        clients.append(client)

    for i, client in enumerate(clients):
        try:
            print(f"[{args.ports[i]}] Khởi tạo anchor mac={macs[i]:#x} ...")
            session = start_anchor(client, macs[i], dest_mac, args)
            sessions.append(session)
            print(f"[{args.ports[i]}] Session {session} -> ranging started (mac={macs[i]:#x})")
        except Exception as e:
            print(f"[{args.ports[i]}] LỖI: {e}")

    if len(sessions) != n:
        print("Không khởi động đủ các anchor, thoát.")
        for client in clients:
            client.close()
        sys.exit(1)

    print("\nRanging... (Ctrl+C để dừng)\n")

    start_time = time.time()

    def should_stop():
        return args.time != -1 and (time.time() - start_time) >= args.time

    try:
        while not should_stop():
            time.sleep(0.1)

            distances = [s.get() for s in states]
            pos = trilaterate(anchors, distances)

            ts = time.strftime("%H:%M:%S", time.localtime())
            dist_str = "  ".join(
                f"d{i}={d:.2f}m" if d is not None else f"d{i}=---"
                for i, d in enumerate(distances)
            )
            if pos is not None:
                print(f"[{ts}]  {dist_str}  =>  (x={pos[0]:.2f}, y={pos[1]:.2f}) m")
            else:
                print(f"[{ts}]  {dist_str}  =>  chưa đủ dữ liệu")

    except KeyboardInterrupt:
        pass

    print("\nĐang dừng ranging...")
    for i, client in enumerate(clients):
        try:
            client.ranging_stop(sessions[i])
        except Exception:
            pass
        try:
            client.session_deinit(sessions[i])
        except Exception:
            pass
        client.close()
    print("Xong.")


if __name__ == "__main__":
    main()
