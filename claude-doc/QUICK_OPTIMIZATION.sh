#!/bin/bash
# HLS_CNN 资源优化快速脚本
# 用途: 自动应用紧急优化，减少资源占用

set -e

PROJECT_ROOT="/home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn"
BACKUP_DIR="${PROJECT_ROOT}/backup_$(date +%Y%m%d_%H%M%S)"

cd "$PROJECT_ROOT"

echo "=========================================="
echo "HLS_CNN 资源优化脚本"
echo "=========================================="
echo ""

# 1. 创建备份
echo "[1/5] 创建代码备份..."
mkdir -p "$BACKUP_DIR"
cp -r src "$BACKUP_DIR/"
echo "✅ 备份已保存到: $BACKUP_DIR"
echo ""

# 2. 检查当前资源使用（如果存在综合报告）
echo "[2/5] 检查当前资源使用..."
SYNTH_REPORT="tests/hw/hls_cnn.prj/solution1/syn/report/cnn_inference_csynth.rpt"
if [ -f "$SYNTH_REPORT" ]; then
    echo "当前资源使用 (优化前):"
    grep -A 15 "Utilization Estimates" "$SYNTH_REPORT" | head -20
else
    echo "⚠️  未找到综合报告，跳过..."
fi
echo ""

# 3. 应用优化补丁
echo "[3/5] 应用优化补丁..."

# 3.1 优化 hls_cnn.cpp - 移除过度数组分割
echo "  - 优化 src/hls_cnn.cpp (数组分割)"
cat > src/hls_cnn.cpp << 'EOF'
/*
 * Copyright 2025 HLS CNN Project
 * OPTIMIZED VERSION - Reduced resource usage
 */

#include "hls_cnn.h"

void hls_cnn::cnn_inference(
    // Input
    data_t input[CONV1_IN_CH][CONV1_IMG_SIZE][CONV1_IMG_SIZE],

    // Conv1 parameters
    weight_t conv1_weights[CONV1_OUT_CH][CONV1_IN_CH][CONV1_KERNEL_SIZE][CONV1_KERNEL_SIZE],
    weight_t conv1_bias[CONV1_OUT_CH],

    // Conv2 parameters
    weight_t conv2_weights[CONV2_OUT_CH][CONV2_IN_CH][CONV2_KERNEL_SIZE][CONV2_KERNEL_SIZE],
    weight_t conv2_bias[CONV2_OUT_CH],

    // FC1 parameters
    weight_t fc1_weights[FC1_OUT_SIZE][FC1_IN_SIZE],
    weight_t fc1_bias[FC1_OUT_SIZE],

    // FC2 parameters
    weight_t fc2_weights[FC2_OUT_SIZE][FC2_IN_SIZE],
    weight_t fc2_bias[FC2_OUT_SIZE],

    // Output
    data_t output[FC2_OUT_SIZE]) {
    
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=m_axi depth=784 port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE mode=m_axi depth=432 port=conv1_weights offset=slave bundle=gmem1
#pragma HLS INTERFACE mode=m_axi depth=16 port=conv1_bias offset=slave bundle=gmem1
#pragma HLS INTERFACE mode=m_axi depth=4608 port=conv2_weights offset=slave bundle=gmem2
#pragma HLS INTERFACE mode=m_axi depth=32 port=conv2_bias offset=slave bundle=gmem2
#pragma HLS INTERFACE mode=m_axi depth=102400 port=fc1_weights offset=slave bundle=gmem3
#pragma HLS INTERFACE mode=m_axi depth=128 port=fc1_bias offset=slave bundle=gmem3
#pragma HLS INTERFACE mode=m_axi depth=1280 port=fc2_weights offset=slave bundle=gmem4
#pragma HLS INTERFACE mode=m_axi depth=10 port=fc2_bias offset=slave bundle=gmem4
#pragma HLS INTERFACE mode=m_axi depth=10 port=output offset=slave bundle=gmem5

  // ===== 优化点 1: 移除过度数组分割，使用 BRAM 存储 =====
  
  // Layer outputs - 使用 BRAM 而非寄存器
  static data_t conv1_out[CONV1_OUT_CH][POOL1_IMG_SIZE][POOL1_IMG_SIZE];
#pragma HLS BIND_STORAGE variable=conv1_out type=ram_2p impl=bram

  static data_t pool1_out[CONV1_OUT_CH][CONV2_IMG_SIZE][CONV2_IMG_SIZE];
#pragma HLS BIND_STORAGE variable=pool1_out type=ram_2p impl=bram

  static data_t conv2_out[CONV2_OUT_CH][POOL2_IMG_SIZE][POOL2_IMG_SIZE];
#pragma HLS BIND_STORAGE variable=conv2_out type=ram_2p impl=bram

  static data_t pool2_out[CONV2_OUT_CH][POOL2_IMG_SIZE / POOL2_SIZE][POOL2_IMG_SIZE / POOL2_SIZE];
#pragma HLS BIND_STORAGE variable=pool2_out type=ram_2p impl=bram

  // Flatten 输出可以使用寄存器（较小）
  static data_t flatten_out[FC1_IN_SIZE];
#pragma HLS ARRAY_PARTITION variable=flatten_out cyclic factor=4

  // FC1 输出可以完全分割（仅 128 个元素）
  static data_t fc1_out[FC1_OUT_SIZE];
#pragma HLS ARRAY_PARTITION variable=fc1_out complete

  // Layer 1: Conv + ReLU
  conv2d<CONV1_IN_CH, CONV1_OUT_CH, CONV1_IMG_SIZE, CONV1_IMG_SIZE, CONV1_KERNEL_SIZE>(
      input, conv1_weights, conv1_bias, conv1_out);

  // Layer 2: Max Pooling
  max_pool2d<CONV1_OUT_CH, POOL1_IMG_SIZE, POOL1_IMG_SIZE, POOL1_SIZE>(
      conv1_out, pool1_out);

  // Layer 3: Conv + ReLU
  conv2d<CONV2_IN_CH, CONV2_OUT_CH, CONV2_IMG_SIZE, CONV2_IMG_SIZE, CONV2_KERNEL_SIZE>(
      pool1_out, conv2_weights, conv2_bias, conv2_out);

  // Layer 4: Max Pooling
  max_pool2d<CONV2_OUT_CH, POOL2_IMG_SIZE, POOL2_IMG_SIZE, POOL2_SIZE>(
      conv2_out, pool2_out);

  // Layer 5: Flatten
  flatten<CONV2_OUT_CH, POOL2_IMG_SIZE / POOL2_SIZE, POOL2_IMG_SIZE / POOL2_SIZE>(
      pool2_out, flatten_out);

  // Layer 6: FC1 + ReLU
  fully_connected<FC1_IN_SIZE, FC1_OUT_SIZE>(
      flatten_out, fc1_weights, fc1_bias, fc1_out, true);

  // Layer 7: FC2 (no ReLU)
  fully_connected<FC2_IN_SIZE, FC2_OUT_SIZE>(
      fc1_out, fc2_weights, fc2_bias, output, false);
}
EOF

echo "  ✅ hls_cnn.cpp 已优化"

# 3.2 优化 hls_cnn.h - 调整循环结构
echo "  - 优化 src/hls_cnn.h (循环结构)"

# 这里我们创建一个优化后的头文件
# 注意：完整重构需要更多测试，这里仅做关键修改

cat > src/hls_cnn_optimized.h << 'EOF'
/*
 * Copyright 2025 HLS CNN Project
 * OPTIMIZED CONVOLUTION - Kernel-first loop order
 */

#ifndef HLS_CNN_OPTIMIZED_H
#define HLS_CNN_OPTIMIZED_H

#include "hls_cnn.h"

namespace hls_cnn {

// ===== 优化版本的 Conv2D: 卷积核循环外置 =====
template <int IN_CH, int OUT_CH, int IMG_H, int IMG_W, int KERNEL_SIZE>
void conv2d_optimized(
    data_t input[IN_CH][IMG_H][IMG_W],
    weight_t weights[OUT_CH][IN_CH][KERNEL_SIZE][KERNEL_SIZE],
    weight_t bias[OUT_CH],
    data_t output[OUT_CH][IMG_H - KERNEL_SIZE + 1][IMG_W - KERNEL_SIZE + 1]) {
    
#pragma HLS INLINE off

  const int OUT_H = IMG_H - KERNEL_SIZE + 1;
  const int OUT_W = IMG_W - KERNEL_SIZE + 1;

  // 初始化输出为 bias
  INIT_OUT_CH:
  for (int oc = 0; oc < OUT_CH; oc++) {
    INIT_OUT_H:
    for (int oh = 0; oh < OUT_H; oh++) {
      INIT_OUT_W:
      for (int ow = 0; ow < OUT_W; ow++) {
        #pragma HLS PIPELINE II=1
        output[oc][oh][ow] = bias[oc];
      }
    }
  }

  // 卷积核循环在外层
  CONV_KH:
  for (int kh = 0; kh < KERNEL_SIZE; kh++) {
    CONV_KW:
    for (int kw = 0; kw < KERNEL_SIZE; kw++) {
      
      CONV_IN_CH:
      for (int ic = 0; ic < IN_CH; ic++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=32
        
        CONV_OUT_CH:
        for (int oc = 0; oc < OUT_CH; oc++) {
          #pragma HLS LOOP_TRIPCOUNT min=16 max=32
          
          weight_t w = weights[oc][ic][kh][kw];
          
          CONV_OUT_H:
          for (int oh = 0; oh < OUT_H; oh++) {
            #pragma HLS LOOP_TRIPCOUNT min=24 max=28
            
            CONV_OUT_W:
            for (int ow = 0; ow < OUT_W; ow++) {
              #pragma HLS PIPELINE II=1
              
              int ih = oh + kh;
              int iw = ow + kw;
              output[oc][oh][ow] += input[ic][ih][iw] * w;
            }
          }
        }
      }
    }
  }

  // ReLU 激活
  RELU_OUT_CH:
  for (int oc = 0; oc < OUT_CH; oc++) {
    RELU_OUT_H:
    for (int oh = 0; oh < OUT_H; oh++) {
      RELU_OUT_W:
      for (int ow = 0; ow < OUT_W; ow++) {
        #pragma HLS PIPELINE II=1
        output[oc][oh][ow] = relu(output[oc][oh][ow]);
      }
    }
  }
}

} // namespace hls_cnn

#endif // HLS_CNN_OPTIMIZED_H
EOF

echo "  ✅ hls_cnn_optimized.h 已创建"
echo ""

# 4. 验证语法
echo "[4/5] 验证代码语法..."
if command -v g++ &> /dev/null; then
    g++ -c -std=c++11 -I. -fsyntax-only src/hls_cnn.cpp 2>&1 | head -20 || true
    echo "  ✅ 语法检查完成"
else
    echo "  ⚠️  未找到 g++，跳过语法检查"
fi
echo ""

# 5. 生成对比报告
echo "[5/5] 生成优化报告..."
cat > claude-doc/OPTIMIZATION_APPLIED.md << 'MDEOF'
# 资源优化已应用

**日期**: $(date)
**备份位置**: $BACKUP_DIR

## 应用的优化

### 1. 数组存储优化 ✅
- **修改文件**: `src/hls_cnn.cpp`
- **变更内容**:
  - 移除 `conv1_out`, `pool1_out`, `conv2_out`, `pool2_out` 的 cyclic partition
  - 添加 `BIND_STORAGE` 指令，强制使用 BRAM
  - 保留 `fc1_out` 的完全分割（仅 128 元素）

**预期效果**:
- BRAM Bank 数量: 16 → 4
- 路由资源: 减少 ~60%
- LUT 使用: 减少 ~40%

### 2. 优化版卷积函数 ✅
- **新增文件**: `src/hls_cnn_optimized.h`
- **变更内容**:
  - 卷积核循环外置
  - 分离初始化、卷积、激活三个阶段
  - 更清晰的累加逻辑

**使用方法**:
```cpp
#include "hls_cnn_optimized.h"
// 在 cnn_inference 中调用
conv2d_optimized<...>(...);  // 替代 conv2d
```

**预期效果**:
- DSP 使用: 300 → ~64
- LUT 使用: 再减少 ~30%
- 延迟: 增加 ~2x (可接受)

## 后续步骤

### 立即测试
```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn

# C 仿真验证功能
make unit_test

# HLS 综合查看资源
make hls_synth

# 查看优化后的资源报告
cat tests/hw/hls_cnn.prj/solution1/syn/report/cnn_inference_csynth.rpt | grep -A 20 "Utilization"
```

### 高级优化（可选）
1. **使用优化版卷积**: 修改 `cnn_inference` 调用 `conv2d_optimized`
2. **权重缓存**: 参考 `ARCHITECTURE_ANALYSIS.md` 第 3.3 节
3. **Dataflow 并行**: 在 `cnn_inference` 添加 `#pragma HLS DATAFLOW`

## 回滚方法

如果优化导致问题，可以恢复备份：
```bash
cp -r $BACKUP_DIR/src/* src/
```

## 参考文档

- [ARCHITECTURE_ANALYSIS.md](ARCHITECTURE_ANALYSIS.md) - 完整优化分析
- [DSP_FIX_SUMMARY.md](DSP_FIX_SUMMARY.md) - DSP 优化
- [QUICK_START.md](QUICK_START.md) - 快速开始

MDEOF

echo "  ✅ 优化报告已生成: claude-doc/OPTIMIZATION_APPLIED.md"
echo ""

echo "=========================================="
echo "✅ 优化完成！"
echo "=========================================="
echo ""
echo "📝 已应用的优化:"
echo "  1. 移除过度数组分割（BRAM 优化）"
echo "  2. 创建优化版卷积函数"
echo ""
echo "🔍 下一步:"
echo "  1. 运行单元测试: make unit_test"
echo "  2. HLS 综合: make hls_synth"
echo "  3. 查看资源报告"
echo ""
echo "📂 备份位置: $BACKUP_DIR"
echo "📄 详细分析: claude-doc/ARCHITECTURE_ANALYSIS.md"
echo ""
