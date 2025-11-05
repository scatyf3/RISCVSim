#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include <cstdint>

using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space reason, we keep it as this large number, but the memory is still 32-bit addressable.

// Pipeline stage structures
struct IFStruct {
    bitset<32>  PC;
    bool        nop;  
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;  
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<32>  Imm;        // Extended to 32-bit for larger immediates
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bitset<3>   funct3;     // Function code for instruction variants
    bitset<7>   funct7;     // Function code for R-type instructions
    bitset<7>   opcode;     // Opcode for instruction type
    bitset<32>  PC;         // Program counter for branch/jump calculations
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        alu_op;     // Extended usage for different operations
    bool        wrt_enable;
    bool        is_branch;  // Indicates branch instruction
    bool        is_jump;    // Indicates jump instruction
    bool        nop;  
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;    
    bitset<5>   Wrt_reg_addr;
    bitset<32>  branch_target; // Target address for branches/jumps
    bool        rd_mem;
    bool        wrt_mem; 
    bool        wrt_enable;
    bool        branch_taken;   // Result of branch condition
    bool        is_jump;        // Jump instruction flag    
    bool        nop;    
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;     
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;     
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};

#endif // COMMON_H