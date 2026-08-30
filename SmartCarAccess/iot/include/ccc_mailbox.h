#pragma once
#include <cstddef>
#include <cstdint>

namespace CCCMailbox {

struct __attribute__((packed)) CCC_Slot {
  uint8_t endpoint_pub[65];         // The phone's public key (ep_PK)
  uint8_t immobilizer_token[32];     // The root of trust for UWB (tok_n)
};

struct __attribute__((packed)) CCC_Mailbox {
  // 1. Identity Assets
  char vehicle_id[9];
  uint8_t vehicle_pub[65];
  uint8_t vehicle_priv[32];            // Keep this highly protected!

  // 2. State & Signaling
  uint16_t signaling_bitmap;
  uint8_t slot_bitmap;                // Bit 0 = Owner(Slot 0), Bits 1-7 = Friends

  // 3. User Data (Owner + 7 Friends)
  CCC_Slot slots[8];

  // 4. Core Validity Flags
  bool vehicle_identity_valid;
};

bool begin();
const CCC_Mailbox& get();

const char* vehicleId();
bool hasVehiclePub();
bool getVehiclePub(uint8_t* out, size_t max);
bool hasVehiclePriv();
bool signVehicleDataP256(const uint8_t* data, size_t dataLen,
                         uint8_t* sigDerOut, size_t sigDerMax,
                         size_t* sigDerLenOut);

bool hasEndpointPub(uint8_t slotIndex = 0);
bool getEndpointPub(uint8_t* out, size_t max, uint8_t slotIndex = 0);
bool setEndpointPub(const uint8_t* pub65, uint8_t slotIndex = 0);
bool clearEndpointPub(uint8_t slotIndex = 0);

bool isSlotActive(uint8_t slotIndex);
bool setSlotActive(uint8_t slotIndex, bool active);

bool hasToken(uint8_t slotIndex);
bool getToken(uint8_t slotIndex, uint8_t out[32]);
bool setToken(uint8_t slotIndex, const uint8_t tok[32]);
bool clearToken(uint8_t slotIndex);
bool ensureToken(uint8_t slotIndex);

uint16_t signalingBitmap();
bool setSignalingBitmap(uint16_t bitmap);
bool setSignalingFlag(uint16_t mask, bool enabled);

// Clear only the mailboxes (endpoint pub, signaling, slot bitmap, tokens).
void clearMailboxes();
// Clear all CCC mailbox data, including vehicle identity.
void clearAll();

} // namespace CCCMailbox
