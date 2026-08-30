#pragma once

// Fixed anchor positions in car-frame metres (default from simulation).
// A0 front, A1 left, A2 rear. Index order matches d0/d1/d2 from the PC bridge.
namespace UwbGeo {

constexpr int kNumAnchors = 3;
constexpr double kAnchorX[kNumAnchors] = {0.0, -0.85, 0.0};
constexpr double kAnchorY[kNumAnchors] = {2.0, 0.0, -2.0};

}  // namespace UwbGeo