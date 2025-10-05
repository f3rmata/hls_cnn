# 紧急LUT优化 - 快速参考

## 🚨 问题

```
ERROR: LUT 55,601 / 53,200 (104.5% 超限)
```

## ✅ 解决方案

### 架构调整: 6-12-84 → 6-10-80

| 参数 | 从 | 到 | 变化 |
|------|----|----|------|
| Conv2通道 | 12 | **10** | -17% |
| FC1大小 | 84 | **80** | -5% |
| FC1输入 | 192 | **160** | (自动) |
| 总参数 | 16K | **13.8K** | -14% |

### HLS优化

```cpp
// 1. Pipeline II: 4 → 8 (减少40% LUT)
#pragma HLS PIPELINE II = 8

// 2. 移除所有ARRAY_PARTITION (减少15% LUT)
// #pragma HLS ARRAY_PARTITION variable = bias complete  ← 全部注释
```

## 📊 预期结果

| 资源 | 预估 | 限制 | 利用率 |
|------|------|------|--------|
| LUT | 48,000 | 53,200 | **90%** ✅ |
| FF | 44,000 | 106,400 | 41% ✅ |
| DSP | 110 | 220 | 50% ✅ |
| BRAM | 68 | 280 | 24% ✅ |

**精度预期**: 94-96% (仍然优秀!)

## 🎯 已修改文件

- ✅ `src/cnn_marco.h` - 架构参数
- ✅ `src/hls_cnn.h` - Pipeline II + 移除ARRAY_PARTITION
- ✅ `tests/mnist/train_ultra_optimized.py` - 新训练脚本

## 🚀 下一步

### 1. 等待HLS综合完成 (~15分钟)

当前正在运行...

### 2. 查看结果

```bash
# 检查LUT使用
grep "LUT" hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt

# 预期看到: ~48,000 / 53,200
```

### 3. 如果通过，训练模型

```bash
cd ../../tests/mnist
python train_ultra_optimized.py
```

### 4. 如果仍超限

备选方案：
```cpp
// Option A: FC1再减小
#define FC1_OUT_SIZE 64  // 从80减到64
// 预期: LUT ~44K, 精度 93-94%

// Option B: Conv2再减小  
#define CONV2_OUT_CH 8  // 从10减到8
// 预期: LUT ~45K, 精度 92-94%

// Option C: Pipeline II再增加
#pragma HLS PIPELINE II = 16  // 从8增到16
// 预期: LUT ~42K, 推理时间20ms
```

## 📈 优化历程

```
LeNet-5原始: 60K参数, 99%精度, LUT超限400%
↓ 减少通道
16-32-128: 25K参数, 98%精度, LUT超限168%
↓ ap_fixed+II=2
6-16-84: 20K参数, 97%精度, LUT超限130%
↓ II=4+移除部分partition
6-12-84: 16K参数, 96-97%精度, LUT超限4.5% ← 上一版
↓ 减小通道+II=8+移除全部partition
6-10-80: 13.8K参数, 94-96%精度, LUT 90% ← 当前版本 ✅
```

## ⏱️ 时间线

- **21:05** - HLS综合开始
- **21:15** - 预计C综合完成
- **21:20** - 预计导出完成
- **21:30** - 开始训练
- **22:00** - 训练完成

---

**状态**: ⏳ 等待HLS综合结果...
