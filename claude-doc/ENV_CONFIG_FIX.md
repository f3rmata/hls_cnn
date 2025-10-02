# 环境配置和芯片型号问题修复指南

## 📋 问题描述

在运行 HLS 流程时遇到以下问题：
1. 环境变量未配置（XILINX_VIVADO, XILINX_VITIS）
2. 找不到芯片型号（XPART）
3. vitis_hls 命令无法找到

## ✅ 已实施的修复

### 1. Makefile 环境检查（参考 Vitis_Libraries）

添加了完整的环境检查机制：

```makefile
############################## Environment Check ##############################
.PHONY: check_vivado check_vitis

check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "ERROR: Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

check_vitis:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/vitis))
	@echo "ERROR: Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif
```

### 2. 芯片型号配置

支持通过环境变量或命令行参数设置芯片型号：

```makefile
# Default part
XPART ?= xc7z020clg400-1

# Can be overridden via command line
# Example: make hls_csim XPART=xc7z020clg400-1
ifneq (,$(XPART))
export HLS_PART := $(XPART)
else
export HLS_PART := xc7z020clg400-1
endif
```

### 3. TCL 脚本更新

`run_hls.tcl` 现在支持从环境变量读取芯片型号：

```tcl
# Device part configuration
# Check if HLS_PART is set in environment (from Makefile)
if {[info exists ::env(HLS_PART)]} {
  set XPART $::env(HLS_PART)
  puts "Using device part from environment: $XPART"
} else {
  # Default part if not specified
  set XPART xc7z020clg400-1
  puts "Using default device part: $XPART"
}
```

### 4. 添加配置显示功能

```bash
make show_config
```

输出：
```
==========================================
HLS CNN Project Configuration
==========================================
XILINX_VIVADO : /path/to/Vivado/2024.1
XILINX_VITIS  : /path/to/Vitis/2024.1
HLS_PART      : xc7z020clg400-1
CUR_DIR       : /path/to/hls_cnn
VPP           : vitis_hls
==========================================
```

## 🚀 使用方法

### 步骤 1: 检查环境

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn

# 运行环境检查脚本
./check_xilinx_env.sh
```

### 步骤 2: 设置 Xilinx 环境

如果检查失败，需要设置环境变量：

#### 方法 A: 临时设置（推荐用于测试）

```bash
# 根据您的实际安装路径调整
source /home/fermata/Development/Software/Xilinx/Vivado/2024.1/settings64.sh
source /home/fermata/Development/Software/Xilinx/Vitis/2024.1/settings64.sh
```

#### 方法 B: 永久设置

编辑 `~/.zshrc` 或 `~/.bashrc`，添加：

```bash
# Xilinx Tools Environment
export XILINX_VIVADO=/home/fermata/Development/Software/Xilinx/Vivado/2024.1
export XILINX_VITIS=/home/fermata/Development/Software/Xilinx/Vitis/2024.1

# Source settings
if [ -f "$XILINX_VIVADO/settings64.sh" ]; then
    source "$XILINX_VIVADO/settings64.sh"
fi

if [ -f "$XILINX_VITIS/settings64.sh" ]; then
    source "$XILINX_VITIS/settings64.sh"
fi
```

然后重新加载：
```bash
source ~/.zshrc  # 或 source ~/.bashrc
```

### 步骤 3: 验证配置

```bash
# 显示项目配置
make show_config

# 应该看到所有环境变量都正确设置
```

### 步骤 4: 运行 HLS

```bash
# 使用默认芯片型号（xc7z020clg400-1）
make hls_csim

# 或者指定特定芯片型号
make hls_csim XPART=xc7z020clg400-1

# 其他 HLS 命令
make hls_synth XPART=xc7z020clg400-1
make hls_cosim XPART=xc7z020clg400-1
```

## 📊 芯片型号配置优先级

1. **命令行参数**（最高优先级）
   ```bash
   make hls_csim XPART=xcu200-fsgd2104-2-e
   ```

2. **环境变量**
   ```bash
   export XPART=xc7z020clg400-1
   make hls_csim
   ```

3. **Makefile 默认值**（最低优先级）
   ```makefile
   XPART ?= xc7z020clg400-1
   ```

## 🔧 支持的芯片型号

### Zynq-7000 系列
- `xc7z020clg400-1` - Zynq-7020（默认）
- `xc7z020clg484-1` - Zynq-7020（更大封装）
- `xc7z010clg400-1` - Zynq-7010
- `xc7z030fbg676-1` - Zynq-7030

### UltraScale+ 系列
- `xczu9eg-ffvb1156-2-e` - ZU9EG
- `xczu7ev-ffvc1156-2-e` - ZU7EV

### Alveo 数据中心加速卡
- `xcu200-fsgd2104-2-e` - U200
- `xcu250-figd2104-2-e` - U250

### 使用示例

```bash
# Zynq-7020
make hls_csim XPART=xc7z020clg400-1

# UltraScale+
make hls_synth XPART=xczu9eg-ffvb1156-2-e

# Alveo U200
make hls_cosim XPART=xcu200-fsgd2104-2-e
```

## 🐛 常见问题排查

### 问题 1: vitis_hls 命令未找到

**症状**:
```
bash: vitis_hls: command not found
```

**解决方案**:
```bash
# 确认环境变量设置
echo $XILINX_VIVADO
echo $XILINX_VITIS

# 手动 source settings
source /path/to/Xilinx/Vivado/2024.1/settings64.sh
source /path/to/Xilinx/Vitis/2024.1/settings64.sh

# 验证
which vitis_hls
vitis_hls -version
```

### 问题 2: 无法找到芯片型号

**症状**:
```
ERROR: Unknown device part 'xc7z020-clg400-1'
```

**原因**: 芯片型号格式错误

**解决方案**:
```bash
# 正确格式（无连字符）
make hls_csim XPART=xc7z020clg400-1

# 错误格式（有连字符）
# make hls_csim XPART=xc7z020-clg400-1  ❌
```

### 问题 3: LD_LIBRARY_PATH 未设置

**症状**:
```
error while loading shared libraries: libxxx.so: cannot open shared object file
```

**解决方案**:
```bash
# Makefile 会自动设置，但如果仍有问题：
export LD_LIBRARY_PATH=$XILINX_VITIS/lib/lnx64.o:$LD_LIBRARY_PATH
```

### 问题 4: LICENSE 文件未找到

**症状**:
```
ERROR: License file not found
```

**解决方案**:
```bash
# 设置 license 服务器
export LM_LICENSE_FILE=port@server

# 或指向 license 文件
export XILINXD_LICENSE_FILE=/path/to/Xilinx.lic
```

## 📖 参考文档

### 项目文档
- `Makefile` - 主构建文件（已更新）
- `tests/hw/run_hls.tcl` - HLS TCL 脚本（已更新）
- `check_xilinx_env.sh` - 环境检查脚本（新）
- `MAKEFILE_GUIDE.md` - Makefile 使用指南

### Xilinx 官方文档
- UG902: Vivado Design Suite User Guide - High-Level Synthesis
- UG1393: Vitis Unified Software Platform Documentation
- UG973: Vivado Design Suite Release Notes

## 🎯 快速参考

### 环境设置脚本

创建 `setup_env.sh`:

```bash
#!/bin/bash
# Xilinx 工具环境设置

# 根据实际路径修改
export XILINX_VIVADO=/home/fermata/Development/Software/Xilinx/Vivado/2024.1
export XILINX_VITIS=/home/fermata/Development/Software/Xilinx/Vitis/2024.1

# Source settings
source $XILINX_VIVADO/settings64.sh
source $XILINX_VITIS/settings64.sh

# 验证
echo "Vivado: $(vivado -version | head -1)"
echo "Vitis HLS: $(vitis_hls -version | head -1)"
```

使用：
```bash
source setup_env.sh
cd /path/to/hls_cnn
make show_config
make hls_csim
```

## ✅ 验证检查清单

- [ ] 运行 `./check_xilinx_env.sh` 全部通过
- [ ] `make show_config` 显示正确的路径
- [ ] `which vitis_hls` 返回有效路径
- [ ] `vitis_hls -version` 显示版本信息
- [ ] `make hls_csim` 成功运行

---

**更新日期**: 2025-10-02  
**状态**: ✅ 环境配置问题已修复  
**测试状态**: 待验证
