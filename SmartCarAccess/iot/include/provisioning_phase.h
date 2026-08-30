#pragma once
#include <Arduino.h>

class PN532; // forward decl

namespace ProvisioningPhase {
  // Initialize provisioning and CCC mailbox bindings.
  void begin();

  // True if a phone public key has already been stored.
  bool isProvisioned();

  // Persist phone long-term public key (raw uncompressed 65 bytes: 0x04||X||Y)
  bool storePhonePubRaw(const uint8_t* pub65);
  // Persist optional certificate chain blob
  bool storeCertChain(const uint8_t* cert, size_t certLen);

  // Persist versioned fast-path artifact generated during provisioning.
  bool storeFastArtifact(uint8_t version, const uint8_t* key32);
  bool hasFastArtifact();
  bool getFastArtifact(uint8_t* outVersion, uint8_t outKey32[32]);
  bool clearFastArtifact();

  // Owner provisioning helper: store phone pub, set slot 0, and generate tok_0.
  bool setOwnerProvisioned(const uint8_t* pub65, bool force);

  // Verify ECDSA-P256 DER signature over data using uncompressed 65-byte public key
  bool verifySignatureP256(const uint8_t* pub65,
                           const uint8_t* data, size_t dataLen,
                           const uint8_t* sigDer, size_t sigLen);

  // Clear all provisioning-related state (mailboxes, cert, flags)
  void clearAll();
  // Clear only phone provisioning artifacts (endpoint pub, cert), keep vehicle identity
  void clearProvisionedOnly();
  // Alias for clearProvisionedOnly (for FSM compatibility)
  void clearProvisionedData();

  // Force provisioning control (for testing and admin operations)
  bool setForceProvisioningFlag(bool enable);  // Set persistent force provisioning flag
  bool isForceProvisioning();                  // Check if force provisioning is enabled
  void setOneShotForce(bool enable);           // Enable one-time force provisioning (cleared after use)

  // Read-back helpers for diagnostics
  // Returns number of bytes copied (0 if missing). Expects max>=65 for full key.
  size_t getPhonePubRaw(uint8_t* out, size_t max);
  // Returns cert chain length (0 if missing). If 'out' provided and max>=len, copies into out.
  size_t getCertChain(uint8_t* out, size_t max);

  // Validate that a provided or stored certificate's subject public key matches the 65-byte pubkey
  bool validateCertPublicKeyMatchesPub(const uint8_t* cert, size_t certLen, const uint8_t* pub65);
  bool validateStoredCertMatchesStoredPub();

  // Deprecated: provisioning via HCE helper is not supported under CCC flow.
  bool runOnceWithHce(PN532& nfc, const uint8_t* aid, size_t aidLen, uint32_t waitMs = 15000);
  
  // Deprecated legacy API.
  size_t getDevicePrivateKeyPEM(uint8_t* out, size_t maxLen);
}
