# Python模型优化总结

## 🎯 目标

为优化后的HLS CNN创建匹配的Python训练模型，并实现量化感知训练(QAT)以最大化FPGA部署精度。

## 📋 修改内容

### 新增文件

1. **`train_mnist_optimized.py`** - 优化模型训练脚本
   - 匹配HLS架构：Conv1(4ch) → Conv2(8ch) → FC1(64) → FC2(10)
   - 实现量化感知训练(QAT)
   - 模拟ap_fixed<16,8>行为
   - 自动导出权重

2. **`train_optimized.sh`** - 快速启动脚本
   - 自动设置Python环境
   - 安装依赖
   - 运行训练

3. **`TRAINING_GUIDE.md`** - 完整训练指南
   - 架构说明
   - QAT原理
   - 最佳实践
   - 故障排除

4. **`compare_models.py`** - 模型对比工具
   - 参数量对比
   - 内存占用对比
   - FLOPs对比

## 🔄 架构变化

### HLS模型架构

```cpp
// cnn_marco.h
#define CONV1_OUT_CH 4    // 从16减少到4
#define CONV2_OUT_CH 8    // 从32减少到8
#define FC1_OUT_SIZE 64   // 从128减少到64
#define CONV1_KERNEL_SIZE 5  // 从3增加到5
#define CONV2_KERNEL_SIZE 5  // 从3增加到5
```

### Python模型架构

```python
class OptimizedHLSCNN(nn.Module):
    def __init__(self):
        # Conv1: 1 -> 4 channels, kernel=5
        self.conv1 = nn.Conv2d(1, 4, kernel_size=5)
        
        # Conv2: 4 -> 8 channels, kernel=5
        self.conv2 = nn.Conv2d(4, 8, kernel_size=5)
        
        # FC1: 128 -> 64
        self.fc1 = nn.Linear(128, 64)
        
        # FC2: 64 -> 10
        self.fc2 = nn.Linear(64, 10)
```

## 📊 对比数据

| 指标 | 原始模型 | 优化模型 | 变化 |
|------|---------|---------|------|
| Conv1通道 | 16 | 4 | -75% |
| Conv2通道 | 32 | 8 | -75% |
| FC1大小 | 128 | 64 | -50% |
| **总参数** | **25,010** | **9,818** | **-61%** |
| 模型大小 | 97.7 KB | 38.4 KB | -61% |
| MACs | 698K | 118K | -83% |

## 🔬 量化感知训练(QAT)

### 核心原理

```python
class FakeQuantize(nn.Module):
    """模拟ap_fixed<16,8>"""
    def forward(self, x):
        # 量化到256个级别
        x = torch.round(x * 256) / 256
        # 截断到[-128, 127.996]
        x = torch.clamp(x, -128, 127.996)
        return x
```

### 应用位置

- ✅ 输入量化
- ✅ Conv1输出量化
- ✅ Conv2输出量化
- ✅ FC1输出量化
- ❌ FC2输出（logits，不量化）

## 🚀 使用方法

### 1. 快速训练

```bash
cd tests/mnist
./train_optimized.sh
```

### 2. 自定义训练

```bash
# 使用QAT（推荐）
python train_mnist_optimized.py --epochs 20 --batch-size 64

# 不使用QAT（更快）
python train_mnist_optimized.py --epochs 20 --no-qat

# 高精度训练
python train_mnist_optimized.py --epochs 30 --lr 0.0005
```

### 3. 对比模型

```bash
python compare_models.py
```

### 4. 查看训练指南

```bash
cat TRAINING_GUIDE.md
```

## 📁 生成的文件

训练完成后会生成：

```
weights/
├── best_model_optimized.pth    # PyTorch模型检查点
├── conv1_weights.bin           # 4×1×5×5 = 100 floats
├── conv1_bias.bin              # 4 floats
├── conv2_weights.bin           # 8×4×5×5 = 800 floats
├── conv2_bias.bin              # 8 floats
├── fc1_weights.bin             # 64×128 = 8192 floats
├── fc1_bias.bin                # 64 floats
├── fc2_weights.bin             # 10×64 = 640 floats
├── fc2_bias.bin                # 10 floats
└── weights_meta.txt            # 元数据
```

## 🔧 HLS集成

### 1. 复制权重

```bash
cp weights/*.bin ../hw/
```

### 2. 更新HLS测试代码

确保`uut_top.cpp`中的权重数组大小匹配：

```cpp
// Conv1: [4][1][5][5]
weight_t conv1_weights[4][1][5][5];
weight_t conv1_bias[4];

// Conv2: [8][4][5][5]
weight_t conv2_weights[8][4][5][5];
weight_t conv2_bias[8];

// FC1: [64][128]
weight_t fc1_weights[64][128];
weight_t fc1_bias[64];

// FC2: [10][64]
weight_t fc2_weights[10][64];
weight_t fc2_bias[10];
```

### 3. 运行C仿真

```bash
cd ../hw
vitis_hls -f run_hls.tcl
# 设置CSIM=1进行C仿真
```

## 📈 预期精度

| 配置 | 预期精度 | 训练时间 |
|------|---------|---------|
| 原始模型(float32) | 98.5-99.0% | 5分钟 |
| 优化模型(无QAT) | 96.5-97.5% | 3分钟 |
| **优化模型(QAT)** | **97.5-98.5%** | **4分钟** |

## ⚠️ 注意事项

### 1. 精度损失

- 参数减少61%会导致一定精度损失
- QAT可以补偿大部分损失
- 预期精度损失< 1%（相比原始模型2-3%）

### 2. 训练技巧

- 使用更多epochs（20-30）
- 考虑数据增强
- 可以使用知识蒸馏从大模型学习
- 启用早停避免过拟合

### 3. 权重范围

- 确保权重在[-128, 127]范围内
- QAT会自动处理
- 导出时检查weights_meta.txt

## 🐛 常见问题

### Q1: 精度太低怎么办？

**A**: 
1. 增加训练epochs到30-40
2. 使用数据增强
3. 尝试知识蒸馏
4. 调整学习率

### Q2: 如何验证权重正确？

**A**:
```bash
# 查看权重范围
python -c "
import numpy as np
w = np.fromfile('weights/conv1_weights.bin', dtype=np.float32)
print(f'Conv1 weights: min={w.min():.4f}, max={w.max():.4f}')
"
```

### Q3: 如何从旧模型迁移？

**A**:
```python
# 使用知识蒸馏
teacher = torch.load('weights/best_model.pth')  # 旧模型
student = OptimizedHLSCNN()  # 新模型
# 训练student模拟teacher的输出
```

## 📚 相关文档

- `TRAINING_GUIDE.md` - 完整训练指南
- `../hw/hls_config.tcl` - HLS配置
- `../../src/cnn_marco.h` - HLS架构定义
- `../../FINAL_OPTIMIZATION_REPORT.md` - 优化总报告

## ✅ 检查清单

训练前：
- [ ] 已下载MNIST数据
- [ ] Python环境已配置
- [ ] PyTorch已安装

训练后：
- [ ] 测试精度 > 97%
- [ ] 权重文件已生成(10个.bin文件)
- [ ] weights_meta.txt检查无误
- [ ] 权重范围在[-128, 127]

HLS集成：
- [ ] 权重文件已复制到../hw/
- [ ] uut_top.cpp数组大小已更新
- [ ] C仿真通过
- [ ] 精度匹配Python结果

---

**状态**: ✅ 已完成  
**测试**: ⏳ 待训练验证  
**文档**: ✅ 已完成

## 🎉 下一步

1. **运行训练**：
   ```bash
   cd tests/mnist
   ./train_optimized.sh
   ```

2. **验证权重**：
   ```bash
   python compare_models.py
   ```

3. **HLS集成**：
   - 复制权重文件
   - 运行HLS仿真
   - 验证精度

4. **FPGA部署**：
   - HLS综合
   - Vivado实现
   - 板上测试
