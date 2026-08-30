#include "fsm/fsm.h"
#include "provisioning_phase.h"
#include "ble/ble_auth.h"
#include "nfc_session.h"
#include <Arduino.h>

namespace FSM {

// Private data structures
static State currentState = State::INIT;
static State previousState = State::INIT;
static StateContext context;
static StateInfo info;
static bool debugLogging = true;

// Event queue
#define EVENT_QUEUE_SIZE 16
struct QueuedEvent {
  Event event;
  uint32_t timestamp;
  void* data;
  size_t data_len;
};
static QueuedEvent eventQueue[EVENT_QUEUE_SIZE];
static size_t queueHead = 0;
static size_t queueTail = 0;

// Timeout
struct TimeoutState {
  bool active;
  uint32_t start_ms;
  uint32_t duration_ms;
} currentTimeout = {false, 0, 0};

// Action callbacks
#define MAX_STATES 16
static StateAction entryActions[MAX_STATES] = {nullptr};
static StateAction exitActions[MAX_STATES] = {nullptr};

// Forward declarations
static void processEvent(Event event);
static bool canTransition(State from, Event event, State to);
static void executeTransition(State newState, Event triggerEvent);
static void checkTimeout();
static void cleanupEventData(void* data, size_t len);

// Guard functions
static bool guardProvisioned(const StateContext& ctx) {
  return ProvisioningPhase::isProvisioned();
}

static bool guardKeysValid(const StateContext& ctx) {
  return ctx.phone_key_valid && ctx.phone_pub_key[0] == 0x04;
}

static bool guardSessionReady(const StateContext& ctx) {
  return ctx.session_keys_ready;
}

// State transition table
const struct {
  State from;
  Event on;
  State to;
  bool (*guard)(const StateContext&);
} transitionTable[] = {
  // INIT → IDLE on system ready
  {State::INIT, Event::SYSTEM_READY, State::IDLE, nullptr},
  
  // IDLE → PROVISIONING on provision start
  {State::IDLE, Event::PROVISION_START, State::PROVISIONING_WAIT_TAP, nullptr},
  
  // IDLE → AUTH on BLE connect (if provisioned)
  {State::IDLE, Event::BLE_CLIENT_CONNECTED, State::AUTH_WAIT_AUTH0, guardProvisioned},

  // Accept credentials stored while idle (NFC flow can run outside FSM)
  {State::IDLE, Event::CREDENTIALS_STORED, State::IDLE, nullptr},
  
  // PROVISIONING flow
  {State::PROVISIONING_WAIT_TAP, Event::NFC_CARD_DETECTED, State::PROVISIONING_SELECT_AID, nullptr},
  {State::PROVISIONING_WAIT_TAP, Event::TIMEOUT, State::IDLE, nullptr},
  {State::PROVISIONING_WAIT_TAP, Event::NFC_CARD_REMOVED, State::IDLE, nullptr},
  
  {State::PROVISIONING_SELECT_AID, Event::SELECT_AID_SUCCESS, State::PROVISIONING_EXCHANGE_KEYS, nullptr},
  {State::PROVISIONING_SELECT_AID, Event::SELECT_AID_FAILED, State::ERROR_HANDLER, nullptr},
  {State::PROVISIONING_SELECT_AID, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},
  
  {State::PROVISIONING_EXCHANGE_KEYS, Event::KEYS_EXCHANGED, State::PROVISIONING_STORE_CREDS, guardKeysValid},
  {State::PROVISIONING_EXCHANGE_KEYS, Event::KEYS_INVALID, State::ERROR_HANDLER, nullptr},
  {State::PROVISIONING_EXCHANGE_KEYS, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},
  
  {State::PROVISIONING_STORE_CREDS, Event::CREDENTIALS_STORED, State::IDLE, nullptr},
  {State::PROVISIONING_STORE_CREDS, Event::ERROR_OCCURRED, State::ERROR_HANDLER, nullptr},
  
  // AUTH flow (CCC tunnel)
  {State::AUTH_WAIT_AUTH0, Event::BLE_AUTH0_RECEIVED, State::AUTH_PROCESSING_AUTH0_STD, nullptr},
  {State::AUTH_WAIT_AUTH0, Event::TIMEOUT, State::IDLE, nullptr},
  {State::AUTH_WAIT_AUTH0, Event::BLE_CLIENT_DISCONNECTED, State::IDLE, nullptr},

  {State::AUTH_PROCESSING_AUTH0_STD, Event::BLE_AUTH0_RESP_SENT, State::AUTH_WAIT_AUTH1_RESP, nullptr},
  {State::AUTH_PROCESSING_AUTH0_STD, Event::AUTH_FAILED, State::ERROR_HANDLER, nullptr},
  {State::AUTH_PROCESSING_AUTH0_STD, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},
  {State::AUTH_PROCESSING_AUTH0_STD, Event::BLE_CLIENT_DISCONNECTED, State::IDLE, nullptr},

  {State::AUTH_WAIT_AUTH1_RESP, Event::BLE_AUTH1_RESP_RECEIVED, State::AUTH_VERIFY_KEYS, nullptr},
  {State::AUTH_WAIT_AUTH1_RESP, Event::BLE_AUTH1_SENT, State::AUTH_WAIT_AUTH1_RESP, nullptr},
  {State::AUTH_WAIT_AUTH1_RESP, Event::AUTH_VERIFIED, State::AUTH_SECURE_CHANNEL_READY, guardSessionReady},
  {State::AUTH_WAIT_AUTH1_RESP, Event::AUTH_FAILED, State::ERROR_HANDLER, nullptr},
  {State::AUTH_WAIT_AUTH1_RESP, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},
  {State::AUTH_WAIT_AUTH1_RESP, Event::BLE_CLIENT_DISCONNECTED, State::IDLE, nullptr},

  // keep legacy VERIFY_KEYS gate but land in new secure-ready state
  {State::AUTH_VERIFY_KEYS, Event::BLE_EXCHANGE_RECEIVED, State::AUTH_VERIFY_KEYS, nullptr},
  {State::AUTH_VERIFY_KEYS, Event::AUTH_VERIFIED, State::AUTH_SECURE_CHANNEL_READY, guardSessionReady},
  {State::AUTH_VERIFY_KEYS, Event::AUTH_FAILED, State::ERROR_HANDLER, nullptr},
  {State::AUTH_VERIFY_KEYS, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},

  {State::AUTH_SECURE_CHANNEL_READY, Event::BLE_EXCHANGE_RECEIVED, State::AUTH_SECURE_CHANNEL_READY, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::BLE_EXCHANGE_RESP_SENT, State::AUTH_SECURE_CHANNEL_READY, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::BLE_CONTROL_FLOW_RECEIVED, State::AUTH_SECURE_CHANNEL_READY, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::BLE_CONTROL_FLOW_RESP_SENT, State::AUTH_SECURE_CHANNEL_READY, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::UNLOCK_REQUESTED, State::UNLOCKING_CHECK_PROXIMITY, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::BLE_CLIENT_DISCONNECTED, State::IDLE, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::TIMEOUT, State::IDLE, nullptr},
  {State::AUTH_SECURE_CHANNEL_READY, Event::AUTH_SESSION_EXPIRED, State::IDLE, nullptr},

  // Legacy session-ready path kept during migration
  {State::AUTH_SESSION_READY, Event::UNLOCK_REQUESTED, State::UNLOCKING_CHECK_PROXIMITY, nullptr},
  {State::AUTH_SESSION_READY, Event::BLE_CLIENT_DISCONNECTED, State::IDLE, nullptr},
  {State::AUTH_SESSION_READY, Event::TIMEOUT, State::IDLE, nullptr},
  {State::AUTH_SESSION_READY, Event::AUTH_SESSION_EXPIRED, State::IDLE, nullptr},
  
  // UNLOCKING flow
  {State::UNLOCKING_CHECK_PROXIMITY, Event::PROXIMITY_OK, State::UNLOCKING_VERIFY_AUTH, nullptr},
  {State::UNLOCKING_CHECK_PROXIMITY, Event::BLE_CONTROL_FLOW_RESP_SENT, State::UNLOCKING_CHECK_PROXIMITY, nullptr},
  {State::UNLOCKING_CHECK_PROXIMITY, Event::PROXIMITY_TOO_FAR, State::AUTH_SESSION_READY, nullptr},
  {State::UNLOCKING_CHECK_PROXIMITY, Event::TIMEOUT, State::AUTH_SESSION_READY, nullptr},
  
  {State::UNLOCKING_VERIFY_AUTH, Event::AUTH_SESSION_VALID, State::UNLOCKING_EXECUTE, nullptr},
  {State::UNLOCKING_VERIFY_AUTH, Event::AUTH_SESSION_EXPIRED, State::ERROR_HANDLER, nullptr},
  {State::UNLOCKING_VERIFY_AUTH, Event::TIMEOUT, State::ERROR_HANDLER, nullptr},
  
  {State::UNLOCKING_EXECUTE, Event::UNLOCK_EXECUTED, State::UNLOCKING_COMPLETE, nullptr},
  {State::UNLOCKING_EXECUTE, Event::ERROR_OCCURRED, State::ERROR_HANDLER, nullptr},
  
  {State::UNLOCKING_COMPLETE, Event::TIMEOUT, State::IDLE, nullptr},
  
  // ADMIN mode
  {State::IDLE, Event::ADMIN_COMMAND, State::ADMIN_MODE, nullptr},
  {State::ADMIN_MODE, Event::FORCE_PROVISION_ON, State::IDLE, nullptr},
  {State::ADMIN_MODE, Event::FORCE_PROVISION_OFF, State::IDLE, nullptr},
  {State::ADMIN_MODE, Event::CLEAR_KEYS, State::IDLE, nullptr},
  {State::ADMIN_MODE, Event::DIAGNOSTICS_REQUEST, State::IDLE, nullptr},
  {State::ADMIN_MODE, Event::RESET_REQUESTED, State::IDLE, nullptr},
  
  // ERROR_HANDLER
  {State::ERROR_HANDLER, Event::RESET_REQUESTED, State::IDLE, nullptr},
  {State::ERROR_HANDLER, Event::TIMEOUT, State::IDLE, nullptr},
  
  // Global error transitions from any state
  // (handled specially in processEvent)
};

void begin() {
  Serial.println("[FSM] ╔═══════════════════════════════════════╗");
  Serial.println("[FSM] ║   Smart Car Access FSM v1.0         ║");
  Serial.println("[FSM] ╚═══════════════════════════════════════╝");
  Serial.println("[FSM] Initializing...");
  
  currentState = State::INIT;
  previousState = State::INIT;
  context.reset();
  info.current = State::INIT;
  info.previous = State::INIT;
  info.lastEvent = Event::SYSTEM_READY;
  info.lastError = ErrorCode::NONE;
  info.enter_time = millis();
  info.transition_count = 0;
  
  // Clear event queue
  queueHead = 0;
  queueTail = 0;
  
  // Initialize subsystems (already done in main.cpp, but verify)
  // NimBLEDevice should already be initialized
  // NFC should already be initialized
  
  // Check if already provisioned and update context
  if (ProvisioningPhase::isProvisioned()) {
    uint8_t pub[65];
    size_t pubLen = ProvisioningPhase::getPhonePubRaw(pub, sizeof(pub));
    if (pubLen == 65 && pub[0] == 0x04) {
      context.phone_key_valid = true;
      Serial.println("[FSM] ✓ Loaded existing provisioning data from NVS");
    }
  }
  
  Serial.println("[FSM] ✓ FSM Core initialized");
  Serial.printf("[FSM] ✓ Event queue: %d slots\n", EVENT_QUEUE_SIZE);
  Serial.printf("[FSM] ✓ Max states: %d\n", MAX_STATES);
  Serial.printf("[FSM] ✓ Transition table: %d entries\n", sizeof(transitionTable) / sizeof(transitionTable[0]));
  
  // Trigger SYSTEM_READY after init
  delay(100);
  triggerEvent(Event::SYSTEM_READY);
  
  Serial.println("[FSM] ✓ System ready - FSM operational");
}

void tick() {
  // Check timeout
  checkTimeout();
  
  // Process queued events (FIFO)
  while (queueTail != queueHead) {
    QueuedEvent& evt = eventQueue[queueTail];
    processEvent(evt.event);
    
    // Cleanup data if any
    if (evt.data != nullptr && evt.data_len > 0) {
      cleanupEventData(evt.data, evt.data_len);
      evt.data = nullptr;
      evt.data_len = 0;
    }
    
    queueTail = (queueTail + 1) % EVENT_QUEUE_SIZE;
  }
  
  // State-specific background work
  switch (currentState) {
    case State::IDLE: {
      // Periodically sync phone_key_valid with NVS (every 5 seconds)
      static uint32_t lastSync = 0;
      if (millis() - lastSync > 5000) {
        bool provisioned = ProvisioningPhase::isProvisioned();
        bool hasValidKey = false;
        if (provisioned) {
          uint8_t pub[65];
          size_t pubLen = ProvisioningPhase::getPhonePubRaw(pub, sizeof(pub));
          hasValidKey = (pubLen == 65 && pub[0] == 0x04);
        }
        
        // Update context if mismatch detected
        if (context.phone_key_valid != hasValidKey) {
          context.phone_key_valid = hasValidKey;
          if (debugLogging) {
            Serial.printf("[FSM] ✓ Synced phone_key_valid: %s\n", hasValidKey ? "YES" : "NO");
          }
        }
        lastSync = millis();
      }
      break;
    }
      
    case State::PROVISIONING_WAIT_TAP:
      // NFCSession polls in its own loop
      break;
      
    case State::AUTH_WAIT_AUTH0:
    case State::AUTH_WAIT_CONNECT:
      // BLE advertising should already be active
      break;
      
    case State::AUTH_SECURE_CHANNEL_READY:
    case State::AUTH_SESSION_READY:
      // Monitor session timeout
      if (context.session_keys_ready) {
        uint32_t elapsed = millis() - context.last_activity_ms;
        if (elapsed > 300000) {  // 5 minutes session timeout
          triggerEvent(Event::AUTH_SESSION_EXPIRED);
        }
      }
      break;
      
    default:
      break;
  }
}

bool triggerEvent(Event event) {
  size_t next = (queueHead + 1) % EVENT_QUEUE_SIZE;
  if (next == queueTail) {
    Serial.println("[FSM] ⚠️  ERROR: Event queue full!");
    return false;
  }
  
  eventQueue[queueHead] = {event, millis(), nullptr, 0};
  queueHead = next;
  
  if (debugLogging) {
    // size_t queueSize = (queueHead - queueTail + EVENT_QUEUE_SIZE) % EVENT_QUEUE_SIZE;
    Serial.printf("[FSM] ▶ Event queued: %s (queue: %u/%u)\n", 
                 eventToString(event), getEventQueueSize(), EVENT_QUEUE_SIZE);
  }
  return true;
}

bool triggerEventWithData(Event event, void* data, size_t data_len) {
  size_t next = (queueHead + 1) % EVENT_QUEUE_SIZE;
  if (next == queueTail) {
    Serial.println("[FSM] ⚠️  ERROR: Event queue full!");
    return false;
  }
  
  // Copy data (caller must ensure data is valid until processed)
  void* dataCopy = nullptr;
  if (data != nullptr && data_len > 0) {
    dataCopy = malloc(data_len);
    if (dataCopy == nullptr) {
      Serial.println("[FSM] ⚠️  ERROR: Failed to allocate event data!");
      return false;
    }
    memcpy(dataCopy, data, data_len);
  }
  
  eventQueue[queueHead] = {event, millis(), dataCopy, data_len};
  queueHead = next;
  
  if (debugLogging) {
    Serial.printf("[FSM] ▶ Event queued (with data): %s [%u bytes]\n", 
                 eventToString(event), (unsigned)data_len);
  }
  return true;
}

static void cleanupEventData(void* data, size_t len) {
  if (data != nullptr) {
    free(data);
  }
}

static void processEvent(Event event) {
  if (debugLogging) {
    Serial.printf("[FSM] ⚙️  Processing: %s (state: %s)\n", 
                 eventToString(event), stateToString(currentState));
  }
  
  info.lastEvent = event;
  context.last_activity_ms = millis();
  
  // Sync phone_key_valid when credentials are stored
  if (event == Event::CREDENTIALS_STORED) {
    uint8_t pub[65];
    size_t pubLen = ProvisioningPhase::getPhonePubRaw(pub, sizeof(pub));
    if (pubLen == 65 && pub[0] == 0x04) {
      context.phone_key_valid = true;
      if (debugLogging) {
        Serial.println("[FSM] ✓ Phone key validated and synced to context");
      }
    }
  }
  
  // Sync phone_key_valid when keys are cleared
  if (event == Event::CLEAR_KEYS) {
    context.phone_key_valid = false;
    if (debugLogging) {
      Serial.println("[FSM] ✓ Phone key cleared from context");
    }
  }
  
  // Find matching transition
  State nextState = currentState;
  bool transitionFound = false;
  
  for (const auto& trans : transitionTable) {
    if (trans.from == currentState && trans.on == event) {
      // Check guard condition
      if (trans.guard == nullptr || trans.guard(context)) {
        nextState = trans.to;
        transitionFound = true;
        break;
      } else {
        if (debugLogging) {
          Serial.printf("[FSM] ✗ Transition guard failed: %s + %s → %s\n",
                       stateToString(trans.from), eventToString(trans.on), 
                       stateToString(trans.to));
        }
      }
    }
  }
  
  // Special handling for global events
  if (!transitionFound) {
    if (event == Event::ERROR_OCCURRED) {
      nextState = State::ERROR_HANDLER;
      transitionFound = true;
    } else if (event == Event::RESET_REQUESTED) {
      nextState = State::IDLE;
      transitionFound = true;
    }
  }
  
  if (transitionFound && nextState != currentState) {
    executeTransition(nextState, event);
  } else if (!transitionFound && debugLogging) {
    Serial.printf("[FSM] ⚠️  No transition: %s + %s\n", 
                 stateToString(currentState), eventToString(event));
  }
}

static void executeTransition(State newState, Event triggerEvent) {
  if (debugLogging) {
    Serial.printf("[FSM] ═══ Transition: %s → %s (event: %s) ═══\n",
                 stateToString(currentState), stateToString(newState),
                 eventToString(triggerEvent));
  }
  
  // Exit action
  if (exitActions[(int)currentState] != nullptr) {
    exitActions[(int)currentState](context);
  }
  
  // Update state
  previousState = currentState;
  currentState = newState;
  info.current = newState;
  info.previous = previousState;
  info.enter_time = millis();
  info.transition_count++;
  
  // Clear timeout on transition
  currentTimeout.active = false;
  
  // Entry action
  if (entryActions[(int)currentState] != nullptr) {
    entryActions[(int)currentState](context);
  }
}

static void checkTimeout() {
  if (!currentTimeout.active) return;
  
  uint32_t elapsed = millis() - currentTimeout.start_ms;
  if (elapsed >= currentTimeout.duration_ms) {
    if (debugLogging) {
      Serial.printf("[FSM] ⏰ Timeout after %u ms (state: %s)\n", 
                   elapsed, stateToString(currentState));
    }
    triggerEvent(Event::TIMEOUT);
    currentTimeout.active = false;
  }
}

// API implementations
State getCurrentState() { 
  return currentState; 
}

StateInfo getStateInfo() { 
  return info; 
}

const StateContext& getContext() { 
  return context; 
}

bool forceState(State newState, bool force) {
  if (!force && currentState == newState) {
    return false;
  }
  
  Serial.printf("[FSM] ⚠️  FORCED STATE CHANGE: %s → %s\n",
               stateToString(currentState), stateToString(newState));
  
  executeTransition(newState, Event::RESET_REQUESTED);
  return true;
}

void reset(bool clearSession) {
  Serial.println("[FSM] 🔄 Resetting FSM...");
  
  if (clearSession) {
    context.reset();
    Serial.println("[FSM]    ✓ Session context cleared");
  }
  
  clearEventQueue();
  currentTimeout.active = false;
  
  executeTransition(State::IDLE, Event::RESET_REQUESTED);
  
  Serial.println("[FSM] ✓ FSM reset complete");
}

void setStateTimeout(uint32_t timeout_ms) {
  currentTimeout.active = true;
  currentTimeout.start_ms = millis();
  currentTimeout.duration_ms = timeout_ms;
  
  if (debugLogging) {
    Serial.printf("[FSM] ⏱️  Timeout set: %u ms (state: %s)\n", 
                 timeout_ms, stateToString(currentState));
  }
}

void clearTimeout() {
  if (currentTimeout.active && debugLogging) {
    Serial.printf("[FSM] ⏱️  Timeout cleared (state: %s)\n", stateToString(currentState));
  }
  currentTimeout.active = false;
}

void onStateEntry(State state, StateAction action) {
  if ((int)state < MAX_STATES) {
    entryActions[(int)state] = action;
  }
}

void onStateExit(State state, StateAction action) {
  if ((int)state < MAX_STATES) {
    exitActions[(int)state] = action;
  }
}

size_t getEventQueueSize() {
  return (queueHead - queueTail + EVENT_QUEUE_SIZE) % EVENT_QUEUE_SIZE;
}

bool isEventQueueFull() {
  return ((queueHead + 1) % EVENT_QUEUE_SIZE) == queueTail;
}

void clearEventQueue() {
  // Cleanup any pending event data
  while (queueTail != queueHead) {
    QueuedEvent& evt = eventQueue[queueTail];
    if (evt.data != nullptr && evt.data_len > 0) {
      cleanupEventData(evt.data, evt.data_len);
      evt.data = nullptr;
      evt.data_len = 0;
    }
    queueTail = (queueTail + 1) % EVENT_QUEUE_SIZE;
  }
  
  queueHead = queueTail = 0;
  if (debugLogging) {
    Serial.println("[FSM] ✓ Event queue cleared");
  }
}

bool isProvisioning() {
  return currentState >= State::PROVISIONING_WAIT_TAP && 
         currentState <= State::PROVISIONING_STORE_CREDS;
}

bool isAuthenticating() {
  switch (currentState) {
    case State::AUTH_WAIT_AUTH0:
    case State::AUTH_PROCESSING_AUTH0_STD:
    case State::AUTH_WAIT_AUTH1_RESP:
    case State::AUTH_VERIFY_KEYS:
    case State::AUTH_SECURE_CHANNEL_READY:
    case State::AUTH_WAIT_CONNECT:
    case State::AUTH_HANDSHAKE:
    case State::AUTH_SESSION_READY:
      return true;
    default:
      return false;
  }
}

bool isUnlocking() {
  return currentState >= State::UNLOCKING_CHECK_PROXIMITY && 
         currentState <= State::UNLOCKING_COMPLETE;
}

bool isInErrorState() {
  return currentState == State::ERROR_HANDLER;
}

uint32_t getTimeInCurrentState() {
  return millis() - info.enter_time;
}

void setDebugLogging(bool enable) {
  debugLogging = enable;
  Serial.printf("[FSM] Debug logging: %s\n", enable ? "ENABLED" : "DISABLED");
}

void printStatus() {
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.println("║              FSM Status Report                           ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.printf("║ Current State: %-41s ║\n", stateToString(currentState));
  Serial.printf("║ Previous State: %-40s ║\n", stateToString(previousState));
  Serial.printf("║ Last Event: %-44s ║\n", eventToString(info.lastEvent));
  Serial.printf("║ Time in State: %-34u ms ║\n", getTimeInCurrentState());
  Serial.printf("║ Transitions: %-42u ║\n", info.transition_count);
  Serial.printf("║ Event Queue: %u / %-42u ║\n", 
               getEventQueueSize(), EVENT_QUEUE_SIZE);
  Serial.printf("║ Timeout Active: %-40s ║\n", currentTimeout.active ? "YES" : "NO");
  if (currentTimeout.active) {
    uint32_t remaining = currentTimeout.duration_ms - (millis() - currentTimeout.start_ms);
    Serial.printf("║   Remaining: %-38u ms ║\n", remaining);
  }
  Serial.println("║                                                          ║");
  Serial.println("║ Context:                                                 ║");
  Serial.printf("║   Phone Key Valid: %-37s ║\n", context.phone_key_valid ? "YES" : "NO");
  Serial.printf("║   Session Ready: %-39s ║\n", context.session_keys_ready ? "YES" : "NO");
  Serial.printf("║   Retry Count: %-41u ║\n", context.retry_count);
  Serial.printf("║   Error Count: %-41u ║\n", context.error_count);
  if (context.last_error != ErrorCode::NONE) {
    Serial.printf("║   Last Error: %-42s ║\n", errorToString(context.last_error));
  }
  Serial.println("╚══════════════════════════════════════════════════════════╝");
}

void printTransitionTable() {
  Serial.println("╔═══════════════════════════════════════════════════════════════════╗");
  Serial.println("║                   FSM Transition Table                            ║");
  Serial.println("╠═══════════════════════════════════════════════════════════════════╣");
  
  for (size_t i = 0; i < sizeof(transitionTable) / sizeof(transitionTable[0]); i++) {
    const auto& trans = transitionTable[i];
    Serial.printf("║ %2u. %-20s + %-22s → %-18s%s ║\n",
                 (unsigned)(i + 1),
                 stateToString(trans.from),
                 eventToString(trans.on),
                 stateToString(trans.to),
                 trans.guard ? " [G]" : "    ");
  }
  
  Serial.println("╠═══════════════════════════════════════════════════════════════════╣");
  Serial.printf("║ Total transitions: %-46u ║\n", 
               (unsigned)(sizeof(transitionTable) / sizeof(transitionTable[0])));
  Serial.println("║ [G] = Guarded transition (requires condition check)              ║");
  Serial.println("╚═══════════════════════════════════════════════════════════════════╝");
}

bool validateConfiguration() {
  Serial.println("[FSM] Validating configuration...");
  
  bool valid = true;
  
  // Check for duplicate transitions
  for (size_t i = 0; i < sizeof(transitionTable) / sizeof(transitionTable[0]); i++) {
    for (size_t j = i + 1; j < sizeof(transitionTable) / sizeof(transitionTable[0]); j++) {
      if (transitionTable[i].from == transitionTable[j].from &&
          transitionTable[i].on == transitionTable[j].on &&
          transitionTable[i].to == transitionTable[j].to) {
        Serial.printf("[FSM] ⚠️  WARNING: Duplicate transition at indices %u and %u\n", 
                     (unsigned)i, (unsigned)j);
        valid = false;
      }
    }
  }
  
  // Check for unreachable states (simple heuristic)
  bool stateReachable[16] = {false};
  stateReachable[(int)State::INIT] = true;  // Starting state
  
  for (const auto& trans : transitionTable) {
    stateReachable[(int)trans.to] = true;
  }
  
  for (int i = 0; i < 16; i++) {
    if (!stateReachable[i]) {
      Serial.printf("[FSM] ⚠️  WARNING: State %s may be unreachable\n", 
                   stateToString((State)i));
    }
  }
  
  Serial.printf("[FSM] Validation %s\n", valid ? "✓ PASSED" : "✗ FAILED");
  return valid;
}

} // namespace FSM
