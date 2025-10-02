# 环境配置快速参考

## 🚀 快速开始（3步）

```bash
# 1. 检查环境
./check_xilinx_env.sh

# 2. 设置环境（如果需要）
source ./setup_env.sh

# 3. 运行 HLS
make hls_csim
```

## 📋 常用命令

### 查看配置
```bash
make show_config          # 显示当前配置
make help                 # 显示所有命令
```

### 运行 HLS（默认芯片）
```bash
make hls_csim            # C 仿真
make hls_synth           # 综合
make hls_cosim           # 协同仿真
```

### 指定芯片型号
```bash
make hls_csim XPART=xc7z020clg400-1        # Zynq-7020
make hls_synth XPART=xczu9eg-ffvb1156-2-e  # ZU9EG
make hls_cosim XPART=xcu200-fsgd2104-2-e   # U200
```

## ⚠️ 故障排查

### 问题：vitis_hls 未找到
```bash
source ./setup_env.sh
which vitis_hls
```

### 问题：环境变量未设置
```bash
echo $XILINX_VIVADO
echo $XILINX_VITIS
# 如果为空，运行 setup_env.sh
```

### 问题：芯片型号错误
```bash
# 正确格式（无连字符）
xc7z020clg400-1  ✅

# 错误格式（有连字符）
xc7z020-clg400-1  ❌
```

## 📖 文档索引

- `ENV_CONFIG_FIX.md` - 详细修复说明
- `ENV_FIX_SUMMARY.md` - 完整总结
- `MAKEFILE_GUIDE.md` - Makefile 指南

## 🎯 支持的芯片

| 系列 | 型号 | 命令 |
|------|------|------|
| Zynq-7000 | 7020 | `XPART=xc7z020clg400-1` |
| UltraScale+ | ZU9EG | `XPART=xczu9eg-ffvb1156-2-e` |
| Alveo | U200 | `XPART=xcu200-fsgd2104-2-e` |

---
**需要帮助？** 运行 `./check_xilinx_env.sh` 进行诊断
