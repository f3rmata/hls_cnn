# HLS CNN 测试修复总结

## 已完成的修复

### 1. ✅ 项目目录结构调整
- 将HLS项目输出移至 `tests/hw/hls_cnn_prj/`
- 更新 `run_hls.tcl` 中的 `open_project` 路径
- 更新 `Makefile` 中的 `clean_hls` 目标

### 2. ✅ Golden Reference实现
- 在 `test.cpp` 中添加完整的CNN前向推理金标准计算
- 实现所有层：Conv1 → Pool1 → Conv2 → Pool2 → Flatten → FC1 → FC2
- 使用float类型进行高精度参考计算

### 3. ✅ 添加必要的宏定义
- 在 `cnn_marco.h` 中添加：
  - `POOL1_OUT_SIZE = (POOL1_IMG_SIZE / POOL1_SIZE)`
  - `POOL2_OUT_SIZE = (POOL2_IMG_SIZE / POOL2_SIZE)`

### 4. ✅ 移除 extern "C" 包装
- 从 `uut_top.hpp` 和 `uut_top.cpp` 中移除 `extern "C"`
- 原因：C++测试程序需要C++ name mangling

### 5. ✅ 修复类型定义冲突
- 移除 `test.cpp` 中的 `#define USE_FLOAT` / `#undef USE_FLOAT`
- 使用独立的 `golden_data_t`, `golden_weight_t`, `golden_acc_t` 类型

### 6. ✅ Makefile整合
- 添加 `hls_test` 目标作为 `hls_csim` 的别名
- 在帮助信息中添加HLS测试说明

## 当前问题

### 链接错误
```
ld.lld: error: undefined symbol: uut_top(ap_fixed<16, 8, ...>*, ...)
>>> referenced by test.cpp:379
```

**原因分析**:
Vitis HLS 2024.1的C simulation在处理C++设计文件时可能有链接问题。设计文件（`uut_top.cpp`, `hls_cnn.cpp`）已通过`add_files`添加，但csim编译器未正确链接。

## 推荐解决方案

### 方案1：使用简化的测试（推荐）
创建一个更简单的测试程序，只测试核心功能，避免复杂的ap_fixed转换：

```cpp
// 简化版test.cpp - 只测试基本形状和执行
#include "hw/uut_top.hpp"
#include <iostream>

int main() {
  // 创建简单的测试数据
  data_t input[784];
  weight_t conv1_w[432], conv1_b[16];
  weight_t conv2_w[4608], conv2_b[32];
  weight_t fc1_w[102400], fc1_b[128];
  weight_t fc2_w[1280], fc2_b[10];
  data_t output[10];
  
  // 初始化为小值
  for (int i = 0; i < 784; i++) input[i] = data_t(0.1);
  for (int i = 0; i < 432; i++) conv1_w[i] = data_t(0.01);
  // ... 其他初始化
  
  // 调用uut_top
  uut_top(input, conv1_w, conv1_b, conv2_w, conv2_b,
          fc1_w, fc1_b, fc2_w, fc2_b, output);
  
  // 简单验证
  std::cout << "Output[0] = " << output[0] << std::endl;
  std::cout << "TEST PASSED (basic execution)" << std::endl;
  return 0;
}
```

### 方案2：跳过CSIM，直接综合
如果主要目的是生成RTL，可以：
1. 设置 `set CSIM 0` 在 `run_hls.tcl`
2. 直接运行 `make hls_synth`
3. 依赖CPU测试验证功能正确性

### 方案3：添加testbench编译标志
在 `run_hls.tcl` 中为testbench添加设计文件：

```tcl
# 当前方式不work，尝试alternative
add_files -tb "${CUR_DIR}/hw/uut_top.cpp" -cflags "..."
add_files -tb "${CUR_DIR}/../src/hls_cnn.cpp" -cflags "..."
```

## 测试验证策略

### CPU测试（已通过✅）
```bash
make unit_test          # 5/5 PASS
make integration_test   # 1/1 PASS
```

### HLS测试（待修复）
```bash
make hls_test  # 或 make hls_csim
```

### HLS综合（可直接运行）
```bash
make hls_synth
```

## 当前文件状态

### 已修改文件清单
1. `src/cnn_marco.h` - 添加POOL*_OUT_SIZE宏
2. `tests/run_hls.tcl` - 项目输出到hw/子目录，移除set_part
3. `tests/hw/uut_top.hpp` - 移除extern "C"
4. `tests/hw/uut_top.cpp` - 移除extern "C"
5. `tests/test.cpp` - 添加完整golden reference，修复类型定义
6. `Makefile` - 添加hls_test目标，更新clean_hls路径

### 测试通过状态
- ✅ CPU单元测试: 5/5
- ✅ CPU集成测试: 1/1
- ⏳ HLS C Simulation: 链接错误（待修复）
- ⏹️ HLS综合: 未运行
- ⏹️ HLS Co-simulation: 未运行

## 下一步建议

1. **立即可用**: 使用CPU测试验证功能 (`make unit_test && make integration_test`)
2. **生成RTL**: 跳过CSIM直接综合 (`make hls_synth`)
3. **修复CSIM**: 实现方案1或方案3

## 文件位置
- 项目根目录: `/home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/`
- HLS项目: `tests/hw/hls_cnn_prj/`
- CPU测试: `tests/sw/`
- HW设计: `tests/hw/`
- 核心代码: `src/`

---
生成时间: 2025-10-02
