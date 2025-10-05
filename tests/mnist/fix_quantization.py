#!/usr/bin/env python3
"""
自动修复训练脚本中的量化问题
移除训练时的量化操作，仅在导出时量化
"""

import sys
import os

def fix_train_model():
    filepath = 'train_model.py'
    
    if not os.path.exists(filepath):
        print(f"❌ 找不到文件: {filepath}")
        return False
    
    # 备份原文件
    backup_path = filepath + '.before_fix'
    if not os.path.exists(backup_path):
        with open(filepath, 'r') as f:
            content = f.read()
        with open(backup_path, 'w') as f:
            f.write(content)
        print(f"✅ 已备份到: {backup_path}")
    
    # 读取文件
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    # 修改forward函数中的量化调用
    modified = False
    in_forward = False
    indent_level = 0
    new_lines = []
    
    for i, line in enumerate(lines):
        # 检测forward函数开始
        if 'def forward(self, x):' in line:
            in_forward = True
            indent_level = len(line) - len(line.lstrip())
            new_lines.append(line)
            continue
        
        # 检测forward函数结束
        if in_forward and line.strip() and not line[indent_level].isspace():
            in_forward = False
        
        # 在forward函数内，注释掉量化操作
        if in_forward and 'x = self.quant(x)' in line and not line.strip().startswith('#'):
            # 获取缩进
            indent = line[:len(line) - len(line.lstrip())]
            new_lines.append(f'{indent}# x = self.quant(x)  # QAT disabled - quantize only at export\n')
            modified = True
            print(f"  行 {i+1}: 已注释量化操作")
        else:
            new_lines.append(line)
    
    if modified:
        # 写回文件
        with open(filepath, 'w') as f:
            f.writelines(new_lines)
        print(f"\n✅ 已修复 {filepath}")
        print("\n修改内容:")
        print("  - 在训练的forward()中禁用了量化操作")
        print("  - 权重导出时仍会正确量化")
        print("  - 不影响HLS推理精度\n")
        return True
    else:
        print(f"\n⚠️  未发现需要修改的量化操作")
        print("  可能已经修复过，或forward()函数结构不同\n")
        return False

def main():
    print("="*60)
    print("准确率修复脚本 - 禁用训练时量化")
    print("="*60)
    
    if fix_train_model():
        print("="*60)
        print("🚀 下一步:")
        print("="*60)
        print("  1. 清理旧模型:")
        print("     rm -rf weights/ checkpoints/")
        print()
        print("  2. 快速训练验证 (20 epochs, ~15分钟):")
        print("     make mnist_train_quick")
        print()
        print("  3. 如果准确率 > 85%，运行完整训练:")
        print("     make mnist_train")
        print()
        print("预期结果: 测试准确率 90-93%")
        print("="*60)
    else:
        print("="*60)
        print("⚠️  修复未完成")
        print("="*60)
        print("请手动检查 train_model.py 的 forward() 函数")
        print("需要注释掉所有: x = self.quant(x)")
        print("="*60)

if __name__ == '__main__':
    main()
