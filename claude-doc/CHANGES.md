# 硬件适配改动总结

本文档简要列出了 HLS CNN 项目硬件适配的所有文件改动。

## 修改的文件

### 1. `src/cnn_marco.h` - 数据类型定义
**改动**: 添加定点类型支持
```cpp
#ifndef USE_FLOAT
    typedef ap_fixed<16, 8> data_t;    // 硬件: 16位定点
    typedef ap_fixed<16, 8> weight_t;
    typedef ap_fixed<32, 16> acc_t;
#else
    typedef float data_t;              // C仿真: 浮点
    typedef float weight_t;
    typedef float acc_t;
#endif
```

### 2. `src/hls_cnn.h` - 激活函数优化
**改动**: sigmoid 函数添加硬件友好的分段线性近似
```cpp
#ifdef USE_FLOAT
    return T(1.0) / (T(1.0) + exp(-float(x)));  // C仿真用exp
#else
    return T(0.5) + T(0.25) * x;  // 硬件用线性近似
#endif
```

### 3. `tests/run_hls.tcl` - HLS脚本
**改动**: 完全重写以支持 uut_top 和新测试流程
- 改变顶层函数: `cnn_inference` → `uut_top`
- 添加新设计文件: `uut_top.cpp`, `uut_top.hpp`
- 新测试文件: `test.cpp`
- 优化 AXI 配置

### 4. `Makefile` - 构建系统
**改动**: 添加 HLS 测试目标
- `make hls_csim` - HLS C仿真
- `make hls_synth` - HLS综合
- `make hls_cosim` - HLS Co-仿真
- `make hls_export` - 导出IP
- `make hls_full` - 完整流程

## 新增的文件

### 5. `src/uut_top.hpp` - 硬件顶层接口
**功能**: 声明硬件可综合的顶层函数
- extern "C" 接口
- 扁平化数组参数 (兼容AXI)
- 9个参数数组 (input + 4层权重/偏置 + output)

### 6. `src/uut_top.cpp` - 硬件顶层实现
**功能**: 包装 cnn_inference 为硬件接口
- HLS Interface pragma (s_axilite + m_axi)
- 扁平数组 → 多维数组重组
- 数组分割优化 (ARRAY_PARTITION)
- 流水线优化 (PIPELINE II=1)

### 7. `tests/test.cpp` - HLS测试框架
**功能**: C/Co-仿真测试代码
- 生成测试数据 (Xavier初始化)
- 浮点/定点转换
- 调用 uut_top
- 精度对比 (tolerance=0.1)

### 8. `HARDWARE_TESTING.md` - 硬件测试指南
**内容**:
- 数据类型转换说明
- HLS接口设计
- 测试流程 (csim/synth/cosim)
- 性能分析与优化
- 常见问题解决

### 9. `HARDWARE_ADAPTATION.md` - 详细改动说明
**内容**:
- 完整改动列表
- 设计决策解释
- 验证策略
- 性能预估
- 优化方向

### 10. `quick_test.sh` - 快速测试脚本
**功能**: 一键运行完整测试流程
```bash
./quick_test.sh
# 自动运行: unit_test → integration_test → hls_csim → hls_synth → (可选) hls_cosim
```

## 文件树对比

### 修改前
```
hls_cnn/
├── src/
│   ├── hls_cnn.h
│   ├── hls_cnn.cpp
│   └── cnn_marco.h
├── tests/
│   ├── unit_test.cpp
│   ├── integration_test.cpp
│   └── run_hls.tcl
├── Makefile
└── README.md
```

### 修改后
```
hls_cnn/
├── src/
│   ├── hls_cnn.h           (已修改)
│   ├── hls_cnn.cpp
│   ├── cnn_marco.h         (已修改)
│   ├── uut_top.hpp         (新增) ★
│   └── uut_top.cpp         (新增) ★
├── tests/
│   ├── unit_test.cpp
│   ├── integration_test.cpp
│   ├── test.cpp            (新增) ★
│   └── run_hls.tcl         (已修改)
├── Makefile                (已修改)
├── README.md
├── HARDWARE_TESTING.md     (新增) ★
├── HARDWARE_ADAPTATION.md  (新增) ★
└── quick_test.sh           (新增) ★
```

## 快速使用指南

### 1. CPU测试 (开发阶段)
```bash
make unit_test          # 5个单元测试
make integration_test   # 端到端测试
```

### 2. HLS C仿真 (验证定点化)
```bash
make hls_csim          # ~1-2分钟
```

### 3. HLS综合 (生成RTL)
```bash
make hls_synth         # ~5-10分钟
```

### 4. 一键测试 (推荐)
```bash
./quick_test.sh        # 自动运行所有测试
```

### 5. 查看报告
```bash
cd tests/hls_cnn_prj/solution1/syn/report
cat uut_top_csynth.rpt
```

## 关键技术点

### 数据类型
- **硬件**: `ap_fixed<16, 8>` (16位定点, 8位整数)
- **累加器**: `ap_fixed<32, 16>` (防止溢出)
- **C仿真**: `float` (通过 USE_FLOAT 宏切换)

### HLS优化
- **流水线**: `#pragma HLS PIPELINE II=1`
- **数组分割**: `#pragma HLS ARRAY_PARTITION dim=1 cyclic factor=4`
- **接口**: `#pragma HLS INTERFACE mode=m_axi`

### AXI接口
- **控制**: s_axilite (寄存器访问)
- **数据**: m_axi (内存映射, 6个独立bundle)

## 验证流程

```
┌─────────────┐
│ CPU测试     │ ✓ 功能验证 (float)
└──────┬──────┘
       ↓
┌─────────────┐
│ HLS C仿真   │ ✓ 定点精度验证 (ap_fixed)
└──────┬──────┘
       ↓
┌─────────────┐
│ HLS综合     │ ✓ 生成RTL
└──────┬──────┘
       ↓
┌─────────────┐
│ HLS Co-仿真 │ ✓ RTL验证 (可选,耗时)
└──────┬──────┘
       ↓
┌─────────────┐
│ 导出IP      │ → Vivado集成
└─────────────┘
```

## 兼容性

- **Vitis HLS**: 2024.1+ (推荐 2024.2)
- **目标平台**: Xilinx Alveo U200/U250/U280
- **编译器**: C++14
- **操作系统**: Linux (Ubuntu 20.04+)

## 下一步

1. ✅ 运行 `./quick_test.sh` 验证改动
2. ✅ 查看综合报告分析性能
3. 🔄 根据报告调整优化参数
4. 🔄 集成到 Vitis Accelerated Application
5. 🔄 在真实FPGA板卡上测试

## 获取帮助

- 详细测试指南: `HARDWARE_TESTING.md`
- 完整改动说明: `HARDWARE_ADAPTATION.md`
- 项目总览: `README.md`
- Make帮助: `make help`

## 注意事项

⚠️ **重要提示**:
1. 首次运行 HLS C仿真确保 Vitis HLS 已正确安装
2. Co-仿真非常耗时 (10-30分钟), 仅在最终验证时使用
3. 定点精度损失需要调整容忍度 (0.1 或更大)
4. 确保有足够的磁盘空间 (综合产物 ~1GB)

## 版本历史

- **v1.0** (2025-01-XX): 初始 CPU 浮点实现
- **v2.0** (2025-01-XX): 硬件适配 (本版本)
  - 添加定点类型支持
  - 创建 uut_top 硬件接口
  - 实现 HLS 测试框架
  - 优化激活函数

---

**生成时间**: 2025-01-XX  
**作者**: HLS CNN Project Team  
**许可**: Apache 2.0
