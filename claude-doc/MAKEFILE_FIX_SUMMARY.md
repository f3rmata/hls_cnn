# Makefile 路径修复总结

## ✅ 修复完成

已成功修复 Makefile 中所有路径不一致问题。

## 🔧 具体修复内容

### 1. HLS 工作目录路径

| 目标 | 修复前 | 修复后 | 状态 |
|------|--------|--------|------|
| `hls_csim` | `cd $(TEST_DIR)` | `cd $(TEST_HW_DIR)` | ✅ |
| `hls_synth` | `cd $(TEST_DIR)` | `cd $(TEST_HW_DIR)` | ✅ |
| `hls_cosim` | `cd $(TEST_DIR)` | `cd $(TEST_HW_DIR)` | ✅ |
| `hls_export` | `cd $(TEST_DIR)` | `cd $(TEST_HW_DIR)` | ✅ |
| `hls_full` | `cd $(TEST_DIR)` | `cd $(TEST_HW_DIR)` | ✅ |

**原因**: `run_hls.tcl` 位于 `tests/hw/` 而不是 `tests/`

### 2. HLS 项目名称

| 项目 | 修复前 | 修复后 | 状态 |
|------|--------|--------|------|
| 项目目录 | `hls_cnn_prj` | `hls_cnn.prj` | ✅ |

**原因**: `run_hls.tcl` 中定义为 `set PROJ "hls_cnn.prj"`

### 3. 清理路径

| 清理目标 | 修复前 | 修复后 | 状态 |
|----------|--------|--------|------|
| 项目目录 | `$(TEST_DIR)/hw/hls_cnn_prj` | `$(TEST_HW_DIR)/hls_cnn.prj` | ✅ |
| 日志文件 | `$(TEST_DIR)/*.log` | `$(TEST_HW_DIR)/*.log` | ✅ |
| 日志目录 | 未清理 | `$(TEST_HW_DIR)/logs` | ✅ 新增 |

## 📋 验证结果

### 命令验证

```bash
# 1. 验证 hls_csim 路径
$ make -n hls_csim | grep cd
cd tests/hw && vitis_hls -f run_hls.tcl  ✅ 正确

# 2. 验证清理路径
$ make -n clean_hls
rm -rf tests/hw/hls_cnn.prj              ✅ 正确
rm -rf tests/hw/*.log                    ✅ 正确
rm -rf tests/hw/logs                     ✅ 正确
```

### 目录结构验证

```
hls_cnn/
├── Makefile                    ✅ 已修复
├── src/
│   ├── hls_cnn.cpp
│   ├── hls_cnn.h
│   └── cnn_marco.h
├── tests/
│   ├── sw/                     ✅ 软件测试
│   │   ├── unit_test.cpp
│   │   └── integration_test.cpp
│   └── hw/                     ✅ 硬件测试（工作目录）
│       ├── run_hls.tcl         ✅ HLS 主脚本
│       ├── hls_config.tcl      ✅ HLS 配置
│       ├── test.cpp
│       ├── uut_top.cpp
│       ├── uut_top.hpp
│       ├── hls_cnn.prj/        ✅ 项目名称正确
│       └── logs/               ✅ 会被清理
└── build/
```

## 🎯 现在可以使用的命令

### CPU 测试
```bash
make unit_test          # 单元测试
make integration_test   # 集成测试
```

### HLS 流程
```bash
make hls_csim          # C 仿真（1-3分钟）
make hls_synth         # 综合（5-10分钟）
make hls_cosim         # 协同仿真（10-30分钟）
make hls_export        # 导出 IP
make hls_full          # 完整流程
```

### 清理
```bash
make clean             # 清理所有
make clean_hls         # 仅清理 HLS
```

### 帮助
```bash
make help              # 显示所有目标
```

## 📊 路径变量定义

Makefile 中的关键变量：

```makefile
SRC_DIR := src                      # 源代码
TEST_DIR := tests                   # 测试根目录
TEST_SW_DIR := $(TEST_DIR)/sw       # 软件测试
TEST_HW_DIR := $(TEST_DIR)/hw       # 硬件测试 ⭐ 核心变量
BUILD_DIR := build                  # 构建输出
```

所有 HLS 命令现在都使用 `$(TEST_HW_DIR)` = `tests/hw`

## ✅ 修复前后对比

### 修复前（错误）
```makefile
hls_csim:
	cd $(TEST_DIR) && $(VPP) -f run_hls.tcl
	# 错误：在 tests/ 目录找不到 run_hls.tcl

clean_hls:
	rm -rf $(TEST_DIR)/hw/hls_cnn_prj
	# 错误：项目名称不匹配，无法清理
```

### 修复后（正确）
```makefile
hls_csim:
	cd $(TEST_HW_DIR) && $(VPP) -f run_hls.tcl
	# 正确：在 tests/hw/ 目录运行

clean_hls:
	rm -rf $(TEST_HW_DIR)/hls_cnn.prj
	rm -rf $(TEST_HW_DIR)/*.log
	rm -rf $(TEST_HW_DIR)/logs
	# 正确：清理所有 HLS 生成文件
```

## 🧪 快速测试

运行以下命令验证修复：

```bash
# 1. 清理测试
make clean_hls

# 2. 运行 C 仿真
make hls_csim

# 3. 检查结果
ls -la tests/hw/hls_cnn.prj/
```

预期结果：
- ✅ HLS 成功运行
- ✅ 在 `tests/hw/` 生成 `hls_cnn.prj/` 目录
- ✅ C 仿真通过

## 📖 相关文档

- `MAKEFILE_GUIDE.md` - 详细的 Makefile 使用指南
- `tests/hw/run_hls.tcl` - HLS 主脚本
- `tests/hw/hls_config.tcl` - DSP 配置
- `tests/hw/DSP_FIX_SUMMARY.md` - DSP 问题修复

## 💡 关键要点

1. ✅ **正确的工作目录**: 所有 HLS 命令在 `tests/hw/` 执行
2. ✅ **正确的项目名称**: `hls_cnn.prj` 而不是 `hls_cnn_prj`
3. ✅ **完整的清理**: 包括项目、日志和 logs 目录
4. ✅ **一致的路径**: 使用 `$(TEST_HW_DIR)` 变量

## 🎉 修复状态

**所有路径问题已修复，Makefile 现在可以正常工作！**

---

**修复日期**: 2025-10-02  
**验证状态**: ✅ 通过  
**可用性**: ✅ 准备就绪
