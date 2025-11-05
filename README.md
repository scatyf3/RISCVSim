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

according to project instructions, we need to support
- R type: add/sub/xor/or/and
- I type: 
    - addi/xori/ori/andi 
    - lw
- j type: jal
- B type: beq/bne
- S tyoe: sw
- halt

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
(base) [scatyf3@DESKTOP-SUAQVFP RISCVSim]$ cd /home/scatyf3/RISCVSim && ls -la result/
total 64
drwxr-xr-x  2 scatyf3 scatyf3 4096 Nov  5 15:12 .
drwxr-xr-x 11 scatyf3 scatyf3 4096 Nov  5 15:12 ..
-rw-r--r--  1 scatyf3 scatyf3 9000 Nov  5 15:12 FS_DMEMResult.txt
-rw-r--r--  1 scatyf3 scatyf3 9000 Nov  5 15:12 SS_DMEMResult.txt
-rw-r--r--  1 scatyf3 scatyf3 6412 Nov  5 15:12 StateResult_FS.txt
-rw-r--r--  1 scatyf3 scatyf3  304 Nov  5 15:12 StateResult_SS.txt
-rw-r--r--  1 scatyf3 scatyf3 9837 Nov  5 15:12 testcase0FS_RFResult.txt
-rw-r--r--  1 scatyf3 scatyf3 6558 Nov  5 15:12 testcase0SS_RFResult.txt
```

based on this, we write a script to check the correctness