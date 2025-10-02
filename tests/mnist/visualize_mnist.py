#!/usr/bin/env python3
"""
Visualize MNIST images and predictions
"""

import os
import sys
import numpy as np
import matplotlib.pyplot as plt

def load_mnist_data(prefix="quick_test"):
    """Load MNIST images and labels"""
    data_dir = "data"
    
    images_file = f"{data_dir}/{prefix}_images.bin"
    labels_file = f"{data_dir}/{prefix}_labels.bin"
    
    if not os.path.exists(images_file):
        print(f"ERROR: {images_file} not found")
        return None, None
    
    # Load images
    images = np.fromfile(images_file, dtype=np.float32)
    num_images = len(images) // (28 * 28)
    images = images.reshape(num_images, 28, 28)
    
    # Load labels
    labels = np.fromfile(labels_file, dtype=np.uint8)
    
    return images, labels

def visualize_samples(images, labels, num_samples=10):
    """Visualize MNIST samples"""
    num_samples = min(num_samples, len(images))
    
    # Create figure
    fig, axes = plt.subplots(2, 5, figsize=(12, 6))
    fig.suptitle('MNIST Samples', fontsize=16)
    
    for i in range(num_samples):
        ax = axes[i // 5, i % 5]
        ax.imshow(images[i], cmap='gray')
        ax.set_title(f'Label: {labels[i]}')
        ax.axis('off')
    
    plt.tight_layout()
    plt.savefig('mnist_samples.png', dpi=150, bbox_inches='tight')
    print("Saved visualization to mnist_samples.png")
    plt.show()

def visualize_with_predictions(images, labels, predictions):
    """Visualize images with labels and predictions"""
    num_samples = min(10, len(images))
    
    fig, axes = plt.subplots(2, 5, figsize=(12, 6))
    fig.suptitle('MNIST Predictions', fontsize=16)
    
    for i in range(num_samples):
        ax = axes[i // 5, i % 5]
        ax.imshow(images[i], cmap='gray')
        
        is_correct = labels[i] == predictions[i]
        color = 'green' if is_correct else 'red'
        symbol = '✓' if is_correct else '✗'
        
        ax.set_title(f'True: {labels[i]}, Pred: {predictions[i]} {symbol}',
                    color=color, fontsize=10)
        ax.axis('off')
    
    plt.tight_layout()
    plt.savefig('mnist_predictions.png', dpi=150, bbox_inches='tight')
    print("Saved predictions to mnist_predictions.png")
    plt.show()

def display_statistics(labels):
    """Display label statistics"""
    print("\n=== Label Distribution ===")
    for i in range(10):
        count = np.sum(labels == i)
        percentage = count / len(labels) * 100
        bar = '█' * int(percentage / 2)
        print(f"Digit {i}: {count:3d} ({percentage:5.1f}%) {bar}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Visualize MNIST data')
    parser.add_argument('dataset', nargs='?', default='quick_test',
                       choices=['quick_test', 'validation', 'train', 'test'],
                       help='Dataset to visualize')
    parser.add_argument('--samples', type=int, default=10,
                       help='Number of samples to show')
    
    args = parser.parse_args()
    
    print("====================================")
    print("MNIST Visualization")
    print("====================================")
    print(f"Dataset: {args.dataset}")
    
    # Load data
    images, labels = load_mnist_data(args.dataset)
    
    if images is None:
        print("\nERROR: Failed to load data")
        print("Run 'make mnist_download' first")
        return 1
    
    print(f"\nLoaded {len(images)} images")
    print(f"Image shape: {images.shape}")
    print(f"Value range: [{images.min():.3f}, {images.max():.3f}]")
    
    # Display statistics
    display_statistics(labels)
    
    # Visualize samples
    print("\n=== Generating Visualization ===")
    visualize_samples(images, labels, args.samples)
    
    print("\n✓ Visualization complete!")
    return 0

if __name__ == "__main__":
    # Check if matplotlib is installed
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("ERROR: matplotlib not installed")
        print("Install with: pip3 install matplotlib")
        sys.exit(1)
    
    sys.exit(main())
