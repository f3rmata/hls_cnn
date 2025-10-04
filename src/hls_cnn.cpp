/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "hls_cnn.h"

/**
 * @brief Complete CNN inference pipeline (LeNet-5 style optimized for Zynq
 * 7020)
 *
 * Architecture (optimized to reduce resource usage):
 * Input [1x28x28] -> Conv1 [6x24x24] -> Pool1 [6x12x12]
 * -> Conv2 [16x8x8] -> Pool2 [16x4x4] -> FC1 [84] -> FC2 [10]
 *
 * Changes from original:
 * - Conv1: 16->6 channels, kernel 3->5
 * - Conv2: 32->16 channels, kernel 3->5
 * - FC1: 128->84 outputs (input size reduced from 800 to 256)
 *
 * @param input Input image [1][28][28]
 * @param conv1_weights Conv1 weights [6][1][5][5]
 * @param conv1_bias Conv1 bias [6]
 * @param conv2_weights Conv2 weights [16][6][5][5]
 * @param conv2_bias Conv2 bias [16]
 * @param fc1_weights FC1 weights [84][256]
 * @param fc1_bias FC1 bias [84]
 * @param fc2_weights FC2 weights [10][84]
 * @param fc2_bias FC2 bias [10]
 * @param output Output logits [10]
 */
void hls_cnn::cnn_inference(
    // Input
    data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE],

    // Conv1 parameters
    weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE]
                          [CONV1_KERNEL_SIZE],
    weight_t conv1_bias[CONV1_OUT_CH],

    // Conv2 parameters
    weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE]
                          [CONV2_KERNEL_SIZE],
    weight_t conv2_bias[CONV2_OUT_CH],

    // FC1 parameters
    weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE],
    weight_t fc1_bias[FC1_OUT_SIZE],

    // FC2 parameters
    weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE],
    weight_t fc2_bias[FC2_OUT_SIZE],

    // Output
    data_t output[FC2_OUT_SIZE]) {
#pragma HLS INTERFACE mode = s_axilite port = return
#pragma HLS INTERFACE mode = m_axi depth = 784 port = input offset =           \
    slave bundle = gmem0
#pragma HLS INTERFACE mode = m_axi depth = 150 port = conv1_weights offset =   \
    slave bundle = gmem1
#pragma HLS INTERFACE mode = m_axi depth = 6 port = conv1_bias offset =        \
    slave bundle = gmem1
#pragma HLS INTERFACE mode = m_axi depth = 2400 port = conv2_weights offset =  \
    slave bundle = gmem2
#pragma HLS INTERFACE mode = m_axi depth = 16 port = conv2_bias offset =       \
    slave bundle = gmem2
#pragma HLS INTERFACE mode = m_axi depth = 21504 port = fc1_weights offset =   \
    slave bundle = gmem3
#pragma HLS INTERFACE mode = m_axi depth = 84 port = fc1_bias offset =         \
    slave bundle = gmem3
#pragma HLS INTERFACE mode = m_axi depth = 840 port = fc2_weights offset =     \
    slave bundle = gmem4
#pragma HLS INTERFACE mode = m_axi depth = 10 port = fc2_bias offset =         \
    slave bundle = gmem4
#pragma HLS INTERFACE mode = m_axi depth = 10 port = output offset =           \
    slave bundle = gmem5

  // Layer outputs - NO array partitioning to minimize LUT usage
  // Arrays are kept in BRAM with minimal multiplexing
  static data_t conv1_out[CONV1_OUT_CH][POOL1_IMG_SIZE][POOL1_IMG_SIZE];
  static data_t pool1_out[CONV1_OUT_CH][CONV2_IMG_SIZE][CONV2_IMG_SIZE];
  static data_t conv2_out[CONV2_OUT_CH][POOL2_IMG_SIZE][POOL2_IMG_SIZE];
  static data_t pool2_out[CONV2_OUT_CH][POOL2_IMG_SIZE / POOL2_SIZE]
                         [POOL2_IMG_SIZE / POOL2_SIZE];
  static data_t flatten_out[FC1_IN_SIZE];
  static data_t fc1_out[FC1_OUT_SIZE];

  // Layer 1: Conv + ReLU
  conv2d<CONV1_IN_CH, CONV1_OUT_CH, CONV1_IMG_SIZE, CONV1_IMG_SIZE,
         CONV1_KERNEL_SIZE>(input, conv1_weights, conv1_bias, conv1_out);

  // Layer 2: Max Pooling
  max_pool2d<CONV1_OUT_CH, POOL1_IMG_SIZE, POOL1_IMG_SIZE, POOL1_SIZE>(
      conv1_out, pool1_out);

  // Layer 3: Conv + ReLU
  conv2d<CONV2_IN_CH, CONV2_OUT_CH, CONV2_IMG_SIZE, CONV2_IMG_SIZE,
         CONV2_KERNEL_SIZE>(pool1_out, conv2_weights, conv2_bias, conv2_out);

  // Layer 4: Max Pooling
  max_pool2d<CONV2_OUT_CH, POOL2_IMG_SIZE, POOL2_IMG_SIZE, POOL2_SIZE>(
      conv2_out, pool2_out);

  // Layer 5: Flatten
  flatten<CONV2_OUT_CH, POOL2_IMG_SIZE / POOL2_SIZE,
          POOL2_IMG_SIZE / POOL2_SIZE>(pool2_out, flatten_out);

  // Layer 6: Fully Connected + ReLU
  fully_connected<FC1_IN_SIZE, FC1_OUT_SIZE>(flatten_out, fc1_weights, fc1_bias,
                                             fc1_out, true);

  // Layer 7: Fully Connected (output layer, no ReLU)
  fully_connected<FC2_IN_SIZE, FC2_OUT_SIZE>(fc1_out, fc2_weights, fc2_bias,
                                             output, false);
}
