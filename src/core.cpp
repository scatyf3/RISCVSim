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
#include <iomanip>
#include <filesystem>

// Core class implementations
Core::Core(string ioDir, InsMem &imem, DataMem &dmem) 
    : myRF(ioDir), ioDir{ioDir}, ext_imem{imem}, ext_dmem{dmem} {}

void Core::setOutputDirectory(const string& outputDir) {
    if (outputDir.empty()) return;
    ioDir = outputDir;
    std::error_code ec;
    std::filesystem::create_directories(ioDir, ec); 
}

void Core::outputPerformanceMetrics(const std::string& output_dir) {
    std::string filename = output_dir + "/PerformanceMetrics.txt";
    std::ofstream outFile;
    
    // Check if this is the first core writing to the file
    bool is_first_write = !std::ifstream(filename).good();
    
    if (is_first_write) {
        outFile.open(filename);  // Create new file
    } else {
        outFile.open(filename, std::ios::app);  // Append to existing file
    }
    
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open " << filename << " for writing." << std::endl;
        return;
    }
    
    // Calculate CPI and IPC
    double cpi = instruction_count > 0 ? (double)cycle / instruction_count : 0.0;
    double ipc = cycle > 0 ? (double)instruction_count / cycle : 0.0;
    
    outFile << "Performance of " << getCoreType() << ":" << std::endl;
    outFile << "#Cycles -> " << cycle << std::endl;
    outFile << "#Instructions -> " << instruction_count << std::endl;
    outFile << "CPI -> " << std::setprecision(16) << cpi << std::endl;
    outFile << "IPC -> " << std::setprecision(16) << ipc << std::endl;
    
    // Add empty line only if this is not the last core
    outFile << std::endl;
    
    outFile.close();
}

void Core::printState(stateStruct state, int cycle) {
    string outputPath = getStateOutputPath();
    ofstream printstate;
    if (cycle == 0)
        printstate.open(outputPath, std::ios_base::trunc);
    else 
        printstate.open(outputPath, std::ios_base::app);
    if (printstate.is_open()) {
        printstate << "----------------------------------------------------------------------" << endl;
        printstate << "State after executing cycle: " << cycle << endl;
        printstate << "IF.PC: " << state.IF.PC.to_ulong()  << endl; // to fit the printed state from 4?
        printstate << "IF.nop: " << (state.IF.nop ? "True" : "False") << endl;
        printstate.close();
    }
    else cout << "Unable to open " << outputPath << " for writing." << endl;
}

// SingleStageCore implementations
SingleStageCore::SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem)
    : Core(ioDir + "SS_", imem, dmem), opFilePath(ioDir + "/StateResult_SS.txt") {
    // Initialize single stage state - PC starts at 0 but will be updated before first print
    state.IF.PC = 0;
    state.IF.nop = false;
    nextState = state;
}

void SingleStageCore::setOutputDirectory(const string& outputDir) {
    Core::setOutputDirectory(outputDir);
    opFilePath = outputDir + "/StateResult_SS.txt";
}

void SingleStageCore::step() {
            // Initialize next state
            nextState = state;
            
            if (state.IF.nop) {
                nopCycles++;
                myRF.outputRF(cycle, ioDir);
                Core::printState(state, cycle);
                cycle++;
                if (nopCycles >= 1) {  // Stop after 2 nop cycles
                    halted = true;
                }
                return;
            }            
            // Fetch instruction
            bitset<32> instruction = ext_imem.readInstr(state.IF.PC);
            
            // Check for HALT instruction (all 1s)
            if (instruction.to_ulong() == 0xFFFFFFFF) {
                nextState.IF.nop = true;
                instruction_count++; // Count HALT as an instruction
                
                // Simple HALT behavior - don't update PC
                nextState.IF.PC = state.IF.PC;  // Keep current PC
                
                // Update state to reflect halt condition for printing
                state = nextState;
                myRF.outputRF(cycle, ioDir);
                Core::printState(state, cycle);
                cycle++;
                return;
            }
            
            // Count this as an executed instruction
            instruction_count++;
            
            // Decode instruction
            uint32_t instr = static_cast<uint32_t>(instruction.to_ulong());
            uint32_t opcode = instr & 0x7F;
            uint32_t rd = (instr >> 7) & 0x1F;
            uint32_t funct3 = (instr >> 12) & 0x7;
            uint32_t rs1 = (instr >> 15) & 0x1F;
            uint32_t rs2 = (instr >> 20) & 0x1F;
            uint32_t funct7 = (instr >> 25) & 0x7F;
            
            // Read register values
            bitset<32> rs1_val = myRF.readRF(bitset<5>(rs1));
            bitset<32> rs2_val = myRF.readRF(bitset<5>(rs2));
            
            // Execute based on opcode
            bitset<32> alu_result;
            bitset<32> write_data;
            bool write_enable = false;
            
            switch (opcode) {
                case 0x33: { // R-type (ADD, SUB, XOR, OR, AND)
                    write_enable = true;
                    if (funct3 == 0 && funct7 == 0) { // ADD
                        alu_result = bitset<32>(rs1_val.to_ulong() + rs2_val.to_ulong());
                    } else if (funct3 == 0 && funct7 == 0x20) { // SUB
                        alu_result = bitset<32>(rs1_val.to_ulong() - rs2_val.to_ulong());
                    } else if (funct3 == 4) { // XOR
                        alu_result = rs1_val ^ rs2_val;
                    } else if (funct3 == 6) { // OR
                        alu_result = rs1_val | rs2_val;
                    } else if (funct3 == 7) { // AND
                        alu_result = rs1_val & rs2_val;
                    }
                    write_data = alu_result;
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    break;
                }
                case 0x13: { // I-type arithmetic (ADDI, XORI, ORI, ANDI)
                    write_enable = true;
                    int32_t imm = static_cast<int32_t>(instr) >> 20; // Sign extend
                    if (funct3 == 0) { // ADDI
                        alu_result = bitset<32>(rs1_val.to_ulong() + imm);
                    } else if (funct3 == 4) { // XORI
                        alu_result = rs1_val ^ bitset<32>(imm);
                    } else if (funct3 == 6) { // ORI
                        alu_result = rs1_val | bitset<32>(imm);
                    } else if (funct3 == 7) { // ANDI
                        alu_result = rs1_val & bitset<32>(imm);
                    }
                    write_data = alu_result;
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    break;
                }
                case 0x03: { // Load (LW)
                    write_enable = true;
                    int32_t imm = static_cast<int32_t>(instr) >> 20; // Sign extend
                    uint32_t address = rs1_val.to_ulong() + imm;
                    write_data = ext_dmem.readDataMem(bitset<32>(address));
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    break;
                }
                case 0x23: { // Store (SW)
                    int32_t imm = ((instr >> 7) & 0x1F) | (((instr >> 25) & 0x7F) << 5);
                    if (imm & 0x800) imm |= 0xFFFFF000; // Sign extend
                    uint32_t address = rs1_val.to_ulong() + imm;
                    ext_dmem.writeDataMem(bitset<32>(address), rs2_val);
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    break;
                }
                case 0x63: { // Branch (BEQ, BNE)
                    int32_t imm = ((instr >> 7) & 0x1) << 11 |
                                  ((instr >> 8) & 0xF) << 1 |
                                  ((instr >> 25) & 0x3F) << 5 |
                                  ((instr >> 31) & 0x1) << 12;
                    if (imm & 0x1000) imm |= 0xFFFFE000; // Sign extend
                    
                    bool take_branch = false;
                    if (funct3 == 0) { // BEQ
                        take_branch = (rs1_val == rs2_val);
                    } else if (funct3 == 1) { // BNE
                        take_branch = (rs1_val != rs2_val);
                    }
                    
                    if (take_branch) {
                        nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + imm);
                    } else {
                        nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    }
                    break;
                }
                case 0x6F: { // JAL
                    write_enable = true;
                    write_data = bitset<32>(state.IF.PC.to_ulong() + 4);
                    int32_t imm = ((instr >> 21) & 0x3FF) << 1 |
                                  ((instr >> 20) & 0x1) << 11 |
                                  ((instr >> 12) & 0xFF) << 12 |
                                  ((instr >> 31) & 0x1) << 20;
                    if (imm & 0x100000) imm |= 0xFFE00000; // Sign extend
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + imm);
                    break;
                }
                default:
                    // Update PC for normal instructions
                    nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    break;
            }
            
            // Write back to register file
            if (write_enable && rd != 0) { // Don't write to register 0
                myRF.writeRF(bitset<5>(rd), write_data);
            }
            
            // Update state to reflect instruction execution
            state = nextState;
            
            myRF.outputRF(cycle, ioDir); // dump RF
            Core::printState(state, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
            
            cycle++;
        }

void SingleStageCore::printState() {
    ofstream printstate;
    if (cycle == 0)
        printstate.open(opFilePath, std::ios_base::trunc);
    else 
        printstate.open(opFilePath, std::ios_base::app);
    if (printstate.is_open()) {
        printstate<<"----------------------------------------------------------------------"<<endl;
        printstate<<"State after executing cycle: "<<cycle<<endl; 
        printstate<<"IF.PC: "<<state.IF.PC.to_ulong()<<endl;
        printstate<<"IF.nop: "<<(state.IF.nop ? "True" : "False")<<endl;
    }
    else cout<<"Unable to open SS StateResult output file." << endl;
    printstate.close();
}


// ==========================================
// HELPER FUNCTIONS (Internal)
// ==========================================

// Helper to convert std::string to uint32_t (if needed) or handle bit manipulation
static uint32_t get_bits(uint32_t val, int high, int low) {
    uint32_t mask = (1ULL << (high - low + 1)) - 1;
    return (val >> low) & mask;
}

static int32_t sign_extend(uint32_t val, int bits) {
    int32_t shift = 32 - bits;
    return ((int32_t)val << shift) >> shift;
}

// Convert uint32 to bitset<32> string for printing (matches Python formatting)
static string int2bin(uint32_t val) {
    return bitset<32>(val).to_string();
}

// ==========================================
// ==========================================
// STAGE LOGIC
// ==========================================

// --- Instruction Fetch ---
InstructionFetchStage::InstructionFetchStage(State_five* s, InsMem* im) 
    : state(s), ins_mem(im) {}

void InstructionFetchStage::run() {
    if (state->IF.nop || state->ID.nop || (state->ID.hazard_nop && state->EX.nop)) {
        return;
    }

    // INTERFACE ADAPTER: uint32 -> bitset<32>
    bitset<32> addr(state->IF.PC);
    
    // INTERFACE CALL
    bitset<32> instr_bits = ins_mem->readInstr(addr);
    
    // INTERFACE ADAPTER: bitset<32> -> uint32
    uint32_t instr = (uint32_t)instr_bits.to_ulong();
    
    if (instr_bits.all()) { // Equivalent to "1"*32 check
        state->IF.nop = true;
        state->ID.nop = true;
    } else {
        state->ID.PC = state->IF.PC;
        state->IF.PC += 4;
        state->ID.instr = instr;
    }
}

// --- Instruction Decode ---
InstructionDecodeStage::InstructionDecodeStage(State_five* s, RegisterFile* r) 
    : state(s), rf(r) {}

int InstructionDecodeStage::detect_hazard(uint32_t rs) {
    if (rs == state->MEM.write_reg_addr && rs != 0 && state->MEM.read_mem == 0) {
        return 2; // EX to 1st
    } else if (rs == state->WB.write_reg_addr && rs != 0 && state->WB.write_enable) {
        return 1; // EX/MEM to 2nd
    } else if (rs == state->MEM.write_reg_addr && rs != 0 && state->MEM.read_mem != 0) {
        state->ID.hazard_nop = true; 
        return 1;
    }
    return 0;
}

uint32_t InstructionDecodeStage::read_data(uint32_t rs, int forward_signal) {
    if (forward_signal == 1) return state->WB.write_data;
    if (forward_signal == 2) return state->MEM.alu_result;
    
    // INTERFACE CALL: readRF(bitset<5>) -> bitset<32>
    return (uint32_t)rf->readRF(bitset<5>(rs)).to_ulong();
}

void InstructionDecodeStage::run() {
    if (state->ID.nop) {
        if (!state->IF.nop) state->ID.nop = false;
        return;
    }

    state->EX.instr = state->ID.instr;
    state->EX.is_I_type = false;
    state->EX.read_mem = false;
    state->EX.write_mem = false;
    state->EX.write_enable = false;
    state->ID.hazard_nop = false;
    state->EX.write_reg_addr = 0;

    uint32_t opcode = get_bits(state->ID.instr, 6, 0);
    uint32_t func3 = get_bits(state->ID.instr, 14, 12);
    uint32_t rd = get_bits(state->ID.instr, 11, 7);
    uint32_t rs1 = get_bits(state->ID.instr, 19, 15);
    uint32_t rs2 = get_bits(state->ID.instr, 24, 20);
    uint32_t func7 = get_bits(state->ID.instr, 31, 25);

    // R-Type
    if (opcode == 0x33) { 
        int fwd1 = detect_hazard(rs1);
        int fwd2 = detect_hazard(rs2);

        if (state->ID.hazard_nop) { state->EX.nop = true; return; }

        state->EX.rs = rs1;
        state->EX.rt = rs2;
        state->EX.read_data_1 = read_data(rs1, fwd1);
        state->EX.read_data_2 = read_data(rs2, fwd2);
        state->EX.write_reg_addr = rd;
        state->EX.write_enable = true;

        if (func3 == 0x0) { 
            state->EX.alu_op = "00";
            if (func7 == 0x20) { 
                state->EX.read_data_2 = -((int32_t)state->EX.read_data_2);
            }
        } else if (func3 == 0x7) state->EX.alu_op = "01"; 
        else if (func3 == 0x6) state->EX.alu_op = "10"; 
        else if (func3 == 0x4) state->EX.alu_op = "11"; 
    }
    // I-Type
    else if (opcode == 0x13 || opcode == 0x03) { 
        int fwd1 = detect_hazard(rs1);
        if (state->ID.hazard_nop) { state->EX.nop = true; return; }

        state->EX.rs = rs1;
        state->EX.read_data_1 = read_data(rs1, fwd1);
        state->EX.write_reg_addr = rd;
        state->EX.is_I_type = true;
        
        state->EX.imm = sign_extend(get_bits(state->ID.instr, 31, 20), 12);
        
        state->EX.write_enable = true;
        state->EX.read_mem = (opcode == 0x03);

        if (func3 == 0x0) state->EX.alu_op = "00";
        else if (func3 == 0x7) state->EX.alu_op = "01";
        else if (func3 == 0x6) state->EX.alu_op = "10";
        else if (func3 == 0x4) state->EX.alu_op = "11";
    }
    // J-Type (JAL)
    else if (opcode == 0x6F) { 
        uint32_t bit31 = get_bits(state->ID.instr, 31, 31);
        uint32_t bit19_12 = get_bits(state->ID.instr, 19, 12);
        uint32_t bit20 = get_bits(state->ID.instr, 20, 20);
        uint32_t bit30_21 = get_bits(state->ID.instr, 30, 21);
        
        uint32_t imm_val = (bit31 << 20) | (bit19_12 << 12) | (bit20 << 11) | (bit30_21 << 1);
        state->EX.imm = sign_extend(imm_val, 21);
        
        state->EX.write_reg_addr = rd;
        state->EX.read_data_1 = state->ID.PC;
        state->EX.read_data_2 = 4;
        state->EX.write_enable = true;
        state->EX.alu_op = "00";
        
        state->IF.PC = state->ID.PC + (int32_t)state->EX.imm;
        state->ID.nop = true;
    }
    // B-Type
    else if (opcode == 0x63) { 
        int fwd1 = detect_hazard(rs1);
        int fwd2 = detect_hazard(rs2);
        if (state->ID.hazard_nop) { state->EX.nop = true; return; }

        state->EX.rs = rs1;
        state->EX.rt = rs2;
        state->EX.read_data_1 = read_data(rs1, fwd1);
        state->EX.read_data_2 = read_data(rs2, fwd2);
        
        int32_t diff = (int32_t)state->EX.read_data_1 - (int32_t)state->EX.read_data_2;
        
        uint32_t bit31 = get_bits(state->ID.instr, 31, 31);
        uint32_t bit7 = get_bits(state->ID.instr, 7, 7);
        uint32_t bit30_25 = get_bits(state->ID.instr, 30, 25);
        uint32_t bit11_8 = get_bits(state->ID.instr, 11, 8);
        
        uint32_t imm_val = (bit31 << 12) | (bit7 << 11) | (bit30_25 << 5) | (bit11_8 << 1);
        state->EX.imm = sign_extend(imm_val, 13);

        bool branch = ((diff == 0 && func3 == 0x0) || (diff != 0 && func3 == 0x1)); 
        
        if (branch) {
            state->IF.PC = state->ID.PC + (int32_t)state->EX.imm;
            state->ID.nop = true;
            state->EX.nop = true;
        } else {
            state->EX.nop = true;
        }
    }
    // S-Type
    else if (opcode == 0x23) { 
        int fwd1 = detect_hazard(rs1);
        int fwd2 = detect_hazard(rs2);
        if (state->ID.hazard_nop) { state->EX.nop = true; return; }

        state->EX.rs = rs1;
        state->EX.rt = rs2;
        state->EX.read_data_1 = read_data(rs1, fwd1);
        state->EX.read_data_2 = read_data(rs2, fwd2);
        
        uint32_t imm_val = (get_bits(state->ID.instr, 31, 25) << 5) | get_bits(state->ID.instr, 11, 7);
        state->EX.imm = sign_extend(imm_val, 12);
        
        state->EX.is_I_type = true;
        state->EX.write_mem = true;
        state->EX.alu_op = "00";
    }

    if (state->IF.nop) state->ID.nop = true;
}

// --- Execution ---
ExecutionStage::ExecutionStage(State_five* s) : state(s) {}

void ExecutionStage::run() {
    if (state->EX.nop) {
        if (!state->ID.nop) state->EX.nop = false;
        return;
    }

    uint32_t operand_1 = state->EX.read_data_1;
    uint32_t operand_2 = (!state->EX.is_I_type && !state->EX.write_mem) 
                         ? state->EX.read_data_2 
                         : state->EX.imm;

    int32_t op1_signed = (int32_t)operand_1;
    int32_t op2_signed = (int32_t)operand_2;
    int32_t result = 0;

    if (state->EX.alu_op == "00") result = op1_signed + op2_signed;
    else if (state->EX.alu_op == "01") result = op1_signed & op2_signed;
    else if (state->EX.alu_op == "10") result = op1_signed | op2_signed;
    else if (state->EX.alu_op == "11") result = op1_signed ^ op2_signed;

    state->MEM.alu_result = (uint32_t)result;
    state->MEM.rs = state->EX.rs;
    state->MEM.rt = state->EX.rt;
    state->MEM.read_mem = state->EX.read_mem;
    state->MEM.write_mem = state->EX.write_mem;
    
    if (state->EX.write_mem) state->MEM.store_data = state->EX.read_data_2;
    
    state->MEM.write_enable = state->EX.write_enable;
    state->MEM.write_reg_addr = state->EX.write_reg_addr;

    if (state->ID.nop) state->EX.nop = true;
}

// --- Memory Access ---
MemoryAccessStage::MemoryAccessStage(State_five* s, DataMem* dm) 
    : state(s), data_mem(dm) {}

void MemoryAccessStage::run() {
    if (state->MEM.nop) {
        if (!state->EX.nop) state->MEM.nop = false;
        return;
    }

    if (state->MEM.read_mem) {
        // INTERFACE ADAPTER
        bitset<32> addr(state->MEM.alu_result);
        bitset<32> data = data_mem->readDataMem(addr);
        state->WB.write_data = (uint32_t)data.to_ulong();
    } else if (state->MEM.write_mem) {
        // INTERFACE ADAPTER
        bitset<32> addr(state->MEM.alu_result);
        bitset<32> data(state->MEM.store_data);
        data_mem->writeDataMem(addr, data);
    } else {
        state->WB.write_data = state->MEM.alu_result;
        state->MEM.store_data = state->MEM.alu_result;
    }

    state->WB.write_enable = state->MEM.write_enable;
    state->WB.write_reg_addr = state->MEM.write_reg_addr;

    if (state->EX.nop) state->MEM.nop = true;
}

// --- Write Back ---
WriteBackStage::WriteBackStage(State_five* s, RegisterFile* r) 
    : state(s), rf(r) {}

void WriteBackStage::run() {
    if (state->WB.nop) {
        if (!state->MEM.nop) state->WB.nop = false;
        return;
    }

    if (state->WB.write_enable) {
        // INTERFACE ADAPTER
        rf->writeRF(bitset<5>(state->WB.write_reg_addr), bitset<32>(state->WB.write_data));
    }

    if (state->MEM.nop) state->WB.nop = true;
}

// ==========================================
// CORE CLASS IMPLEMENTATION
// ==========================================

FiveStageCore::FiveStageCore(string ioDir, InsMem& imem, DataMem& dmem)
    : ioDir(ioDir), 
      opFilePath(ioDir + "/StateResult_FS.txt"),
      ext_imem(&imem), 
      ext_dmem(&dmem),
      myRF(ioDir),
      if_stage(&state, &imem),
      id_stage(&state, &myRF),
      ex_stage(&state),
      mem_stage(&state, &dmem),
      wb_stage(&state, &myRF),
      cycle(0), num_instr(0), halted(false) {
          myRF.setFilePrefix("FS_");
      }

void FiveStageCore::step() {
    // Check if already halted (all stages were nop in previous cycle)
    bool was_all_nop = state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop;
    
    // Remember states before running stages
    bool id_was_nop = state.ID.nop;
    bool if_was_nop = state.IF.nop;
    uint32_t prev_id_instr = state.ID.instr;

    // Run stages in reverse order
    wb_stage.run();
    mem_stage.run();
    ex_stage.run();
    id_stage.run();
    if_stage.run();

    // Count instruction when:
    // 1. ID was nop but now has an instruction, OR
    // 2. ID was not nop and now has a different instruction
    // 3. IF fetched HALT (IF was not nop, but now both IF and ID are nop)
    if (!state.ID.nop) {
        if (id_was_nop || state.ID.instr != prev_id_instr) {
            num_instr++;
        }
    } else if (!if_was_nop && state.IF.nop && state.ID.nop) {
        // IF fetched HALT instruction, count it
        num_instr++;
    }

    myRF.outputRF(cycle);
    printState(state, cycle);

    cycle++;
    
    // Set halted if all stages were nop before this step
    if (was_all_nop) {
        halted = true;
    }
}

bool FiveStageCore::isHalted() const { 
    return halted; 
}



void FiveStageCore::printState(State_five state, int cycle) {
    ofstream printstate;
    if (cycle == 0)
        printstate.open(opFilePath, std::ios_base::trunc);
    else 
        printstate.open(opFilePath, std::ios_base::app);
    if (printstate.is_open()) {
        printstate<<"----------------------------------------------------------------------"<<endl;
        printstate<<"State after executing cycle: "<<cycle<<endl; 

        printstate<<"IF.nop: "<<(state.IF.nop ? "True" : "False")<<endl;
        printstate<<"IF.PC: "<<state.IF.PC<<endl;        

        printstate<<"ID.nop: "<<(state.ID.nop ? "True" : "False")<<endl;
        printstate<<"ID.Instr: "<<bitset<32>(state.ID.instr)<<endl; 

        printstate<<"EX.nop: "<<(state.EX.nop ? "True" : "False")<<endl;
        // Print EX.instr - empty if instr is 0, otherwise show the instruction
        if (state.EX.instr == 0) {
            printstate<<"EX.instr: "<<endl;
        } else {
            printstate<<"EX.instr: "<<bitset<32>(state.EX.instr)<<endl;
        }
        printstate<<"EX.Read_data1: "<<bitset<32>(state.EX.read_data_1)<<endl;
        printstate<<"EX.Read_data2: "<<bitset<32>(state.EX.read_data_2)<<endl;
        // Imm: 12 bits when EX has/had an instruction, 32 bits only for initial empty state
        if (state.EX.instr == 0) {
            printstate<<"EX.Imm: "<<bitset<32>(state.EX.imm)<<endl;  // 32 bits for initial empty state
        } else {
            printstate<<"EX.Imm: "<<bitset<12>(state.EX.imm & 0xFFF)<<endl;  // 12 bits when has instruction
        }
        printstate<<"EX.Rs: "<<bitset<5>(state.EX.rs)<<endl;
        printstate<<"EX.Rt: "<<bitset<5>(state.EX.rt)<<endl;
        printstate<<"EX.Wrt_reg_addr: "<<bitset<5>(state.EX.write_reg_addr)<<endl;
        printstate<<"EX.is_I_type: "<<(state.EX.is_I_type ? 1 : 0)<<endl; 
        printstate<<"EX.rd_mem: "<<(state.EX.read_mem ? 1 : 0)<<endl;
        printstate<<"EX.wrt_mem: "<<(state.EX.write_mem ? 1 : 0)<<endl;        
        printstate<<"EX.alu_op: "<<state.EX.alu_op<<endl;
        printstate<<"EX.wrt_enable: "<<(state.EX.write_enable ? 1 : 0)<<endl;

        printstate<<"MEM.nop: "<<(state.MEM.nop ? "True" : "False")<<endl;
        printstate<<"MEM.ALUresult: "<<bitset<32>(state.MEM.alu_result)<<endl;
        printstate<<"MEM.Store_data: "<<bitset<32>(state.MEM.store_data)<<endl; 
        printstate<<"MEM.Rs: "<<bitset<5>(state.MEM.rs)<<endl;
        printstate<<"MEM.Rt: "<<bitset<5>(state.MEM.rt)<<endl;   
        // MEM.Wrt_reg_addr: 6 bits if wrt_enable is 0, otherwise 5 bits
        if (state.MEM.write_reg_addr == 0 && !state.MEM.write_enable) {
            printstate<<"MEM.Wrt_reg_addr: "<<bitset<6>(0)<<endl;
        } else {
            printstate<<"MEM.Wrt_reg_addr: "<<bitset<5>(state.MEM.write_reg_addr)<<endl;
        }
        printstate<<"MEM.rd_mem: "<<(state.MEM.read_mem ? 1 : 0)<<endl;
        printstate<<"MEM.wrt_mem: "<<(state.MEM.write_mem ? 1 : 0)<<endl; 
        printstate<<"MEM.wrt_enable: "<<(state.MEM.write_enable ? 1 : 0)<<endl;         

        printstate<<"WB.nop: "<<(state.WB.nop ? "True" : "False")<<endl;
        printstate<<"WB.Wrt_data: "<<bitset<32>(state.WB.write_data)<<endl;
        printstate<<"WB.Rs: "<<bitset<5>(state.WB.rs)<<endl;
        printstate<<"WB.Rt: "<<bitset<5>(state.WB.rt)<<endl;
        printstate<<"WB.Wrt_reg_addr: "<<bitset<5>(state.WB.write_reg_addr)<<endl;
        printstate<<"WB.wrt_enable: "<<(state.WB.write_enable ? 1 : 0)<<endl;
    }
    else cout<<"Unable to open FS StateResult output file." << endl;
    printstate.close();
}

void FiveStageCore::setOutputDirectory(const string& outputDir) {
    ioDir = outputDir;
    opFilePath = outputDir + "/StateResult_FS.txt";
    myRF.outputFile = outputDir + "/FS_RFResult.txt";
}

void FiveStageCore::outputPerformanceMetrics(const string& outputDir) {
    string perfFile = outputDir + "/PerformanceMetrics.txt";
    ofstream perfOut(perfFile, ios::app);
    if (perfOut.is_open()) {
        perfOut << "Performance of Five Stage:" << endl;
        perfOut << "#Cycles -> " << cycle << endl;
        perfOut << "#Instructions -> " << num_instr << endl;
        if (num_instr > 0) {
            double cpi = (double)cycle / num_instr;
            double ipc = (double)num_instr / cycle;
            perfOut << "CPI -> " << fixed << setprecision(16) << cpi << endl;
            perfOut << "IPC -> " << fixed << setprecision(16) << ipc << endl;
        }
        perfOut << endl;
        perfOut.close();
    } else {
        cout << "Unable to open performance metrics file: " << perfFile << endl;
    }
}