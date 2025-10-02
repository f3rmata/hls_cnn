# Makefile 使用指南

## 📋 路径修复说明

已修复 Makefile 中以下路径不一致问题：

### 修复内容

1. **HLS 脚本路径**
   - ❌ 错误: `cd $(TEST_DIR)` → 指向 `tests/`
   - ✅ 正确: `cd $(TEST_HW_DIR)` → 指向 `tests/hw/`
   - 原因: `run_hls.tcl` 位于 `tests/hw/` 目录下

2. **HLS 项目名称**
   - ❌ 错误: `hls_cnn_prj`
   - ✅ 正确: `hls_cnn.prj`
   - 原因: `run_hls.tcl` 中定义为 `hls_cnn.prj`

3. **清理路径**
   - ✅ 添加: `rm -rf $(TEST_HW_DIR)/logs`
   - 原因: HLS 会在 `tests/hw/logs/` 生成日志文件

## 🗂️ 项目目录结构

```
hls_cnn/
├── Makefile                    # 主构建文件（已修复）
├── src/
│   ├── hls_cnn.h              # CNN 头文件
│   ├── hls_cnn.cpp            # CNN 实现
│   └── cnn_marco.h            # 宏定义
├── tests/
│   ├── sw/                    # 软件测试
│   │   ├── unit_test.cpp
│   │   └── integration_test.cpp
│   └── hw/                    # 硬件测试
│       ├── run_hls.tcl        # HLS 主脚本 ⭐
│       ├── hls_config.tcl     # HLS 配置
│       ├── test.cpp           # 测试平台
│       ├── uut_top.cpp        # 硬件顶层
│       ├── uut_top.hpp        # 硬件顶层头文件
│       ├── hls_cnn.prj/       # HLS 项目（生成）
│       └── logs/              # 日志目录（生成）
├── build/                     # 构建输出
└── claude-doc/                # 文档目录
```

## 🎯 Makefile 变量定义

```makefile
# 关键目录变量
SRC_DIR := src                      # 源代码目录
TEST_DIR := tests                   # 测试根目录
TEST_SW_DIR := $(TEST_DIR)/sw       # 软件测试
TEST_HW_DIR := $(TEST_DIR)/hw       # 硬件测试 ⭐
BUILD_DIR := build                  # 构建输出
```

## 📝 使用方法

### 1. CPU 软件测试

```bash
# 运行单元测试
make unit_test

# 运行集成测试
make integration_test

# 运行所有测试
make all
```

### 2. HLS 硬件流程

```bash
# C 仿真（快速，推荐首先运行）
make hls_csim

# 仅综合（不运行仿真）
make hls_synth

# 协同仿真（慢，验证 RTL）
make hls_cosim

# 导出 IP（用于 Vivado 集成）
make hls_export

# 完整流程（csim + synth + cosim）
make hls_full
```

### 3. 清理

```bash
# 清理所有构建文件
make clean

# 仅清理 HLS 项目
make clean_hls
```

### 4. 帮助

```bash
# 显示所有可用目标
make help
```

## 🔧 工作目录说明

### 所有 HLS 命令的工作目录

所有 `hls_*` 目标都会先切换到 `tests/hw/` 目录：

```makefile
cd $(TEST_HW_DIR) && vitis_hls -f run_hls.tcl
```

这是因为：
1. `run_hls.tcl` 位于 `tests/hw/`
2. `hls_config.tcl` 位于 `tests/hw/`
3. 相对路径配置基于 `tests/hw/`

### HLS 生成的文件

运行 HLS 后，会在 `tests/hw/` 下生成：

```
tests/hw/
├── hls_cnn.prj/           # HLS 项目目录
│   ├── sol/               # 解决方案
│   │   ├── syn/           # 综合结果
│   │   │   └── report/    # 综合报告
│   │   ├── sim/           # 仿真结果
│   │   │   └── report/    # 仿真报告
│   │   └── impl/          # 实现结果（如果导出）
│   └── solution1.log      # 日志
├── logs/                  # 其他日志
├── vitis_hls.log         # Vitis HLS 主日志
└── hls_run.log           # 运行日志（如果使用脚本）
```

## ⚙️ 配置说明

### sed 命令说明

Makefile 使用 `sed` 临时修改 `run_hls.tcl` 中的标志：

```makefile
# 仅运行综合（禁用 C 仿真）
sed -i 's/set CSIM 1/set CSIM 0/' run_hls.tcl
vitis_hls -f run_hls.tcl
sed -i 's/set CSIM 0/set CSIM 1/' run_hls.tcl  # 恢复
```

这样可以：
- 保持 `run_hls.tcl` 默认配置不变
- 根据需要临时启用/禁用不同阶段
- 自动恢复原始设置

### 控制标志

`run_hls.tcl` 中的控制标志：

```tcl
set CSIM 1          # C 仿真
set CSYNTH 1        # C 综合
set COSIM 1         # 协同仿真
set VIVADO_SYN 0    # Vivado 综合
set VIVADO_IMPL 0   # Vivado 实现
```

## 🚀 典型工作流程

### 开发流程

```bash
# 1. 先运行软件测试（快速验证算法）
make unit_test
make integration_test

# 2. 运行 HLS C 仿真（验证 HLS 兼容性）
make hls_csim

# 3. 运行 HLS 综合（查看资源使用）
make hls_synth

# 4. 查看综合报告
cat tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt

# 5. 如果需要，运行协同仿真（验证 RTL）
make hls_cosim

# 6. 导出 IP（用于 Vivado 项目）
make hls_export
```

### 快速测试

```bash
# 快速验证（仅 C 仿真）
make clean_hls && make hls_csim
```

### 完整验证

```bash
# 完整流程（包含 RTL 验证）
make clean && make hls_full
```

## 📊 性能估算

| 目标 | 时间估算 | 说明 |
|------|---------|------|
| `unit_test` | < 1 秒 | 快速单元测试 |
| `integration_test` | < 5 秒 | CPU 端到端测试 |
| `hls_csim` | 1-3 分钟 | HLS C 仿真 |
| `hls_synth` | 5-10 分钟 | HLS 综合 |
| `hls_cosim` | 10-30 分钟 | RTL 协同仿真（慢）⚠️ |
| `hls_export` | 10-20 分钟 | IP 导出 |
| `hls_full` | 20-45 分钟 | 完整流程 |

## 🔍 故障排查

### 问题 1: 找不到 run_hls.tcl

**错误**:
```
cd tests && vitis_hls -f run_hls.tcl
ERROR: cannot find run_hls.tcl
```

**原因**: 工作目录错误，应该在 `tests/hw/`

**解决**: 已修复，现在使用 `cd $(TEST_HW_DIR)`

### 问题 2: 清理不完整

**现象**: `make clean_hls` 后仍有残留文件

**原因**: 旧 Makefile 使用了错误的项目名称

**解决**: 已修复路径：
- `tests/hw/hls_cnn.prj` （正确）
- `tests/hw/logs/`

### 问题 3: sed 修改不生效

**原因**: sed 在某些系统上需要备份文件

**解决**: 使用 `-i` 选项（Linux）或 `-i ''`（macOS）

## 📖 扩展阅读

- `run_hls.tcl` - HLS 主脚本配置
- `hls_config.tcl` - DSP 和优化配置
- `DSP_FIX_SUMMARY.md` - DSP 问题修复文档

## ✅ 验证 Makefile

运行以下命令验证修复：

```bash
# 1. 验证变量
make -n hls_csim | grep "cd"

# 应该看到: cd tests/hw && vitis_hls -f run_hls.tcl

# 2. 验证清理路径
make -n clean_hls

# 应该看到: rm -rf tests/hw/hls_cnn.prj tests/hw/*.log tests/hw/logs

# 3. 运行测试
make clean_hls
make hls_csim
```

---

**最后更新**: 2025-10-02  
**状态**: ✅ 所有路径已修复
