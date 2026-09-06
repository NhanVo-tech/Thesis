#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "soc/timer_group_struct.h"
#include "nfc_session.h"
#include "ccc_mailbox.h"
#include "ble/ble.h"
#include "fsm/fsm.h"
#include "fsm/fsm_integration.h"
#include "test/test_fsm.h"
#include "uwb/access_controller.h"
#include "uwb/uwb_bridge.h"

namespace {

TaskHandle_t g_fsmTask = nullptr, g_nfcTask = nullptr, g_uwbTask = nullptr;

// Note: anchor sessions (CMD:START/STOP_RANGING over USB-CDC) are now driven
// explicitly by the phone via CCC tunnel instructions 0x84/0x85, not by FSM
// state entry hooks. See ble_auth.cpp kInsRangingStart/kInsRangingStop.

void fsmTaskFn(void* p) { (void)p; for (;;) { FSM::tick(); vTaskDelay(pdMS_TO_TICKS(1)); } }
void nfcTaskFn(void* p) { (void)p; for (;;) { NfcSession::tick(); vTaskDelay(pdMS_TO_TICKS(2)); } }

// Reads PC RANGE/ACK frames from USB-CDC. Runs in the UWB task (priority 5) so
// it is not starved by the NFC polling task (priority 4); reading here keeps
// RANGE frames flowing at a steady rate instead of arriving in bursts.
void handleConsole() {
  static String line;
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      line.trim();
      if (line.startsWith("RANGE:") || line.startsWith("ACK:")) {
        UwbBridge::feedLine(line.c_str());
      } else if (line == "help") {
        Serial.println("  help - show this message");
      }
      line = "";
    } else {
      line += c;
      if (line.length() > 128) line = "";
    }
  }
}

void uwbTaskFn(void* p) {
  (void)p;
  for (;;) {
    handleConsole();
    UwbBridge::tick();
    AccessController::tick();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

}  // namespace

void setup() {
  // Disable both ESP32-S3 watchdogs. Their timeouts are baked into the
  // precompiled Arduino framework (cannot be raised from platformio.ini) and
  // they fire spuriously on this board:
  //   * MWDT1 (TIMERG1) = interrupt watchdog, 300 ms -> rst:0x8 (TG1WDT_SYS_RST)
  //     trips on short critical sections / BLE RF flash-cache stalls.
  //   * MWDT0 (TIMERG0) = task watchdog, 5 s, watches CPU0 idle -> rst:0x7
  //     (TG0WDT_SYS_RST) trips during long NVS page-recovery flash erases that
  //     freeze both CPUs.
  TIMERG1.wdtwprotect.val = 0x50D83AA1;  // unlock
  TIMERG1.wdtconfig0.val = 0;            // disable IWDT (MWDT1)
  TIMERG1.wdtwprotect.val = 0;           // re-lock

  TIMERG0.wdtwprotect.val = 0x50D83AA1;  // unlock
  TIMERG0.wdtconfig0.val = 0;            // disable TWDT (MWDT0)
  TIMERG0.wdtwprotect.val = 0;           // re-lock

  Serial.begin(115200);
  delay(2000);
  while (!Serial) delay(10);

  Serial.println("╔═════════════════════════════════════════╗");
  Serial.println("║  Smart Car Access — PC Bridge Architecture ║");
  Serial.println("╚═════════════════════════════════════════╝");

  CCCMailbox::begin();
  FSM::begin();
  BLEMod::begin();
  NfcSession::begin(Serial2, 44, 43, 115200);
  UwbBridge::begin();
  AccessController::begin();

  xTaskCreatePinnedToCore(fsmTaskFn, "FSM", 8192, nullptr, 6, &g_fsmTask, 1);
  xTaskCreatePinnedToCore(nfcTaskFn, "NFC", 8192, nullptr, 4, &g_nfcTask, 1);
  xTaskCreatePinnedToCore(uwbTaskFn, "UWB", 20480, nullptr, 5, &g_uwbTask, 1);

  Serial.println("\nReady. PC sends RANGE frames over USB-CDC.\n");
}

void loop() {
  BLEMod::tick();
  vTaskDelay(pdMS_TO_TICKS(50));
}