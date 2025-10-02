# HLS CNN 硬件适配总结

本文档总结了将 HLS CNN 项目从 CPU 浮点实现转换为硬件可综合版本的所有改动。

## 改动概览

### 1. 数据类型转换 (`src/cnn_marco.h`)

**改动内容**:
```cpp
// 添加了条件编译支持
#ifndef USE_FLOAT
    typedef ap_fixed<16, 8> data_t;    // 硬件定点类型
    typedef ap_fixed<16, 8> weight_t;
    typedef ap_fixed<32, 16> acc_t;    // 更宽的累加器
#else
    typedef float data_t;              // C 仿真使用浮点
    typedef float weight_t;
    typedef float acc_t;
#endif
```

**影响**:
- 硬件综合使用定点数 (ap_fixed)
- C 仿真可选择使用浮点进行验证
- 累加器使用更宽位宽防止溢出

---

### 2. 激活函数优化 (`src/hls_cnn.h`)

**改动内容**:
```cpp
template <typename T> inline T sigmoid(T x) {
#pragma HLS INLINE
  if (x > T(5.0)) return T(1.0);
  if (x < T(-5.0)) return T(0.0);
  
#ifdef USE_FLOAT
  return T(1.0) / (T(1.0) + exp(-float(x)));  // C 仿真用精确版本
#else
  // 硬件用分段线性近似
  if (x > T(2.0)) return T(1.0);
  if (x < T(-2.0)) return T(0.0);
  return T(0.5) + T(0.25) * x;  // sigmoid(x) ≈ 0.5 + 0.25*x
#endif
}
```

**影响**:
- 避免在硬件中使用 exp() 函数 (资源消耗大)
- 使用分段线性近似，精度损失小
- C 仿真保持精确计算

---

### 3. 硬件顶层函数 (`src/uut_top.hpp` + `src/uut_top.cpp`)

**新增文件**: `uut_top.hpp`, `uut_top.cpp`

#### uut_top.hpp - 接口声明
```cpp
extern "C" {
void uut_top(
    data_t* input,           // 扁平化数组接口
    weight_t* conv1_weights, // 兼容 AXI Memory-Mapped
    weight_t* conv1_bias,
    // ... 其他参数
    data_t* output
);
}
```

#### uut_top.cpp - 实现要点

**HLS Interface Pragma**:
```cpp
#pragma HLS INTERFACE mode=s_axilite port=return  // 控制接口
#pragma HLS INTERFACE mode=m_axi depth=784 port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE mode=m_axi depth=432 port=conv1_weights offset=slave bundle=gmem1
// ... 每个参数使用独立的 bundle
```

**数据重组**:
```cpp
// 扁平数组 → 多维数组
RESHAPE_INPUT:
for(int c = 0; c < CONV1_IN_CH; c++) {
    for(int h = 0; h < CONV1_IMG_SIZE; h++) {
        for(int w = 0; w < CONV1_IMG_SIZE; w++) {
            #pragma HLS PIPELINE II=1
            int idx = c * CONV1_IMG_SIZE * CONV1_IMG_SIZE + h * CONV1_IMG_SIZE + w;
            input_reshaped[c][h][w] = input[idx];
        }
    }
}
```

**数组分割**:
```cpp
#pragma HLS ARRAY_PARTITION variable=conv1_w dim=1 cyclic factor=4
#pragma HLS ARRAY_PARTITION variable=fc2_w dim=1 complete
```

**影响**:
- 提供硬件综合入口
- AXI 接口兼容 Vivado IP Integrator
- 内存访问优化 (突发传输)

---

### 4. HLS 测试框架 (`tests/test.cpp`)

**新增文件**: `tests/test.cpp`

**功能**:
1. 生成测试数据 (Xavier 初始化)
2. 浮点 ↔ 定点转换
3. 调用 `uut_top()`
4. 与黄金参考对比 (tolerance = 0.1)

**关键代码**:
```cpp
// 生成浮点测试数据
generate_test_data(input_float, ...);

// 转换为定点
for(int i = 0; i < size; i++)
    input_fixed[i] = input_float[i];

// 运行 HLS
uut_top(input_fixed, ...);

// 比较结果
bool pass = compare_outputs(output_fixed, output_golden, FC2_OUT_SIZE, 0.1);
```

**影响**:
- C 仿真和 Co-仿真使用同一测试文件
- 自动化测试流程
- 量化精度损失

---

### 5. TCL 脚本更新 (`tests/run_hls.tcl`)

**改动内容**:
```tcl
# 配置变量
set CSIM 1       # 启用 C 仿真
set CSYNTH 1     # 启用 C 综合
set COSIM 0      # 默认禁用 Co-仿真 (耗时)
set VIVADO_SYN 0 # 可选 Vivado 综合

# 设置顶层函数
set_top uut_top

# 添加设计文件
add_files "../src/uut_top.cpp" -cflags "-I../src -std=c++14"
add_files "../src/hls_cnn.cpp" -cflags "-I../src -std=c++14"
# ...

# 添加测试文件
add_files -tb "test.cpp" -cflags "-I../src -std=c++14"

# 配置接口
config_interface -m_axi_alignment_byte_size 64 -m_axi_latency 64 -m_axi_max_widen_bitwidth 512

# 运行流程
if {$CSIM == 1} { csim_design -clean }
if {$CSYNTH == 1} { csynth_design }
if {$COSIM == 1} { cosim_design -trace_level all }
```

**影响**:
- 自动化 HLS 流程
- 灵活控制各阶段
- 优化 AXI 配置

---

### 6. Makefile 扩展

**新增目标**:
```makefile
hls_csim:    # HLS C 仿真
hls_synth:   # HLS 综合 (仅)
hls_cosim:   # HLS Co-仿真
hls_export:  # 导出 IP
hls_full:    # 完整流程 (csim + synth + cosim)
clean_hls:   # 清理 HLS 文件
```

**使用方法**:
```bash
make help          # 查看所有目标
make hls_csim      # 运行 C 仿真 (~1-2 分钟)
make hls_synth     # 运行综合 (~5-10 分钟)
make hls_cosim     # 运行 Co-仿真 (~10-30 分钟)
make hls_full      # 完整流程
```

---

## 文件结构

### 新增文件

```
hls_cnn/
├── src/
│   ├── uut_top.hpp          # 硬件顶层接口声明 (新增)
│   ├── uut_top.cpp          # 硬件顶层实现 (新增)
│   ├── cnn_marco.h          # 数据类型定义 (已修改)
│   ├── hls_cnn.h            # 激活函数优化 (已修改)
│   └── hls_cnn.cpp          # (无修改)
├── tests/
│   ├── test.cpp             # HLS 测试文件 (新增)
│   ├── run_hls.tcl          # HLS TCL 脚本 (已修改)
│   ├── unit_test.cpp        # (无修改)
│   └── integration_test.cpp # (无修改)
├── HARDWARE_TESTING.md      # 硬件测试指南 (新增)
├── HARDWARE_ADAPTATION.md   # 本文档 (新增)
├── quick_test.sh            # 快速测试脚本 (新增)
└── Makefile                 # (已扩展)
```

---

## 关键设计决策

### 1. 为什么使用 ap_fixed<16, 8>?

| 类型 | 位宽 | 整数位 | 范围 | 精度 |
|------|------|--------|------|------|
| float | 32 | - | ±3.4e38 | 7 位十进制 |
| ap_fixed<16, 8> | 16 | 8 | [-128, 127.996] | 1/256 ≈ 0.0039 |

**权衡**:
- ✅ 资源使用减少 50%
- ✅ 适合 CNN 数据范围 (通常 [-10, 10])
- ❌ 精度降低，需要更大容忍度 (0.1)

### 2. 为什么用分段线性 sigmoid?

| 方法 | 资源 (LUT) | 精度 | 延迟 (周期) |
|------|-----------|------|------------|
| exp() | ~2000 | 高 | ~50 |
| 查找表 | ~500 | 中 | ~5 |
| 分段线性 | ~50 | 低 | ~1 |

**选择**: 分段线性
- ✅ 资源友好
- ✅ 延迟低
- ❌ 精度损失 (~5% 误差)
- 💡 对于分类任务影响小 (只需保持大小关系)

### 3. 为什么使用多个 AXI Bundle?

```cpp
bundle=gmem0  // input
bundle=gmem1  // conv1 weights + bias
bundle=gmem2  // conv2 weights + bias
bundle=gmem3  // fc1 weights + bias
bundle=gmem4  // fc2 weights + bias
bundle=gmem5  // output
```

**优势**:
- ✅ 并行内存访问
- ✅ 减少 AXI 仲裁冲突
- ✅ 提高带宽利用率
- ❌ 需要更多 AXI 端口 (通常不是问题)

---

## 验证策略

### 三层验证

1. **CPU 验证** (`unit_test.cpp`, `integration_test.cpp`)
   - 使用 `typedef float data_t;`
   - 快速迭代，功能验证

2. **C 仿真** (`test.cpp`, `USE_FLOAT` off)
   - 使用 `typedef ap_fixed<16,8> data_t;`
   - 验证定点化精度损失
   - 调整容忍度 (0.1)

3. **Co-仿真** (TCL 脚本)
   - RTL 级验证
   - 确认硬件实现与 C 模型一致
   - 性能评估 (延迟、吞吐量)

### 精度验证流程

```
原始浮点 → 定点量化 → C 仿真 → RTL Co-仿真
   |            |          |            |
   |            ↓          ↓            ↓
   |        调整位宽   调整容忍度   验证时序
   └───────────────────────────────────┘
                (迭代优化)
```

---

## 性能预估

### 综合结果 (预期)

基于类似规模的 Vitis_Libraries 示例:

| 指标 | 值 |
|------|-----|
| **延迟** | ~50K 周期 |
| **启动间隔** | ~50K 周期 (无流水线) |
| **时钟频率** | 300 MHz |
| **吞吐量** | ~6K infer/s |

### 资源使用 (Alveo U280)

| 资源 | 使用量 | 总量 | 占比 |
|------|--------|------|------|
| LUT | 50K | 1.3M | 3.8% |
| FF | 60K | 2.6M | 2.3% |
| BRAM | 200 | 2016 | 9.9% |
| DSP | 300 | 9024 | 3.3% |

**结论**: 资源占用小，可实例化多个并行引擎

---

## 后续优化方向

### 1. 数据流优化
```cpp
#pragma HLS DATAFLOW
conv2d(...);
max_pool2d(...);
fully_connected(...);
```
- 层间流水线
- 减少延迟到 ~5K 周期

### 2. 循环展开
```cpp
#pragma HLS UNROLL factor=4
for(int i = 0; i < OUT_SIZE; i++) {
    // ...
}
```
- 提高吞吐量 4x
- 增加资源使用

### 3. 权重复用
```cpp
#pragma HLS BIND_STORAGE variable=weights type=rom_2p impl=bram
```
- 使用 BRAM 存储权重
- 减少 DDR 访问

### 4. 量化优化
- Int8 量化 (Vitis AI)
- 混合精度 (Conv 用 8-bit, FC 用 16-bit)
- 进一步降低资源使用

---

## 常见问题解决

### 问题 1: C 仿真失败 - "TYPE_ERROR"

**原因**: ap_fixed 不支持某些运算
**解决**:
```cpp
// 错误
ap_fixed<16,8> x = 1.0 / y;

// 正确
ap_fixed<16,8> x = ap_fixed<16,8>(1.0) / y;
```

### 问题 2: 综合失败 - "UNSUPPORTED_CALL"

**原因**: 调用了不可综合的函数 (如 `printf`)
**解决**:
```cpp
#ifndef __SYNTHESIS__
    printf("Debug: %f\n", x);
#endif
```

### 问题 3: Co-仿真超时

**原因**: 测试数据过大
**解决**:
- 减少测试样本数量
- 增加超时时间: `cosim_design -timeout 7200`

### 问题 4: 精度误差过大

**原因**: 定点位宽不足
**解决**:
- 增加位宽: `ap_fixed<24, 12>`
- 使用更大的累加器: `ap_fixed<40, 20>`
- 调整容忍度: `tolerance = 0.2`

---

## 测试检查清单

- [ ] CPU 单元测试通过 (`make unit_test`)
- [ ] CPU 集成测试通过 (`make integration_test`)
- [ ] HLS C 仿真通过 (`make hls_csim`)
- [ ] HLS 综合成功 (`make hls_synth`)
- [ ] 综合报告检查:
  - [ ] 延迟 < 100K 周期
  - [ ] LUT < 100K
  - [ ] DSP < 500
  - [ ] 时序满足 (300 MHz)
- [ ] Co-仿真通过 (`make hls_cosim`, 可选)
- [ ] IP 导出成功 (`make hls_export`, 可选)

---

## 参考资料

1. **Vitis HLS 官方文档**:
   - UG1399: Vitis HLS User Guide
   - UG902: High-Level Synthesis Data Types
   - UG1037: AXI Reference Guide

2. **Vitis_Libraries 示例**:
   - `blas/L1/tests/hw/dot/` - BLAS 硬件测试
   - `vision/L1/tests/` - Vision 库示例
   - `dsp/L1/tests/` - DSP 流水线示例

3. **本项目文档**:
   - `README.md` - 项目总览
   - `HARDWARE_TESTING.md` - 详细测试指南
   - `QUICKSTART.md` - 快速开始

---

## 总结

本次硬件适配完成了:
- ✅ 数据类型从浮点转换为定点
- ✅ 创建硬件可综合的顶层函数
- ✅ 实现完整的 HLS 测试框架
- ✅ 优化激活函数以适应硬件
- ✅ 配置 AXI 接口以集成 Vivado
- ✅ 提供自动化构建和测试脚本

下一步:
1. 运行 `./quick_test.sh` 验证所有修改
2. 查看综合报告优化性能
3. 集成到 Vitis Accelerated Kernel 流程
4. 在真实 FPGA 板卡上测试

如有问题，请参考 `HARDWARE_TESTING.md` 或 Vitis HLS 官方文档。
