# MNIST 测试快速指南

## 概览

本目录包含用于在MNIST手写数字数据集上测试HLS CNN的完整工具链，包括：
- 数据下载和预处理
- PyTorch模型训练
- C++推理测试
- 性能评估和指标分析

## 快速开始

### 方法一：一键设置（推荐）

```bash
cd tests/mnist
./setup.sh
```

这会自动：
1. 下载MNIST数据集
2. 编译测试程序
3. 运行快速测试（10张图片）

### 方法二：手动步骤

```bash
# 1. 下载MNIST数据集
make mnist_download

# 2. 运行快速测试（随机权重）
make mnist_test_quick

# 3. (可选) 训练CNN模型
make mnist_train

# 4. 使用训练权重推理
make mnist_inference_quick
```

## 详细使用说明

### 1. 下载MNIST数据集

```bash
make mnist_download
```

这会下载并处理：
- **训练集**: 60,000张图片
- **测试集**: 10,000张图片
- **验证集**: 100张随机图片（从测试集中选取）
- **快速测试集**: 10张随机图片（从测试集中选取）

所有数据保存在 `data/` 目录下的二进制格式文件中。

### 2. 使用随机权重测试（验证推理流程）

```bash
# 快速测试 - 10张图片（约10秒）
make mnist_test_quick

# 验证测试 - 100张图片（约1分钟）
make mnist_test_validation

# 完整测试 - 10,000张图片（约10分钟）
make mnist_test_full
```

**预期结果**：
- 准确率约10%（随机猜测）
- 用于验证推理流程是否正确

### 3. 训练CNN模型

首先安装PyTorch（如果尚未安装）：

```bash
pip3 install torch torchvision
```

然后训练模型：

```bash
# 默认训练10个epoch
make mnist_train

# 或手动指定参数
cd tests/mnist
python3 train_mnist.py --epochs 20 --batch-size 64 --lr 0.001
```

**训练时间**：
- CPU: 约5-10分钟（10 epochs）
- GPU: 约1-2分钟（10 epochs）

**预期结果**：
- 测试集准确率: 95-98%
- 训练权重保存在 `weights/` 目录

### 4. 使用训练权重推理

```bash
# 快速推理 - 10张图片
make mnist_inference_quick

# 验证推理 - 100张图片
make mnist_inference_validation

# 完整推理 - 10,000张图片
make mnist_inference_full
```

**预期结果**：
- 准确率: 95-98%
- 显示混淆矩阵和每类指标

## 网络架构

```
输入 [1×28×28]
    ↓
卷积层1 [16×26×26] (3×3卷积核)
    ↓ ReLU
最大池化1 [16×13×13] (2×2池化)
    ↓
卷积层2 [32×11×11] (3×3卷积核)
    ↓ ReLU
最大池化2 [32×5×5] (2×2池化)
    ↓ 展平 [800]
全连接层1 [128]
    ↓ ReLU
全连接层2 [10]
    ↓
输出 [10] (类别logits)
```

**总参数数量**: ~108,000

## 测试输出说明

### 基本统计信息
```
Total images: 100
Correct predictions: 97
Accuracy: 97.00%
```

### 混淆矩阵
显示实际类别与预测类别的对应关系：
```
Actual \ Pred     0    1    2    3    4    5    6    7    8    9
----------------------------------------------------------------
    0            10    0    0    0    0    0    0    0    0    0
    1             0   11    0    0    0    0    0    0    0    0
    ...
```

### 每类指标
```
Class | Accuracy | Precision | Recall | F1-Score
-------------------------------------------------------
  0   |  100.0%  |   100.0%  | 100.0% |  100.0%
  1   |  100.0%  |   100.0%  | 100.0% |  100.0%
  ...
```

- **Accuracy**: 该类的分类准确率
- **Precision**: 预测为该类的样本中，实际为该类的比例
- **Recall**: 实际为该类的样本中，被正确预测的比例
- **F1-Score**: Precision和Recall的调和平均

## 文件结构

```
mnist/
├── README.md              # 本文件（英文版）
├── README_CN.md          # 本文件（中文版）
├── setup.sh              # 快速设置脚本
├── download_mnist.py     # MNIST下载和预处理
├── train_mnist.py        # CNN训练脚本
├── mnist_test.cpp        # 随机权重测试
├── mnist_inference.cpp   # 训练权重推理测试
├── data/                 # MNIST数据集（自动生成）
│   ├── train_images.bin
│   ├── train_labels.bin
│   ├── test_images.bin
│   ├── test_labels.bin
│   ├── validation_images.bin
│   ├── validation_labels.bin
│   ├── quick_test_images.bin
│   └── quick_test_labels.bin
└── weights/              # 训练权重（训练后生成）
    ├── conv1_weights.bin
    ├── conv1_bias.bin
    ├── conv2_weights.bin
    ├── conv2_bias.bin
    ├── fc1_weights.bin
    ├── fc1_bias.bin
    ├── fc2_weights.bin
    ├── fc2_bias.bin
    ├── best_model.pth    # PyTorch模型
    └── weights_meta.txt  # 权重元数据
```

## Make目标总结

### 数据准备
```bash
make mnist_download        # 下载MNIST数据集
```

### 随机权重测试（验证流程）
```bash
make mnist_test_quick      # 10张图片
make mnist_test_validation # 100张图片
make mnist_test_full       # 10,000张图片
```

### 模型训练
```bash
make mnist_train           # 训练CNN并导出权重
```

### 训练权重推理（实际性能）
```bash
make mnist_inference_quick      # 10张图片
make mnist_inference_validation # 100张图片
make mnist_inference_full       # 10,000张图片
```

### 清理
```bash
make clean_mnist          # 清理MNIST数据和权重
```

## 故障排除

### "Cannot open file" 错误
确保先运行 `make mnist_download` 下载数据。

### "PyTorch not installed" 错误
安装PyTorch：
```bash
pip3 install torch torchvision
```

### 编译错误
确保设置了正确的Xilinx HLS工具路径：
```bash
source /path/to/Xilinx/Vitis_HLS/2024.1/settings64.sh
```

### 低准确率（使用训练权重）
- 检查权重文件是否正确加载
- 验证数据归一化是否与训练时一致
- 确保网络架构完全匹配

## 性能基准

### 随机权重
- **准确率**: ~10% (随机猜测)
- **用途**: 验证推理流程

### 训练权重（10 epochs）
- **训练集准确率**: ~99%
- **测试集准确率**: 95-98%
- **推理速度**: ~100-1000 images/sec (取决于硬件)

### HLS硬件实现
- **预期加速**: 10-100x (相比CPU实现)
- **功耗**: 低功耗（相比GPU）
- **延迟**: 亚毫秒级单图推理

## 进阶使用

### 调整训练参数

```bash
python3 train_mnist.py \
    --epochs 20 \        # 训练轮数
    --batch-size 128 \   # 批次大小
    --lr 0.0005          # 学习率
```

### 自定义测试数据

修改 `download_mnist.py` 中的参数来创建不同大小的测试集。

### 导出其他格式的权重

修改 `train_mnist.py` 中的 `export_weights()` 函数以支持其他格式。

## 集成到HLS流程

在完成软件验证后，可以继续进行硬件综合：

```bash
# 运行HLS C仿真
make hls_csim

# 运行HLS综合
make hls_synth

# 运行HLS协同仿真（RTL验证）
make hls_cosim
```

## 参考资料

- [MNIST数据集](http://yann.lecun.com/exdb/mnist/)
- [LeNet论文](http://yann.lecun.com/exdb/publis/pdf/lecun-01a.pdf)
- [Xilinx Vitis HLS文档](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)
- [PyTorch文档](https://pytorch.org/docs/stable/index.html)

## 许可证

Copyright 2025 HLS CNN Project
Licensed under the Apache License, Version 2.0
