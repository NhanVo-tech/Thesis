#pragma once
#include "fsm_states.h"
#include <cstddef>

namespace FSM {

// Initialize FSM and all subsystems
void begin();

// Main FSM loop - call this in loop()
void tick();

// Trigger an event externally (from BLE/NFC callbacks)
// Returns true if event was queued successfully
bool triggerEvent(Event event);

// Trigger event with additional data/context
bool triggerEventWithData(Event event, void* data, size_t data_len);

// Get current state
State getCurrentState();

// Get detailed state info
StateInfo getStateInfo();

// Force state transition (for testing)
bool forceState(State newState, bool force = false);

// Reset FSM to IDLE
void reset(bool clearSession = false);

// Check if in specific state groups
bool isProvisioning();
bool isAuthenticating();
bool isUnlocking();
bool isInErrorState();

// Get time spent in current state (ms)
uint32_t getTimeInCurrentState();

// Get state context (read-only)
const StateContext& getContext();

// Set state timeout for current state
void setStateTimeout(uint32_t timeout_ms);

// Clear any pending timeout
void clearTimeout();

// Register state entry/exit callbacks
void onStateEntry(State state, StateAction action);
void onStateExit(State state, StateAction action);

// Event queue status
size_t getEventQueueSize();
bool isEventQueueFull();
void clearEventQueue();

// Enable/disable debug logging
void setDebugLogging(bool enable);

// Print FSM status to Serial
void printStatus();

// Print state transition table
void printTransitionTable();

// Validate FSM configuration (check for unreachable states, missing transitions)
bool validateConfiguration();

} // namespace FSM
