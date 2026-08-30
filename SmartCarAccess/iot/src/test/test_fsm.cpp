/**
 * Unit tests for FSM transition logic
 * 
 * Tests cover:
 * - State transitions with and without guards
 * - Event queue operations (FIFO)
 * - Timeout management
 * - Entry/exit actions
 * - Error handling
 * - Context management
 */

#include <Arduino.h>
#include "fsm/fsm.h"
#include "fsm/fsm_integration.h"

using namespace FSM;  // Use FSM types (State, Event, ErrorCode, StateContext)

// Test counters
static int test_passed = 0;
static int test_failed = 0;

// Test assertion macro
#define TEST_ASSERT(condition, message) \
  do { \
    if (condition) { \
      Serial.printf("✓ PASS: %s\n", message); \
      test_passed++; \
    } else { \
      Serial.printf("✗ FAIL: %s\n", message); \
      test_failed++; \
    } \
  } while(0)

#define TEST_ASSERT_EQ(actual, expected, message) \
  do { \
    if ((actual) == (expected)) { \
      Serial.printf("✓ PASS: %s (expected: %d, got: %d)\n", message, (int)expected, (int)actual); \
      test_passed++; \
    } else { \
      Serial.printf("✗ FAIL: %s (expected: %d, got: %d)\n", message, (int)expected, (int)actual); \
      test_failed++; \
    } \
  } while(0)

// Test helper: Wait for FSM to process events
void processEvents() {
  for (int i = 0; i < 10; i++) {
    FSM::tick();
    delay(10);
  }
}

// Test 1: Initial state after begin()
void test_initial_state() {
  Serial.println("\n=== Test 1: Initial State ===");
  
  FSM::begin();
  processEvents();
  
  // Should transition from INIT → IDLE automatically
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, "Initial state should be IDLE after SYSTEM_READY");
}

// Test 2: Basic transition (IDLE → PROVISIONING)
void test_basic_transition() {
  Serial.println("\n=== Test 2: Basic Transition ===");
  
  FSM::reset(false);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, "State should be IDLE after reset");
  
  FSM::triggerEvent(Event::PROVISION_START);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_WAIT_TAP, 
                "Should transition to PROVISIONING_WAIT_TAP on PROVISION_START");
}

// Test 3: Guarded transition (requires provisioned)
void test_guarded_transition() {
  Serial.println("\n=== Test 3: Guarded Transition ===");
  
  FSM::reset(false);
  processEvents();
  
  // Try BLE connect without provisioning - should not transition
  FSM::triggerEvent(Event::BLE_CLIENT_CONNECTED);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, 
                "Should stay in IDLE when connecting without provisioning (guard failed)");
  
  // TODO: Provision device, then try again - should succeed
}

// Test 4: Event queue FIFO ordering
void test_event_queue() {
  Serial.println("\n=== Test 4: Event Queue ===");
  
  FSM::reset(false);
  FSM::clearEventQueue();
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getEventQueueSize(), 0, "Queue should be empty after clear");
  
  // Queue multiple events
  FSM::triggerEvent(Event::PROVISION_START);
  FSM::triggerEvent(Event::NFC_CARD_DETECTED);
  FSM::triggerEvent(Event::SELECT_AID_SUCCESS);
  
  size_t qsize = FSM::getEventQueueSize();
  TEST_ASSERT_EQ(qsize, 3, "Queue should have 3 events");
  
  // Process and verify FIFO order
  processEvents();
  
  // After processing PROVISION_START → should be in PROVISIONING_WAIT_TAP
  // After NFC_CARD_DETECTED → should be in PROVISIONING_SELECT_AID
  // After SELECT_AID_SUCCESS → should be in PROVISIONING_EXCHANGE_KEYS
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_EXCHANGE_KEYS,
                "Should process events in FIFO order");
}

// Test 5: Timeout functionality
void test_timeout() {
  Serial.println("\n=== Test 5: Timeout ===");
  
  FSM::reset(false);
  processEvents();
  
  FSM::triggerEvent(Event::PROVISION_START);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_WAIT_TAP, "Should be in PROVISIONING_WAIT_TAP");
  
  // Set 2 second timeout
  FSM::setStateTimeout(2000);
  
  // Wait for timeout to trigger
  delay(2500);
  processEvents();
  
  // Should transition back to IDLE on timeout
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, "Should return to IDLE after timeout");
}

// Test 6: Entry/Exit actions
static bool entry_called = false;
static bool exit_called = false;

void test_entry_action(StateContext& ctx) {
  entry_called = true;
  Serial.println("  → Entry action called");
}

void test_exit_action(StateContext& ctx) {
  exit_called = true;
  Serial.println("  → Exit action called");
}

void test_entry_exit_actions() {
  Serial.println("\n=== Test 6: Entry/Exit Actions ===");
  
  FSM::reset(false);
  processEvents();
  
  entry_called = false;
  exit_called = false;
  
  FSM::onStateEntry(State::PROVISIONING_WAIT_TAP, test_entry_action);
  FSM::onStateExit(State::PROVISIONING_WAIT_TAP, test_exit_action);
  
  FSM::triggerEvent(Event::PROVISION_START);
  processEvents();
  
  TEST_ASSERT(entry_called, "Entry action should be called on state entry");
  
  FSM::triggerEvent(Event::NFC_CARD_REMOVED);
  processEvents();
  
  TEST_ASSERT(exit_called, "Exit action should be called on state exit");
}

// Test 7: Context data storage
void test_context_storage() {
  Serial.println("\n=== Test 7: Context Storage ===");
  
  FSM::reset(true);  // Clear context
  processEvents();
  
  const StateContext& ctx = FSM::getContext();
  
  // Initially should be all zeros
  TEST_ASSERT(!ctx.phone_key_valid, "Phone key should be invalid initially");
  TEST_ASSERT(!ctx.session_keys_ready, "Session keys should not be ready initially");
  TEST_ASSERT_EQ(ctx.retry_count, 0, "Retry count should be 0");
  
  // Store test data
  uint8_t test_uid[4] = {0x11, 0x22, 0x33, 0x44};
  FSMIntegration::storeNFCUID(test_uid);
  
  TEST_ASSERT_EQ(ctx.nfc_uid[0], 0x11, "NFC UID should be stored");
  TEST_ASSERT_EQ(ctx.nfc_uid[3], 0x44, "NFC UID bytes should match");
  
  FSMIntegration::markPhoneKeyValid();
  TEST_ASSERT(ctx.phone_key_valid, "Phone key valid flag should be set");
  
  FSMIntegration::incrementRetry();
  TEST_ASSERT_EQ(ctx.retry_count, 1, "Retry count should increment");
  
  FSMIntegration::resetRetry();
  TEST_ASSERT_EQ(ctx.retry_count, 0, "Retry count should reset");
}

// Test 8: Error handling
void test_error_handling() {
  Serial.println("\n=== Test 8: Error Handling ===");
  
  FSM::reset(false);
  processEvents();
  
  FSM::triggerEvent(Event::PROVISION_START);
  FSM::triggerEvent(Event::NFC_CARD_DETECTED);
  FSM::triggerEvent(Event::SELECT_AID_SUCCESS);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_EXCHANGE_KEYS, "Should be in EXCHANGE_KEYS");
  
  // Trigger error
  FSMIntegration::NFC::onError(ErrorCode::NFC_APDU_FAILED);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::ERROR_HANDLER, "Should transition to ERROR_HANDLER");
  
  const StateContext& ctx = FSM::getContext();
  TEST_ASSERT_EQ(ctx.last_error, ErrorCode::NFC_APDU_FAILED, "Error code should be stored");
  TEST_ASSERT_EQ(ctx.error_count, 1, "Error count should increment");
  
  // Reset from error
  FSM::triggerEvent(Event::RESET_REQUESTED);
  processEvents();
  
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, "Should return to IDLE after reset");
}

// Test 9: Full provisioning flow
void test_provisioning_flow() {
  Serial.println("\n=== Test 9: Full Provisioning Flow ===");
  
  FSM::reset(true);
  processEvents();
  
  uint8_t test_uid[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  uint8_t test_key[65];
  test_key[0] = 0x04;  // Uncompressed format
  for (int i = 1; i < 65; i++) test_key[i] = i;
  
  // Start provisioning
  FSM::triggerEvent(Event::PROVISION_START);
  processEvents();
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_WAIT_TAP, "Step 1: WAIT_TAP");
  
  // Card detected
  FSMIntegration::NFC::onCardDetected(test_uid);
  processEvents();
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_SELECT_AID, "Step 2: SELECT_AID");
  
  // AID selected
  FSMIntegration::NFC::onSelectAIDSuccess();
  processEvents();
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_EXCHANGE_KEYS, "Step 3: EXCHANGE_KEYS");
  
  // Keys exchanged
  FSMIntegration::NFC::onKeysExchanged(test_key);
  processEvents();
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::PROVISIONING_STORE_CREDS, "Step 4: STORE_CREDS");
  
  // Stored
  FSMIntegration::NFC::onCredentialsStored();
  processEvents();
  TEST_ASSERT_EQ(FSM::getCurrentState(), State::IDLE, "Step 5: Back to IDLE");
  
  const StateContext& ctx = FSM::getContext();
  TEST_ASSERT(ctx.phone_key_valid, "Phone key should be valid after provisioning");
  TEST_ASSERT_EQ(ctx.retry_count, 0, "Retry count should be reset after success");
}

// Test 10: Event queue overflow
void test_event_queue_overflow() {
  Serial.println("\n=== Test 10: Event Queue Overflow ===");
  
  FSM::clearEventQueue();
  
  // Fill queue (16 slots)
  bool success = true;
  for (int i = 0; i < 16; i++) {
    success = FSM::triggerEvent(Event::SYSTEM_READY);
    if (!success) break;
  }
  
  TEST_ASSERT(success, "Should successfully queue 16 events");
  TEST_ASSERT(FSM::isEventQueueFull(), "Queue should be full");
  
  // Try to add one more - should fail
  success = FSM::triggerEvent(Event::SYSTEM_READY);
  TEST_ASSERT(!success, "Should fail to queue when full");
  
  FSM::clearEventQueue();
  TEST_ASSERT_EQ(FSM::getEventQueueSize(), 0, "Queue should be empty after clear");
}

// Test runner
void run_fsm_tests() {
  Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║           FSM UNIT TEST SUITE                             ║");
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
  
  test_passed = 0;
  test_failed = 0;
  
  test_initial_state();
  test_basic_transition();
  test_guarded_transition();
  test_event_queue();
  test_timeout();
  test_entry_exit_actions();
  test_context_storage();
  test_error_handling();
  test_provisioning_flow();
  test_event_queue_overflow();
  
  Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║           TEST RESULTS                                    ║");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.printf("║ PASSED: %-49d ║\n", test_passed);
  Serial.printf("║ FAILED: %-49d ║\n", test_failed);
  Serial.printf("║ TOTAL:  %-49d ║\n", test_passed + test_failed);
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
  
  if (test_failed == 0) {
    Serial.println("✓✓✓ ALL TESTS PASSED ✓✓✓");
  } else {
    Serial.printf("⚠️  %d TEST(S) FAILED\n", test_failed);
  }
}
