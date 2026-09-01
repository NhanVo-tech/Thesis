#include "uwb/uci_session_manager.h"
#include "uwb/uci_door_unlock.h"

namespace UwbUci {

namespace {

constexpr uint8_t kGidSession = 0x01;
constexpr uint8_t kGidRanging = 0x02;

constexpr uint8_t kOidSessionInit = 0x00;
constexpr uint8_t kOidSessionDeinit = 0x01;
constexpr uint8_t kOidSessionSetAppConfig = 0x03;

constexpr uint8_t kOidRangingStart = 0x00;
constexpr uint8_t kOidRangingStop = 0x01;

constexpr uint8_t kAppDeviceType = 0x00;
constexpr uint8_t kAppRangingRoundUsage = 0x01;
constexpr uint8_t kAppStsConfig = 0x02;
constexpr uint8_t kAppMultiNodeMode = 0x03;
constexpr uint8_t kAppChannelNumber = 0x04;
constexpr uint8_t kAppDeviceMacAddress = 0x06;
constexpr uint8_t kAppDstMacAddress = 0x07;
constexpr uint8_t kAppSlotDuration = 0x08;
constexpr uint8_t kAppRangingInterval = 0x09;
constexpr uint8_t kAppAoaResultReq = 0x0D;
constexpr uint8_t kAppDeviceRole = 0x11;
constexpr uint8_t kAppRframeConfig = 0x12;
constexpr uint8_t kAppRssiReporting = 0x13;
constexpr uint8_t kAppPreambleCodeIndex = 0x14;
constexpr uint8_t kAppSfdId = 0x15;
constexpr uint8_t kAppScheduleMode = 0x22;
constexpr uint8_t kAppSlotsPerRr = 0x1B;
constexpr uint8_t kAppHoppingMode = 0x2C;
constexpr uint8_t kAppResultReportConfig = 0x2E;
constexpr uint8_t kAppVendorId = 0x27;
constexpr uint8_t kAppStaticStsIv = 0x28;

constexpr uint8_t kSessionTypeRanging = 0x00;
constexpr uint8_t kStatusOk = 0x00;
constexpr double kNearFieldSaturationReuseThresholdM = 0.5;
constexpr double kAntennaOffsetM = 0.24;

}  // namespace

UciSessionManager::UciSessionManager(IUciLink& link)
    : link_(link),
      waitingResponse_(false),
      expectedGid_(0),
      expectedOid_(0),
      responseReady_(false),
      responseStatus_(0xFF),
      responsePayload_(),
      activeSessionId_(0),
      activeSessionIdValid_(false),
      sessionActive_(false),
      activeCfg_(),
      rangingNotifCount_(0) {
  link_.setPacketCallback([this](const UciPacket& packet) { onPacket(packet); });
  lstm_ai_.begin();
}

void UciSessionManager::poll() {
  link_.poll();
}

uint32_t UciSessionManager::rangingNotificationCount() const {
  return rangingNotifCount_;
}

bool UciSessionManager::runOnce(const UciRunConfig& cfg) {
  if (sessionActive_) {
    Serial.println("[UCI] Existing session active, stopping it before restart");
    if (!stopActiveSession()) {
      Serial.println("[UCI] Failed to stop previous session");
      return false;
    }
  }

  rangingNotifCount_ = 0;
  activeSessionId_ = 0;
  activeSessionIdValid_ = false;
  first_reading_ = true;
  Serial.println("[UCI] ==== Run sequence begin ====");

  // STAGE1_DISABLED: replaced by multi-anchor Stage 2 flow
#if 0
  const bool initOk = commandSessionInit(cfg);
  if (!initOk) {
    Serial.println("[UCI] session_init failed");
    return false;
  }

  const bool cfgOk = commandSessionSetAppConfig(cfg);
  if (!cfgOk) {
    Serial.println("[UCI] session_set_app_config failed");
    commandSessionDeinit(cfg);
    return false;
  }

  const bool startOk = commandRangingStart(cfg);
  if (!startOk) {
    Serial.println("[UCI] ranging_start failed");
    commandSessionDeinit(cfg);
    return false;
  }

  activeCfg_ = cfg;
  sessionActive_ = true;
  Serial.printf("[UCI] ==== Session active. notif_count=%lu ====" "\n", static_cast<unsigned long>(rangingNotifCount_));
  return true;
#else
  (void)cfg;
  Serial.println("[UCI] Stage 1 single-anchor UCI flow disabled (Stage 2 multi-anchor active)");
  return false;
#endif  // STAGE1_DISABLED
}

bool UciSessionManager::stopActiveSession() {
  if (!sessionActive_) {
    Serial.println("[UCI] No active session to stop");
    return true;
  }

  Serial.printf("[UCI] ==== Stop sequence begin. notif_count=%lu ====" "\n",
                static_cast<unsigned long>(rangingNotifCount_));

  bool ok = true;
  if (!commandRangingStop(activeCfg_)) {
    Serial.println("[UCI] ranging_stop failed");
    ok = false;
  }
  if (!commandSessionDeinit(activeCfg_)) {
    Serial.println("[UCI] session_deinit failed");
    ok = false;
  }

  activeSessionId_ = 0;
  activeSessionIdValid_ = false;
  sessionActive_ = false;
  first_reading_ = true;
  Serial.printf("[UCI] ==== Stop sequence done. result=%s ====" "\n", ok ? "SUCCESS" : "FAIL");
  return ok;
}

bool UciSessionManager::isSessionActive() const {
  return sessionActive_;
}

void UciSessionManager::onPacket(const UciPacket& packet) {
  if (packet.mt == Mt::Response) {
    if (waitingResponse_ && packet.gid == expectedGid_ && packet.oid == expectedOid_) {
      responsePayload_ = packet.payload;
      responseStatus_ = packet.payload.empty() ? 0xFF : packet.payload[0];
      responseReady_ = true;
    }
    return;
  }

  if (packet.mt == Mt::Notification && packet.gid == kGidRanging && packet.oid == kOidRangingStart) {
    rangingNotifCount_++;
    Serial.printf("[UCI] RangingData notification #%lu payload_len=%u\n",
                  static_cast<unsigned long>(rangingNotifCount_),
                  static_cast<unsigned>(packet.payload.size()));
    // Dump payload hex for debugging distance parsing
    // Serial.print("[UCI] Ranging payload hex:");
    // for (size_t i = 0; i < packet.payload.size(); ++i) {
    //   Serial.printf(" %02X", packet.payload[i]);
    // }
    // Serial.println();
    
    // Parse measurement per FiRa/CCC UCI spec:
    // - payload[24] = number of measurements (n)
    // - for first measurement: status at payload[27], distance at payload[29:30] (uint16 little-endian, cm)
    if (packet.payload.size() >= 31) {
      const uint8_t num_meas = packet.payload[24];
      Serial.printf("[UCI] num_measurements=%u\n", static_cast<unsigned>(num_meas));
      if (num_meas > 0 && packet.payload.size() >= 31) {
        const uint8_t status = packet.payload[27];
        // Handle saturation / too-close before attempting to decode distance
        if (status == 0x1B && !first_reading_) {
          const double lastFilteredDistance = UwbDoorUnlock::getLastFilteredDistance();
          if (lastFilteredDistance > -1.0 && lastFilteredDistance < kNearFieldSaturationReuseThresholdM) {
            Serial.printf("[UCI] Saturation Error 0x1B detected. Reusing last filtered distance: %.2fm\n",
                          lastFilteredDistance);
            UwbDoorUnlock::handleRangingDistance(lastFilteredDistance);
            return;
          }
        }

        if (status == 0x00) {
          const size_t dist_off = 29;
          // Use unsigned decode then reinterpret as signed to avoid underflow issues
          uint16_t raw_cm_u = static_cast<uint16_t>(packet.payload[dist_off]) |
                              (static_cast<uint16_t>(packet.payload[dist_off + 1]) << 8);
          int16_t dist_cm = static_cast<int16_t>(raw_cm_u);

          double rawDistanceMeters = static_cast<double>(dist_cm) / 100.0;

          // Apply antenna offset (may produce slight negative values before sanity check)
          rawDistanceMeters += kAntennaOffsetM;
          
          



          // =========================================================================================
          // [GIẢ LẬP RELAY ATTACK TOÁN HỌC TẬP TRUNG] - DÙNG CHO LABEL 2
          // Mô phỏng 3 kịch bản: 
          // 1. Dịch chuyển tức thời (Classic Jitter)
          // 2. Bóng ma trườn tới (Creeping Attack)
          // 3. Step & Hold Attack
          // =========================================================================================
          // static int current_attack_type = 2; // 0: None, 1: Jitter, 2: Creeping, 3: Step & Hold
          // static int attack_timer = 0;
          // static float simulated_distance = 0.0;

          // // Chỉ kích hoạt tấn công mới nếu đang không trong đợt tấn công nào (5% cơ hội)
          // if (current_attack_type == 0 && random(0, 100) < 5) {
          //     current_attack_type = random(1, 4); // Chọn ngẫu nhiên 1 trong 3 kiểu tấn công
          //     attack_timer = 0;
          //     simulated_distance = rawDistanceMeters; // Lấy vị trí thực tại t = T_attack làm mốc
              
          //     Serial.printf("\n[ATTACK KICK-OFF] Loai: %d\n", current_attack_type);
          // }

          // // THỰC THI TOÁN HỌC CỦA TỪNG LOẠI TẤN CÔNG
          // if (current_attack_type == 1) {
          //     // KỊCH BẢN 1: CLASSIC JITTER (Rung lắc ngẫu nhiên)
          //     float jump = random(300, 1500) / 100.0; 
          //     if (random(0,2) == 0) jump = -jump;
          //     rawDistanceMeters += jump;
              
          //     if (++attack_timer > random(15, 30)) current_attack_type = 0; // Kéo dài 1.5s - 3s
              
          // } else if (current_attack_type == 2) {
          //     // KỊCH BẢN 2: CREEPING ATTACK (Bóng ma trườn tới)
          //     // Vận tốc giả mạo v_spoof = 1.0 đến 1.5 m/s (0.1 - 0.15m / frame 100ms)
          //     float v_spoof = (random(10, 15) / 100.0); 
          //     simulated_distance -= v_spoof; // Trừ dần để ép khoảng cách ngắn lại
              
          //     // Nhiễu trắng Gaussian mô phỏng R_normal
          //     float nu = random(-5, 5) / 100.0; 
          //     rawDistanceMeters = simulated_distance + nu;
              
          //     // Kết thúc tấn công nếu đã lọt vào vùng 2m hoặc kéo dài quá 4s
          //     if (++attack_timer > 40 || rawDistanceMeters < 1.0) current_attack_type = 0;
              
          // } else if (current_attack_type == 3) {
          //     // KỊCH BẢN 3: STEP & HOLD (Thao túng Innovation)
          //     if (attack_timer == 0) {
          //         // Bước nhảy Delta d (giảm đột ngột 3m đến 8m)
          //         simulated_distance -= (random(300, 800) / 100.0);
          //         if (simulated_distance < 0.5) simulated_distance = 0.5 + (random(0,5)/100.0);
          //     }
              
          //     // Bơm nhiễu siêu nhỏ R_spoof << R_normal (chỉ dao động 1-2 cm)
          //     float nu_spoof = random(-2, 2) / 100.0;
          //     rawDistanceMeters = simulated_distance + nu_spoof;
              
          //     if (++attack_timer > 35) current_attack_type = 0; // Giữ chặt 3.5 giây
          // }

          // // CHỐT CHẶN BẢO VỆ DỮ LIỆU
          // // Tuyệt đối không để giá trị âm lọt vào làm chết hàm Sanity Check bên dưới
          // if (rawDistanceMeters < 0.1) {
          //     rawDistanceMeters = 0.15 + (random(0, 10) / 100.0); 
          // }


          // [CHỈ DÙNG ĐỂ THU THẬP CREEPING ATTACK]
          // static int attack_timer = 0;
          // static float simulated_distance = 15.0; // Bắt đầu giả mạo từ xa 15m

          // // Kích hoạt tấn công liên tục
          // attack_timer++;

          // // Vận tốc giả mạo tuyến tính v_spoof = 1.2 m/s (0.12m/frame)
          // float v_spoof = 0.12; 
          // simulated_distance -= v_spoof; // Trừ dần để ép khoảng cách ngắn lại

          // // Nhiễu trắng Gaussian siêu nhỏ R_normal (chỉ dao động 3cm)
          // float nu = random(-3, 3) / 100.0; 
          // rawDistanceMeters = simulated_distance + nu;

          // // Nếu đã trườn tới mốc 0.5m thì reset lại về 15m để thu lượt mới
          // if (simulated_distance < 0.5) {
          //     simulated_distance = 15.0; 
          //     Serial.println("[ATTACK] Reset Creeping Attack về 15m");
          // }
          
          // =========================================================================================
          // [CHỈ DÙNG ĐỂ THU THẬP STEP & HOLD ATTACK - NHÃN 2]
          // Kịch bản: Đứng im ở 8m -> Giật sập xuống 2m trong 1 frame -> Giữ nguyên 2m siêu ổn định
          // =========================================================================================
          // static int attack_timer = 0;
          // static float base_distance = 8.0; // Bắt đầu ở 8.0m
          // static float hold_distance = 2.0; // Khoảng cách hacker muốn ép xe tin tưởng (2.0m)
          // static bool is_holding = false;

          // attack_timer++;

          // if (!is_holding) {
          //     // Giai đoạn 1: Khởi động ở 8m với nhiễu bình thường (Mô phỏng lúc chưa tấn công)
          //     rawDistanceMeters = base_distance + (random(-5, 5) / 100.0);
              
          //     // Đợi khoảng 1 giây (10 frames) rồi mới bắt đầu ra đòn
          //     if (attack_timer > 10) {
          //         is_holding = true;
          //         Serial.println("[ATTACK] Kích hoạt STEP: Giật sập từ 8m xuống 2.0m!");
          //     }
          // } else {
          //     // Giai đoạn 2: Ép khoảng cách xuống 2.0m và BƠM NHIỄU SIÊU NHỎ (Thao túng Kalman)
          //     float nu_spoof = random(-1, 2) / 100.0; // R_spoof cực nhỏ: chỉ dao động 1-2 cm
          //     rawDistanceMeters = hold_distance + nu_spoof;
              
          //     // Giữ chặt (Hold) trong 3.5 giây (35 frames), sau đó reset lại từ đầu để thu lượt mới
          //     if (attack_timer > 45) {
          //         is_holding = false;
          //         attack_timer = 0;
          //         // Có thể random lại base_distance để dữ liệu đa dạng hơn
          //         base_distance = random(700, 1200) / 100.0; // Random lại từ 7m đến 12m
          //         Serial.printf("[ATTACK] Reset lại, chuẩn bị nhảy từ %.2fm\n", base_distance);
          //     }
          // }
          // =========================================================================================
          // =========================================================================================





          // Sanity check: allow slight negative due to offset, but drop extreme values
          if (rawDistanceMeters < -1.0 || rawDistanceMeters > 30.0) {
            Serial.printf("[UCI] Dropped out-of-bounds reading: %.2fm\n", rawDistanceMeters);
            return;
          }

          double filteredDistanceMeters = rawDistanceMeters;
          if (first_reading_) {
            uwbFilter_ = Kalman(0.05, 0.2, 1.0, rawDistanceMeters);
            first_reading_ = false;
          } else {
            filteredDistanceMeters = uwbFilter_.getFilteredValue(rawDistanceMeters);
          }

          const double residual = rawDistanceMeters - filteredDistanceMeters;
          Serial.printf("[UCI] Valid Distance: Raw=%.2fm, Filtered=%.2fm, Res=%.2fm (status=0x%02X)\n",
                        rawDistanceMeters, filteredDistanceMeters, residual, status);
          Serial.printf("[LSTM_DATA],%lu,%.2f,%.2f,%.2f\n",
                        static_cast<unsigned long>(millis()),
                        rawDistanceMeters,
                        filteredDistanceMeters,
                        residual);
          
          // Run LSTM inference to detect relay attacks
          float p_walk = 0.0f, p_loiter = 0.0f, p_attack = 0.0f;
          bool ai_ready = lstm_ai_.predict(static_cast<float>(filteredDistanceMeters),
                                            static_cast<float>(residual),
                                            p_walk, p_loiter, p_attack);
          
          if (ai_ready) {
            Serial.printf("[AI] Walk: %.2f | Loiter: %.2f | Attack: %.2f\n",
                          p_walk, p_loiter, p_attack);
            // STAGE1_DISABLED: replaced by multi-anchor Stage 2 flow
#if 0
            UwbDoorUnlock::handleRangingWithAI(filteredDistanceMeters, p_walk, p_loiter, p_attack);
#endif
          } else {
            Serial.printf("[AI] Window warm-up: %d/15 frames\n", static_cast<int>(lstm_ai_.getFrameCount()));
            // During warm-up, still process distance but don't make lock decisions
            UwbDoorUnlock::handleRangingDistance(filteredDistanceMeters);
          }
        } else if (status != 0x1B) {
          Serial.printf("[UCI] Ignoring measurement. Status error: 0x%02X\n", status);
        }
      }
    } else {
      Serial.println("[UCI] Payload too small for measurement parsing");
    }
  }
}

bool UciSessionManager::sendCommandWithRetry(
    uint8_t gid,
    uint8_t oid,
    const std::vector<uint8_t>& payload,
    uint32_t timeoutMs,
    uint8_t retries,
    std::vector<uint8_t>* outPayload,
    uint8_t* outStatus) {
  for (uint8_t attempt = 0; attempt <= retries; ++attempt) {
    waitingResponse_ = true;
    expectedGid_ = gid;
    expectedOid_ = oid;
    responseReady_ = false;
    responseStatus_ = 0xFF;
    responsePayload_.clear();

    // Log outgoing UCI command for debugging
    Serial.printf("[UCI] Sending cmd gid=0x%02X oid=0x%02X payload_len=%u attempt=%u\n", gid, oid, (unsigned)payload.size(), static_cast<unsigned>(attempt + 1));
    Serial.print("[UCI] Outgoing payload:");
    for (size_t i = 0; i < payload.size(); ++i) Serial.printf(" %02X", payload[i]);
    Serial.println();
    const bool sendOk = link_.sendPacket(Mt::Command, gid, oid, payload, 0);
    if (!sendOk) {
      waitingResponse_ = false;
      Serial.printf("[UCI] send failed gid=0x%02X oid=0x%02X attempt=%u\n", gid, oid, static_cast<unsigned>(attempt + 1));
      continue;
    }

    const uint32_t startMs = millis();
    while ((millis() - startMs) < timeoutMs) {
      poll();
      if (responseReady_) {
        waitingResponse_ = false;
        if (outPayload != nullptr) {
          *outPayload = responsePayload_;
        }
        if (outStatus != nullptr) {
          *outStatus = responseStatus_;
        }
        if (responseStatus_ == kStatusOk) {
          return true;
        }
        Serial.printf("[UCI] rsp bad status gid=0x%02X oid=0x%02X status=0x%02X attempt=%u\n",
                      gid,
                      oid,
                      responseStatus_,
                      static_cast<unsigned>(attempt + 1));
        break;
      }
      delay(2);
    }

    waitingResponse_ = false;
    if (!responseReady_) {
      Serial.printf("[UCI] rsp timeout gid=0x%02X oid=0x%02X attempt=%u\n", gid, oid, static_cast<unsigned>(attempt + 1));
    }
  }

  return false;
}

bool UciSessionManager::commandSessionInit(const UciRunConfig& cfg) {
  std::vector<uint8_t> payload;
  appendLe(payload, cfg.sessionId, 4);
  payload.push_back(kSessionTypeRanging);

  std::vector<uint8_t> rsp;
  uint8_t status = 0xFF;
  const bool ok = sendCommandWithRetry(kGidSession, kOidSessionInit, payload, 1200, 2, &rsp, &status);
  if (!ok) {
    return false;
  }

  if (rsp.size() >= 5) {
    activeSessionId_ = static_cast<uint32_t>(rsp[1]) |
                       (static_cast<uint32_t>(rsp[2]) << 8) |
                       (static_cast<uint32_t>(rsp[3]) << 16) |
                       (static_cast<uint32_t>(rsp[4]) << 24);
    activeSessionIdValid_ = true;
    Serial.printf("[UCI] session_init returned session_handle=%lu\n",
                  static_cast<unsigned long>(activeSessionId_));
  } else {
    activeSessionId_ = cfg.sessionId;
    activeSessionIdValid_ = true;
    Serial.printf("[UCI] session_init uses session_id=%lu (FiRa 1.x style)\n",
                  static_cast<unsigned long>(activeSessionId_));
  }

  return true;
}

bool UciSessionManager::commandSessionSetAppConfig(const UciRunConfig& cfg) {
  std::vector<uint8_t> payload;
  appendLe(payload, effectiveSessionId(), 4);

  std::vector<uint8_t> tlvs;
  uint8_t v1[8];

  v1[0] = cfg.controlee ? 0x00 : 0x01;
  appendTlv(tlvs, kAppDeviceType, v1, 1);

  v1[0] = cfg.controlee ? 0x00 : 0x01;
  appendTlv(tlvs, kAppDeviceRole, v1, 1);

  v1[0] = 0x00;
  appendTlv(tlvs, kAppMultiNodeMode, v1, 1);

  v1[0] = 0x02;
  appendTlv(tlvs, kAppRangingRoundUsage, v1, 1);

  v1[0] = cfg.channel;
  appendTlv(tlvs, kAppChannelNumber, v1, 1);

  v1[0] = cfg.scheduleMode;
  appendTlv(tlvs, kAppScheduleMode, v1, 1);

  appendLe(tlvs, kAppDeviceMacAddress, 1);
  appendLe(tlvs, 2, 1);
  appendLe(tlvs, cfg.localMac, 2);

  appendLe(tlvs, kAppDstMacAddress, 1);
  appendLe(tlvs, 2, 1);
  appendLe(tlvs, cfg.destMac, 2);

  appendLe(tlvs, kAppSlotDuration, 1);
  appendLe(tlvs, 2, 1);
  appendLe(tlvs, cfg.slotDuration, 2);

  appendLe(tlvs, kAppRangingInterval, 1);
  appendLe(tlvs, 4, 1);
  appendLe(tlvs, cfg.rangingDuration, 4);

  v1[0] = cfg.rframeConfig;
  appendTlv(tlvs, kAppRframeConfig, v1, 1);

  v1[0] = 1;
  appendTlv(tlvs, kAppRssiReporting, v1, 1);

  v1[0] = cfg.preambleIdx;
  appendTlv(tlvs, kAppPreambleCodeIndex, v1, 1);

  v1[0] = cfg.sfd;
  appendTlv(tlvs, kAppSfdId, v1, 1);

  v1[0] = cfg.slotsPerRr;
  appendTlv(tlvs, kAppSlotsPerRr, v1, 1);

  v1[0] = cfg.hoppingMode;
  appendTlv(tlvs, kAppHoppingMode, v1, 1);

  v1[0] = cfg.stsConfig;
  appendTlv(tlvs, kAppStsConfig, v1, 1);

  v1[0] = cfg.aoaReport;
  appendTlv(tlvs, kAppAoaResultReq, v1, 1);

  v1[0] = cfg.resultReportConfig;
  appendTlv(tlvs, kAppResultReportConfig, v1, 1);

  appendLe(tlvs, kAppVendorId, 1);
  appendLe(tlvs, 2, 1);
  appendLe(tlvs, cfg.vendorId, 2);

  appendLe(tlvs, kAppStaticStsIv, 1);
  appendLe(tlvs, 6, 1);
  tlvs.insert(tlvs.end(), cfg.staticStsIv, cfg.staticStsIv + 6);

  size_t count = 0;
  for (size_t i = 0; i + 1 < tlvs.size();) {
    const uint8_t len = tlvs[i + 1];
    i += static_cast<size_t>(2 + len);
    count++;
  }

  payload.push_back(static_cast<uint8_t>(count & 0xFF));
  payload.insert(payload.end(), tlvs.begin(), tlvs.end());

  uint8_t status = 0xFF;
  std::vector<uint8_t> rsp;
  return sendCommandWithRetry(kGidSession, kOidSessionSetAppConfig, payload, 2000, 2, &rsp, &status);
}

bool UciSessionManager::commandRangingStart(const UciRunConfig& cfg) {
  std::vector<uint8_t> payload;
  appendLe(payload, effectiveSessionId(), 4);
  uint8_t status = 0xFF;
  return sendCommandWithRetry(kGidRanging, kOidRangingStart, payload, 1200, 2, nullptr, &status);
}

bool UciSessionManager::commandRangingStop(const UciRunConfig& cfg) {
  std::vector<uint8_t> payload;
  appendLe(payload, effectiveSessionId(), 4);
  uint8_t status = 0xFF;
  return sendCommandWithRetry(kGidRanging, kOidRangingStop, payload, 1200, 1, nullptr, &status);
}

bool UciSessionManager::commandSessionDeinit(const UciRunConfig& cfg) {
  std::vector<uint8_t> payload;
  appendLe(payload, effectiveSessionId(), 4);
  uint8_t status = 0xFF;
  return sendCommandWithRetry(kGidSession, kOidSessionDeinit, payload, 1200, 1, nullptr, &status);
}

uint32_t UciSessionManager::effectiveSessionId() const {
  return activeSessionIdValid_ ? activeSessionId_ : 0;
}

bool UciSessionManager::waitForAtLeastOneRangingNotification(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    poll();
    if (rangingNotifCount_ > 0) {
      return true;
    }
    delay(5);
  }
  return false;
}

void UciSessionManager::appendLe(std::vector<uint8_t>& dst, uint64_t v, uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    dst.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
  }
}

void UciSessionManager::appendTlv(std::vector<uint8_t>& dst, uint8_t tag, const uint8_t* value, uint8_t len) {
  dst.push_back(tag);
  dst.push_back(len);
  for (uint8_t i = 0; i < len; ++i) {
    dst.push_back(value[i]);
  }
}

}  // namespace UwbUci
