#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "uci_uart.h"
#include "uci_session.h"
#include "espnow_link.h"

#ifndef ANCHOR_UART_RX
#define ANCHOR_UART_RX 4
#endif
#ifndef ANCHOR_UART_TX
#define ANCHOR_UART_TX 5
#endif
#ifndef ANCHOR_UART_BAUD
#define ANCHOR_UART_BAUD 115200
#endif
#ifndef ANCHOR_ID
#error "ANCHOR_ID must be defined (0, 1, or 2)"
#endif

#ifndef MASTER_MAC_0
#define MASTER_MAC_0 0x00
#define MASTER_MAC_1 0x00
#define MASTER_MAC_2 0x00
#define MASTER_MAC_3 0x00
#define MASTER_MAC_4 0x00
#define MASTER_MAC_5 0x00
#endif

void onRanging(uint32_t seq, uint16_t distCm, uint8_t status,
               uint8_t nlos, int8_t rssi) {
  Serial.printf("[RANGE] seq=%u dist=%d cm status=0x%02X\n",
                static_cast<unsigned>(seq), static_cast<int>(static_cast<int16_t>(distCm)), status);
  EspNowLink::sendRanging(seq, distCm, status, nlos, rssi);
}

// UCI session runs in its own task — does NOT block main loop
void uciTask(void* param) {
  (void)param;
  for (;;) {
    if (EspNowLink::hasPendingStart()) {
      UciSession::Config cfg = EspNowLink::getPendingConfig();
      UciSession::run(cfg);
      // After session, loop continues polling for next start
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

TaskHandle_t g_uciTask = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);

  Serial.printf("\n[ANCHOR-%d] UCI Bridge ESP32-C3\n", ANCHOR_ID);

  const uint8_t masterMac[6] = {MASTER_MAC_0, MASTER_MAC_1, MASTER_MAC_2,
                                 MASTER_MAC_3, MASTER_MAC_4, MASTER_MAC_5};

  UciUart::begin(ANCHOR_UART_RX, ANCHOR_UART_TX, ANCHOR_UART_BAUD);

  if (!EspNowLink::begin(masterMac)) {
    Serial.println("[MAIN] ESP-NOW init failed");
  }

  // Run UCI session in separate task (stack = 8KB)
  xTaskCreate(uciTask, "UCI", 8192, nullptr, 3, &g_uciTask);

  Serial.printf("[ANCHOR-%d] Ready. loop() prints every 3s.\n", ANCHOR_ID);
}

void loop() {
  digitalWrite(8, HIGH);

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 3000) {
    Serial.println("loop()");
    lastPrint = millis();
  }

  // poll UART for ranging data (non-blocking)
  UciUart::poll(onRanging);
  vTaskDelay(pdMS_TO_TICKS(2));
}
