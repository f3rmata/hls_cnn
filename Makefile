# Makefile for HLS CNN Project
# Copyright 2025 HLS CNN Project

############################## Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

# Directories
SRC_DIR := src
TEST_DIR := tests
TEST_SW_DIR := $(TEST_DIR)/sw
TEST_HW_DIR := $(TEST_DIR)/hw
BUILD_DIR := build

# C++ Compiler
CXX := g++
CXXFLAGS := -std=c++14 -I$(SRC_DIR) -Wall -Wextra

############################## Environment Check ##############################
# Check for Xilinx tools installation
.PHONY: check_vivado check_vitis

check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "ERROR: Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

check_vitis:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/vitis))
	@echo "ERROR: Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

# Vitis HLS executable
VPP := vitis_hls

# Add Vivado to PATH
export PATH := $(XILINX_VIVADO)/bin:$(PATH)

# Set library path for Vitis tools
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif

# Xilinx HLS include path
XILINX_HLS := /home/fermata/Development/Software/Xilinx/Vitis_HLS/2024.1/include
ifneq (,$(wildcard $(XILINX_HLS)))
CXXFLAGS += -I$(XILINX_HLS)
endif

############################## Device Configuration ##############################
# Default platform and part
PLATFORM ?= xilinx_u200_gen3x16_xdma_2_202110_1
XPART ?= xc7z020clg400-1

# Allow XPART to be set via environment or command line
# Example: make hls_csim XPART=xc7z020clg400-1
ifneq (,$(XPART))
export HLS_PART := $(XPART)
else
export HLS_PART := xc7z020clg400-1
endif

# Display configured part
.PHONY: show_config
show_config:
	@echo "=========================================="
	@echo "HLS CNN Project Configuration"
	@echo "=========================================="
	@echo "XILINX_VIVADO : $(XILINX_VIVADO)"
	@echo "XILINX_VITIS  : $(XILINX_VITIS)"
	@echo "HLS_PART      : $(HLS_PART)"
	@echo "CUR_DIR       : $(CUR_DIR)"
	@echo "VPP           : $(VPP)"
	@echo "=========================================="

# MNIST test directory
TEST_MNIST_DIR := $(TEST_DIR)/mnist

# Targets
.PHONY: all clean unit_test integration_test hls_test hls_csim hls_synth hls_cosim hls_export help
.PHONY: mnist_download mnist_train mnist_test mnist_test_quick mnist_test_validation mnist_test_full
.PHONY: mnist_inference mnist_inference_quick mnist_inference_validation mnist_inference_full

all: unit_test integration_test

help:
	@echo ""
	@echo "=========================================="
	@echo "HLS CNN Project - Build Targets"
	@echo "=========================================="
	@echo ""
	@echo "Configuration:"
	@echo "  make show_config       - Show current configuration"
	@echo ""
	@echo "CPU Testing:"
	@echo "  make unit_test          - Build and run unit tests (CPU)"
	@echo "  make integration_test   - Build and run integration test (CPU)"
	@echo ""
	@echo "MNIST Testing (Random Weights):"
	@echo "  make mnist_download     - Download MNIST dataset"
	@echo "  make mnist_test_quick   - Test on 10 MNIST images (fast)"
	@echo "  make mnist_test_validation - Test on 100 MNIST images"
	@echo "  make mnist_test_full    - Test on 10,000 MNIST images (slow)"
	@echo ""
	@echo "MNIST Testing (Trained Weights):"
	@echo "  make mnist_train        - Train CNN and export weights (requires PyTorch)"
	@echo "  make mnist_inference_quick - Inference on 10 images with trained weights"
	@echo "  make mnist_inference_validation - Inference on 100 images"
	@echo "  make mnist_inference_full - Inference on 10,000 images"
	@echo ""
	@echo "HLS Hardware Flow:"
	@echo "  make hls_test          - Run all HLS tests (C sim)"
	@echo "  make hls_csim          - Run HLS C simulation"
	@echo "  make hls_synth         - Run HLS synthesis only"
	@echo "  make hls_cosim         - Run HLS co-simulation (RTL verification)"
	@echo "  make hls_export        - Export IP for Vivado integration"
	@echo "  make hls_full          - Run complete HLS flow (csim + synth + cosim)"
	@echo ""
	@echo "Device Configuration:"
	@echo "  XPART=<part_name>      - Specify FPGA part (default: xc7z020clg400-1)"
	@echo "  Example: make hls_csim XPART=xc7z020clg400-1"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean             - Clean all build artifacts"
	@echo "  make clean_hls         - Clean only HLS project files"
	@echo "  make clean_mnist       - Clean MNIST data and weights"
	@echo ""

# Unit test (CPU only)
unit_test: $(BUILD_DIR)/unit_test
	@echo "Running unit tests..."
	@LD_LIBRARY_PATH="" $(BUILD_DIR)/unit_test

$(BUILD_DIR)/unit_test: $(TEST_SW_DIR)/unit_test.cpp $(SRC_DIR)/hls_cnn.h $(SRC_DIR)/cnn_marco.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

# Integration test (CPU only)
integration_test: $(BUILD_DIR)/integration_test
	@echo "Running integration test..."
	@LD_LIBRARY_PATH="" $(BUILD_DIR)/integration_test

$(BUILD_DIR)/integration_test: $(TEST_SW_DIR)/integration_test.cpp $(SRC_DIR)/hls_cnn.cpp $(SRC_DIR)/hls_cnn.h $(SRC_DIR)/cnn_marco.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SW_DIR)/integration_test.cpp $(SRC_DIR)/hls_cnn.cpp -lm

# HLS Test (alias for C simulation)
hls_test: hls_csim

# HLS C Simulation (fast, uses testbench)
hls_csim: check_vivado
	@echo "=========================================="
	@echo "Running HLS C Simulation..."
	@echo "Part: $(HLS_PART)"
	@echo "=========================================="
	cd $(TEST_HW_DIR) && HLS_PART=$(HLS_PART) $(VPP) -f run_hls.tcl

# HLS Synthesis only
hls_synth: check_vivado
	@echo "=========================================="
	@echo "Running HLS Synthesis (CSIM=0)..."
	@echo "Part: $(HLS_PART)"
	@echo "=========================================="
	cd $(TEST_HW_DIR) && sed -i 's/set CSIM 1/set CSIM 0/' run_hls.tcl && \
	HLS_PART=$(HLS_PART) $(VPP) -f run_hls.tcl && \
	sed -i 's/set CSIM 0/set CSIM 1/' run_hls.tcl

# HLS Co-simulation (RTL verification - slow)
hls_cosim: check_vivado
	@echo "=========================================="
	@echo "Running HLS Co-simulation..."
	@echo "Part: $(HLS_PART)"
	@echo "Warning: This can take 10-30 minutes!"
	@echo "=========================================="
	cd $(TEST_HW_DIR) && sed -i 's/set COSIM 0/set COSIM 1/' run_hls.tcl && \
	HLS_PART=$(HLS_PART) $(VPP) -f run_hls.tcl && \
	sed -i 's/set COSIM 1/set COSIM 0/' run_hls.tcl

# HLS Export IP
hls_export: check_vivado
	@echo "=========================================="
	@echo "Exporting IP for Vivado..."
	@echo "Part: $(HLS_PART)"
	@echo "=========================================="
	cd $(TEST_HW_DIR) && sed -i 's/set VIVADO_SYN 0/set VIVADO_SYN 1/' run_hls.tcl && \
	HLS_PART=$(HLS_PART) $(VPP) -f run_hls.tcl && \
	sed -i 's/set VIVADO_SYN 1/set VIVADO_SYN 0/' run_hls.tcl

# Full HLS flow
hls_full: check_vivado
	@echo "=========================================="
	@echo "Running complete HLS flow (csim + synth + cosim)..."
	@echo "Part: $(HLS_PART)"
	@echo "=========================================="
	cd $(TEST_HW_DIR) && sed -i 's/set COSIM 0/set COSIM 1/' run_hls.tcl && \
	HLS_PART=$(HLS_PART) $(VPP) -f run_hls.tcl && \
	sed -i 's/set COSIM 1/set COSIM 0/' run_hls.tcl

# Clean
clean: clean_hls
	rm -rf $(BUILD_DIR)
	rm -rf vitis_hls.log
	@echo "Clean complete"

# Clean only HLS files
clean_hls:
	rm -rf $(TEST_HW_DIR)/hls_cnn.prj
	rm -rf $(TEST_HW_DIR)/*.log
	rm -rf $(TEST_HW_DIR)/logs
	@echo "HLS project files cleaned"

# Clean MNIST data and weights
clean_mnist:
	rm -rf $(TEST_MNIST_DIR)/data
	rm -rf $(TEST_MNIST_DIR)/weights
	rm -f $(BUILD_DIR)/mnist_test
	@echo "MNIST data and weights cleaned"

############################## MNIST Test Targets ##############################

# Download MNIST dataset
mnist_download:
	@echo "=========================================="
	@echo "Downloading MNIST Dataset"
	@echo "=========================================="
	cd $(TEST_MNIST_DIR) && python3 download_mnist.py

# Train CNN and export weights
mnist_train:
	@echo "=========================================="
	@echo "Training CNN on MNIST"
	@echo "=========================================="
	@which python3 > /dev/null || (echo "ERROR: python3 not found" && exit 1)
	@python3 -c "import torch" 2>/dev/null || (echo "ERROR: PyTorch not installed. Install with: pip3 install torch torchvision" && exit 1)
	cd $(TEST_MNIST_DIR) && python3 train_mnist.py --epochs 10

# Build MNIST test executable
$(BUILD_DIR)/mnist_test: $(TEST_MNIST_DIR)/mnist_test.cpp $(SRC_DIR)/hls_cnn.cpp $(SRC_DIR)/hls_cnn.h $(SRC_DIR)/cnn_marco.h
	@mkdir -p $(BUILD_DIR)
	@echo "Building MNIST test..."
	$(CXX) $(CXXFLAGS) -DUSE_FLOAT -o $@ $(TEST_MNIST_DIR)/mnist_test.cpp $(SRC_DIR)/hls_cnn.cpp -lm

# Quick MNIST test (10 images)
mnist_test_quick: $(BUILD_DIR)/mnist_test
	@echo "=========================================="
	@echo "Running Quick MNIST Test (10 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/data/quick_test_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_test quick

# Validation MNIST test (100 images)
mnist_test_validation: $(BUILD_DIR)/mnist_test
	@echo "=========================================="
	@echo "Running Validation MNIST Test (100 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/data/validation_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_test validation

# Full MNIST test (10,000 images)
mnist_test_full: $(BUILD_DIR)/mnist_test
	@echo "=========================================="
	@echo "Running Full MNIST Test (10,000 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/data/test_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_test test

# Convenient alias
mnist_test: mnist_test_quick

# Build MNIST inference executable (with trained weights)
$(BUILD_DIR)/mnist_inference: $(TEST_MNIST_DIR)/mnist_inference.cpp $(SRC_DIR)/hls_cnn.cpp $(SRC_DIR)/hls_cnn.h $(SRC_DIR)/cnn_marco.h
	@mkdir -p $(BUILD_DIR)
	@echo "Building MNIST inference test..."
	$(CXX) $(CXXFLAGS) -DUSE_FLOAT -o $@ $(TEST_MNIST_DIR)/mnist_inference.cpp $(SRC_DIR)/hls_cnn.cpp -lm

# Quick MNIST inference test (10 images with trained weights)
mnist_inference_quick: $(BUILD_DIR)/mnist_inference
	@echo "=========================================="
	@echo "Running Quick MNIST Inference (10 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/weights/conv1_weights.bin ] || (echo "ERROR: Trained weights not found. Run 'make mnist_train' first." && exit 1)
	@[ -f $(TEST_MNIST_DIR)/data/quick_test_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_inference quick

# Validation MNIST inference test (100 images with trained weights)
mnist_inference_validation: $(BUILD_DIR)/mnist_inference
	@echo "=========================================="
	@echo "Running Validation MNIST Inference (100 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/weights/conv1_weights.bin ] || (echo "ERROR: Trained weights not found. Run 'make mnist_train' first." && exit 1)
	@[ -f $(TEST_MNIST_DIR)/data/validation_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_inference validation

# Full MNIST inference test (10,000 images with trained weights)
mnist_inference_full: $(BUILD_DIR)/mnist_inference
	@echo "=========================================="
	@echo "Running Full MNIST Inference (10,000 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/weights/conv1_weights.bin ] || (echo "ERROR: Trained weights not found. Run 'make mnist_train' first." && exit 1)
	@[ -f $(TEST_MNIST_DIR)/data/test_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_inference test
	@echo "=========================================="
	@echo "Running Full MNIST Inference (10,000 images)"
	@echo "=========================================="
	@[ -f $(TEST_MNIST_DIR)/weights/conv1_weights.bin ] || (echo "ERROR: Trained weights not found. Run 'make mnist_train' first." && exit 1)
	@[ -f $(TEST_MNIST_DIR)/data/test_images.bin ] || (echo "ERROR: MNIST data not found. Run 'make mnist_download' first." && exit 1)
	cd $(TEST_MNIST_DIR) && ../../$(BUILD_DIR)/mnist_inference test

# Convenient alias
mnist_inference: mnist_inference_quick

.DEFAULT_GOAL := help
