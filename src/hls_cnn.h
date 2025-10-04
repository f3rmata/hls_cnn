/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef HLS_CNN_H
#define HLS_CNN_H

#include "ap_int.h"
#include "cnn_marco.h"
#include "hls_math.h"
#include "hls_stream.h"

namespace hls_cnn {

// ========== Activation Functions ==========

template <typename T> inline T relu(T x) {
#pragma HLS INLINE
  return (x > T(0)) ? x : T(0);
}

template <typename T> inline T sigmoid(T x) {
#pragma HLS INLINE
  // Hardware-friendly sigmoid approximation using piecewise linear
  // This avoids exp() which is expensive in hardware

  // Clamp to reasonable range
  if (x > T(5.0))
    return T(1.0);
  if (x < T(-5.0))
    return T(0.0);

#ifdef USE_FLOAT
  // Use accurate exp() for C simulation only
  return T(1.0) / (T(1.0) + exp(-float(x)));
#else
  // Piecewise linear approximation for hardware
  // sigmoid(x) ≈ 0.5 + 0.25*x for |x| < 2
  if (x > T(2.0))
    return T(1.0);
  if (x < T(-2.0))
    return T(0.0);
  return T(0.5) + T(0.25) * x;
#endif
}

// ========== Convolution Layer ==========

/**
 * @brief 2D Convolution with flexible parameters
 * @param input Input feature map [in_ch][height][width]
 * @param weights Convolution weights [out_ch][in_ch][kernel_h][kernel_w]
 * @param bias Bias terms [out_ch]
 * @param output Output feature map [out_ch][out_height][out_width]
 */
template <int IN_CH, int OUT_CH, int IMG_H, int IMG_W, int KERNEL_SIZE>
void conv2d(
    data_t input[IN_CH][IMG_H][IMG_W],
    weight_t weights[OUT_CH][IN_CH][KERNEL_SIZE][KERNEL_SIZE],
    weight_t bias[OUT_CH],
    data_t output[OUT_CH][IMG_H - KERNEL_SIZE + 1][IMG_W - KERNEL_SIZE + 1]) {
#pragma HLS INLINE off
// Reduced array partitioning for resource optimization
#pragma HLS ARRAY_PARTITION variable = bias complete

  const int OUT_H = IMG_H - KERNEL_SIZE + 1;
  const int OUT_W = IMG_W - KERNEL_SIZE + 1;

// Output channels loop
CONV_OUT_CH:
  for (int oc = 0; oc < OUT_CH; oc++) {
#pragma HLS LOOP_TRIPCOUNT min = 6 max = 16

  // Output spatial loops
  CONV_OUT_H:
    for (int oh = 0; oh < OUT_H; oh++) {
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 24

    CONV_OUT_W:
      for (int ow = 0; ow < OUT_W; ow++) {
#pragma HLS LOOP_TRIPCOUNT min = 8 max = 24
// Increased II to save LUT resources (II=4 uses fewer multiplexers)
#pragma HLS PIPELINE II = 4

        acc_t sum = bias[oc];

      // Input channels loop
      CONV_IN_CH:
        for (int ic = 0; ic < IN_CH; ic++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 16

        // Kernel loops
        CONV_KH:
          for (int kh = 0; kh < KERNEL_SIZE; kh++) {
#pragma HLS LOOP_TRIPCOUNT min = 5 max = 5

          CONV_KW:
            for (int kw = 0; kw < KERNEL_SIZE; kw++) {
#pragma HLS LOOP_TRIPCOUNT min = 5 max = 5

              int ih = oh + kh;
              int iw = ow + kw;
              sum += input[ic][ih][iw] * weights[oc][ic][kh][kw];
            }
          }
        }

        output[oc][oh][ow] = relu(sum);
      }
    }
  }
}

// ========== Max Pooling Layer ==========

/**
 * @brief 2D Max Pooling
 * @param input Input feature map [channels][height][width]
 * @param output Output feature map [channels][height/pool][width/pool]
 */
template <int CHANNELS, int IMG_H, int IMG_W, int POOL_SIZE>
void max_pool2d(data_t input[CHANNELS][IMG_H][IMG_W],
                data_t output[CHANNELS][IMG_H / POOL_SIZE][IMG_W / POOL_SIZE]) {
#pragma HLS INLINE off

  const int OUT_H = IMG_H / POOL_SIZE;
  const int OUT_W = IMG_W / POOL_SIZE;

POOL_CH:
  for (int c = 0; c < CHANNELS; c++) {
#pragma HLS LOOP_TRIPCOUNT min = 6 max = 16

  POOL_OH:
    for (int oh = 0; oh < OUT_H; oh++) {
#pragma HLS LOOP_TRIPCOUNT min = 4 max = 12

    POOL_OW:
      for (int ow = 0; ow < OUT_W; ow++) {
#pragma HLS LOOP_TRIPCOUNT min = 4 max = 12
// Increased II to save LUT resources
#pragma HLS PIPELINE II = 4

        data_t max_val = input[c][oh * POOL_SIZE][ow * POOL_SIZE];

      POOL_PH:
        for (int ph = 0; ph < POOL_SIZE; ph++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2

        POOL_PW:
          for (int pw = 0; pw < POOL_SIZE; pw++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2

            int ih = oh * POOL_SIZE + ph;
            int iw = ow * POOL_SIZE + pw;
            data_t val = input[c][ih][iw];

            if (val > max_val) {
              max_val = val;
            }
          }
        }

        output[c][oh][ow] = max_val;
      }
    }
  }
}

// ========== Fully Connected Layer ==========

/**
 * @brief Fully connected (dense) layer using GEMM-like approach
 * @param input Input vector [in_size]
 * @param weights Weight matrix [out_size][in_size]
 * @param bias Bias vector [out_size]
 * @param output Output vector [out_size]
 */
template <int IN_SIZE, int OUT_SIZE>
void fully_connected(data_t input[IN_SIZE], weight_t weights[OUT_SIZE][IN_SIZE],
                     weight_t bias[OUT_SIZE], data_t output[OUT_SIZE],
                     bool use_relu = true) {
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable = bias complete

FC_OUT:
  for (int o = 0; o < OUT_SIZE; o++) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 84

    acc_t sum = bias[o];

  FC_IN:
    for (int i = 0; i < IN_SIZE; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 128
// Increased II to save LUT resources
#pragma HLS PIPELINE II = 4

      sum += input[i] * weights[o][i];
    }

    output[o] = use_relu ? relu(sum) : sum;
  }
}

// ========== Flatten Layer ==========

/**
 * @brief Flatten 3D tensor to 1D vector
 */
template <int CHANNELS, int HEIGHT, int WIDTH>
void flatten(data_t input[CHANNELS][HEIGHT][WIDTH],
             data_t output[CHANNELS * HEIGHT * WIDTH]) {
#pragma HLS INLINE off

  int idx = 0;
FLATTEN_C:
  for (int c = 0; c < CHANNELS; c++) {
  FLATTEN_H:
    for (int h = 0; h < HEIGHT; h++) {
    FLATTEN_W:
      for (int w = 0; w < WIDTH; w++) {
// Increased II to save LUT resources
#pragma HLS PIPELINE II = 4
        output[idx++] = input[c][h][w];
      }
    }
  }
} // ========== Top-level CNN Inference ==========

/**
 * @brief Complete CNN inference pipeline declaration
 */
void cnn_inference(data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE],
                   weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH]
                                         [CONV1_KERNEL_SIZE][CONV1_KERNEL_SIZE],
                   weight_t conv1_bias[CONV1_OUT_CH],
                   weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH]
                                         [CONV2_KERNEL_SIZE][CONV2_KERNEL_SIZE],
                   weight_t conv2_bias[CONV2_OUT_CH],
                   weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE],
                   weight_t fc1_bias[FC1_OUT_SIZE],
                   weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE],
                   weight_t fc2_bias[FC2_OUT_SIZE],
                   data_t output[FC2_OUT_SIZE]);

} // namespace hls_cnn

#endif // HLS_CNN_H
