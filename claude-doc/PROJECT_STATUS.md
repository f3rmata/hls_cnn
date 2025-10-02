# HLS CNN 项目状态 - 2025-10-02

## 📊 项目概览

**项目名称**: HLS CNN (Simplified LeNet-style CNN)  
**目标设备**: Xilinx Zynq-7020 (xc7z020-clg400-1)  
**工具版本**: Vitis HLS 2024.1  
**状态**: ✅ 开发就绪

## ✅ 最近修复的问题

### 1. DSP48E1 OPMODE 警告（已修复）
- **问题**: Co-simulation 中出现数百条 DSP OPMODE 警告
- **根因**: 浮点加法器使用了无效的 DSP48E1 配置
- **解决方案**: 
  - 创建 `hls_config.tcl` 配置浮点运算实现
  - 加法/减法使用 Fabric（LUT）实现
  - 乘法继续使用 DSP（保持效率）
- **效果**: OPMODE 警告从数百条降至 0-5 条
- **文档**: `tests/hw/DSP_FIX_SUMMARY.md`

### 2. Makefile 路径不一致（已修复）
- **问题**: 
  - HLS 命令在错误目录 (`tests/`) 运行
  - 项目名称不匹配 (`hls_cnn_prj` vs `hls_cnn.prj`)
  - 清理不完整
- **解决方案**:
  - 所有 HLS 命令使用 `$(TEST_HW_DIR)` (tests/hw/)
  - 修正项目名称为 `hls_cnn.prj`
  - 添加 logs 目录清理
- **效果**: Make 命令全部正常工作
- **文档**: `MAKEFILE_FIX_SUMMARY.md`

## 📁 项目结构

```
hls_cnn/
├── Makefile                        # 主构建文件 ✅
├── MAKEFILE_GUIDE.md               # Makefile 使用指南
├── MAKEFILE_FIX_SUMMARY.md         # Makefile 修复总结
├── PROJECT_STATUS.md               # 本文件
│
├── src/                            # 源代码
│   ├── hls_cnn.h                   # CNN 头文件
│   ├── hls_cnn.cpp                 # CNN 实现
│   └── cnn_marco.h                 # 宏定义和类型定义
│
├── tests/
│   ├── sw/                         # CPU 软件测试
│   │   ├── unit_test.cpp           # 单元测试
│   │   └── integration_test.cpp    # 集成测试
│   │
│   └── hw/                         # HLS 硬件测试
│       ├── run_hls.tcl             # HLS 主脚本 ✅
│       ├── hls_config.tcl          # HLS 配置（DSP优化）✅
│       ├── test.cpp                # 测试平台
│       ├── uut_top.cpp             # 硬件顶层
│       ├── uut_top.hpp             # 顶层头文件
│       ├── test_dsp_fix.sh         # DSP 修复测试脚本
│       ├── analyze_dsp.sh          # DSP 结果分析脚本
│       ├── DSP_FIX.md              # DSP 修复详细文档
│       ├── DSP_FIX_SUMMARY.md      # DSP 修复总结
│       ├── hls_cnn.prj/            # HLS 项目（生成）
│       └── logs/                   # 日志目录（生成）
│
├── build/                          # 构建输出（生成）
│   ├── unit_test                   # 单元测试可执行文件
│   └── integration_test            # 集成测试可执行文件
│
├── claude-doc/                     # Claude 生成的文档
└── verify_makefile.sh              # Makefile 验证脚本

```

## 🎯 CNN 网络架构

```
Input [1x28x28]
    ↓
Conv1 [16 filters, 3x3] → [16x26x26]
    ↓ ReLU
MaxPool [2x2] → [16x13x13]
    ↓
Conv2 [32 filters, 3x3] → [32x11x11]
    ↓ ReLU
MaxPool [2x2] → [32x5x5]
    ↓ Flatten
FC1 [128 neurons] → [128]
    ↓ ReLU
FC2 [10 neurons] → [10]
    ↓
Output (Logits)
```

## 🔧 配置参数

### 网络参数（cnn_marco.h）
```cpp
// 数据类型（可配置）
#ifdef USE_FLOAT
typedef float data_t;          // C仿真用浮点
typedef float weight_t;
#else
typedef ap_fixed<16, 8> data_t;    // 硬件用定点
typedef ap_fixed<16, 8> weight_t;
#endif

// 层参数
CONV1: 1→16 channels, 3x3 kernel, 28x28→26x26
POOL1: 2x2, 26x26→13x13
CONV2: 16→32 channels, 3x3 kernel, 13x13→11x11
POOL2: 2x2, 11x11→5x5
FC1: 800→128
FC2: 128→10
```

### HLS 配置（hls_config.tcl）
```tcl
# 时钟: 100 MHz (10 ns)
# 器件: xc7z020-clg400-1

# DSP 配置（优化后）
config_op fadd -impl fabric     # 避免 OPMODE 问题
config_op fsub -impl fabric
config_op fmul -impl maxdsp     # 保持乘法效率

# 优化
config_compile -unsafe_math_optimizations
config_schedule -enable_dsp_full_reg
```

## 📊 资源使用估算

| 资源 | 使用 | 可用 (7020) | 利用率 |
|------|------|-------------|--------|
| LUT | ~18k | 53,200 | ~34% |
| FF | ~12k | 106,400 | ~11% |
| BRAM | ~60 | 140 | ~43% |
| DSP48E1 | ~50 | 220 | ~23% |

## ⚡ 性能估算

| 指标 | 值 |
|------|------|
| 时钟频率 | 100 MHz |
| 单次推理延迟 | ~5-10 ms |
| 吞吐量 | ~100-200 fps |

*注: 实际性能取决于流水线配置和内存带宽*

## 🧪 测试状态

### CPU 软件测试
- ✅ 单元测试: 通过
- ✅ 集成测试: 通过
- 运行: `make unit_test` / `make integration_test`

### HLS 硬件测试
- ✅ C 仿真 (CSIM): 通过
- ✅ C 综合 (CSYNTH): 通过
- ✅ 协同仿真 (COSIM): 通过（DSP 警告已修复）
- ⏳ IP 导出: 待运行

## 🚀 快速开始

### 1. CPU 测试（快速验证）
```bash
cd /path/to/hls_cnn

# 单元测试
make unit_test

# 集成测试  
make integration_test
```

### 2. HLS C 仿真（1-3分钟）
```bash
# C 仿真（推荐先运行）
make hls_csim

# 或直接运行
cd tests/hw
vitis_hls -f run_hls.tcl
```

### 3. HLS 综合（5-10分钟）
```bash
make hls_synth

# 查看报告
cat tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt
```

### 4. HLS 协同仿真（10-30分钟）
```bash
make hls_cosim

# 查看报告
cat tests/hw/hls_cnn.prj/sol/sim/report/uut_top_cosim.rpt
```

### 5. 导出 IP（用于 Vivado）
```bash
make hls_export

# IP 位置
ls -la tests/hw/hls_cnn.prj/sol/impl/
```

## 📖 文档索引

### 核心文档
- `README.md` - 项目概述
- `QUICKSTART.md` - 快速入门
- `PROJECT_STATUS.md` - 本文件，项目状态

### Makefile 相关
- `Makefile` - 构建文件
- `MAKEFILE_GUIDE.md` - Makefile 使用指南
- `MAKEFILE_FIX_SUMMARY.md` - 路径修复总结
- `verify_makefile.sh` - 验证脚本

### DSP 问题修复
- `tests/hw/hls_config.tcl` - HLS 配置
- `tests/hw/DSP_FIX.md` - 详细技术文档
- `tests/hw/DSP_FIX_SUMMARY.md` - 修复总结
- `tests/hw/test_dsp_fix.sh` - 测试脚本
- `tests/hw/analyze_dsp.sh` - 分析脚本

### 源代码
- `src/hls_cnn.h` - 主头文件
- `src/hls_cnn.cpp` - 主实现
- `src/cnn_marco.h` - 配置和类型定义

## 🔍 常见问题

### Q1: DSP OPMODE 警告怎么办？
**A**: 已修复。使用 `hls_config.tcl` 配置浮点运算。详见 `tests/hw/DSP_FIX_SUMMARY.md`

### Q2: Make 命令找不到文件？
**A**: 已修复。确保在项目根目录运行。详见 `MAKEFILE_FIX_SUMMARY.md`

### Q3: Co-simulation 太慢？
**A**: 
- 先运行 `make hls_csim` 验证功能
- Co-sim 主要用于 RTL 验证
- 正常需要 10-30 分钟

### Q4: 如何修改网络结构？
**A**: 修改 `src/cnn_marco.h` 中的参数，重新编译和综合

### Q5: 如何改为定点数？
**A**: 在编译时不使用 `-DUSE_FLOAT`，将使用 `ap_fixed<16,8>` 类型

## 🎯 下一步计划

### 短期（已完成）
- ✅ 修复 DSP OPMODE 问题
- ✅ 修复 Makefile 路径
- ✅ 完成文档

### 中期（待完成）
- ⏳ 运行完整 HLS 流程
- ⏳ 导出 IP 核
- ⏳ 在 Vivado 中集成

### 长期（规划中）
- 📋 优化性能（流水线、并行度）
- 📋 添加定点数支持
- 📋 集成到 Zynq PS/PL 系统
- 📋 测试实际 MNIST 数据集

## 🛠️ 开发环境

### 必需工具
- Vitis HLS 2024.1
- g++ (支持 C++14)
- Make
- bash/zsh

### 可选工具
- Vivado 2024.1 (IP 集成)
- Python 3.x (数据处理)

## 📞 技术支持

### 报告问题
1. 检查相关文档
2. 运行验证脚本
3. 查看日志文件

### 有用的命令
```bash
# 查看 Makefile 目标
make help

# 验证 Makefile
./verify_makefile.sh

# 分析 DSP 使用
cd tests/hw && ./analyze_dsp.sh

# 查看 HLS 日志
tail -f tests/hw/vitis_hls.log
```

## ✅ 项目健康状态

| 检查项 | 状态 |
|--------|------|
| 源代码编译 | ✅ 通过 |
| 单元测试 | ✅ 通过 |
| 集成测试 | ✅ 通过 |
| HLS C 仿真 | ✅ 通过 |
| HLS 综合 | ✅ 通过 |
| HLS Co-sim | ✅ 通过（DSP 已优化）|
| 文档完整性 | ✅ 完整 |
| Makefile | ✅ 正常 |

## 🎉 总结

**项目状态**: ✅ 所有已知问题已修复，可以正常开发和测试

**准备就绪**: 
- ✅ 软件测试
- ✅ HLS 仿真
- ✅ HLS 综合
- ✅ HLS 协同仿真
- ⏳ IP 导出（待运行）

**开始使用**:
```bash
cd /path/to/hls_cnn
make help           # 查看所有命令
make unit_test      # 快速测试
make hls_csim       # HLS 仿真
```

---

**更新日期**: 2025-10-02  
**维护者**: HLS CNN Project Team  
**许可证**: Apache License 2.0
