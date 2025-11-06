#include "common.h"
#include "insmem.h"
#include "datamem.h"
#include "registerfile.h"
#include "core.h"

// Standard headers
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

using std::string;
using std::ofstream;
using std::cout;
using std::endl;
using std::uint32_t;

// Core class implementations
Core::Core(string ioDir, InsMem &imem, DataMem &dmem) 
    : myRF(ioDir), ioDir{ioDir}, ext_imem{imem}, ext_dmem{dmem} {}

void Core::setOutputDirectory(const string& outputDir) {
    if (outputDir.empty()) return;
    ioDir = outputDir;
    std::error_code ec;
    std::filesystem::create_directories(ioDir, ec); 
}

void Core::printState(stateStruct state, int cycle) {
    ofstream printstate;
    if (cycle == 0)
        printstate.open(opFilePath, std::ios_base::trunc);
    else 
        printstate.open(opFilePath, std::ios_base::app);
    if (printstate.is_open()) {
        printstate << "State after executing instruction " << cycle << ":" << endl;
        printstate << "PC: " << state.IF.PC << endl;
        printstate << "IF.nop: " << state.IF.nop << endl;
        printstate.close();
    }
    else cout << "Unable to open " << opFilePath << " for writing." << endl;
}

// SingleStageCore implementations
SingleStageCore::SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem)
    : Core(ioDir + "SS_", imem, dmem), opFilePath(ioDir + "StateResult_SS.txt") {}

void SingleStageCore::setOutputDirectory(const string& outputDir) {
    Core::setOutputDirectory(outputDir);
    opFilePath = ioDir + "/StateResult_SS.txt";
}

void SingleStageCore::step() {
    if (halted) return;
    
    // Fetch instruction
    bitset<32> instruction = ext_imem.readInstr(state.IF.PC);
    
    // Check for halt
    if (instruction.to_ulong() == 0xFFFFFFFF) {
        halted = true;
        state.IF.nop = true;
        return;
    }
    
    // Decode and execute instruction
    bitset<7> opcode = bitset<7>((instruction.to_ulong()) & 0x7F);
    
    nextState = state;
    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
    nextState.IF.nop = false;
    
    if (opcode == bitset<7>(0x33)) { // R-type
        bitset<5> rs1 = bitset<5>((instruction.to_ulong() >> 15) & 0x1F);
        bitset<5> rs2 = bitset<5>((instruction.to_ulong() >> 20) & 0x1F);
        bitset<5> rd = bitset<5>((instruction.to_ulong() >> 7) & 0x1F);
        bitset<3> funct3 = bitset<3>((instruction.to_ulong() >> 12) & 0x7);
        bitset<7> funct7 = bitset<7>((instruction.to_ulong() >> 25) & 0x7F);
        
        bitset<32> rs1_val = myRF.readRF(rs1);
        bitset<32> rs2_val = myRF.readRF(rs2);
        bitset<32> result;
        
        if (funct3 == bitset<3>(0x0)) { // ADD/SUB
            if (funct7 == bitset<7>(0x00)) { // ADD
                result = bitset<32>(rs1_val.to_ulong() + rs2_val.to_ulong());
            } else if (funct7 == bitset<7>(0x20)) { // SUB
                result = bitset<32>(rs1_val.to_ulong() - rs2_val.to_ulong());
            }
        } else if (funct3 == bitset<3>(0x4)) { // XOR
            result = rs1_val ^ rs2_val;
        } else if (funct3 == bitset<3>(0x6)) { // OR
            result = rs1_val | rs2_val;
        } else if (funct3 == bitset<3>(0x7)) { // AND
            result = rs1_val & rs2_val;
        }
        
        if (rd != bitset<5>(0)) {
            myRF.writeRF(rd, result);
        }
        
    } else if (opcode == bitset<7>(0x13)) { // I-type
        bitset<5> rs1 = bitset<5>((instruction.to_ulong() >> 15) & 0x1F);
        bitset<5> rd = bitset<5>((instruction.to_ulong() >> 7) & 0x1F);
        bitset<3> funct3 = bitset<3>((instruction.to_ulong() >> 12) & 0x7);
        bitset<12> imm = bitset<12>((instruction.to_ulong() >> 20) & 0xFFF);
        
        // Sign extend immediate
        int32_t imm_val = (int32_t)imm.to_ulong();
        if (imm[11]) imm_val |= 0xFFFFF000; // Sign extend
        
        bitset<32> rs1_val = myRF.readRF(rs1);
        bitset<32> result;
        
        if (funct3 == bitset<3>(0x0)) { // ADDI
            result = bitset<32>((int32_t)rs1_val.to_ulong() + imm_val);
        } else if (funct3 == bitset<3>(0x4)) { // XORI
            result = rs1_val ^ bitset<32>(imm_val);
        } else if (funct3 == bitset<3>(0x6)) { // ORI
            result = rs1_val | bitset<32>(imm_val);
        } else if (funct3 == bitset<3>(0x7)) { // ANDI
            result = rs1_val & bitset<32>(imm_val);
        }
        
        if (rd != bitset<5>(0)) {
            myRF.writeRF(rd, result);
        }
        
    } else if (opcode == bitset<7>(0x03)) { // LW
        bitset<5> rs1 = bitset<5>((instruction.to_ulong() >> 15) & 0x1F);
        bitset<5> rd = bitset<5>((instruction.to_ulong() >> 7) & 0x1F);
        bitset<12> imm = bitset<12>((instruction.to_ulong() >> 20) & 0xFFF);
        
        // Sign extend immediate
        int32_t imm_val = (int32_t)imm.to_ulong();
        if (imm[11]) imm_val |= 0xFFFFF000;
        
        bitset<32> rs1_val = myRF.readRF(rs1);
        bitset<32> address = bitset<32>((int32_t)rs1_val.to_ulong() + imm_val);
        bitset<32> data = ext_dmem.readDataMem(address);
        
        if (rd != bitset<5>(0)) {
            myRF.writeRF(rd, data);
        }
        
    } else if (opcode == bitset<7>(0x23)) { // SW
        bitset<5> rs1 = bitset<5>((instruction.to_ulong() >> 15) & 0x1F);
        bitset<5> rs2 = bitset<5>((instruction.to_ulong() >> 20) & 0x1F);
        bitset<7> imm_11_5 = bitset<7>((instruction.to_ulong() >> 25) & 0x7F);
        bitset<5> imm_4_0 = bitset<5>((instruction.to_ulong() >> 7) & 0x1F);
        
        // Reconstruct immediate
        bitset<12> imm = bitset<12>((imm_11_5.to_ulong() << 5) | imm_4_0.to_ulong());
        int32_t imm_val = (int32_t)imm.to_ulong();
        if (imm[11]) imm_val |= 0xFFFFF000;
        
        bitset<32> rs1_val = myRF.readRF(rs1);
        bitset<32> rs2_val = myRF.readRF(rs2);
        bitset<32> address = bitset<32>((int32_t)rs1_val.to_ulong() + imm_val);
        
        ext_dmem.writeDataMem(address, rs2_val);
        
    } else if (opcode == bitset<7>(0x63)) { // B-type
        bitset<5> rs1 = bitset<5>((instruction.to_ulong() >> 15) & 0x1F);
        bitset<5> rs2 = bitset<5>((instruction.to_ulong() >> 20) & 0x1F);
        bitset<3> funct3 = bitset<3>((instruction.to_ulong() >> 12) & 0x7);
        
        // Extract immediate bits
        bitset<1> imm_12 = bitset<1>((instruction.to_ulong() >> 31) & 0x1);
        bitset<1> imm_11 = bitset<1>((instruction.to_ulong() >> 7) & 0x1);
        bitset<6> imm_10_5 = bitset<6>((instruction.to_ulong() >> 25) & 0x3F);
        bitset<4> imm_4_1 = bitset<4>((instruction.to_ulong() >> 8) & 0xF);
        
        // Reconstruct immediate
        bitset<13> imm = bitset<13>((imm_12.to_ulong() << 12) | 
                                   (imm_11.to_ulong() << 11) |
                                   (imm_10_5.to_ulong() << 5) |
                                   (imm_4_1.to_ulong() << 1));
        
        int32_t imm_val = (int32_t)imm.to_ulong();
        if (imm[12]) imm_val |= 0xFFFFE000;
        
        bitset<32> rs1_val = myRF.readRF(rs1);
        bitset<32> rs2_val = myRF.readRF(rs2);
        
        bool take_branch = false;
        if (funct3 == bitset<3>(0x0)) { // BEQ
            take_branch = (rs1_val == rs2_val);
        } else if (funct3 == bitset<3>(0x1)) { // BNE
            take_branch = (rs1_val != rs2_val);
        }
        
        if (take_branch) {
            nextState.IF.PC = bitset<32>((int32_t)state.IF.PC.to_ulong() + imm_val);
        }
        
    } else if (opcode == bitset<7>(0x6F)) { // JAL
        bitset<5> rd = bitset<5>((instruction.to_ulong() >> 7) & 0x1F);
        
        // Extract immediate bits
        bitset<1> imm_20 = bitset<1>((instruction.to_ulong() >> 31) & 0x1);
        bitset<10> imm_10_1 = bitset<10>((instruction.to_ulong() >> 21) & 0x3FF);
        bitset<1> imm_11 = bitset<1>((instruction.to_ulong() >> 20) & 0x1);
        bitset<8> imm_19_12 = bitset<8>((instruction.to_ulong() >> 12) & 0xFF);
        
        // Reconstruct immediate
        bitset<21> imm = bitset<21>((imm_20.to_ulong() << 20) |
                                   (imm_19_12.to_ulong() << 12) |
                                   (imm_11.to_ulong() << 11) |
                                   (imm_10_1.to_ulong() << 1));
        
        int32_t imm_val = (int32_t)imm.to_ulong();
        if (imm[20]) imm_val |= 0xFFE00000;
        
        if (rd != bitset<5>(0)) {
            myRF.writeRF(rd, bitset<32>(state.IF.PC.to_ulong() + 4));
        }
        
        nextState.IF.PC = bitset<32>((int32_t)state.IF.PC.to_ulong() + imm_val);
    }
    
    state = nextState;
    cycle++;
}

void SingleStageCore::printState() {
    myRF.outputRF(cycle);
    Core::printState(state, cycle);
}

// FiveStageCore implementations  
FiveStageCore::FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem)
    : Core(ioDir + "FS_", imem, dmem), opFilePath(ioDir + "StateResult_FS.txt") {}

void FiveStageCore::setOutputDirectory(const string& outputDir) {
    Core::setOutputDirectory(outputDir);
    opFilePath = ioDir + "/StateResult_FS.txt";
}

void FiveStageCore::step() {
    if (halted) return;
    
    // Simple implementation - same as single stage for now
    // TODO: Implement proper pipeline stages
    
    // Fetch instruction
    bitset<32> instruction = ext_imem.readInstr(state.IF.PC);
    
    // Check for halt
    if (instruction.to_ulong() == 0xFFFFFFFF) {
        halted = true;
        return;
    }
    
    // For now, execute same as single stage
    // This should be expanded to proper 5-stage pipeline
    SingleStageCore temp(ioDir, ext_imem, ext_dmem);
    temp.state = state;
    temp.myRF = myRF;
    temp.step();
    state = temp.state;
    myRF = temp.myRF;
    
    cycle++;
}

void FiveStageCore::printState() {
    myRF.outputRF(cycle);
    Core::printState(state, cycle);
}