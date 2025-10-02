/*
 * Copyright 2025 HLS CNN Project
 * Unit tests for individual CNN components
 */

#include "../../src/hls_cnn.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace hls_cnn;
using namespace std;

#define EPSILON 1e-4

// ========== Test Utilities ==========

bool float_equal(float a, float b, float eps = EPSILON) {
  return fabs(a - b) < eps;
}

void init_random(float &val) {
  val = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Random [-1, 1]
}

template <int SIZE> void init_random_array(float arr[SIZE]) {
  for (int i = 0; i < SIZE; i++) {
    init_random(arr[i]);
  }
}

// ========== Unit Test 1: ReLU Activation ==========

bool test_relu() {
  cout << "\n=== Testing ReLU Activation ===" << endl;

  float inputs[] = {-2.0f, -0.5f, 0.0f, 0.5f, 2.0f};
  float expected[] = {0.0f, 0.0f, 0.0f, 0.5f, 2.0f};

  bool passed = true;
  for (int i = 0; i < 5; i++) {
    float result = relu(inputs[i]);
    if (!float_equal(result, expected[i])) {
      cout << "FAIL: relu(" << inputs[i] << ") = " << result << ", expected "
           << expected[i] << endl;
      passed = false;
    }
  }

  if (passed) {
    cout << "PASS: ReLU activation test" << endl;
  }
  return passed;
}

// ========== Unit Test 2: 2D Convolution ==========

bool test_conv2d() {
  cout << "\n=== Testing 2D Convolution ===" << endl;

  const int IN_CH = 1;
  const int OUT_CH = 1;
  const int IMG_SIZE = 5;
  const int KERNEL = 3;
  const int OUT_SIZE = IMG_SIZE - KERNEL + 1; // 3x3

  // Simple input (all ones)
  data_t input[IN_CH][IMG_SIZE][IMG_SIZE];
  for (int h = 0; h < IMG_SIZE; h++) {
    for (int w = 0; w < IMG_SIZE; w++) {
      input[0][h][w] = 1.0f;
    }
  }

  // Simple kernel (all ones) -> should sum to 9 for each position
  weight_t weights[OUT_CH][IN_CH][KERNEL][KERNEL];
  for (int kh = 0; kh < KERNEL; kh++) {
    for (int kw = 0; kw < KERNEL; kw++) {
      weights[0][0][kh][kw] = 1.0f;
    }
  }

  weight_t bias[OUT_CH] = {0.0f};
  data_t output[OUT_CH][OUT_SIZE][OUT_SIZE];

  conv2d<IN_CH, OUT_CH, IMG_SIZE, IMG_SIZE, KERNEL>(input, weights, bias,
                                                    output);

  // Check output (all should be 9.0 after ReLU)
  bool passed = true;
  float expected = 9.0f;

  for (int h = 0; h < OUT_SIZE; h++) {
    for (int w = 0; w < OUT_SIZE; w++) {
      if (!float_equal(output[0][h][w], expected, 0.01f)) {
        cout << "FAIL: output[" << h << "][" << w << "] = " << output[0][h][w]
             << ", expected " << expected << endl;
        passed = false;
      }
    }
  }

  if (passed) {
    cout << "PASS: Conv2D test (output sum correct)" << endl;
  }
  return passed;
}

// ========== Unit Test 3: Max Pooling ==========

bool test_max_pool() {
  cout << "\n=== Testing Max Pooling ===" << endl;

  const int CH = 1;
  const int IMG_SIZE = 4;
  const int POOL = 2;
  const int OUT_SIZE = IMG_SIZE / POOL; // 2x2

  // Input with specific pattern
  data_t input[CH][IMG_SIZE][IMG_SIZE] = {{{1.0f, 2.0f, 3.0f, 4.0f},
                                           {5.0f, 6.0f, 7.0f, 8.0f},
                                           {9.0f, 10.0f, 11.0f, 12.0f},
                                           {13.0f, 14.0f, 15.0f, 16.0f}}};

  data_t output[CH][OUT_SIZE][OUT_SIZE];

  max_pool2d<CH, IMG_SIZE, IMG_SIZE, POOL>(input, output);

  // Expected max values
  float expected[OUT_SIZE][OUT_SIZE] = {{6.0f, 8.0f}, {14.0f, 16.0f}};

  bool passed = true;
  for (int h = 0; h < OUT_SIZE; h++) {
    for (int w = 0; w < OUT_SIZE; w++) {
      if (!float_equal(output[0][h][w], expected[h][w])) {
        cout << "FAIL: output[" << h << "][" << w << "] = " << output[0][h][w]
             << ", expected " << expected[h][w] << endl;
        passed = false;
      }
    }
  }

  if (passed) {
    cout << "PASS: Max Pooling test" << endl;
  }
  return passed;
}

// ========== Unit Test 4: Fully Connected ==========

bool test_fully_connected() {
  cout << "\n=== Testing Fully Connected Layer ===" << endl;

  const int IN_SIZE = 4;
  const int OUT_SIZE = 2;

  data_t input[IN_SIZE] = {1.0f, 2.0f, 3.0f, 4.0f};

  // Simple weights
  weight_t weights[OUT_SIZE][IN_SIZE] = {
      {1.0f, 0.0f, 0.0f, 0.0f}, // Output 0: sum of input[0]
      {0.0f, 1.0f, 1.0f, 1.0f}  // Output 1: sum of input[1:3]
  };

  weight_t bias[OUT_SIZE] = {0.5f, -1.0f};
  data_t output[OUT_SIZE];

  fully_connected<IN_SIZE, OUT_SIZE>(input, weights, bias, output, true);

  // Expected:
  // output[0] = relu(1*1 + 0.5) = 1.5
  // output[1] = relu(2 + 3 + 4 - 1) = 8.0
  float expected[OUT_SIZE] = {1.5f, 8.0f};

  bool passed = true;
  for (int i = 0; i < OUT_SIZE; i++) {
    if (!float_equal(output[i], expected[i])) {
      cout << "FAIL: output[" << i << "] = " << output[i] << ", expected "
           << expected[i] << endl;
      passed = false;
    }
  }

  if (passed) {
    cout << "PASS: Fully Connected test" << endl;
  }
  return passed;
}

// ========== Unit Test 5: Flatten ==========

bool test_flatten() {
  cout << "\n=== Testing Flatten Layer ===" << endl;

  const int CH = 2;
  const int H = 2;
  const int W = 2;
  const int FLAT_SIZE = CH * H * W;

  data_t input[CH][H][W] = {{{1.0f, 2.0f}, {3.0f, 4.0f}},
                            {{5.0f, 6.0f}, {7.0f, 8.0f}}};

  data_t output[FLAT_SIZE];

  flatten<CH, H, W>(input, output);

  // Expected order: C-H-W (channel-major)
  float expected[FLAT_SIZE] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};

  bool passed = true;
  for (int i = 0; i < FLAT_SIZE; i++) {
    if (!float_equal(output[i], expected[i])) {
      cout << "FAIL: output[" << i << "] = " << output[i] << ", expected "
           << expected[i] << endl;
      passed = false;
    }
  }

  if (passed) {
    cout << "PASS: Flatten test" << endl;
  }
  return passed;
}

// ========== Main Test Runner ==========

int main() {
  srand(time(NULL));

  cout << "========================================" << endl;
  cout << "HLS CNN Unit Tests" << endl;
  cout << "========================================" << endl;

  int passed = 0;
  int total = 5;

  if (test_relu())
    passed++;
  if (test_conv2d())
    passed++;
  if (test_max_pool())
    passed++;
  if (test_fully_connected())
    passed++;
  if (test_flatten())
    passed++;

  cout << "\n========================================" << endl;
  cout << "Test Results: " << passed << "/" << total << " passed" << endl;
  cout << "========================================" << endl;

  return (passed == total) ? 0 : 1;
}
