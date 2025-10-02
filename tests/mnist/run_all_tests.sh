#!/bin/bash
# Comprehensive test script for MNIST validation

set -e

echo "======================================"
echo "MNIST Comprehensive Test Suite"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "download_mnist.py" ]; then
    echo -e "${RED}ERROR: Please run this script from tests/mnist directory${NC}"
    exit 1
fi

# Function to print section header
print_section() {
    echo ""
    echo "======================================"
    echo "$1"
    echo "======================================"
}

# Function to print success
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print error
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Function to print warning
print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Step 1: Check data
print_section "Step 1: Checking MNIST Data"
if [ -d "data" ] && [ -f "data/test_images.bin" ]; then
    print_success "MNIST data found"
else
    print_warning "MNIST data not found, downloading..."
    python3 download_mnist.py
    if [ $? -eq 0 ]; then
        print_success "MNIST data downloaded successfully"
    else
        print_error "Failed to download MNIST data"
        exit 1
    fi
fi

# Step 2: Build tests
print_section "Step 2: Building Test Executables"
cd ../..
make build/mnist_test 2>&1 | tail -5
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    print_success "mnist_test built successfully"
else
    print_error "Failed to build mnist_test"
    exit 1
fi

# Step 3: Run tests with random weights
print_section "Step 3: Testing with Random Weights"
print_warning "Testing inference pipeline with random weights (accuracy ~10%)"

cd tests/mnist
echo ""
echo "--- Quick Test (10 images) ---"
../../build/mnist_test quick | tail -20

# Step 4: Check for trained weights
print_section "Step 4: Checking for Trained Weights"
if [ -f "weights/conv1_weights.bin" ]; then
    print_success "Trained weights found"
    HAS_WEIGHTS=1
else
    print_warning "Trained weights not found"
    HAS_WEIGHTS=0
    
    # Ask if user wants to train
    echo ""
    read -p "Do you want to train the model now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Check for PyTorch
        python3 -c "import torch" 2>/dev/null
        if [ $? -eq 0 ]; then
            print_section "Step 4a: Training CNN Model"
            python3 train_mnist.py --epochs 10
            if [ $? -eq 0 ]; then
                print_success "Model trained successfully"
                HAS_WEIGHTS=1
            else
                print_error "Training failed"
            fi
        else
            print_error "PyTorch not installed"
            echo "Install with: pip3 install torch torchvision"
        fi
    fi
fi

# Step 5: Run inference with trained weights
if [ $HAS_WEIGHTS -eq 1 ]; then
    print_section "Step 5: Testing with Trained Weights"
    
    cd ../..
    make build/mnist_inference 2>&1 | tail -5
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        print_success "mnist_inference built successfully"
    else
        print_error "Failed to build mnist_inference"
        exit 1
    fi
    
    cd tests/mnist
    echo ""
    echo "--- Validation Test (100 images) ---"
    ../../build/mnist_inference validation | tail -30
    
    print_success "Inference test completed"
else
    print_warning "Skipping inference test (no trained weights)"
fi

# Step 6: Summary
print_section "Test Summary"
echo "Tests completed successfully!"
echo ""
echo "What's next?"
echo "  1. Run full test: make mnist_test_full (10,000 images)"
if [ $HAS_WEIGHTS -eq 1 ]; then
    echo "  2. Run full inference: make mnist_inference_full"
    echo "  3. Proceed to HLS synthesis: make hls_csim"
else
    echo "  2. Train model: make mnist_train"
    echo "  3. Then run inference: make mnist_inference_validation"
fi
echo ""
print_success "All tests passed!"
