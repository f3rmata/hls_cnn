#!/bin/bash
# Quick training script for MNIST CNN
# Architecture: 6-8-64 (Conv1[6] -> Conv2[8] -> FC1[64] -> FC2[10])

set -e

echo "========================================"
echo "MNIST CNN Training for Zynq 7020"
echo "Architecture: 6-8-64"
echo "========================================"
echo ""

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found"
    exit 1
fi

# Check PyTorch
if ! python3 -c "import torch" 2>/dev/null; then
    echo "ERROR: PyTorch not installed"
    echo "Install with: pip3 install torch torchvision"
    exit 1
fi

# Check data
if [ ! -f data/train_images.bin ]; then
    echo "MNIST data not found. Downloading..."
    python3 download_mnist.py
fi

# Training options
EPOCHS=${1:-60}
BATCH_SIZE=${2:-32}

echo "Training parameters:"
echo "  Epochs: $EPOCHS"
echo "  Batch size: $BATCH_SIZE"
echo "  Expected time: ~$(($EPOCHS * 40 / 60)) minutes"
echo ""
echo "Starting training..."
echo "========================================"
echo ""

python3 train_model.py --epochs $EPOCHS --batch-size $BATCH_SIZE

echo ""
echo "========================================"
echo "Training complete!"
echo "========================================"
echo ""
echo "Next steps:"
echo "  1. Check weights in weights/ directory"
echo "  2. Run 'make mnist_inference_full' to test"
echo "  3. Run 'make hls_csim' for HLS simulation"
echo ""
