#pragma once

#include <cstdint>

namespace PKETelemetry {

constexpr int kRssiUnknown = 127;

enum class Event {
  ScanWake,
  Connect,
  Auth0Received,
  Auth1Sent,
  AuthVerified,
  ControlFlowAck,
  RssiSnapshot,
  UnlockDecision,
};

const char* eventName(Event event);

void startAttempt(const char* vehicleIdAscii = nullptr);
uint32_t attemptId();
void setVehicleId(const char* vehicleIdAscii);

void emit(Event event,
          int rssiDbm = kRssiUnknown,
          const char* unlockDecision = nullptr,
          const char* details = nullptr);

}  // namespace PKETelemetry
