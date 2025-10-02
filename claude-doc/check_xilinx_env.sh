#!/bin/bash
############################################################
# Xilinx 工具环境检查脚本
# 用于验证 Vitis/Vivado 环境配置
############################################################

echo "================================================"
echo "Xilinx 工具环境检查"
echo "================================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

check_count=0
pass_count=0
fail_count=0

# 检查函数
check_env() {
    local var_name="$1"
    local desc="$2"
    
    ((check_count++))
    echo -n "[$check_count] 检查 $desc ... "
    
    if [ -n "${!var_name}" ]; then
        echo -e "${GREEN}✓${NC}"
        echo "    $var_name = ${!var_name}"
        ((pass_count++))
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "    $var_name 未设置"
        ((fail_count++))
        return 1
    fi
}

check_file() {
    local file_path="$1"
    local desc="$2"
    
    ((check_count++))
    echo -n "[$check_count] 检查 $desc ... "
    
    if [ -f "$file_path" ]; then
        echo -e "${GREEN}✓${NC}"
        echo "    路径: $file_path"
        ((pass_count++))
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "    文件不存在: $file_path"
        ((fail_count++))
        return 1
    fi
}

check_command() {
    local cmd="$1"
    local desc="$2"
    
    ((check_count++))
    echo -n "[$check_count] 检查 $desc ... "
    
    if command -v "$cmd" &> /dev/null; then
        local version=$(eval "$cmd --version 2>&1 | head -1" || echo "版本信息不可用")
        echo -e "${GREEN}✓${NC}"
        echo "    命令: $cmd"
        echo "    版本: $version"
        ((pass_count++))
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "    命令未找到: $cmd"
        ((fail_count++))
        return 1
    fi
}

echo "======================================"
echo "1. 环境变量检查"
echo "======================================"
check_env "XILINX_VIVADO" "Vivado 安装路径"
check_env "XILINX_VITIS" "Vitis 安装路径"
check_env "XILINX_HLS" "Vitis HLS 安装路径"

echo ""
echo "======================================"
echo "2. 关键可执行文件检查"
echo "======================================"

if [ -n "$XILINX_VIVADO" ]; then
    check_file "$XILINX_VIVADO/bin/vivado" "Vivado 可执行文件"
fi

if [ -n "$XILINX_VITIS" ]; then
    check_file "$XILINX_VITIS/bin/vitis" "Vitis 可执行文件"
    check_file "$XILINX_VITIS/bin/v++" "v++ 编译器"
fi

echo ""
echo "======================================"
echo "3. 命令行工具检查"
echo "======================================"
check_command "vitis_hls" "Vitis HLS"
check_command "vivado" "Vivado"

echo ""
echo "======================================"
echo "4. 库路径检查"
echo "======================================"

if [ -n "$LD_LIBRARY_PATH" ]; then
    echo -e "${GREEN}✓${NC} LD_LIBRARY_PATH 已设置"
    echo "    包含以下路径:"
    echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -i xilinx | sed 's/^/    /'
    ((check_count++))
    ((pass_count++))
else
    echo -e "${YELLOW}!${NC} LD_LIBRARY_PATH 未设置（可能影响运行）"
    ((check_count++))
fi

echo ""
echo "======================================"
echo "检查结果汇总"
echo "======================================"
echo -e "总计: $check_count 项"
echo -e "${GREEN}通过: $pass_count${NC}"
echo -e "${RED}失败: $fail_count${NC}"
echo ""

if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}✅ 所有检查通过！环境配置正确。${NC}"
    echo ""
    echo "现在可以运行以下命令："
    echo "  cd /path/to/hls_cnn"
    echo "  make show_config    # 显示项目配置"
    echo "  make hls_csim       # 运行 HLS C 仿真"
    echo ""
    exit 0
else
    echo -e "${RED}❌ 发现 $fail_count 个问题。${NC}"
    echo ""
    echo "请按照以下步骤修复："
    echo ""
    echo "1. 设置 Xilinx 工具环境变量："
    echo "   source /path/to/Xilinx/Vivado/2024.1/settings64.sh"
    echo "   source /path/to/Xilinx/Vitis/2024.1/settings64.sh"
    echo ""
    echo "2. 或者添加到 ~/.bashrc 或 ~/.zshrc："
    echo "   export XILINX_VIVADO=/path/to/Xilinx/Vivado/2024.1"
    echo "   export XILINX_VITIS=/path/to/Xilinx/Vitis/2024.1"
    echo "   source \$XILINX_VIVADO/settings64.sh"
    echo "   source \$XILINX_VITIS/settings64.sh"
    echo ""
    echo "3. 验证设置："
    echo "   ./check_xilinx_env.sh"
    echo ""
    exit 1
fi
