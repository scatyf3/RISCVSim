# project structure

first according to the give cpp file in commit `c75d65a0f7ebfb728013e6f4d3d4df47fa899c6f`, we reconstruct them to 4 parts
- core, with class `Core`, `SingleStageCore` and `FiveStageCore`
- datamem, with class `DataMem`
- insmem, with class `InsMem`
- registerfile, with class `DataMem`
- general header `common.h`

apart from that, we build the test file and print function for each class to test the implementation.


thus the project structure is

```
(base) [scatyf3@DESKTOP-SUAQVFP RISCVSim]$ cd /home/scatyf3/RISCVSim && find . -type f -name "*.h" -o -name "*.cpp" -o -name "Makefile" | head -20
./include/datamem.h
./include/common.h
./include/core.h
./include/insmem.h
./include/registerfile.h
./test/test_registerfile.cpp
./test/test_imem.cpp
./test/test_datamem.cpp
./src/insmem.cpp
./src/datamem.cpp
./src/registerfile.cpp
./sim.cpp
./Makefile
```

# TODOs

- InsMem's readInstr
- DataMem's 
    - readDataMem
    - writeDataMem
- RegisterFile's
    - readRF
    - writeRF
- SingleStageCore's 
    - step

## InsMem

Instruction memory is organized by 4 line x 8 bits = 32 bits's instruction, thus we need to read 4 lines after the given `ReadAddress`

```c++
bitset<32> InsMem::readInstr(bitset<32> ReadAddress) {    
    // read instruction memory
    bitset<32> val;
    for (int i = 0; i < 4; i++) {
        bitset<32> byte_val = bitset<32>(IMem[ReadAddress.to_ulong() + i].to_ulong());
        val |= (byte_val << (i * 8));
    }
    return val;
}
```

## DataMem

same as ReadInstr in InsMem, we can implement memory I/O function in DataMem

```c++
bitset<32> DataMem::readDataMem(bitset<32> Address) {	
    // read data memory
    bitset<32> val;
    for (int i = 0; i < 4; i++) {
        bitset<32> byte_val = bitset<32>(DMem[Address.to_ulong() + i].to_ulong());
        val |= (byte_val << (i * 8));
    }
    return val;
}

void DataMem::writeDataMem(bitset<32> Address, bitset<32> WriteData) {
    // write into memory - little endian
    uint32_t addr = Address.to_ulong();
    uint32_t data = WriteData.to_ulong();
    
    for (int i = 0; i < 4; i++) {
        DMem[addr + i] = bitset<8>((data >> (i * 8)) & 0xFF);
    }
}
```

## RegisterFile

- `readRF` given the register number, read the according register, and return the 32 bits binary value inside
- `writeRF` given the register number and 32bits value, write the value to register

```c++
bitset<32> RegisterFile::readRF(bitset<5> Reg_addr) {   
    uint32_t addr = Reg_addr.to_ulong();
    if (addr < 32) {
        return Registers[addr];
    }
    return bitset<32>(0);
}

void RegisterFile::writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
    uint32_t addr = Reg_addr.to_ulong();
    if (addr < 32 && addr != 0) {  // Register 0 is always 0
        Registers[addr] = Wrt_reg_data;
    }
}
```


## SingleStageCore

### target

according to project instructions, we need to support
- R type: add/sub/xor/or/and
- I type: 
    - addi/xori/ori/andi 
    - lw
- j type: jal
- B type: beq/bne
- S tyoe: sw
- halt

### build test pipeline

To check the correstness of our program, we need to compare standard output in `Sample_Testcases_SS_FS/output` and file dumped from our simulator. However, the standard dump method output txt file in same path where we give to simulator, eg, `Sample_Testcases_SS_FS/input/testcase0`, thus we need to modify output method

```c++
// datamem
void outputDataMem();
void outputDataMem(string outputDir); 
// register file
void outputRF(int cycle);
void outputRF(int cycle, string outputDir); 
// set core's output dir
virtual void printState() = 0;
void setOutputDirectory(string outputDir); 
```

Thus we can print all outputfile in result.
```
(base) ➜  testcase0 git:(main) ✗ ls -a               
.                  FS_DMEMResult.txt  SS_DMEMResult.txt  StateResult_FS.txt
..                 FS_RFResult.txt    SS_RFResult.txt    StateResult_SS.txt
```

based on this, we write a script to check the correctness, seen `test.py`


### explain for each file

FS/SS
- FS: five stage version
- SS: single stage version

data
- DMEMResult: data memory
- StateResult: processor state at each cycle 
- RFResult:  register file



### implement

IRL RISCV execute stage: IF - ID - EX - MEM - WB

- check if nop
- IF
- check if halt
- decode instruction(ID)
    - `opcode`: instruct[6:0]
    - `rd` (dest reg): instruct[11:7]
    - `funct3`: instruct[14:12]
    - `rs1` (source reg 1): instruct[19:15]
    - `rs2` (source reg 2): instruct[24:20]
    - `funct7`: instruct[31:25]
- execute EX and MEM
    - **R-type (opcode=0x33)**: (ADD, SUB, XOR, OR, AND)
        - **EX**: ALU result = `rs1_val` (op) `rs2_val` (op determined by `funct3`/`funct7`)
        - **WB**: `write_data` = ALU result
    - **I-type (opcode=0x13)**: (ADDI, XORI, ORI, ANDI)
        - **EX**: Get sign-extended `imm`; ALU result = `rs1_val` (op) `imm` (op determined by `funct3`)
        - **WB**: `write_data` = ALU result
    - **Load (opcode=0x03)**: (LW)
        - **EX**: Get sign-extended `imm`; Address = `rs1_val` + `imm`
        - **MEM**: Read from Data Memory at `Address`
        - **WB**: `write_data` = Data from Memory
    - **Store (opcode=0x23)**: (SW)
        - **EX**: Get sign-extended S-type `imm` [31:25, 11:7]; Address = `rs1_val` + `imm`
        - **MEM**: Write `rs2_val` to Data Memory at `Address`
    - **Branch (opcode=0x63)**: (BEQ, BNE)
        - **EX**: Get sign-extended B-type `imm`; Check condition (`rs1 == rs2` or `rs1 != rs2` based on `funct3`)
        - **IF Update**: If (condition true), `nextPC` = `PC` + `imm`; else `nextPC` = `PC` + 4
    - **JAL (opcode=0x6F)**:
        - **EX**: Get sign-extended J-type `imm`
        - **WB**: `write_data` = `PC` + 4 (return address)
        - **IF Update**: `nextPC` = `PC` + `imm`
    - **Default (others)**:
        - **IF Update**: `nextPC` = `PC` + 4
- WB



### error fix

#### mem

fix error from core, to modify memory in core, we need `ext_imem` and `ext_dmem` to be reference instead of value, else, you can not dump the modified version of memory to txt

in the given code, Core has following member variables

```c++
class Core {
    public:
        InsMem ext_imem; 
        DataMem ext_dmem;
};
```

in main code, we use core and memorys as follow

```c++
InsMem imem = InsMem("Imem", ioDir);
DataMem dmem_ss = DataMem("SS", ioDir);
DataMem dmem_fs = DataMem("FS", ioDir);

SingleStageCore SSCore(ioDir, imem, dmem_ss);

// do something to SSCore's dmem

// dump dmem
dmem_ss.outputDataMem(resultDir);
```

however, this code contains unexpected behaivor, since SSCore modify it's dmem_ss instead of dmem_ss in main, thus the dump result stays no change.

Thus we need to change Core's member variables to reference

```c++
class Core {
    public:
        InsMem& ext_imem; 
        DataMem& ext_dmem;
};
```

#### file io
```
hexdump -C Sample_Testcases_SS_FS/input/testcase1/imem.txt | tail -5
```

我看到了问题！文件使用的是 Windows 风格的换行符 \r\n (0d 0a)，而且文件末尾没有换行符。这可能会导致读取的字符串包含回车符 \r，从而使bitset构造函数失败。

#### fit data format

different testcase has different data format, which is 

```shell
(base) ➜  RISCVSim git:(main) ✗ head -3 Sample_Testcases_SS_FS/output/testcase0/SS_RFResult.txt
State of RF after executing cycle:  0
00000000000000000000000000000000
00000000101001010000000000000000
(base) ➜  RISCVSim git:(main) ✗ head -3 Sample_Testcases_SS_FS/output/testcase2/SS_RFResult.txt
State of RF after executing cycle:  0
00000000000000000000000000000000
00000000000000000000000000000000
(base) ➜  RISCVSim git:(main) ✗ head -3 Sample_Testcases_SS_FS/output/testcase1/SS_RFResult.txt
----------------------------------------------------------------------
State of RF after executing cycle:0
00000000000000000000000000000000
```

thus we modify test.py, allow it can compare with `---` and without slash

another problem is in testcase0 and testcase1,  we do not update PC in and after halt


```
SW R4, R0, #4 
----------------------------------------------------------------------
State after executing cycle: 4
IF.PC: 20
IF.nop: False
HALT   
----------------------------------------------------------------------
State after executing cycle: 5
IF.PC: 20
IF.nop: True
----------------------------------------------------------------------
State after executing cycle: 6
IF.PC: 20
IF.nop: True

```

however, in testcase2, we update PC in halt
```
24: B2: BNE R4, R2, #-16 
----------------------------------------------------------------------
State after executing cycle: 33
IF.PC: 28
IF.nop: False
28:     HALT   
----------------------------------------------------------------------
State after executing cycle: 34
IF.PC: 32
IF.nop: True
----------------------------------------------------------------------
State after executing cycle: 35
IF.PC: 32
IF.nop: True

```

I do not understand the logic of this, but to fit the output, I write this logit to control the halt behavior according to previous instructions

```c++

// Check for HALT instruction (all 1s)
if (instruction.to_ulong() == 0xFFFFFFFF) {
    nextState.IF.nop = true;
    
    // Check if previous instruction was a branch by reading it
    bool updatePC = true;  // Default: update PC
    if (state.IF.PC.to_ulong() >= 4) {
        bitset<32> prev_pc(state.IF.PC.to_ulong() - 4);
        bitset<32> prev_instr = ext_imem.readInstr(prev_pc);
        uint32_t prev_opcode = prev_instr.to_ulong() & 0x7F;
        
        // If previous instruction was NOT a branch, don't update PC
        if (prev_opcode != 0x63) {  // 0x63 is B-type (branch)
            updatePC = false;
        }
    }
    
    if (updatePC) {
        nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
    } else {
        nextState.IF.PC = state.IF.PC;  // Keep current PC
    }
```