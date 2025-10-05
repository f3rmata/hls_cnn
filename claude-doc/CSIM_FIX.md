# HLS C仿真修复说明

## 📅 日期
2025年10月5日

## 🐛 问题

运行 `vitis_hls -f run_hls.tcl` 时出现链接错误：

```
ld.lld: error: undefined symbol: uut_top(float*, float*, ...)
>>> referenced by test.cpp:383
```

## 🔍 根本原因

TCL脚本中的编译选项配置问题：

1. **设计文件** (`hls_cnn.cpp`, `uut_top.cpp`) 编译时**没有** `-DUSE_FLOAT`
   - 使用 `ap_fixed<16,8>` 类型
   - 函数签名: `uut_top(ap_fixed<16,8>*, ...)`

2. **测试文件** (`test.cpp`) 编译时**有** `-DUSE_FLOAT`
   - 使用 `float` 类型
   - 函数调用: `uut_top(float*, ...)`

3. **链接时**符号不匹配，导致 `undefined symbol` 错误

## ✅ 解决方案

修改 `run_hls.tcl`，在C仿真时为设计文件也添加 `-DUSE_FLOAT`：

```tcl
# Add design files
# Note: For CSIM, we use -DUSE_FLOAT to match testbench types
#       For CSYNTH, HLS will compile WITHOUT -DUSE_FLOAT automatically
if {$CSIM == 1 && $CSYNTH == 0} {
  # C Simulation only - use float for easy debugging
  add_files "${SRC_DIR}/hls_cnn.cpp" -cflags "-I${SRC_DIR} -std=c++14 -DUSE_FLOAT"
  add_files "${CUR_DIR}/uut_top.cpp" -cflags "-I${SRC_DIR} -I${CUR_DIR} -std=c++14 -DUSE_FLOAT"
} else {
  # Synthesis or both - use fixed-point for hardware
  add_files "${SRC_DIR}/hls_cnn.cpp" -cflags "-I${SRC_DIR} -std=c++14"
  add_files "${CUR_DIR}/uut_top.cpp" -cflags "-I${SRC_DIR} -I${CUR_DIR} -std=c++14"
}
```

## 📊 修复效果

**修复前**:
```
ERROR: [SIM 211-100] 'csim_design' failed: compilation error(s).
```

**修复后**:
```
========================================
TEST PASSED!
========================================
INFO: [SIM 211-1] CSim done with 0 errors.
```

## 🔧 工作原理

### C仿真模式 (CSIM=1, CSYNTH=0)

- **目的**: 快速验证功能，使用float便于调试
- **编译**: 所有文件都带 `-DUSE_FLOAT`
- **类型**: `data_t` = `float`, `weight_t` = `float`
- **优点**: 编译快，精度高，便于调试

### 综合模式 (CSYNTH=1)

- **目的**: 生成硬件，使用定点优化资源
- **编译**: 设计文件**不带** `-DUSE_FLOAT`
- **类型**: `data_t` = `ap_fixed<16,8>`, `weight_t` = `ap_fixed<16,8>`
- **优点**: 资源使用少，符合硬件实现

### 类型定义 (cnn_marco.h)

```cpp
#ifndef USE_FLOAT
// Fixed-point types for hardware synthesis
typedef ap_fixed<16, 8> data_t;
typedef ap_fixed<16, 8> weight_t;
typedef ap_fixed<32, 16> acc_t;
#else
// Floating-point types for C simulation
typedef float data_t;
typedef float weight_t;
typedef float acc_t;
#endif
```

## 🚀 使用方法

### 方法1: 使用Makefile (推荐)

```bash
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn

# C仿真
make hls_csim

# 综合
make hls_synth

# 协同仿真
make hls_cosim
```

### 方法2: 直接使用TCL

```bash
cd tests/hw

# C仿真 (使用float)
vitis_hls -f run_hls.tcl

# 修改TCL脚本，设置 CSYNTH=1，然后运行综合
# set CSYNTH 1
vitis_hls -f run_hls.tcl
```

## ⚠️ 注意事项

### 1. 不要同时运行CSIM和CSYNTH

如果在TCL中设置：
```tcl
set CSIM 1
set CSYNTH 1
```

会导致设计文件使用 `ap_fixed`（因为条件判断 `$CSIM == 1 && $CSYNTH == 0` 为false），但综合需要的就是这个。

**推荐做法**：
- C仿真：`CSIM=1, CSYNTH=0`
- 综合：`CSIM=0, CSYNTH=1`
- 分开运行

### 2. Testbench始终使用float

Testbench文件 (`test.cpp`) 始终带 `-DUSE_FLOAT`，因为：
- Testbench不参与综合
- 使用float便于golden reference计算
- 与设计文件的C仿真版本类型一致

### 3. 综合后的精度

- C仿真（float）：最大误差接近0
- 综合（ap_fixed<16,8>）：预期误差 < 0.1（由testbench tolerance控制）

## 📁 相关文件

- `tests/hw/run_hls.tcl` - ✅ 已修复
- `src/cnn_marco.h` - 类型定义
- `tests/hw/test.cpp` - 测试bench
- `tests/hw/uut_top.cpp` - 硬件顶层
- `src/hls_cnn.cpp` - CNN实现

## 📚 参考文档

- **[Vitis HLS User Guide UG1399](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls)**
- **[HLS Arbitrary Precision Types](https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/C-Arbitrary-Precision-Integer-Types)**

## ✅ 验收标准

修复成功的标志：

1. ✅ `vitis_hls -f run_hls.tcl` 运行成功
2. ✅ 输出 `TEST PASSED!`
3. ✅ `CSim done with 0 errors`
4. ✅ 最大误差 = 0（float模式下）

---

**状态**: ✅ 已修复并测试通过  
**下一步**: 运行综合验证资源使用
