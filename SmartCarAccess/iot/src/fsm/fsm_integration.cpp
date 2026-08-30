#include "fsm/fsm_integration.h"
#include "fsm/fsm.h"
#include "provisioning_phase.h"
#include "ble/ble_auth.h"
#include <Arduino.h>
#include <cstring>

using namespace FSM;

namespace FSMIntegration {

// ===== NFC Event Wrappers =====
namespace NFC {

void onCardDetected(const uint8_t uid[4]) {
  storeNFCUID(uid);
  Serial.printf("[FSMIntegration::NFC] Card detected: %02X:%02X:%02X:%02X\n",
               uid[0], uid[1], uid[2], uid[3]);
  FSM::triggerEvent(Event::NFC_CARD_DETECTED);
}

void onCardRemoved() {
  Serial.println("[FSMIntegration::NFC] Card removed");
  FSM::triggerEvent(Event::NFC_CARD_REMOVED);
}

void onSelectAIDSuccess() {
  Serial.println("[FSMIntegration::NFC] SELECT AID success");
  FSM::triggerEvent(Event::SELECT_AID_SUCCESS);
}

void onSelectAIDFailed() {
  Serial.println("[FSMIntegration::NFC] SELECT AID failed");
  storeLastError(ErrorCode::NFC_SELECT_AID_FAILED);
  FSM::triggerEvent(Event::SELECT_AID_FAILED);
}

void onKeysExchanged(const uint8_t phone_pub_key[65]) {
  Serial.println("[FSMIntegration::NFC] Keys exchanged");
  storePhonePublicKey(phone_pub_key);
  markPhoneKeyValid();
  FSM::triggerEvent(Event::KEYS_EXCHANGED);
}

void onKeysInvalid() {
  Serial.println("[FSMIntegration::NFC] Keys invalid");
  storeLastError(ErrorCode::INVALID_KEY_FORMAT);
  FSM::triggerEvent(Event::KEYS_INVALID);
}

void onCredentialsStored() {
  Serial.println("[FSMIntegration::NFC] Credentials stored");
  resetRetry();
  FSM::triggerEvent(Event::CREDENTIALS_STORED);
}

void onTimeout() {
  Serial.println("[FSMIntegration::NFC] Timeout");
  storeLastError(ErrorCode::NFC_TIMEOUT);
  FSM::triggerEvent(Event::TIMEOUT);
}

void onError(ErrorCode errorCode) {
  Serial.printf("[FSMIntegration::NFC] Error: %s\n", errorToString(errorCode));
  storeLastError(errorCode);
  FSM::triggerEvent(Event::ERROR_OCCURRED);
}

} // namespace NFC

// ===== BLE Event Wrappers =====
namespace BLE {

void onClientConnected(const uint8_t clientAddr[6]) {
  Serial.printf("[FSMIntegration::BLE] Client connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
               clientAddr[0], clientAddr[1], clientAddr[2], 
               clientAddr[3], clientAddr[4], clientAddr[5]);
  FSM::triggerEvent(Event::BLE_CLIENT_CONNECTED);
}

void onClientDisconnected() {
  Serial.println("[FSMIntegration::BLE] Client disconnected");
  FSM::triggerEvent(Event::BLE_CLIENT_DISCONNECTED);
}

void onAuth0Received() {
  Serial.println("[FSMIntegration::BLE] AUTH0 received");
  FSM::triggerEvent(Event::BLE_AUTH0_RECEIVED);
}

void onAuth0ResponseSent() {
  Serial.println("[FSMIntegration::BLE] AUTH0 response sent");
  FSM::triggerEvent(Event::BLE_AUTH0_RESP_SENT);
}

void onAuth1Sent() {
  Serial.println("[FSMIntegration::BLE] AUTH1 sent");
  FSM::triggerEvent(Event::BLE_AUTH1_SENT);
}

void onAuth1ResponseReceived() {
  Serial.println("[FSMIntegration::BLE] AUTH1 response received");
  FSM::triggerEvent(Event::BLE_AUTH1_RESP_RECEIVED);
}

void onExchangeReceived() {
  Serial.println("[FSMIntegration::BLE] EXCHANGE received");
  FSM::triggerEvent(Event::BLE_EXCHANGE_RECEIVED);
}

void onExchangeResponseSent() {
  Serial.println("[FSMIntegration::BLE] EXCHANGE response sent");
  FSM::triggerEvent(Event::BLE_EXCHANGE_RESP_SENT);
}

void onControlFlowReceived() {
  Serial.println("[FSMIntegration::BLE] CONTROL FLOW received");
  FSM::triggerEvent(Event::BLE_CONTROL_FLOW_RECEIVED);
}

void onControlFlowResponseSent() {
  Serial.println("[FSMIntegration::BLE] CONTROL FLOW response sent");
  FSM::triggerEvent(Event::BLE_CONTROL_FLOW_RESP_SENT);
}

void onClientHelloReceived(const uint8_t client_ephemeral_pub[65]) {
  Serial.println("[FSMIntegration::BLE] CLIENT_HELLO received");
  storePhoneEphemeralKey(client_ephemeral_pub);
  FSM::triggerEvent(Event::CLIENT_HELLO_RECEIVED);
}

void onServerHelloSent(const uint8_t ecu_ephemeral_pub[65]) {
  Serial.println("[FSMIntegration::BLE] SERVER_HELLO sent");
  storeECUEphemeralKey(ecu_ephemeral_pub);
  FSM::triggerEvent(Event::SERVER_HELLO_SENT);
}

void onAuthVerified() {
  Serial.println("[FSMIntegration::BLE] Auth verified");
  markSessionReady();
  resetRetry();
  FSM::triggerEvent(Event::AUTH_VERIFIED);
}

void onAuthFailed() {
  Serial.println("[FSMIntegration::BLE] Auth failed");
  incrementRetry();
  storeLastError(ErrorCode::BLE_AUTH_FAILED);
  FSM::triggerEvent(Event::AUTH_FAILED);
}

void onSessionExpired() {
  Serial.println("[FSMIntegration::BLE] Session expired");
  storeLastError(ErrorCode::SESSION_EXPIRED);
  FSM::triggerEvent(Event::AUTH_SESSION_EXPIRED);
}

void onUnlockRequested() {
  Serial.println("[FSMIntegration::BLE] Unlock requested");
  FSM::triggerEvent(Event::UNLOCK_REQUESTED);
}

void onError(ErrorCode errorCode) {
  Serial.printf("[FSMIntegration::BLE] Error: %s\n", errorToString(errorCode));
  storeLastError(errorCode);
  FSM::triggerEvent(Event::ERROR_OCCURRED);
}

void onAdminCommand() {
  Serial.println("[FSMIntegration::BLE] Admin command received");
  FSM::triggerEvent(Event::ADMIN_COMMAND);
}

} // namespace BLE

// ===== Unlock Flow Event Wrappers =====
namespace Unlock {

void onProximityOK() {
  Serial.println("[FSMIntegration::Unlock] Proximity OK");
  FSM::triggerEvent(Event::PROXIMITY_OK);
}

void onProximityTooFar() {
  Serial.println("[FSMIntegration::Unlock] Proximity too far");
  FSM::triggerEvent(Event::PROXIMITY_TOO_FAR);
}

void onSessionValid() {
  Serial.println("[FSMIntegration::Unlock] Session valid");
  FSM::triggerEvent(Event::AUTH_SESSION_VALID);
}

void onUnlockExecuted() {
  Serial.println("[FSMIntegration::Unlock] Unlock executed");
  FSM::triggerEvent(Event::UNLOCK_EXECUTED);
}

void onTimeout() {
  Serial.println("[FSMIntegration::Unlock] Timeout");
  storeLastError(ErrorCode::AUTH_TIMEOUT);
  FSM::triggerEvent(Event::TIMEOUT);
}

void onError(ErrorCode errorCode) {
  Serial.printf("[FSMIntegration::Unlock] Error: %s\n", errorToString(errorCode));
  storeLastError(errorCode);
  FSM::triggerEvent(Event::ERROR_OCCURRED);
}

} // namespace Unlock

// ===== Admin Mode Event Wrappers =====
namespace Admin {

void onForceProvisionOn() {
  Serial.println("[FSMIntegration::Admin] Force provision ON");
  FSM::triggerEvent(Event::FORCE_PROVISION_ON);
}

void onForceProvisionOff() {
  Serial.println("[FSMIntegration::Admin] Force provision OFF");
  FSM::triggerEvent(Event::FORCE_PROVISION_OFF);
}

void onClearKeys() {
  Serial.println("[FSMIntegration::Admin] Clear keys");
  FSM::triggerEvent(Event::CLEAR_KEYS);
}

void onDiagnosticsRequest() {
  Serial.println("[FSMIntegration::Admin] Diagnostics requested");
  FSM::triggerEvent(Event::DIAGNOSTICS_REQUEST);
}

} // namespace Admin

// ===== System Event Wrappers =====
namespace System {

void onSystemReady() {
  Serial.println("[FSMIntegration::System] System ready");
  FSM::triggerEvent(Event::SYSTEM_READY);
}

void onResetRequested() {
  Serial.println("[FSMIntegration::System] Reset requested");
  FSM::triggerEvent(Event::RESET_REQUESTED);
}

void onError(ErrorCode errorCode) {
  Serial.printf("[FSMIntegration::System] Error: %s\n", errorToString(errorCode));
  storeLastError(errorCode);
  FSM::triggerEvent(Event::ERROR_OCCURRED);
}

} // namespace System

// ===== Serial Command Wrappers =====
namespace SerialCmd {

void startProvisioning() {
  Serial.println("[FSMIntegration::SerialCmd] Start provisioning");
  FSM::triggerEvent(Event::PROVISION_START);
}

void toggleForceProvision() {
  Serial.println("[FSMIntegration::SerialCmd] Toggle force provision");
  // Toggle internal flag in ProvisioningPhase
  ProvisioningPhase::setForceProvisioningFlag(!ProvisioningPhase::isForceProvisioning());
  if (ProvisioningPhase::isForceProvisioning()) {
    FSM::triggerEvent(Event::FORCE_PROVISION_ON);
  } else {
    FSM::triggerEvent(Event::FORCE_PROVISION_OFF);
  }
}

void armOneShotForce() {
  Serial.println("[FSMIntegration::SerialCmd] Arm one-shot force");
  ProvisioningPhase::setOneShotForce(true);
}

void clearKeys() {
  Serial.println("[FSMIntegration::SerialCmd] Clear keys");
  ProvisioningPhase::clearProvisionedData();
  FSM::triggerEvent(Event::CLEAR_KEYS);
}

void clearAll() {
  Serial.println("[FSMIntegration::SerialCmd] Clear all");
  ProvisioningPhase::clearProvisionedData();
  FSM::reset(true);  // Clear session context
  System::onResetRequested();
}

void printDiagnostics() {
  Serial.println("[FSMIntegration::SerialCmd] Print diagnostics");
  
  Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║             SYSTEM DIAGNOSTICS                            ║");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  
  // Provisioning status
  Serial.println("║ Provisioning Status:                                      ║");
  Serial.printf("║   Provisioned: %-42s ║\n", 
               ProvisioningPhase::isProvisioned() ? "YES" : "NO");
  Serial.printf("║   Force Mode: %-43s ║\n", 
               ProvisioningPhase::isForceProvisioning() ? "ENABLED" : "DISABLED");
  // BLE Auth status (from FSM context)
  const StateContext& ctx = FSM::getContext();
  Serial.println("║                                                           ║");
  Serial.println("║ BLE Session Status:                                       ║");
  Serial.printf("║   Phone Key Valid: %-36s ║\n", 
               ctx.phone_key_valid ? "YES" : "NO");
  Serial.printf("║   Session Active: %-39s ║\n", 
               ctx.session_keys_ready ? "YES" : "NO");
  
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
  
  // FSM status
  FSM::printStatus();
  
  FSM::triggerEvent(Event::DIAGNOSTICS_REQUEST);
}

void printFSMStatus() {
  Serial.println("[FSMIntegration::SerialCmd] Print FSM status");
  FSM::printStatus();
}

} // namespace SerialCmd

// ===== Helper Functions =====

void storePhonePublicKey(const uint8_t key[65]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.phone_pub_key, key, 65);
  Serial.println("[FSMIntegration] Phone public key stored");
}

void storePhoneEphemeralKey(const uint8_t key[65]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.phone_ephemeral_pub, key, 65);
  Serial.println("[FSMIntegration] Phone ephemeral key stored");
}

void storeECUEphemeralKey(const uint8_t key[65]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.ecu_ephemeral_pub, key, 65);
  Serial.println("[FSMIntegration] ECU ephemeral key stored");
}

void storeSharedSecret(const uint8_t secret[32]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.shared_secret, secret, 32);
  Serial.println("[FSMIntegration] Shared secret stored");
}

void storeSessionKeys(const uint8_t enc_key[32], const uint8_t mac_key[32]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.session_enc_key, enc_key, 32);
  memcpy(ctx.session_mac_key, mac_key, 32);
  Serial.println("[FSMIntegration] Session keys stored");
}

void markSessionReady() {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  ctx.session_keys_ready = true;
  Serial.println("[FSMIntegration] Session marked ready");
}

void markPhoneKeyValid() {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  ctx.phone_key_valid = true;
  Serial.println("[FSMIntegration] Phone key marked valid");
}

void storeNFCUID(const uint8_t uid[4]) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  memcpy(ctx.nfc_uid, uid, 4);
  Serial.printf("[FSMIntegration] NFC UID stored: %02X:%02X:%02X:%02X\n",
               uid[0], uid[1], uid[2], uid[3]);
}

void incrementRetry() {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  ctx.retry_count++;
  Serial.printf("[FSMIntegration] Retry count: %u\n", ctx.retry_count);
}

void resetRetry() {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  ctx.retry_count = 0;
  Serial.println("[FSMIntegration] Retry count reset");
}

void storeLastError(ErrorCode error) {
  StateContext& ctx = const_cast<StateContext&>(FSM::getContext());
  ctx.last_error = error;
  ctx.error_count++;
  Serial.printf("[FSMIntegration] Error stored: %s (total: %u)\n", 
               errorToString(error), ctx.error_count);
}

} // namespace FSMIntegration
