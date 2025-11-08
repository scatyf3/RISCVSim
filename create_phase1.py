#!/usr/bin/env python3
"""
Smart Code Merger Script
Merge header files and source files into a single main.cpp, avoiding duplicate definitions
"""

import os
import re
from pathlib import Path

class SmartMerger:
    def __init__(self):
        self.system_includes = set()
        self.output_lines = []
        
    def extract_system_includes(self, content):
        """Extract system header includes"""
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
        """Remove header file include guards"""
        lines = content.split('\n')
        result = []
        skip_next_define = False
        
        for line in lines:
            stripped = line.strip()
            
            # Skip #ifndef XXX_H
            if re.match(r'#ifndef\s+\w+_H', stripped):
                skip_next_define = True
                continue
            
            # Skip #define XXX_H
            if skip_next_define and re.match(r'#define\s+\w+_H', stripped):
                skip_next_define = False
                continue
            
            # Skip #endif // XXX_H (at end of file)
            if re.match(r'#endif\s*//.*_H', stripped):
                continue
                
            result.append(line)
        
        return '\n'.join(result)
    
    def remove_local_includes(self, content):
        """Remove local header file includes"""
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            # Skip local includes #include "xxx.h"
            if stripped.startswith('#include "'):
                continue
            result.append(line)
        
        return '\n'.join(result)
    
    def remove_using_namespace(self, content):
        """Remove using namespace statements (will be added later uniformly)"""
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('using namespace'):
                continue
            result.append(line)
        
        return '\n'.join(result)
    
    def process_header(self, file_path):
        """Process header file: extract class declarations"""
        print(f"Processing header file: {file_path}")
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extract system includes
        self.extract_system_includes(content)
        
        # Remove include guards
        content = self.remove_include_guards(content)
        
        # Remove local includes
        content = self.remove_local_includes(content)
        
        # Remove using namespace
        content = self.remove_using_namespace(content)
        
        return content.strip()
    
    def process_source(self, file_path):
        """Process source file: extract function implementations"""
        print(f"Processing source file: {file_path}")
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extract system includes
        self.extract_system_includes(content)
        
        # Remove all include statements
        content = self.remove_local_includes(content)
        
        # Remove using statements (we'll add them uniformly at the beginning of file)
        lines = content.split('\n')
        result = []
        
        for line in lines:
            stripped = line.strip()
            # Skip include statements
            if stripped.startswith('#include'):
                continue
            # Skip using statements
            if stripped.startswith('using std::') or stripped.startswith('using namespace'):
                continue
            result.append(line)
        
        return '\n'.join(result).strip()
    
    def merge_files(self):
        """Merge all files"""
        
        # 1. Add system header files
        print("=" * 60)
        print("Starting file merge...")
        print("=" * 60)
        
        self.output_lines.append("// ==================== System Headers ====================")
        
        # Required system header files
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
        
        # 2. Add header file content (class declarations)
        self.output_lines.append("// ==================== Class Declarations (from headers) ====================")
        
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
        
        # 3. Add source file content (function implementations)
        self.output_lines.append("\n// ==================== Function Implementations (from sources) ====================")
        
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
        
        # 4. Add main function
        if os.path.exists('sim.cpp'):
            print("Processing main file: sim.cpp")
            with open('sim.cpp', 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Extract system includes
            self.extract_system_includes(content)
            
            # Remove include statements
            lines = content.split('\n')
            result = []
            for line in lines:
                stripped = line.strip()
                if not stripped.startswith('#include'):
                    result.append(line)
            
            self.output_lines.append("\n// ==================== Main Function (from sim.cpp) ====================")
            self.output_lines.append('\n'.join(result))
        
        print("=" * 60)
        print("Merge completed!")
        print("=" * 60)
    
    def write_output(self, output_file):
        """Write output file"""
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(self.output_lines))
        
        print(f"\n✅ Successfully created: {output_file}")
        print(f"📊 Total lines: {len(self.output_lines)}")
        
def create_submission_structure():
    """Create submission directory structure"""
    print("\nCreating directory structure...")
    
    # Create directories
    os.makedirs('phase1/code', exist_ok=True)
    os.makedirs('phase1/submissions', exist_ok=True)
    
    # Merge code
    merger = SmartMerger()
    merger.merge_files()
    merger.write_output('phase1/code/main.cpp')
    
    # Copy README.md to code directory
    import shutil
    if os.path.exists('README.md'):
        shutil.copy2('README.md', 'phase1/code/README.md')
        print(f"✅ Copied project documentation: phase1/code/README.md")
    
    # Copy test script to code directory
    if os.path.exists('test.py'):
        shutil.copy2('test.py', 'phase1/code/test.py')
        print(f"✅ Copied test script: phase1/code/test.py")
    
    # Copy test cases to code directory
    if os.path.exists('Sample_Testcases_SS_FS'):
        shutil.copytree('Sample_Testcases_SS_FS', 'phase1/code/Sample_Testcases_SS_FS', dirs_exist_ok=True)
        print(f"✅ Copied test cases: phase1/code/Sample_Testcases_SS_FS")
    
    # Create simple compilation script (without using Makefile)
    compile_script = """#!/bin/bash
# Simple compilation script
echo "Compiling RISC-V Simulator..."
g++ -std=c++17 -Wall -Wextra -o simulator main.cpp
if [ $? -eq 0 ]; then
    echo "✅ Compilation successful: simulator"
else
    echo "❌ Compilation failed"
    exit 1
fi
"""
    
    with open('phase1/code/compile.sh', 'w', encoding='utf-8') as f:
        f.write(compile_script)
    
    # Set execution permissions
    import stat
    os.chmod('phase1/code/compile.sh', stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
    
    print(f"✅ Created compilation script: phase1/code/compile.sh")
    
    # Copy submission.md to submissions directory
    if os.path.exists('submission.md'):
        shutil.copy2('submission.md', 'phase1/submissions/submission.md')
        print(f"✅ Copied submission documentation: phase1/submissions/submission.md")
    
    print("\n" + "=" * 60)
    print("📦 Complete submission package structure created!")
    print("=" * 60)
    print("\nDirectory structure:")
    print("phase1/")
    print("├── code/")
    print("│   ├── main.cpp                    # Merged source code")
    print("│   ├── compile.sh                  # Compilation script")
    print("│   ├── test.py                     # Automated testing")
    print("│   ├── README.md                   # Project documentation")  
    print("│   └── Sample_Testcases_SS_FS/    # Test cases")
    print("└── submissions/")
    print("    └── submission.md              # Submission documentation")
    print("\nNext steps:")
    print("1. cd phase1/code && ./compile.sh   # Test compilation")
    print("2. python3 test.py                  # Run tests")
    print("3. cd ../.. && zip -r phase1.zip phase1/  # Create submission package")
    print()

if __name__ == "__main__":
    create_submission_structure()
