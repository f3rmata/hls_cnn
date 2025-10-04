############################################################
# HLS Configuration for Zynq 7020 Resource Optimization
# Optimized for LeNet-5 style CNN with ap_fixed<16,8>
############################################################

# Solution configuration
config_compile -pipeline_loops 64

# Schedule configuration (reduced aggressiveness)
config_schedule -effort medium

# RTL generation configuration
config_rtl -reset all -reset_async -reset_level low
config_rtl -module_auto_prefix

# Array optimization (very conservative for resource saving)
config_array_partition -complete_threshold 64

# Interface configuration
config_interface -m_axi_addr64
config_interface -m_axi_alignment_byte_size 64

puts "HLS Configuration loaded successfully (Zynq 7020 optimized with ap_fixed)"
