/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef CNN_MARCO_H
#define CNN_MARCO_H

#include "ap_fixed.h"
#include "ap_int.h"

// Network configuration parameters
#define MAX_IMG_HEIGHT 32
#define MAX_IMG_WIDTH 32
#define MAX_CHANNELS 64
#define MAX_KERNEL_SIZE 5
#define MAX_POOL_SIZE 2

// Layer configuration
#define CONV1_IN_CH 1
#define CONV1_OUT_CH 16
#define CONV1_KERNEL_SIZE 3
#define CONV1_IMG_SIZE 28

#define POOL1_SIZE 2
#define POOL1_IMG_SIZE (CONV1_IMG_SIZE - CONV1_KERNEL_SIZE + 1)
#define POOL1_OUT_SIZE (POOL1_IMG_SIZE / POOL1_SIZE)

#define CONV2_IN_CH 16
#define CONV2_OUT_CH 32
#define CONV2_KERNEL_SIZE 3
#define CONV2_IMG_SIZE (POOL1_IMG_SIZE / POOL1_SIZE)

#define POOL2_SIZE 2
#define POOL2_IMG_SIZE (CONV2_IMG_SIZE - CONV2_KERNEL_SIZE + 1)
#define POOL2_OUT_SIZE (POOL2_IMG_SIZE / POOL2_SIZE)

#define FC1_IN_SIZE                                                            \
  (CONV2_OUT_CH * (POOL2_IMG_SIZE / POOL2_SIZE) * (POOL2_IMG_SIZE / POOL2_SIZE))
#define FC1_OUT_SIZE 128

#define FC2_IN_SIZE 128
#define FC2_OUT_SIZE 10

// Hardware-compatible data types
// Using ap_fixed<W, I> where W=total bits, I=integer bits
// ap_fixed<16, 8> = 16 bits total, 8 integer bits, 8 fractional bits
// Range: [-128, 127.99609375] with precision 1/256

#ifndef USE_FLOAT
// Fixed-point types for hardware synthesis
typedef ap_fixed<16, 8> data_t;   // Data values: 16-bit fixed-point
typedef ap_fixed<16, 8> weight_t; // Weight values: 16-bit fixed-point
typedef ap_fixed<32, 16> acc_t;   // Accumulator: 32-bit for higher precision
#else
// Floating-point types for C simulation and validation
typedef float data_t;
typedef float weight_t;
typedef float acc_t;
#endif

#endif // CNN_MARCO_H
