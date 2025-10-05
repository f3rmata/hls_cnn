#!/usr/bin/env python3
"""
环境验证脚本 - 检查训练所需的所有依赖
"""

import sys
import os

def check_item(name, check_func):
    """检查单个项目"""
    try:
        result = check_func()
        if result:
            print(f"✅ {name}: {result}")
            return True
        else:
            print(f"✅ {name}")
            return True
    except Exception as e:
        print(f"❌ {name}: {str(e)}")
        return False

def main():
    print("="*70)
    print("环境检查 - MNIST CNN训练")
    print("="*70)
    print()
    
    all_ok = True
    
    # 1. Python版本
    print("1. Python环境")
    all_ok &= check_item(
        "Python版本",
        lambda: f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
    )
    
    # 2. 必需的包
    print("\n2. Python包")
    
    # NumPy
    all_ok &= check_item(
        "NumPy",
        lambda: __import__('numpy').__version__
    )
    
    # PyTorch
    all_ok &= check_item(
        "PyTorch",
        lambda: __import__('torch').__version__
    )
    
    # TorchVision (可选)
    check_item(
        "TorchVision",
        lambda: __import__('torchvision').__version__
    )
    
    # 3. CUDA (可选)
    print("\n3. GPU支持 (可选)")
    try:
        import torch
        if torch.cuda.is_available():
            print(f"✅ CUDA可用: {torch.version.cuda}")
            print(f"   设备: {torch.cuda.get_device_name(0)}")
        else:
            print("ℹ️  CUDA不可用 (将使用CPU训练)")
    except:
        print("ℹ️  无法检查CUDA")
    
    # 4. 数据文件
    print("\n4. 数据文件")
    data_dir = os.path.join(os.path.dirname(__file__), 'data')
    
    files_to_check = [
        'train_images.bin',
        'train_labels.bin',
        'test_images.bin',
        'test_labels.bin'
    ]
    
    for filename in files_to_check:
        filepath = os.path.join(data_dir, filename)
        if os.path.exists(filepath):
            size = os.path.getsize(filepath)
            size_mb = size / (1024 * 1024)
            print(f"✅ {filename}: {size_mb:.1f} MB")
        else:
            print(f"❌ {filename}: 不存在")
            all_ok = False
    
    # 5. 目录检查
    print("\n5. 目录结构")
    dirs_to_check = ['data', 'weights', 'checkpoints']
    
    for dirname in dirs_to_check:
        dirpath = os.path.join(os.path.dirname(__file__), dirname)
        if os.path.exists(dirpath):
            print(f"✅ {dirname}/: 存在")
        else:
            print(f"ℹ️  {dirname}/: 不存在 (训练时会自动创建)")
    
    # 6. 训练脚本
    print("\n6. 训练脚本")
    script_path = os.path.join(os.path.dirname(__file__), 'train_model.py')
    if os.path.exists(script_path):
        # 检查量化是否被禁用
        with open(script_path, 'r') as f:
            content = f.read()
            if '# x = self.quant(x)' in content:
                print("✅ train_model.py: 存在 (量化已禁用)")
            else:
                print("⚠️  train_model.py: 存在 (量化可能未禁用)")
                all_ok = False
    else:
        print("❌ train_model.py: 不存在")
        all_ok = False
    
    # 总结
    print("\n" + "="*70)
    if all_ok:
        print("✅ 环境检查通过！可以开始训练")
        print("\n推荐命令:")
        print("  ./run_train.sh verify    # 快速验证 (5 epochs)")
        print("  ./run_train.sh quick     # 快速训练 (20 epochs)")
        print("  ./run_train.sh full      # 完整训练 (60 epochs)")
    else:
        print("❌ 环境检查失败，请修复以上问题")
        print("\n常见问题修复:")
        print("  1. 缺少PyTorch: conda install pytorch -c pytorch")
        print("  2. 缺少数据: python3 download_mnist.py")
        print("  3. 量化未禁用: python3 fix_quantization.py")
    print("="*70)
    
    return 0 if all_ok else 1

if __name__ == '__main__':
    sys.exit(main())
