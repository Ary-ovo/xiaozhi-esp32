import os
import shutil

# ================= 配置区域 =================
# 填写你的 data 文件夹路径
DATA_DIR = "/home/linxiaoyan/work_space/xiaozhi-icon/data"
# ===========================================

def fix_folder_structure():
    if not os.path.exists(DATA_DIR):
        print(f"错误: 找不到目录 {DATA_DIR}")
        return

    print(f"正在修复目录结构: {DATA_DIR} ...")
    
    # 获取 data 目录下所有的子项
    items = os.listdir(DATA_DIR)
    
    success_count = 0

    for item_name in items:
        # 构造完整路径
        item_path = os.path.join(DATA_DIR, item_name)
        
        # 1. 检查这是否是一个“伪装成文件”的文件夹
        #    (即：它名字叫 xxx.bin，但实际上是个文件夹)
        if os.path.isdir(item_path) and item_name.endswith(".bin"):
            
            # 在这个文件夹里找真正的 bin 文件
            # 通常里面的文件也叫 xxx.bin 或者 xxx_argb8888.bin 等
            inner_files = os.listdir(item_path)
            found_bin = False
            
            for inner_file in inner_files:
                if inner_file.endswith(".bin"):
                    # 找到了真正的文件
                    src_file = os.path.join(item_path, inner_file)
                    
                    # 构造一个临时路径 (避免和文件夹重名冲突)
                    temp_dst = os.path.join(DATA_DIR, f"temp_{item_name}")
                    
                    # A. 把文件移出来，变成临时文件
                    shutil.move(src_file, temp_dst)
                    print(f"[提取] {inner_file} -> temp_{item_name}")
                    found_bin = True
                    break
            
            if found_bin:
                # B. 安全地删除那个占位的文件夹
                try:
                    shutil.rmtree(item_path) # 强制删除文件夹及其残留内容
                    print(f"[删除] 冗余文件夹: {item_name}")
                    
                    # C. 把临时文件改回正确的名字
                    final_dst = os.path.join(DATA_DIR, item_name)
                    os.rename(temp_dst, final_dst)
                    print(f"[重名] 恢复为: {item_name}")
                    success_count += 1
                except Exception as e:
                    print(f"处理 {item_name} 时出错: {e}")
            else:
                print(f"[跳过] 文件夹 {item_name} 里没找到 bin 文件")

    print("-" * 30)
    print(f"修复完成！共还原了 {success_count} 个文件。")

if __name__ == "__main__":
    fix_folder_structure()