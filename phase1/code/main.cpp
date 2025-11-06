// ==================== 系统头文件 ====================
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <map>
#include <filesystem>

using namespace std;

// ==================== 类声明 (来自头文件) ====================

// ---------- include/common.h ----------
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include <cstdint>


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


// ---------- include/insmem.h ----------
class InsMem
{
public:
    string id, ioDir;
    
    InsMem(string name, string ioDir);
    bitset<32> readInstr(bitset<32> ReadAddress);
    
    // Debug functions
    void debugPrintMemory(int start, int end);
    size_t debugGetMemorySize();
    bitset<8> debugGetMemoryByte(int index);
    
private:
    vector<bitset<8>> IMem;
    string getFileSeparator();
};


// ---------- include/datamem.h ----------
class DataMem    
{
public: 
    string id, opFilePath, ioDir;
    
    DataMem(string name, string ioDir);
    bitset<32> readDataMem(bitset<32> Address);
    void writeDataMem(bitset<32> Address, bitset<32> WriteData);
    void outputDataMem();
    void outputDataMem(string outputDir); 
    
    // Debug functions
    void debugPrintMemory(int start, int end);
    bitset<8> debugGetMemoryByte(int index);

private:
    vector<bitset<8>> DMem;
    string getFileSeparator();
};


// ---------- include/registerfile.h ----------
class RegisterFile
{
public:
    string outputFile;
    
    RegisterFile(string ioDir);
    bitset<32> readRF(bitset<5> Reg_addr);
    void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data);
    void outputRF(int cycle);
    void outputRF(int cycle, string outputDir); 
    void setFilePrefix(string prefix);  // Add method to set file prefix 
    
    // Debug functions
    void debugPrintRegisters();
    bitset<32> debugGetRegister(int index);
    void debugSetRegister(int index, bitset<32> value);

private:
    vector<bitset<32>> Registers;
    string filePrefix;  // Add file prefix member
};


// ---------- include/core.h ----------
class Core {
public:
    RegisterFile myRF;
    uint32_t cycle = 0;
    bool halted = false;
    string ioDir;
    struct stateStruct state, nextState;
    InsMem& ext_imem;
    DataMem& ext_dmem;
    
    Core(string ioDir, InsMem &imem, DataMem &dmem);
    virtual ~Core() = default;
    virtual void step() {}
    virtual void printState() {}
    virtual void setOutputDirectory(const string& outputDir);
    
protected:
    virtual string getStateOutputPath() const = 0;
    void printState(stateStruct state, int cycle);
};

class SingleStageCore : public Core {
public:
    SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step();
    void printState();
    void setOutputDirectory(const string& outputDir);

protected:
    string getStateOutputPath() const override { return opFilePath; }

private:
    stateStruct state, nextState;
    string opFilePath;
    int nopCycles = 0;
};

class FiveStageCore : public Core {
public:
    FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step();
    void printState();
    void setOutputDirectory(const string& outputDir);

protected:
    string getStateOutputPath() const override { return opFilePath; }

private:
    stateStruct state, nextState;
    string opFilePath;
};


// ==================== 函数实现 (来自源文件) ====================

// ---------- src/insmem.cpp ----------
InsMem::InsMem(string name, string ioDir) {       
    id = name;
    this->ioDir = ioDir;
    IMem.resize(MemSize);
    ifstream imem;
    string line;
    int i = 0;
    
    string filepath = ioDir + getFileSeparator() + "imem.txt";
    imem.open(filepath);
    
    if (imem.is_open()) {
        // 4 line x 8 bits = 32 bits instruction
        while (getline(imem, line)) {
            // Remove carriage return if present (for Windows line endings)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // Skip empty lines
            if (!line.empty()) {
                IMem[i] = bitset<8>(line);
                i++;
            }
        }                    
    }
    else {
        cout << "Unable to open IMEM input file: " << filepath << endl;
    }
    imem.close();                     
}

bitset<32> InsMem::readInstr(bitset<32> ReadAddress) {    
    // read instruction memory - big endian (imem.txt stores bytes in big-endian order)
    bitset<32> val;
    for (int i = 0; i < 4; i++) {
        bitset<32> byte_val = bitset<32>(IMem[ReadAddress.to_ulong() + i].to_ulong());
        val |= (byte_val << ((3 - i) * 8));  // Changed: 3-i for big-endian
    }
    return val;
}

void InsMem::debugPrintMemory(int start, int end) {
    cout << "Memory contents from " << start << " to " << end << ":" << endl;
    for (int i = start; i <= end && i < MemSize; i++) {
        cout << "IMem[" << i << "] = " << IMem[i] << " (0x" << hex << IMem[i].to_ulong() << dec << ")" << endl;
    }
}

size_t InsMem::debugGetMemorySize() {
    return IMem.size();
}

bitset<8> InsMem::debugGetMemoryByte(int index) {
    if (index >= 0 && index < MemSize) {
        return IMem[index];
    }
    return bitset<8>(0);
}

string InsMem::getFileSeparator() {
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}


// ---------- src/datamem.cpp ----------
DataMem::DataMem(string name, string ioDir) : id{name}, ioDir{ioDir} {
    DMem.resize(MemSize);
    opFilePath = ioDir + getFileSeparator() + name + "_DMEMResult.txt";
    ifstream dmem;
    string line;
    int i = 0;
    
    string filepath = ioDir + getFileSeparator() + "dmem.txt";
    dmem.open(filepath);
    
    if (dmem.is_open()) {
        while (getline(dmem, line)) {
            // Remove carriage return if present (for Windows line endings)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // Skip empty lines
            if (!line.empty()) {
                DMem[i] = bitset<8>(line);
                i++;
            }
        }
    }
    else {
        cout << "Unable to open DMEM input file: " << filepath << endl;
    }
    dmem.close();          
}

bitset<32> DataMem::readDataMem(bitset<32> Address) {	
    // read data memory - big endian (dmem.txt stores bytes in big-endian order)
    bitset<32> val;
    for (int i = 0; i < 4; i++) {
        bitset<32> byte_val = bitset<32>(DMem[Address.to_ulong() + i].to_ulong());
        val |= (byte_val << ((3 - i) * 8));  // Changed: 3-i for big-endian
    }
    return val;
}

void DataMem::writeDataMem(bitset<32> Address, bitset<32> WriteData) {
    // write into memory - big endian (dmem.txt stores bytes in big-endian order)
    uint32_t addr = Address.to_ulong();
    uint32_t data = WriteData.to_ulong();
    
    for (int i = 0; i < 4; i++) {
        DMem[addr + i] = bitset<8>((data >> ((3 - i) * 8)) & 0xFF);  // Changed: 3-i for big-endian
    }
}

void DataMem::outputDataMem() {
    ofstream dmemout;
    dmemout.open(opFilePath, std::ios_base::trunc);
    if (dmemout.is_open()) {
        for (int j = 0; j < 1000; j++) {     
            dmemout << DMem[j] << endl;
        }
    }
    else {
        cout << "Unable to open " << id << " DMEM result file." << endl;
    }
    dmemout.close();
}

void DataMem::outputDataMem(string outputDir) {
    // Create result directory if it doesn't exist
    string createDirCommand = "mkdir -p " + outputDir;
    system(createDirCommand.c_str());
    
    // Generate output file path in the specified directory
    string outputPath = outputDir + getFileSeparator() + id + "_DMEMResult.txt";
    
    ofstream dmemout;
    dmemout.open(outputPath, std::ios_base::trunc);
    if (dmemout.is_open()) {
        for (int j = 0; j < 1000; j++) {     
            dmemout << DMem[j] << endl;
        }
    }
    else {
        cout << "Unable to open " << id << " DMEM result file at: " << outputPath << endl;
    }
    dmemout.close();
}

void DataMem::debugPrintMemory(int start, int end) {
    cout << "Data Memory contents from " << start << " to " << end << ":" << endl;
    for (int i = start; i <= end && i < MemSize; i++) {
        cout << "DMem[" << i << "] = " << DMem[i] << " (0x" << hex << DMem[i].to_ulong() << dec << ")" << endl;
    }
}

bitset<8> DataMem::debugGetMemoryByte(int index) {
    if (index >= 0 && index < MemSize) {
        return DMem[index];
    }
    return bitset<8>(0);
}

string DataMem::getFileSeparator() {
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}


// ---------- src/registerfile.cpp ----------
RegisterFile::RegisterFile(string ioDir): outputFile {ioDir + "RFResult.txt"}, filePrefix("SS") {
    Registers.resize(32);  
    Registers[0] = bitset<32>(0);  
}

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

void RegisterFile::setFilePrefix(string prefix) {
    filePrefix = prefix;
}

void RegisterFile::outputRF(int cycle) {
    ofstream rfout;
    if (cycle == 0)
        rfout.open(outputFile, std::ios_base::trunc);
    else 
        rfout.open(outputFile, std::ios_base::app);
        
    if (rfout.is_open()) {
        rfout << "State of RF after executing cycle:  " << cycle << endl;
        for (int j = 0; j < 32; j++) {
            rfout << Registers[j] << endl;
        }
    }
    else {
        cout << "Unable to open RF output file." << endl;
    }
    rfout.close();               
}

void RegisterFile::outputRF(int cycle, string outputDir) {
    // Create directory if it doesn't exist
    string createDirCommand = "mkdir -p " + outputDir;
    system(createDirCommand.c_str());
    
    // Always use the correct filename with prefix
    string filename = filePrefix + "_RFResult.txt";
    
    // Generate new output path
    string outputPath = outputDir + "/" + filename;
    
    ofstream rfout;
    if (cycle == 0)
        rfout.open(outputPath, std::ios_base::trunc);
    else 
        rfout.open(outputPath, std::ios_base::app);
        
    if (rfout.is_open()) {
        rfout << "State of RF after executing cycle:  " << cycle << endl;
        for (int j = 0; j < 32; j++) {
            rfout << Registers[j] << endl;
        }
    }
    else {
        cout << "Unable to open RF output file: " << outputPath << endl;
    }
    rfout.close();               
}

void RegisterFile::debugPrintRegisters() {
    cout << "Register File contents:" << endl;
    for (int i = 0; i < 32; i++) {
        cout << "R" << i << ": " << Registers[i] << " (0x" << hex << Registers[i].to_ulong() << dec << ")" << endl;
    }
}

bitset<32> RegisterFile::debugGetRegister(int index) {
    if (index >= 0 && index < 32) {
        return Registers[index];
    }
    return bitset<32>(0);
}

void RegisterFile::debugSetRegister(int index, bitset<32> value) {
    if (index > 0 && index < 32) {  // Register 0 is always 0
        Registers[index] = value;
    }
}


// ---------- src/core.cpp ----------
// Standard headers

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
                // Update state to reflect halt condition for printing
                state = nextState;
                myRF.outputRF(cycle, ioDir);
                Core::printState(state, cycle);
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
    opFilePath = outputDir + "/StateResult_FS.txt";
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
            Core::printState(state, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
            state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
            cycle++;
        }

void FiveStageCore::printState() {
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


// ==================== Main 函数 (来自 sim.cpp) ====================

// Function to extract testcase name from path
string extractTestcaseName(const string& path) {
    // Find the last occurrence of '/' or '\'
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos) {
        string dirname = path.substr(lastSlash + 1);
        // Check if it's a testcase directory
        if (dirname.find("testcase") == 0) {
            return dirname;
        }
    }
    
    // Fallback: look for testcase in the path
    size_t pos = path.find("testcase");
    if (pos != string::npos) {
        // Extract testcase name (assume format like "testcase0", "testcase1", etc.)
        size_t start = pos;
        size_t end = start + 8; // "testcase" length
        while (end < path.length() && isdigit(path[end])) {
            end++;
        }
        return path.substr(start, end - start);
    }
    
    return "default"; // fallback name
}

int main(int argc, char* argv[]) {
	
	string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    }
    else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    }
    else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
    DataMem dmem_ss = DataMem("SS", ioDir);
	DataMem dmem_fs = DataMem("FS", ioDir);

    // Extract testcase name and create result subdirectory
    string testcaseName = extractTestcaseName(ioDir);
    string resultDir = "result/" + testcaseName;
    
    cout << "Testcase: " << testcaseName << endl;
    cout << "Result directory: " << resultDir << endl;

	SingleStageCore SSCore(ioDir, imem, dmem_ss);
	// FiveStageCore FSCore(ioDir, imem, dmem_fs);  // Disabled for now

    // Set output directory for single stage core
    SSCore.setOutputDirectory(resultDir);
    // FSCore.setOutputDirectory(resultDir);  // Disabled for now

    while (1) {
		if (!SSCore.halted)
			SSCore.step();
		
		// Disable five-stage core execution for now
		// if (!FSCore.halted)
		//	FSCore.step();

		if (SSCore.halted) // && FSCore.halted)
			break;
    }
    
	// dump SS data mem to result directory (FS disabled for now)
	dmem_ss.outputDataMem(resultDir);
	// dmem_fs.outputDataMem(resultDir);  // Disabled for now

	return 0;
}