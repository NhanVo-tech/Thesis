#include "uwb/lstm_inference.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include <esp_heap_caps.h>

constexpr int kTensorArenaSize = 35 * 1024; 
static uint8_t* tensor_arena = nullptr;

LstmInference::LstmInference()
    : frame_count(0), last_filtered_m(-1.0f), model(nullptr), interpreter(nullptr), input(nullptr), output(nullptr) {
  for (int i = 0; i < TIME_STEPS; i++) {
    for (int j = 0; j < NUM_FEATURES; j++) {
      window[i][j] = 0.0f;
    }
  }
}

bool LstmInference::begin() {
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;

  model = tflite::GetModel(uwb_lstm_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("[AI] LỖI: TFLite schema version không khớp!");
    return false;
  }

  if (tensor_arena == nullptr) {
     tensor_arena = (uint8_t*)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
     if (tensor_arena == nullptr) {
        Serial.println("[AI] LỖI: Không đủ RAM nội bộ!");
        return false;
     }
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("[AI] AllocateTensors() thất bại!");
    return false;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
  
  // KIỂM TRA SỨC KHỎE MÔ HÌNH
  Serial.println("================ AI MODEL INFO ================");
  Serial.printf("- Input Type: %d (1=Float32, 9=Int8)\n", input->type);
  Serial.printf("- Output Type: %d (1=Float32, 9=Int8)\n", output->type);
  Serial.printf("- Input Bytes: %d\n", input->bytes);
  Serial.printf("- Output Bytes: %d\n", output->bytes);
  Serial.println("===============================================");

  if (input->type != kTfLiteFloat32) {
      Serial.println("[AI] CẢNH BÁO: Mô hình không dùng Float32. Code sẽ bị crash nếu tiếp tục!");
  }

  Serial.println("[AI] AI model initialized successfully!");
  return true;
}

// BỌC GIÁP HÀM NORMALIZE TRÁNH CHIA CHO 0
float LstmInference::normalize(float value, int feature_index) {
  if (scaler_scale[feature_index] == 0.0f) {
      Serial.printf("[AI] LỖI NGHIÊM TRỌNG: Chia cho 0 tại feature %d\n", feature_index);
      return 0.0f; // Trả về 0 để cứu mạch khỏi crash
  }
  return (value - scaler_mean[feature_index]) / scaler_scale[feature_index];
}

void LstmInference::shiftWindow() {
  for (int i = 0; i < TIME_STEPS - 1; i++) {
    for (int j = 0; j < NUM_FEATURES; j++) {
      window[i][j] = window[i + 1][j];
    }
  }
}

bool LstmInference::predict(float current_filtered_m, float current_residual_m,
                            float &p_walk, float &p_loiter, float &p_attack) {
  float velocity = 0.0f;
  if (last_filtered_m >= 0.0f) {
    velocity = current_filtered_m - last_filtered_m;
  }
  last_filtered_m = current_filtered_m;

  if (frame_count == TIME_STEPS) {
    shiftWindow();
  } else {
    frame_count++;
  }

  int current_idx = (frame_count == TIME_STEPS) ? (TIME_STEPS - 1) : (frame_count - 1);
  window[current_idx][0] = normalize(current_filtered_m, 0);   
  window[current_idx][1] = normalize(current_residual_m, 1);   
  window[current_idx][2] = normalize(velocity, 2);             

  if (frame_count < TIME_STEPS) {
    return false;
  }

  // BỌC GIÁP: KIỂM TRA BỘ NHỚ TRƯỚC KHI NẠP DỮ LIỆU
  int expected_elements = TIME_STEPS * NUM_FEATURES;
  if (input->bytes < expected_elements * sizeof(float)) {
      Serial.println("[AI] LỖI: Tensor Input nhỏ hơn kích thước cửa sổ! Tránh ghi đè bộ nhớ.");
      return false;
  }

  int tensor_idx = 0;
  for (int i = 0; i < TIME_STEPS; i++) {
    for (int j = 0; j < NUM_FEATURES; j++) {
      input->data.f[tensor_idx++] = window[i][j];
    }
  }

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("[AI] Invoke() crashed!");
    return false;
  }

  // BỌC GIÁP: KIỂM TRA SỐ LƯỢNG OUTPUT TENSOR
  if (output->bytes < 3 * sizeof(float)) {
      Serial.println("[AI] LỖI: Tensor Output không đủ 3 nhãn phân loại!");
      return false;
  }

  p_walk = output->data.f[0];
  p_loiter = output->data.f[1];
  p_attack = output->data.f[2];

  return true;
}