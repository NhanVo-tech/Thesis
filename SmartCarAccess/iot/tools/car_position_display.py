#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tkinter top-down UWB localization map for the SmartCarAccess demo.

The display is metric-native: anchors, ranges, positions, clearance, grid
labels, and readouts are all metres. Localization and access-control decisions
stay on the ESP32; this module only renders the queue events produced by
localization_demo.py.
"""

from __future__ import annotations

import math
import time
import tkinter as tk
import tkinter.font as tkfont
from collections import deque
from contextlib import contextmanager
from dataclasses import dataclass
from tkinter import ttk
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


POLL_INTERVAL_MS = 33
TRAIL_SECONDS = 12.0
TRAIL_POINTS = 120

WALK_SPEED_MPS = 1.4
WALK_TAU_S = 0.30
MIN_TURN_RADIUS_M = 0.6

ZONE_REACH_M = 6.0
INNER_RANGE_M = 2.0
ZONE_FADE_TAU = 0.35
ZONE_HYSTERESIS_M = 0.15

BG = "#eef2f7"
SURFACE = "#ffffff"
HEADER = "#0d2748"
GRID = "#dde5ef"
GRID_AXIS = "#b6c4d6"
BODY_FILL = "#c9d8ec"
BODY_EDGE = "#12325c"
DOT_COLOR = "#e2452d"
TRAIL_COLOR = "#f6c3b6"
CLEARANCE_COLOR = "#2f7a55"
ZONE_EDGE = "#e3e9f1"
ZONE_OUTER_FILL = "#cfe3f7"
ZONE_INNER_FILL = "#ffd79a"
ZONE_LABEL_IDLE = "#aab7c7"
ZONE_LABEL_ACTIVE = "#12325c"
RANGE_COLOR = "#7a93b5"
FRESH_COLOR = "#12325c"
STALE_COLOR = "#a0aebf"
SETTLING_COLOR = "#b5731f"
GREEN = "#2f7a55"
BLUE = "#1565c0"
RED = "#e2452d"

READOUT_CHARS = 33
STATUS_WRAP_PX = 225

CAR_HALF_WIDTH_M = 0.95
CAR_HALF_LENGTH_M = 2.0
DRIVER_RADIUS_M = 0.3

ANCHOR_COLORS = ["#2f7a55", "#1565c0", "#e26d2c"]
ANCHOR_EDGE_COLORS = ["#1c5138", "#0d47a1", "#a34517"]

ZONE_NAMES = (
    "FrontLeft", "FrontRight",
    "RightFront", "RightRear",
    "RearRight", "RearLeft",
    "LeftRear", "LeftFront",
)
ZONE_LABELS = {
    "FrontLeft": "FRONT L",
    "FrontRight": "FRONT R",
    "RightFront": "RIGHT F",
    "RightRear": "RIGHT R",
    "RearRight": "REAR R",
    "RearLeft": "REAR L",
    "LeftRear": "LEFT R",
    "LeftFront": "LEFT F",
}


@dataclass(frozen=True)
class Anchor:
    name: str
    x: float
    y: float
    device_id: int
    color: str = "#0d2748"
    edge_color: str = "#12325c"


@dataclass
class RangeSample:
    anchor: Anchor
    distance_m: Optional[float] = None
    timestamp: float = 0.0

    @property
    def valid(self) -> bool:
        return (
            self.distance_m is not None
            and math.isfinite(self.distance_m)
            and self.distance_m > 0.0
        )


@dataclass
class PositionFix:
    x: float = float("nan")
    y: float = float("nan")
    vx: float = 0.0
    vy: float = 0.0
    timestamp: float = 0.0
    valid: bool = False
    residual_m: Optional[float] = None
    held: bool = False

    @property
    def speed_mps(self) -> float:
        return math.hypot(self.vx, self.vy)

    @property
    def heading_deg(self) -> float:
        if self.speed_mps <= 1e-6:
            return 0.0
        return math.degrees(math.atan2(self.vy, self.vx)) % 360.0


def default_anchors() -> List[Anchor]:
    return [
        Anchor("d0 · rear", 0.0, -2.0, 0, ANCHOR_COLORS[0], ANCHOR_EDGE_COLORS[0]),
        Anchor("d1 · right", 0.95, 0.0, 1, ANCHOR_COLORS[1], ANCHOR_EDGE_COLORS[1]),
        Anchor("d2 · left", -0.95, 0.0, 2, ANCHOR_COLORS[2], ANCHOR_EDGE_COLORS[2]),
    ]


def _fit(text: str, width: int = READOUT_CHARS) -> str:
    return text[:width].ljust(width)


def blend_hex(color_a: str, color_b: str, ratio: float) -> str:
    ratio = min(max(ratio, 0.0), 1.0)
    channels = []
    for offset in (1, 3, 5):
        left = int(color_a[offset:offset + 2], 16)
        right = int(color_b[offset:offset + 2], 16)
        channels.append(round(left + (right - left) * ratio))
    return "#%02x%02x%02x" % tuple(channels)


def arc_points(
    cx: float,
    cy: float,
    radius: float,
    start_deg: float,
    end_deg: float,
    step_deg: float = 6.0,
) -> List[Tuple[float, float]]:
    steps = max(int(abs(end_deg - start_deg) / step_deg), 1)
    points = []
    for index in range(steps + 1):
        angle = math.radians(start_deg + (end_deg - start_deg) * index / steps)
        points.append((cx + radius * math.cos(angle),
                       cy + radius * math.sin(angle)))
    return points


class Body:
    """Fixed 4.00 x 1.90 m car body centred at the firmware origin."""

    def __init__(self) -> None:
        self.cx = 0.0
        self.cy = 0.0
        self.half_w = CAR_HALF_WIDTH_M
        self.half_l = CAR_HALF_LENGTH_M
        self.reach = ZONE_REACH_M
        self.inner = INNER_RANGE_M

    @property
    def width(self) -> float:
        return 2.0 * self.half_w

    @property
    def length(self) -> float:
        return 2.0 * self.half_l

    def to_local(self, x_m: float, y_m: float) -> Tuple[float, float]:
        return x_m - self.cx, y_m - self.cy

    def to_world(self, u_m: float, v_m: float) -> Tuple[float, float]:
        return u_m + self.cx, v_m + self.cy

    def clearance(self, x_m: float, y_m: float) -> Tuple[float, Tuple[float, float]]:
        u, v = self.to_local(x_m, y_m)
        near_u = min(max(u, -self.half_w), self.half_w)
        near_v = min(max(v, -self.half_l), self.half_l)
        return math.hypot(u - near_u, v - near_v), self.to_world(near_u, near_v)

    def zone_for(self, x_m: float, y_m: float,
                 previous: Optional[str] = None) -> Optional[str]:
        clearance, _nearest = self.clearance(x_m, y_m)
        if clearance <= 0.0 or clearance > self.reach:
            return None
        u, v = self.to_local(x_m, y_m)
        over_x = abs(u) - self.half_w
        over_y = abs(v) - self.half_l
        facing_end = over_y >= over_x
        if facing_end:
            side = "Front" if v > 0.0 else "Rear"
            half = "Right" if u >= 0.0 else "Left"
        else:
            side = "Right" if u > 0.0 else "Left"
            half = "Front" if v >= 0.0 else "Rear"
        candidate = side + half

        if previous is None or previous == candidate or previous not in ZONE_NAMES:
            return candidate
        if previous.startswith(side):
            margin = abs(u) if facing_end else abs(v)
        else:
            margin = abs(over_y - over_x) / math.sqrt(2.0)
        return candidate if margin > ZONE_HYSTERESIS_M else previous

    def zone_polygon(self, zone: str) -> List[Tuple[float, float]]:
        hw, hl, reach = self.half_w, self.half_l, self.reach
        outlines = {
            "FrontLeft": [(-hw, hl), (0.0, hl), (0.0, hl + reach)]
                         + arc_points(-hw, hl, reach, 90.0, 135.0),
            "FrontRight": [(0.0, hl), (hw, hl)]
                          + arc_points(hw, hl, reach, 45.0, 90.0)
                          + [(0.0, hl + reach)],
            "RightFront": [(hw, 0.0), (hw, hl)]
                          + arc_points(hw, hl, reach, 45.0, 0.0)
                          + [(hw + reach, 0.0)],
            "RightRear": [(hw, 0.0), (hw + reach, 0.0), (hw + reach, -hl)]
                         + arc_points(hw, -hl, reach, 0.0, -45.0)
                         + [(hw, -hl)],
            "RearRight": [(hw, -hl)]
                         + arc_points(hw, -hl, reach, -45.0, -90.0)
                         + [(0.0, -(hl + reach)), (0.0, -hl)],
            "RearLeft": [(0.0, -hl), (0.0, -(hl + reach))]
                        + arc_points(-hw, -hl, reach, -90.0, -135.0)
                        + [(-hw, -hl)],
            "LeftRear": [(-hw, -hl)]
                        + arc_points(-hw, -hl, reach, -135.0, -180.0)
                        + [(-(hw + reach), 0.0), (-hw, 0.0)],
            "LeftFront": [(-hw, 0.0), (-(hw + reach), 0.0)]
                         + arc_points(-hw, hl, reach, 180.0, 135.0)
                         + [(-hw, hl)],
        }
        return [self.to_world(u, v) for u, v in outlines[zone]]

    def zone_label_anchor(self, zone: str) -> Tuple[float, float]:
        hw, hl = self.half_w, self.half_l
        offset = self.reach * 0.42
        half_x = (hw + offset) / 2.0
        half_y = (hl + offset) / 2.0
        local = {
            "FrontLeft": (-half_x, hl + offset),
            "FrontRight": (half_x, hl + offset),
            "RightFront": (hw + offset, half_y),
            "RightRear": (hw + offset, -half_y),
            "RearRight": (half_x, -(hl + offset)),
            "RearLeft": (-half_x, -(hl + offset)),
            "LeftRear": (-(hw + offset), -half_y),
            "LeftFront": (-(hw + offset), half_y),
        }[zone]
        return self.to_world(*local)

    def offset_outline(self, standoff: float,
                       step_deg: float = 6.0) -> List[Tuple[float, float]]:
        hw, hl = self.half_w, self.half_l
        points: List[Tuple[float, float]] = [(-(hw + standoff), 0.0)]

        def arc(cx: float, cy: float, start_deg: float,
                end_deg: float) -> None:
            points.extend(
                arc_points(cx, cy, standoff, start_deg, end_deg, step_deg)
            )

        points.append((-(hw + standoff), hl))
        arc(-hw, hl, 180.0, 90.0)
        points.append((hw, hl + standoff))
        arc(hw, hl, 90.0, 0.0)
        points.append((hw + standoff, -hl))
        arc(hw, -hl, 0.0, -90.0)
        points.append((-hw, -(hl + standoff)))
        arc(-hw, -hl, -90.0, -180.0)
        points.append((-(hw + standoff), 0.0))
        return [self.to_world(u, v) for u, v in points]


class WalkPathFilter:
    def __init__(self, max_speed: float = WALK_SPEED_MPS,
                 tau: float = WALK_TAU_S) -> None:
        self.max_speed = max_speed
        self.tau = tau
        self.reset()

    def reset(self) -> None:
        self._bearing: Optional[float] = None
        self._radius: Optional[float] = None
        self._time: Optional[float] = None

    @staticmethod
    def _to_polar(x_m: float, y_m: float) -> Tuple[float, float]:
        return math.hypot(x_m, y_m), math.atan2(x_m, y_m)

    @staticmethod
    def _to_cartesian(radius: float, bearing: float) -> Tuple[float, float]:
        return radius * math.sin(bearing), radius * math.cos(bearing)

    @staticmethod
    def _wrap(angle: float) -> float:
        return (angle + math.pi) % (2 * math.pi) - math.pi

    def update(self, x_m: float, y_m: float,
               now: Optional[float] = None) -> Tuple[float, float]:
        now = time.monotonic() if now is None else now
        radius, bearing = self._to_polar(x_m, y_m)
        if self._bearing is None or self._radius is None:
            self._radius, self._bearing, self._time = radius, bearing, now
            return x_m, y_m

        dt = max(now - (self._time or now), 0.0)
        self._time = now
        if dt <= 0.0:
            return self._to_cartesian(self._radius, self._bearing)

        gain = 1.0 - math.exp(-dt / self.tau)
        d_bearing = self._wrap(bearing - self._bearing) * gain
        d_radius = (radius - self._radius) * gain
        turn_radius = max(self._radius, MIN_TURN_RADIUS_M)
        step = math.hypot(turn_radius * d_bearing, d_radius)
        budget = self.max_speed * dt
        if step > budget:
            scale = budget / step
            d_bearing *= scale
            d_radius *= scale

        self._bearing += d_bearing
        self._radius = max(self._radius + d_radius, 0.0)
        return self._to_cartesian(self._radius, self._bearing)


class CarView(tk.Canvas):
    def __init__(self, master, body: Body, anchors: Sequence[Anchor],
                 margin_m: float = 6.0, **kwargs) -> None:
        super().__init__(master, background=SURFACE, highlightthickness=0,
                         **kwargs)
        self.body = body
        self.anchors = list(anchors)
        self.margin_m = margin_m
        self.position: Optional[Tuple[float, float]] = None
        self.held = False
        self.trail: deque[Tuple[float, float, float]] = deque(maxlen=TRAIL_POINTS)
        self.ranges: Dict[int, float] = {}
        self.show_ranges = True
        self.active_zone: Optional[str] = None
        self.active_inner = False
        self._zone_levels = {name: 0.0 for name in ZONE_NAMES}
        self._last_redraw = time.monotonic()
        self._batch = 0
        self.bind("<Configure>", lambda _event: self.redraw())

    @contextmanager
    def batch(self):
        self._batch += 1
        try:
            yield
        finally:
            self._batch -= 1
            if self._batch == 0:
                self.redraw()

    def set_margin(self, margin_m: float) -> None:
        self.margin_m = min(max(float(margin_m), 1.0), 10.0)
        self.redraw()

    def set_show_ranges(self, enabled: bool) -> None:
        self.show_ranges = enabled
        self.redraw()

    def set_ranges(self, ranges: Dict[int, float]) -> None:
        self.ranges = {idx: value for idx, value in ranges.items() if value > 0.0}
        self.redraw()

    def update_position(self, x_m: float, y_m: float,
                        held: bool = False) -> None:
        self.position = (x_m, y_m)
        self.held = held
        now = time.monotonic()
        self.trail.append((now, x_m, y_m))
        while self.trail and now - self.trail[0][0] > TRAIL_SECONDS:
            self.trail.popleft()
        self.active_zone = self.body.zone_for(x_m, y_m, self.active_zone)
        self.active_inner = self.body.clearance(x_m, y_m)[0] <= self.body.inner
        self.redraw()

    def hide_position(self) -> None:
        self.position = None
        self.active_zone = None
        self.active_inner = False
        self.redraw()

    def clear_trail(self) -> None:
        self.trail.clear()
        self.redraw()

    def to_screen(self, x_m: float, y_m: float) -> Tuple[float, float]:
        w, h = max(self.winfo_width(), 1), max(self.winfo_height(), 1)
        span_x = self.body.width + 2.0 * self.margin_m
        span_y = self.body.length + 2.0 * self.margin_m
        scale = min(w / span_x, h / span_y)
        return (w / 2.0 + (x_m - self.body.cx) * scale,
                h / 2.0 - (y_m - self.body.cy) * scale)

    def world_scale(self) -> float:
        w, h = max(self.winfo_width(), 1), max(self.winfo_height(), 1)
        span_x = self.body.width + 2.0 * self.margin_m
        span_y = self.body.length + 2.0 * self.margin_m
        return min(w / span_x, h / span_y)

    def redraw(self) -> None:
        if self._batch:
            return
        now = time.monotonic()
        dt = max(now - self._last_redraw, 0.0)
        self._last_redraw = now
        target_zone = self.active_zone
        for name, level in self._zone_levels.items():
            target = 1.0 if name == target_zone else 0.0
            if dt > 0.0:
                step = 1.0 - math.exp(-dt / ZONE_FADE_TAU)
                self._zone_levels[name] = level + (target - level) * step

        self.delete("all")
        self._draw_zones()
        self._draw_grid()
        self._draw_inner_band()
        self._draw_body()
        if self.show_ranges:
            self._draw_range_circles()
        self._draw_anchors()
        self._draw_trail()
        self._draw_position()

    def zone_levels(self) -> Dict[str, float]:
        return dict(self._zone_levels)

    def _draw_grid(self) -> None:
        w, h = max(self.winfo_width(), 1), max(self.winfo_height(), 1)
        scale = self.world_scale()
        x_span = w / scale / 2.0
        y_span = h / scale / 2.0
        x_min = math.floor(-x_span)
        x_max = math.ceil(x_span)
        y_min = math.floor(-y_span)
        y_max = math.ceil(y_span)

        for x in range(x_min, x_max + 1):
            sx, _ = self.to_screen(float(x), 0.0)
            color = GRID_AXIS if x == 0 else GRID
            self.create_line(sx, 0, sx, h, fill=color, width=1)
            if x != 0:
                self.create_text(sx, h - 8, text=str(x),
                                 anchor="s", fill="#8a99ad",
                                 font=("Segoe UI", 7))
        for y in range(y_min, y_max + 1):
            _, sy = self.to_screen(0.0, float(y))
            color = GRID_AXIS if y == 0 else GRID
            self.create_line(0, sy, w, sy, fill=color, width=1)
            if y != 0:
                self.create_text(6, sy, text=str(y),
                                 anchor="w", fill="#8a99ad",
                                 font=("Segoe UI", 7))

    def _draw_zones(self) -> None:
        for name in ZONE_NAMES:
            level = self._zone_levels[name]
            lit = ZONE_INNER_FILL if self.active_inner else ZONE_OUTER_FILL
            fill = blend_hex(SURFACE, lit, level)
            points = []
            for x_m, y_m in self.body.zone_polygon(name):
                points.extend(self.to_screen(x_m, y_m))
            self.create_polygon(points, fill=fill, outline=ZONE_EDGE, width=1)
            lx, ly = self.to_screen(*self.body.zone_label_anchor(name))
            color = blend_hex(ZONE_LABEL_IDLE, ZONE_LABEL_ACTIVE, level)
            self.create_text(lx, ly, text=ZONE_LABELS[name], fill=color,
                             font=("Segoe UI Semibold", 9))

    def _draw_inner_band(self) -> None:
        points = []
        for x_m, y_m in self.body.offset_outline(self.body.inner):
            points.extend(self.to_screen(x_m, y_m))
        self.create_line(*points, fill=GRID_AXIS, width=1, dash=(3, 4))

    def _draw_body(self) -> None:
        hw, hl = self.body.half_w, self.body.half_l
        x0, y0 = self.to_screen(-hw, -hl)
        x1, y1 = self.to_screen(hw, hl)
        left, right = sorted((x0, x1))
        top, bottom = sorted((y0, y1))
        self.create_rectangle(left, top, right, bottom, fill=BODY_FILL,
                              outline=BODY_EDGE, width=2)
        wx0, wy = self.to_screen(-hw, hl - 0.45)
        wx1, _ = self.to_screen(hw, hl - 0.45)
        self.create_line(wx0, wy, wx1, wy, fill=BODY_EDGE, width=2)
        label_x, label_y = self.to_screen(0.0, -hl * 0.45)
        self.create_text(label_x, label_y,
                         text="%.2f m x %.2f m" %
                         (self.body.length, self.body.width),
                         fill=BODY_EDGE, font=("Segoe UI", 8))
        ox, oy = self.to_screen(0.0, 0.0)
        self.create_line(ox - 6, oy, ox + 6, oy, fill=BODY_EDGE)
        self.create_line(ox, oy - 6, ox, oy + 6, fill=BODY_EDGE)

    def _draw_range_circles(self) -> None:
        scale = self.world_scale()
        for anchor in self.anchors:
            distance = self.ranges.get(anchor.device_id)
            if not distance:
                continue
            sx, sy = self.to_screen(anchor.x, anchor.y)
            radius = distance * scale
            self.create_oval(sx - radius, sy - radius, sx + radius, sy + radius,
                             outline=RANGE_COLOR, width=1, dash=(3, 4))
            self.create_text(sx, sy - radius - 8,
                             text="%s %.2f m" % (anchor.name, distance),
                             fill=anchor.color, font=("Segoe UI", 7))

    def _draw_anchors(self) -> None:
        for anchor in self.anchors:
            sx, sy = self.to_screen(anchor.x, anchor.y)
            self.create_oval(sx - 4, sy - 4, sx + 4, sy + 4,
                             fill=anchor.color, outline=SURFACE, width=1)
            u, v = self.body.to_local(anchor.x, anchor.y)
            offset_x = 14 if u > 0 else (-14 if u < 0 else 0)
            offset_y = -12 if v > 0 else (12 if v < 0 else 0)
            self.create_text(sx + offset_x, sy + offset_y, text=anchor.name,
                             fill=anchor.color, font=("Segoe UI", 7))

    def _draw_trail(self) -> None:
        if len(self.trail) < 2:
            return
        points = []
        for _ts, x, y in self.trail:
            points.extend(self.to_screen(x, y))
        self.create_line(*points, fill=TRAIL_COLOR, width=2, smooth=True)

    def _draw_position(self) -> None:
        if self.position is None:
            return
        x_m, y_m = self.position
        sx, sy = self.to_screen(x_m, y_m)
        clearance, nearest = self.body.clearance(x_m, y_m)
        if clearance > 0.0:
            nx, ny = self.to_screen(*nearest)
            self.create_line(nx, ny, sx, sy, fill=CLEARANCE_COLOR,
                             width=1, dash=(4, 3))
            self.create_oval(nx - 3, ny - 3, nx + 3, ny + 3,
                             fill=CLEARANCE_COLOR, outline="")
            span = math.hypot(sx - nx, sy - ny) or 1.0
            off_x = -(sy - ny) / span * 12.0
            off_y = (sx - nx) / span * 12.0
            self.create_text((nx + sx) / 2.0 + off_x,
                             (ny + sy) / 2.0 + off_y,
                             text=f"{clearance:.2f} m",
                             fill=CLEARANCE_COLOR,
                             font=("Segoe UI Semibold", 9))
        ring = SETTLING_COLOR if self.held else DOT_COLOR
        self.create_oval(sx - 14, sy - 14, sx + 14, sy + 14, outline=ring)
        self.create_oval(sx - 7, sy - 7, sx + 7, sy + 7,
                         fill=DOT_COLOR, outline=SURFACE, width=2)
        self.create_text(sx + 18, sy + 14,
                         text=f"({x_m:+.2f}, {y_m:+.2f}) m",
                         anchor="w", fill=DOT_COLOR,
                         font=("Segoe UI Semibold", 9))


def configure_styles(root: tk.Tk) -> None:
    style = ttk.Style(root)
    try:
        style.theme_use("clam")
    except tk.TclError:
        pass
    root.configure(background=BG)
    default = tkfont.nametofont("TkDefaultFont")
    default.configure(family="Segoe UI", size=10)
    style.configure(".", background=BG, foreground=FRESH_COLOR,
                    font=("Segoe UI", 10))
    style.configure("Header.TFrame", background=HEADER)
    style.configure("HeaderTitle.TLabel", background=HEADER,
                    foreground="#ffffff", font=("Segoe UI", 18, "bold"))
    style.configure("HeaderSubtitle.TLabel", background=HEADER,
                    foreground="#b8c9dc", font=("Segoe UI", 10))
    style.configure("Surface.TFrame", background=SURFACE)
    style.configure("Section.TLabelframe", background=SURFACE,
                    foreground=FRESH_COLOR, bordercolor="#d6e0ed")
    style.configure("Section.TLabelframe.Label", background=SURFACE,
                    foreground=FRESH_COLOR, font=("Segoe UI", 10, "bold"))
    style.configure("Section.TFrame", background=SURFACE)
    style.configure("TButton", font=("Segoe UI", 9))
    style.configure("TCheckbutton", background=SURFACE, foreground=FRESH_COLOR)


class PositionApp(ttk.Frame):
    def __init__(self, root: tk.Tk, anchors: Optional[Sequence[Anchor]] = None,
                 margin_m: float = 6.0) -> None:
        super().__init__(root, style="Surface.TFrame")
        self.root = root
        self.anchors = list(anchors or default_anchors())
        self.body = Body()
        self.walk_filter = WalkPathFilter()
        self.closed = False
        self._tick_id: Optional[str] = None
        self._last_ranges: Dict[int, RangeSample] = {}
        self._last_fix: Optional[PositionFix] = None
        self._access_state = "LOCKED"
        self._ignition = False
        self._fresh_range_ids: set[int] = set()

        self.pack(fill="both", expand=True)
        self._build_header()
        self._build_main(margin_m)
        self._tick()

    def _build_header(self) -> None:
        header = ttk.Frame(self, style="Header.TFrame", padding=(18, 12))
        header.pack(fill="x")
        ttk.Label(header, text="UWB Localization",
                  style="HeaderTitle.TLabel").pack(anchor="w")
        ttk.Label(header,
                  text="UWB trilateration — 3 anchors over 4.00 x 1.90 m",
                  style="HeaderSubtitle.TLabel").pack(anchor="w", pady=(2, 0))

    def _build_main(self, margin_m: float) -> None:
        main = ttk.Frame(self, style="Surface.TFrame")
        main.pack(fill="both", expand=True)

        self.view = CarView(main, self.body, self.anchors, margin_m=margin_m)
        self.view.pack(side="left", fill="both", expand=True)

        side = ttk.Frame(main, style="Surface.TFrame", padding=(12, 12))
        side.pack(side="right", fill="y")

        controls = ttk.LabelFrame(side, text="Control",
                                  style="Section.TLabelframe", padding=10)
        controls.pack(fill="x")
        ttk.Button(controls, text="Clear trail",
                   command=self._clear_trail).pack(anchor="w")
        status_box = ttk.Frame(controls, style="Section.TFrame")
        status_box.pack(fill="x", pady=(10, 0))
        self.status_var = tk.StringVar(value="Idle")
        ttk.Label(status_box, textvariable=self.status_var,
                  wraplength=STATUS_WRAP_PX, justify="left").pack(anchor="w")

        ranges = ttk.LabelFrame(side, text="Anchor ranges",
                                style="Section.TLabelframe", padding=10)
        ranges.pack(fill="x", pady=(12, 0))
        self.range_vars: Dict[int, tk.StringVar] = {}
        self.range_labels: Dict[int, ttk.Label] = {}
        for anchor in self.anchors:
            var = tk.StringVar(value=self._range_row(anchor))
            label = ttk.Label(ranges, textvariable=var, font=("Consolas", 10))
            label.pack(anchor="w")
            self.range_vars[anchor.device_id] = var
            self.range_labels[anchor.device_id] = label

        readout = ttk.LabelFrame(side, text="Position",
                                 style="Section.TLabelframe", padding=10)
        readout.pack(fill="x", pady=(12, 0))
        self.x_var = tk.StringVar(value=_fit("x        -"))
        self.y_var = tk.StringVar(value=_fit("y        -"))
        self.speed_var = tk.StringVar(value=_fit("speed    -"))
        self.heading_var = tk.StringVar(value=_fit("heading  -"))
        self.residual_var = tk.StringVar(value=_fit("residual -"))
        self.solver_var = tk.StringVar(value=_fit("solver   -"))
        for var in (self.x_var, self.y_var, self.speed_var, self.heading_var,
                    self.residual_var, self.solver_var):
            ttk.Label(readout, textvariable=var,
                      font=("Consolas", 10)).pack(anchor="w")

        access = ttk.LabelFrame(side, text="Access",
                                style="Section.TLabelframe", padding=10)
        access.pack(fill="x", pady=(12, 0))
        self.door_var = tk.StringVar(value=_fit("door     LOCKED"))
        self.engine_var = tk.StringVar(value=_fit("engine   OFF"))
        self.driver_var = tk.StringVar(value=_fit("driver   -"))
        self.door_label = ttk.Label(access, textvariable=self.door_var,
                                    font=("Consolas", 10, "bold"))
        self.engine_label = ttk.Label(access, textvariable=self.engine_var,
                                      font=("Consolas", 10, "bold"))
        self.driver_label = ttk.Label(access, textvariable=self.driver_var,
                                      font=("Consolas", 10, "bold"))
        for label in (self.door_label, self.engine_label, self.driver_label):
            label.pack(anchor="w")

        options = ttk.LabelFrame(side, text="View options",
                                 style="Section.TLabelframe", padding=10)
        options.pack(fill="x", pady=(12, 0))
        self.filter_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Constrain to walking path",
                        variable=self.filter_var,
                        command=self.walk_filter.reset).pack(anchor="w")
        self.circles_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Show range circles",
                        variable=self.circles_var,
                        command=self._toggle_circles).pack(anchor="w")

        zoom = ttk.LabelFrame(side, text="View margin (m)",
                              style="Section.TLabelframe", padding=10)
        zoom.pack(fill="x", pady=(12, 0))
        self.margin_var = tk.DoubleVar(value=margin_m)
        ttk.Scale(zoom, from_=1.0, to=10.0, variable=self.margin_var,
                  command=lambda _v: self.view.set_margin(
                      self.margin_var.get())).pack(fill="x")

    @staticmethod
    def _range_row(anchor: Anchor, distance_m: Optional[float] = None,
                   age: Optional[float] = None, note: str = "") -> str:
        if distance_m is None:
            return _fit(f"{anchor.name:<12}      --")
        age_text = "-" if age is None else f"{age:4.1f}s"
        return _fit(f"{anchor.name:<12} {distance_m:5.2f} m {age_text}{note}")

    def _clear_trail(self) -> None:
        self.walk_filter.reset()
        self.view.clear_trail()

    def _toggle_circles(self) -> None:
        self.view.set_show_ranges(self.circles_var.get())

    def close(self) -> None:
        self.closed = True
        if self._tick_id is not None:
            try:
                self.after_cancel(self._tick_id)
            except tk.TclError:
                pass
        try:
            self.root.destroy()
        except tk.TclError:
            pass

    def run(self) -> None:
        try:
            self.root.mainloop()
        except tk.TclError:
            pass
        self.closed = True

    def _tick(self) -> None:
        if self.closed:
            return
        self._refresh_range_rows()
        self._refresh_access()
        self.view.redraw()
        self._tick_id = self.after(POLL_INTERVAL_MS, self._tick)

    def update_ranges(self, distances: Sequence[float]) -> None:
        now = time.monotonic()
        self._fresh_range_ids = set()
        for anchor, distance in zip(self.anchors, distances):
            if distance is not None and math.isfinite(distance) and distance > 0.0:
                self._last_ranges[anchor.device_id] = RangeSample(
                    anchor=anchor, distance_m=float(distance), timestamp=now)
                self._fresh_range_ids.add(anchor.device_id)
        self._refresh_range_rows()

    def update_position(self, fix: PositionFix) -> None:
        self._last_fix = fix
        if not fix.valid or not math.isfinite(fix.x) or not math.isfinite(fix.y):
            self.view.hide_position()
            self.x_var.set(_fit("x        -"))
            self.y_var.set(_fit("y        -"))
            self.speed_var.set(_fit("speed    -"))
            self.heading_var.set(_fit("heading  -"))
            self.residual_var.set(_fit("residual -"))
            self.solver_var.set(_fit("solver   held"))
            return

        x_m, y_m = fix.x, fix.y
        if self.filter_var.get():
            u, v = self.body.to_local(x_m, y_m)
            u, v = self.walk_filter.update(u, v, now=fix.timestamp or None)
            x_m, y_m = self.body.to_world(u, v)
        self.view.update_position(x_m, y_m, held=fix.held)

        self.x_var.set(_fit(f"x     {x_m:+8.3f} m"))
        self.y_var.set(_fit(f"y     {y_m:+8.3f} m"))
        self.speed_var.set(_fit(f"speed  {fix.speed_mps:8.3f} m/s"))
        self.heading_var.set(_fit(f"heading {fix.heading_deg:7.1f} deg"))
        if fix.residual_m is None or not math.isfinite(fix.residual_m):
            self.residual_var.set(_fit("residual      -"))
        else:
            self.residual_var.set(_fit(f"residual {fix.residual_m:6.3f} m"))
        solver = "held" if fix.held else "ESP32 trilateration"
        self.solver_var.set(_fit(f"solver   {solver}"))
        self._refresh_access()

    def update_access(self, state: Optional[str] = None,
                      ignition: Optional[bool] = None) -> None:
        if state is not None:
            self._access_state = state
        if ignition is not None:
            self._ignition = bool(ignition)
        self._refresh_access()

    def _refresh_range_rows(self) -> None:
        now = time.monotonic()
        circles: Dict[int, float] = {}
        fresh = []
        missing = []
        for anchor in self.anchors:
            sample = self._last_ranges.get(anchor.device_id)
            label = self.range_labels[anchor.device_id]
            var = self.range_vars[anchor.device_id]
            if sample is None:
                var.set(self._range_row(anchor))
                label.configure(foreground=STALE_COLOR)
                missing.append(anchor.name.split(" ")[0])
                continue
            age = now - sample.timestamp
            stale = age > 1.0 or anchor.device_id not in self._fresh_range_ids
            note = "  stale" if stale else ""
            var.set(self._range_row(anchor, sample.distance_m, age, note))
            label.configure(foreground=STALE_COLOR if stale else FRESH_COLOR)
            if sample.valid and not stale:
                circles[anchor.device_id] = float(sample.distance_m)
                fresh.append(anchor.name.split(" ")[0])
            else:
                missing.append(anchor.name.split(" ")[0])

        self.view.set_ranges(circles)
        if len(fresh) == len(self.anchors) and self._last_fix is not None:
            self.status_var.set("Tracking")
        elif fresh:
            self.status_var.set(
                f"{len(fresh)}/3 anchors - waiting for {', '.join(missing)}")
        else:
            self.status_var.set("Waiting for anchor ranges...")

    def _refresh_access(self) -> None:
        state_color = {
            "LOCKED": RED,
            "DOOR_UNLOCKED": GREEN,
            "OCCUPIED": BLUE,
        }.get(self._access_state, FRESH_COLOR)
        self.door_var.set(_fit(f"door     {self._access_state}"))
        self.door_label.configure(foreground=state_color)
        self.engine_var.set(_fit(f"engine   {'ON' if self._ignition else 'OFF'}"))
        self.engine_label.configure(foreground=BLUE if self._ignition else FRESH_COLOR)

        driver = False
        if self._last_fix is not None and self._last_fix.valid:
            left_anchor = self.anchors[2]
            driver = (
                math.hypot(self._last_fix.x - left_anchor.x,
                           self._last_fix.y - left_anchor.y)
                <= DRIVER_RADIUS_M
            )
        self.driver_var.set(_fit(f"driver   {'DETECTED' if driver else '-'}"))
        self.driver_label.configure(foreground=GREEN if driver else FRESH_COLOR)


def create_position_window(title: str = "UWB Localization",
                           margin_m: float = 6.0) -> PositionApp:
    root = tk.Tk()
    root.title(title)
    root.geometry("1080x820")
    root.minsize(860, 680)
    configure_styles(root)
    app = PositionApp(root, default_anchors(), margin_m=margin_m)
    root.protocol("WM_DELETE_WINDOW", app.close)
    return app


def run_event_loop(app: PositionApp) -> None:
    app.run()


def _demo_feed(app: PositionApp, state: Dict[str, float]) -> None:
    if app.closed:
        return
    state["t"] += 0.05
    t = state["t"]
    radius = 3.0 + 0.45 * math.sin(t * 0.33)
    x = radius * math.cos(t * 0.45)
    y = radius * math.sin(t * 0.45)
    vx = -radius * 0.45 * math.sin(t * 0.45)
    vy = radius * 0.45 * math.cos(t * 0.45)
    dists = [math.hypot(x - a.x, y - a.y) for a in app.anchors]
    if int(t) % 18 in (7, 8):
        dists[1] = 0.0
    app.update_ranges(dists)
    app.update_position(PositionFix(
        x=x, y=y, vx=vx, vy=vy, valid=True, timestamp=time.monotonic()))
    if int(t) % 24 > 15:
        door = "OCCUPIED"
    elif int(t) % 24 > 8:
        door = "DOOR_UNLOCKED"
    else:
        door = "LOCKED"
    app.update_access(door, int(t) % 24 > 18)
    app.after(50, lambda: _demo_feed(app, state))


if __name__ == "__main__":
    demo = create_position_window("UWB Localization (self-test)")
    _demo_feed(demo, {"t": 0.0})
    run_event_loop(demo)
