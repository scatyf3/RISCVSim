#include "../include/core.h"

Core::Core(string ioDir, InsMem &imem, DataMem &dmem): myRF(ioDir), ioDir{ioDir}, ext_imem {imem}, ext_dmem {dmem} {}

void Core::setOutputDirectory(const string& outputDir) {
    if (outputDir.empty()) return;
    ioDir = outputDir;
    std::error_code ec;
    std::filesystem::create_directories(ioDir, ec); 
}

SingleStageCore::SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir, imem, dmem), opFilePath(ioDir + "/StateResult_SS.txt") {
    // Initialize state
    state.IF.PC = 0;
    state.IF.nop = false;
    nextState = state;
    
    // Set RegisterFile prefix for single stage
    myRF.setFilePrefix("SS");
}


void SingleStageCore::setOutputDirectory(const string& outputDir) {
    Core::setOutputDirectory(outputDir);
    opFilePath = ioDir + "/StateResult_SS.txt";
}

void SingleStageCore::step() {
            // Initialize next state
            nextState = state;
            
            if (state.IF.nop) {
				// testcase0 StateResult has 7 cycles before halting
                myRF.outputRF(cycle, ioDir);
                printState(state, cycle);  // Use current state, not nextState
                halted = true;
                cycle++;
                return;
            }            
			// Fetch instruction
            bitset<32> instruction = ext_imem.readInstr(state.IF.PC);
            
            // Check for HALT instruction (all 1s)
            if (instruction.to_ulong() == 0xFFFFFFFF) {
                nextState.IF.nop = true;
                // Don't set halted = true here, let it be handled in the next cycle
                myRF.outputRF(cycle, ioDir);
                printState(nextState, cycle);
                state = nextState;
                cycle++;
                return;
            }
            
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
                    break;
                }
                case 0x03: { // Load (LW)
                    write_enable = true;
                    int32_t imm = static_cast<int32_t>(instr) >> 20; // Sign extend
                    uint32_t address = rs1_val.to_ulong() + imm;
                    write_data = ext_dmem.readDataMem(bitset<32>(address));
                    break;
                }
                case 0x23: { // Store (SW)
                    int32_t imm = ((instr >> 7) & 0x1F) | (((instr >> 25) & 0x7F) << 5);
                    if (imm & 0x800) imm |= 0xFFFFF000; // Sign extend
                    uint32_t address = rs1_val.to_ulong() + imm;
                    ext_dmem.writeDataMem(bitset<32>(address), rs2_val);
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
            
            // Update PC for non-branch/jump instructions
            if (opcode != 0x63 && opcode != 0x6F && opcode != 0x67) {
                nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
            }
            
            myRF.outputRF(cycle, ioDir); // dump RF
            printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
            
            state = nextState; // The end of the cycle and updates the current state with the values calculated in this cycle
            cycle++;
        }

void SingleStageCore::printState(stateStruct state, int cycle) {
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

FiveStageCore::FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir, imem, dmem), opFilePath(ioDir + "/StateResult_FS.txt") {
    // Initialize pipeline state - all stages start as NOP except IF
    state.IF.PC = 0;
    state.IF.nop = false;
    state.ID.nop = true;
    state.ID.Instr = 0;
    state.EX.nop = true;
    state.MEM.nop = true;
    state.WB.nop = true;
    nextState = state;
    
    // Set RegisterFile prefix for five stage
    myRF.setFilePrefix("FS");
}

void FiveStageCore::setOutputDirectory(const string& outputDir) {
    Core::setOutputDirectory(outputDir);
    opFilePath = ioDir + "/StateResult_FS.txt";
}

void FiveStageCore::step() {
            /* Your implementation */
            /* --------------------- WB stage --------------------- */
            if (!state.WB.nop && state.WB.wrt_enable && state.WB.Wrt_reg_addr.to_ulong() != 0) {
                myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
            }
            
            /* --------------------- MEM stage -------------------- */
            nextState.WB.nop = state.MEM.nop;
            if (!state.MEM.nop) {
                nextState.WB.Rs = state.MEM.Rs;
                nextState.WB.Rt = state.MEM.Rt;
                nextState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                nextState.WB.wrt_enable = state.MEM.wrt_enable;
                
                if (state.MEM.rd_mem) {
                    // Load instruction
                    nextState.WB.Wrt_data = ext_dmem.readDataMem(state.MEM.ALUresult);
                } else {
                    // ALU result
                    nextState.WB.Wrt_data = state.MEM.ALUresult;
                }
                
                if (state.MEM.wrt_mem) {
                    // Store instruction
                    ext_dmem.writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
                }
            } else {
                nextState.WB.Rs = 0;
                nextState.WB.Rt = 0;
                nextState.WB.Wrt_reg_addr = 0;
                nextState.WB.Wrt_data = 0;
                nextState.WB.wrt_enable = false;
            }
            
            /* --------------------- EX stage --------------------- */
            nextState.MEM.nop = state.EX.nop;
            if (!state.EX.nop) {
                nextState.MEM.Rs = state.EX.Rs;
                nextState.MEM.Rt = state.EX.Rt;
                nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
                nextState.MEM.rd_mem = state.EX.rd_mem;
                nextState.MEM.wrt_mem = state.EX.wrt_mem;
                nextState.MEM.wrt_enable = state.EX.wrt_enable;
                nextState.MEM.Store_data = state.EX.Read_data2;
                
                // ALU operation
                uint32_t opcode = state.EX.opcode.to_ulong();
                uint32_t funct3 = state.EX.funct3.to_ulong();
                uint32_t funct7 = state.EX.funct7.to_ulong();
                
                bitset<32> alu_result;
                if (opcode == 0x33) { // R-type
                    if (funct3 == 0 && funct7 == 0) { // ADD
                        alu_result = bitset<32>(state.EX.Read_data1.to_ulong() + state.EX.Read_data2.to_ulong());
                    } else if (funct3 == 0 && funct7 == 0x20) { // SUB
                        alu_result = bitset<32>(state.EX.Read_data1.to_ulong() - state.EX.Read_data2.to_ulong());
                    } else if (funct3 == 4) { // XOR
                        alu_result = state.EX.Read_data1 ^ state.EX.Read_data2;
                    } else if (funct3 == 6) { // OR
                        alu_result = state.EX.Read_data1 | state.EX.Read_data2;
                    } else if (funct3 == 7) { // AND
                        alu_result = state.EX.Read_data1 & state.EX.Read_data2;
                    }
                } else if (opcode == 0x13) { // I-type arithmetic
                    if (funct3 == 0) { // ADDI
                        alu_result = bitset<32>(state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong());
                    } else if (funct3 == 4) { // XORI
                        alu_result = state.EX.Read_data1 ^ state.EX.Imm;
                    } else if (funct3 == 6) { // ORI
                        alu_result = state.EX.Read_data1 | state.EX.Imm;
                    } else if (funct3 == 7) { // ANDI
                        alu_result = state.EX.Read_data1 & state.EX.Imm;
                    }
                } else if (opcode == 0x03 || opcode == 0x23) { // Load/Store
                    alu_result = bitset<32>(state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong());
                } else if (opcode == 0x63) { // Branch
                    // Branch target calculation
                    alu_result = bitset<32>(state.EX.PC.to_ulong() + state.EX.Imm.to_ulong());
                    // Branch condition check
                    bool take_branch = false;
                    if (funct3 == 0) { // BEQ
                        take_branch = (state.EX.Read_data1 == state.EX.Read_data2);
                    } else if (funct3 == 1) { // BNE
                        take_branch = (state.EX.Read_data1 != state.EX.Read_data2);
                    }
                    nextState.MEM.branch_taken = take_branch;
                    nextState.MEM.branch_target = alu_result;
                } else if (opcode == 0x6F || opcode == 0x67) { // JAL/JALR
                    if (opcode == 0x6F) { // JAL
                        alu_result = bitset<32>(state.EX.PC.to_ulong() + state.EX.Imm.to_ulong());
                    } else { // JALR
                        alu_result = bitset<32>((state.EX.Read_data1.to_ulong() + state.EX.Imm.to_ulong()) & ~1);
                    }
                    nextState.MEM.is_jump = true;
                    nextState.MEM.branch_target = alu_result;
                }
                
                nextState.MEM.ALUresult = alu_result;
            } else {
                nextState.MEM.Rs = 0;
                nextState.MEM.Rt = 0;
                nextState.MEM.Wrt_reg_addr = 0;
                nextState.MEM.ALUresult = 0;
                nextState.MEM.Store_data = 0;
                nextState.MEM.rd_mem = false;
                nextState.MEM.wrt_mem = false;
                nextState.MEM.wrt_enable = false;
                nextState.MEM.branch_taken = false;
                nextState.MEM.is_jump = false;
                nextState.MEM.branch_target = 0;
            }
            
            /* --------------------- ID stage --------------------- */
            nextState.EX.nop = state.ID.nop;
            if (!state.ID.nop) {
                uint32_t instr = static_cast<uint32_t>(state.ID.Instr.to_ulong());
                uint32_t opcode = instr & 0x7F;
                uint32_t rd = (instr >> 7) & 0x1F;
                uint32_t funct3 = (instr >> 12) & 0x7;
                uint32_t rs1 = (instr >> 15) & 0x1F;
                uint32_t rs2 = (instr >> 20) & 0x1F;
                uint32_t funct7 = (instr >> 25) & 0x7F;
                
                nextState.EX.opcode = bitset<7>(opcode);
                nextState.EX.funct3 = bitset<3>(funct3);
                nextState.EX.funct7 = bitset<7>(funct7);
                nextState.EX.Rs = bitset<5>(rs1);
                nextState.EX.Rt = bitset<5>(rs2);
                nextState.EX.Wrt_reg_addr = bitset<5>(rd);
                nextState.EX.PC = state.IF.PC; // Current PC for branch calculations
                
                // Read register file
                nextState.EX.Read_data1 = myRF.readRF(bitset<5>(rs1));
                nextState.EX.Read_data2 = myRF.readRF(bitset<5>(rs2));
                
                // Immediate generation
                int32_t imm = 0;
                if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) { // I-type
                    imm = static_cast<int32_t>(instr) >> 20;
                } else if (opcode == 0x23) { // S-type
                    imm = ((instr >> 7) & 0x1F) | (((instr >> 25) & 0x7F) << 5);
                    if (imm & 0x800) imm |= 0xFFFFF000;
                } else if (opcode == 0x63) { // B-type
                    imm = ((instr >> 7) & 0x1) << 11 |
                          ((instr >> 8) & 0xF) << 1 |
                          ((instr >> 25) & 0x3F) << 5 |
                          ((instr >> 31) & 0x1) << 12;
                    if (imm & 0x1000) imm |= 0xFFFFE000;
                } else if (opcode == 0x6F) { // J-type
                    imm = ((instr >> 21) & 0x3FF) << 1 |
                          ((instr >> 20) & 0x1) << 11 |
                          ((instr >> 12) & 0xFF) << 12 |
                          ((instr >> 31) & 0x1) << 20;
                    if (imm & 0x100000) imm |= 0xFFE00000;
                }
                nextState.EX.Imm = bitset<32>(static_cast<uint32_t>(imm));
                
                // Control signals
                nextState.EX.is_I_type = (opcode == 0x13 || opcode == 0x03 || opcode == 0x67);
                nextState.EX.rd_mem = (opcode == 0x03);
                nextState.EX.wrt_mem = (opcode == 0x23);
                nextState.EX.wrt_enable = (opcode == 0x33 || opcode == 0x13 || opcode == 0x03 || 
                                          opcode == 0x6F || opcode == 0x67);
                nextState.EX.is_branch = (opcode == 0x63);
                nextState.EX.is_jump = (opcode == 0x6F || opcode == 0x67);
                nextState.EX.alu_op = true; // Simplified
            } else {
                nextState.EX.Read_data1 = 0;
                nextState.EX.Read_data2 = 0;
                nextState.EX.Imm = 0;
                nextState.EX.Rs = 0;
                nextState.EX.Rt = 0;
                nextState.EX.Wrt_reg_addr = 0;
                nextState.EX.funct3 = 0;
                nextState.EX.funct7 = 0;
                nextState.EX.opcode = 0;
                nextState.EX.PC = 0;
                nextState.EX.is_I_type = false;
                nextState.EX.rd_mem = false;
                nextState.EX.wrt_mem = false;
                nextState.EX.alu_op = false;
                nextState.EX.wrt_enable = false;
                nextState.EX.is_branch = false;
                nextState.EX.is_jump = false;
            }
            
            /* --------------------- IF stage --------------------- */
            nextState.ID.nop = state.IF.nop;
            if (!state.IF.nop) {
                bitset<32> instruction = ext_imem.readInstr(state.IF.PC);
                nextState.ID.Instr = instruction;
                
                // Check for HALT
                if (instruction.to_ulong() == 0xFFFFFFFF) {
                    nextState.IF.nop = true;
                    nextState.ID.nop = true;
                } else {
                    // Handle branch/jump from MEM stage
                    if (state.MEM.branch_taken || state.MEM.is_jump) {
                        nextState.IF.PC = state.MEM.branch_target;
                        // Flush ID stage
                        nextState.ID.nop = true;
                        nextState.ID.Instr = 0;
                    } else {
                        nextState.IF.PC = bitset<32>(state.IF.PC.to_ulong() + 4);
                    }
                }
            } else {
                nextState.ID.Instr = 0;
                nextState.IF.PC = state.IF.PC;
            }
            
            // Check if all stages are NOP
            if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
                halted = true;
        
            myRF.outputRF(cycle, ioDir); // dump RF
            printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
            state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
            cycle++;
        }

void FiveStageCore::printState(stateStruct state, int cycle) {
    ofstream printstate;
    if (cycle == 0)
        printstate.open(opFilePath, std::ios_base::trunc);
    else 
        printstate.open(opFilePath, std::ios_base::app);
    if (printstate.is_open()) {
        printstate<<"----------------------------------------------------------------------"<<endl;
        printstate<<"State after executing cycle: "<<cycle<<endl; 

        printstate<<"IF.nop: "<<(state.IF.nop ? "True" : "False")<<endl;
        printstate<<"IF.PC: "<<state.IF.PC.to_ulong()<<endl;        

        printstate<<"ID.nop: "<<(state.ID.nop ? "True" : "False")<<endl;
        printstate<<"ID.Instr: "<<state.ID.Instr<<endl; 

        printstate<<"EX.nop: "<<(state.EX.nop ? "True" : "False")<<endl;
        printstate<<"EX.instr: "<<endl;  // This field seems empty in expected output
        printstate<<"EX.Read_data1: "<<state.EX.Read_data1<<endl;
        printstate<<"EX.Read_data2: "<<state.EX.Read_data2<<endl;
        printstate<<"EX.Imm: "<<state.EX.Imm<<endl; 
        printstate<<"EX.Rs: "<<state.EX.Rs<<endl;
        printstate<<"EX.Rt: "<<state.EX.Rt<<endl;
        printstate<<"EX.Wrt_reg_addr: "<<state.EX.Wrt_reg_addr<<endl;
        printstate<<"EX.is_I_type: "<<state.EX.is_I_type<<endl; 
        printstate<<"EX.rd_mem: "<<state.EX.rd_mem<<endl;
        printstate<<"EX.wrt_mem: "<<state.EX.wrt_mem<<endl;        
        printstate<<"EX.alu_op: "<<state.EX.alu_op<<endl;
        printstate<<"EX.wrt_enable: "<<state.EX.wrt_enable<<endl;

        printstate<<"MEM.nop: "<<(state.MEM.nop ? "True" : "False")<<endl;
        printstate<<"MEM.instr: "<<endl;  // This field seems empty in expected output
        printstate<<"MEM.ALUresult: "<<state.MEM.ALUresult<<endl;
        printstate<<"MEM.Store_data: "<<state.MEM.Store_data<<endl; 
        printstate<<"MEM.Rs: "<<state.MEM.Rs<<endl;
        printstate<<"MEM.Rt: "<<state.MEM.Rt<<endl;   
        printstate<<"MEM.Wrt_reg_addr: "<<state.MEM.Wrt_reg_addr<<endl;              
        printstate<<"MEM.rd_mem: "<<state.MEM.rd_mem<<endl;
        printstate<<"MEM.wrt_mem: "<<state.MEM.wrt_mem<<endl; 
        printstate<<"MEM.wrt_enable: "<<state.MEM.wrt_enable<<endl;         

        printstate<<"WB.nop: "<<(state.WB.nop ? "True" : "False")<<endl;
        printstate<<"WB.instr: "<<endl;  // This field seems empty in expected output
        printstate<<"WB.Wrt_data: "<<state.WB.Wrt_data<<endl;
        printstate<<"WB.Rs: "<<state.WB.Rs<<endl;
        printstate<<"WB.Rt: "<<state.WB.Rt<<endl;
        printstate<<"WB.Wrt_reg_addr: "<<state.WB.Wrt_reg_addr<<endl;
        printstate<<"WB.wrt_enable: "<<state.WB.wrt_enable<<endl;
    }
    else cout<<"Unable to open FS StateResult output file." << endl;
    printstate.close();
}