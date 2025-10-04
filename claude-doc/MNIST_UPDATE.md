# HLS CNN - MNIST测试更新日志

## 新增功能

本次更新为HLS CNN项目添加了完整的MNIST手写数字识别测试套件。

## 新增文件

### tests/mnist/ 目录

1. **数据处理**
   - `download_mnist.py` - MNIST数据集下载和预处理工具
   - 自动下载、归一化并转换为二进制格式
   - 创建多个测试子集（快速/验证/完整）

2. **模型训练**
   - `train_mnist.py` - PyTorch CNN训练脚本
   - 匹配HLS CNN架构
   - 导出权重为二进制格式供C++使用
   - 支持GPU加速

3. **C++测试程序**
   - `mnist_test.cpp` - 使用随机权重的推理测试
   - `mnist_inference.cpp` - 使用训练权重的推理测试
   - 支持批量测试和性能分析
   - 提供混淆矩阵和指标分析

4. **辅助脚本**
   - `setup.sh` - 一键快速设置脚本
   - `run_all_tests.sh` - 综合测试脚本
   - `visualize_mnist.py` - 数据可视化工具

5. **文档**
   - `README.md` - 英文文档
   - `README_CN.md` - 中文文档
   - 详细的使用说明和故障排除

## Makefile更新

新增以下目标：

### 数据准备
```bash
make mnist_download          # 下载MNIST数据集
```

### 随机权重测试（验证流程）
```bash
make mnist_test_quick        # 10张图片（快速）
make mnist_test_validation   # 100张图片（中速）
make mnist_test_full         # 10,000张图片（慢速）
```

### 模型训练
```bash
make mnist_train             # 训练CNN并导出权重
```

### 训练权重推理（实际性能）
```bash
make mnist_inference_quick        # 10张图片
make mnist_inference_validation   # 100张图片
make mnist_inference_full         # 10,000张图片
```

### 清理
```bash
make clean_mnist             # 清理MNIST数据和权重
```

## 使用流程

### 快速开始（验证推理流程）

```bash
# 1. 下载数据
make mnist_download

# 2. 快速测试（随机权重，约10%准确率）
make mnist_test_quick
```

### 完整流程（训练+验证）

```bash
# 1. 下载数据
make mnist_download

# 2. 训练模型（需要PyTorch）
pip3 install torch torchvision
make mnist_train

# 3. 验证训练结果（应该95-98%准确率）
make mnist_inference_validation

# 4. 完整测试
make mnist_inference_full
```

### 自动化测试

```bash
cd tests/mnist
./run_all_tests.sh
```

## 网络架构

实现的CNN架构（简化LeNet风格）：

```
输入: [1×28×28]
  ↓
Conv1: [16×26×26] (3×3卷积，步长1)
  ↓ ReLU激活
MaxPool1: [16×13×13] (2×2池化)
  ↓
Conv2: [32×11×11] (3×3卷积，步长1)
  ↓ ReLU激活
MaxPool2: [32×5×5] (2×2池化)
  ↓ 展平
FC1: [128] (800→128全连接)
  ↓ ReLU激活
FC2: [10] (128→10全连接)
  ↓
输出: [10] (类别概率)
```

**参数统计**：
- Conv1: 16×1×3×3 + 16 = 160
- Conv2: 32×16×3×3 + 32 = 4,640
- FC1: 128×800 + 128 = 102,528
- FC2: 10×128 + 10 = 1,290
- **总计**: ~108,618 参数

## 性能指标

### 随机权重基线
- **准确率**: ~10% (随机猜测)
- **用途**: 验证推理流程正确性

### 训练权重（10 epochs）
- **训练集准确率**: ~99%
- **测试集准确率**: 95-98%
- **训练时间**: 
  - CPU: 5-10分钟
  - GPU: 1-2分钟

### 推理性能（C++实现）
- **速度**: ~100-1000 images/sec（取决于CPU）
- **内存**: ~2MB（模型参数）

## 测试数据集

### MNIST原始数据
- **训练集**: 60,000张28×28灰度图像
- **测试集**: 10,000张28×28灰度图像
- **类别**: 0-9共10个数字
- **来源**: http://yann.lecun.com/exdb/mnist/

### 生成的子集
- **quick_test**: 10张（快速验证）
- **validation**: 100张（开发测试）
- **test**: 10,000张（完整评估）

## 输出格式

测试程序提供以下输出：

1. **基本统计**
   - 总图像数
   - 正确预测数
   - 总体准确率

2. **混淆矩阵**
   - 显示每个类别的预测分布
   - 帮助识别易混淆的数字对

3. **每类指标**
   - Accuracy（准确率）
   - Precision（精确率）
   - Recall（召回率）
   - F1-Score（F1分数）

## 依赖项

### 必需（数据和基本测试）
- Python 3.6+
- NumPy
- g++ 支持C++14

### 可选（训练和可视化）
- PyTorch 1.8+ （训练模型）
- Matplotlib （数据可视化）
- CUDA（GPU加速训练）

安装方法：
```bash
# 基本依赖
pip3 install numpy

# 训练依赖
pip3 install torch torchvision

# 可视化依赖
pip3 install matplotlib
```

## 集成到HLS流程

完成软件验证后，可继续HLS硬件实现：

```bash
# C仿真（软件级验证）
make hls_csim

# 综合（生成RTL）
make hls_synth

# 协同仿真（RTL验证）
make hls_cosim

# 导出IP（Vivado集成）
make hls_export
```

## 目录结构

```
hls_cnn/
├── tests/
│   └── mnist/
│       ├── README.md              # 英文文档
│       ├── README_CN.md           # 中文文档
│       ├── setup.sh               # 快速设置
│       ├── run_all_tests.sh       # 综合测试
│       ├── download_mnist.py      # 数据下载
│       ├── train_mnist.py         # 模型训练
│       ├── mnist_test.cpp         # 随机权重测试
│       ├── mnist_inference.cpp    # 训练权重推理
│       ├── visualize_mnist.py     # 数据可视化
│       ├── data/                  # MNIST数据（自动生成）
│       │   ├── train_images.bin
│       │   ├── train_labels.bin
│       │   ├── test_images.bin
│       │   ├── test_labels.bin
│       │   ├── validation_images.bin
│       │   ├── validation_labels.bin
│       │   └── quick_test_images.bin
│       └── weights/               # 训练权重（训练后生成）
│           ├── conv1_weights.bin
│           ├── conv1_bias.bin
│           ├── conv2_weights.bin
│           ├── conv2_bias.bin
│           ├── fc1_weights.bin
│           ├── fc1_bias.bin
│           ├── fc2_weights.bin
│           └── fc2_bias.bin
└── build/
    ├── mnist_test              # 编译后的测试程序
    └── mnist_inference         # 编译后的推理程序
```

## 已知问题和限制

1. **PyTorch依赖**: 训练功能需要PyTorch，但测试不需要
2. **内存使用**: 完整测试需要~300MB RAM
3. **训练时间**: CPU训练较慢，建议使用GPU
4. **固定点转换**: 当前使用浮点，需要手动转换为定点进行HLS综合

## 后续改进计划

1. **性能优化**
   - 添加多线程支持
   - 优化内存布局
   - 批量推理

2. **功能增强**
   - 支持ONNX模型导入
   - 添加量化支持（INT8）
   - 实时可视化

3. **测试扩展**
   - 添加性能benchmark
   - 支持其他数据集（CIFAR-10, Fashion-MNIST）
   - 硬件加速器验证

## 参考资料

- [MNIST数据库](http://yann.lecun.com/exdb/mnist/)
- [LeNet论文](http://yann.lecun.com/exdb/publis/pdf/lecun-01a.pdf)
- [Xilinx Vitis HLS文档](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)
- [PyTorch文档](https://pytorch.org/docs/)

## 贡献者

- 初始实现：HLS CNN项目团队
- MNIST集成：2025年10月

## 许可证

Copyright 2025 HLS CNN Project
Licensed under the Apache License, Version 2.0
