#ifndef FSM_INTEGRATION_H
#define FSM_INTEGRATION_H

#include "fsm.h"
#include <cstdint>
#include <cstddef>

/**
 * Helper wrappers to trigger FSM events from subsystem callbacks.
 * These functions bridge NFC, BLE, and other components to the FSM.
 */

namespace FSMIntegration {

// ===== NFC Event Wrappers =====
namespace NFC {
  /**
   * Called when NFC card is detected (UID read)
   * @param uid 4-byte card UID
   */
  void onCardDetected(const uint8_t uid[4]);
  
  /**
   * Called when NFC card is removed (lost communication)
   */
  void onCardRemoved();
  
  /**
   * Called when SELECT AID succeeds
   */
  void onSelectAIDSuccess();
  
  /**
   * Called when SELECT AID fails after retries
   */
  void onSelectAIDFailed();
  
  /**
   * Called when key exchange completes successfully
   * @param phone_pub_key 65-byte uncompressed ECDSA public key from phone
   */
  void onKeysExchanged(const uint8_t phone_pub_key[65]);
  
  /**
   * Called when received keys are invalid (format or signature check failed)
   */
  void onKeysInvalid();
  
  /**
   * Called when credentials are stored in NVS
   */
  void onCredentialsStored();
  
  /**
   * Called when NFC timeout occurs (e.g., waiting for card tap)
   */
  void onTimeout();
  
  /**
   * Called when NFC error occurs
   * @param errorCode Error code from ErrorCode enum
   */
  void onError(FSM::ErrorCode errorCode);
}

// ===== BLE Event Wrappers =====
namespace BLE {
  /**
   * Called when BLE client connects
   * @param clientAddr 6-byte BLE address of connected client
   */
  void onClientConnected(const uint8_t clientAddr[6]);
  
  /**
   * Called when BLE client disconnects
   */
  void onClientDisconnected();

  // CCC tunnel step wrappers
  void onAuth0Received();
  void onAuth0ResponseSent();
  void onAuth1Sent();
  void onAuth1ResponseReceived();
  void onExchangeReceived();
  void onExchangeResponseSent();
  void onControlFlowReceived();
  void onControlFlowResponseSent();
  
  /**
   * Called when CLIENT_HELLO is received (handshake init)
   * @param client_ephemeral_pub 65-byte ephemeral public key from phone
   */
  void onClientHelloReceived(const uint8_t client_ephemeral_pub[65]);
  
  /**
   * Called when SERVER_HELLO is sent (handshake response)
   * @param ecu_ephemeral_pub 65-byte ephemeral public key from ECU
   */
  void onServerHelloSent(const uint8_t ecu_ephemeral_pub[65]);
  
  /**
   * Called when authentication is verified (signature + session keys ready)
   */
  void onAuthVerified();
  
  /**
   * Called when authentication fails (signature mismatch)
   */
  void onAuthFailed();
  
  /**
   * Called when session expires (timeout or explicit invalidation)
   */
  void onSessionExpired();
  
  /**
   * Called when unlock is requested via BLE
   */
  void onUnlockRequested();
  
  /**
   * Called when BLE error occurs
   * @param errorCode Error code from ErrorCode enum
   */
  void onError(FSM::ErrorCode errorCode);
  
  /**
   * Called when admin command is received
   */
  void onAdminCommand();
}

// ===== Unlock Flow Event Wrappers =====
namespace Unlock {
  /**
   * Called when proximity check passes (RSSI/distance OK)
   */
  void onProximityOK();
  
  /**
   * Called when proximity check fails (phone too far)
   */
  void onProximityTooFar();
  
  /**
   * Called when session validation passes
   */
  void onSessionValid();
  
  /**
   * Called when unlock action is executed (relay activated)
   */
  void onUnlockExecuted();
  
  /**
   * Called when unlock timeout occurs
   */
  void onTimeout();
  
  /**
   * Called when unlock error occurs
   * @param errorCode Error code from ErrorCode enum
   */
  void onError(FSM::ErrorCode errorCode);
}

// ===== Admin Mode Event Wrappers =====
namespace Admin {
  /**
   * Called when "force provision on" is requested
   */
  void onForceProvisionOn();
  
  /**
   * Called when "force provision off" is requested
   */
  void onForceProvisionOff();
  
  /**
   * Called when "clear keys" is requested
   */
  void onClearKeys();
  
  /**
   * Called when diagnostics are requested
   */
  void onDiagnosticsRequest();
}

// ===== System Event Wrappers =====
namespace System {
  /**
   * Called when system is ready (after init)
   */
  void onSystemReady();
  
  /**
   * Called when reset is requested
   */
  void onResetRequested();
  
  /**
   * Called when generic error occurs
   * @param errorCode Error code from ErrorCode enum
   */
  void onError(FSM::ErrorCode errorCode);
}

// ===== Serial Command Wrappers =====
namespace SerialCmd {
  void startProvisioning();
  void toggleForceProvision();
  void armOneShotForce();
  void clearKeys();
  void clearAll();
  void printDiagnostics();
  void printFSMStatus();
}

// ===== Helper Functions =====
/**
 * Store phone public key in FSM context
 * @param key 65-byte uncompressed ECDSA public key
 */
void storePhonePublicKey(const uint8_t key[65]);

/**
 * Store phone ephemeral public key in FSM context
 * @param key 65-byte uncompressed ECDH ephemeral public key
 */
void storePhoneEphemeralKey(const uint8_t key[65]);

/**
 * Store ECU ephemeral public key in FSM context
 * @param key 65-byte uncompressed ECDH ephemeral public key
 */
void storeECUEphemeralKey(const uint8_t key[65]);

/**
 * Store shared secret in FSM context
 * @param secret 32-byte ECDH shared secret
 */
void storeSharedSecret(const uint8_t secret[32]);

/**
 * Store session keys in FSM context
 * @param enc_key 32-byte AES-256 encryption key
 * @param mac_key 32-byte HMAC key
 */
void storeSessionKeys(const uint8_t enc_key[32], const uint8_t mac_key[32]);

/**
 * Mark session keys as ready
 */
void markSessionReady();

/**
 * Mark phone key as valid
 */
void markPhoneKeyValid();

/**
 * Store NFC UID in FSM context
 * @param uid 4-byte card UID
 */
void storeNFCUID(const uint8_t uid[4]);

/**
 * Increment retry counter
 */
void incrementRetry();

/**
 * Reset retry counter
 */
void resetRetry();

/**
 * Store last error in FSM context
 * @param error ErrorCode
 */
void storeLastError(FSM::ErrorCode error);

} // namespace FSMIntegration

#endif // FSM_INTEGRATION_H