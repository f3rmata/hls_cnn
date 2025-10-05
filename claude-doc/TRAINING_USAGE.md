# 训练脚本使用指南

## 🚀 快速开始

### 方法1: 使用智能启动器 (推荐) ⭐

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist

# 快速验证 (5 epochs, ~3分钟)
./run_train.sh verify

# 快速训练 (20 epochs, ~15分钟)
./run_train.sh quick

# 完整训练 (60 epochs, ~40分钟)
./run_train.sh full

# 自定义参数
./run_train.sh custom --epochs 30 --lr 0.002
```

**优点**: 
- ✅ 自动检测和激活conda环境
- ✅ 自动检查数据是否存在
- ✅ 自动验证PyTorch安装
- ✅ 无需手动cd和conda activate

### 方法2: 手动训练

```bash
# 1. 进入目录
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist

# 2. 激活环境
conda activate hls_cnn

# 3. 运行训练
python3 train_model.py --epochs 5

# 或使用make
cd ../..  # 回到hls_cnn目录
make mnist_train_quick
```

## 📋 训练参数

### 基本参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--epochs` | 60 | 训练轮数 |
| `--batch-size` | 32 | 批次大小 |
| `--lr` | 0.0015 | 初始学习率 |
| `--dropout` | 0.4 | Dropout比率 |
| `--no-augment` | False | 禁用数据增强 |
| `--device` | auto | 设备: cuda/cpu/auto |

### 使用示例

```bash
# 快速验证 (5 epochs, 不使用数据增强)
python3 train_model.py --epochs 5 --no-augment

# 快速训练 (20 epochs)
python3 train_model.py --epochs 20

# 完整训练 (60 epochs, 大batch)
python3 train_model.py --epochs 60 --batch-size 64

# 低学习率训练
python3 train_model.py --epochs 40 --lr 0.001

# GPU训练 (如果有)
python3 train_model.py --epochs 60 --device cuda
```

## 🔧 环境配置

### 检查环境

```bash
# 查看conda环境
conda env list

# 激活环境
conda activate hls_cnn

# 验证PyTorch
python3 -c "import torch; print(torch.__version__)"
```

### 如果环境不存在

```bash
# 创建环境
conda create -n hls_cnn python=3.9 -y

# 激活环境
conda activate hls_cnn

# 安装依赖
conda install pytorch torchvision -c pytorch
conda install numpy
```

## 📊 训练模式对比

| 模式 | Epochs | 时间 | 预期准确率 | 用途 |
|------|--------|------|-----------|------|
| **验证** | 5 | ~3分钟 | 75-80% | 快速检查修复是否成功 |
| **快速** | 20 | ~15分钟 | 88-91% | 日常开发测试 |
| **完整** | 60 | ~40分钟 | 92-93% | 最终部署 |

## 🐛 故障排除

### 问题1: 未激活conda环境

**现象**:
```
ModuleNotFoundError: No module named 'torch'
```

**解决**:
```bash
conda activate hls_cnn
# 或使用智能启动器
./run_train.sh verify
```

### 问题2: 准确率仍然很低 (~11%)

**现象**:
```
Epoch 5: Test Acc: 11.5%
```

**检查**:
```bash
# 确认量化已被禁用
grep "# x = self.quant(x)" train_model.py

# 应该看到4行被注释的量化操作
```

**修复**:
```bash
# 重新运行修复脚本
python3 fix_quantization.py

# 清理旧模型
rm -rf weights/ checkpoints/

# 重新训练
./run_train.sh verify
```

### 问题3: 数据不存在

**现象**:
```
FileNotFoundError: data/train_images.bin
```

**解决**:
```bash
# 下载数据
python3 download_mnist.py

# 或直接运行智能启动器（会自动下载）
./run_train.sh verify
```

### 问题4: 内存不足

**现象**:
```
RuntimeError: CUDA out of memory
```

**解决**:
```bash
# 方案1: 减小batch size
python3 train_model.py --batch-size 16

# 方案2: 使用CPU
python3 train_model.py --device cpu

# 方案3: 禁用数据增强（减少内存）
python3 train_model.py --no-augment
```

## 📁 输出文件

### 训练完成后

```
tests/mnist/
├── weights/                    # 权重文件（用于HLS）
│   ├── conv1_weights.bin      # 600 bytes
│   ├── conv1_bias.bin         # 24 bytes
│   ├── conv2_weights.bin      # 4.7K
│   ├── conv2_bias.bin         # 32 bytes
│   ├── fc1_weights.bin        # 32K
│   ├── fc1_bias.bin           # 256 bytes
│   ├── fc2_weights.bin        # 2.5K
│   └── fc2_bias.bin           # 40 bytes
├── checkpoints/
│   └── best_model.pth         # PyTorch模型检查点
└── quick_test.log             # 训练日志（如果用了验证脚本）
```

### 检查权重

```bash
# 查看权重文件
ls -lh weights/

# 验证文件大小
du -sh weights/
# 应该显示约 40K

# 查看权重范围（需要Python）
python3 << 'EOF'
import numpy as np
w = np.fromfile('weights/conv1_weights.bin', dtype=np.float32)
print(f'Conv1 weights: shape={w.shape}, range=[{w.min():.3f}, {w.max():.3f}]')
EOF
```

## 🎯 最佳实践

### 1. 开发流程

```bash
# Step 1: 快速验证代码改动
./run_train.sh verify  # 5 epochs

# Step 2: 如果验证通过，快速训练
./run_train.sh quick   # 20 epochs

# Step 3: 如果准确率满意，完整训练
./run_train.sh full    # 60 epochs
```

### 2. 准确率目标

- ✅ 验证模式 (5 epochs): > 75%
- ✅ 快速模式 (20 epochs): > 88%
- ✅ 完整模式 (60 epochs): > 90%

如果达不到目标，检查：
1. 量化是否被禁用 (`grep "# x = self.quant" train_model.py`)
2. 数据是否正确加载 (60000张训练图像)
3. conda环境是否正确激活

### 3. HLS集成

训练完成后:

```bash
# 1. 检查权重文件
ls -lh tests/mnist/weights/

# 2. 运行HLS C仿真
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
make hls_csim

# 3. 运行综合
make hls_synth

# 4. 查看资源使用
grep "LUT" tests/hw/hls_cnn.prj/sol/syn/report/*_csynth.rpt
```

## 📚 相关文档

- **[HOW_TO_IMPROVE_ACCURACY.md](HOW_TO_IMPROVE_ACCURACY.md)** - 准确率提升快速指南
- **[ACCURACY_IMPROVEMENT.md](ACCURACY_IMPROVEMENT.md)** - 详细的问题分析
- **[QUANTIZATION_FIX_SUMMARY.md](QUANTIZATION_FIX_SUMMARY.md)** - 量化修复技术细节
- **[TRAINING_README.md](TRAINING_README.md)** - 完整训练文档

## 💡 提示

1. **首次训练**: 使用 `./run_train.sh verify` 快速验证
2. **日常开发**: 使用 `./run_train.sh quick` 平衡速度和精度
3. **最终部署**: 使用 `./run_train.sh full` 获得最佳精度
4. **调试问题**: 检查日志文件 `quick_test.log`

---

**快速命令**:
```bash
# 最简单的方式
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist
./run_train.sh verify
```
