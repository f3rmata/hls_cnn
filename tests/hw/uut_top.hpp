/*
 * Copyright 2025 HLS CNN Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef UUT_TOP_HPP
#define UUT_TOP_HPP

#include "../../src/cnn_marco.h"
#include "ap_int.h"

// UUT (Unit Under Test) Top Function
// This is the hardware entry point for Vitis HLS synthesis

/**
 * @brief CNN Inference Hardware Top Function
 *
 * This function serves as the hardware-synthesizable entry point for
 * the CNN inference accelerator. It uses HLS interface pragmas for
 * proper hardware integration.
 *
 * @param input Input image [CONV1_IN_CH * CONV1_IMG_SIZE * CONV1_IMG_SIZE]
 * @param conv1_weights Conv1 weights (flattened)
 * @param conv1_bias Conv1 bias
 * @param conv2_weights Conv2 weights (flattened)
 * @param conv2_bias Conv2 bias
 * @param fc1_weights FC1 weights (flattened)
 * @param fc1_bias FC1 bias
 * @param fc2_weights FC2 weights (flattened)
 * @param fc2_bias FC2 bias
 * @param output Output logits [FC2_OUT_SIZE]
 */
void uut_top(
    // Input
    data_t *input,

    // Conv1 parameters (flattened arrays for AXI interface)
    weight_t *conv1_weights, weight_t *conv1_bias,

    // Conv2 parameters
    weight_t *conv2_weights, weight_t *conv2_bias,

    // FC1 parameters
    weight_t *fc1_weights, weight_t *fc1_bias,

    // FC2 parameters
    weight_t *fc2_weights, weight_t *fc2_bias,

    // Output
    data_t *output);

#endif // UUT_TOP_HPP
