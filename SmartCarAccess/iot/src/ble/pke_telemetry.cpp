#include <Arduino.h>
#include <cstring>

#include "ble/pke_telemetry.h"

namespace {
uint32_t s_attemptId = 0;
char s_vehicleId[9] = {0};

void copyVehicleId(const char* vehicleIdAscii) {
  memset(s_vehicleId, 0, sizeof(s_vehicleId));
  if (!vehicleIdAscii) return;
  strncpy(s_vehicleId, vehicleIdAscii, 8);
  s_vehicleId[8] = '\0';
}
}  // namespace

namespace PKETelemetry {

const char* eventName(Event event) {
  switch (event) {
    case Event::ScanWake: return "scan_wake";
    case Event::Connect: return "connect";
    case Event::Auth0Received: return "auth0_received";
    case Event::Auth1Sent: return "auth1_sent";
    case Event::AuthVerified: return "auth_verified";
    case Event::ControlFlowAck: return "control_flow_ack";
    case Event::RssiSnapshot: return "rssi_snapshot";
    case Event::UnlockDecision: return "unlock_decision";
  }
  return "unknown";
}

void startAttempt(const char* vehicleIdAscii) {
  s_attemptId++;
  copyVehicleId(vehicleIdAscii);
}

uint32_t attemptId() {
  return s_attemptId;
}

void setVehicleId(const char* vehicleIdAscii) {
  copyVehicleId(vehicleIdAscii);
}

void emit(Event event, int rssiDbm, const char* unlockDecision, const char* details) {
  Serial.print("[PKE-TLM] {");
  Serial.print("\"schema\":\"pke.v1\",");
  Serial.print("\"source\":\"esp\",");
  Serial.print("\"attempt_id\":");
  Serial.print(s_attemptId);
  Serial.print(",\"event\":\"");
  Serial.print(eventName(event));
  Serial.print("\",");
  Serial.print("\"ts_ms\":");
  Serial.print((unsigned long)millis());
  Serial.print(",\"vehicle_id\":");
  if (s_vehicleId[0] != '\0') {
    Serial.print("\"");
    Serial.print(s_vehicleId);
    Serial.print("\"");
  } else {
    Serial.print("null");
  }
  Serial.print(",\"rssi_dbm\":");
  if (rssiDbm == kRssiUnknown) {
    Serial.print("null");
  } else {
    Serial.print(rssiDbm);
  }
  Serial.print(",\"unlock_decision\":");
  if (unlockDecision) {
    Serial.print("\"");
    Serial.print(unlockDecision);
    Serial.print("\"");
  } else {
    Serial.print("null");
  }
  Serial.print(",\"details\":");
  if (details) {
    Serial.print("\"");
    Serial.print(details);
    Serial.print("\"");
  } else {
    Serial.print("null");
  }
  Serial.println("}");
}

}  // namespace PKETelemetry
