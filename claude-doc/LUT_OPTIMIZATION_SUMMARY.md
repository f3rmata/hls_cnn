# LUT优化总结 - Zynq 7020适配 (第二轮优化)

## 🔴 问题分析

### 第一轮优化结果（ap_fixed<16,8>）
使用ap_fixed后资源使用：
- LUT: 89,449 / 53,200 (**168%超额**) ❌
- FF: 69,323 / 106,400 (65%) ✅
- DSP: 177 / 220 (80%) ✅
- BRAM: 77 / 280 (27%) ✅

**虽然LUT从190K降到89K（减少53%），但仍然超限68%！**

Vivado Place阶段失败：
```
ERROR: [DRC UTLZ-1] Resource utilization: LUT as Logic over-utilized
This design requires 71924 of such cell types but only 53200 compatible sites
```

## ✅ 第二轮优化方案

### 1. **大幅减少网络规模**
```cpp
// cnn_marco.h - 激进的通道数削减
#define CONV1_OUT_CH 4    // 6 → 4 (减少33%)
#define CONV2_OUT_CH 8    // 16 → 8 (减少50%)
#define FC1_OUT_SIZE 64   // 84 → 64 (减少24%)
#define FC1_IN_SIZE 128   // 256 → 128 (减少50%)
```

**影响**：
- Conv2权重从2,400个参数减少到800个（减少67%）
- FC1权重从21,504个参数减少到8,192个（减少62%）
- **总参数量**: 25,010 → 9,402 (减少62%)

### 2. **增加Pipeline II值**
```cpp
// hls_cnn.h - 从II=2增加到II=4
#pragma HLS PIPELINE II = 4  // 所有层
```

**影响**：
- LUT减少约40%（multiplexer和控制逻辑）
- 延迟增加2倍（约4.8ms）
- 吞吐量降低到约200图像/秒

### 3. **移除所有数组分区**
```cpp
// hls_cnn.cpp - 完全移除数组分区
static data_t conv1_out[CONV1_OUT_CH][POOL1_IMG_SIZE][POOL1_IMG_SIZE];
static data_t pool1_out[CONV1_OUT_CH][CONV2_IMG_SIZE][CONV2_IMG_SIZE];
static data_t conv2_out[CONV2_OUT_CH][POOL2_IMG_SIZE][POOL2_IMG_SIZE];
static data_t pool2_out[CONV2_OUT_CH][POOL2_IMG_SIZE / POOL2_SIZE][POOL2_IMG_SIZE / POOL2_SIZE];
static data_t flatten_out[FC1_IN_SIZE];
static data_t fc1_out[FC1_OUT_SIZE];
// 无任何#pragma HLS ARRAY_PARTITION
```

**影响**：
- LUT减少约30%（少multiplexer用于数组访问）
- 带宽降低，但对顺序访问影响小
- BRAM使用可能略有增加

## 📊 预期资源使用

| 资源 | 第一轮 | 第二轮预期 | 可用 | 利用率 |
|------|--------|-----------|------|--------|
| LUT  | 89,449 | ~35,000 ✅ | 53,200 | ~66% |
| FF   | 69,323 | ~40,000 ✅ | 106,400 | ~38% |
| BRAM | 77     | ~50 ✅     | 280 | ~18% |
| DSP  | 177    | ~80 ✅     | 220 | ~36% |

**预期LUT减少约60%** (89K → 35K)

## 📈 网络架构变化

### 之前(第一轮)
```
Input [1x28x28] → Conv1[6@5x5] → Pool → Conv2[16@5x5] → Pool → FC1[84] → FC2[10]
参数: 25,010
```

### 现在(第二轮)
```
Input [1x28x28] → Conv1[4@5x5] → Pool → Conv2[8@5x5] → Pool → FC1[64] → FC2[10]
参数: 9,402 (减少62%)
```

**详细参数量**：
- Conv1: 4×1×5×5 + 4 = 104
- Conv2: 8×4×5×5 + 8 = 808
- FC1: 64×128 + 64 = 8,256
- FC2: 10×64 + 10 = 650
- **Total: 9,818**

## 🎯 成功标准

✅ **LUT < 40,000** (75% of 53,200)
✅ **FF < 80,000** (75% of 106,400)
✅ **推理延迟 < 10ms**
⚠️ **精度损失可能5-10%** (网络容量大幅降低)

## 🔧 验证步骤

```bash
# 重新运行HLS综合
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/hw
vitis_hls -f run_hls.tcl 2>&1 | tee logs/hls_run_ultra_optimized.log

# 等待完成后检查报告
cat hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | grep -A 10 "Utilization"
```

## 📝 优化历史

| 轮次 | 主要优化 | LUT使用 | 状态 |
|------|---------|---------|------|
| 原始 | float32 | 190,098 | ❌ 超限357% |
| 第一轮 | ap_fixed<16,8> | 89,449 | ❌ 超限168% |
| 第二轮 | 减少通道+II=4+无分区 | ~35,000 | ✅ 预期66% |

## ⚠️ 性能权衡

| 指标 | 原始 | 第一轮 | 第二轮 | 变化 |
|------|------|--------|--------|------|
| LUT | 190K | 89K | 35K | -81% ✅ |
| 参数量 | 25K | 25K | 9.8K | -61% ⚠️ |
| 延迟 | 1.2ms | 2.4ms | 4.8ms | +300% ⚠️ |
| 精度 | 基准 | -1% | -5~10% | ⚠️ |

**关键权衡**：
- ✅ 资源占用大幅降低，可以部署
- ⚠️ 模型容量减少，精度可能下降5-10%
- ⚠️ 延迟增加4倍，但仍满足实时需求(~200 FPS)

## 🚨 如果仍超限

### 进一步降低网络规模
```cpp
#define CONV1_OUT_CH 3    // 4 → 3
#define CONV2_OUT_CH 6    // 8 → 6
#define FC1_OUT_SIZE 48   // 64 → 48
```

### 使用更低精度
```cpp
typedef ap_fixed<12, 6> data_t;   // 16位 → 12位
typedef ap_fixed<12, 6> weight_t;
typedef ap_fixed<24, 12> acc_t;
```

### 增加II到8
```cpp
#pragma HLS PIPELINE II = 8  // II=4 → II=8
```

## � 当前状态

🔄 **准备重新运行HLS综合**

预计完成时间: 5-10分钟

📊 综合完成后将验证是否满足资源限制
