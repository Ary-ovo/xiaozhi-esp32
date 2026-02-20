import os
import subprocess
import glob

# ================= 配置区域 =================
# 1. 这里填写存放 PNG 图片的目录路径 (可以是相对路径或绝对路径)
#    例如: "S:/icons" 或者 "/home/linxiaoyan/icons"
INPUT_DIR = "/home/linxiaoyan/work_space/xiaozhi-icon" 

# 2. 这里填写 LVGLImage.py 的路径 (如果脚本和转换器在同一目录下，保持默认即可)
CONVERTER_SCRIPT = "./LVGLImage.py"

# 3. 设置颜色格式 (根据你的屏幕配置选择)
#    带透明度图标通常用: CF_TRUE_COLOR_ALPHA
#    不带透明度通常用: CF_TRUE_COLOR_565 或 CF_TRUE_COLOR
COLOR_FORMAT = "I1" 

# 4. 输出格式 (bin 文件)
OUTPUT_FORMAT = "BIN"
# ===========================================

def main():
    # 确保 LVGLImage.py 存在
    if not os.path.exists(CONVERTER_SCRIPT):
        print(f"错误: 找不到转换脚本 {CONVERTER_SCRIPT}")
        return

    # 构造输出目录路径
    data_dir = os.path.join(INPUT_DIR, "data")
    
    # 如果 data 目录不存在，创建它
    if not os.path.exists(data_dir):
        print(f"创建输出目录: {data_dir}")
        os.makedirs(data_dir)

    # 查找所有的 PNG 文件
    png_files = glob.glob(os.path.join(INPUT_DIR, "*.png"))
    
    if not png_files:
        print(f"在 {INPUT_DIR} 没有找到 PNG 文件")
        return

    print(f"找到 {len(png_files)} 个 PNG 文件，准备转换...")

    for png_file in png_files:
        # 获取文件名 (不带路径)
        file_name = os.path.basename(png_file)
        # 构造输出文件名 (.bin)
        file_name_no_ext = os.path.splitext(file_name)[0]
        output_file = os.path.join(data_dir, file_name_no_ext + ".bin")

        # 构造命令
        # 格式: python3 LVGLImage.py <输入文件> --ofmt <格式> --cf <颜色> -o <输出文件>
        cmd = [
            "python3", 
            CONVERTER_SCRIPT,
            png_file,
            "--ofmt", OUTPUT_FORMAT,
            "--cf", COLOR_FORMAT,
            "-o", output_file
        ]

        print(f"正在转换: {file_name} -> data/{file_name_no_ext}.bin")
        
        try:
            # 执行命令
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            print(f"转换失败: {file_name}")
            print(e)

    print("所有转换完成！")

if __name__ == "__main__":
    main()