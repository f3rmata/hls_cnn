/*
 * MNIST Test for HLS CNN
 * Tests CNN inference on MNIST handwritten digit dataset
 */

#include "../../src/hls_cnn.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace hls_cnn;
using namespace std;

// MNIST dataset constants
const int MNIST_IMG_SIZE = 28;
const int MNIST_IMG_PIXELS = MNIST_IMG_SIZE * MNIST_IMG_SIZE;
const int MNIST_NUM_CLASSES = 10;

// ========== Utility Functions ==========

/**
 * Load binary float32 data from file
 */
bool load_binary_float(const char *filename, float *data, int size) {
  ifstream file(filename, ios::binary);
  if (!file.is_open()) {
    cerr << "ERROR: Cannot open file " << filename << endl;
    return false;
  }

  file.read(reinterpret_cast<char *>(data), size * sizeof(float));
  if (!file.good()) {
    cerr << "ERROR: Failed to read " << size << " floats from " << filename
         << endl;
    return false;
  }

  file.close();
  return true;
}

/**
 * Load binary uint8 data from file
 */
bool load_binary_uint8(const char *filename, unsigned char *data, int size) {
  ifstream file(filename, ios::binary);
  if (!file.is_open()) {
    cerr << "ERROR: Cannot open file " << filename << endl;
    return false;
  }

  file.read(reinterpret_cast<char *>(data), size);
  if (!file.good()) {
    cerr << "ERROR: Failed to read " << size << " bytes from " << filename
         << endl;
    return false;
  }

  file.close();
  return true;
}

/**
 * Initialize random weights for testing
 */
void init_random_weights() {
  srand(42); // Fixed seed for reproducibility
}

float random_weight(float scale = 0.1f) {
  return ((float)rand() / RAND_MAX * 2.0f - 1.0f) * scale;
}

/**
 * Get predicted class from output logits
 */
int get_prediction(const data_t output[FC2_OUT_SIZE]) {
  int pred = 0;
  float max_val = output[0];
  for (int i = 1; i < FC2_OUT_SIZE; i++) {
    if (output[i] > max_val) {
      max_val = output[i];
      pred = i;
    }
  }
  return pred;
}

/**
 * Print confusion matrix
 */
void print_confusion_matrix(const vector<vector<int>> &confusion,
                            int num_classes) {
  cout << "\n=== Confusion Matrix ===" << endl;
  cout << "Actual \\ Pred  ";
  for (int i = 0; i < num_classes; i++) {
    printf("%4d ", i);
  }
  cout << endl;
  cout << string(15 + num_classes * 5, '-') << endl;

  for (int i = 0; i < num_classes; i++) {
    printf("    %d         ", i);
    for (int j = 0; j < num_classes; j++) {
      printf("%4d ", confusion[i][j]);
    }
    cout << endl;
  }
}

/**
 * Calculate accuracy metrics
 */
void calculate_metrics(const vector<vector<int>> &confusion, int num_classes) {
  int total = 0;
  int correct = 0;

  cout << "\n=== Per-Class Metrics ===" << endl;
  cout << "Class | Accuracy | Precision | Recall | F1-Score" << endl;
  cout << string(55, '-') << endl;

  for (int i = 0; i < num_classes; i++) {
    int tp = confusion[i][i];
    int fp = 0, fn = 0, tn = 0;

    for (int j = 0; j < num_classes; j++) {
      if (j != i) {
        fp += confusion[j][i]; // Predicted i but actually j
        fn += confusion[i][j]; // Predicted j but actually i
      }
    }

    int class_total = 0;
    for (int j = 0; j < num_classes; j++) {
      class_total += confusion[i][j];
    }

    float accuracy = (class_total > 0) ? (float)tp / class_total : 0.0f;
    float precision = (tp + fp > 0) ? (float)tp / (tp + fp) : 0.0f;
    float recall = (tp + fn > 0) ? (float)tp / (tp + fn) : 0.0f;
    float f1 = (precision + recall > 0)
                   ? 2 * precision * recall / (precision + recall)
                   : 0.0f;

    printf("  %d   |  %5.2f%%  |   %5.2f%%  | %5.2f%% |  %5.2f%%\n", i,
           accuracy * 100, precision * 100, recall * 100, f1 * 100);

    total += class_total;
    correct += tp;
  }

  float overall_accuracy = (total > 0) ? (float)correct / total : 0.0f;
  cout << string(55, '-') << endl;
  printf("Overall Accuracy: %.2f%% (%d/%d)\n", overall_accuracy * 100, correct,
         total);
}

// ========== Main Test Function ==========

int main(int argc, char **argv) {
  cout << "=====================================" << endl;
  cout << "   MNIST Test for HLS CNN" << endl;
  cout << "=====================================" << endl;

  // Parse arguments
  const char *images_file = "data/quick_test_images.bin";
  const char *labels_file = "data/quick_test_labels.bin";
  int num_images = 10;

  if (argc >= 2) {
    if (strcmp(argv[1], "validation") == 0) {
      images_file = "data/validation_images.bin";
      labels_file = "data/validation_labels.bin";
      num_images = 100;
    } else if (strcmp(argv[1], "test") == 0) {
      images_file = "data/test_images.bin";
      labels_file = "data/test_labels.bin";
      num_images = 10000;
    } else if (strcmp(argv[1], "quick") == 0) {
      // Use defaults
    } else {
      cout << "Usage: " << argv[0] << " [quick|validation|test]" << endl;
      cout << "  quick:      10 images (fast)" << endl;
      cout << "  validation: 100 images (medium)" << endl;
      cout << "  test:       10000 images (full test set, slow)" << endl;
      return 1;
    }
  }

  cout << "\nTest Configuration:" << endl;
  cout << "  Images file: " << images_file << endl;
  cout << "  Labels file: " << labels_file << endl;
  cout << "  Number of images: " << num_images << endl;

  // Allocate memory for images and labels
  vector<float> images(num_images * MNIST_IMG_PIXELS);
  vector<unsigned char> labels(num_images);

  // Load MNIST data
  cout << "\n=== Loading MNIST Data ===" << endl;
  if (!load_binary_float(images_file, images.data(), images.size())) {
    return 1;
  }
  cout << "Loaded " << num_images << " images" << endl;

  if (!load_binary_uint8(labels_file, labels.data(), labels.size())) {
    return 1;
  }
  cout << "Loaded " << num_images << " labels" << endl;

  // Initialize random weights (in real scenario, load pretrained weights)
  cout << "\n=== Initializing Network ===" << endl;
  cout << "WARNING: Using random weights (not trained)" << endl;
  cout << "For accurate results, load pretrained weights" << endl;

  init_random_weights();

  // Allocate network parameters
  static weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE]
                               [CONV1_KERNEL_SIZE];
  static weight_t conv1_bias[CONV1_OUT_CH];
  static weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE]
                               [CONV2_KERNEL_SIZE];
  static weight_t conv2_bias[CONV2_OUT_CH];
  static weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE];
  static weight_t fc1_bias[FC1_OUT_SIZE];
  static weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE];
  static weight_t fc2_bias[FC2_OUT_SIZE];

  // Initialize with random values (He initialization)
  for (int i = 0; i < CONV1_OUT_CH; i++) {
    conv1_bias[i] = 0.0f;
    for (int j = 0; j < CONV1_IN_CH; j++) {
      for (int k = 0; k < CONV1_KERNEL_SIZE; k++) {
        for (int l = 0; l < CONV1_KERNEL_SIZE; l++) {
          conv1_weights[i][j][k][l] = random_weight(sqrt(
              2.0f / (CONV1_IN_CH * CONV1_KERNEL_SIZE * CONV1_KERNEL_SIZE)));
        }
      }
    }
  }

  for (int i = 0; i < CONV2_OUT_CH; i++) {
    conv2_bias[i] = 0.0f;
    for (int j = 0; j < CONV2_IN_CH; j++) {
      for (int k = 0; k < CONV2_KERNEL_SIZE; k++) {
        for (int l = 0; l < CONV2_KERNEL_SIZE; l++) {
          conv2_weights[i][j][k][l] = random_weight(sqrt(
              2.0f / (CONV2_IN_CH * CONV2_KERNEL_SIZE * CONV2_KERNEL_SIZE)));
        }
      }
    }
  }

  for (int i = 0; i < FC1_OUT_SIZE; i++) {
    fc1_bias[i] = 0.0f;
    for (int j = 0; j < FC1_IN_SIZE; j++) {
      fc1_weights[i][j] = random_weight(sqrt(2.0f / FC1_IN_SIZE));
    }
  }

  for (int i = 0; i < FC2_OUT_SIZE; i++) {
    fc2_bias[i] = 0.0f;
    for (int j = 0; j < FC2_IN_SIZE; j++) {
      fc2_weights[i][j] = random_weight(sqrt(2.0f / FC2_IN_SIZE));
    }
  }

  cout << "Network initialized with random weights" << endl;

  // Run inference on all images
  cout << "\n=== Running Inference ===" << endl;

  vector<vector<int>> confusion(MNIST_NUM_CLASSES,
                                vector<int>(MNIST_NUM_CLASSES, 0));
  int correct = 0;

  for (int img_idx = 0; img_idx < num_images; img_idx++) {
    // Prepare input
    static data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE];
    for (int h = 0; h < MNIST_IMG_SIZE; h++) {
      for (int w = 0; w < MNIST_IMG_SIZE; w++) {
        input[0][h][w] =
            images[img_idx * MNIST_IMG_PIXELS + h * MNIST_IMG_SIZE + w];
      }
    }

    // Run inference
    static data_t output[FC2_OUT_SIZE];
    cnn_inference(input, conv1_weights, conv1_bias, conv2_weights, conv2_bias,
                  fc1_weights, fc1_bias, fc2_weights, fc2_bias, output);

    // Get prediction
    int pred = get_prediction(output);
    int actual = labels[img_idx];

    // Update confusion matrix
    confusion[actual][pred]++;

    if (pred == actual) {
      correct++;
    }

    // Print progress
    if ((img_idx + 1) % 10 == 0 || img_idx == num_images - 1) {
      printf("\rProcessed %d/%d images (%.1f%%)...", img_idx + 1, num_images,
             (img_idx + 1) * 100.0f / num_images);
      fflush(stdout);
    }

    // Print first few predictions
    if (img_idx < 10) {
      cout << "\n  Image " << img_idx << ": Predicted=" << pred
           << ", Actual=" << actual;
      if (pred == actual)
        cout << " ✓";
      else
        cout << " ✗";
    }
  }

  cout << "\n\n=== Test Results ===" << endl;
  cout << "Total images: " << num_images << endl;
  cout << "Correct predictions: " << correct << endl;
  cout << "Accuracy: " << (float)correct / num_images * 100.0f << "%" << endl;

  // Print confusion matrix and metrics
  if (num_images <= 1000) {
    print_confusion_matrix(confusion, MNIST_NUM_CLASSES);
    calculate_metrics(confusion, MNIST_NUM_CLASSES);
  }

  cout << "\n=====================================" << endl;
  cout << "   Test Complete" << endl;
  cout << "=====================================" << endl;

  return 0;
}
