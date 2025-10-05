# HLS C仿真错误修复 - 总结

## 问题
```
ld.lld: error: undefined symbol: uut_top(float*, ...)
ERROR: [SIM 211-100] 'csim_design' failed: compilation error(s).
```

## 原因
设计文件使用 `ap_fixed<16,8>` 类型，测试文件使用 `float` 类型，链接时符号不匹配。

## 解决方案
修改 `tests/hw/run_hls.tcl`，C仿真时为设计文件也添加 `-DUSE_FLOAT`：

```tcl
if {$CSIM == 1 && $CSYNTH == 0} {
  # C Simulation - use float
  add_files "${SRC_DIR}/hls_cnn.cpp" -cflags "... -DUSE_FLOAT"
  add_files "${CUR_DIR}/uut_top.cpp" -cflags "... -DUSE_FLOAT"
} else {
  # Synthesis - use fixed-point
  add_files "${SRC_DIR}/hls_cnn.cpp" -cflags "..."
  add_files "${CUR_DIR}/uut_top.cpp" -cflags "..."
}
```

## 结果
```
TEST PASSED!
INFO: [SIM 211-1] CSim done with 0 errors.
Maximum error: 0
```

## 使用方法
```bash
# C仿真
cd /home/fermata/Development/FPGA/Vitis/2025-fpga-comp-prj/hls_cnn
make hls_csim

# 或直接运行
cd tests/hw
vitis_hls -f run_hls.tcl
```

## 详细说明
见 [CSIM_FIX.md](CSIM_FIX.md)
