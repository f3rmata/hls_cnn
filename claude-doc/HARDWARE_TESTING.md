# HLS CNN 硬件测试指南

本文档介绍如何使用 Vitis HLS 对 CNN 项目进行硬件综合和验证。

## 目录
1. [数据类型转换](#数据类型转换)
2. [硬件接口设计](#硬件接口设计)
3. [测试流程](#测试流程)
4. [性能分析](#性能分析)

---

## 数据类型转换

### 从浮点到定点

为了支持硬件综合，我们将所有浮点类型转换为定点类型：

```cpp
// 原始浮点类型 (仅用于 C 仿真验证)
#ifdef USE_FLOAT
    typedef float data_t;
    typedef float weight_t;
    typedef float acc_t;
#else
    // 硬件定点类型
    typedef ap_fixed<16, 8> data_t;    // 16 位宽，8 位整数部分
    typedef ap_fixed<16, 8> weight_t;  // 范围: [-128, 127.996]
    typedef ap_fixed<32, 16> acc_t;    // 32 位累加器，16 位整数
#endif
```

### 定点类型说明

- **data_t / weight_t**: `ap_fixed<16, 8>`
  - 总位宽: 16 位
  - 整数位: 8 位
  - 小数位: 7 位 (16-8-1符号位)
  - 表示范围: [-128, 127.996]
  - 精度: 1/128 ≈ 0.0078

- **acc_t**: `ap_fixed<32, 16>`
  - 总位宽: 32 位
  - 整数位: 16 位
  - 用于累加运算，防止溢出

### 精度权衡

定点化可能带来精度损失：
- 浮点 (float): ~7 位十进制精度
- 定点 (ap_fixed<16,8>): ~2-3 位十进制精度
- 建议在 C 仿真阶段设置容忍度: **0.1** (10%)

---

## 硬件接口设计

### UUT Top 函数

`uut_top()` 是硬件综合的入口函数，使用扁平化数组接口以兼容 AXI：

```cpp
extern "C" void uut_top(
    data_t* input,              // [784]   - 输入图像
    weight_t* conv1_weights,    // [432]   - Conv1 权重
    weight_t* conv1_bias,       // [16]    - Conv1 偏置
    weight_t* conv2_weights,    // [4608]  - Conv2 权重
    weight_t* conv2_bias,       // [32]    - Conv2 偏置
    weight_t* fc1_weights,      // [102400]- FC1 权重
    weight_t* fc1_bias,         // [128]   - FC1 偏置
    weight_t* fc2_weights,      // [1280]  - FC2 权重
    weight_t* fc2_bias,         // [10]    - FC2 偏置
    data_t* output              // [10]    - 输出分类结果
);
```

### HLS Interface Pragma

```cpp
// 控制接口 (AXI-Lite)
#pragma HLS INTERFACE mode=s_axilite port=return

// 数据接口 (AXI Memory-Mapped)
#pragma HLS INTERFACE mode=m_axi depth=784 port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE mode=m_axi depth=432 port=conv1_weights offset=slave bundle=gmem1
...
```

- **s_axilite**: 用于控制和状态寄存器
- **m_axi**: 内存映射接口，支持突发传输
- **bundle**: 将相关接口分组到不同的 AXI 端口

### 数据重组

硬件 top 函数负责将扁平数组转换为多维数组：

```cpp
// 输入: input[784] → input_reshaped[1][28][28]
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

---

## 测试流程

### 1. C 仿真 (C Simulation)

快速验证算法功能，使用软件模拟：

```bash
make hls_csim
```

- **目的**: 验证算法正确性
- **速度**: 快 (~1-2 分钟)
- **使用**: 定点类型，但在 CPU 上执行
- **测试文件**: `tests/test.cpp`

#### 测试流程
1. 生成随机测试数据 (Xavier 初始化)
2. 转换为定点类型
3. 调用 `uut_top()`
4. 与黄金参考对比 (容忍度 0.1)

```cpp
// 生成测试数据
generate_test_data(input, conv1_w, conv1_b, ...);

// 转换为定点
for(int i = 0; i < size; i++)
    input_fixed[i] = input[i];

// 运行 HLS
uut_top(input_fixed, conv1_w_fixed, ...);

// 比较结果
bool pass = compare_outputs(output_fixed, output_golden, FC2_OUT_SIZE, 0.1);
```

### 2. C 综合 (C Synthesis)

将 C++ 代码转换为 RTL (Verilog/VHDL)：

```bash
make hls_synth
```

- **目的**: 生成硬件描述语言
- **速度**: 中等 (~5-10 分钟)
- **输出**: RTL 代码、资源报告、时序报告

#### 综合报告内容
- **延迟 (Latency)**: 时钟周期数
- **启动间隔 (II)**: 流水线间隔
- **资源使用**: LUT, FF, BRAM, DSP
- **时序**: 最大频率

查看报告：
```bash
cd tests/hls_cnn_prj/solution1/syn/report
cat uut_top_csynth.rpt
```

### 3. Co-仿真 (Co-Simulation)

RTL 级验证，运行实际硬件模拟：

```bash
make hls_cosim
```

⚠️ **警告**: Co-仿真非常耗时 (10-30 分钟)！

- **目的**: 验证 RTL 实现与 C 模型一致
- **速度**: 慢 (取决于数据规模)
- **使用场景**: 最终验证阶段

```bash
# 完整流程 (csim + synth + cosim)
make hls_full
```

### 4. IP 导出

生成可在 Vivado 中集成的 IP 核：

```bash
make hls_export
```

导出格式:
- **IP Catalog**: 用于 Vivado IP Integrator
- **描述**: "HLS CNN Inference Engine"
- **版本**: 1.0

---

## 性能分析

### 资源估算

基于网络架构 (LeNet-5 变体):

| 模块 | 参数量 | 操作数 (Ops) |
|------|--------|-------------|
| Conv1 (16@3×3) | 432 | ~324K |
| Conv2 (32@3×3) | 4608 | ~648K |
| FC1 (800→128) | 102,400 | ~205K |
| FC2 (128→10) | 1,280 | ~2.6K |
| **总计** | **108,720** | **~1.18M** |

### 预期硬件资源 (Alveo U280)

| 资源 | 使用量 | 百分比 |
|------|--------|--------|
| LUT | ~50K | 3% |
| FF | ~60K | 2% |
| BRAM | ~200 | 10% |
| DSP | ~300 | 3% |

### 优化建议

#### 1. 流水线优化
```cpp
#pragma HLS PIPELINE II=1  // 启动间隔 = 1
```

#### 2. 数组分割
```cpp
#pragma HLS ARRAY_PARTITION variable=weights dim=1 cyclic factor=4
```

#### 3. 数据流优化
```cpp
#pragma HLS DATAFLOW  // 层间流水线
```

#### 4. 循环展开
```cpp
#pragma HLS UNROLL factor=4  // 部分展开
```

---

## 常见问题

### Q1: C 仿真失败，报告精度误差过大

**解决方案**:
1. 增加定点位宽: `ap_fixed<24, 12>`
2. 调整容忍度: `tolerance = 0.2`
3. 检查溢出: 使用更大的累加器位宽

### Q2: 综合时序不满足 (Timing violation)

**解决方案**:
1. 降低时钟频率: `set CLOCK_PERIOD 5.0` (200 MHz → 250 MHz)
2. 增加流水线深度: `#pragma HLS PIPELINE II=2`
3. 减少并行度: 降低 `UNROLL factor`

### Q3: 资源使用过多 (Resource overflow)

**解决方案**:
1. 减少数组分割: 降低 `ARRAY_PARTITION factor`
2. 使用 BRAM 而非寄存器存储权重
3. 时分复用: 循环复用计算单元

### Q4: Co-仿真卡住

**解决方案**:
1. 减少测试数据量: 使用更少的测试样本
2. 检查 deadlock: 验证 dataflow 依赖关系
3. 增加超时时间: 在 TCL 中设置 `cosim_design -timeout 3600`

---

## 参考资料

1. **Vitis HLS User Guide (UG1399)**
   - [Xilinx Documentation](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)

2. **Vitis_Libraries BLAS 示例**
   - `/Vitis_Libraries/blas/L1/tests/hw/dot/`

3. **定点数据类型指南**
   - `ap_fixed<W, I>`: 总位宽 W，整数位 I
   - 推荐阅读: UG902 (HLS Data Types)

4. **接口综合指南**
   - AXI4 协议: UG1037
   - Memory-Mapped 接口: UG1399 Chapter 5

---

## 快速参考

### 完整测试流程
```bash
# 1. CPU 测试 (开发阶段)
make unit_test
make integration_test

# 2. HLS C 仿真 (快速验证)
make hls_csim

# 3. HLS 综合 (生成 RTL)
make hls_synth

# 4. Co-仿真 (可选，最终验证)
make hls_cosim

# 5. 导出 IP (集成到 Vivado)
make hls_export

# 清理
make clean
```

### TCL 脚本控制变量

编辑 `tests/run_hls.tcl`:

```tcl
set CSIM 1        # 启用 C 仿真
set CSYNTH 1      # 启用 C 综合
set COSIM 0       # 禁用 Co-仿真 (耗时)
set VIVADO_SYN 0  # 禁用 Vivado 综合
```

---

## 结论

本项目展示了如何：
1. ✅ 将浮点 CNN 转换为定点实现
2. ✅ 设计硬件兼容的接口 (AXI)
3. ✅ 实现完整的 HLS 测试流程
4. ✅ 遵循 Vitis_Libraries 设计模式

下一步建议：
- 🔄 在真实 FPGA 板卡上测试 (U200/U250/U280)
- 🔄 集成到 Vitis Accelerated Kernel 流程
- 🔄 使用 Vitis AI 量化工具进一步优化

如有问题，请参考 `README.md` 和 Vitis HLS 官方文档。
