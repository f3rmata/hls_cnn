# CNN模型优化 - 适配Zynq 7020

## 优化目标
将CNN模型从严重超出资源限制优化到可在Zynq 7020上部署。

## Zynq 7020 资源限制
- **BRAM:** 280 (18K)
- **DSP:** 220
- **FF:** 106,400
- **LUT:** 53,200

## 优化前资源使用（超出限制）
- **BRAM:** 400/280 (**142%超额**)
- **DSP:** 219/220 (99%)
- **FF:** 273,763/106,400 (**257%超额**)
- **LUT:** 198,657/53,200 (**373%超额**)

## 优化策略

### 1. 网络架构调整（参照LeNet-5）

#### 卷积层通道数减少
**Conv1层：**
- 输出通道：16 → **6** (减少62.5%)
- 卷积核大小：3x3 → **5x5** (增加感受野补偿)
- 输出尺寸：26x26 → **24x24**

**Conv2层：**
- 输出通道：32 → **16** (减少50%)
- 输入通道：16 → **6**
- 卷积核大小：3x3 → **5x5**
- 输出尺寸：11x11 → **8x8**

#### 全连接层参数减少
**FC1层：**
- 输入大小：800 (32×5×5) → **256** (16×4×4) (减少68%)
- 输出大小：128 → **84** (减少34.4%)
- 参数量：102,400 → **21,504** (减少79%)

**FC2层：**
- 输入大小：128 → **84** (减少34.4%)
- 参数量：1,280 → **840** (减少34.4%)

### 2. HLS优化指令调整

#### 数组分区优化
```cpp
// 减少分区因子以节省资源
// 之前：
#pragma HLS ARRAY_PARTITION variable = conv1_out dim = 1 cyclic factor = 4
#pragma HLS ARRAY_PARTITION variable = flatten_out cyclic factor = 16
#pragma HLS ARRAY_PARTITION variable = fc1_out cyclic factor = 8

// 优化后：
#pragma HLS ARRAY_PARTITION variable = conv1_out dim = 1 cyclic factor = 2  // 减半
#pragma HLS ARRAY_PARTITION variable = flatten_out cyclic factor = 8        // 减半
#pragma HLS ARRAY_PARTITION variable = fc1_out cyclic factor = 4            // 减半
```

#### 卷积层权重分区
```cpp
// 之前：
#pragma HLS ARRAY_PARTITION variable = weights dim = 1 cyclic factor = 2

// 优化后：移除权重分区以节省资源
```

### 3. HLS配置优化 (hls_config.tcl)

#### 调度策略调整
```tcl
# 之前：
config_schedule -effort high

# 优化后：
config_schedule -effort medium  # 减少资源使用
```

#### 浮点运算配置
```tcl
# 之前：
config_op fmul -impl maxdsp -latency 2    # 最大化DSP使用
config_op fadd -impl fabric -latency 3
config_op fsub -impl fabric -latency 3

# 优化后：
config_op fmul -impl meddsp -latency 3    # 中等DSP使用，减少DSP需求
config_op fadd -impl fabric -latency 4    # 增加延迟，降低时钟要求
config_op fsub -impl fabric -latency 4
config_op fdiv -impl fabric -latency 12   # 增加延迟
config_op fsqrt -impl fabric -latency 12
```

#### 数组分区阈值
```tcl
# 之前：
config_array_partition -complete_threshold 1024

# 优化后：
config_array_partition -complete_threshold 256  # 减少自动完全分区
```

## 预期优化效果

### 资源使用估算

#### BRAM减少
- Conv层输出缓冲减少：~60%
- 权重存储减少：~75%
- **预期BRAM使用：~160** (在限制内)

#### DSP使用
- 乘法器使用meddsp代替maxdsp
- 卷积通道减少导致并行度降低
- **预期DSP使用：~120-150** (在限制内)

#### FF/LUT减少
- 数组分区因子减半：~50%资源减少
- 网络规模减少：~60%资源减少
- **预期FF使用：~80,000** (在限制内)
- **预期LUT使用：~40,000** (在限制内)

### 性能影响

#### 延迟增加
- 卷积核从3x3增加到5x5：计算量增加~2.78倍
- 但通道数减少可部分补偿
- **预期总延迟：~2-3ms** (原来~4.77ms)

#### 准确度影响
- LeNet-5架构经典且有效
- 较小的通道数可能降低1-2%准确度
- **可通过训练优化权重来补偿**

## 网络架构对比

### 优化前
```
Input[1×28×28] 
→ Conv1[16×26×26] → Pool1[16×13×13]
→ Conv2[32×11×11] → Pool2[32×5×5] 
→ FC1[128] → FC2[10]
```

### 优化后（LeNet-5风格）
```
Input[1×28×28] 
→ Conv1[6×24×24] → Pool1[6×12×12]
→ Conv2[16×8×8] → Pool2[16×4×4] 
→ FC1[84] → FC2[10]
```

## 后续步骤

1. **验证综合结果**：检查实际资源使用
2. **重新训练模型**：使用新架构训练权重
3. **性能测试**：验证推理准确度
4. **时序优化**：如果时序不满足，调整时钟周期或进一步优化

## 相关文件修改

- `src/cnn_marco.h`：网络架构参数定义
- `src/hls_cnn.h`：模板循环参数和分区指令
- `src/hls_cnn.cpp`：接口定义和数组分区
- `tests/hw/hls_config.tcl`：HLS综合配置

## 参考

本优化参考了经典的LeNet-5架构和lenet5_hls项目的实现策略。
