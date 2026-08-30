#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "nfc_session.h"
#include "ccc_mailbox.h"
#include "ble/ble.h"
#include "fsm/fsm.h"
#include "fsm/fsm_integration.h"
#include "test/test_fsm.h"
#include "app/anchor_bridge.h"
#include "uwb/uci_door_unlock.h"
#include "uwb/uwb_bridge.h"

namespace {

constexpr uint8_t kNumAnchors = 1;

constexpr uint8_t kAuxMacs[3][6] = {
  {0x44, 0xB1, 0x76, 0x18, 0xE4, 0x64},  // anchor-0
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

TaskHandle_t g_fsmTask = nullptr, g_nfcTask = nullptr, g_uwbTask = nullptr;

// FSM entry hooks: start ranging once the secure channel is up, stop back in IDLE.
void onEnterSecureChannel(FSM::StateContext&) { UwbBridge::sendStart(); }
void onEnterIdle(FSM::StateContext&) { UwbBridge::sendStop(); }

void fsmTaskFn(void* p) { (void)p; for (;;) { FSM::tick(); vTaskDelay(pdMS_TO_TICKS(1)); } }
void nfcTaskFn(void* p) { (void)p; for (;;) { NfcSession::tick(); vTaskDelay(pdMS_TO_TICKS(2)); } }
void uwbTaskFn(void* p) {
  (void)p;
  for (;;) {
    AnchorBridge::tick();
    UwbBridge::tick();
    UwbDoorUnlock::tick();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void handleConsole() {
  static String line;
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      line.trim();
      if (line.startsWith("RANGE:") || line.startsWith("ACK:")) {
        UwbBridge::feedLine(line.c_str());
      } else if (line == "now_start") {
        App::SessionConfig cfg;
        AnchorBridge::submitConfig(cfg, nullptr);
        AnchorBridge::requestStart(nullptr);
      } else if (line == "help") {
        Serial.println("  now_start - start ranging (default config)");
      }
      line = "";
    } else {
      line += c;
      if (line.length() > 128) line = "";
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(2000);
  while (!Serial) delay(10);

  Serial.println("╔═════════════════════════════════════════╗");
  Serial.println("║  Smart Car Access — ESP-NOW Architecture ║");
  Serial.println("╚═════════════════════════════════════════╝");

  CCCMailbox::begin();
  FSM::begin();
  BLEMod::begin();
  NfcSession::begin(Serial2, 44, 43, 115200);
  UwbDoorUnlock::begin();
  UwbBridge::begin();

  FSM::onStateEntry(FSM::AUTH_SECURE_CHANNEL_READY, onEnterSecureChannel);
  FSM::onStateEntry(FSM::IDLE, onEnterIdle);

  if (!AnchorBridge::begin(kAuxMacs, kNumAnchors)) {
    Serial.println("[MAIN] Bridge init failed");
  }

  xTaskCreatePinnedToCore(fsmTaskFn, "FSM", 8192, nullptr, 6, &g_fsmTask, 1);
  xTaskCreatePinnedToCore(nfcTaskFn, "NFC", 8192, nullptr, 4, &g_nfcTask, 1);
  xTaskCreatePinnedToCore(uwbTaskFn, "UWB", 20480, nullptr, 5, &g_uwbTask, 1);

  Serial.println("\nReady. Phone OOB → auto start. Or type 'now_start'.\n");
}

void loop() {
  BLEMod::tick();
  handleConsole();
  vTaskDelay(pdMS_TO_TICKS(50));
}