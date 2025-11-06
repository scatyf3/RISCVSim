# RISC-V 模拟器编译指南

## 编译方法

```bash
cd code/
g++ -std=c++17 -o simulator main.cpp
```

## 运行方法

```bash
./simulator <input_directory>
```

例如：
```bash
./simulator ../../Sample_Testcases_SS_FS/input/testcase0
```

## 输出文件

结果将生成在 `result/` 目录下：
- `SS_DMEMResult.txt` - 单阶段数据存储器结果
- `FS_DMEMResult.txt` - 五阶段数据存储器结果  
- `StateResult_SS.txt` - 单阶段状态结果
- `StateResult_FS.txt` - 五阶段状态结果
- `<testcase>SS_RFResult.txt` - 单阶段寄存器文件结果
- `<testcase>FS_RFResult.txt` - 五阶段寄存器文件结果

## 支持的指令

- **R型**: ADD, SUB, XOR, OR, AND
- **I型**: ADDI, XORI, ORI, ANDI, LW
- **J型**: JAL
- **B型**: BEQ, BNE
- **S型**: SW
- **特殊**: HALT (0xFFFFFFFF)
