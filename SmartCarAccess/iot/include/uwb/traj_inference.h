#pragma once

#include <Arduino.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

class TrajInference {
 public:
  static constexpr int TIME_STEPS = 25;
  static constexpr int NUM_FEATURES = 4;
  static constexpr int NUM_CLASSES = 4;

  TrajInference();

  bool begin();
  bool predict(float x, float y, float vx, float vy, float p[NUM_CLASSES]);
  int getFrameCount() const { return frame_count; }

 private:
  float window[TIME_STEPS][NUM_FEATURES];
  int frame_count;

  // StandardScaler fit on [x, y, vx, vy] over the training runs
  // (tools/train_traj_model.py).
  const float scaler_mean[NUM_FEATURES] = {-1.176990f, -0.693135f, -0.019802f, 0.006809f};
  const float scaler_scale[NUM_FEATURES] = {1.118417f, 1.425282f, 0.187332f, 0.270498f};

  const tflite::Model* model;
  tflite::MicroInterpreter* interpreter;
  TfLiteTensor* input;
  TfLiteTensor* output;

  void shiftWindow();
  float normalize(float value, int feature_index) const;
};
