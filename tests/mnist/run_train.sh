#!/bin/bash
# 智能训练启动器 - 自动处理环境和路径

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================================================"
echo "MNIST CNN 训练启动器"
echo "======================================================================"

# 1. 检查并进入正确目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
echo "✅ 工作目录: $SCRIPT_DIR"

# 2. 检查数据是否存在
if [ ! -f "data/train_images.bin" ]; then
    echo -e "${YELLOW}⚠️  MNIST数据不存在，正在下载...${NC}"
    python3 download_mnist.py
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ 数据下载完成${NC}"
    else
        echo -e "${RED}❌ 数据下载失败${NC}"
        exit 1
    fi
else
    echo "✅ MNIST数据已存在"
fi

# 3. 检查conda环境
echo
echo "检查Python环境..."

CURRENT_ENV="${CONDA_DEFAULT_ENV:-none}"
echo "当前conda环境: $CURRENT_ENV"

if [ "$CURRENT_ENV" = "base" ] || [ "$CURRENT_ENV" = "none" ]; then
    echo -e "${YELLOW}⚠️  不在目标环境中，尝试激活 hls_cnn...${NC}"
    
    # 初始化conda
    if [ -f "$HOME/miniconda3/etc/profile.d/conda.sh" ]; then
        source "$HOME/miniconda3/etc/profile.d/conda.sh"
    elif [ -f "$HOME/anaconda3/etc/profile.d/conda.sh" ]; then
        source "$HOME/anaconda3/etc/profile.d/conda.sh"
    elif command -v conda &> /dev/null; then
        eval "$(conda shell.bash hook)"
    else
        echo -e "${RED}❌ 未找到conda，无法激活环境${NC}"
        echo "请手动运行："
        echo "  conda activate hls_cnn"
        echo "  cd $SCRIPT_DIR"
        echo "  python3 train_model.py $@"
        exit 1
    fi
    
    # 激活环境
    conda activate hls_cnn 2>/dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ 已激活 hls_cnn 环境${NC}"
    else
        echo -e "${RED}❌ 无法激活 hls_cnn 环境${NC}"
        echo "请确保环境存在: conda env list"
        exit 1
    fi
fi

# 4. 检查PyTorch
echo
python3 -c "import torch; print(f'✅ PyTorch版本: {torch.__version__}')" 2>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ PyTorch未安装${NC}"
    echo "请运行: conda install pytorch torchvision -c pytorch"
    exit 1
fi

# 5. 解析参数，提供默认值
MODE="${1:-quick}"  # 默认快速训练

echo
echo "======================================================================"
echo "开始训练"
echo "======================================================================"

case "$MODE" in
    "verify"|"test"|"v")
        echo "模式: 快速验证 (5 epochs)"
        python3 train_model.py --epochs 5 --batch-size 32 --lr 0.001
        ;;
    
    "quick"|"q")
        echo "模式: 快速训练 (20 epochs)"
        python3 train_model.py --epochs 20 --batch-size 32 --lr 0.0015
        ;;
    
    "full"|"f")
        echo "模式: 完整训练 (60 epochs)"
        python3 train_model.py --epochs 60 --batch-size 32 --lr 0.0015
        ;;
    
    "custom"|"c")
        echo "模式: 自定义参数"
        shift  # 移除第一个参数
        python3 train_model.py "$@"
        ;;
    
    *)
        echo "未知模式: $MODE"
        echo
        echo "用法:"
        echo "  $0 [mode] [args]"
        echo
        echo "模式:"
        echo "  verify, v     - 快速验证 (5 epochs, ~3分钟)"
        echo "  quick, q      - 快速训练 (20 epochs, ~15分钟) [默认]"
        echo "  full, f       - 完整训练 (60 epochs, ~40分钟)"
        echo "  custom, c     - 自定义参数，后接 train_model.py 参数"
        echo
        echo "示例:"
        echo "  $0 verify              # 验证修复"
        echo "  $0 quick               # 快速训练"
        echo "  $0 full                # 完整训练"
        echo "  $0 custom --epochs 30  # 自定义30个epoch"
        exit 1
        ;;
esac

TRAIN_EXIT=$?

echo
echo "======================================================================"
if [ $TRAIN_EXIT -eq 0 ]; then
    echo -e "${GREEN}✅ 训练完成${NC}"
    echo
    echo "权重文件:"
    if [ -d "weights" ]; then
        ls -lh weights/*.bin 2>/dev/null | awk '{print "  " $9, $5}'
        echo
        echo "下一步:"
        echo "  1. 运行HLS C仿真: cd ../../ && make hls_csim"
        echo "  2. 运行综合: make hls_synth"
    fi
else
    echo -e "${RED}❌ 训练失败 (退出代码: $TRAIN_EXIT)${NC}"
fi
echo "======================================================================"

exit $TRAIN_EXIT
