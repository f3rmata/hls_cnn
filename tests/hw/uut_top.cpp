/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "uut_top.hpp"
#include "../../src/hls_cnn.h"

/**
 * @brief CNN Inference Hardware Top Function Implementation
 *
 * This function wraps the CNN inference pipeline with proper HLS interface
 * pragmas for hardware synthesis. It converts flattened input arrays into
 * multi-dimensional arrays expected by the CNN layers.
 */
void uut_top(data_t *input, weight_t *conv1_weights, weight_t *conv1_bias,
             weight_t *conv2_weights, weight_t *conv2_bias,
             weight_t *fc1_weights, weight_t *fc1_bias, weight_t *fc2_weights,
             weight_t *fc2_bias, data_t *output) {
// HLS Interface pragmas for AXI integration
#pragma HLS INTERFACE mode = s_axilite port = return

// AXI Master interfaces for memory access
#pragma HLS INTERFACE mode = m_axi depth = 784 port = input offset =           \
    slave bundle = gmem0
#pragma HLS INTERFACE mode = m_axi depth = 432 port = conv1_weights offset =   \
    slave bundle = gmem1
#pragma HLS INTERFACE mode = m_axi depth = 16 port = conv1_bias offset =       \
    slave bundle = gmem1
#pragma HLS INTERFACE mode = m_axi depth = 4608 port = conv2_weights offset =  \
    slave bundle = gmem2
#pragma HLS INTERFACE mode = m_axi depth = 32 port = conv2_bias offset =       \
    slave bundle = gmem2
#pragma HLS INTERFACE mode = m_axi depth = 102400 port = fc1_weights offset =  \
    slave bundle = gmem3
#pragma HLS INTERFACE mode = m_axi depth = 128 port = fc1_bias offset =        \
    slave bundle = gmem3
#pragma HLS INTERFACE mode = m_axi depth = 1280 port = fc2_weights offset =    \
    slave bundle = gmem4
#pragma HLS INTERFACE mode = m_axi depth = 10 port = fc2_bias offset =         \
    slave bundle = gmem4
#pragma HLS INTERFACE mode = m_axi depth = 10 port = output offset =           \
    slave bundle = gmem5

  // Reshape flattened input to 3D array
  static data_t input_reshaped[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE];
#pragma HLS ARRAY_PARTITION variable = input_reshaped dim = 1 complete

RESHAPE_INPUT:
  for (int c = 0; c < CONV1_IN_CH; c++) {
    for (int h = 0; h < CONV1_IMG_SIZE; h++) {
      for (int w = 0; w < CONV1_IMG_SIZE; w++) {
#pragma HLS PIPELINE II = 1
        int idx = c * CONV1_IMG_SIZE * CONV1_IMG_SIZE + h * CONV1_IMG_SIZE + w;
        input_reshaped[c][h][w] = input[idx];
      }
    }
  }

  // Reshape Conv1 weights
  static weight_t conv1_w[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE]
                         [CONV1_KERNEL_SIZE];
#pragma HLS ARRAY_PARTITION variable = conv1_w dim = 1 cyclic factor = 4

RESHAPE_CONV1_W:
  for (int oc = 0; oc < CONV1_OUT_CH; oc++) {
    for (int ic = 0; ic < CONV1_IN_CH; ic++) {
      for (int kh = 0; kh < CONV1_KERNEL_SIZE; kh++) {
        for (int kw = 0; kw < CONV1_KERNEL_SIZE; kw++) {
#pragma HLS PIPELINE II = 1
          int idx = oc * CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE +
                    ic * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE +
                    kh * CONV1_KERNEL_SIZE + kw;
          conv1_w[oc][ic][kh][kw] = conv1_weights[idx];
        }
      }
    }
  }

  // Reshape Conv1 bias
  static weight_t conv1_b[CONV1_OUT_CH];
#pragma HLS ARRAY_PARTITION variable = conv1_b complete

RESHAPE_CONV1_B:
  for (int i = 0; i < CONV1_OUT_CH; i++) {
#pragma HLS PIPELINE II = 1
    conv1_b[i] = conv1_bias[i];
  }

  // Reshape Conv2 weights
  static weight_t conv2_w[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE]
                         [CONV2_KERNEL_SIZE];
#pragma HLS ARRAY_PARTITION variable = conv2_w dim = 1 cyclic factor = 4

RESHAPE_CONV2_W:
  for (int oc = 0; oc < CONV2_OUT_CH; oc++) {
    for (int ic = 0; ic < CONV2_IN_CH; ic++) {
      for (int kh = 0; kh < CONV2_KERNEL_SIZE; kh++) {
        for (int kw = 0; kw < CONV2_KERNEL_SIZE; kw++) {
#pragma HLS PIPELINE II = 1
          int idx = oc * CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE +
                    ic * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE +
                    kh * CONV2_KERNEL_SIZE + kw;
          conv2_w[oc][ic][kh][kw] = conv2_weights[idx];
        }
      }
    }
  }

  // Reshape Conv2 bias
  static weight_t conv2_b[CONV2_OUT_CH];
#pragma HLS ARRAY_PARTITION variable = conv2_b complete

RESHAPE_CONV2_B:
  for (int i = 0; i < CONV2_OUT_CH; i++) {
#pragma HLS PIPELINE II = 1
    conv2_b[i] = conv2_bias[i];
  }

  // Reshape FC1 weights
  static weight_t fc1_w[FC1_OUT_SIZE][FC1_IN_SIZE];
#pragma HLS ARRAY_PARTITION variable = fc1_w dim = 1 cyclic factor = 8

RESHAPE_FC1_W:
  for (int o = 0; o < FC1_OUT_SIZE; o++) {
    for (int i = 0; i < FC1_IN_SIZE; i++) {
#pragma HLS PIPELINE II = 1
      int idx = o * FC1_IN_SIZE + i;
      fc1_w[o][i] = fc1_weights[idx];
    }
  }

  // Reshape FC1 bias
  static weight_t fc1_b[FC1_OUT_SIZE];
#pragma HLS ARRAY_PARTITION variable = fc1_b complete

RESHAPE_FC1_B:
  for (int i = 0; i < FC1_OUT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
    fc1_b[i] = fc1_bias[i];
  }

  // Reshape FC2 weights
  static weight_t fc2_w[FC2_OUT_SIZE][FC2_IN_SIZE];
#pragma HLS ARRAY_PARTITION variable = fc2_w dim = 1 complete

RESHAPE_FC2_W:
  for (int o = 0; o < FC2_OUT_SIZE; o++) {
    for (int i = 0; i < FC2_IN_SIZE; i++) {
#pragma HLS PIPELINE II = 1
      int idx = o * FC2_IN_SIZE + i;
      fc2_w[o][i] = fc2_weights[idx];
    }
  }

  // Reshape FC2 bias
  static weight_t fc2_b[FC2_OUT_SIZE];
#pragma HLS ARRAY_PARTITION variable = fc2_b complete

RESHAPE_FC2_B:
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
    fc2_b[i] = fc2_bias[i];
  }

  // Output buffer
  static data_t output_buf[FC2_OUT_SIZE];
#pragma HLS ARRAY_PARTITION variable = output_buf complete

  // Call the CNN inference function
  hls_cnn::cnn_inference(input_reshaped, conv1_w, conv1_b, conv2_w, conv2_b,
                         fc1_w, fc1_b, fc2_w, fc2_b, output_buf);

// Copy output
COPY_OUTPUT:
  for (int i = 0; i < FC2_OUT_SIZE; i++) {
#pragma HLS PIPELINE II = 1
    output[i] = output_buf[i];
  }
}
