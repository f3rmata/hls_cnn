# MNIST Training Guide - 6-8-64 Architecture

## 📋 模型架构

当前HLS实现使用以下架构（定义在`src/cnn_marco.h`）：

```
输入: 1×28×28 (MNIST灰度图像)
  ↓
Conv1: 6个5×5卷积核 → 6×24×24
  BatchNorm + ReLU + Quantize
  ↓
MaxPool 2×2 → 6×12×12
  ↓
Conv2: 8个5×5×6卷积核 → 8×8×8
  BatchNorm + ReLU + Quantize
  ↓
MaxPool 2×2 → 8×4×4
  ↓
Flatten → 128 (8×4×4)
  ↓
FC1: 128→64 + ReLU + Dropout
  BatchNorm + Quantize
  ↓
FC2: 64→10 (输出logits)
  Quantize
```

**总参数**: ~10,000  
**目标精度**: 90-93%  
**数据类型**: ap_fixed<16,8> (16位定点，8整数位，8小数位)

## 🚀 快速开始

### 1. 下载MNIST数据集

```bash
make mnist_download
```

这将下载并处理MNIST数据集到`tests/mnist/data/`目录。

### 2. 训练模型

**标准训练** (60 epochs, ~40分钟):
```bash
make mnist_train
```

**快速训练** (20 epochs, ~15分钟, 用于测试):
```bash
make mnist_train_quick
```

### 3. 验证权重导出

训练完成后，权重会自动导出到`tests/mnist/weights/`：
```
weights/
├── conv1_weights.bin  (600 bytes)
├── conv1_bias.bin     (24 bytes)
├── conv2_weights.bin  (4,800 bytes)
├── conv2_bias.bin     (32 bytes)
├── fc1_weights.bin    (32,768 bytes)
├── fc1_bias.bin       (256 bytes)
├── fc2_weights.bin    (2,560 bytes)
└── fc2_bias.bin       (40 bytes)
```

### 4. 运行推理测试

```bash
# 快速测试 (10张图片)
make mnist_inference_quick

# 完整测试 (10,000张图片)
make mnist_inference_full
```

## 🎯 训练参数说明

### 默认参数 (60 epochs)

```python
--epochs 60          # 训练轮数
--batch-size 32      # 批大小 (较小更稳定)
--lr 0.0015          # 学习率
--dropout 0.4        # Dropout率
```

### 自定义训练

```bash
cd tests/mnist

# 更长训练
python3 train_model.py --epochs 80 --batch-size 32

# 更快训练
python3 train_model.py --epochs 30 --batch-size 64 --lr 0.002

# 无数据增强 (更快但精度可能低)
python3 train_model.py --epochs 40 --no-augment

# 使用CPU
python3 train_model.py --epochs 60 --device cpu
```

## 📊 预期结果

### 训练曲线

```
Epoch  1/60:  Train Loss: 0.8234, Acc: 72.34%  |  Test Loss: 0.4521, Acc: 85.23%
Epoch 10/60:  Train Loss: 0.2156, Acc: 93.45%  |  Test Loss: 0.1834, Acc: 91.12%
Epoch 20/60:  Train Loss: 0.1234, Acc: 95.67%  |  Test Loss: 0.1123, Acc: 92.89%
Epoch 30/60:  Train Loss: 0.0892, Acc: 96.89%  |  Test Loss: 0.0956, Acc: 93.45%
Epoch 40/60:  Train Loss: 0.0723, Acc: 97.45%  |  Test Loss: 0.0834, Acc: 93.78%
Epoch 50/60:  Train Loss: 0.0645, Acc: 97.89%  |  Test Loss: 0.0789, Acc: 93.92%
  *** New best: 93.92% - Model saved ***
```

### 最终精度

| 训练方式 | 预期测试精度 |
|---------|-------------|
| 标准训练 (60 epochs) | 90-93% |
| 快速训练 (20 epochs) | 88-91% |
| 无数据增强 | 87-90% |

## 🔍 关键特性

### 1. 量化感知训练 (QAT)

模型使用`FakeQuantize`层模拟HLS的ap_fixed<16,8>量化：
- **范围**: -128 到 127.996
- **精度**: 1/256 (约0.004)
- **好处**: Python和HLS精度差异<1%

### 2. BatchNorm融合

训练时使用BatchNorm加速收敛，导出时自动融合到卷积/全连接层权重中：
```python
# 训练时
Conv → BN → ReLU

# HLS部署时
Conv(融合BN的权重) → ReLU
```

**优势**: 零额外计算开销！

### 3. 数据增强

默认启用随机平移±2像素：
- 训练集从60,000张→120,000张
- 提升泛化能力
- 精度提升2-3%

### 4. 训练技巧

- **Dropout**: 0.4 防止过拟合
- **Label Smoothing**: 0.15 提升泛化
- **Weight Decay**: 0.0002 L2正则化
- **Cosine Annealing**: 学习率平滑衰减
- **Gradient Clipping**: 防止梯度爆炸
- **Early Stopping**: 15轮无改进自动停止

## 🛠️ 故障排除

### 问题1: PyTorch未安装

```
ERROR: PyTorch not installed
```

**解决**:
```bash
pip3 install torch torchvision
```

### 问题2: MNIST数据未找到

```
ERROR: MNIST data not found
```

**解决**:
```bash
make mnist_download
```

### 问题3: 精度太低 (<85%)

**可能原因**:
- Epoch太少
- 学习率不合适
- 数据未正确归一化

**解决**:
```bash
# 增加epoch
python3 train_model.py --epochs 80

# 调整学习率
python3 train_model.py --lr 0.001

# 检查数据
python3 -c "import numpy as np; d=np.fromfile('data/train_images.bin', dtype=np.float32); print(f'Range: {d.min():.3f} to {d.max():.3f}')"
# 应该输出: Range: 0.000 to 1.000
```

### 问题4: 训练/测试精度差距大 (>5%)

**症状**: 过拟合
```
Train Acc: 98%
Test Acc: 88%  # 差距10%
```

**解决**:
```bash
# 增加Dropout
python3 train_model.py --dropout 0.5

# 增加Weight Decay (需修改代码)
# 或减少训练轮数
python3 train_model.py --epochs 40
```

## 📈 与HLS集成

### 1. 验证权重格式

```bash
cd tests/mnist/weights
ls -lh *.bin

# 预期输出 (文件大小)
# conv1_weights.bin: 600 B   (6×1×5×5×4)
# conv1_bias.bin:    24 B    (6×4)
# conv2_weights.bin: 4.7K    (8×6×5×5×4)
# conv2_bias.bin:    32 B    (8×4)
# fc1_weights.bin:   32K     (64×128×4)
# fc1_bias.bin:      256 B   (64×4)
# fc2_weights.bin:   2.5K    (10×64×4)
# fc2_bias.bin:      40 B    (10×4)
```

### 2. HLS C仿真

```bash
make hls_csim
```

这将使用导出的权重在HLS中运行C仿真。

### 3. HLS综合

```bash
make hls_synth
```

检查资源使用：
```
Target: Zynq 7020 (xc7z020clg400-1)
LUT:  ~42,000 / 53,200 (79%)  ✓
FF:   ~40,000 / 106,400 (38%) ✓
DSP:  ~90 / 220 (41%)         ✓
BRAM: ~60 / 280 (21%)         ✓
```

### 4. 精度验证

Python训练精度和HLS推理精度应该非常接近：
```
Python模型测试精度: 93.45%
HLS C仿真精度:      93.12%
差异:               0.33%  ✓ (< 1%可接受)
```

## 📝 文件说明

### 训练相关
- `train_model.py` - 主训练脚本 (与HLS架构完全匹配)
- `download_mnist.py` - MNIST数据下载脚本
- `best_model.pth` - 最佳模型检查点

### 推理相关
- `mnist_inference.cpp` - HLS推理测试 (使用训练权重)
- `mnist_test.cpp` - HLS测试 (使用随机权重)

### 数据目录
- `data/` - MNIST数据集二进制文件
- `weights/` - 导出的权重文件

### 废弃文件 (可删除)
```bash
make clean_old_scripts
```

这将删除：
- `train_mnist.py`
- `train_mnist_optimized.py`
- `train_improved.py`
- `train_ultra_optimized.py`
- `train_optimized.sh`
- `train_improved.sh`

## 🎓 进阶优化

### 提升精度到95%+

如果需要更高精度（代价是更多资源）：

1. **增加通道数** (修改`src/cnn_marco.h`):
```cpp
#define CONV2_OUT_CH 10  // 从8增到10
#define FC1_OUT_SIZE 80  // 从64增到80
```

2. **重新训练**:
```bash
python3 train_model.py --epochs 80
```

3. **重新综合**:
```bash
make hls_synth
# 检查LUT是否超限
```

### 减小模型 (如果LUT仍超限)

1. **减小通道数**:
```cpp
#define CONV1_OUT_CH 4   // 从6减到4 (会显著降低精度!)
#define CONV2_OUT_CH 6   // 从8减到6
```

2. **增加Pipeline II**:
```cpp
// 在 src/hls_cnn.h 中
#pragma HLS PIPELINE II = 16  // 从8增到16
```

## 📚 参考资料

- [HLS CNN项目README](../../README.md)
- [HLS架构定义](../../src/cnn_marco.h)
- [Vitis HLS文档](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)

---

**最后更新**: 2025-10-04  
**架构版本**: 6-8-64 (最终优化版)
