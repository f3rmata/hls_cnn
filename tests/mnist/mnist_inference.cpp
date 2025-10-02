/*
 * MNIST Inference Test with Pretrained Weights
 * Loads trained weights from binary files and runs inference
 */

#include "../../src/hls_cnn.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace hls_cnn;
using namespace std;

// MNIST constants
const int MNIST_IMG_SIZE = 28;
const int MNIST_IMG_PIXELS = MNIST_IMG_SIZE * MNIST_IMG_SIZE;
const int MNIST_NUM_CLASSES = 10;

// Weight file paths
const char *WEIGHT_DIR = "weights";

// ========== Weight Loading Functions ==========

template <typename T>
bool load_binary_file(const char *filename, T *data, int size) {
  ifstream file(filename, ios::binary);
  if (!file.is_open()) {
    cerr << "ERROR: Cannot open file " << filename << endl;
    return false;
  }

  file.read(reinterpret_cast<char *>(data), size * sizeof(T));
  if (!file.good()) {
    cerr << "ERROR: Failed to read " << size << " elements from " << filename
         << endl;
    file.close();
    return false;
  }

  file.close();
  cout << "Loaded " << size << " elements from " << filename << endl;
  return true;
}

bool load_weights(weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH]
                                        [CONV1_KERNEL_SIZE][CONV1_KERNEL_SIZE],
                  weight_t conv1_bias[CONV1_OUT_CH],
                  weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH]
                                        [CONV2_KERNEL_SIZE][CONV2_KERNEL_SIZE],
                  weight_t conv2_bias[CONV2_OUT_CH],
                  weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE],
                  weight_t fc1_bias[FC1_OUT_SIZE],
                  weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE],
                  weight_t fc2_bias[FC2_OUT_SIZE]) {

  char filepath[256];
  bool success = true;

  // Load Conv1 weights [16, 1, 3, 3] = 144 elements
  sprintf(filepath, "%s/conv1_weights.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)conv1_weights,
                              CONV1_OUT_CH * CONV1_IN_CH * CONV1_KERNEL_SIZE *
                                  CONV1_KERNEL_SIZE);

  sprintf(filepath, "%s/conv1_bias.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)conv1_bias, CONV1_OUT_CH);

  // Load Conv2 weights [32, 16, 3, 3] = 4608 elements
  sprintf(filepath, "%s/conv2_weights.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)conv2_weights,
                              CONV2_OUT_CH * CONV2_IN_CH * CONV2_KERNEL_SIZE *
                                  CONV2_KERNEL_SIZE);

  sprintf(filepath, "%s/conv2_bias.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)conv2_bias, CONV2_OUT_CH);

  // Load FC1 weights [128, 800] = 102400 elements
  sprintf(filepath, "%s/fc1_weights.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)fc1_weights,
                              FC1_OUT_SIZE * FC1_IN_SIZE);

  sprintf(filepath, "%s/fc1_bias.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)fc1_bias, FC1_OUT_SIZE);

  // Load FC2 weights [10, 128] = 1280 elements
  sprintf(filepath, "%s/fc2_weights.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)fc2_weights,
                              FC2_OUT_SIZE * FC2_IN_SIZE);

  sprintf(filepath, "%s/fc2_bias.bin", WEIGHT_DIR);
  success &= load_binary_file(filepath, (float *)fc2_bias, FC2_OUT_SIZE);

  return success;
}

// ========== Data Loading Functions ==========

bool load_binary_float(const char *filename, float *data, int size) {
  return load_binary_file(filename, data, size);
}

bool load_binary_uint8(const char *filename, unsigned char *data, int size) {
  return load_binary_file(filename, data, size);
}

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

void calculate_metrics(const vector<vector<int>> &confusion, int num_classes) {
  int total = 0;
  int correct = 0;

  cout << "\n=== Per-Class Metrics ===" << endl;
  cout << "Class | Accuracy | Precision | Recall | F1-Score" << endl;
  cout << string(55, '-') << endl;

  for (int i = 0; i < num_classes; i++) {
    int tp = confusion[i][i];
    int fp = 0, fn = 0;

    for (int j = 0; j < num_classes; j++) {
      if (j != i) {
        fp += confusion[j][i];
        fn += confusion[i][j];
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

// ========== Main Function ==========

int main(int argc, char **argv) {
  cout << "=====================================" << endl;
  cout << "   MNIST Inference with Trained Model" << endl;
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
      return 1;
    }
  }

  cout << "\nTest Configuration:" << endl;
  cout << "  Images file: " << images_file << endl;
  cout << "  Labels file: " << labels_file << endl;
  cout << "  Number of images: " << num_images << endl;

  // Allocate memory
  vector<float> images(num_images * MNIST_IMG_PIXELS);
  vector<unsigned char> labels(num_images);

  // Load MNIST data
  cout << "\n=== Loading MNIST Data ===" << endl;
  if (!load_binary_float(images_file, images.data(), images.size())) {
    return 1;
  }

  if (!load_binary_uint8(labels_file, labels.data(), labels.size())) {
    return 1;
  }

  // Allocate network parameters
  cout << "\n=== Loading Trained Weights ===" << endl;

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

  if (!load_weights(conv1_weights, conv1_bias, conv2_weights, conv2_bias,
                    fc1_weights, fc1_bias, fc2_weights, fc2_bias)) {
    cerr << "\nERROR: Failed to load weights!" << endl;
    cerr << "Please run 'make mnist_train' first to train the model." << endl;
    return 1;
  }

  cout << "✓ All weights loaded successfully" << endl;

  // Run inference
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

    confusion[actual][pred]++;
    if (pred == actual)
      correct++;

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

  // Print results
  cout << "\n\n=== Test Results ===" << endl;
  cout << "Total images: " << num_images << endl;
  cout << "Correct predictions: " << correct << endl;
  printf("Accuracy: %.2f%%\n", (float)correct / num_images * 100.0f);

  if (num_images <= 1000) {
    print_confusion_matrix(confusion, MNIST_NUM_CLASSES);
    calculate_metrics(confusion, MNIST_NUM_CLASSES);
  }

  cout << "\n=====================================" << endl;
  cout << "   Test Complete" << endl;
  cout << "=====================================" << endl;

  return 0;
}
