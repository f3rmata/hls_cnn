#!/bin/bash
# 快速验证准确率修复效果
# 训练5个epoch查看准确率是否正常

echo "======================================================================"
echo "快速验证 - 5 epochs测试"
echo "======================================================================"
echo "如果修复成功，5个epoch后准确率应该 > 70%"
echo "如果仍然 ~11%，说明还有其他问题"
echo "======================================================================"
echo

cd "$(dirname "$0")"

# 检查conda环境
if [ -z "$CONDA_DEFAULT_ENV" ] || [ "$CONDA_DEFAULT_ENV" = "base" ]; then
    echo "⚠️  未检测到conda环境或在base环境中"
    echo "正在尝试激活 hls_cnn 环境..."
    echo
    
    # 尝试激活conda
    if command -v conda &> /dev/null; then
        eval "$(conda shell.bash hook)"
        conda activate hls_cnn 2>/dev/null
        
        if [ $? -eq 0 ]; then
            echo "✅ 已激活 hls_cnn 环境"
        else
            echo "❌ 无法激活 hls_cnn 环境"
            echo "请手动运行: conda activate hls_cnn"
            exit 1
        fi
    else
        echo "❌ 未找到conda，请手动激活环境"
        exit 1
    fi
fi

echo "当前环境: $CONDA_DEFAULT_ENV"
echo

python3 train_model.py \
    --epochs 5 \
    --batch-size 32 \
    --lr 0.001 \
    2>&1 | tee quick_test.log

echo
echo "======================================================================"
echo "测试完成 - 检查结果"
echo "======================================================================"

# 提取最后的准确率
FINAL_ACC=$(tail -20 quick_test.log | grep "Epoch  5/5" | grep -oP "Test.*Acc:\s*\K[0-9.]+")

if [ -n "$FINAL_ACC" ]; then
    echo "5个epoch后测试准确率: $FINAL_ACC%"
    echo
    
    # 使用bc进行浮点数比较
    if command -v bc > /dev/null; then
        IS_GOOD=$(echo "$FINAL_ACC > 50" | bc -l)
        if [ "$IS_GOOD" -eq 1 ]; then
            echo "✅ 修复成功！准确率正常提升"
            echo "   建议运行完整训练: make mnist_train"
        else
            echo "⚠️  准确率仍然偏低（<50%）"
            echo "   可能需要进一步调试"
        fi
    else
        # 如果没有bc，简单判断
        if (( $(echo "$FINAL_ACC > 50" | awk '{print ($1 > 50)}') )); then
            echo "✅ 修复成功！准确率正常提升"
        else
            echo "⚠️  准确率仍然偏低"
        fi
    fi
else
    echo "⚠️  无法提取准确率，请手动检查 quick_test.log"
fi

echo "======================================================================"
