# HLS CNN MNIST测试 - 快速使用指南

## 🚀 5分钟快速开始

### 方法1：一键运行（最简单）

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist
./setup.sh
```

这会自动完成：
- ✓ 下载MNIST数据集
- ✓ 编译测试程序  
- ✓ 运行快速测试

### 方法2：使用Makefile（推荐）

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn

# 1. 下载数据（只需运行一次）
make mnist_download

# 2. 快速测试（10张图片，随机权重）
make mnist_test_quick
```

### 方法3：完整流程（包含训练）

```bash
# 前提：安装PyTorch
pip3 install torch torchvision

# 1. 下载数据
make mnist_download

# 2. 训练模型（10个epoch，约5-10分钟）
make mnist_train

# 3. 验证训练结果（100张图片，应该95-98%准确率）
make mnist_inference_validation

# 4. 完整测试（10,000张图片）
make mnist_inference_full
```

## 📊 预期结果

### 随机权重测试
```
Total images: 10
Correct predictions: 1
Accuracy: 10.00%
```
✓ 这是正常的！用于验证推理流程正确。

### 训练权重测试
```
Total images: 100
Correct predictions: 97
Accuracy: 97.00%
```
✓ 准确率应该在95-98%之间。

## 🔧 所有可用命令

### 数据管理
```bash
make mnist_download        # 下载MNIST数据集
make clean_mnist          # 清理所有数据和权重
```

### 测试（随机权重）
```bash
make mnist_test_quick      # 10张图片（~10秒）
make mnist_test_validation # 100张图片（~1分钟）
make mnist_test_full       # 10,000张图片（~10分钟）
```

### 训练
```bash
make mnist_train           # 训练CNN（需要PyTorch）
```

### 推理（训练权重）
```bash
make mnist_inference_quick      # 10张图片
make mnist_inference_validation # 100张图片
make mnist_inference_full       # 10,000张图片
```

### 辅助工具
```bash
cd tests/mnist

# 运行综合测试
./run_all_tests.sh

# 可视化数据（需要matplotlib）
python3 visualize_mnist.py quick_test
python3 visualize_mnist.py validation
```

## 📁 生成的文件

运行后会生成以下目录：

```
tests/mnist/
├── data/                  # MNIST数据集（~50MB）
│   ├── train_images.bin   # 60,000张训练图片
│   ├── test_images.bin    # 10,000张测试图片
│   ├── validation_images.bin  # 100张验证图片
│   └── quick_test_images.bin  # 10张快速测试图片
└── weights/               # 训练权重（~500KB）
    ├── conv1_weights.bin
    ├── conv2_weights.bin
    ├── fc1_weights.bin
    └── fc2_weights.bin
```

## ❓ 常见问题

### Q: "Cannot open file data/quick_test_images.bin"
A: 运行 `make mnist_download` 下载数据

### Q: "PyTorch not installed"
A: 安装PyTorch: `pip3 install torch torchvision`（仅训练需要）

### Q: "libstdc++.so.6: version GLIBCXX_3.4.XX not found"
A: 已修复！Makefile 会自动处理库路径冲突。如果仍有问题，参见 [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

### Q: "Trained weights not found"
A: 运行 `make mnist_train` 训练模型

### Q: 准确率很低（使用训练权重）
A: 确保已经运行 `make mnist_train` 并成功训练

### Q: 训练太慢
A: 
- 使用GPU（如果有）
- 减少epoch数：`cd tests/mnist && python3 train_mnist.py --epochs 5`

## 🔧 故障排除

详细的故障排除指南，请参阅：
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - 完整的问题解决指南

## 📖 详细文档

- **英文文档**: `tests/mnist/README.md`
- **中文文档**: `tests/mnist/README_CN.md`
- **更新日志**: `tests/mnist/MNIST_UPDATE.md`

## 🎯 下一步

完成MNIST测试后，可以：

1. **进行HLS综合**
   ```bash
   make hls_csim    # HLS C仿真
   make hls_synth   # HLS综合
   make hls_cosim   # HLS协同仿真
   ```

2. **查看综合报告**
   ```bash
   cd tests/hw
   cat hls_cnn.prj/solution1/syn/report/cnn_inference_csynth.rpt
   ```

3. **导出IP核**
   ```bash
   make hls_export
   ```

## 🌟 快速测试清单

- [ ] 下载数据: `make mnist_download`
- [ ] 快速测试: `make mnist_test_quick`
- [ ] (可选) 训练模型: `make mnist_train`
- [ ] (可选) 验证训练: `make mnist_inference_validation`
- [ ] 查看Makefile帮助: `make help`

---

**提示**: 如果遇到任何问题，查看详细文档或运行 `make help`
