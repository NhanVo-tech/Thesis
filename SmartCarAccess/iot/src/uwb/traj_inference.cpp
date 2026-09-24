#include "uwb/traj_inference.h"

#include <cstring>
#include <esp_heap_caps.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"

#if __has_include("uwb/uwb_traj_model.h")
#include "uwb/uwb_traj_model.h"
#define UWB_TRAJ_MODEL_AVAILABLE 1
#else
#define UWB_TRAJ_MODEL_AVAILABLE 0
#endif

namespace {

constexpr int kTensorArenaSize = 44 * 1024;
uint8_t* g_tensorArena = nullptr;

}  // namespace

TrajInference::TrajInference()
    : frame_count(0),
      model(nullptr),
      interpreter(nullptr),
      input(nullptr),
      output(nullptr) {
  for (int i = 0; i < TIME_STEPS; ++i) {
    for (int j = 0; j < NUM_FEATURES; ++j) {
      window[i][j] = 0.0f;
    }
  }
}

bool TrajInference::begin() {
#if !UWB_TRAJ_MODEL_AVAILABLE
  Serial.println("[AI] begin skipped: uwb/uwb_traj_model.h not found");
  return false;
#else
  model = tflite::GetModel(uwb_traj_model);
  if (model == nullptr || model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("[AI] begin failed: TFLite schema mismatch");
    return false;
  }

  if (g_tensorArena == nullptr) {
    g_tensorArena = static_cast<uint8_t*>(
        heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (g_tensorArena == nullptr) {
      g_tensorArena = static_cast<uint8_t*>(
          heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_8BIT));
    }
    if (g_tensorArena == nullptr) {
      Serial.println("[AI] begin failed: tensor arena allocation failed");
      return false;
    }
  }

  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;
  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, g_tensorArena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("[AI] begin failed: AllocateTensors");
    return false;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
  if (input == nullptr || output == nullptr) {
    Serial.println("[AI] begin failed: missing tensors");
    return false;
  }
  if (input->type != kTfLiteFloat32 || output->type != kTfLiteFloat32) {
    Serial.println("[AI] begin failed: model I/O must be float32");
    return false;
  }

  const int expected_input_bytes = TIME_STEPS * NUM_FEATURES * sizeof(float);
  const int expected_output_bytes = NUM_CLASSES * sizeof(float);
  if (input->bytes < expected_input_bytes || output->bytes < expected_output_bytes) {
    Serial.printf("[AI] begin failed: tensor shape mismatch in=%d out=%d\n",
                  input->bytes, output->bytes);
    return false;
  }

  frame_count = 0;
  std::memset(window, 0, sizeof(window));
  Serial.println("[AI] begin ok");
  return true;
#endif
}

bool TrajInference::predict(float x, float y, float vx, float vy,
                            float p[NUM_CLASSES]) {
  if (p == nullptr) return false;
  for (int i = 0; i < NUM_CLASSES; ++i) {
    p[i] = 0.0f;
  }
  if (interpreter == nullptr || input == nullptr || output == nullptr) {
    return false;
  }

  if (frame_count == TIME_STEPS) {
    shiftWindow();
  } else {
    ++frame_count;
  }

  const int current_idx =
      (frame_count == TIME_STEPS) ? (TIME_STEPS - 1) : (frame_count - 1);
  window[current_idx][0] = normalize(x, 0);
  window[current_idx][1] = normalize(y, 1);
  window[current_idx][2] = normalize(vx, 2);
  window[current_idx][3] = normalize(vy, 3);

  if (frame_count < TIME_STEPS) {
    return false;
  }

  const int expected_input_bytes = TIME_STEPS * NUM_FEATURES * sizeof(float);
  const int expected_output_bytes = NUM_CLASSES * sizeof(float);
  if (input->bytes < expected_input_bytes || output->bytes < expected_output_bytes) {
    Serial.println("[AI] predict failed: tensor too small");
    return false;
  }

  int tensor_idx = 0;
  for (int i = 0; i < TIME_STEPS; ++i) {
    for (int j = 0; j < NUM_FEATURES; ++j) {
      input->data.f[tensor_idx++] = window[i][j];
    }
  }

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("[AI] predict failed: Invoke");
    return false;
  }

  for (int i = 0; i < NUM_CLASSES; ++i) {
    p[i] = output->data.f[i];
  }
  return true;
}

void TrajInference::shiftWindow() {
  for (int i = 0; i < TIME_STEPS - 1; ++i) {
    for (int j = 0; j < NUM_FEATURES; ++j) {
      window[i][j] = window[i + 1][j];
    }
  }
}

float TrajInference::normalize(float value, int feature_index) const {
  if (feature_index < 0 || feature_index >= NUM_FEATURES) {
    return 0.0f;
  }
  const float scale = scaler_scale[feature_index];
  if (scale == 0.0f) {
    return 0.0f;
  }
  return (value - scaler_mean[feature_index]) / scale;
}
