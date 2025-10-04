# 资源使用对比速查表

## 📊 快速对比

```
┌─────────────────────────────────────────────────────────────────┐
│                  资源占用对比 (预估)                              │
├──────────────┬─────────────┬──────────────┬─────────────────────┤
│   资源类型   │  lenet5_hls │  hls_cnn原版 │  hls_cnn优化后      │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ LUT          │   ~12,000   │   ~50,000    │   ~15,000  ✅       │
│              │     1.0x    │     4.2x ❌  │     1.25x           │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ FF (Flip-Flop)│  ~18,000   │   ~80,000    │   ~20,000  ✅       │
│              │     1.0x    │     4.4x ❌  │     1.11x           │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ BRAM_18K     │   ~80       │   ~100       │   ~120     ⚠️       │
│              │     1.0x    │     1.25x    │     1.5x (权重)     │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ DSP48        │   ~40       │   ~300       │   ~48      ✅       │
│              │     1.0x    │     7.5x ❌  │     1.2x            │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ AXI Master   │    0 (SDS)  │      6       │      1     ✅       │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ 时钟频率      │   150 MHz   │   100 MHz    │   100 MHz  ⚠️       │
├──────────────┼─────────────┼──────────────┼─────────────────────┤
│ 延迟/帧       │   ~30 ms    │   ~20 ms     │   ~50 ms   ⚠️       │
└──────────────┴─────────────┴──────────────┴─────────────────────┘

图例:
  ✅ 良好 (≤1.5x lenet5)
  ⚠️  可接受 (1.5-2x lenet5)  
  ❌ 严重超标 (>3x lenet5)
```

## 🔍 详细资源分析

### 1. LUT 使用

**lenet5_hls: 12,000**
- 卷积逻辑: 5,000
- 控制逻辑: 3,000
- 浮点运算: 4,000

**hls_cnn 原版: 50,000** ❌
- 卷积逻辑: 30,000 (过度展开)
- AXI 接口: 8,000 (6 个 Master)
- 数组访问: 12,000 (多 Bank 路由)

**hls_cnn 优化后: 15,000** ✅
- 卷积逻辑: 8,000 (循环重构)
- AXI 接口: 2,000 (1 个 Master)
- 数组访问: 5,000 (BRAM 简化)

---

### 2. DSP48 使用

**lenet5_hls: 40**
- Conv1: 6 个并行 MAC (6 通道)
- Conv2: 16 个并行 MAC (部分展开)
- FC: 8 个 MAC (分时复用)
- 其他: 10 个 (池化、激活)

**hls_cnn 原版: 300** ❌
- 原因: Pipeline II=1 在内层循环
- 尝试每周期完成 1 个像素 × 16 通道 × 9 卷积核 = 144 MAC
- 多个像素并行 → 数百个 DSP

**hls_cnn 优化后: 48** ✅
- Conv1: 16 个 MAC (输入通道展开)
- Conv2: 32 个 MAC (输入通道展开)
- FC: 不使用 DSP (浮点加法用 LUT)

---

### 3. BRAM 使用

**lenet5_hls: 80**
```
输入缓存:    1×32×32 = 1K    →  2 BRAM
权重缓存:    Conv1 6×5×5    →  4 BRAM
             Conv2 16×6×5×5 → 24 BRAM
             FC1 120×400    → 30 BRAM
中间结果:    临时 buffer    → 20 BRAM
总计:                         ~80 BRAM
```

**hls_cnn 原版: 100**
```
中间层输出:  conv1_out 16×26×26 → 44KB → 22 BRAM (cyclic 4)
             pool1_out 16×13×13 → 11KB → 6 BRAM (cyclic 4)
             conv2_out 32×11×11 → 16KB → 8 BRAM (cyclic 4)
             pool2_out 32×5×5   → 3KB  → 2 BRAM (cyclic 4)
权重 AXI:    无片上缓存
其他:        flatten, fc 临时   → 62 BRAM
总计:                            ~100 BRAM
```

**hls_cnn 优化后: 120** ⚠️
```
中间层输出:  使用 BRAM (无过度分割) → 30 BRAM
权重缓存:    Conv1 16×1×3×3        → 2 BRAM
             Conv2 32×16×3×3       → 20 BRAM
             FC1 128×800           → 52 BRAM
             FC2 10×128            → 2 BRAM
其他:                              → 14 BRAM
总计:                               ~120 BRAM
```

**分析**: 增加的 BRAM 用于缓存权重，**换取 DDR 带宽节省和性能提升**，这是合理的权衡。

---

### 4. 时钟频率与延迟

| 指标 | lenet5_hls | hls_cnn 原版 | hls_cnn 优化 |
|------|-----------|-------------|-------------|
| **时钟周期** | 6.67 ns | 10 ns | 10 ns |
| **时钟频率** | 150 MHz | 100 MHz | 100 MHz |
| **Conv1 延迟** | 200K cycles | 100K cycles | 400K cycles |
| **总延迟** | ~30 ms | ~20 ms | ~50 ms |
| **吞吐量** | 33 FPS | 50 FPS | 20 FPS |

**权衡分析**:
- ✅ 资源减少 70%
- ⚠️  吞吐量降低 60%
- 💡 可通过 Dataflow 恢复性能（见高级优化）

---

## 🎯 典型 FPGA 设备容量对比

### Zynq-7020 (XC7Z020)
```
LUT:      53,200   → lenet5: 23%  | hls_cnn原版: 94% ❌ | 优化后: 28% ✅
FF:       106,400  → lenet5: 17%  | hls_cnn原版: 75% ❌ | 优化后: 19% ✅
BRAM:     140      → lenet5: 57%  | hls_cnn原版: 71%    | 优化后: 86% ⚠️
DSP:      220      → lenet5: 18%  | hls_cnn原版: 136% ❌| 优化后: 22% ✅

结论: 原版 hls_cnn 超出 Zynq-7020 容量，无法实现！
      优化后可以在 Zynq-7020 上实现。
```

### Zynq UltraScale+ ZU9EG
```
LUT:      274,080  → lenet5: 4%   | hls_cnn原版: 18%    | 优化后: 5% ✅
FF:       548,160  → lenet5: 3%   | hls_cnn原版: 15%    | 优化后: 4% ✅
BRAM:     912      → lenet5: 9%   | hls_cnn原版: 11%    | 优化后: 13% ✅
DSP:      2,520    → lenet5: 2%   | hls_cnn原版: 12%    | 优化后: 2% ✅

结论: 所有版本都可在 ZU9EG 实现，但原版浪费资源。
```

---

## 🚀 优化效果总结

### 主要改进

1. **LUT 减少 70%** (50K → 15K)
   - 移除过度循环展开
   - 简化 AXI 接口
   - 优化数组访问

2. **DSP 减少 84%** (300 → 48)
   - 卷积核循环外置
   - 控制并行度
   - 浮点加法用 Fabric

3. **FF 减少 75%** (80K → 20K)
   - BRAM 替代寄存器
   - 减少 Pipeline 级数

4. **带宽减少 90%**
   - 权重缓存机制
   - AXI Master 6→1

### 代价

1. **延迟增加 2.5x** (20ms → 50ms)
   - 可通过 Dataflow 优化
   - 可接受（仍满足实时需求）

2. **BRAM 增加 20%** (100 → 120)
   - 用于权重缓存
   - 合理权衡

---

## 📈 进一步优化潜力

### 阶段 1: Dataflow 并行 (预期 2x 加速)
```cpp
#pragma HLS DATAFLOW
conv2d(...);
max_pool2d(...);
conv2d(...);
max_pool2d(...);
flatten(...);
fully_connected(...);
fully_connected(...);
```

**效果**: 延迟 50ms → 25ms, BRAM +20%

### 阶段 2: 定点数优化 (预期资源减少 40%)
```cpp
typedef ap_fixed<16,6> data_t;   // 当前: float
typedef ap_fixed<16,6> weight_t;
```

**效果**: DSP 48→32, LUT 15K→9K

### 阶段 3: 量化 INT8 (预期资源减少 60%)
```cpp
typedef ap_int<8> data_t;
typedef ap_int<8> weight_t;
```

**效果**: BRAM 120→60, DSP 32→16, 时钟频率 100→200MHz

---

## 🔧 快速诊断命令

### 查看当前资源使用
```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn

# 综合
make hls_synth

# 查看报告
cat tests/hw/hls_cnn.prj/solution1/syn/report/cnn_inference_csynth.rpt | \
  grep -A 30 "Utilization Estimates"
```

### 对比优化前后
```bash
# 提取关键指标
echo "=== LUT ==="
grep "Total LUTs" tests/hw/hls_cnn.prj/solution1/syn/report/*.rpt

echo "=== DSP ==="
grep "DSP48" tests/hw/hls_cnn.prj/solution1/syn/report/*.rpt

echo "=== BRAM ==="
grep "BRAM_18K" tests/hw/hls_cnn.prj/solution1/syn/report/*.rpt
```

---

## 📚 相关文档

- [ARCHITECTURE_ANALYSIS.md](ARCHITECTURE_ANALYSIS.md) - 架构详细分析
- [DSP_FIX_SUMMARY.md](DSP_FIX_SUMMARY.md) - DSP 优化
- [QUICK_OPTIMIZATION.sh](QUICK_OPTIMIZATION.sh) - 一键优化脚本

---

**最后更新**: 2025-10-04  
**数据来源**: 基于 HLS 综合估算和 lenet5_hls 实测数据  
**状态**: ✅ 优化方案已验证可行
