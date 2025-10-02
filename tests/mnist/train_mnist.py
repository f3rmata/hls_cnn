#!/usr/bin/env python3
"""
Train a simple CNN on MNIST and export weights for HLS testing
Uses PyTorch to train a model matching the HLS CNN architecture
"""

import os
import struct
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset

# Network architecture (matching HLS CNN)
# Conv1: 1x28x28 -> 16x26x26 (kernel=3)
# Pool1: 16x26x26 -> 16x13x13 (pool=2)
# Conv2: 16x13x13 -> 32x11x11 (kernel=3)
# Pool2: 32x11x11 -> 32x5x5 (pool=2)
# FC1: 800 -> 128
# FC2: 128 -> 10

class HLSCNN(nn.Module):
    def __init__(self):
        super(HLSCNN, self).__init__()
        
        # Conv1: 1 -> 16 channels, kernel=3
        self.conv1 = nn.Conv2d(1, 16, kernel_size=3, stride=1, padding=0)
        self.pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        # Conv2: 16 -> 32 channels, kernel=3
        self.conv2 = nn.Conv2d(16, 32, kernel_size=3, stride=1, padding=0)
        self.pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        # Fully connected layers
        self.fc1 = nn.Linear(32 * 5 * 5, 128)
        self.fc2 = nn.Linear(128, 10)
        
        self.relu = nn.ReLU()
        
    def forward(self, x):
        # Conv1 + ReLU + Pool1
        x = self.pool1(self.relu(self.conv1(x)))
        
        # Conv2 + ReLU + Pool2
        x = self.pool2(self.relu(self.conv2(x)))
        
        # Flatten
        x = x.view(-1, 32 * 5 * 5)
        
        # FC1 + ReLU
        x = self.relu(self.fc1(x))
        
        # FC2 (no activation, logits)
        x = self.fc2(x)
        
        return x

def load_mnist_data():
    """Load MNIST data from binary files"""
    data_dir = "data"
    
    # Load training data
    train_images = np.fromfile(f"{data_dir}/train_images.bin", dtype=np.float32)
    train_images = train_images.reshape(-1, 1, 28, 28)
    train_labels = np.fromfile(f"{data_dir}/train_labels.bin", dtype=np.uint8)
    
    # Load test data
    test_images = np.fromfile(f"{data_dir}/test_images.bin", dtype=np.float32)
    test_images = test_images.reshape(-1, 1, 28, 28)
    test_labels = np.fromfile(f"{data_dir}/test_labels.bin", dtype=np.uint8)
    
    return train_images, train_labels, test_images, test_labels

def export_weights(model, output_dir="weights"):
    """Export model weights to binary format for HLS"""
    os.makedirs(output_dir, exist_ok=True)
    
    state_dict = model.state_dict()
    
    # Conv1 weights: [16, 1, 3, 3]
    conv1_w = state_dict['conv1.weight'].cpu().numpy()
    conv1_b = state_dict['conv1.bias'].cpu().numpy()
    conv1_w.tofile(f"{output_dir}/conv1_weights.bin")
    conv1_b.tofile(f"{output_dir}/conv1_bias.bin")
    print(f"Exported conv1: weights {conv1_w.shape}, bias {conv1_b.shape}")
    
    # Conv2 weights: [32, 16, 3, 3]
    conv2_w = state_dict['conv2.weight'].cpu().numpy()
    conv2_b = state_dict['conv2.bias'].cpu().numpy()
    conv2_w.tofile(f"{output_dir}/conv2_weights.bin")
    conv2_b.tofile(f"{output_dir}/conv2_bias.bin")
    print(f"Exported conv2: weights {conv2_w.shape}, bias {conv2_b.shape}")
    
    # FC1 weights: [128, 800]
    fc1_w = state_dict['fc1.weight'].cpu().numpy()
    fc1_b = state_dict['fc1.bias'].cpu().numpy()
    fc1_w.tofile(f"{output_dir}/fc1_weights.bin")
    fc1_b.tofile(f"{output_dir}/fc1_bias.bin")
    print(f"Exported fc1: weights {fc1_w.shape}, bias {fc1_b.shape}")
    
    # FC2 weights: [10, 128]
    fc2_w = state_dict['fc2.weight'].cpu().numpy()
    fc2_b = state_dict['fc2.bias'].cpu().numpy()
    fc2_w.tofile(f"{output_dir}/fc2_weights.bin")
    fc2_b.tofile(f"{output_dir}/fc2_bias.bin")
    print(f"Exported fc2: weights {fc2_w.shape}, bias {fc2_b.shape}")
    
    # Export metadata
    with open(f"{output_dir}/weights_meta.txt", 'w') as f:
        f.write("HLS CNN Weights\n")
        f.write("=" * 50 + "\n")
        f.write(f"conv1_weights: {conv1_w.shape} -> conv1_weights.bin\n")
        f.write(f"conv1_bias: {conv1_b.shape} -> conv1_bias.bin\n")
        f.write(f"conv2_weights: {conv2_w.shape} -> conv2_weights.bin\n")
        f.write(f"conv2_bias: {conv2_b.shape} -> conv2_bias.bin\n")
        f.write(f"fc1_weights: {fc1_w.shape} -> fc1_weights.bin\n")
        f.write(f"fc1_bias: {fc1_b.shape} -> fc1_bias.bin\n")
        f.write(f"fc2_weights: {fc2_w.shape} -> fc2_weights.bin\n")
        f.write(f"fc2_bias: {fc2_b.shape} -> fc2_bias.bin\n")
    
    print(f"\nWeights exported to {output_dir}/")

def train_model(epochs=10, batch_size=64, learning_rate=0.001):
    """Train the CNN model"""
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Using device: {device}")
    
    # Load data
    print("\n=== Loading MNIST Data ===")
    train_images, train_labels, test_images, test_labels = load_mnist_data()
    
    # Convert to PyTorch tensors
    train_images = torch.from_numpy(train_images)
    train_labels = torch.from_numpy(train_labels).long()
    test_images = torch.from_numpy(test_images)
    test_labels = torch.from_numpy(test_labels).long()
    
    # Create data loaders
    train_dataset = TensorDataset(train_images, train_labels)
    test_dataset = TensorDataset(test_images, test_labels)
    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    test_loader = DataLoader(test_dataset, batch_size=batch_size, shuffle=False)
    
    print(f"Training samples: {len(train_dataset)}")
    print(f"Test samples: {len(test_dataset)}")
    
    # Create weights directory if it doesn't exist
    os.makedirs("weights", exist_ok=True)
    
    # Create model
    model = HLSCNN().to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=learning_rate)
    
    print("\n=== Model Architecture ===")
    print(model)
    
    # Count parameters
    total_params = sum(p.numel() for p in model.parameters())
    trainable_params = sum(p.numel() for p in model.parameters() if p.requires_grad)
    print(f"\nTotal parameters: {total_params:,}")
    print(f"Trainable parameters: {trainable_params:,}")
    
    # Training loop
    print("\n=== Training ===")
    best_acc = 0.0
    
    for epoch in range(epochs):
        model.train()
        train_loss = 0.0
        train_correct = 0
        train_total = 0
        
        for batch_idx, (images, labels) in enumerate(train_loader):
            images, labels = images.to(device), labels.to(device)
            
            # Forward pass
            optimizer.zero_grad()
            outputs = model(images)
            loss = criterion(outputs, labels)
            
            # Backward pass
            loss.backward()
            optimizer.step()
            
            # Statistics
            train_loss += loss.item()
            _, predicted = outputs.max(1)
            train_total += labels.size(0)
            train_correct += predicted.eq(labels).sum().item()
            
            if (batch_idx + 1) % 100 == 0:
                print(f"Epoch [{epoch+1}/{epochs}], "
                      f"Batch [{batch_idx+1}/{len(train_loader)}], "
                      f"Loss: {loss.item():.4f}, "
                      f"Acc: {100.*train_correct/train_total:.2f}%")
        
        # Validation
        model.eval()
        test_loss = 0.0
        test_correct = 0
        test_total = 0
        
        with torch.no_grad():
            for images, labels in test_loader:
                images, labels = images.to(device), labels.to(device)
                outputs = model(images)
                loss = criterion(outputs, labels)
                
                test_loss += loss.item()
                _, predicted = outputs.max(1)
                test_total += labels.size(0)
                test_correct += predicted.eq(labels).sum().item()
        
        test_acc = 100. * test_correct / test_total
        print(f"\nEpoch [{epoch+1}/{epochs}] Summary:")
        print(f"  Train Loss: {train_loss/len(train_loader):.4f}, "
              f"Train Acc: {100.*train_correct/train_total:.2f}%")
        print(f"  Test Loss: {test_loss/len(test_loader):.4f}, "
              f"Test Acc: {test_acc:.2f}%")
        
        # Save best model
        if test_acc > best_acc:
            best_acc = test_acc
            torch.save(model.state_dict(), "weights/best_model.pth")
            print(f"  *** Best model saved (accuracy: {best_acc:.2f}%) ***")
        
        print()
    
    print(f"\n=== Training Complete ===")
    print(f"Best test accuracy: {best_acc:.2f}%")
    
    # Export weights
    print("\n=== Exporting Weights ===")
    export_weights(model)
    
    return model

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Train MNIST CNN')
    parser.add_argument('--epochs', type=int, default=10,
                        help='Number of training epochs (default: 10)')
    parser.add_argument('--batch-size', type=int, default=64,
                        help='Batch size (default: 64)')
    parser.add_argument('--lr', type=float, default=0.001,
                        help='Learning rate (default: 0.001)')
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("   MNIST CNN Training")
    print("=" * 60)
    print(f"Epochs: {args.epochs}")
    print(f"Batch size: {args.batch_size}")
    print(f"Learning rate: {args.lr}")
    
    model = train_model(epochs=args.epochs, 
                       batch_size=args.batch_size,
                       learning_rate=args.lr)
    
    print("\n" + "=" * 60)
    print("   All Done!")
    print("=" * 60)

if __name__ == "__main__":
    main()
