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

# Targets
.PHONY: all clean unit_test integration_test hls_test hls_csim hls_synth hls_cosim hls_export help

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
	@echo ""

# Unit test (CPU only)
unit_test: $(BUILD_DIR)/unit_test
	@echo "Running unit tests..."
	@$(BUILD_DIR)/unit_test

$(BUILD_DIR)/unit_test: $(TEST_SW_DIR)/unit_test.cpp $(SRC_DIR)/hls_cnn.h $(SRC_DIR)/cnn_marco.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

# Integration test (CPU only)
integration_test: $(BUILD_DIR)/integration_test
	@echo "Running integration test..."
	@$(BUILD_DIR)/integration_test

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

.DEFAULT_GOAL := help
