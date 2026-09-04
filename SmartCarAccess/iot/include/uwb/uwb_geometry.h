#pragma once

// Fixed anchor positions in car-frame metres.
// Index order matches d0/d1/d2 from the PC bridge:
//   d0 (COM11) = rear centre, d1 (COM19) = right side, d2 (COM12) = left B-pillar.
namespace UwbGeo {

constexpr int kNumAnchors = 3;
constexpr double kAnchorX[kNumAnchors] = {0.0, 0.95, -0.95};
constexpr double kAnchorY[kNumAnchors] = {-2.0, 0.0, 0.0};

}  // namespace UwbGeo