#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""dashboard_hmi.py — High-Performance HMI (ISA-101) localization dashboard.

PyQt6 + pyqtgraph reimplementation of the localization UI. A neutral, "boring"
baseline makes anomalies immediately obvious: color is reserved for state changes
and alerts only.

Public API (do not change):

    win = DashboardWindow()
    win.show()
    win.set_data(d0, d1, d2, x, y, speed, heading, door_open, engine_on)

All rendering, toast firing and driver-detection logic lives inside
DashboardWindow. The data-acquisition / BLE layer is untouched.
"""

import math
import sys
import time

import numpy as np
import pyqtgraph as pg
from PyQt6.QtCore import (
    Qt, QTimer, QPropertyAnimation, QEasingCurve, pyqtProperty, QObject,
    QPointF,
)
from PyQt6.QtGui import QColor, QFont
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QLabel, QFrame, QVBoxLayout,
    QHBoxLayout, QGraphicsOpacityEffect,
)

# -----------------------------------------------------------------------------
# Palette — ISA-101 neutral baseline; color only on change/alert.
# -----------------------------------------------------------------------------
BG = "#F0EBE0"          # warm beige/cream window background
CANVAS_BG = "#F6EFDD"   # map canvas background
GRAY = "#3A3A3A"        # normal text
GRAY_LABEL = "#888888"  # secondary labels
GRAY_FAINT = "#AAAAAA"  # uncertainty ring
GRID = "#E0D8CC"        # very faint grid
CAR_STROKE = "#8A8070"
ARROW = "#555555"

GREEN = "#2D862D"       # door open / driver detected
BLUE = "#4169E1"        # engine on
ORANGE = "#FF8C00"      # warning / out-of-range

# Anchor identity colors (green / blue / orange).
ANCHOR_COLORS = ["#2D862D", "#4169E1", "#FF8C00"]
ANCHOR_LABELS = ["d0", "d1", "d2"]

FONT_FAMILY = "JetBrains Mono"

# -----------------------------------------------------------------------------
# Geometry — matches localization_demo.py / uwb_geometry.h.
# -----------------------------------------------------------------------------
ANCHORS = [
    (0.0, -2.0),    # d0 — rear centre
    (0.95, 0.0),    # d1 — right side
    (-0.95, 0.0),   # d2 — left B-pillar (driver door)
]
CAR_LENGTH = 4.0
CAR_WIDTH = 2.0
VIEW_LIMIT = 6.5

DRIVER_ANCHOR = ANCHORS[2]      # left B-pillar
DRIVER_RADIUS = 0.3             # driver-detection radius (m)

# Nominal ranges for pose metrics (orange when exceeded).
X_RANGE = (-VIEW_LIMIT, VIEW_LIMIT)
Y_RANGE = (-VIEW_LIMIT, VIEW_LIMIT)
SPEED_MAX = 3.0
HEADING_RANGE = (0.0, 360.0)

TRAIL_KEEP = 120


def _mono(size, bold=False):
    f = QFont(FONT_FAMILY, size)
    f.setStyleHint(QFont.StyleHint.Monospace)
    f.setBold(bold)
    return f


# -----------------------------------------------------------------------------
# Animated status value label (color + weight animate over 200ms).
# -----------------------------------------------------------------------------
class StatusValue(QLabel):
    """A value label whose text color animates on state change."""

    def __init__(self, text=""):
        super().__init__(text)
        self.setFont(_mono(11))
        self._color = QColor(GRAY)
        self._anim = QPropertyAnimation(self, b"textColor")
        self._anim.setDuration(200)
        self._anim.setEasingCurve(QEasingCurve.Type.InOutCubic)
        self._apply(self._color)

    def _apply(self, color):
        self.setStyleSheet(f"color: {color.name()};")

    def get_text_color(self):
        return self._color

    def set_text_color(self, color):
        self._color = color
        self._apply(color)

    textColor = pyqtProperty(QColor, get_text_color, set_text_color)

    def animate_to(self, hex_color, bold):
        self.setFont(_mono(11, bold))
        target = QColor(hex_color)
        self._anim.stop()
        self._anim.setStartValue(self._color)
        self._anim.setEndValue(target)
        self._anim.start()


# -----------------------------------------------------------------------------
# Toast — narrow text strip, slides in from right, holds, fades out.
# -----------------------------------------------------------------------------
class Toast(QLabel):
    def __init__(self, parent, word, rest, color):
        super().__init__(parent)
        self.setAttribute(Qt.WidgetAttribute.WA_TransparentForMouseEvents)
        self.setFont(_mono(10))
        # Bold event word, regular rest — via rich text.
        rest_html = f"<span style='font-weight:normal'>{rest}</span>" if rest else ""
        self.setText(
            f"<span style='color:{color};font-weight:bold'>{word}</span>"
            f"<span style='color:{GRAY}'>{rest_html}</span>"
        )
        self.setStyleSheet("background: transparent;")
        self.adjustSize()

        self._effect = QGraphicsOpacityEffect(self)
        self.setGraphicsEffect(self._effect)
        self._effect.setOpacity(1.0)

        self._slide = QPropertyAnimation(self, b"pos")
        self._slide.setDuration(150)
        self._slide.setEasingCurve(QEasingCurve.Type.OutCubic)

        self._fade = QPropertyAnimation(self._effect, b"opacity")
        self._fade.setDuration(300)
        self._fade.finished.connect(self._on_faded)

        self._on_done = None

    def show_at(self, final_pos, on_done):
        self._on_done = on_done
        self.move(self.parent().width(), final_pos.y())
        self.show()
        self._slide.setStartValue(QPointF(self.parent().width(), final_pos.y()))
        self._slide.setEndValue(QPointF(final_pos.x(), final_pos.y()))
        self._slide.start()
        QTimer.singleShot(150 + 2500, self._begin_fade)

    def move_to(self, pos):
        self.move(int(pos.x()), int(pos.y()))

    def _begin_fade(self):
        self._fade.setStartValue(1.0)
        self._fade.setEndValue(0.0)
        self._fade.start()

    def _on_faded(self):
        if self._on_done:
            self._on_done(self)
        self.deleteLater()


class ToastManager(QObject):
    """Stacks toasts vertically at the top-right of a host widget."""

    MARGIN = 12
    GAP = 8

    def __init__(self, host):
        super().__init__(host)
        self._host = host
        self._active = []
        self._last_fire = {}   # key -> monotonic time

    def fire(self, key, word, rest, color):
        now = time.monotonic()
        if now - self._last_fire.get(key, -1e9) < 1.0:
            return   # de-dupe: same event within 1s
        self._last_fire[key] = now

        toast = Toast(self._host, word, rest, color)
        self._active.append(toast)
        self._relayout()
        # place() uses the toast's own final position from relayout
        final = self._final_pos(toast)
        toast.show_at(final, self._remove)

    def _final_pos(self, toast):
        x = self._host.width() - toast.width() - self.MARGIN
        y = self.MARGIN
        for t in self._active:
            if t is toast:
                break
            y += t.height() + self.GAP
        return QPointF(x, y)

    def _relayout(self):
        y = self.MARGIN
        for t in self._active:
            x = self._host.width() - t.width() - self.MARGIN
            t.move_to(QPointF(x, y))
            y += t.height() + self.GAP

    def _remove(self, toast):
        if toast in self._active:
            self._active.remove(toast)
        self._relayout()


# -----------------------------------------------------------------------------
# Heading arrow — angle animates over 100ms; drawn as line + triangle head.
# -----------------------------------------------------------------------------
class HeadingArrow(QObject):
    LENGTH = 0.9   # metres

    def __init__(self, plot):
        super().__init__()
        self._plot = plot
        self._angle = 0.0
        self._x = 0.0
        self._y = 0.0
        self._line = pg.PlotDataItem(pen=pg.mkPen(ARROW, width=2))
        self._head = pg.PlotDataItem(
            pen=pg.mkPen(ARROW, width=1),
            fillLevel=0, brush=pg.mkBrush(ARROW))
        plot.addItem(self._line)
        plot.addItem(self._head)
        self._anim = QPropertyAnimation(self, b"angle")
        self._anim.setDuration(100)
        self._anim.setEasingCurve(QEasingCurve.Type.InOutQuad)

    def get_angle(self):
        return self._angle

    def set_angle(self, a):
        self._angle = a
        self._redraw()

    angle = pyqtProperty(float, get_angle, set_angle)

    def set_pose(self, x, y, heading_deg, visible):
        self._x, self._y = x, y
        if not visible:
            self._line.setData([], [])
            self._head.setData([], [])
            return
        target = heading_deg
        # shortest-path interpolation
        cur = self._angle
        diff = (target - cur + 180) % 360 - 180
        self._anim.stop()
        self._anim.setStartValue(cur)
        self._anim.setEndValue(cur + diff)
        self._anim.start()

    def _redraw(self):
        a = math.radians(self._angle)
        tx = self._x + self.LENGTH * math.cos(a)
        ty = self._y + self.LENGTH * math.sin(a)
        self._line.setData([self._x, tx], [self._y, ty])
        # arrowhead triangle
        head_len = 0.22
        head_w = 0.12
        bx = tx - head_len * math.cos(a)
        by = ty - head_len * math.sin(a)
        px, py = -math.sin(a), math.cos(a)
        p1 = (bx + head_w * px, by + head_w * py)
        p2 = (bx - head_w * px, by - head_w * py)
        self._head.setData([tx, p1[0], p2[0], tx], [ty, p1[1], p2[1], ty])


# -----------------------------------------------------------------------------
# Metric card — plain frame; orange left border when out of nominal range.
# -----------------------------------------------------------------------------
class MetricCard(QFrame):
    def __init__(self, label, value_color=GRAY):
        super().__init__()
        self._value_color = value_color
        self._out_of_range = False
        self.setFrameShape(QFrame.Shape.NoFrame)

        lay = QVBoxLayout(self)
        lay.setContentsMargins(4, 4, 4, 4)
        lay.setSpacing(2)

        self._value = QLabel("--")
        self._value.setFont(_mono(28, bold=True))
        self._value.setStyleSheet(f"color: {value_color};")
        self._value.setAlignment(Qt.AlignmentFlag.AlignCenter)

        self._label = QLabel(label)
        self._label.setFont(_mono(9))
        self._label.setStyleSheet(f"color: {GRAY_LABEL};")
        self._label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        lay.addWidget(self._value)
        lay.addWidget(self._label)
        self._apply_style()

    def _apply_style(self):
        if self._out_of_range:
            self.setStyleSheet(
                f"MetricCard {{ border-left: 2px solid {ORANGE}; }}")
        else:
            self.setStyleSheet("MetricCard { border: none; }")

    def set_value(self, text, out_of_range=False, color=None):
        self._value.setText(text)
        col = ORANGE if out_of_range else (color or self._value_color)
        self._value.setStyleSheet(f"color: {col};")
        if out_of_range != self._out_of_range:
            self._out_of_range = out_of_range
            self._apply_style()


# -----------------------------------------------------------------------------
# Main window.
# -----------------------------------------------------------------------------
class DashboardWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("UWB Localization — HMI")
        self.setStyleSheet(f"QMainWindow {{ background: {BG}; }}")
        self.resize(760, 1000)

        self._door_open = False
        self._engine_on = False
        self._driver_detected = False
        self._trail = []

        central = QWidget()
        central.setStyleSheet(f"background: {BG};")
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(12, 12, 12, 12)
        root.setSpacing(12)

        self._build_topbar(root)
        self._build_map(root)
        self._build_metrics(root)

        self._toasts = ToastManager(central)

        # 30 fps repaint tick (drives clock + any deferred redraw).
        self._timer = QTimer(self)
        self._timer.setInterval(33)
        self._timer.timeout.connect(self._tick)
        self._timer.start()

    # ---- top bar -----------------------------------------------------------
    def _build_topbar(self, root):
        bar = QWidget()
        bar.setFixedHeight(32)
        h = QHBoxLayout(bar)
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(8)

        def _gray_label(text):
            lbl = QLabel(text)
            lbl.setFont(_mono(11))
            lbl.setStyleSheet(f"color: {GRAY};")
            return lbl

        h.addWidget(_gray_label("DOOR"))
        self._door_value = StatusValue("LOCKED")
        h.addWidget(self._door_value)

        divider = QFrame()
        divider.setFrameShape(QFrame.Shape.VLine)
        divider.setStyleSheet(f"color: {GRAY_LABEL};")
        h.addSpacing(8)
        h.addWidget(divider)
        h.addSpacing(8)

        h.addWidget(_gray_label("ENGINE"))
        self._engine_value = StatusValue("OFF")
        h.addWidget(self._engine_value)

        h.addStretch(1)

        self._clock = QLabel("00:00:00.000")
        self._clock.setFont(_mono(9))
        self._clock.setStyleSheet(f"color: {GRAY_LABEL};")
        h.addWidget(self._clock)

        root.addWidget(bar)

    # ---- map ---------------------------------------------------------------
    def _build_map(self, root):
        pg.setConfigOptions(antialias=True)
        self._plot = pg.PlotWidget()
        self._plot.setBackground(CANVAS_BG)
        self._plot.setAspectLocked(True)
        self._plot.setXRange(-VIEW_LIMIT, VIEW_LIMIT, padding=0)
        self._plot.setYRange(-VIEW_LIMIT, VIEW_LIMIT, padding=0)
        self._plot.hideAxis("bottom")
        self._plot.hideAxis("left")
        self._plot.setMenuEnabled(False)
        self._plot.setMouseEnabled(False, False)

        vb = self._plot.getPlotItem().getViewBox()
        vb.setBackgroundColor(CANVAS_BG)

        self._draw_grid()
        self._draw_car()
        self._draw_anchors()

        # Uncertainty ring (dashed, updated per frame).
        self._ring = pg.PlotDataItem(
            pen=pg.mkPen(GRAY_FAINT, width=1,
                         style=Qt.PenStyle.DashLine))
        self._plot.addItem(self._ring)

        # EKF trail + position dot.
        self._trail_item = pg.PlotDataItem(pen=pg.mkPen(BLUE, width=1.5))
        self._plot.addItem(self._trail_item)
        self._pos_item = pg.ScatterPlotItem(
            size=10, brush=pg.mkBrush(BLUE), pen=pg.mkPen("#0D47A1", width=1))
        self._plot.addItem(self._pos_item)

        self._arrow = HeadingArrow(self._plot)

        root.addWidget(self._plot, stretch=1)

    def _draw_grid(self):
        step = 0.1  # 10 cm
        pen = pg.mkPen(GRID, width=1)
        n = int(VIEW_LIMIT / step)
        for i in range(-n, n + 1):
            v = i * step
            self._plot.addItem(pg.PlotDataItem(
                [v, v], [-VIEW_LIMIT, VIEW_LIMIT], pen=pen))
            self._plot.addItem(pg.PlotDataItem(
                [-VIEW_LIMIT, VIEW_LIMIT], [v, v], pen=pen))

    def _draw_car(self):
        x0, y0 = -CAR_WIDTH / 2, -CAR_LENGTH / 2
        xs = [x0, x0 + CAR_WIDTH, x0 + CAR_WIDTH, x0, x0]
        ys = [y0, y0, y0 + CAR_LENGTH, y0 + CAR_LENGTH, y0]
        self._plot.addItem(pg.PlotDataItem(
            xs, ys, pen=pg.mkPen(CAR_STROKE, width=2)))

    def _draw_anchors(self):
        for i, (ax, ay) in enumerate(ANCHORS):
            dot = pg.ScatterPlotItem(
                [ax], [ay], size=8,
                brush=pg.mkBrush(ANCHOR_COLORS[i]),
                pen=pg.mkPen(ANCHOR_COLORS[i]))
            self._plot.addItem(dot)
            txt = pg.TextItem(ANCHOR_LABELS[i], color=GRAY_LABEL, anchor=(0, 0.5))
            txt.setFont(_mono(9))
            txt.setPos(ax + 0.18, ay)
            self._plot.addItem(txt)

    # ---- metrics -----------------------------------------------------------
    def _build_metrics(self, root):
        row1 = QHBoxLayout()
        row1.setSpacing(8)
        self._cards_range = [
            MetricCard("d0 · rear", ANCHOR_COLORS[0]),
            MetricCard("d1 · right", ANCHOR_COLORS[1]),
            MetricCard("d2 · left", ANCHOR_COLORS[2]),
        ]
        for c in self._cards_range:
            row1.addWidget(c)
        root.addLayout(row1)

        row2 = QHBoxLayout()
        row2.setSpacing(8)
        self._card_x = MetricCard("x (m)")
        self._card_y = MetricCard("y (m)")
        self._card_speed = MetricCard("speed (m/s)")
        self._card_heading = MetricCard("heading (°)")
        for c in (self._card_x, self._card_y, self._card_speed,
                  self._card_heading):
            row2.addWidget(c)
        root.addLayout(row2)

    # ---- clock tick --------------------------------------------------------
    def _tick(self):
        t = time.time()
        ms = int((t - int(t)) * 1000)
        self._clock.setText(time.strftime("%H:%M:%S.", time.localtime(t))
                            + f"{ms:03d}")

    # ---- public API --------------------------------------------------------
    def set_data(self, d0, d1, d2, x, y, speed, heading,
                 door_open, engine_on):
        # --- status transitions -> labels + toasts --------------------------
        if door_open != self._door_open:
            self._door_open = door_open
            if door_open:
                self._door_value.setText("UNLOCKED")
                self._door_value.animate_to(GREEN, bold=True)
                self._toasts.fire("door", "UNLOCKED", "  —  door is open", GREEN)
            else:
                self._door_value.setText("LOCKED")
                self._door_value.animate_to(GRAY, bold=False)
                self._toasts.fire("door", "LOCKED", "  —  door is closed", GRAY)

        if engine_on != self._engine_on:
            self._engine_on = engine_on
            if engine_on:
                self._engine_value.setText("ON")
                self._engine_value.animate_to(BLUE, bold=True)
                self._toasts.fire("engine", "ENGINE ON",
                                  "  —  engine started", BLUE)
            else:
                self._engine_value.setText("OFF")
                self._engine_value.animate_to(GRAY, bold=False)
                self._toasts.fire("engine", "ENGINE OFF", "", GRAY)

        # --- driver detection (position within 0.3 m of driver anchor) ------
        dist_driver = math.hypot(x - DRIVER_ANCHOR[0], y - DRIVER_ANCHOR[1])
        in_zone = dist_driver <= DRIVER_RADIUS
        if in_zone and not self._driver_detected:
            self._driver_detected = True
            self._toasts.fire("driver", "DRIVER DETECTED",
                              "  —  engine start allowed", GREEN)
        elif not in_zone:
            self._driver_detected = False

        # --- ranging cards --------------------------------------------------
        for card, d in zip(self._cards_range, (d0, d1, d2)):
            if d > 0.0:
                card.set_value(f"{d:.2f}")
            else:
                card.set_value("--")

        # --- pose cards (orange when out of nominal range) ------------------
        self._card_x.set_value(f"{x:+.2f}",
                               out_of_range=not (X_RANGE[0] <= x <= X_RANGE[1]))
        self._card_y.set_value(f"{y:+.2f}",
                               out_of_range=not (Y_RANGE[0] <= y <= Y_RANGE[1]))
        self._card_speed.set_value(f"{speed:.2f}",
                                   out_of_range=speed > SPEED_MAX)
        hnorm = heading % 360.0
        self._card_heading.set_value(f"{hnorm:.0f}", out_of_range=False)

        # --- map: trail, position, uncertainty ring, heading arrow ----------
        if self._trail and math.hypot(x - self._trail[-1][0],
                                      y - self._trail[-1][1]) > 1.5:
            self._trail.clear()   # break trail on teleport / re-lock
        self._trail.append((x, y))
        if len(self._trail) > TRAIL_KEEP:
            self._trail = self._trail[-TRAIL_KEEP:]
        arr = np.array(self._trail)
        self._trail_item.setData(arr[:, 0], arr[:, 1])
        self._pos_item.setData([x], [y])

        # uncertainty ring: mean ranging residual vs anchor geometry.
        err = self._mean_ranging_error(x, y, d0, d1, d2)
        self._ring.setData(*self._circle(x, y, max(err, 0.05)))

        show_arrow = speed >= 0.05
        self._arrow.set_pose(x, y, hnorm, show_arrow)

    # ---- helpers -----------------------------------------------------------
    @staticmethod
    def _mean_ranging_error(x, y, d0, d1, d2):
        errs = []
        for (ax, ay), d in zip(ANCHORS, (d0, d1, d2)):
            if d > 0.0:
                errs.append(abs(math.hypot(x - ax, y - ay) - d))
        return float(np.mean(errs)) if errs else 0.0

    @staticmethod
    def _circle(cx, cy, r, n=48):
        th = np.linspace(0, 2 * math.pi, n)
        return cx + r * np.cos(th), cy + r * np.sin(th)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, "_toasts"):
            self._toasts._relayout()


# -----------------------------------------------------------------------------
# Standalone smoke test (synthetic data) — run: python dashboard_hmi.py
# -----------------------------------------------------------------------------
def _demo():
    app = QApplication(sys.argv)
    win = DashboardWindow()
    win.show()

    state = {"t": 0.0}

    def feed():
        state["t"] += 0.05
        t = state["t"]
        # a person circling the car
        x = 2.5 * math.cos(t * 0.4)
        y = 2.5 * math.sin(t * 0.4)
        speed = 1.0
        heading = math.degrees(math.atan2(
            math.cos(t * 0.4), -math.sin(t * 0.4))) % 360
        d0 = math.hypot(x - ANCHORS[0][0], y - ANCHORS[0][1])
        d1 = math.hypot(x - ANCHORS[1][0], y - ANCHORS[1][1])
        d2 = math.hypot(x - ANCHORS[2][0], y - ANCHORS[2][1])
        door = (int(t) % 20) > 10
        engine = (int(t) % 20) > 15
        win.set_data(d0, d1, d2, x, y, speed, heading, door, engine)

    timer = QTimer()
    timer.timeout.connect(feed)
    timer.start(50)

    sys.exit(app.exec())


if __name__ == "__main__":
    _demo()
