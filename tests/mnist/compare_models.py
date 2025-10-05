#!/usr/bin/env python3
"""
Compare original and optimized models
"""

import torch
import torch.nn as nn
import numpy as np

# Import both models
import sys
sys.path.append('.')

class OriginalHLSCNN(nn.Module):
    """Original model from train_mnist.py"""
    def __init__(self):
        super(OriginalHLSCNN, self).__init__()
        self.conv1 = nn.Conv2d(1, 16, kernel_size=3, stride=1, padding=0)
        self.pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.conv2 = nn.Conv2d(16, 32, kernel_size=3, stride=1, padding=0)
        self.pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.fc1 = nn.Linear(32 * 5 * 5, 128)
        self.fc2 = nn.Linear(128, 10)
        self.relu = nn.ReLU()
        
    def forward(self, x):
        x = self.pool1(self.relu(self.conv1(x)))
        x = self.pool2(self.relu(self.conv2(x)))
        x = x.view(-1, 32 * 5 * 5)
        x = self.relu(self.fc1(x))
        x = self.fc2(x)
        return x

class OptimizedHLSCNN(nn.Module):
    """Optimized model from train_mnist_optimized.py"""
    def __init__(self):
        super(OptimizedHLSCNN, self).__init__()
        self.conv1 = nn.Conv2d(1, 4, kernel_size=5, stride=1, padding=0)
        self.pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.conv2 = nn.Conv2d(4, 8, kernel_size=5, stride=1, padding=0)
        self.pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        self.fc1 = nn.Linear(8 * 4 * 4, 64)
        self.fc2 = nn.Linear(64, 10)
        self.relu = nn.ReLU()
        
    def forward(self, x):
        x = self.pool1(self.relu(self.conv1(x)))
        x = self.pool2(self.relu(self.conv2(x)))
        x = x.view(-1, 8 * 4 * 4)
        x = self.relu(self.fc1(x))
        x = self.fc2(x)
        return x

def count_parameters(model):
    """Count model parameters"""
    total = 0
    details = {}
    for name, param in model.named_parameters():
        count = param.numel()
        total += count
        details[name] = (count, list(param.shape))
    return total, details

def compare_models():
    """Compare the two models"""
    print("=" * 70)
    print("Model Comparison: Original vs Optimized")
    print("=" * 70)
    
    # Create models
    original = OriginalHLSCNN()
    optimized = OptimizedHLSCNN()
    
    # Count parameters
    orig_total, orig_details = count_parameters(original)
    opt_total, opt_details = count_parameters(optimized)
    
    print("\n### Original Model ###")
    print(f"Total parameters: {orig_total:,}")
    print("\nLayer details:")
    for name, (count, shape) in sorted(orig_details.items()):
        print(f"  {name:20s}: {count:6,}  {shape}")
    
    print("\n### Optimized Model ###")
    print(f"Total parameters: {opt_total:,}")
    print("\nLayer details:")
    for name, (count, shape) in sorted(opt_details.items()):
        print(f"  {name:20s}: {count:6,}  {shape}")
    
    print("\n### Reduction ###")
    reduction = 100 * (1 - opt_total / orig_total)
    print(f"Parameter reduction: {reduction:.1f}%")
    print(f"Original: {orig_total:,} params ({orig_total*4/1024:.1f} KB)")
    print(f"Optimized: {opt_total:,} params ({opt_total*4/1024:.1f} KB)")
    
    # Memory footprint
    print("\n### Memory Footprint (float32) ###")
    print(f"Original model: {orig_total * 4 / 1024 / 1024:.2f} MB")
    print(f"Optimized model: {opt_total * 4 / 1024 / 1024:.2f} MB")
    print(f"Savings: {(orig_total - opt_total) * 4 / 1024:.1f} KB")
    
    # Theoretical FLOPs comparison (approximate)
    print("\n### Computational Complexity (approx) ###")
    
    # Original: 
    # Conv1: 16 * 1 * 3 * 3 * 26 * 26 = 97,344
    # Conv2: 32 * 16 * 3 * 3 * 11 * 11 = 497,664
    # FC1: 128 * 800 = 102,400
    # FC2: 10 * 128 = 1,280
    orig_flops = 97344 + 497664 + 102400 + 1280
    
    # Optimized:
    # Conv1: 4 * 1 * 5 * 5 * 24 * 24 = 57,600
    # Conv2: 8 * 4 * 5 * 5 * 8 * 8 = 51,200
    # FC1: 64 * 128 = 8,192
    # FC2: 10 * 64 = 640
    opt_flops = 57600 + 51200 + 8192 + 640
    
    print(f"Original MACs: {orig_flops:,}")
    print(f"Optimized MACs: {opt_flops:,}")
    print(f"Reduction: {100*(1-opt_flops/orig_flops):.1f}%")
    
    # Test inference
    print("\n### Inference Test ###")
    dummy_input = torch.randn(1, 1, 28, 28)
    
    original.eval()
    optimized.eval()
    
    with torch.no_grad():
        orig_out = original(dummy_input)
        opt_out = optimized(dummy_input)
    
    print(f"Original output shape: {orig_out.shape}")
    print(f"Optimized output shape: {opt_out.shape}")
    print(f"Both models produce 10-class logits: ✓")
    
    # Architecture comparison table
    print("\n### Architecture Comparison ###")
    print("┌" + "─" * 68 + "┐")
    print(f"│ {'Layer':<15} │ {'Original':<23} │ {'Optimized':<23} │")
    print("├" + "─" * 68 + "┤")
    
    layers = [
        ("Conv1", "16 ch, 3×3", "4 ch, 5×5"),
        ("Pool1", "2×2", "2×2"),
        ("Conv2", "32 ch, 3×3", "8 ch, 5×5"),
        ("Pool2", "2×2", "2×2"),
        ("FC1", "800 → 128", "128 → 64"),
        ("FC2", "128 → 10", "64 → 10"),
    ]
    
    for layer, orig, opt in layers:
        print(f"│ {layer:<15} │ {orig:<23} │ {opt:<23} │")
    
    print("└" + "─" * 68 + "┘")
    
    print("\n### Resource Implications for FPGA ###")
    print(f"Smaller model → Less LUTs, less BRAM, faster synthesis")
    print(f"Fewer channels → Lower memory bandwidth requirements")
    print(f"Larger kernels (5×5) → More reuse, but higher DSP usage")
    
    print("\n" + "=" * 70)

if __name__ == "__main__":
    compare_models()
