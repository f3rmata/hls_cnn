/*
 * Copyright 2025 HLS CNN Project
 * Integration test for complete CNN inference
 */

#include "../../src/hls_cnn.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace hls_cnn;

// ========== Initialization Utilities ==========

void init_random_weights() {
  srand(42); // Fixed seed for reproducibility
}

template <typename T>
void init_array(T *arr, int size, float mean = 0.0f, float stddev = 0.1f) {
  for (int i = 0; i < size; i++) {
    // Simple random initialization
    float r = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    arr[i] = T(mean + r * stddev);
  }
}

void init_simple_image(
    data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE]) {
  // Create a simple test pattern (cross shape)
  for (int h = 0; h < CONV1_IMG_SIZE; h++) {
    for (int w = 0; w < CONV1_IMG_SIZE; w++) {
      if (h == CONV1_IMG_SIZE / 2 || w == CONV1_IMG_SIZE / 2) {
        input[0][h][w] = data_t(1.0f);
      } else {
        input[0][h][w] = data_t(0.0f);
      }
    }
  }
}

int argmax(data_t *arr, int size) {
  int max_idx = 0;
  data_t max_val = arr[0];
  for (int i = 1; i < size; i++) {
    if (arr[i] > max_val) {
      max_val = arr[i];
      max_idx = i;
    }
  }
  return max_idx;
}

// ========== Integration Test ==========

bool test_cnn_inference() {
  cout << "\n=== CNN Integration Test ===" << endl;
  cout << "Network architecture:" << endl;
  cout << "  Input: [1x28x28]" << endl;
  cout << "  Conv1: [16x26x26] (3x3 kernel)" << endl;
  cout << "  Pool1: [16x13x13] (2x2 max pool)" << endl;
  cout << "  Conv2: [32x11x11] (3x3 kernel)" << endl;
  cout << "  Pool2: [32x5x5] (2x2 max pool)" << endl;
  cout << "  Flatten: [800]" << endl;
  cout << "  FC1: [128] (with ReLU)" << endl;
  cout << "  FC2: [10] (logits)" << endl;
  cout << endl;

  // Allocate memory
  static data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE];

  static weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE]
                               [CONV1_KERNEL_SIZE];
  static weight_t conv1_bias[CONV1_OUT_CH];

  static weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE]
                               [CONV2_KERNEL_SIZE];
  static weight_t conv2_bias[CONV2_OUT_CH];

  static weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE];
  static weight_t fc1_bias[FC1_OUT_SIZE];

  static weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE];
  static weight_t fc2_bias[FC2_OUT_SIZE];

  static data_t output[FC2_OUT_SIZE];

  // Initialize with random weights
  init_random_weights();

  cout << "Initializing network parameters..." << endl;
  init_array((weight_t *)conv1_weights, CONV1_OUT_CH * CONV1_IN_CH *
                                            CONV1_KERNEL_SIZE *
                                            CONV1_KERNEL_SIZE);
  init_array(conv1_bias, CONV1_OUT_CH, 0.0f, 0.01f);

  init_array((weight_t *)conv2_weights, CONV2_OUT_CH * CONV2_IN_CH *
                                            CONV2_KERNEL_SIZE *
                                            CONV2_KERNEL_SIZE);
  init_array(conv2_bias, CONV2_OUT_CH, 0.0f, 0.01f);

  init_array((weight_t *)fc1_weights, FC1_OUT_SIZE * FC1_IN_SIZE, 0.0f, 0.01f);
  init_array(fc1_bias, FC1_OUT_SIZE, 0.0f, 0.01f);

  init_array((weight_t *)fc2_weights, FC2_OUT_SIZE * FC2_IN_SIZE, 0.0f, 0.01f);
  init_array(fc2_bias, FC2_OUT_SIZE, 0.0f, 0.01f);

  // Create test input
  cout << "Creating test input (cross pattern)..." << endl;
  init_simple_image(input);

  // Run inference
  cout << "Running CNN inference..." << endl;
  cnn_inference(input, conv1_weights, conv1_bias, conv2_weights, conv2_bias,
                fc1_weights, fc1_bias, fc2_weights, fc2_bias, output);

  // Check output
  cout << "\nOutput logits:" << endl;
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    cout << "  Class " << i << ": " << output[i] << endl;
  }

  int predicted_class = argmax(output, FC2_OUT_SIZE);
  cout << "\nPredicted class: " << predicted_class << endl;

  // Sanity checks
  bool passed = true;

  // Check that outputs are finite
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    float val = output[i].to_double();
    if (!isfinite(val)) {
      cout << "FAIL: Output[" << i << "] is not finite: " << output[i] << endl;
      passed = false;
    }
  }

  // Check that output values are reasonable (not all zeros, not too large)
  float sum = 0.0f;
  float max_abs = 0.0f;
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    float val = output[i].to_double();
    sum += fabs(val);
    max_abs = fmax(max_abs, fabs(val));
  }

  if (sum < 1e-6) {
    cout << "FAIL: All outputs are near zero" << endl;
    passed = false;
  }

  if (max_abs > 1e6) {
    cout << "FAIL: Output values too large (max_abs = " << max_abs << ")"
         << endl;
    passed = false;
  }

  if (passed) {
    cout << "\nPASS: CNN inference test" << endl;
    cout << "  - All outputs are finite" << endl;
    cout << "  - Output sum: " << sum << endl;
    cout << "  - Max abs value: " << max_abs << endl;
  }

  return passed;
}

// ========== Performance Test ==========

void test_performance() {
  cout << "\n=== Performance Estimation ===" << endl;

  // Calculate theoretical operation counts
  long long conv1_ops = (long long)CONV1_OUT_CH * CONV1_IN_CH *
                        (CONV1_IMG_SIZE - CONV1_KERNEL_SIZE + 1) *
                        (CONV1_IMG_SIZE - CONV1_KERNEL_SIZE + 1) *
                        CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE *
                        2; // MAC = 2 ops

  long long conv2_ops = (long long)CONV2_OUT_CH * CONV2_IN_CH *
                        (CONV2_IMG_SIZE - CONV2_KERNEL_SIZE + 1) *
                        (CONV2_IMG_SIZE - CONV2_KERNEL_SIZE + 1) *
                        CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE * 2;

  long long fc1_ops = (long long)FC1_OUT_SIZE * FC1_IN_SIZE * 2;
  long long fc2_ops = (long long)FC2_OUT_SIZE * FC2_IN_SIZE * 2;

  long long total_ops = conv1_ops + conv2_ops + fc1_ops + fc2_ops;

  cout << "Operation counts:" << endl;
  cout << "  Conv1: " << conv1_ops / 1e6 << " M ops" << endl;
  cout << "  Conv2: " << conv2_ops / 1e6 << " M ops" << endl;
  cout << "  FC1: " << fc1_ops / 1e6 << " M ops" << endl;
  cout << "  FC2: " << fc2_ops / 1e6 << " M ops" << endl;
  cout << "  Total: " << total_ops / 1e6 << " M ops" << endl;

  // Memory footprint
  long long weights_size =
      CONV1_OUT_CH * CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE +
      CONV2_OUT_CH * CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE +
      FC1_OUT_SIZE * FC1_IN_SIZE + FC2_OUT_SIZE * FC2_IN_SIZE;

  cout << "\nMemory footprint:" << endl;
  cout << "  Weights: " << weights_size * sizeof(weight_t) / 1024.0 << " KB"
       << endl;
  cout << "  Input: "
       << CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE * sizeof(data_t) /
              1024.0
       << " KB" << endl;
}

// ========== Main ==========

int main() {
  cout << "========================================" << endl;
  cout << "HLS CNN Integration Test" << endl;
  cout << "========================================" << endl;

  bool passed = test_cnn_inference();

  test_performance();

  cout << "\n========================================" << endl;
  if (passed) {
    cout << "Integration Test: PASSED" << endl;
  } else {
    cout << "Integration Test: FAILED" << endl;
  }
  cout << "========================================" << endl;

  return passed ? 0 : 1;
}
