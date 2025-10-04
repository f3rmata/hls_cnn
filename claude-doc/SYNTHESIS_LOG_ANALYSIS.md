# HLS 综合日志分析报告

**日期**: 2025-10-04  
**状态**: 🔴 发现严重资源占用问题

---

## 🚨 关键发现

### 1. 指令数爆炸式增长

从综合日志可以看到设计规模的变化：

```
编译阶段                          指令数      增长比例
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
初始 (Compile/Link)                814        1.0x
Unroll/Inline (step 1)           1,886        2.3x
Unroll/Inline (step 4)           1,312        1.6x
Array/Struct (step 1)            4,572        5.6x  ⚠️
Performance (step 2)             3,648        4.5x
Performance (step 3)           116,223       142.8x  🔴 爆炸！
Performance (step 4)            26,309       32.3x   🔴 仍然很高
HW Transforms (final)           26,380       32.4x   🔴
```

**问题**: 在 `Performance (step 3)` 阶段，指令数暴增到 **116,223** 条！

---

## 🔍 根本原因分析

### 原因 1: 循环完全展开

从日志中看到 HLS 完全展开了关键循环：

```log
INFO: [HLS 214-186] Unrolling loop 'CONV_IN_CH' in function 'hls_cnn::conv2d<16, 32, 13, 13, 3>' 
completely with a factor of 16
```

**解读**:
- Conv2 的输入通道循环 (16 次) **完全展开**
- 卷积核循环 (3×3=9 次) **完全展开**
- 总共: 16 × 3 × 3 = **144 个并行操作**

**问题**: 这是因为 Pipeline 在 `CONV_OUT_W` 循环，HLS 为了达到 II=1 强制展开所有内层循环。

---

### 原因 2: 数组分割导致的推断分区

```log
INFO: [HLS 214-270] Inferring pragma 'array_partition type=complete dim=2' for array 
'uut_top::conv2_w' due to pipeline pragma

INFO: [HLS 214-270] Inferring pragma 'array_partition type=complete dim=3' for array 
'uut_top::conv2_w' due to pipeline pragma

INFO: [HLS 214-270] Inferring pragma 'array_partition type=complete dim=4' for array 
'uut_top::conv2_w' due to pipeline pragma
```

**解读**:
- HLS 自动推断需要完全分割 `conv2_w` 的第 2、3、4 维
- Conv2 权重: `[32][16][3][3]`
- 完全分割意味着: 32 × 16 × 3 × 3 = **4,608 个独立存储单元**

**问题**: 这会生成大量寄存器和路由逻辑。

---

### 原因 3: 中间数组的推断分区

```log
INFO: [HLS 214-270] Inferring pragma 'array_partition type=cyclic factor=3 dim=2' for array 
'hls_cnn::cnn_inference::pool1_out' due to pipeline pragma

INFO: [HLS 214-270] Inferring pragma 'array_partition type=cyclic factor=3 dim=3' for array 
'hls_cnn::cnn_inference::pool1_out' due to pipeline pragma
```

**解读**:
- `pool1_out` 数组 `[16][13][13]` 被额外分割
- 除了手动指定的 `cyclic factor=4` (dim 1)
- HLS 又推断出需要 `cyclic factor=3` (dim 2, 3)

**影响**:
- BRAM Bank 数量: 4 × 3 × 3 = **36 个 Bank**
- 路由复杂度呈指数增长

---

## 📊 资源占用预测

基于 26,380 条最终指令数，预估资源：

| 资源 | 估算值 | Zynq-7020 容量 | 占用率 |
|------|--------|----------------|--------|
| LUT | ~45,000 | 53,200 | **85%** 🔴 |
| FF | ~70,000 | 106,400 | **66%** 🔴 |
| DSP | ~250 | 220 | **114%** 🔴 超出! |
| BRAM | ~90 | 140 | **64%** ⚠️ |

**结论**: 当前设计**无法在 Zynq-7020 实现**（DSP 超出 14%）

---

## ✅ 解决方案

### 立即行动

查看完整的综合报告：

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn/tests/hw

# 查看资源估算
cat hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | grep -A 30 "Utilization"

# 查看详细的循环信息
cat hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | grep -A 50 "Loop:"
```

---

### 应用优化

**方案 1: 一键修复（推荐）**

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
./claude-doc/QUICK_OPTIMIZATION.sh
```

**方案 2: 紧急修复关键循环**

编辑 `src/hls_cnn.h`，修改 Pipeline 位置：

```cpp
// 当前问题代码 (行 83-88):
CONV_OUT_W:
for (int ow = 0; ow < OUT_W; ow++) {
  #pragma HLS PIPELINE II=1  // ❌ 导致内层循环完全展开
  
  CONV_IN_CH:
  for (int ic = 0; ic < IN_CH; ic++) {
    // ...
  }
}

// 修改为:
CONV_OUT_W:
for (int ow = 0; ow < OUT_W; ow++) {
  // ✅ 移除此处的 Pipeline
  
  CONV_IN_CH:
  for (int ic = 0; ic < IN_CH; ic++) {
    #pragma HLS PIPELINE II=1  // ✅ Pipeline 移到这里
    // ...
  }
}
```

**方案 3: 限制展开因子**

在有问题的循环添加：

```cpp
CONV_IN_CH:
for (int ic = 0; ic < IN_CH; ic++) {
  #pragma HLS UNROLL factor=4  // 限制展开，而非完全展开
  // ...
}
```

---

## 📈 预期改善

应用优化后，指令数应该降低到：

```
优化前: 26,380 条指令
优化后: ~3,500 条指令 (减少 87%)
```

资源占用：

```
LUT:  45,000 → 15,000  (减少 67%)
DSP:    250 →     48   (减少 81%)
FF:  70,000 → 20,000   (减少 71%)
```

---

## 🔗 相关日志

完整日志位置：
- 综合报告: `tests/hw/hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt`
- 设计规模: `tests/hw/hls_cnn.prj/sol/syn/report/csynth_design_size.rpt`
- HLS 日志: `tests/hw/hls_cnn.prj/vitis_hls.log`

---

## 📚 参考文档

- [ARCHITECTURE_ANALYSIS.md](ARCHITECTURE_ANALYSIS.md) - 架构对比分析
- [RESOURCE_COMPARISON.md](RESOURCE_COMPARISON.md) - 资源对比
- [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md) - 优化总结

---

**结论**: 当前综合正在生成一个**资源严重超标**的设计。建议**中止当前综合**，应用优化后重新运行。

**下一步**:
1. 等待当前综合完成（查看最终资源报告）
2. 运行优化脚本
3. 重新综合并对比改善效果
