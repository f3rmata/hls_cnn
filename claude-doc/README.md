# MNIST Test Suite for HLS CNN

This directory contains tools and tests for validating the HLS CNN implementation using the MNIST handwritten digit dataset.

## Directory Structure

```
mnist/
├── README.md                 # This file
├── download_mnist.py         # Download and prepare MNIST dataset
├── train_mnist.py           # Train CNN and export weights
├── mnist_test.cpp           # C++ test program for HLS CNN
├── data/                    # MNIST dataset (created by download_mnist.py)
│   ├── train_images.bin
│   ├── train_labels.bin
│   ├── test_images.bin
│   ├── test_labels.bin
│   ├── validation_images.bin
│   ├── validation_labels.bin
│   └── quick_test_images.bin
└── weights/                 # Trained model weights (created by train_mnist.py)
    ├── conv1_weights.bin
    ├── conv1_bias.bin
    ├── conv2_weights.bin
    ├── conv2_bias.bin
    ├── fc1_weights.bin
    ├── fc1_bias.bin
    ├── fc2_weights.bin
    └── fc2_bias.bin
```

## Quick Start

### 1. Download MNIST Dataset

```bash
cd tests/mnist
python3 download_mnist.py
```

This will:
- Download MNIST dataset from Yann LeCun's website
- Convert to binary format for C++ testing
- Create training, test, validation, and quick test subsets

### 2. (Optional) Train CNN Model

If you want to use trained weights instead of random weights:

```bash
# Install PyTorch if not already installed
pip3 install torch torchvision

# Train the model (may take 5-10 minutes on CPU, faster on GPU)
python3 train_mnist.py --epochs 10
```

This will:
- Train a CNN matching the HLS architecture
- Export weights to binary format
- Save the best model based on test accuracy

### 3. Run Tests

Compile and run the C++ test:

```bash
# Quick test (10 images)
make mnist_test_quick

# Validation test (100 images)
make mnist_test_validation

# Full test (10,000 images)
make mnist_test_full
```

Or compile and run manually:

```bash
# Compile
g++ -std=c++14 -I../../src -I/path/to/xilinx/hls/include \
    mnist_test.cpp -o mnist_test -DUSE_FLOAT

# Run with different datasets
./mnist_test quick        # 10 images (fast)
./mnist_test validation   # 100 images (medium)
./mnist_test test         # 10,000 images (slow)
```

## Dataset Information

### MNIST Dataset
- **Source**: http://yann.lecun.com/exdb/mnist/
- **Training images**: 60,000
- **Test images**: 10,000
- **Image size**: 28×28 grayscale
- **Classes**: 10 digits (0-9)

### Generated Subsets
- **quick_test**: 10 random images for quick validation
- **validation**: 100 random images for development testing
- **test**: Full 10,000 test images for final evaluation

## CNN Architecture

The HLS CNN implements a simplified LeNet-style architecture:

```
Input [1×28×28]
    ↓
Conv1 [16×26×26] (kernel=3×3, stride=1)
    ↓ ReLU
MaxPool1 [16×13×13] (pool=2×2, stride=2)
    ↓
Conv2 [32×11×11] (kernel=3×3, stride=1)
    ↓ ReLU
MaxPool2 [32×5×5] (pool=2×2, stride=2)
    ↓ Flatten
FC1 [128] (800→128)
    ↓ ReLU
FC2 [10] (128→10)
    ↓
Output [10] (class logits)
```

**Total parameters**: ~108,000

## Test Output

The test program provides:

1. **Basic Statistics**
   - Total images processed
   - Correct predictions
   - Overall accuracy

2. **Confusion Matrix**
   - Shows predicted vs actual classes
   - Helps identify which digits are confused

3. **Per-Class Metrics**
   - Accuracy per digit
   - Precision, Recall, F1-Score
   - Helps identify which digits are harder to recognize

## Expected Results

### With Random Weights
- Accuracy: ~10% (random guessing)
- Use this to verify the inference pipeline works

### With Trained Weights
- Expected accuracy: 95-98% on test set
- Should achieve >90% after just 2-3 epochs
- Best results after 10-20 epochs

## Troubleshooting

### "Cannot open file" errors
Make sure you've run `download_mnist.py` first to create the data files.

### Compilation errors
Ensure you have:
- Xilinx HLS tools installed
- Correct include paths in compilation command
- C++14 or later compiler

### Low accuracy with trained weights
- Verify weights were exported correctly
- Check that data normalization matches training
- Ensure network architecture matches exactly

## Integration with Makefile

Add these targets to the main Makefile:

```makefile
# MNIST test targets
.PHONY: mnist_download mnist_train mnist_test_quick mnist_test_validation mnist_test_full

mnist_download:
	cd tests/mnist && python3 download_mnist.py

mnist_train:
	cd tests/mnist && python3 train_mnist.py --epochs 10

mnist_test_quick: build/mnist_test
	cd tests/mnist && ../../build/mnist_test quick

mnist_test_validation: build/mnist_test
	cd tests/mnist && ../../build/mnist_test validation

mnist_test_full: build/mnist_test
	cd tests/mnist && ../../build/mnist_test test

build/mnist_test: tests/mnist/mnist_test.cpp $(SRC_DIR)/*.h
	$(CXX) $(CXXFLAGS) -DUSE_FLOAT tests/mnist/mnist_test.cpp -o $@
```

## References

- [MNIST Database](http://yann.lecun.com/exdb/mnist/)
- [LeNet Paper](http://yann.lecun.com/exdb/publis/pdf/lecun-01a.pdf)
- [Xilinx Vitis HLS Documentation](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)

## License

Copyright 2025 HLS CNN Project
Licensed under the Apache License, Version 2.0
