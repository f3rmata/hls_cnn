#!/usr/bin/env python3
"""
MNIST Dataset Downloader and Preprocessor
Downloads MNIST dataset and converts it to binary format for HLS testing
"""

import os
import sys
import gzip
import struct
import numpy as np
from urllib.request import urlretrieve

# MNIST URLs
MNIST_URL = "https://raw.githubusercontent.com/sunsided/mnist/refs/heads/master/"
TRAIN_IMAGES = "train-images-idx3-ubyte.gz"
TRAIN_LABELS = "train-labels-idx1-ubyte.gz"
TEST_IMAGES = "t10k-images-idx3-ubyte.gz"
TEST_LABELS = "t10k-labels-idx1-ubyte.gz"

def download_file(url, filename):
    """Download file if it doesn't exist"""
    if not os.path.exists(filename):
        print(f"Downloading {filename}...")
        urlretrieve(url, filename)
        print(f"Downloaded {filename}")
    else:
        print(f"{filename} already exists")

def load_mnist_images(filename):
    """Load MNIST images from gz file"""
    with gzip.open(filename, 'rb') as f:
        # Read magic number and dimensions
        magic, num, rows, cols = struct.unpack(">IIII", f.read(16))
        if magic != 2051:
            raise ValueError(f"Invalid magic number {magic} in {filename}")
        
        # Read all images
        images = np.frombuffer(f.read(), dtype=np.uint8)
        images = images.reshape(num, rows, cols)
        
        # Normalize to [0, 1]
        images = images.astype(np.float32) / 255.0
        
        return images

def load_mnist_labels(filename):
    """Load MNIST labels from gz file"""
    with gzip.open(filename, 'rb') as f:
        # Read magic number and number of labels
        magic, num = struct.unpack(">II", f.read(8))
        if magic != 2049:
            raise ValueError(f"Invalid magic number {magic} in {filename}")
        
        # Read all labels
        labels = np.frombuffer(f.read(), dtype=np.uint8)
        
        return labels

def save_binary(images, labels, prefix):
    """Save images and labels in binary format"""
    # Save images as float32
    images_file = f"{prefix}_images.bin"
    images.astype(np.float32).tofile(images_file)
    print(f"Saved {images.shape[0]} images to {images_file}")
    
    # Save labels as uint8
    labels_file = f"{prefix}_labels.bin"
    labels.astype(np.uint8).tofile(labels_file)
    print(f"Saved {labels.shape[0]} labels to {labels_file}")
    
    # Save metadata
    meta_file = f"{prefix}_meta.txt"
    with open(meta_file, 'w') as f:
        f.write(f"num_images: {images.shape[0]}\n")
        f.write(f"image_height: {images.shape[1]}\n")
        f.write(f"image_width: {images.shape[2]}\n")
        f.write(f"num_classes: {len(np.unique(labels))}\n")
    print(f"Saved metadata to {meta_file}")

def create_test_subset(images, labels, num_samples=100):
    """Create a smaller test subset for quick validation"""
    indices = np.random.choice(len(images), num_samples, replace=False)
    return images[indices], labels[indices]

def main():
    # Create data directory
    data_dir = "data"
    os.makedirs(data_dir, exist_ok=True)
    os.chdir(data_dir)
    
    # Download MNIST files
    print("=" * 60)
    print("Downloading MNIST Dataset")
    print("=" * 60)
    
    download_file(MNIST_URL + TRAIN_IMAGES, TRAIN_IMAGES)
    download_file(MNIST_URL + TRAIN_LABELS, TRAIN_LABELS)
    download_file(MNIST_URL + TEST_IMAGES, TEST_IMAGES)
    download_file(MNIST_URL + TEST_LABELS, TEST_LABELS)
    
    # Load training data
    print("\n" + "=" * 60)
    print("Loading Training Data")
    print("=" * 60)
    train_images = load_mnist_images(TRAIN_IMAGES)
    train_labels = load_mnist_labels(TRAIN_LABELS)
    print(f"Training images shape: {train_images.shape}")
    print(f"Training labels shape: {train_labels.shape}")
    
    # Load test data
    print("\n" + "=" * 60)
    print("Loading Test Data")
    print("=" * 60)
    test_images = load_mnist_images(TEST_IMAGES)
    test_labels = load_mnist_labels(TEST_LABELS)
    print(f"Test images shape: {test_images.shape}")
    print(f"Test labels shape: {test_labels.shape}")
    
    # Save full datasets
    print("\n" + "=" * 60)
    print("Saving Binary Files")
    print("=" * 60)
    save_binary(train_images, train_labels, "train")
    save_binary(test_images, test_labels, "test")
    
    # Create small validation subset
    print("\n" + "=" * 60)
    print("Creating Validation Subset")
    print("=" * 60)
    val_images, val_labels = create_test_subset(test_images, test_labels, 100)
    save_binary(val_images, val_labels, "validation")
    
    # Create quick test subset
    print("\n" + "=" * 60)
    print("Creating Quick Test Subset")
    print("=" * 60)
    quick_images, quick_labels = create_test_subset(test_images, test_labels, 10)
    save_binary(quick_images, quick_labels, "quick_test")
    
    # Print statistics
    print("\n" + "=" * 60)
    print("Dataset Statistics")
    print("=" * 60)
    print(f"Training set: {len(train_images)} images")
    print(f"Test set: {len(test_images)} images")
    print(f"Validation set: {len(val_images)} images")
    print(f"Quick test set: {len(quick_images)} images")
    print(f"Image size: {train_images.shape[1]}x{train_images.shape[2]}")
    print(f"Number of classes: {len(np.unique(train_labels))}")
    
    # Show label distribution
    print("\nLabel distribution in test set:")
    for i in range(10):
        count = np.sum(test_labels == i)
        print(f"  Digit {i}: {count} images ({count/len(test_labels)*100:.1f}%)")
    
    print("\n" + "=" * 60)
    print("MNIST Dataset Preparation Complete!")
    print("=" * 60)

if __name__ == "__main__":
    main()
