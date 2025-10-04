# MNIST 测试故障排除

## 常见问题和解决方案

### 1. libstdc++ 版本冲突

**问题症状**：
```
../../build/mnist_test: /path/to/Xilinx/Vitis/.../libstdc++.so.6: version `GLIBCXX_3.4.32' not found
```

**原因**：
Xilinx Vitis 工具链在 `LD_LIBRARY_PATH` 中设置了旧版本的 `libstdc++`，而系统的 g++ 编译器（如 GCC 15）需要更新的版本。

**解决方案**：
Makefile 已经修复，会在运行 MNIST 测试时临时清除 `LD_LIBRARY_PATH`：

```makefile
cd $(TEST_MNIST_DIR) && LD_LIBRARY_PATH="" ../../$(BUILD_DIR)/mnist_test quick
```

这样测试程序会使用系统的标准库，而不是 Xilinx 的旧版本。

**手动运行**：
如果直接运行可执行文件，使用：
```bash
cd tests/mnist
LD_LIBRARY_PATH="" ../../build/mnist_test quick
```

### 2. "Cannot open file" 错误

**问题症状**：
```
ERROR: Cannot open file data/quick_test_images.bin
```

**解决方案**：
首先下载 MNIST 数据集：
```bash
make mnist_download
```

### 3. "PyTorch not installed" 错误

**问题症状**：
```
ERROR: PyTorch not installed
```

**解决方案**（仅训练需要）：
```bash
pip3 install torch torchvision
```

注意：只有 `make mnist_train` 需要 PyTorch，测试不需要。

### 4. "Trained weights not found" 错误

**问题症状**：
```
ERROR: Trained weights not found. Run 'make mnist_train' first.
```

**解决方案**：
```bash
# 确保已安装 PyTorch
pip3 install torch torchvision

# 训练模型
make mnist_train
```

或者使用随机权重测试（不需要训练）：
```bash
make mnist_test_quick  # 而不是 mnist_inference_quick
```

### 5. 编译错误：找不到 ap_fixed.h

**问题症状**：
```
fatal error: ap_fixed.h: No such file or directory
```

**解决方案**：
确保设置了 Xilinx HLS 环境：
```bash
source /path/to/Xilinx/Vitis_HLS/2024.1/settings64.sh
```

或检查 Makefile 中的 `XILINX_HLS` 路径是否正确。

### 6. 低准确率（使用训练权重）

**问题症状**：
使用 `mnist_inference_*` 时准确率仍然很低（~10%）

**可能原因**：
1. 权重文件未正确加载
2. 训练未成功完成
3. 数据归一化问题

**排查步骤**：
```bash
# 1. 检查权重文件是否存在
ls -lh tests/mnist/weights/

# 2. 重新训练
make mnist_train

# 3. 查看训练日志，确保达到 95%+ 准确率

# 4. 再次测试
make mnist_inference_validation
```

### 7. 训练速度慢

**问题**：
CPU 训练需要很长时间

**解决方案**：
1. **使用 GPU**（如果有）：
   ```bash
   # PyTorch 会自动检测和使用 GPU
   make mnist_train
   ```

2. **减少 epoch 数**：
   ```bash
   cd tests/mnist
   python3 train_mnist.py --epochs 5  # 而不是默认的 10
   ```

3. **减少批次大小**（如果内存不足）：
   ```bash
   python3 train_mnist.py --batch-size 32  # 默认 64
   ```

### 8. 内存不足

**问题症状**：
```
Killed
```
或
```
std::bad_alloc
```

**解决方案**：
1. **使用较小的测试集**：
   ```bash
   make mnist_test_quick       # 10 张图片
   make mnist_test_validation  # 100 张图片
   # 而不是
   make mnist_test_full        # 10,000 张图片
   ```

2. **关闭其他应用程序**

3. **增加系统交换空间**

### 9. Python 依赖问题

**问题症状**：
```
ModuleNotFoundError: No module named 'numpy'
```

**解决方案**：
安装所有 Python 依赖：
```bash
# 基本依赖（数据下载）
pip3 install numpy

# 训练依赖（可选）
pip3 install torch torchvision

# 可视化依赖（可选）
pip3 install matplotlib
```

### 10. 权限问题

**问题症状**：
```
Permission denied
```

**解决方案**：
确保脚本有执行权限：
```bash
chmod +x tests/mnist/*.sh
chmod +x tests/mnist/*.py
```

## 调试技巧

### 1. 详细输出

运行测试时查看完整输出：
```bash
make mnist_test_quick 2>&1 | tee test.log
```

### 2. 检查库依赖

```bash
ldd build/mnist_test
```

### 3. 验证数据文件

```bash
# 检查文件大小
ls -lh tests/mnist/data/

# 快速测试数据：10 × 28 × 28 × 4 bytes = 31,360 bytes
# 验证数据：100 × 28 × 28 × 4 bytes = 313,600 bytes
```

### 4. 测试单个组件

```bash
# 只编译，不运行
make build/mnist_test

# 手动运行
cd tests/mnist
LD_LIBRARY_PATH="" ../../build/mnist_test quick
```

### 5. 查看 Make 变量

```bash
make show_config
```

## 环境验证清单

在运行测试前，验证环境：

```bash
# ✓ Python 3 可用
python3 --version

# ✓ g++ 可用
g++ --version

# ✓ Make 可用
make --version

# ✓ NumPy 已安装
python3 -c "import numpy; print(numpy.__version__)"

# (可选) PyTorch 已安装
python3 -c "import torch; print(torch.__version__)"

# (可选) Xilinx 工具可用
which vitis_hls
```

## 获取帮助

如果问题仍未解决：

1. **查看完整文档**：
   - [README.md](README.md) - 英文文档
   - [README_CN.md](README_CN.md) - 中文文档
   - [QUICKSTART.md](QUICKSTART.md) - 快速开始

2. **查看日志**：
   ```bash
   # 训练日志
   less tests/mnist/training.log
   
   # HLS 日志
   less tests/hw/vitis_hls.log
   ```

3. **检查系统要求**：
   - Linux 系统（推荐 Ubuntu 20.04+）
   - 至少 4GB RAM
   - 至少 2GB 磁盘空间
   - g++ 7.5 或更高版本

4. **重新开始**：
   ```bash
   # 清理所有
   make clean
   make clean_mnist
   
   # 重新下载数据
   make mnist_download
   
   # 重新测试
   make mnist_test_quick
   ```

## 已知限制

1. **固定点转换**：当前测试使用浮点（`USE_FLOAT`），HLS 综合需要手动转换为定点
2. **内存使用**：完整测试（10,000 张图片）需要约 300MB RAM
3. **训练时间**：CPU 训练较慢（5-10 分钟），建议使用 GPU
4. **平台支持**：主要在 Linux 上测试，Windows/macOS 可能需要调整

## 报告问题

提供以下信息：
- 操作系统和版本
- g++ 版本：`g++ --version`
- Python 版本：`python3 --version`
- Xilinx 工具版本
- 完整错误消息
- 重现步骤
