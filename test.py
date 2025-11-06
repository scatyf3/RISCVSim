#!/usr/bin/env python3
"""
RISC-V Simulator Test Script
Compiles the simulator and runs test cases 0, 1, and 2
Compares output with expected results
"""

import os
import subprocess
import sys
import shutil
import filecmp
from pathlib import Path

def run_command(cmd, cwd=None, input_text=None):
    """Run a shell command and return the result"""
    try:
        result = subprocess.run(
            cmd, 
            shell=True, 
            cwd=cwd,
            input=input_text,
            text=True,
            capture_output=True,
            timeout=30
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        print(f"Command timed out: {cmd}")
        return -1, "", "Timeout"
    except Exception as e:
        print(f"Error running command: {cmd}\n{e}")
        return -1, "", str(e)

def compile_simulator():
    """Compile the simulator using make"""
    print("=== Compiling Simulator ===")
    
    # Clean first
    returncode, stdout, stderr = run_command("make clean")
    if returncode != 0:
        print(f"Make clean warning: {stderr}")
    
    # Build
    returncode, stdout, stderr = run_command("make")
    if returncode != 0:
        print(f"Compilation failed!")
        print(f"stdout: {stdout}")
        print(f"stderr: {stderr}")
        return False
    
    print("Compilation successful!")
    return True

def run_testcase(testcase_num):
    """Run a specific test case"""
    testcase = f"testcase{testcase_num}"
    input_path = f"Sample_Testcases_SS_FS/input/{testcase}"
    result_path = f"result/{testcase}"
    expected_path = f"Sample_Testcases_SS_FS/output/{testcase}"
    
    print(f"\n=== Running {testcase} ===")
    
    # Check if input directory exists
    if not os.path.exists(input_path):
        print(f"Input directory not found: {input_path}")
        return False
    
    # Create result directory
    os.makedirs(result_path, exist_ok=True)
    
    # Prepare input for simulator (path containing memory files)
    simulator_input = f"{input_path}\n{testcase}\n{result_path}\n"
    
    # Run simulator
    print(f"Running simulator with input path: {input_path}")
    returncode, stdout, stderr = run_command("./simulator", input_text=simulator_input)
    
    if returncode != 0:
        print(f"Simulator failed for {testcase}")
        print(f"stdout: {stdout}")
        print(f"stderr: {stderr}")
        return False
    
    print(f"Simulator completed for {testcase}")
    if stdout:
        print(f"Output: {stdout}")
    
    return True

def normalize_content(content):
    """Remove separator lines and normalize spacing in content"""
    lines = content.strip().split('\n')
    normalized_lines = []
    
    for line in lines:
        line = line.strip()
        # Skip separator lines (lines containing only dashes)
        if line and not all(c == '-' for c in line):
            # Normalize spacing: remove extra spaces and normalize around colons
            # Replace multiple spaces with single space
            line = ' '.join(line.split())
            # Normalize colon spacing for cycle numbers specifically
            if 'cycle:' in line:
                # Handle "cycle: 0", "cycle:0", "cycle:  0" all as "cycle:0"
                line = line.replace('cycle:', 'cycle:').replace('cycle: ', 'cycle:')
            normalized_lines.append(line)
    
    return '\n'.join(normalized_lines)

def compare_files_ignore_separators(result_file, expected_file):
    """Compare two files while ignoring separator lines and spacing differences"""
    try:
        with open(result_file, 'r') as f:
            result_content = f.read()
        with open(expected_file, 'r') as f:
            expected_content = f.read()
        
        # Normalize both contents
        result_normalized = normalize_content(result_content)
        expected_normalized = normalize_content(expected_content)
        
        return result_normalized == expected_normalized
        
    except Exception as e:
        print(f"Error reading files: {e}")
        return False

def compare_results(testcase_num):
    """Compare simulation results with expected output"""
    testcase = f"testcase{testcase_num}"
    result_path = f"result/{testcase}"
    expected_path = f"Sample_Testcases_SS_FS/output/{testcase}"
    
    print(f"\n=== Comparing Results for {testcase} ===")
    
    if not os.path.exists(expected_path):
        print(f"Expected output directory not found: {expected_path}")
        return False
    
    if not os.path.exists(result_path):
        print(f"Result directory not found: {result_path}")
        return False
    
    # Files to compare (only Single Stage)
    files_to_compare = [
        "StateResult_SS.txt",
        "SS_RFResult.txt",
        "SS_DMEMResult.txt"
    ]
    
    all_match = True
    
    for filename in files_to_compare:
        result_file = os.path.join(result_path, filename)
        expected_file = os.path.join(expected_path, filename)
        
        if not os.path.exists(expected_file):
            print(f"Expected file not found: {expected_file}")
            continue
            
        if not os.path.exists(result_file):
            print(f"❌ Result file missing: {filename}")
            all_match = False
            continue
        
        try:
            # Use custom comparison that ignores separator lines
            if compare_files_ignore_separators(result_file, expected_file):
                print(f"✅ {filename}: MATCH")
            else:
                print(f"❌ {filename}: MISMATCH")
                all_match = False
                
                # Show first few lines of difference
                with open(result_file, 'r') as f:
                    result_content = f.read().strip()
                with open(expected_file, 'r') as f:
                    expected_content = f.read().strip()
                
                print(f"  Expected: {expected_content[:100]}...")
                print(f"  Got:      {result_content[:100]}...")
                
        except Exception as e:
            print(f"❌ Error comparing {filename}: {e}")
            all_match = False
    
    return all_match
    """Compare simulation results with expected output"""
    testcase = f"testcase{testcase_num}"
    result_path = f"result/{testcase}"
    expected_path = f"Sample_Testcases_SS_FS/output/{testcase}"
    
    print(f"\n=== Comparing Results for {testcase} ===")
    
    if not os.path.exists(expected_path):
        print(f"Expected output directory not found: {expected_path}")
        return False
    
    if not os.path.exists(result_path):
        print(f"Result directory not found: {result_path}")
        return False
    
    # Files to compare (only Single Stage)
    files_to_compare = [
        "StateResult_SS.txt",
        "SS_RFResult.txt",
        "SS_DMEMResult.txt"
    ]
    
    all_match = True
    
    for filename in files_to_compare:
        result_file = os.path.join(result_path, filename)
        expected_file = os.path.join(expected_path, filename)
        
        if not os.path.exists(expected_file):
            print(f"Expected file not found: {expected_file}")
            continue
            
        if not os.path.exists(result_file):
            print(f"❌ Result file missing: {filename}")
            all_match = False
            continue
        
        try:
            if filecmp.cmp(result_file, expected_file, shallow=False):
                print(f"✅ {filename}: MATCH")
            else:
                print(f"❌ {filename}: MISMATCH")
                all_match = False
                
                # Show first few lines of difference
                with open(result_file, 'r') as f:
                    result_content = f.read().strip()
                with open(expected_file, 'r') as f:
                    expected_content = f.read().strip()
                
                print(f"  Expected: {expected_content[:100]}...")
                print(f"  Got:      {result_content[:100]}...")
                
        except Exception as e:
            print(f"❌ Error comparing {filename}: {e}")
            all_match = False
    
    return all_match

def main():
    """Main test function"""
    print("RISC-V Simulator Test Suite")
    print("=" * 40)
    
    # Change to project directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    # Step 1: Compile simulator
    if not compile_simulator():
        print("❌ Compilation failed, exiting")
        sys.exit(1)
    
    # Check if simulator executable exists
    if not os.path.exists("./simulator"):
        print("❌ Simulator executable not found")
        sys.exit(1)
    
    # Step 2: Run test cases
    test_results = {}
    
    for testcase_num in [0, 1, 2]:
        success = run_testcase(testcase_num)
        test_results[testcase_num] = success
        
        if success:
            # Compare results
            match = compare_results(testcase_num)
            test_results[testcase_num] = match
        else:
            test_results[testcase_num] = False
    
    # Step 3: Summary
    print("\n" + "=" * 40)
    print("TEST SUMMARY")
    print("=" * 40)
    
    all_passed = True
    for testcase_num in [0, 1, 2]:
        status = "✅ PASS" if test_results[testcase_num] else "❌ FAIL"
        print(f"Testcase {testcase_num}: {status}")
        if not test_results[testcase_num]:
            all_passed = False
    
    if all_passed:
        print("\n🎉 All tests passed!")
        sys.exit(0)
    else:
        print("\n❌ Some tests failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()

