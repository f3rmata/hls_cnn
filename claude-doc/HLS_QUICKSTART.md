# HLS流程快速指南

## 🚀 快速开始

### 1. C仿真 (验证功能)

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
make hls_csim
```

**预期输出**:
```
TEST PASSED!
INFO: [SIM 211-1] CSim done with 0 errors.
Maximum error: 0
```

**用途**: 验证CNN功能正确性，使用float类型，速度快

### 2. 综合 (生成硬件)

```bash
make hls_synth
```

**预期输出**:
```
Synthesis completed.
Performance & Resource Estimates:
+ Timing: 
  * Target clock: 10.00ns (100MHz)
  * Estimated clock: ~8.5ns
+ Latency: ...
+ Resources:
  * LUT: ~42,000 (79%)
  * FF: ~40,000 (37%)
  * DSP: ~90 (41%)
  * BRAM: ~140 (50%)
```

**用途**: 生成RTL，检查资源使用和时序

### 3. 协同仿真 (验证RTL)

```bash
make hls_cosim
```

**预期输出**:
```
RTL Co-simulation: PASS
Latency: ...cycles
Throughput: ...
```

**用途**: 验证RTL与C模型一致性

## 📋 完整流程

### 方案A: 分步执行 (推荐)

```bash
# 1. C仿真 (~10秒)
make hls_csim

# 2. 如果C仿真通过，运行综合 (~5-10分钟)
make hls_synth

# 3. 检查综合报告
cat tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt

# 4. 如果资源满意，运行协同仿真 (~30分钟)
make hls_cosim
```

### 方案B: 一键执行 (耗时长)

```bash
# 修改 run_hls.tcl，设置所有步骤为1
cd tests/hw
# 编辑 run_hls.tcl:
#   set CSIM 1
#   set CSYNTH 1
#   set COSIM 1

vitis_hls -f run_hls.tcl
```

## 🔧 高级选项

### 指定FPGA器件

```bash
# 默认: xc7z020clg400-1 (Zynq 7020)
make hls_csim

# 指定其他器件
make hls_csim XPART=xc7z020clg484-1
make hls_synth XPART=xc7z035ffg676-2
```

### 修改时钟频率

编辑 `tests/hw/run_hls.tcl`:
```tcl
# Clock period (100 MHz = 10 ns)
set CLKP 10  # 改为 8 = 125MHz, 或 12 = 83MHz
```

### 修改优化选项

编辑 `tests/hw/hls_config.tcl`:
```tcl
# Pipeline配置
config_compile -pipeline_loops 64  # 改为32或128

# DSP配置
config_schedule -enable_dsp_full_reg  # 移除此行禁用DSP全寄存器
```

## 📊 输出文件

### C仿真

```
tests/hw/hls_cnn.prj/sol/csim/
├── build/           # 编译的可执行文件
└── report/          # 仿真报告
```

### 综合

```
tests/hw/hls_cnn.prj/sol/syn/
├── report/
│   ├── uut_top_csynth.rpt       # 资源和性能报告
│   └── uut_top_csynth.xml       # XML格式报告
├── verilog/         # 生成的RTL代码
└── vhdl/            # VHDL代码(如果选择)
```

### 协同仿真

```
tests/hw/hls_cnn.prj/sol/sim/
├── report/          # 协同仿真报告
├── verilog/         # 仿真使用的RTL
└── wrapc/           # C wrapper文件
```

## 🐛 故障排除

### 问题1: undefined symbol错误

**现象**:
```
ld.lld: error: undefined symbol: uut_top(float*, ...)
```

**解决**: 已修复！确保使用最新的 `run_hls.tcl`

### 问题2: 综合资源超限

**现象**:
```
ERROR: [XFORM] Resource usage exceeds available resources
```

**解决**:
1. 检查 `hls_config.tcl` 优化选项
2. 减少数组分区
3. 降低pipeline目标

### 问题3: 时序不满足

**现象**:
```
WARNING: [SYN] Timing constraints not met
Estimated clock: 11.5ns (target: 10ns)
```

**解决**:
1. 增加时钟周期: `set CLKP 12`
2. 启用DSP全寄存器: `config_schedule -enable_dsp_full_reg`
3. 增加pipeline II: 修改源代码中的 `#pragma HLS PIPELINE II=8`

## 📚 查看报告

### 综合报告

```bash
# 性能和资源总览
cat tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | less

# 查找特定信息
grep "LUT" tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt
grep "Latency" tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt
```

### 关键指标

1. **资源使用**
   - LUT < 53,200 (Zynq 7020限制)
   - FF < 106,400
   - DSP < 220
   - BRAM < 280 (18K blocks)

2. **时序**
   - Clock period: 10ns (100MHz)
   - Slack: 应为正值

3. **延迟**
   - Min latency: 最小执行周期
   - Max latency: 最大执行周期
   - Interval: 吞吐率(每隔多少周期可处理一个新输入)

## 💡 提示

1. **C仿真优先**: 先确保C仿真通过再综合
2. **增量优化**: 一次调整一个参数
3. **保存报告**: 每次综合后保存报告文件
4. **版本控制**: 重要的配置提交到Git

## 📖 相关文档

- **[CSIM_FIX.md](CSIM_FIX.md)** - C仿真修复详细说明
- **[README_CSIM_FIX.md](README_CSIM_FIX.md)** - 快速修复总结
- **[Vitis HLS User Guide](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)**

---

**快速命令参考**:
```bash
# C仿真
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn && make hls_csim

# 综合
make hls_synth

# 查看报告
cat tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | grep -A 20 "Performance & Resource Estimates"
```
