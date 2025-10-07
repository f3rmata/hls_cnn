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

// =====================================================================
// Global limits
// =====================================================================
#define MAX_IMG_HEIGHT 32
#define MAX_IMG_WIDTH 32
#define MAX_CHANNELS 64
#define MAX_KERNEL_SIZE 5
#define MAX_POOL_SIZE 2

// =====================================================================
// Architecture Profiles
// Select one via compiler defines or default to BALANCED:
//   -DPROFILE_ULTRA    : minimum resources, lower throughput/accuracy
//   -DPROFILE_BALANCED : good accuracy with moderate resources
//   -DPROFILE_MAX      : highest accuracy, more resources
// =====================================================================
#if !defined(PROFILE_ULTRA) && !defined(PROFILE_BALANCED) && !defined(PROFILE_MAX)
#define PROFILE_BALANCED
#endif

// =====================================================================
// Layer configuration (LeNet-style) for Zynq-7020
// Derived dimensions are kept explicit to make HLS estimates predictable.
// =====================================================================
#define CONV1_IN_CH 1
#define CONV1_KERNEL_SIZE 5
#define CONV1_IMG_SIZE 28

#define POOL1_SIZE 2
#define POOL1_IMG_SIZE (CONV1_IMG_SIZE - CONV1_KERNEL_SIZE + 1) // 24
#define POOL1_OUT_SIZE (POOL1_IMG_SIZE / POOL1_SIZE)            // 12

#define CONV2_KERNEL_SIZE 5
#define CONV2_IMG_SIZE (POOL1_IMG_SIZE / POOL1_SIZE) // 12

#define POOL2_SIZE 2
#define POOL2_IMG_SIZE (CONV2_IMG_SIZE - CONV2_KERNEL_SIZE + 1) // 8
#define POOL2_OUT_SIZE (POOL2_IMG_SIZE / POOL2_SIZE)            // 4

// Profile-specific channel counts and pipeline target
#if defined(PROFILE_ULTRA)
// Ultra-low resource profile (original project defaults)
#define CONV1_OUT_CH 6
#define CONV2_IN_CH 6
#define CONV2_OUT_CH 8
#define FC1_OUT_SIZE 64
#define HLS_PIPELINE_II 8
#elif defined(PROFILE_MAX)
// Maximum accuracy profile (near classic LeNet-5 capacity)
#define CONV1_OUT_CH 8
#define CONV2_IN_CH CONV1_OUT_CH
#define CONV2_OUT_CH 16
#define FC1_OUT_SIZE 84
#define HLS_PIPELINE_II 2
#else
// Balanced profile (accuracy vs. resources)
#define CONV1_OUT_CH 6
#define CONV2_IN_CH CONV1_OUT_CH
#define CONV2_OUT_CH 12
#define FC1_OUT_SIZE 84
#define HLS_PIPELINE_II 4
#endif

// Flattened feature size after Pool2
#define FC1_IN_SIZE                                                            \
  (CONV2_OUT_CH * (POOL2_IMG_SIZE / POOL2_SIZE) *                              \
   (POOL2_IMG_SIZE / POOL2_SIZE))

// Output layer configuration
#define FC2_IN_SIZE FC1_OUT_SIZE
#define FC2_OUT_SIZE 10

// =====================================================================
// Hardware-compatible data types
// Using ap_fixed<W, I> where W=total bits, I=integer bits
// ap_fixed<16, 8> = 16 bits total, 8 integer bits, 8 fractional bits
// Range: [-128, 127.99609375] with precision 1/256
// =====================================================================
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
