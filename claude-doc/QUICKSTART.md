# HLS CNN 快速入门指南

## 5分钟快速体验

### 第一步：验证环境

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
ls -la
# 应该看到: src/ tests/ Makefile README.md
```

### 第二步：运行所有测试

```bash
# 单元测试（验证各层功能）
make unit_test

# 期望输出:
# ========================================
# Test Results: 5/5 passed
# ========================================
```

```bash
# 集成测试（验证完整CNN流程）
make integration_test

# 期望输出:
# ========================================
# Integration Test: PASSED
# ========================================
```

### 第三步：查看项目文档

```bash
# 完整说明
cat README.md

# 项目总结
cat PROJECT_SUMMARY.md
```

---

## 核心文件说明

| 文件 | 用途 | 重要程度 |
|------|------|----------|
| `src/hls_cnn.h` | CNN层实现（卷积、池化、FC） | ⭐⭐⭐⭐⭐ |
| `src/hls_cnn.cpp` | 顶层推理函数 | ⭐⭐⭐⭐⭐ |
| `src/cnn_marco.h` | 网络配置参数 | ⭐⭐⭐⭐ |
| `tests/unit_test.cpp` | 单元测试 | ⭐⭐⭐ |
| `tests/integration_test.cpp` | 集成测试 | ⭐⭐⭐ |
| `tests/run_hls.tcl` | HLS综合脚本 | ⭐⭐ |

---

## 常用命令

```bash
# 清理构建文件
make clean

# 查看帮助
make help

# 仅编译（不运行）
make build/unit_test
make build/integration_test

# HLS C仿真（需要Vitis HLS）
make hls_csim

# HLS完整综合（需要Vitis HLS + 长时间）
make hls_synth
```

---

## 修改网络结构示例

### 增加卷积层输出通道

编辑 `src/cnn_marco.h`:

```cpp
// 修改前
#define CONV1_OUT_CH 16

// 修改后
#define CONV1_OUT_CH 32  // 从16改为32
```

### 更改卷积核大小

```cpp
// 修改前
#define CONV1_KERNEL_SIZE 3

// 修改后
#define CONV1_KERNEL_SIZE 5  // 从3x3改为5x5
```

### 增加全连接层神经元

```cpp
// 修改前
#define FC1_OUT_SIZE 128

// 修改后
#define FC1_OUT_SIZE 256  // 从128改为256
```

**注意**：修改后需要重新编译测试！

---

## 性能优化提示

### 1. 增加并行度

在 `src/hls_cnn.h` 的数组分割处修改 `factor`:

```cpp
// 修改前
#pragma HLS ARRAY_PARTITION variable=weights dim=1 cyclic factor=2

// 修改后（更高并行度）
#pragma HLS ARRAY_PARTITION variable=weights dim=1 cyclic factor=4
```

### 2. 调整流水线深度

```cpp
// 修改前
#pragma HLS PIPELINE II=1

// 修改后（放宽时序）
#pragma HLS PIPELINE II=2
```

### 3. 使用定点数（减少资源）

修改 `src/cnn_marco.h`:

```cpp
// 修改前
typedef float data_t;

// 修改后
#include "ap_fixed.h"
typedef ap_fixed<16,8> data_t;  // 16位定点数
```

---

## 故障排查

### 问题：编译错误 "hls_math.h not found"

**解决方案**：
```bash
# 设置Vitis HLS环境变量
source /opt/Xilinx/Vitis/2024.1/settings64.sh
```

### 问题：测试失败

**检查步骤**：
1. 确认没有修改过测试代码
2. 清理并重新编译：`make clean && make unit_test`
3. 查看详细错误信息

### 问题：HLS综合失败

**可能原因**：
- 时钟频率过高 → 修改 `run_hls.tcl` 中的时钟周期
- 资源不足 → 减少并行度或使用更小网络

---

## 下一步学习

1. **阅读代码**：从 `src/hls_cnn.h` 开始，理解各层实现
2. **修改参数**：尝试调整网络结构，观察性能变化
3. **添加新层**：参考现有层实现，添加 BatchNorm 或 Dropout
4. **HLS综合**：在真实FPGA环境中综合，查看资源使用和时序
5. **集成应用**：将CNN模块集成到更大的系统中

---

## 支持与反馈

- 查看 `README.md` 获取完整文档
- 查看 `PROJECT_SUMMARY.md` 了解项目概况
- 遇到问题请检查各层单元测试是否通过

**祝你使用愉快！** 🚀
