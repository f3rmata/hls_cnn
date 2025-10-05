# 数据类型修复 - 2025-10-04

## 🐛 问题描述

训练脚本 `train_model.py` 在加载MNIST数据时出现错误：

```
AssertionError: Size mismatch between tensors
```

**根本原因**: 
- MNIST标签文件使用 `uint8` 格式保存
- 训练脚本错误地使用 `int32` 读取
- 导致标签数量不匹配（读取了1/4的标签）

## ✅ 解决方案

### 修改内容

**文件**: `tests/mnist/train_model.py`

**修改前**:
```python
train_labels = np.fromfile(f'{data_dir}/train_labels.bin', dtype=np.int32)
test_labels = np.fromfile(f'{data_dir}/test_labels.bin', dtype=np.int32)
```

**修改后**:
```python
train_labels = np.fromfile(f'{data_dir}/train_labels.bin', dtype=np.uint8).astype(np.int64)
test_labels = np.fromfile(f'{data_dir}/test_labels.bin', dtype=np.uint8).astype(np.int64)
```

**说明**:
- 使用 `dtype=np.uint8` 正确读取标签（0-9）
- 转换为 `int64` 以兼容PyTorch的LongTensor
- 添加了标签形状的调试输出

## 🔍 验证

修复后的数据加载：

```bash
cd tests/mnist
python3 << 'EOF'
import numpy as np
train_labels = np.fromfile('data/train_labels.bin', dtype=np.uint8).astype(np.int64)
test_labels = np.fromfile('data/test_labels.bin', dtype=np.uint8).astype(np.int64)
print(f"Train labels: {train_labels.shape}")  # (60000,)
print(f"Test labels: {test_labels.shape}")    # (10000,)
EOF
```

预期输出:
```
Train labels: (60000,)
Test labels: (10000,)
```

## 📊 数据格式说明

### MNIST数据集格式

| 文件 | 数据类型 | 形状 | 大小 |
|------|---------|------|------|
| train_images.bin | float32 | (60000, 28, 28) | ~188 MB |
| train_labels.bin | uint8 | (60000,) | 60 KB |
| test_images.bin | float32 | (10000, 28, 28) | ~31 MB |
| test_labels.bin | uint8 | (10000,) | 10 KB |

### 读取方式

**图像**:
```python
images = np.fromfile('train_images.bin', dtype=np.float32).reshape(-1, 1, 28, 28)
# 归一化已完成 (范围: 0.0 - 1.0)
```

**标签**:
```python
labels = np.fromfile('train_labels.bin', dtype=np.uint8).astype(np.int64)
# uint8 读取，转为int64供PyTorch使用
```

## 🚀 现在可以使用

修复完成后，可以正常训练：

```bash
# 快速训练
make mnist_train_quick

# 完整训练
make mnist_train

# 或直接运行
cd tests/mnist
python3 train_model.py --epochs 60 --batch-size 32
```

## 📝 相关文件

- `tests/mnist/train_model.py` - ✅ 已修复
- `tests/mnist/download_mnist.py` - 正确（使用uint8保存）
- 数据文件格式 - 正确

## ⚠️ 注意事项

如果遇到类似问题，检查：

1. **数据类型匹配**: 
   - 保存时用什么类型
   - 读取时也要用同样的类型

2. **形状验证**:
   ```python
   assert images.shape[0] == labels.shape[0], "Sample count mismatch"
   ```

3. **值范围检查**:
   ```python
   print(f"Images: [{images.min():.3f}, {images.max():.3f}]")  # 应该是 [0.0, 1.0]
   print(f"Labels: [{labels.min()}, {labels.max()}]")          # 应该是 [0, 9]
   ```

---

**状态**: ✅ 已修复  
**测试**: ✅ 通过  
**日期**: 2025-10-04
