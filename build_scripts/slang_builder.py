import os
import sys
import subprocess
import argparse
from pathlib import Path

def compile_slang_to_spv(input_dir, output_dir, slangc_path="slangc"):
    """
    将输入文件夹中的.slang着色器文件编译成SPIR-V文件

    Args:
        input_dir (str): 输入文件夹路径
        output_dir (str): 输出文件夹路径
        slangc_path (str): slangc编译器路径，默认为'slangc'
    """

    # 确保输出目录存在
    Path(output_dir).mkdir(parents=True, exist_ok=True)

    # 查找所有的.slang文件:cite[2]:cite[5]:cite[8]
    slang_files = []
    for item in Path(input_dir).iterdir():
        if item.is_file() and item.suffix.lower() == '.slang':
            slang_files.append(item)

    if not slang_files:
        print(f"在目录 {input_dir} 中未找到任何.slang文件")
        return

    print(f"找到 {len(slang_files)} 个.slang文件")

    successful_compiles = 0
    failed_compiles = 0

    for slang_file in slang_files:
        try:
            # 构建输出文件路径
            output_filename = slang_file.stem + ".spv"
            output_path = Path(output_dir) / output_filename

            print(f"正在编译: {slang_file.name} -> {output_filename}")

            # 构建slangc命令:cite[1]
            # 使用优化选项 -O 进行优化
            cmd = [
                slangc_path,
                "-O",  # 启用优化
                "-fvk-use-gl-layout",
                "-matrix-layout-column-major",
                "-o", str(output_path),
                str(slang_file)
            ]

            # 执行编译命令
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30  # 30秒超时
            )

            if result.returncode == 0:
                print(f"✓ 成功编译: {slang_file.name}\n")
                successful_compiles += 1
            else:
                print(f"✗ 编译失败: {slang_file.name}")
                print(f"  错误信息: {result.stderr}")
                failed_compiles += 1

        except subprocess.TimeoutExpired:
            print(f"✗ 编译超时: {slang_file.name}")
            failed_compiles += 1
        except Exception as e:
            print(f"✗ 处理文件时出错 {slang_file.name}: {str(e)}")
            failed_compiles += 1

    # 输出总结信息
    print(f"\n编译完成!")
    print(f"成功: {successful_compiles}, 失败: {failed_compiles}")

    # if failed_compiles > 0:
    #     sys.exit(1)

def main():
    parser = argparse.ArgumentParser(
        description="将.slang着色器文件编译为SPIR-V字节码",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python slang_compiler.py input_shaders/ output_spv/
  python slang_compiler.py -c /usr/local/bin/slangc input/ output/
        """
    )

    parser.add_argument(
        "input_dir",
        help="包含.slang文件的输入目录"
    )

    parser.add_argument(
        "output_dir",
        help="SPIR-V文件的输出目录"
    )

    parser.add_argument(
        "-c", "--compiler",
        dest="slangc_path",
        default="slangc",
        help="slangc编译器路径 (默认: 'slangc')"
    )

    args = parser.parse_args()

    # 验证输入目录
    if not os.path.isdir(args.input_dir):
        print(f"错误: 输入目录 '{args.input_dir}' 不存在")
        sys.exit(1)

    # 检查编译器是否存在
    try:
        subprocess.run([args.slangc_path, "--version"],
                       capture_output=True, timeout=5)
    except (subprocess.TimeoutExpired, FileNotFoundError):
        print(f"错误: 找不到slangc编译器 '{Path(args.slangc_path).resolve()}'")
        print("请确保已安装Shader-Slang并正确配置PATH环境变量")
        print("或使用 -c 参数指定完整的编译器路径")
        sys.exit(1)

    # 执行编译
    compile_slang_to_spv(args.input_dir, args.output_dir, args.slangc_path)

if __name__ == "__main__":
    main()