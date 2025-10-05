# 优化模型训练指南

## 📋 概述

本文档说明如何训练优化后的CNN模型，该模型已经适配Zynq 7020的资源限制。

### 网络架构对比

| 层 | 原始模型 | 优化模型 | 减少 |
|----|---------|---------|------|
| Conv1 | 16通道, 3×3 | 4通道, 5×5 | -75% |
| Conv2 | 32通道, 3×3 | 8通道, 5×5 | -75% |
| FC1 | 128神经元 | 64神经元 | -50% |
| **总参数** | **25,010** | **9,818** | **-61%** |

### 关键特性

✅ **量化感知训练(QAT)**：模拟ap_fixed<16,8>行为  
✅ **匹配HLS架构**：与FPGA设计完全一致  
✅ **参数减少61%**：更小的模型，更快的推理  
✅ **自动导出权重**：直接用于HLS测试

## 🚀 快速开始

### 方法1：使用脚本

```bash
cd tests/mnist
./train_optimized.sh
```

### 方法2：手动训练

```bash
cd tests/mnist

# 1. 确保有MNIST数据
python download_mnist.py

# 2. 训练模型（使用QAT）
python train_mnist_optimized.py --epochs 20 --batch-size 64 --lr 0.001

# 3. 不使用QAT训练（更快，但可能精度略低）
python train_mnist_optimized.py --epochs 20 --no-qat
```

## 📊 模型架构详情

### 优化后的网络

```
输入: [1×28×28] (784像素)
  ↓
Conv1: 4个5×5卷积核, stride=1, padding=0
  输出: [4×24×24]
  参数: 4×1×5×5 + 4 = 104
  ↓
MaxPool1: 2×2池化
  输出: [4×12×12]
  ↓
Conv2: 8个5×5×4卷积核, stride=1, padding=0
  输出: [8×8×8]
  参数: 8×4×5×5 + 8 = 808
  ↓
MaxPool2: 2×2池化
  输出: [8×4×4]
  ↓
Flatten: 展平
  输出: [128]
  ↓
FC1: 全连接层, 128→64
  输出: [64] + ReLU
  参数: 64×128 + 64 = 8,256
  ↓
FC2: 输出层, 64→10
  输出: [10] (Logits)
  参数: 10×64 + 10 = 650

总参数: 9,818
总运算: ~185K MACs/推理
```

## 🔬 量化感知训练(QAT)

### 什么是QAT？

量化感知训练在训练过程中模拟量化误差，使模型学会适应低精度表示。

### 实现细节

```python
class FakeQuantize(nn.Module):
    """模拟ap_fixed<16,8>行为"""
    def __init__(self, num_bits=16, num_int_bits=8):
        # ap_fixed<16,8>参数
        # - 16位总宽度
        # - 8位整数部分
        # - 8位小数部分
        # 范围: [-128, 127.996]
        # 精度: 1/256 = 0.00390625
        self.scale = 256  # 2^8
        self.min_val = -128
        self.max_val = 127.996
        
    def forward(self, x):
        # 量化: round(x * 256) / 256
        x_quant = torch.round(x * self.scale) / self.scale
        # 截断到有效范围
        x_quant = torch.clamp(x_quant, self.min_val, self.max_val)
        return x_quant
```

### QAT的优势

| 对比 | 无QAT | 有QAT |
|------|-------|-------|
| 训练时间 | 更快 | 稍慢(+10%) |
| 精度损失 | 2-5% | <1% |
| FPGA精度匹配 | 较差 | 很好 |
| 推荐场景 | 快速测试 | 生产部署 |

## 📈 训练参数

### 推荐设置

```bash
# 标准训练（推荐）
python train_mnist_optimized.py \
    --epochs 20 \
    --batch-size 64 \
    --lr 0.001

# 快速训练（测试用）
python train_mnist_optimized.py \
    --epochs 10 \
    --batch-size 128 \
    --lr 0.002 \
    --no-qat

# 高精度训练
python train_mnist_optimized.py \
    --epochs 30 \
    --batch-size 32 \
    --lr 0.0005
```

### 学习率调度

训练使用StepLR调度器：
- 每10个epoch学习率减半
- 初始lr: 0.001
- Epoch 10: 0.0005
- Epoch 20: 0.00025

## 📁 输出文件

训练完成后生成以下文件：

```
weights/
├── best_model_optimized.pth    # PyTorch模型（用于继续训练）
├── conv1_weights.bin           # Conv1权重（4×1×5×5 = 100个float32）
├── conv1_bias.bin              # Conv1偏置（4个float32）
├── conv2_weights.bin           # Conv2权重（8×4×5×5 = 800个float32）
├── conv2_bias.bin              # Conv2偏置（8个float32）
├── fc1_weights.bin             # FC1权重（64×128 = 8192个float32）
├── fc1_bias.bin                # FC1偏置（64个float32）
├── fc2_weights.bin             # FC2权重（10×64 = 640个float32）
├── fc2_bias.bin                # FC2偏置（10个float32）
└── weights_meta.txt            # 元数据和统计信息
```

### 二进制格式

所有`.bin`文件使用**小端序float32**格式：
- 每个值占4字节
- IEEE 754单精度浮点格式
- 可直接在HLS C++中读取

## 🔧 HLS集成

### 1. 复制权重文件

```bash
# 从Python训练目录复制到HLS测试目录
cp weights/*.bin ../hw/
```

### 2. 在HLS中加载权重

```cpp
// uut_top.cpp
void load_weights() {
    FILE* fp;
    
    // Load conv1 weights
    fp = fopen("conv1_weights.bin", "rb");
    fread(conv1_weights_flat, sizeof(float), 100, fp);
    fclose(fp);
    
    // Reshape to [4][1][5][5]
    int idx = 0;
    for(int oc=0; oc<4; oc++) {
        for(int ic=0; ic<1; ic++) {
            for(int kh=0; kh<5; kh++) {
                for(int kw=0; kw<5; kw++) {
                    conv1_weights[oc][ic][kh][kw] = conv1_weights_flat[idx++];
                }
            }
        }
    }
    
    // 类似地加载其他层...
}
```

### 3. 运行HLS仿真

```bash
cd ../hw
vitis_hls -f run_hls.tcl
```

## 📊 预期性能

### 精度基准

| 模型 | 参数量 | 测试精度 | 备注 |
|------|--------|---------|------|
| 原始(float32) | 25K | 98.5-99.0% | 基准 |
| 优化(无QAT) | 9.8K | 96.5-97.5% | -2% |
| **优化(QAT)** | **9.8K** | **97.5-98.5%** | **-0.5%** |

### 训练时间

| 设置 | GPU (RTX 3080) | CPU (8核) |
|------|----------------|-----------|
| 10 epochs | ~2分钟 | ~10分钟 |
| 20 epochs | ~4分钟 | ~20分钟 |
| 30 epochs | ~6分钟 | ~30分钟 |

## 🎯 最佳实践

### 1. 数据增强

对于更好的泛化，可以添加数据增强：

```python
from torchvision import transforms

transform = transforms.Compose([
    transforms.RandomRotation(5),
    transforms.RandomAffine(0, translate=(0.1, 0.1)),
])
```

### 2. 早停

监控验证损失，避免过拟合：

```python
# 在train_mnist_optimized.py中添加
patience = 5
no_improve = 0

if test_loss > best_loss:
    no_improve += 1
    if no_improve >= patience:
        print("Early stopping!")
        break
```

### 3. 知识蒸馏

从大模型迁移知识到小模型：

```python
# 训练大模型
teacher = LargeModel()  # 原始16-32通道模型
train(teacher)

# 使用teacher指导student训练
student = OptimizedHLSCNN()  # 4-8通道模型
distill_train(student, teacher, temperature=3.0)
```

## 🐛 故障排除

### 问题1：精度太低

**症状**：测试精度 < 95%

**解决方案**：
1. 增加训练轮数到30-40
2. 降低学习率到0.0005
3. 增加batch size到128
4. 使用数据增强
5. 考虑知识蒸馏

### 问题2：训练不收敛

**症状**：Loss不下降或震荡

**解决方案**：
1. 降低学习率
2. 启用梯度裁剪（QAT模式已启用）
3. 检查数据归一化
4. 减小batch size

### 问题3：权重范围异常

**症状**：权重值超出[-128, 127]

**解决方案**：
1. 确保使用QAT模式
2. 检查fake_quant层是否工作
3. 添加权重正则化
4. 降低学习率

## 📚 参考资料

### 量化相关

- [PyTorch Quantization](https://pytorch.org/docs/stable/quantization.html)
- [Quantization-Aware Training](https://arxiv.org/abs/1712.05877)
- [Fixed-Point Arithmetic](https://en.wikipedia.org/wiki/Fixed-point_arithmetic)

### HLS相关

- [Vitis HLS User Guide](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2024_1/ug1399-vitis-hls.pdf)
- [ap_fixed Reference](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/C-Arbitrary-Precision-Fixed-Point-Types)

## 💡 进阶技巧

### 混合精度训练

不同层使用不同精度：

```python
# Conv层使用ap_fixed<12,6>（更低精度）
# FC层使用ap_fixed<16,8>（标准精度）
self.fake_quant_conv1 = FakeQuantize(12, 6)
self.fake_quant_fc1 = FakeQuantize(16, 8)
```

### 稀疏化

进一步减少参数量：

```python
import torch.nn.utils.prune as prune

# 剪枝50%的权重
prune.l1_unstructured(model.conv1, name='weight', amount=0.5)
prune.l1_unstructured(model.fc1, name='weight', amount=0.5)
```

### 通道剪枝

自动找到最优通道数：

```python
# 使用通道重要性分析
from channel_pruning import analyze_channels

important_channels = analyze_channels(model)
# 根据分析结果调整网络结构
```

## ✅ 检查清单

训练前：
- [ ] MNIST数据已下载
- [ ] Python环境已配置
- [ ] PyTorch已安装

训练后：
- [ ] 测试精度 > 97%
- [ ] 权重文件已生成
- [ ] weights_meta.txt检查无误
- [ ] 权重范围在[-128, 127]内

HLS集成前：
- [ ] 权重文件已复制到HLS目录
- [ ] HLS C仿真通过
- [ ] 精度匹配Python结果(误差<1%)

---

**最后更新**: 2025-10-04  
**版本**: 1.0  
**作者**: HLS CNN Optimization Team
