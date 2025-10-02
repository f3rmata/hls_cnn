#!/bin/bash
############################################################
# 环境配置修复验证脚本
# 验证所有修复是否正确应用
############################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "================================================"
echo "环境配置修复验证"
echo "================================================"
echo ""

# 步骤 1: 检查脚本文件
echo -e "${BLUE}[1/5]${NC} 检查新增脚本..."
echo ""

files=(
    "check_xilinx_env.sh"
    "setup_env.sh"
    "verify_env_fix.sh"
    "ENV_CONFIG_FIX.md"
)

for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $file 存在"
    else
        echo -e "${RED}✗${NC} $file 不存在"
    fi
done

echo ""

# 步骤 2: 检查 Makefile 更新
echo -e "${BLUE}[2/5]${NC} 检查 Makefile 更新..."
echo ""

checks=(
    "check_vivado:"
    "check_vitis:"
    "XPART ?="
    "HLS_PART :="
    "show_config:"
)

for check in "${checks[@]}"; do
    if grep -q "$check" Makefile; then
        echo -e "${GREEN}✓${NC} 找到: $check"
    else
        echo -e "${RED}✗${NC} 未找到: $check"
    fi
done

echo ""

# 步骤 3: 检查 run_hls.tcl 更新
echo -e "${BLUE}[3/5]${NC} 检查 run_hls.tcl 更新..."
echo ""

if grep -q "env(HLS_PART)" tests/hw/run_hls.tcl; then
    echo -e "${GREEN}✓${NC} run_hls.tcl 支持环境变量 HLS_PART"
else
    echo -e "${RED}✗${NC} run_hls.tcl 未更新"
fi

if grep -q "xc7z020clg400-1" tests/hw/run_hls.tcl; then
    echo -e "${GREEN}✓${NC} 默认芯片型号已设置"
else
    echo -e "${YELLOW}!${NC} 默认芯片型号可能未设置"
fi

echo ""

# 步骤 4: 测试 Makefile 命令（dry-run）
echo -e "${BLUE}[4/5]${NC} 测试 Makefile 命令（dry-run）..."
echo ""

commands=(
    "help"
    "show_config"
)

for cmd in "${commands[@]}"; do
    echo -n "测试: make $cmd ... "
    if make -n $cmd > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
    fi
done

echo ""

# 步骤 5: 检查环境变量
echo -e "${BLUE}[5/5]${NC} 检查 Xilinx 环境变量..."
echo ""

if [ -n "$XILINX_VIVADO" ]; then
    echo -e "${GREEN}✓${NC} XILINX_VIVADO = $XILINX_VIVADO"
else
    echo -e "${YELLOW}!${NC} XILINX_VIVADO 未设置"
    echo "    运行: source ./setup_env.sh"
fi

if [ -n "$XILINX_VITIS" ]; then
    echo -e "${GREEN}✓${NC} XILINX_VITIS = $XILINX_VITIS"
else
    echo -e "${YELLOW}!${NC} XILINX_VITIS 未设置"
fi

echo ""

# 总结
echo "================================================"
echo "验证总结"
echo "================================================"
echo ""

if [ -z "$XILINX_VIVADO" ]; then
    echo -e "${YELLOW}⚠️  环境变量未设置${NC}"
    echo ""
    echo "下一步操作:"
    echo "  1. 运行环境检查:"
    echo "     ./check_xilinx_env.sh"
    echo ""
    echo "  2. 如果检查失败，设置环境:"
    echo "     source ./setup_env.sh"
    echo ""
    echo "  3. 验证配置:"
    echo "     make show_config"
    echo ""
    echo "  4. 运行 HLS 测试:"
    echo "     make hls_csim XPART=xc7z020clg400-1"
    echo ""
else
    echo -e "${GREEN}✅ 文件更新完成，环境已配置${NC}"
    echo ""
    echo "可以直接运行:"
    echo "  make show_config    # 显示配置"
    echo "  make hls_csim       # C 仿真（使用默认芯片）"
    echo ""
    echo "指定芯片型号:"
    echo "  make hls_csim XPART=xc7z020clg400-1"
    echo "  make hls_synth XPART=xczu9eg-ffvb1156-2-e"
    echo ""
fi

echo "================================================"
echo "详细文档:"
echo "  ENV_CONFIG_FIX.md - 完整修复说明"
echo "  MAKEFILE_GUIDE.md - Makefile 使用指南"
echo "================================================"
echo ""
