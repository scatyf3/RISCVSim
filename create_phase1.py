#!/usr/bin/env python3
"""
智能代码合并脚本
将头文件和源文件合并为单个 main.cpp，避免重复定义
"""

import os
import re
from pathlib import Path

class SmartMerger:
    def __init__(self):
        self.system_includes = set()
        self.output_lines = []
        
    def extract_system_includes(self, content):
        """提取系统头文件包含"""
        lines = content.split('\n')
        includes = []
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('#include <'):
                if stripped not in self.system_includes:
                    self.system_includes.add(stripped)
                    includes.append(stripped)
        return includes
    
    def remove_include_guards(self, content):
        """移除头文件保护宏"""
        lines = content.split('\n')
        result = []
        skip_next_define = False
        
        for line in lines:
            stripped = line.strip()
            
            # 跳过 #ifndef XXX_H
            if re.match(r'#ifndef\s+\w+_H', stripped):
                skip_next_define = True
                continue
            
            # 跳过 #define XXX_H
            if skip_next_define and re.match(r'#define\s+\w+_H', stripped):
                skip_next_define = False
                continue
            
            # 跳过 #endif // XXX_H (文件末尾的)
            if re.match(r'#endif\s*//.*_H', stripped):
                continue
                
            result.append(line)
        
        return '\n'.join(result)
    
    def remove_local_includes(self, content):
        """移除本地头文件包含"""
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            # 跳过本地包含 #include "xxx.h"
            if stripped.startswith('#include "'):
                continue
            result.append(line)
        
        return '\n'.join(result)
    
    def remove_using_namespace(self, content):
        """移除 using namespace 语句（稍后统一添加）"""
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('using namespace'):
                continue
            result.append(line)
        
        return '\n'.join(result)
    
    def process_header(self, file_path):
        """处理头文件：提取类声明"""
        print(f"处理头文件: {file_path}")
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 提取系统包含
        self.extract_system_includes(content)
        
        # 移除包含保护
        content = self.remove_include_guards(content)
        
        # 移除本地包含
        content = self.remove_local_includes(content)
        
        # 移除 using namespace
        content = self.remove_using_namespace(content)
        
        return content.strip()
    
    def process_source(self, file_path):
        """处理源文件：提取函数实现"""
        print(f"处理源文件: {file_path}")
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 提取系统包含
        self.extract_system_includes(content)
        
        # 移除所有包含语句
        content = self.remove_local_includes(content)
        
        # 移除 using 语句（我们会在文件开头统一添加）
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            # 跳过包含语句
            if stripped.startswith('#include'):
                continue
            # 跳过 using 语句
            if stripped.startswith('using std::') or stripped.startswith('using namespace'):
                continue
            result.append(line)
        
        return '\n'.join(result).strip()
    
    def merge_files(self):
        """合并所有文件"""
        
        # 1. 添加系统头文件
        print("=" * 60)
        print("开始合并文件...")
        print("=" * 60)
        
        self.output_lines.append("// ==================== 系统头文件 ====================")
        
        # 必需的系统头文件
        required_includes = [
            "#include <iostream>",
            "#include <string>",
            "#include <vector>",
            "#include <bitset>",
            "#include <fstream>",
            "#include <cstdint>",
            "#include <algorithm>",
            "#include <map>",
            "#include <filesystem>",
        ]
        
        for inc in required_includes:
            self.output_lines.append(inc)
        
        self.output_lines.append("")
        self.output_lines.append("using namespace std;")
        self.output_lines.append("")
        
        # 2. 添加头文件内容（类声明）
        self.output_lines.append("// ==================== 类声明 (来自头文件) ====================")
        
        header_files = [
            'include/common.h',
            'include/insmem.h',
            'include/datamem.h',
            'include/registerfile.h',
            'include/core.h'
        ]
        
        for header in header_files:
            if os.path.exists(header):
                content = self.process_header(header)
                if content:
                    self.output_lines.append(f"\n// ---------- {header} ----------")
                    self.output_lines.append(content)
                    self.output_lines.append("")
        
        # 3. 添加源文件内容（函数实现）
        self.output_lines.append("\n// ==================== 函数实现 (来自源文件) ====================")
        
        source_files = [
            'src/insmem.cpp',
            'src/datamem.cpp',
            'src/registerfile.cpp',
            'src/core.cpp'
        ]
        
        for source in source_files:
            if os.path.exists(source):
                content = self.process_source(source)
                if content:
                    self.output_lines.append(f"\n// ---------- {source} ----------")
                    self.output_lines.append(content)
                    self.output_lines.append("")
        
        # 4. 添加 main 函数
        if os.path.exists('sim.cpp'):
            print("处理主文件: sim.cpp")
            with open('sim.cpp', 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 提取系统包含
            self.extract_system_includes(content)
            
            # 移除包含语句
            lines = content.split('\n')
            result = []
            for line in lines:
                stripped = line.strip()
                if not stripped.startswith('#include'):
                    result.append(line)
            
            self.output_lines.append("\n// ==================== Main 函数 (来自 sim.cpp) ====================")
            self.output_lines.append('\n'.join(result))
        
        print("=" * 60)
        print("合并完成！")
        print("=" * 60)
    
    def write_output(self, output_file):
        """写入输出文件"""
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(self.output_lines))
        
        print(f"\n✅ 成功创建: {output_file}")
        print(f"📊 总行数: {len(self.output_lines)}")
        
def create_submission_structure():
    """创建提交目录结构"""
    print("\n创建目录结构...")
    
    # 创建目录
    os.makedirs('phase1/code', exist_ok=True)
    os.makedirs('phase1/submissions', exist_ok=True)
    
    # 合并代码
    merger = SmartMerger()
    merger.merge_files()
    merger.write_output('phase1/code/main.cpp')
    
    # 拷贝 README.md 到 code 目录
    import shutil
    if os.path.exists('README.md'):
        shutil.copy2('README.md', 'phase1/code/README.md')
        print(f"✅ 拷贝项目文档: phase1/code/README.md")
    
    # 拷贝测试脚本到 code 目录
    if os.path.exists('test.py'):
        shutil.copy2('test.py', 'phase1/code/test.py')
        print(f"✅ 拷贝测试脚本: phase1/code/test.py")
    
    # 拷贝测试用例到 code 目录
    if os.path.exists('Sample_Testcases_SS_FS'):
        shutil.copytree('Sample_Testcases_SS_FS', 'phase1/code/Sample_Testcases_SS_FS', dirs_exist_ok=True)
        print(f"✅ 拷贝测试用例: phase1/code/Sample_Testcases_SS_FS")
    
    # 创建简单的编译脚本（不使用 Makefile）
    compile_script = """#!/bin/bash
# 简单编译脚本
echo "编译 RISC-V 模拟器..."
g++ -std=c++17 -Wall -Wextra -o simulator main.cpp
if [ $? -eq 0 ]; then
    echo "✅ 编译成功: simulator"
else
    echo "❌ 编译失败"
    exit 1
fi
"""
    
    with open('phase1/code/compile.sh', 'w', encoding='utf-8') as f:
        f.write(compile_script)
    
    # 设置执行权限
    import stat
    os.chmod('phase1/code/compile.sh', stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
    
    print(f"✅ 创建编译脚本: phase1/code/compile.sh")
    
    # 拷贝 submission.md 到 submissions 目录
    if os.path.exists('submission.md'):
        shutil.copy2('submission.md', 'phase1/submissions/submission.md')
        print(f"✅ 拷贝提交说明: phase1/submissions/submission.md")
    
    print("\n" + "=" * 60)
    print("📦 完整提交包结构创建完成！")
    print("=" * 60)
    print("\n目录结构:")
    print("phase1/")
    print("├── code/")
    print("│   ├── main.cpp                    # 合并的源代码")
    print("│   ├── compile.sh                  # 编译脚本")
    print("│   ├── test.py                     # 自动化测试")
    print("│   ├── README.md                   # 项目文档")  
    print("│   └── Sample_Testcases_SS_FS/    # 测试用例")
    print("└── submissions/")
    print("    └── submission.md              # 提交说明")
    print("\n下一步:")
    print("1. cd phase1/code && ./compile.sh   # 测试编译")
    print("2. python3 test.py                  # 运行测试")
    print("3. cd ../.. && zip -r phase1.zip phase1/  # 创建提交压缩包")
    print()

if __name__ == "__main__":
    create_submission_structure()
