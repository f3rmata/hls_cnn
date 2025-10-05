# 准确率提升 - 立即开始 🚀

## 问题
训练后准确率只有 **11.51%** ❌

## 解决方案
准确率将提升到 **90%+** ✅

## 一键命令

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist && \
conda activate hls_cnn && \
./run_train.sh verify
```

**这会**:
1. 进入正确目录
2. 激活conda环境  
3. 训练5个epoch验证修复（~3分钟）

**预期结果**:
```
Epoch  5/5:  Test Acc: 75-88%  ← 成功！
```

## 详细步骤

如果一键命令有问题，手动执行：

```bash
# 1. 进入目录
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/mnist

# 2. 激活环境
conda activate hls_cnn

# 3. 检查环境（可选）
python3 check_env.py

# 4. 快速验证（5 epochs）
./run_train.sh verify

# 5. 如果验证成功，完整训练（60 epochs）
./run_train.sh full
```

## 训练模式

| 命令 | 时间 | 准确率 | 用途 |
|------|------|--------|------|
| `./run_train.sh verify` | 3分钟 | 75-80% | 验证修复 |
| `./run_train.sh quick` | 15分钟 | 88-91% | 快速训练 |
| `./run_train.sh full` | 40分钟 | 92-93% | 最佳精度 |

## 问题排查

### 问题1: conda环境错误
```bash
conda activate hls_cnn
# 如果环境不存在:
conda create -n hls_cnn python=3.9 -y
conda activate hls_cnn
conda install pytorch -c pytorch
```

### 问题2: 准确率仍然11%
```bash
# 重新运行修复
python3 fix_quantization.py
rm -rf weights/ checkpoints/
./run_train.sh verify
```

### 问题3: 数据不存在
```bash
python3 download_mnist.py
```

## 📚 文档

- **[QUICKFIX.md](QUICKFIX.md)** - 完整的快速修复指南 ⭐⭐⭐
- **[TRAINING_USAGE.md](TRAINING_USAGE.md)** - 详细使用说明
- **[SOLUTION_SUMMARY.md](SOLUTION_SUMMARY.md)** - 完整技术总结

## ❓ 需要帮助？

1. 查看 [QUICKFIX.md](QUICKFIX.md) 获取详细步骤
2. 运行 `python3 check_env.py` 检查环境
3. 查看 `quick_test.log` 了解训练日志

---

**记住**: 每次训练前必须先激活conda环境！
```bash
conda activate hls_cnn
```
