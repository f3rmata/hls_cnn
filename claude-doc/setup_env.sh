#!/bin/bash
############################################################
# Xilinx 工具环境快速设置脚本
# 用于快速配置 Vivado 和 Vitis 环境
############################################################

echo "================================================"
echo "Xilinx 工具环境设置"
echo "================================================"

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# 检测 Xilinx 工具安装路径
XILINX_BASE="/home/fermata/Development/Software/Xilinx"

# 搜索可用的 Vivado 版本
echo ""
echo "正在搜索 Xilinx 工具..."

VIVADO_VERSIONS=(
    "$XILINX_BASE/Vivado/2024.1"
    "$XILINX_BASE/Vivado_HLS/2024.1"
)

VITIS_VERSIONS=(
    "$XILINX_BASE/Vitis/2024.1"
)

# 查找 Vivado
VIVADO_PATH=""
for path in "${VIVADO_VERSIONS[@]}"; do
    if [ -f "$path/settings64.sh" ]; then
        VIVADO_PATH="$path"
        break
    fi
done

# 查找 Vitis
VITIS_PATH=""
for path in "${VITIS_VERSIONS[@]}"; do
    if [ -f "$path/settings64.sh" ]; then
        VITIS_PATH="$path"
        break
    fi
done

# 显示找到的工具
if [ -n "$VIVADO_PATH" ]; then
    echo -e "${GREEN}✓${NC} 找到 Vivado: $VIVADO_PATH"
else
    echo -e "${RED}✗${NC} 未找到 Vivado"
fi

if [ -n "$VITIS_PATH" ]; then
    echo -e "${GREEN}✓${NC} 找到 Vitis: $VITIS_PATH"
else
    echo -e "${YELLOW}!${NC} 未找到 Vitis（可选）"
fi

echo ""

# 如果找到了工具，设置环境
if [ -n "$VIVADO_PATH" ]; then
    echo "================================================"
    echo "设置环境变量..."
    echo "================================================"
    
    export XILINX_VIVADO="$VIVADO_PATH"
    echo "export XILINX_VIVADO=$VIVADO_PATH"
    
    if [ -n "$VITIS_PATH" ]; then
        export XILINX_VITIS="$VITIS_PATH"
        echo "export XILINX_VITIS=$VITIS_PATH"
    fi
    
    echo ""
    echo "================================================"
    echo "加载 Xilinx 设置..."
    echo "================================================"
    
    # Source Vivado settings
    if [ -f "$VIVADO_PATH/settings64.sh" ]; then
        echo "加载: $VIVADO_PATH/settings64.sh"
        source "$VIVADO_PATH/settings64.sh"
    fi
    
    # Source Vitis settings
    if [ -n "$VITIS_PATH" ] && [ -f "$VITIS_PATH/settings64.sh" ]; then
        echo "加载: $VITIS_PATH/settings64.sh"
        source "$VITIS_PATH/settings64.sh"
    fi
    
    echo ""
    echo "================================================"
    echo "环境设置完成"
    echo "================================================"
    echo ""
    
    # 验证关键命令
    echo "验证工具可用性:"
    
    if command -v vitis_hls &> /dev/null; then
        echo -e "${GREEN}✓${NC} vitis_hls: $(which vitis_hls)"
        vitis_hls -version 2>&1 | head -1
    else
        echo -e "${RED}✗${NC} vitis_hls 未找到"
    fi
    
    if command -v vivado &> /dev/null; then
        echo -e "${GREEN}✓${NC} vivado: $(which vivado)"
    else
        echo -e "${RED}✗${NC} vivado 未找到"
    fi
    
    echo ""
    echo -e "${GREEN}✅ 环境设置成功！${NC}"
    echo ""
    echo "现在可以运行:"
    echo "  cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn"
    echo "  make show_config"
    echo "  make hls_csim"
    echo ""
    echo "注意: 此环境仅在当前终端会话有效。"
    echo "如需永久设置，请将以下内容添加到 ~/.zshrc:"
    echo ""
    echo "  source $(dirname "$0")/setup_env.sh"
    echo ""
    
else
    echo -e "${RED}❌ 未找到 Xilinx 工具！${NC}"
    echo ""
    echo "请检查 Xilinx 工具是否已安装在:"
    echo "  $XILINX_BASE"
    echo ""
    echo "或手动编辑此脚本，修改 XILINX_BASE 变量。"
    echo ""
    exit 1
fi
