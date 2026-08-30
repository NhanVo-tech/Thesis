#pragma once

// Placeholder EKF sink for the 2D position from trilateration.
// No-op for now; will be replaced by the real filter downstream.
namespace Ekf {

void update(double x, double y);

}  // namespace Ekf