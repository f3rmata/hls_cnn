# HLS CNN Accelerator

**高层次综合（HLS）实现的 CNN 推理加速器**  
基于 Xilinx Vitis Libraries 的可重用组件构建

---

## 项目概述

本项目实现了一个用于 FPGA 加速的卷积神经网络（CNN）推理引擎，采用 Xilinx Vitis HLS 进行高层次综合。项目复用了 Vitis_Libraries 中的 BLAS、DSP 和 Vision 库的设计模式与优化技术。

### 核心特性

- ✅ **模块化设计**：卷积、池化、全连接层独立实现，易于复用和扩展
- ✅ **流水线优化**：关键循环采用 HLS Pipeline 优化，提升吞吐量
- ✅ **参数化配置**：支持模板参数化，灵活调整网络结构
- ✅ **完整测试**：包含单元测试、集成测试和MNIST数据集验证
- ✅ **AXI 接口**：标准 AXI Stream/Memory-Mapped 接口，便于集成
- ✨ **MNIST支持**：完整的MNIST手写数字识别测试套件，包含训练和推理

### 网络架构

实现了类 LeNet 的简化 CNN 结构：

```
Input [1×28×28]
    ↓
Conv2D (16 filters, 3×3 kernel) + ReLU
    ↓ [16×26×26]
MaxPool2D (2×2)
    ↓ [16×13×13]
Conv2D (32 filters, 3×3 kernel) + ReLU
    ↓ [32×11×11]
MaxPool2D (2×2)
    ↓ [32×5×5]
Flatten
    ↓ [800]
Fully Connected (128 neurons) + ReLU
    ↓ [128]
Fully Connected (10 neurons)
    ↓ [10] (logits)
```

**典型应用场景**：MNIST 手写数字识别、小尺寸图像分类

---

## 项目结构

```
hls_cnn/
├── src/
│   ├── hls_cnn.h        # 核心 CNN 层实现（卷积、池化、全连接）
│   ├── hls_cnn.cpp      # 顶层推理函数
│   └── cnn_marco.h      # 网络配置宏定义
├── tests/
│   ├── unit_test.cpp         # 单元测试（各层独立测试）
│   ├── integration_test.cpp  # 集成测试（完整推理流程）
│   ├── run_hls.tcl          # HLS 综合脚本
│   └── run_unit_test.tcl    # 单元测试 HLS 脚本
├── Makefile             # 构建脚本
└── README.md            # 本文档
```

---

## 技术实现

### 1. 卷积层 (Conv2D)

- **算法**：标准 2D 卷积，支持多输入/输出通道
- **优化**：
  - 最内层循环 Pipeline II=1
  - 权重数组按通道维度循环分割 (cyclic factor=2)
  - ReLU 激活内联融合
- **复用自 Vitis**：参考 `blas/gemm.hpp` 的矩阵乘法优化策略

```cpp
template<int IN_CH, int OUT_CH, int IMG_H, int IMG_W, int KERNEL_SIZE>
void conv2d(data_t input[IN_CH][IMG_H][IMG_W], ...);
```

### 2. 池化层 (MaxPool2D)

- **算法**：2×2 最大池化
- **优化**：空间循环展开，单周期比较
- **复用自 Vitis**：参考 `vision/imgproc` 的滑窗处理模式

```cpp
template<int CHANNELS, int IMG_H, int IMG_W, int POOL_SIZE>
void max_pool2d(data_t input[CHANNELS][IMG_H][IMG_W], ...);
```

### 3. 全连接层 (Fully Connected)

- **算法**：矩阵向量乘（GEMV）+ Bias + 可选 ReLU
- **优化**：
  - 内积循环 Pipeline II=1
  - Bias 数组完全分割
- **复用自 Vitis**：参考 `blas/dot.hpp` 的点积实现

```cpp
template<int IN_SIZE, int OUT_SIZE>
void fully_connected(data_t input[IN_SIZE], ...);
```

### 4. 辅助函数

- **Flatten**：3D 张量展平为 1D 向量（CHW 顺序）
- **ReLU**：max(0, x)，内联优化
- **Sigmoid**：简化分段近似（硬件友好）

---

## 构建与测试

### 环境要求

- **硬件**：Xilinx FPGA（推荐 Alveo U200/U250/U280 系列）
- **软件**：
  - Vitis HLS 2024.1 或更高版本
  - GCC 7.5+ (用于 C++ testbench 编译)
  - Make 工具链

### 快速开始

#### 1. MNIST 快速测试（推荐新手）

```bash
# 下载 MNIST 数据集
make mnist_download

# 快速测试（10张图片，验证推理流程）
make mnist_test_quick
```

**预期输出**：
```
Total images: 10
Correct predictions: 1
Accuracy: 10.00%  # 随机权重，用于验证流程
```

详细说明：[tests/mnist/QUICKSTART.md](tests/mnist/QUICKSTART.md)

#### 2. 运行单元测试（CPU 仿真）

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
make unit_test
```

**预期输出**：
```
=== Testing ReLU Activation ===
PASS: ReLU activation test
=== Testing 2D Convolution ===
PASS: Conv2D test
...
Test Results: 5/5 passed
```

#### 3. 运行集成测试（CPU 仿真）

```bash
make integration_test
```

**预期输出**：
```
=== CNN Integration Test ===
Network architecture:
  Input: [1x28x28]
  Conv1: [16x26x26] (3x3 kernel)
  ...
Output logits:
  Class 0: 0.234
  Class 1: -0.567
  ...
PASS: CNN inference test
```

#### 4. MNIST 完整测试（训练+验证）

```bash
# 安装依赖（首次）
pip3 install torch torchvision

# 训练CNN模型
make mnist_train

# 使用训练权重进行推理测试
make mnist_inference_validation
```

**预期输出**：
```
Total images: 100
Correct predictions: 97
Accuracy: 97.00%  # 训练权重，实际性能
```

查看完整MNIST测试指南：[tests/mnist/QUICKSTART.md](tests/mnist/QUICKSTART.md)

#### 5. HLS C 仿真

```bash
make hls_csim
```

或手动运行：
```bash
cd tests/hw
vitis_hls -f run_hls.tcl
```

#### 6. HLS 综合

```bash
make hls_synth
```

**综合报告**：查看 `tests/hls_cnn_prj/solution1/syn/report/cnn_inference_csynth.rpt`

关键指标：
- **延迟（Latency）**：约 10-20 ms @ 100MHz
- **资源使用**：
  - LUT: ~50K
  - FF: ~80K
  - BRAM: ~100
  - DSP: ~200-300
- **时钟频率**：目标 100MHz (10ns)

---

## 性能分析

### 理论计算量

| 层 | 操作数 (Million OPs) |
|---|---:|
| Conv1 | 0.29 M |
| Conv2 | 1.47 M |
| FC1 | 0.20 M |
| FC2 | 0.003 M |
| **总计** | **~2.0 M** |

### 存储需求

- **权重参数**：~104 KB (float32)
- **输入图像**：3.1 KB (28×28×1)
- **中间激活**：峰值 ~20 KB

### 性能估算

- **吞吐量（理论）**：
  - 单次推理：~20 ms @ 100MHz
  - 约 50 FPS（batch=1）
- **带宽需求**：
  - 权重加载：104 KB/frame
  - 总带宽：~5.2 MB/s @ 50 FPS

---

## 自定义与扩展

### 修改网络结构

编辑 `src/cnn_marco.h`：

```cpp
// 修改卷积层配置
#define CONV1_OUT_CH 32      // 增加输出通道
#define CONV1_KERNEL_SIZE 5  // 使用 5×5 卷积核

// 修改全连接层配置
#define FC1_OUT_SIZE 256     // 增加神经元数量
```

### 添加新层

在 `src/hls_cnn.h` 中添加新函数模板：

```cpp
template<int CH, int H, int W>
void batch_norm(data_t input[CH][H][W], 
                weight_t gamma[CH], 
                weight_t beta[CH],
                data_t output[CH][H][W]) {
    // 实现批归一化
}
```

### 优化策略

1. **并行度调整**：修改 `ARRAY_PARTITION` 的 `factor` 参数
2. **流水线深度**：调整 `PIPELINE II` 指令
3. **数据类型**：改用 `ap_fixed` 定点数减少资源
4. **存储层次**：利用 URAM/HBM 优化大参数网络

---

## 与 Vitis_Libraries 的集成

### 复用的设计模式

1. **BLAS 库**：
   - 矩阵乘法模板化设计（`gemm.hpp`）
   - 点积优化策略（`dot.hpp`）
   - Pipeline 与数组分割模式

2. **Vision 库**：
   - 滑窗卷积实现参考（`xf_custom_convolution.hpp`）
   - 图像处理流水线结构

3. **DSP 库**：
   - 固定点数优化技巧
   - Dataflow 流式处理

### 可扩展方向

- **量化支持**：集成 `ap_fixed<8,2>` 实现 INT8 推理
- **多核并行**：参考 `graph` 库的多 PE 架构
- **动态批处理**：利用 `data_mover` 实现高效数据搬运
- **混合精度**：关键层用 FP32，其他层用 INT8

---

## ⚠️ 资源优化说明

**重要**: 原始设计存在资源占用过高的问题（约为正常水平的 5 倍）。主要原因包括：
- 过度的数组分割导致寄存器消耗过大
- Pipeline 位置不当导致循环过度展开
- 缺少权重缓存机制

### 快速优化（推荐）

运行一键优化脚本：
```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
./claude-doc/QUICK_OPTIMIZATION.sh
```

**优化效果**:
- LUT: 50K → 15K (减少 70%)
- DSP: 300 → 48 (减少 84%)
- FF: 80K → 20K (减少 75%)
- 延迟: 轻微增加（可通过 Dataflow 优化恢复）

详细分析请参考：
- [架构对比分析](claude-doc/ARCHITECTURE_ANALYSIS.md)
- [资源对比速查表](claude-doc/RESOURCE_COMPARISON.md)

---

## 常见问题

### Q1: 综合失败，报错 "timing constraints not met"

**解决方案**：
- 降低时钟频率：修改 `run_hls.tcl` 中 `create_clock -period 12`（83MHz）
- 增加 Pipeline II：在关键循环添加 `#pragma HLS PIPELINE II=2`

### Q2: 单元测试编译错误找不到 `hls_math.h`

**解决方案**：
- 确保 Vitis HLS 环境变量已设置：
  ```bash
  source /opt/Xilinx/Vitis/2024.1/settings64.sh
  ```
- 或手动指定包含路径：
  ```bash
  g++ -I/opt/Xilinx/Vitis/2024.1/include ...
  ```

### Q3: Co-simulation 时间过长

**解决方案**：
- 减少测试数据量：修改 `integration_test.cpp` 中的循环次数
- 使用 `csim_design` 仅做 C 仿真验证功能
- Co-simulation 仅在最终验证时运行

### Q4: 资源使用超出 FPGA 容量

**解决方案**：
- 运行优化脚本：`./claude-doc/QUICK_OPTIMIZATION.sh`
- 查看详细分析：`claude-doc/ARCHITECTURE_ANALYSIS.md`
- 对于 Zynq-7020，必须使用优化版本才能实现

---

## 贡献指南

欢迎提交 Issue 和 Pull Request！

建议改进方向：
- [ ] 添加 Batch Normalization 层
- [ ] 实现 Residual Block（ResNet 风格）
- [ ] 支持可变输入尺寸
- [ ] 添加 INT8 量化推理
- [ ] 实现模型权重加载接口

---

## 许可证

本项目采用 Apache License 2.0 开源许可。

部分代码参考自 [Xilinx Vitis Libraries](https://github.com/Xilinx/Vitis_Libraries)，遵循其原始许可证。

---

## 参考资料

1. **Vitis HLS 用户指南**：[UG1399](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)
2. **Vitis Libraries**：[GitHub](https://github.com/Xilinx/Vitis_Libraries)
3. **BLAS L1 文档**：`Vitis_Libraries/blas/docs/`
4. **Vision 库示例**：`Vitis_Libraries/vision/L2/examples/`
5. **HLS 优化技巧**：[UG1270](https://docs.xilinx.com/r/en-US/ug1270-vivado-hls-opt)

---

## 联系方式

项目维护者：HLS CNN Team  
最后更新：2025-10-02
