#include "fsm/fsm_states.h"
#include <cstring>

namespace FSM {

// Note: StateContext::reset() is already inline in the header

// State to string conversion
const char* stateToString(State state) {
  switch (state) {
    case State::INIT: return "INIT";
    case State::IDLE: return "IDLE";
    case State::PROVISIONING_WAIT_TAP: return "PROV_WAIT_TAP";
    case State::PROVISIONING_SELECT_AID: return "PROV_SELECT_AID";
    case State::PROVISIONING_EXCHANGE_KEYS: return "PROV_EXCHANGE_KEYS";
    case State::PROVISIONING_STORE_CREDS: return "PROV_STORE_CREDS";
    case State::AUTH_WAIT_AUTH0: return "AUTH_WAIT_AUTH0";
    case State::AUTH_PROCESSING_AUTH0_STD: return "AUTH_PROCESSING_AUTH0_STD";
    case State::AUTH_WAIT_AUTH1_RESP: return "AUTH_WAIT_AUTH1_RESP";
    case State::AUTH_SECURE_CHANNEL_READY: return "AUTH_SECURE_CHANNEL_READY";
    case State::AUTH_WAIT_CONNECT: return "AUTH_WAIT_CONNECT";
    case State::AUTH_HANDSHAKE: return "AUTH_HANDSHAKE";
    case State::AUTH_VERIFY_KEYS: return "AUTH_VERIFY_KEYS";
    case State::AUTH_SESSION_READY: return "AUTH_SESSION_READY";
    case State::UNLOCKING_CHECK_PROXIMITY: return "UNLOCK_CHECK_PROX";
    case State::UNLOCKING_VERIFY_AUTH: return "UNLOCK_VERIFY_AUTH";
    case State::UNLOCKING_EXECUTE: return "UNLOCK_EXECUTE";
    case State::UNLOCKING_COMPLETE: return "UNLOCK_COMPLETE";
    case State::ADMIN_MODE: return "ADMIN_MODE";
    case State::ERROR_HANDLER: return "ERROR_HANDLER";
    default: return "UNKNOWN";
  }
}

// Event to string conversion
const char* eventToString(Event event) {
  switch (event) {
    case Event::SYSTEM_READY: return "SYSTEM_READY";
    case Event::PROVISION_START: return "PROVISION_START";
    case Event::NFC_CARD_DETECTED: return "NFC_CARD_DETECTED";
    case Event::NFC_CARD_REMOVED: return "NFC_CARD_REMOVED";
    case Event::SELECT_AID_SUCCESS: return "SELECT_AID_SUCCESS";
    case Event::SELECT_AID_FAILED: return "SELECT_AID_FAILED";
    case Event::KEYS_EXCHANGED: return "KEYS_EXCHANGED";
    case Event::KEYS_INVALID: return "KEYS_INVALID";
    case Event::CREDENTIALS_STORED: return "CREDENTIALS_STORED";
    case Event::BLE_CLIENT_CONNECTED: return "BLE_CLIENT_CONNECTED";
    case Event::BLE_CLIENT_DISCONNECTED: return "BLE_CLIENT_DISCONNECTED";
    case Event::BLE_AUTH0_RECEIVED: return "BLE_AUTH0_RECEIVED";
    case Event::BLE_AUTH0_RESP_SENT: return "BLE_AUTH0_RESP_SENT";
    case Event::BLE_AUTH1_SENT: return "BLE_AUTH1_SENT";
    case Event::BLE_AUTH1_RESP_RECEIVED: return "BLE_AUTH1_RESP_RECEIVED";
    case Event::BLE_EXCHANGE_RECEIVED: return "BLE_EXCHANGE_RECEIVED";
    case Event::BLE_EXCHANGE_RESP_SENT: return "BLE_EXCHANGE_RESP_SENT";
    case Event::BLE_CONTROL_FLOW_RECEIVED: return "BLE_CONTROL_FLOW_RECEIVED";
    case Event::BLE_CONTROL_FLOW_RESP_SENT: return "BLE_CONTROL_FLOW_RESP_SENT";
    case Event::CLIENT_HELLO_RECEIVED: return "CLIENT_HELLO_RECEIVED";
    case Event::SERVER_HELLO_SENT: return "SERVER_HELLO_SENT";
    case Event::AUTH_VERIFIED: return "AUTH_VERIFIED";
    case Event::AUTH_FAILED: return "AUTH_FAILED";
    case Event::AUTH_SESSION_EXPIRED: return "AUTH_SESSION_EXPIRED";
    case Event::UNLOCK_REQUESTED: return "UNLOCK_REQUESTED";
    case Event::PROXIMITY_OK: return "PROXIMITY_OK";
    case Event::PROXIMITY_TOO_FAR: return "PROXIMITY_TOO_FAR";
    case Event::AUTH_SESSION_VALID: return "AUTH_SESSION_VALID";
    case Event::UNLOCK_EXECUTED: return "UNLOCK_EXECUTED";
    case Event::ADMIN_COMMAND: return "ADMIN_COMMAND";
    case Event::FORCE_PROVISION_ON: return "FORCE_PROVISION_ON";
    case Event::FORCE_PROVISION_OFF: return "FORCE_PROVISION_OFF";
    case Event::CLEAR_KEYS: return "CLEAR_KEYS";
    case Event::DIAGNOSTICS_REQUEST: return "DIAGNOSTICS_REQUEST";
    case Event::ERROR_OCCURRED: return "ERROR_OCCURRED";
    case Event::RESET_REQUESTED: return "RESET_REQUESTED";
    case Event::TIMEOUT: return "TIMEOUT";
    default: return "UNKNOWN";
  }
}

// Error code to string conversion
const char* errorToString(ErrorCode error) {
  switch (error) {
    case ErrorCode::NONE: return "NONE";
    case ErrorCode::NFC_INIT_FAILED: return "NFC_INIT_FAILED";
    case ErrorCode::NFC_SAM_CONFIG_FAILED: return "NFC_SAM_CONFIG_FAILED";
    case ErrorCode::NFC_TIMEOUT: return "NFC_TIMEOUT";
    case ErrorCode::NFC_APDU_FAILED: return "NFC_APDU_FAILED";
    case ErrorCode::NFC_SELECT_AID_FAILED: return "NFC_SELECT_AID_FAILED";
    case ErrorCode::BLE_INIT_FAILED: return "BLE_INIT_FAILED";
    case ErrorCode::BLE_DISCONNECT_UNEXPECTED: return "BLE_DISCONNECT_UNEXPECTED";
    case ErrorCode::BLE_INVALID_DATA: return "BLE_INVALID_DATA";
    case ErrorCode::BLE_AUTH_FAILED: return "BLE_AUTH_FAILED";
    case ErrorCode::CRYPTO_SIGNATURE_INVALID: return "CRYPTO_SIGNATURE_INVALID";
    case ErrorCode::CRYPTO_ECDH_FAILED: return "CRYPTO_ECDH_FAILED";
    case ErrorCode::CRYPTO_HMAC_MISMATCH: return "CRYPTO_HMAC_MISMATCH";
    case ErrorCode::INVALID_KEY_FORMAT: return "INVALID_KEY_FORMAT";
    case ErrorCode::STORAGE_WRITE_FAILED: return "STORAGE_WRITE_FAILED";
    case ErrorCode::STORAGE_READ_FAILED: return "STORAGE_READ_FAILED";
    case ErrorCode::INVALID_STATE_TRANSITION: return "INVALID_STATE_TRANSITION";
    case ErrorCode::NOT_PROVISIONED: return "NOT_PROVISIONED";
    case ErrorCode::SESSION_NOT_READY: return "SESSION_NOT_READY";
    case ErrorCode::SESSION_EXPIRED: return "SESSION_EXPIRED";
    case ErrorCode::AUTH_TIMEOUT: return "AUTH_TIMEOUT";
    default: return "UNDEFINED";
  }
}

} // namespace FSM
