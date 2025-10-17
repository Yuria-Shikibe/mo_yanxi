import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
import shlex

def check_slangc_compiler(slangc_path):
    """检查slangc编译器是否有效"""
    try:
        result = subprocess.run([slangc_path, "-v"],
                                capture_output=True,
                                text=True,
                                timeout=10)
        if result.returncode == 0:
            print(f"✓ slangc编译器有效: {slangc_path}")
            return True
        else:
            print(f"✗ slangc编译器无效: {result.stderr}")
            return False
    except FileNotFoundError:
        print(f"✗ 找不到slangc编译器: {slangc_path}")
        return False
    except subprocess.TimeoutExpired:
        print(f"✗ slangc编译器执行超时")
        return False
    except Exception as e:
        print(f"✗ 检查slangc编译器时出错: {e}")
        return False

def check_json_file(json_path):
    """检查JSON配置文件是否存在"""
    if not os.path.isfile(json_path):
        print(f"✗ JSON配置文件不存在: {json_path}")
        return False

    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            json.load(f)  # 尝试解析JSON
        print(f"✓ JSON配置文件有效: {json_path}")
        return True
    except json.JSONDecodeError as e:
        print(f"✗ JSON配置文件格式错误: {e}")
        return False
    except Exception as e:
        print(f"✗ 读取JSON配置文件时出错: {e}")
        return False

def parse_config(json_path):
    """解析JSON配置文件"""
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            config = json.load(f)

        # 验证必需字段
        if 'common_options' not in config:
            raise ValueError("JSON配置中缺少 'common_options' 字段")

        if 'shaders' not in config:
            raise ValueError("JSON配置中缺少 'shaders' 字段")

        common_options = config.get('common_options', [])
        shaders = config.get('shaders', [])
        shader_root = config.get('shader_root', '')  # 可选字段

        # 验证shaders数组结构
        for i, shader in enumerate(shaders):
            if 'file' not in shader:
                raise ValueError(f"shaders[{i}] 中缺少 'file' 字段")
            if 'options' not in shader:
                shader['options'] = []

        print(f"✓ 成功解析配置文件: {len(shaders)} 个着色器文件")
        if shader_root:
            print(f"✓ 使用着色器根目录: {shader_root}")

        return common_options, shaders, shader_root

    except Exception as e:
        print(f"✗ 解析配置文件时出错: {e}")
        return None, None, None

def compile_shader(
        slangc_path: str,
        output_dir: str,
        common_options ,
        shader_file : str,
        input_file : str,
        shader_options):
    """编译单个着色器文件"""
    try:
        shader_name = shader_file.replace(os.sep, '.').replace('/', '.')
        shader_name = Path(shader_name).with_suffix('.spv')
        output_file = os.path.join(output_dir, shader_name)

        args = [slangc_path]
        args.extend(common_options)
        args.extend(shader_options)
        args.extend(['-o', output_file])
        args.append(input_file)

        print(f"\n正在编译: {shader_file}")
        print(f"输出文件: {output_file}")
        print(f"编译命令: {' '.join(args)}")

        # 执行编译
        result = subprocess.run(args, capture_output=True, text=True, timeout=30)

        if result.returncode == 0:
            print(f"✓ 编译成功: {shader_file}")
            return True, result.stdout
        else:
            print(f"✗ 编译失败: {shader_file}")
            print(f"错误信息: {result.stderr}")

            # 如果编译失败，删除目标文件
            if os.path.exists(output_file):
                os.remove(output_file)
                print(f"已删除编译失败的目标文件: {output_file}")

            return False, result.stderr

    except subprocess.TimeoutExpired:
        print(f"✗ 编译超时: {shader_file}")
        # 删除可能部分生成的文件
        output_file = os.path.join(output_dir, f"{Path(shader_file).stem}.spv")
        if os.path.exists(output_file):
            os.remove(output_file)
        return False, "编译超时"
    except Exception as e:
        print(f"✗ 编译过程中发生异常: {e}")
        return False, str(e)

def main():
    parser = argparse.ArgumentParser(description='SLANG着色器批量编译工具')
    parser.add_argument('slangc_path', help='slangc编译器路径')
    parser.add_argument('output_dir', help='输出目录（相对路径）')
    parser.add_argument('config_file', help='配置文件路径（相对路径）')

    args = parser.parse_args()

    # 获取当前脚本所在目录的绝对路径
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # 处理相对路径
    slangc_path = Path(args.slangc_path).absolute()
    output_dir = Path(args.output_dir).absolute()
    config_file = Path(args.config_file).absolute()

    print("=" * 50)
    print("SLANG着色器批量编译工具")
    print("=" * 50)
    print(f"编译器路径: {slangc_path}")
    print(f"输出目录: {output_dir}")
    print(f"配置文件: {config_file}")
    print("-" * 50)

    # 步骤1: 检查slangc编译器
    if not check_slangc_compiler(slangc_path):
        sys.exit(1)

    # 步骤2: 检查JSON配置文件
    if not check_json_file(config_file):
        sys.exit(1)

    # 步骤3: 解析JSON配置
    common_options, shaders, shader_root = parse_config(config_file)
    if common_options is None:
        sys.exit(1)

    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    print(f"✓ 输出目录已创建/确认: {output_dir}")

    # 步骤4: 编译所有着色器
    print("\n开始编译着色器...")
    print("-" * 50)

    success_count = 0
    fail_count = 0

    for shader_config in shaders:
        shader_file = shader_config['file']
        shader_options = shader_config['options']


        # 处理着色器文件的路径
        if shader_root:
            # 如果指定了shader_root，则相对于脚本目录+shader_root
            full_shader_path = Path(shader_root).resolve().joinpath(shader_file)
        else:
            # 否则相对于脚本目录
            full_shader_path = os.path.join(script_dir, shader_file)

        if not os.path.isfile(full_shader_path):
            print(f"✗ 着色器文件不存在: {full_shader_path}")
            fail_count += 1
            continue

        success, message = compile_shader(
            str(slangc_path),
            str(output_dir),
            common_options,
            str(shader_file),
            str(full_shader_path),
            shader_options
        )

        if success:
            success_count += 1
        else:
            fail_count += 1

    # 输出总结
    print("\n" + "=" * 50)
    print("编译总结:")
    print(f"成功: {success_count} 个文件")
    print(f"失败: {fail_count} 个文件")
    print(f"总计: {len(shaders)} 个文件")

    if fail_count > 0:
        print("\n注意: 有编译失败的文件，已自动删除对应的.spv目标文件")
        # sys.exit(1)
    else:
        print("\n✓ 所有着色器编译完成!")

if __name__ == "__main__":
    main()