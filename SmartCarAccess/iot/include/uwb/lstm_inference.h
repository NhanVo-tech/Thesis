#pragma once

#include <Arduino.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "uwb/uwb_lstm_model.h"

#define TIME_STEPS 25
#define NUM_FEATURES 3

/**
 * LSTM Inference Module: Processes UWB distance and residual data
 * to detect relay attacks via a TinyML model running on-device.
 * 
 * Architecture:
 * 1. Maintains a 25-frame sliding window of [distance, residual, velocity]
 * 2. Normalizes input using z-score parameters from training set
 * 3. Runs TFLite Micro inference to output softmax probabilities
 * 4. Labels: p_walk (normal approach), p_loiter (hovering), p_attack (relay attack)
 */
class LstmInference {
 public:
  LstmInference();
  
  /**
   * Initialize the LSTM model interpreter.
   * Must be called once during setup.
   * @return true if model loaded successfully
   */
  bool begin();
  
  /**
   * Feed new data point into sliding window and predict if ready.
   * Requires 15 frames before returning a valid prediction.
   * @param current_filtered_m Kalman-filtered UWB distance (meters)
   * @param current_residual_m Residual (raw - filtered) from Kalman
   * @param p_walk Output: probability of normal walking approach
   * @param p_loiter Output: probability of loitering / hovering
   * @param p_attack Output: probability of relay attack
   * @return true if prediction is valid (window full); false during warm-up
   */
  bool predict(float current_filtered_m, float current_residual_m,
               float &p_walk, float &p_loiter, float &p_attack);

  /**
   * Get current frame count in sliding window (for diagnostics)
   */
  int getFrameCount() const { return frame_count; }

 private:
  // Sliding window storing the last 25 frames of [distance, residual, velocity]
  float window[TIME_STEPS][NUM_FEATURES];
  int frame_count;
  float last_filtered_m;

  // Normalization parameters (z-score scaler from training set)
  const float scaler_mean[NUM_FEATURES] = {4.870000f, -0.120000f, -0.080000f};
  const float scaler_scale[NUM_FEATURES] = {4.330000f, 0.200000f, 0.130000f};

  // TFLite Micro components
  const tflite::Model* model;
  tflite::MicroInterpreter* interpreter;
  TfLiteTensor* input;
  TfLiteTensor* output;

  /**
   * Shift window left to discard oldest frame and make room for new data
   */
  void shiftWindow();

  /**
   * Normalize a single value using z-score (standardization)
   * @param value Raw value to normalize
   * @param feature_index Index of feature (0=distance, 1=residual, 2=velocity)
   * @return Normalized value: (value - mean) / scale
   */
  float normalize(float value, int feature_index);
};
