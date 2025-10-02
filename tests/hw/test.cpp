/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "uut_top.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>

// Golden reference using float types (separate from HLS types)
typedef float golden_data_t;
typedef float golden_weight_t;
typedef float golden_acc_t;

/**
 * @brief Generate random test data
 */
void generate_test_data(golden_data_t *input, golden_weight_t *conv1_w,
                        golden_weight_t *conv1_b, golden_weight_t *conv2_w,
                        golden_weight_t *conv2_b, golden_weight_t *fc1_w,
                        golden_weight_t *fc1_b, golden_weight_t *fc2_w,
                        golden_weight_t *fc2_b) {
  // Initialize random seed
  std::srand(12345);

  // Generate input (normalized to [0, 1])
  for (int i = 0; i < CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE; i++) {
    input[i] = static_cast<golden_data_t>(std::rand()) / RAND_MAX;
  }

  // Generate Conv1 weights and bias (Xavier initialization)
  int conv1_w_size =
      CONV1_OUT_CH * CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE;
  golden_data_t conv1_scale =
      std::sqrt(2.0 / static_cast<golden_data_t>(
                          CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE));
  for (int i = 0; i < conv1_w_size; i++) {
    conv1_w[i] = (static_cast<golden_data_t>(std::rand()) / RAND_MAX - 0.5) *
                 2.0 * conv1_scale;
  }
  for (int i = 0; i < CONV1_OUT_CH; i++) {
    conv1_b[i] = 0.0;
  }

  // Generate Conv2 weights and bias
  int conv2_w_size =
      CONV2_OUT_CH * CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE;
  golden_data_t conv2_scale =
      std::sqrt(2.0 / static_cast<golden_data_t>(
                          CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE));
  for (int i = 0; i < conv2_w_size; i++) {
    conv2_w[i] = (static_cast<golden_data_t>(std::rand()) / RAND_MAX - 0.5) *
                 2.0 * conv2_scale;
  }
  for (int i = 0; i < CONV2_OUT_CH; i++) {
    conv2_b[i] = 0.0;
  }

  // Generate FC1 weights and bias
  int fc1_w_size = FC1_OUT_SIZE * FC1_IN_SIZE;
  golden_data_t fc1_scale =
      std::sqrt(2.0 / static_cast<golden_data_t>(FC1_IN_SIZE));
  for (int i = 0; i < fc1_w_size; i++) {
    fc1_w[i] = (static_cast<golden_data_t>(std::rand()) / RAND_MAX - 0.5) *
               2.0 * fc1_scale;
  }
  for (int i = 0; i < FC1_OUT_SIZE; i++) {
    fc1_b[i] = 0.0;
  }

  // Generate FC2 weights and bias
  int fc2_w_size = FC2_OUT_SIZE * FC2_IN_SIZE;
  golden_data_t fc2_scale =
      std::sqrt(2.0 / static_cast<golden_data_t>(FC2_IN_SIZE));
  for (int i = 0; i < fc2_w_size; i++) {
    fc2_w[i] = (static_cast<golden_data_t>(std::rand()) / RAND_MAX - 0.5) *
               2.0 * fc2_scale;
  }
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    fc2_b[i] = 0.0;
  }
}

/**
 * @brief Compute golden reference using float
 */
void compute_golden(
    const golden_data_t *input_flat, const golden_weight_t *conv1_w_flat,
    const golden_weight_t *conv1_b_flat, const golden_weight_t *conv2_w_flat,
    const golden_weight_t *conv2_b_flat, const golden_weight_t *fc1_w_flat,
    const golden_weight_t *fc1_b_flat, const golden_weight_t *fc2_w_flat,
    const golden_weight_t *fc2_b_flat, golden_data_t *output_golden) {
  // Reshape inputs to multi-dimensional arrays
  golden_data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE];
  for (int c = 0; c < CONV1_IN_CH; c++) {
    for (int h = 0; h < CONV1_IMG_SIZE; h++) {
      for (int w = 0; w < CONV1_IMG_SIZE; w++) {
        input[c][h][w] = input_flat[c * CONV1_IMG_SIZE * CONV1_IMG_SIZE +
                                    h * CONV1_IMG_SIZE + w];
      }
    }
  }

  golden_weight_t conv1_w[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE]
                         [CONV1_KERNEL_SIZE];
  for (int oc = 0; oc < CONV1_OUT_CH; oc++) {
    for (int ic = 0; ic < CONV1_IN_CH; ic++) {
      for (int kh = 0; kh < CONV1_KERNEL_SIZE; kh++) {
        for (int kw = 0; kw < CONV1_KERNEL_SIZE; kw++) {
          int idx = oc * CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE +
                    ic * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE +
                    kh * CONV1_KERNEL_SIZE + kw;
          conv1_w[oc][ic][kh][kw] = conv1_w_flat[idx];
        }
      }
    }
  }

  golden_weight_t conv1_b[CONV1_OUT_CH];
  for (int i = 0; i < CONV1_OUT_CH; i++) {
    conv1_b[i] = conv1_b_flat[i];
  }

  golden_weight_t conv2_w[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE]
                         [CONV2_KERNEL_SIZE];
  for (int oc = 0; oc < CONV2_OUT_CH; oc++) {
    for (int ic = 0; ic < CONV2_IN_CH; ic++) {
      for (int kh = 0; kh < CONV2_KERNEL_SIZE; kh++) {
        for (int kw = 0; kw < CONV2_KERNEL_SIZE; kw++) {
          int idx = oc * CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE +
                    ic * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE +
                    kh * CONV2_KERNEL_SIZE + kw;
          conv2_w[oc][ic][kh][kw] = conv2_w_flat[idx];
        }
      }
    }
  }

  golden_weight_t conv2_b[CONV2_OUT_CH];
  for (int i = 0; i < CONV2_OUT_CH; i++) {
    conv2_b[i] = conv2_b_flat[i];
  }

  golden_weight_t fc1_w[FC1_OUT_SIZE][FC1_IN_SIZE];
  for (int o = 0; o < FC1_OUT_SIZE; o++) {
    for (int i = 0; i < FC1_IN_SIZE; i++) {
      fc1_w[o][i] = fc1_w_flat[o * FC1_IN_SIZE + i];
    }
  }

  golden_weight_t fc1_b[FC1_OUT_SIZE];
  for (int i = 0; i < FC1_OUT_SIZE; i++) {
    fc1_b[i] = fc1_b_flat[i];
  }

  golden_weight_t fc2_w[FC2_OUT_SIZE][FC2_IN_SIZE];
  for (int o = 0; o < FC2_OUT_SIZE; o++) {
    for (int i = 0; i < FC2_IN_SIZE; i++) {
      fc2_w[o][i] = fc2_w_flat[o * FC2_IN_SIZE + i];
    }
  }

  golden_weight_t fc2_b[FC2_OUT_SIZE];
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    fc2_b[i] = fc2_b_flat[i];
  }

  // Perform golden reference CNN inference using float
  // Conv1 + ReLU
  golden_data_t conv1_out[CONV1_OUT_CH][POOL1_IMG_SIZE][POOL1_IMG_SIZE];
  for (int oc = 0; oc < CONV1_OUT_CH; oc++) {
    for (int oh = 0; oh < POOL1_IMG_SIZE; oh++) {
      for (int ow = 0; ow < POOL1_IMG_SIZE; ow++) {
        golden_acc_t sum = conv1_b[oc];
        for (int ic = 0; ic < CONV1_IN_CH; ic++) {
          for (int kh = 0; kh < CONV1_KERNEL_SIZE; kh++) {
            for (int kw = 0; kw < CONV1_KERNEL_SIZE; kw++) {
              int ih = oh + kh;
              int iw = ow + kw;
              sum += input[ic][ih][iw] * conv1_w[oc][ic][kh][kw];
            }
          }
        }
        conv1_out[oc][oh][ow] = (sum > 0.0f) ? sum : 0.0f; // ReLU
      }
    }
  }

  // Pool1 (Max Pool)
  golden_data_t pool1_out[CONV1_OUT_CH][POOL1_OUT_SIZE][POOL1_OUT_SIZE];
  for (int c = 0; c < CONV1_OUT_CH; c++) {
    for (int oh = 0; oh < POOL1_OUT_SIZE; oh++) {
      for (int ow = 0; ow < POOL1_OUT_SIZE; ow++) {
        golden_data_t max_val = conv1_out[c][oh * POOL1_SIZE][ow * POOL1_SIZE];
        for (int ph = 0; ph < POOL1_SIZE; ph++) {
          for (int pw = 0; pw < POOL1_SIZE; pw++) {
            golden_data_t val =
                conv1_out[c][oh * POOL1_SIZE + ph][ow * POOL1_SIZE + pw];
            if (val > max_val)
              max_val = val;
          }
        }
        pool1_out[c][oh][ow] = max_val;
      }
    }
  }

  // Conv2 + ReLU
  golden_data_t conv2_out[CONV2_OUT_CH][POOL2_IMG_SIZE][POOL2_IMG_SIZE];
  for (int oc = 0; oc < CONV2_OUT_CH; oc++) {
    for (int oh = 0; oh < POOL2_IMG_SIZE; oh++) {
      for (int ow = 0; ow < POOL2_IMG_SIZE; ow++) {
        golden_acc_t sum = conv2_b[oc];
        for (int ic = 0; ic < CONV2_IN_CH; ic++) {
          for (int kh = 0; kh < CONV2_KERNEL_SIZE; kh++) {
            for (int kw = 0; kw < CONV2_KERNEL_SIZE; kw++) {
              int ih = oh + kh;
              int iw = ow + kw;
              sum += pool1_out[ic][ih][iw] * conv2_w[oc][ic][kh][kw];
            }
          }
        }
        conv2_out[oc][oh][ow] = (sum > 0.0f) ? sum : 0.0f; // ReLU
      }
    }
  }

  // Pool2 (Max Pool)
  golden_data_t pool2_out[CONV2_OUT_CH][POOL2_OUT_SIZE][POOL2_OUT_SIZE];
  for (int c = 0; c < CONV2_OUT_CH; c++) {
    for (int oh = 0; oh < POOL2_OUT_SIZE; oh++) {
      for (int ow = 0; ow < POOL2_OUT_SIZE; ow++) {
        golden_data_t max_val = conv2_out[c][oh * POOL2_SIZE][ow * POOL2_SIZE];
        for (int ph = 0; ph < POOL2_SIZE; ph++) {
          for (int pw = 0; pw < POOL2_SIZE; pw++) {
            golden_data_t val =
                conv2_out[c][oh * POOL2_SIZE + ph][ow * POOL2_SIZE + pw];
            if (val > max_val)
              max_val = val;
          }
        }
        pool2_out[c][oh][ow] = max_val;
      }
    }
  }

  // Flatten
  golden_data_t flatten_out[FC1_IN_SIZE];
  int idx = 0;
  for (int c = 0; c < CONV2_OUT_CH; c++) {
    for (int h = 0; h < POOL2_OUT_SIZE; h++) {
      for (int w = 0; w < POOL2_OUT_SIZE; w++) {
        flatten_out[idx++] = pool2_out[c][h][w];
      }
    }
  }

  // FC1 + ReLU
  golden_data_t fc1_out[FC1_OUT_SIZE];
  for (int o = 0; o < FC1_OUT_SIZE; o++) {
    golden_acc_t sum = fc1_b[o];
    for (int i = 0; i < FC1_IN_SIZE; i++) {
      sum += flatten_out[i] * fc1_w[o][i];
    }
    fc1_out[o] = (sum > 0.0f) ? sum : 0.0f; // ReLU
  }

  // FC2 (output logits, no activation)
  for (int o = 0; o < FC2_OUT_SIZE; o++) {
    golden_acc_t sum = fc2_b[o];
    for (int i = 0; i < FC2_IN_SIZE; i++) {
      sum += fc1_out[i] * fc2_w[o][i];
    }
    output_golden[o] = sum;
  }
}

/**
 * @brief Compare outputs with tolerance
 */
bool compare_outputs(const data_t *output_hls,
                     const golden_data_t *output_golden, int size,
                     golden_data_t tolerance) {
  bool pass = true;
  golden_data_t max_error = 0.0;

  for (int i = 0; i < size; i++) {
    golden_data_t hls_val = static_cast<golden_data_t>(output_hls[i]);
    golden_data_t golden_val = output_golden[i];
    golden_data_t error = std::abs(hls_val - golden_val);

    if (error > max_error) {
      max_error = error;
    }

    if (error > tolerance) {
      std::cout << "Mismatch at index " << i << ": HLS=" << hls_val
                << ", Golden=" << golden_val << ", Error=" << error
                << std::endl;
      pass = false;
    }
  }

  std::cout << "Maximum error: " << max_error << std::endl;
  return pass;
}

/**
 * @brief Main test function
 */
int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "HLS CNN C/Co-Simulation Test" << std::endl;
  std::cout << "========================================" << std::endl;

  // Allocate memory for test data
  golden_data_t *input =
      new golden_data_t[CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE];

  int conv1_w_size =
      CONV1_OUT_CH * CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE;
  golden_weight_t *conv1_w = new golden_weight_t[conv1_w_size];
  golden_weight_t *conv1_b = new golden_weight_t[CONV1_OUT_CH];

  int conv2_w_size =
      CONV2_OUT_CH * CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE;
  golden_weight_t *conv2_w = new golden_weight_t[conv2_w_size];
  golden_weight_t *conv2_b = new golden_weight_t[CONV2_OUT_CH];

  int fc1_w_size = FC1_OUT_SIZE * FC1_IN_SIZE;
  golden_weight_t *fc1_w = new golden_weight_t[fc1_w_size];
  golden_weight_t *fc1_b = new golden_weight_t[FC1_OUT_SIZE];

  int fc2_w_size = FC2_OUT_SIZE * FC2_IN_SIZE;
  golden_weight_t *fc2_w = new golden_weight_t[fc2_w_size];
  golden_weight_t *fc2_b = new golden_weight_t[FC2_OUT_SIZE];

  golden_data_t *output_golden = new golden_data_t[FC2_OUT_SIZE];

  // Generate test data
  std::cout << "Generating test data..." << std::endl;
  generate_test_data(input, conv1_w, conv1_b, conv2_w, conv2_b, fc1_w, fc1_b,
                     fc2_w, fc2_b);

  // Convert to fixed-point for HLS
  data_t *input_fixed =
      new data_t[CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE];
  weight_t *conv1_w_fixed = new weight_t[conv1_w_size];
  weight_t *conv1_b_fixed = new weight_t[CONV1_OUT_CH];
  weight_t *conv2_w_fixed = new weight_t[conv2_w_size];
  weight_t *conv2_b_fixed = new weight_t[CONV2_OUT_CH];
  weight_t *fc1_w_fixed = new weight_t[fc1_w_size];
  weight_t *fc1_b_fixed = new weight_t[FC1_OUT_SIZE];
  weight_t *fc2_w_fixed = new weight_t[fc2_w_size];
  weight_t *fc2_b_fixed = new weight_t[FC2_OUT_SIZE];
  data_t *output_fixed = new data_t[FC2_OUT_SIZE];

  // Convert float to fixed-point
  for (int i = 0; i < CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE; i++)
    input_fixed[i] = input[i];
  for (int i = 0; i < conv1_w_size; i++)
    conv1_w_fixed[i] = conv1_w[i];
  for (int i = 0; i < CONV1_OUT_CH; i++)
    conv1_b_fixed[i] = conv1_b[i];
  for (int i = 0; i < conv2_w_size; i++)
    conv2_w_fixed[i] = conv2_w[i];
  for (int i = 0; i < CONV2_OUT_CH; i++)
    conv2_b_fixed[i] = conv2_b[i];
  for (int i = 0; i < fc1_w_size; i++)
    fc1_w_fixed[i] = fc1_w[i];
  for (int i = 0; i < FC1_OUT_SIZE; i++)
    fc1_b_fixed[i] = fc1_b[i];
  for (int i = 0; i < fc2_w_size; i++)
    fc2_w_fixed[i] = fc2_w[i];
  for (int i = 0; i < FC2_OUT_SIZE; i++)
    fc2_b_fixed[i] = fc2_b[i];

  // Run HLS implementation
  std::cout << "Running HLS implementation..." << std::endl;
  uut_top(input_fixed, conv1_w_fixed, conv1_b_fixed, conv2_w_fixed,
          conv2_b_fixed, fc1_w_fixed, fc1_b_fixed, fc2_w_fixed, fc2_b_fixed,
          output_fixed);

  // Compute golden reference
  std::cout << "Computing golden reference..." << std::endl;
  compute_golden(input, conv1_w, conv1_b, conv2_w, conv2_b, fc1_w, fc1_b, fc2_w,
                 fc2_b, output_golden);

  // Compare outputs
  std::cout << "Comparing outputs..." << std::endl;
  golden_data_t tolerance = 0.1; // Tolerance for fixed-point arithmetic
  bool pass =
      compare_outputs(output_fixed, output_golden, FC2_OUT_SIZE, tolerance);

  // Print results
  std::cout << "\n========================================" << std::endl;
  if (pass) {
    std::cout << "TEST PASSED!" << std::endl;
  } else {
    std::cout << "TEST FAILED!" << std::endl;
  }
  std::cout << "========================================" << std::endl;

  // Cleanup
  delete[] input;
  delete[] conv1_w;
  delete[] conv1_b;
  delete[] conv2_w;
  delete[] conv2_b;
  delete[] fc1_w;
  delete[] fc1_b;
  delete[] fc2_w;
  delete[] fc2_b;
  delete[] output_golden;
  delete[] input_fixed;
  delete[] conv1_w_fixed;
  delete[] conv1_b_fixed;
  delete[] conv2_w_fixed;
  delete[] conv2_b_fixed;
  delete[] fc1_w_fixed;
  delete[] fc1_b_fixed;
  delete[] fc2_w_fixed;
  delete[] fc2_b_fixed;
  delete[] output_fixed;

  return pass ? 0 : 1;
}
