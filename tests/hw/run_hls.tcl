############################################################
# HLS CNN Project - Vitis HLS TCL Script
# Copyright 2025 HLS CNN Project
############################################################

# Flow control flags
set CSIM 0
set CSYNTH 1
set COSIM 0
set VIVADO_SYN 1
set VIVADO_IMPL 1

# Directory setup
set CUR_DIR [pwd]
set PROJ_ROOT $CUR_DIR/../..
set SRC_DIR $PROJ_ROOT/src

# Project configuration
set PROJ "hls_cnn.prj"
set SOLN "sol"

# Device part configuration
# Check if HLS_PART is set in environment (from Makefile)
if {[info exists ::env(HLS_PART)]} {
  set XPART $::env(HLS_PART)
  puts "Using device part from environment: $XPART"
} else {
  # Default part if not specified
  set XPART xc7z020clg400-1
  puts "Using default device part: $XPART"
}

# Clock period (100 MHz = 10 ns)
if {![info exists CLKP]} {
  set CLKP 10
}

# Create/reset project
open_project -reset $PROJ

# Add design files - REMOVED -DUSE_FLOAT to use ap_fixed<16,8> for hardware
add_files "${SRC_DIR}/hls_cnn.cpp" -cflags "-I${SRC_DIR} -std=c++14"
add_files "${CUR_DIR}/uut_top.cpp" -cflags "-I${SRC_DIR} -I${CUR_DIR} -std=c++14"

# Add testbench files - keep USE_FLOAT for C simulation accuracy
add_files -tb "${CUR_DIR}/test.cpp" -cflags "-I${SRC_DIR} -I${CUR_DIR} -std=c++14 -DUSE_FLOAT"

# Set top function
set_top uut_top

# Create/reset solution
open_solution -reset $SOLN

# Set target device and clock
set_part $XPART
create_clock -period $CLKP

# Load HLS configuration to fix DSP48E1 OPMODE issues
source "${CUR_DIR}/hls_config.tcl"

# Run C simulation
if {$CSIM == 1} {
  puts "========================================="
  puts "Running C Simulation..."
  puts "========================================="
  csim_design
}

# Run C synthesis
if {$CSYNTH == 1} {
  puts "========================================="
  puts "Running C Synthesis..."
  puts "========================================="
  csynth_design
}

# Run Co-simulation
if {$COSIM == 1} {
  puts "========================================="
  puts "Running Co-Simulation..."
  puts "========================================="
  cosim_design -rtl verilog
}

# Export design for Vivado synthesis
if {$VIVADO_SYN == 1} {
  puts "========================================="
  puts "Exporting Design for Synthesis..."
  puts "========================================="
  export_design -flow syn -rtl verilog
}

# Export design for Vivado implementation
if {$VIVADO_IMPL == 1} {
  puts "========================================="
  puts "Exporting Design for Implementation..."
  puts "========================================="
  export_design -flow impl -rtl verilog
}

puts "========================================="
puts "HLS Flow Completed Successfully!"
puts "========================================="

exit