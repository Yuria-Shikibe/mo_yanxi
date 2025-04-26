import os
import subprocess
import argparse
from concurrent.futures import ThreadPoolExecutor


def compile_proto(proto_file, proto_root, output_dir):
    """使用protoc编译单个proto文件"""
    # 计算相对于proto_root的相对路径，用于保持目录结构
    rel_path = os.path.relpath(os.path.dirname(proto_file), proto_root)
    output_path = os.path.join(output_dir, rel_path)

    # 创建输出目录
    os.makedirs(output_path, exist_ok=True)

    # 构建protoc命令
    cmd = [
        "protoc",
        f"--proto_path={proto_root}",
        f"--cpp_out={output_path}",
        proto_file
    ]

    try:
        subprocess.run(cmd, check=True)
        print(f"成功编译: {proto_file} -> {output_path}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"编译失败: {proto_file}, 错误: {e}")
        return False


def find_proto_files(root_dir):
    """递归查找所有.proto文件"""
    proto_files = []
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(".proto"):
                proto_files.append(os.path.join(dirpath, filename))
    return proto_files


def main(input_dir, output_dir):
    # 获取所有proto文件
    proto_files = find_proto_files(input_dir)

    if not proto_files:
        print(f"在目录 {input_dir} 中未找到.proto文件")
        return

    print(f"找到 {len(proto_files)} 个.proto文件，开始编译...")

    # 使用线程池并行编译
    with ThreadPoolExecutor() as executor:
        results = list(executor.map(
            lambda f: compile_proto(f, input_dir, output_dir),
            proto_files
        ))

    success_count = sum(results)
    print(f"编译完成，成功: {success_count}, 失败: {len(proto_files) - success_count}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="批量编译.proto文件")
    parser.add_argument("-i", "--input_dir", required=True, help="包含.proto文件的输入目录")
    parser.add_argument("-o", "--output_dir", required=True, help="输出目录")
    args = parser.parse_args()

    # 确保输入目录存在
    if not os.path.isdir(args.input_dir):
        print(f"错误: 输入目录 {args.input_dir} 不存在")
        exit(1)

    # 创建输出目录
    os.makedirs(args.output_dir, exist_ok=True)

    main(args.input_dir, args.output_dir)
