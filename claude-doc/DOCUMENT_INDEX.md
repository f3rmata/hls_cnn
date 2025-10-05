# 项目文档索引

## 📋 快速导航

### 🚀 新用户必读

1. **[QUICKSTART.md](QUICKSTART.md)** - 5分钟快速开始
   - 最快上手方式
   - 基本命令
   - 常见问题

2. **[README.md](README.md)** - 项目总览
   - 项目简介
   - 完整架构说明
   - 详细功能列表

### 🎓 训练相关

3. **[tests/mnist/QUICKFIX.md](../tests/mnist/QUICKFIX.md)** - ⭐⭐⭐ 最快速的修复指南
   - 3步快速修复
   - 一键命令
   - 问题排查清单
   - **推荐首先阅读**

4. **[tests/mnist/TRAINING_USAGE.md](../tests/mnist/TRAINING_USAGE.md)** - 训练脚本使用指南
   - 智能启动器使用
   - 参数说明
   - 故障排除
   - 最佳实践

5. **[tests/mnist/TRAINING_README.md](../tests/mnist/TRAINING_README.md)** - 详细训练文档
   - 架构说明
   - 训练参数
   - HLS集成

6. **[tests/mnist/ACCURACY_IMPROVEMENT.md](../tests/mnist/ACCURACY_IMPROVEMENT.md)** - 准确率提升详细指南
   - 问题根源分析
   - 多种解决方案
   - 技术原理

7. **[tests/mnist/QUANTIZATION_FIX_SUMMARY.md](../tests/mnist/QUANTIZATION_FIX_SUMMARY.md)** - 量化修复技术总结
   - 根本原因分析
   - 数学解释
   - 修复效果对比

8. **训练工具脚本**
   - `tests/mnist/run_train.sh` - ⭐ 智能训练启动器
   - `tests/mnist/check_env.py` - 环境检查工具
   - `tests/mnist/fix_quantization.py` - 自动修复量化问题
   - `tests/mnist/train.sh` - 简单启动脚本
   - `tests/mnist/train_model.py` - 主训练脚本

9. **[tests/mnist/DATA_FIX.md](../tests/mnist/DATA_FIX.md)** - 数据类型修复
   - uint8/int32 数据类型问题
   - 修复步骤
   - 验证方法

### 🔧 优化相关

5. **[FINAL_SOLUTION.md](FINAL_SOLUTION.md)** - 最终优化方案
   - 6-8-64架构说明
   - 资源使用分析
   - 优化历程

6. **[STATUS.md](STATUS.md)** - 当前状态
   - 实时综合状态
   - 资源使用预估
   - 风险评估

### 📦 更新日志

7. **[UPDATE_SUMMARY.md](UPDATE_SUMMARY.md)** - 项目更新总结
   - 核心更新内容
   - 迁移指南
   - 性能对比

8. **[CLEANUP_SUMMARY.md](CLEANUP_SUMMARY.md)** - 清理工作总结
   - 新增文件
   - 删除文件
   - 使用方法

## 🗂️ 文档分类

### 按用途分类

#### 入门文档
- [QUICKSTART.md](QUICKSTART.md) ⭐ 推荐首读
- [README.md](README.md)

#### 训练文档
- [tests/mnist/TRAINING_README.md](tests/mnist/TRAINING_README.md) ⭐ 训练必读
- `tests/mnist/train_model.py` (内有详细注释)

#### 技术文档
- [FINAL_SOLUTION.md](FINAL_SOLUTION.md)
- [STATUS.md](STATUS.md)
- `src/cnn_marco.h` (架构定义)

#### 更新文档
- [UPDATE_SUMMARY.md](UPDATE_SUMMARY.md)
- [CLEANUP_SUMMARY.md](CLEANUP_SUMMARY.md)
- [QUICK_FIX.md](QUICK_FIX.md)

### 按读者分类

#### 使用者
```
QUICKSTART.md
    ↓
tests/mnist/TRAINING_README.md
    ↓
Makefile (make help)
```

#### 开发者
```
README.md
    ↓
FINAL_SOLUTION.md
    ↓
src/cnn_marco.h
    ↓
src/hls_cnn.h
```

#### 维护者
```
UPDATE_SUMMARY.md
    ↓
CLEANUP_SUMMARY.md
    ↓
STATUS.md
```

## 📚 完整文档列表

### 根目录文档

| 文件 | 说明 | 优先级 |
|------|------|--------|
| QUICKSTART.md | 快速开始指南 | ⭐⭐⭐ |
| README.md | 项目主文档 | ⭐⭐⭐ |
| FINAL_SOLUTION.md | 最终优化方案 | ⭐⭐ |
| UPDATE_SUMMARY.md | 更新总结 | ⭐⭐ |
| CLEANUP_SUMMARY.md | 清理说明 | ⭐ |
| STATUS.md | 当前状态 | ⭐ |
| QUICK_FIX.md | 快速修复 | ⭐ |
| Makefile | 构建脚本 | ⭐⭐⭐ |

### 训练目录文档

| 文件 | 说明 | 优先级 |
|------|------|--------|
| tests/mnist/TRAINING_README.md | 训练详细文档 | ⭐⭐⭐ |
| tests/mnist/train_model.py | 训练脚本 | ⭐⭐⭐ |
| tests/mnist/train.sh | 启动脚本 | ⭐⭐ |
| tests/mnist/download_mnist.py | 数据下载 | ⭐⭐ |

### 源代码文档

| 文件 | 说明 | 优先级 |
|------|------|--------|
| src/cnn_marco.h | 架构定义 | ⭐⭐⭐ |
| src/hls_cnn.h | HLS实现 | ⭐⭐ |
| src/hls_cnn.cpp | 顶层函数 | ⭐⭐ |

### Claude文档 (历史)

| 目录 | 说明 |
|------|------|
| claude-doc/ | 之前的优化文档 (历史参考) |

## 🎯 使用场景

### 场景1: 首次使用

```
阅读: QUICKSTART.md
执行: make mnist_download
执行: make mnist_train_quick
阅读: tests/mnist/TRAINING_README.md (了解详情)
```

### 场景2: 训练模型

```
阅读: tests/mnist/TRAINING_README.md
执行: make mnist_train
验证: make mnist_inference_full
```

### 场景3: HLS开发

```
阅读: README.md
阅读: FINAL_SOLUTION.md
修改: src/cnn_marco.h
执行: make hls_synth
```

### 场景4: 问题排查

```
阅读: tests/mnist/TRAINING_README.md (故障排除章节)
阅读: QUICKSTART.md (常见问题)
查看: STATUS.md (当前状态)
```

### 场景5: 了解优化过程

```
阅读: FINAL_SOLUTION.md (最终方案)
阅读: UPDATE_SUMMARY.md (更新历程)
阅读: claude-doc/ (详细历史)
```

## 🔍 关键概念索引

### 架构相关
- 6-8-64架构: FINAL_SOLUTION.md, QUICKSTART.md
- 资源使用: STATUS.md, FINAL_SOLUTION.md
- ap_fixed<16,8>: tests/mnist/TRAINING_README.md

### 训练相关
- QAT: tests/mnist/TRAINING_README.md
- BatchNorm融合: tests/mnist/TRAINING_README.md
- 数据增强: tests/mnist/train_model.py

### HLS相关
- Pipeline II: src/hls_cnn.h, FINAL_SOLUTION.md
- 数组分区: FINAL_SOLUTION.md
- 综合报告: STATUS.md

## 💡 推荐阅读路径

### 路径A: 快速上手 (30分钟)
```
1. QUICKSTART.md (5分钟)
2. make mnist_download (2分钟)
3. make mnist_train_quick (15分钟)
4. make mnist_inference_full (1分钟)
5. tests/mnist/TRAINING_README.md (7分钟)
```

### 路径B: 深入理解 (2小时)
```
1. README.md (15分钟)
2. FINAL_SOLUTION.md (20分钟)
3. tests/mnist/TRAINING_README.md (20分钟)
4. src/cnn_marco.h + src/hls_cnn.h (30分钟)
5. make mnist_train (40分钟)
```

### 路径C: 完整掌握 (1天)
```
1. 路径B的所有内容
2. UPDATE_SUMMARY.md (15分钟)
3. make hls_synth (15分钟)
4. 实验不同参数 (4小时)
5. 阅读claude-doc/ (1小时)
```

## 📞 获取帮助

### 命令行帮助
```bash
make help              # Makefile所有命令
python3 train_model.py --help  # 训练参数说明
```

### 文档快速查找
```bash
# 查找关键词
grep -r "关键词" *.md

# 查看目录
ls -la *.md
ls -la tests/mnist/*.md
```

## 🔄 文档更新

### 最后更新
- **日期**: 2025-10-04
- **版本**: 1.0.0
- **架构**: 6-8-64

### 更新内容
- ✅ 统一训练脚本
- ✅ 完善文档体系
- ✅ 更新Makefile
- ✅ 清理旧文件

---

**提示**: 如果这是您第一次使用本项目，强烈建议从 [QUICKSTART.md](QUICKSTART.md) 开始！
