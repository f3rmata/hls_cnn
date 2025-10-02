#!/bin/bash
############################################################
# Makefile 路径验证脚本
# 用于验证 Makefile 中的路径配置是否正确
############################################################

set -e

echo "================================================"
echo "Makefile 路径验证"
echo "================================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass_count=0
fail_count=0

# 验证函数
verify_path() {
    local desc="$1"
    local path="$2"
    local expected="$3"
    
    echo -n "检查 $desc ... "
    
    if [[ "$path" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ 通过${NC}"
        ((pass_count++))
        return 0
    else
        echo -e "${RED}✗ 失败${NC}"
        echo "  期望: $expected"
        echo "  实际: $path"
        ((fail_count++))
        return 1
    fi
}

verify_file_exists() {
    local desc="$1"
    local file="$2"
    
    echo -n "检查 $desc ... "
    
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓ 存在${NC}"
        ((pass_count++))
        return 0
    else
        echo -e "${RED}✗ 不存在${NC}"
        echo "  路径: $file"
        ((fail_count++))
        return 1
    fi
}

verify_dir_exists() {
    local desc="$1"
    local dir="$2"
    
    echo -n "检查 $desc ... "
    
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓ 存在${NC}"
        ((pass_count++))
        return 0
    else
        echo -e "${YELLOW}! 不存在（可能未生成）${NC}"
        return 1
    fi
}

echo "1️⃣  验证关键文件存在性"
echo "----------------------------------------"
verify_file_exists "Makefile" "Makefile"
verify_file_exists "run_hls.tcl" "tests/hw/run_hls.tcl"
verify_file_exists "hls_config.tcl" "tests/hw/hls_config.tcl"
verify_file_exists "uut_top.cpp" "tests/hw/uut_top.cpp"
verify_file_exists "test.cpp" "tests/hw/test.cpp"
verify_file_exists "hls_cnn.cpp" "src/hls_cnn.cpp"
verify_file_exists "hls_cnn.h" "src/hls_cnn.h"
echo ""

echo "2️⃣  验证 Makefile 变量定义"
echo "----------------------------------------"

# 提取 Makefile 变量
SRC_DIR=$(grep "^SRC_DIR :=" Makefile | cut -d'=' -f2 | tr -d ' ')
TEST_DIR=$(grep "^TEST_DIR :=" Makefile | cut -d'=' -f2 | tr -d ' ')
TEST_SW_DIR=$(grep "^TEST_SW_DIR :=" Makefile | cut -d'=' -f2 | tr -d ' ')
TEST_HW_DIR=$(grep "^TEST_HW_DIR :=" Makefile | cut -d'=' -f2 | tr -d ' ')
BUILD_DIR=$(grep "^BUILD_DIR :=" Makefile | cut -d'=' -f2 | tr -d ' ')

echo "SRC_DIR     = $SRC_DIR"
echo "TEST_DIR    = $TEST_DIR"
echo "TEST_SW_DIR = $TEST_SW_DIR"
echo "TEST_HW_DIR = $TEST_HW_DIR"
echo "BUILD_DIR   = $BUILD_DIR"
echo ""

echo "3️⃣  验证 HLS 命令工作目录"
echo "----------------------------------------"

# 检查 hls_csim 目标
csim_cmd=$(make -n hls_csim 2>/dev/null | grep "cd")
verify_path "hls_csim 工作目录" "$csim_cmd" "tests/hw"

# 检查 hls_synth 目标
synth_cmd=$(make -n hls_synth 2>/dev/null | grep "cd")
verify_path "hls_synth 工作目录" "$synth_cmd" "tests/hw"

# 检查 hls_cosim 目标
cosim_cmd=$(make -n hls_cosim 2>/dev/null | grep "cd")
verify_path "hls_cosim 工作目录" "$cosim_cmd" "tests/hw"

# 检查 hls_export 目标
export_cmd=$(make -n hls_export 2>/dev/null | grep "cd")
verify_path "hls_export 工作目录" "$export_cmd" "tests/hw"

echo ""

echo "4️⃣  验证清理目标"
echo "----------------------------------------"

# 检查 clean_hls 目标
clean_cmd=$(make -n clean_hls 2>/dev/null)
verify_path "清理 HLS 项目名称" "$clean_cmd" "hls_cnn.prj"
verify_path "清理日志路径" "$clean_cmd" "tests/hw/*.log"

echo ""

echo "5️⃣  验证目录结构"
echo "----------------------------------------"
verify_dir_exists "源代码目录" "src"
verify_dir_exists "测试目录" "tests"
verify_dir_exists "软件测试目录" "tests/sw"
verify_dir_exists "硬件测试目录" "tests/hw"
verify_dir_exists "构建目录" "build"
verify_dir_exists "HLS 项目目录" "tests/hw/hls_cnn.prj"

echo ""

echo "6️⃣  验证 run_hls.tcl 配置"
echo "----------------------------------------"

if [ -f "tests/hw/run_hls.tcl" ]; then
    # 检查项目名称
    proj_name=$(grep "^set PROJ" tests/hw/run_hls.tcl | cut -d'"' -f2)
    echo -n "HLS 项目名称 ... "
    if [ "$proj_name" == "hls_cnn.prj" ]; then
        echo -e "${GREEN}✓ 正确 ($proj_name)${NC}"
        ((pass_count++))
    else
        echo -e "${RED}✗ 错误 ($proj_name)${NC}"
        ((fail_count++))
    fi
    
    # 检查是否引用了 hls_config.tcl
    echo -n "hls_config.tcl 引用 ... "
    if grep -q "source.*hls_config.tcl" tests/hw/run_hls.tcl; then
        echo -e "${GREEN}✓ 已引用${NC}"
        ((pass_count++))
    else
        echo -e "${RED}✗ 未引用${NC}"
        ((fail_count++))
    fi
    
    # 检查路径设置
    echo -n "路径变量设置 ... "
    if grep -q "CUR_DIR.*pwd" tests/hw/run_hls.tcl && \
       grep -q "PROJ_ROOT.*CUR_DIR" tests/hw/run_hls.tcl && \
       grep -q "SRC_DIR.*PROJ_ROOT" tests/hw/run_hls.tcl; then
        echo -e "${GREEN}✓ 正确${NC}"
        ((pass_count++))
    else
        echo -e "${YELLOW}! 可能需要检查${NC}"
    fi
fi

echo ""

echo "================================================"
echo "验证结果汇总"
echo "================================================"
echo -e "${GREEN}通过: $pass_count${NC}"
echo -e "${RED}失败: $fail_count${NC}"
echo ""

if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}✅ 所有检查通过！Makefile 配置正确。${NC}"
    echo ""
    echo "可以运行以下命令进行测试："
    echo "  make unit_test           # 单元测试"
    echo "  make hls_csim            # HLS C 仿真"
    echo "  make clean_hls           # 清理 HLS 文件"
    echo ""
    exit 0
else
    echo -e "${RED}❌ 发现 $fail_count 个问题，请检查并修复。${NC}"
    echo ""
    echo "请参考 MAKEFILE_GUIDE.md 了解详细信息"
    echo ""
    exit 1
fi
