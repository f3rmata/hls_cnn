# Zynq 7020 优化快速参考

## 关键变更总结

### 网络参数 (cnn_marco.h)
```cpp
// Conv1: 16→6 通道, 3x3→5x5 卷积核
#define CONV1_OUT_CH 6
#define CONV1_KERNEL_SIZE 5

// Conv2: 32→16 通道, 3x3→5x5 卷积核  
#define CONV2_OUT_CH 16
#define CONV2_KERNEL_SIZE 5

// FC1: 800→256 输入, 128→84 输出
#define FC1_IN_SIZE 256
#define FC1_OUT_SIZE 84

// FC2: 128→84 输入
#define FC2_IN_SIZE 84
```

### 数组分区减半 (hls_cnn.cpp)
```cpp
// 所有中间层缓冲区分区因子从4→2或16→8
#pragma HLS ARRAY_PARTITION variable = conv1_out dim = 1 cyclic factor = 2
#pragma HLS ARRAY_PARTITION variable = flatten_out cyclic factor = 8
#pragma HLS ARRAY_PARTITION variable = fc1_out cyclic factor = 4
```

### HLS配置优化 (hls_config.tcl)
```tcl
config_schedule -effort medium              # high→medium
config_op fmul -impl meddsp -latency 3     # maxdsp→meddsp
config_op fadd -impl fabric -latency 4     # latency 3→4
config_op fsub -impl fabric -latency 4     # latency 3→4
config_array_partition -complete_threshold 256  # 1024→256
```

## 资源预期

| 资源 | 7020限制 | 优化前 | 预期优化后 | 状态 |
|------|---------|--------|-----------|------|
| BRAM | 280 | 400 (142%) | ~160 (57%) | ✅ OK |
| DSP | 220 | 219 (99%) | ~120 (55%) | ✅ OK |
| FF | 106K | 274K (257%) | ~80K (75%) | ✅ OK |
| LUT | 53K | 199K (373%) | ~40K (75%) | ✅ OK |

## 性能影响

- **延迟**: ~2-3ms (vs 4.77ms原始)
- **准确度**: 预计降低1-2% (可通过重训练优化)
- **吞吐量**: 类似或更好（更小的模型）

## 重新训练权重

需要使用新架构重新训练模型：
```bash
cd hls_cnn/tests/mnist
# 修改 train_mnist.py 以匹配新架构
python train_mnist.py
```

## 验证综合结果

```bash
cd hls_cnn/tests/hw
cat hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt | grep -A 10 "Utilization Estimates"
```

## 下一步

1. ✅ 修改网络架构
2. ⏳ 等待HLS综合完成
3. ⬜ 检查资源使用报告
4. ⬜ 如需要进一步优化：
   - 减少时钟频率 (10ns → 15ns)
   - 使用定点数代替浮点
   - 减少并行度
5. ⬜ 重新训练模型权重
6. ⬜ 验证功能和性能
