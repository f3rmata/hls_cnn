# 快速开始指南 - 6-8-64 架构

## 🎯 当前架构

```
Conv1[6] → Pool → Conv2[8] → Pool → FC1[64] → FC2[10]
总参数: ~10,000
目标精度: 90-93%
目标设备: Zynq 7020 (xc7z020clg400-1)
```

## ⚡ 5分钟快速开始

### 1. 下载MNIST数据

```bash
make mnist_download
```

### 2. 训练模型

**快速训练** (20 epochs, ~15分钟):
```bash
make mnist_train_quick
```

**完整训练** (60 epochs, ~40分钟):
```bash
make mnist_train
```

### 3. 测试推理

```bash
make mnist_inference_full
```

预期输出:
```
Testing on 10000 images...
Accuracy: 93.45%
```

### 4. HLS综合

```bash
make hls_synth
```

预期资源:
```
LUT:  42,020 / 53,200 (79%)  ✓
FF:   39,690 / 106,400 (37%) ✓
DSP:      89 / 220 (40%)     ✓
BRAM:     58 / 280 (21%)     ✓
```

## 📁 关键文件

| 文件 | 说明 |
|------|------|
| `src/cnn_marco.h` | HLS架构定义 (6-8-64) |
| `src/hls_cnn.h` | HLS层实现 |
| `src/hls_cnn.cpp` | HLS顶层函数 |
| `tests/mnist/train_model.py` | Python训练脚本 |
| `tests/mnist/train.sh` | 快速训练脚本 |
| `Makefile` | 构建和测试 |

## 🛠️ 完整工作流

```bash
# 1. 准备数据
make mnist_download

# 2. 训练并导出权重
make mnist_train

# 3. 验证推理精度
make mnist_inference_full

# 4. HLS C仿真
make hls_csim

# 5. HLS综合
make hls_synth

# 6. (可选) RTL协同仿真
make hls_cosim

# 7. (可选) 导出IP
make hls_export
```

## 🎓 自定义训练

```bash
cd tests/mnist

# 标准训练
python3 train_model.py --epochs 60 --batch-size 32

# 快速实验
python3 train_model.py --epochs 20 --batch-size 64

# 无数据增强
python3 train_model.py --epochs 40 --no-augment

# 使用CPU
python3 train_model.py --epochs 60 --device cpu
```

## 📊 预期结果

### 训练精度

| 训练方式 | 测试精度 | 时间 |
|---------|---------|------|
| 快速 (20 epochs) | 88-91% | ~15分钟 |
| 标准 (60 epochs) | 90-93% | ~40分钟 |
| 扩展 (80 epochs) | 91-94% | ~55分钟 |

### 资源使用

| 资源 | 使用 | 可用 | 利用率 |
|------|------|------|--------|
| LUT | ~42K | 53.2K | 79% |
| FF | ~40K | 106.4K | 37% |
| DSP | ~90 | 220 | 41% |
| BRAM | ~60 | 280 | 21% |

**✅ 所有资源在安全范围内!**

## 🔧 故障排除

### PyTorch未安装

```bash
pip3 install torch torchvision
```

### 数据未找到

```bash
make mnist_download
```

### LUT超限

如果综合后LUT仍超限 (>53,200)：

1. 检查当前使用量:
```bash
grep "LUT" tests/hw/hls_cnn.prj/sol/syn/report/*_csynth.rpt
```

2. 如果超限，减小架构:
```cpp
// 编辑 src/cnn_marco.h
#define CONV2_OUT_CH 6  // 从8减到6
#define FC1_OUT_SIZE 48 // 从64减到48
```

3. 重新训练:
```bash
make mnist_train
```

### 精度太低 (<88%)

1. 增加训练轮数:
```bash
python3 tests/mnist/train_model.py --epochs 80
```

2. 调整学习率:
```bash
python3 tests/mnist/train_model.py --lr 0.001
```

3. 检查数据:
```bash
python3 -c "import numpy as np; d=np.fromfile('tests/mnist/data/train_images.bin', dtype=np.float32); print(f'Range: {d.min():.3f} to {d.max():.3f}')"
# 应该输出: Range: 0.000 to 1.000
```

## 📚 更多文档

- [完整README](README.md) - 项目详细说明
- [训练指南](tests/mnist/TRAINING_README.md) - 详细训练文档
- [最终方案](FINAL_SOLUTION.md) - 优化过程说明
- [清理总结](CLEANUP_SUMMARY.md) - 项目更新说明

## 🚀 下一步

### 提升精度

如果需要更高精度（会增加资源使用）：

```cpp
// src/cnn_marco.h
#define CONV2_OUT_CH 10  // 从8增到10
#define FC1_OUT_SIZE 80  // 从64增到80
```

然后重新训练和综合。

### 部署到硬件

1. 导出IP:
```bash
make hls_export
```

2. 在Vivado中集成IP

3. 生成比特流

4. 部署到Zynq 7020板卡

## 💡 提示

- **首次使用**: 先运行 `make mnist_train_quick` 快速验证流程
- **GPU加速**: 如有NVIDIA GPU，训练时间可减少70%
- **参数调优**: 修改架构后务必重新训练
- **版本控制**: 每次重大修改前备份权重文件

---

**最后更新**: 2025-10-04  
**架构版本**: 6-8-64  
**状态**: ✅ 生产就绪
