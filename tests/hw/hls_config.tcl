############################################################
# HLS Configuration for DSP48E1 Optimization
# This file contains advanced configuration to avoid DSP OPMODE issues
############################################################

# Solution configuration
config_compile -pipeline_loops 64
config_compile -unsafe_math_optimizations

# Schedule configuration
config_schedule -effort high
config_schedule -enable_dsp_full_reg
config_schedule -relax_ii_for_timing

# Floating-point operations configuration
# fadd: Floating-point addition
# - fabric: Use LUTs instead of DSP (safer, but uses more LUTs)
# - fulldsp: Use full DSP implementation (may cause OPMODE issues)
# - meddsp: Use medium DSP implementation (balanced)
# - nodsp: No DSP at all
config_op fadd -impl fabric -latency 3

# fmul: Floating-point multiplication
# - maxdsp: Maximum DSP usage (recommended for multiply)
# - meddsp: Medium DSP usage
config_op fmul -impl maxdsp -latency 2

# fsub: Floating-point subtraction (similar to fadd)
config_op fsub -impl fabric -latency 3

# Other floating-point operations
config_op fdiv -impl fabric -latency 10
config_op fsqrt -impl fabric -latency 10
config_op fexp -impl fabric
config_op flog -impl fabric

# RTL generation configuration
config_rtl -reset all -reset_async -reset_level low
config_rtl -module_auto_prefix

# Array optimization
config_array_partition -complete_threshold 1024
# config_array_partition -throughput_driven

# Interface configuration
config_interface -m_axi_addr64
config_interface -m_axi_alignment_byte_size 64

puts "HLS Configuration loaded successfully"
