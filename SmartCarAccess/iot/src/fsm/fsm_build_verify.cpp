/**
 * FSM Build Verification
 * 
 * This file helps verify that all FSM components compile correctly.
 * Include this in your build to catch compilation errors early.
 */

#include "fsm/fsm.h"
#include "fsm/fsm_integration.h"
#include <Arduino.h>

// Verification function - not called, just for compile-time checks
void verify_fsm_compilation() {
  // Test FSM API
  FSM::begin();
  FSM::tick();
  
  auto state = FSM::getCurrentState();
  auto info = FSM::getStateInfo();
  auto& ctx = FSM::getContext();
  
  FSM::triggerEvent(FSM::Event::SYSTEM_READY);
  FSM::triggerEventWithData(FSM::Event::PROVISION_START, nullptr, 0);
  
  FSM::setStateTimeout(1000);
  FSM::clearTimeout();
  
  FSM::reset(false);
  FSM::printStatus();
  FSM::printTransitionTable();
  FSM::validateConfiguration();
  
  // Test integration wrappers
  uint8_t uid[4] = {0};
  uint8_t key[65] = {0};
  uint8_t addr[6] = {0};
  
  FSMIntegration::NFC::onCardDetected(uid);
  FSMIntegration::NFC::onCardRemoved();
  FSMIntegration::NFC::onSelectAIDSuccess();
  FSMIntegration::NFC::onSelectAIDFailed();
  FSMIntegration::NFC::onKeysExchanged(key);
  FSMIntegration::NFC::onKeysInvalid();
  FSMIntegration::NFC::onCredentialsStored();
  FSMIntegration::NFC::onTimeout();
  FSMIntegration::NFC::onError(FSM::ErrorCode::NFC_TIMEOUT);
  
  FSMIntegration::BLE::onClientConnected(addr);
  FSMIntegration::BLE::onClientDisconnected();
  FSMIntegration::BLE::onClientHelloReceived(key);
  FSMIntegration::BLE::onServerHelloSent(key);
  FSMIntegration::BLE::onAuthVerified();
  FSMIntegration::BLE::onAuthFailed();
  FSMIntegration::BLE::onSessionExpired();
  FSMIntegration::BLE::onUnlockRequested();
  FSMIntegration::BLE::onError(FSM::ErrorCode::BLE_INIT_FAILED);
  FSMIntegration::BLE::onAdminCommand();
  
  FSMIntegration::Unlock::onProximityOK();
  FSMIntegration::Unlock::onProximityTooFar();
  FSMIntegration::Unlock::onSessionValid();
  FSMIntegration::Unlock::onUnlockExecuted();
  FSMIntegration::Unlock::onTimeout();
  FSMIntegration::Unlock::onError(FSM::ErrorCode::AUTH_TIMEOUT);
  
  FSMIntegration::Admin::onForceProvisionOn();
  FSMIntegration::Admin::onForceProvisionOff();
  FSMIntegration::Admin::onClearKeys();
  FSMIntegration::Admin::onDiagnosticsRequest();
  
  FSMIntegration::System::onSystemReady();
  FSMIntegration::System::onResetRequested();
  FSMIntegration::System::onError(FSM::ErrorCode::NONE);
  
  FSMIntegration::SerialCmd::startProvisioning();
  FSMIntegration::SerialCmd::toggleForceProvision();
  FSMIntegration::SerialCmd::armOneShotForce();
  FSMIntegration::SerialCmd::clearKeys();
  FSMIntegration::SerialCmd::clearAll();
  FSMIntegration::SerialCmd::printDiagnostics();
  FSMIntegration::SerialCmd::printFSMStatus();
  
  // Test helper functions
  FSMIntegration::storePhonePublicKey(key);
  FSMIntegration::storePhoneEphemeralKey(key);
  FSMIntegration::storeECUEphemeralKey(key);
  FSMIntegration::storeSharedSecret(key);
  FSMIntegration::storeSessionKeys(key, key);
  FSMIntegration::markSessionReady();
  FSMIntegration::markPhoneKeyValid();
  FSMIntegration::storeNFCUID(uid);
  FSMIntegration::incrementRetry();
  FSMIntegration::resetRetry();
  FSMIntegration::storeLastError(FSM::ErrorCode::NONE);
  
  // Test string conversion
  FSM::stateToString(FSM::State::IDLE);
  FSM::eventToString(FSM::Event::SYSTEM_READY);
  FSM::errorToString(FSM::ErrorCode::NONE);
  
  Serial.println("✓ FSM compilation verified");
}
