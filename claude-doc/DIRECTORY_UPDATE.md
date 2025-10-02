# 目录结构更新说明

## 变更内容

测试文件已重组为更标准的目录结构，遵循 Vitis_Libraries 的组织方式：

### 新目录结构

```
hls_cnn/
├── src/                          # 核心源代码（不变）
│   ├── hls_cnn.h
│   ├── hls_cnn.cpp
│   └── cnn_marco.h
├── tests/
│   ├── sw/                       # 软件测试（CPU）
│   │   ├── unit_test.cpp         # 单元测试
│   │   └── integration_test.cpp  # 集成测试
│   ├── hw/                       # 硬件测试（HLS）
│   │   ├── uut_top.hpp           # 硬件顶层接口
│   │   └── uut_top.cpp           # 硬件顶层实现
│   ├── test.cpp                  # HLS C/Co-仿真测试
│   └── run_hls.tcl               # HLS TCL脚本
├── build/                        # 编译产物
├── Makefile
└── quick_test.sh
```

### 修改的文件

#### 1. Makefile
- 添加 `TEST_SW_DIR` 和 `TEST_HW_DIR` 变量
- 更新 unit_test 规则: `$(TEST_SW_DIR)/unit_test.cpp`
- 更新 integration_test 规则: `$(TEST_SW_DIR)/integration_test.cpp`

#### 2. tests/run_hls.tcl
- 更新设计文件路径: `${CUR_DIR}/hw/uut_top.cpp`
- 更新设计文件路径: `${CUR_DIR}/hw/uut_top.hpp`
- 添加 `-I${CUR_DIR}/hw` 到测试文件编译选项

#### 3. tests/test.cpp
- 更新 include: `#include "hw/uut_top.hpp"`

#### 4. tests/hw/uut_top.hpp
- 更新 include: `#include "../../src/cnn_marco.h"`

#### 5. tests/hw/uut_top.cpp
- 更新 include: `#include "../../src/hls_cnn.h"`

## 使用方法

### CPU 测试
```bash
make unit_test          # 编译并运行单元测试
make integration_test   # 编译并运行集成测试
```

### HLS 测试
```bash
make hls_csim          # HLS C 仿真
make hls_synth         # HLS 综合
make hls_cosim         # HLS Co-仿真
```

### 一键测试
```bash
./quick_test.sh        # 运行所有测试
```

## 优势

1. **清晰的分离**: 软件测试 (sw/) 与硬件测试 (hw/) 分开
2. **标准化**: 遵循 Vitis_Libraries 的目录组织
3. **易维护**: 每个组件职责明确
4. **可扩展**: 便于添加更多测试或硬件实现

## 兼容性

- ✅ 与原有的构建流程完全兼容
- ✅ Make 目标保持不变
- ✅ 所有文档引用已更新
- ✅ quick_test.sh 无需修改

## 文件映射

| 旧位置 | 新位置 |
|--------|--------|
| `tests/unit_test.cpp` | `tests/sw/unit_test.cpp` |
| `tests/integration_test.cpp` | `tests/sw/integration_test.cpp` |
| `src/uut_top.hpp` | `tests/hw/uut_top.hpp` |
| `src/uut_top.cpp` | `tests/hw/uut_top.cpp` |
| `tests/test.cpp` | `tests/test.cpp` (不变) |
| `tests/run_hls.tcl` | `tests/run_hls.tcl` (不变) |

## 注意事项

- 所有相对路径已更新为相对于新的目录结构
- HLS 综合时，`uut_top.*` 文件从 `tests/hw/` 读取
- C 仿真的测试文件 (`test.cpp`) 仍在 `tests/` 根目录
- 核心代码 (`src/`) 未受影响

更新日期: 2025-10-02
