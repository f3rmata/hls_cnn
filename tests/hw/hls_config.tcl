############################################################
# HLS Configuration for Zynq 7020 Resource Optimization
# Ultra-aggressive optimization for strict LUT constraints
############################################################

# Solution configuration
config_compile -pipeline_loops 64

# Schedule configuration - enable resource sharing
config_schedule -effort medium -enable_dsp_full_reg

# RTL generation configuration
config_rtl -reset all -reset_async -reset_level low
config_rtl -module_auto_prefix

# Array optimization - disable automatic partitioning
config_array_partition -complete_threshold 0

# Interface configuration
config_interface -m_axi_addr64
config_interface -m_axi_alignment_byte_size 64

puts "HLS Configuration loaded successfully (Ultra-optimized for Zynq 7020)"
