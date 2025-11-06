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
    
    // Debug functions
    void debugPrintRegisters();
    bitset<32> debugGetRegister(int index);
    void debugSetRegister(int index, bitset<32> value);

private:
    vector<bitset<32>> Registers;
};


// ---------- include/core.h ----------
class Core {
public:
    RegisterFile myRF;
    uint32_t cycle = 0;
    bool halted = false;
    string ioDir;
    struct stateStruct state, nextState;
    InsMem ext_imem;
    DataMem ext_dmem;
    
    Core(string ioDir, InsMem &imem, DataMem &dmem);
    virtual ~Core() = default;
    virtual void step() {}
    virtual void printState() {}
    virtual void setOutputDirectory(const string& outputDir);
    
protected:
    string opFilePath;
    void printState(stateStruct state, int cycle);
};

class SingleStageCore : public Core {
public:
    SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step() override;
    void printState() override;
    void setOutputDirectory(const string& outputDir) override;
    
private:
    string opFilePath;
};

class FiveStageCore : public Core {
public:
    FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step() override;
    void printState() override;
    void setOutputDirectory(const string& outputDir) override;
    
private:
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
            IMem[i] = bitset<8>(line);
            i++;
        }                    
    }
    else {
        cout << "Unable to open IMEM input file: " << filepath << endl;
    }
    imem.close();                     
}

bitset<32> InsMem::readInstr(bitset<32> ReadAddress) {    
    // read instruction memory
    bitset<32> val;
    for (int i = 0; i < 4; i++) {
        bitset<32> byte_val = bitset<32>(IMem[ReadAddress.to_ulong() + i].to_ulong());
        val |= (byte_val << (i * 8));
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
            DMem[i] = bitset<8>(line);
            i++;
        }
    }
    else {
        cout << "Unable to open DMEM input file: " << filepath << endl;
    }
    dmem.close();          
}

bitset<32> DataMem::readDataMem(bitset<32> Address) {	
    // read data memory - little endian
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
RegisterFile::RegisterFile(string ioDir): outputFile {ioDir + "RFResult.txt"} {
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

void RegisterFile::outputRF(int cycle) {
    ofstream rfout;
    if (cycle == 0)
        rfout.open(outputFile, std::ios_base::trunc);
    else 
        rfout.open(outputFile, std::ios_base::app);
        
    if (rfout.is_open()) {
        rfout << "State of RF after executing cycle:\t" << cycle << endl;
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
    
    // Extract the original filename pattern from outputFile
    string filename;
    size_t lastSlash = outputFile.find_last_of("/\\");
    if (lastSlash != string::npos) {
        filename = outputFile.substr(lastSlash + 1);
    } else {
        filename = outputFile;
    }
    
    // Generate new output path
    string outputPath = outputDir + "/" + filename;
    
    ofstream rfout;
    if (cycle == 0)
        rfout.open(outputPath, std::ios_base::trunc);
    else 
        rfout.open(outputPath, std::ios_base::app);
        
    if (rfout.is_open()) {
        rfout << "State of RF after executing cycle:\t" << cycle << endl;
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
	FiveStageCore FSCore(ioDir, imem, dmem_fs);

    // Set output directory for both cores
    SSCore.setOutputDirectory(resultDir);
    FSCore.setOutputDirectory(resultDir);

    while (1) {
		if (!SSCore.halted)
			SSCore.step();
		
		if (!FSCore.halted)
			FSCore.step();

		if (SSCore.halted && FSCore.halted)
			break;
    }
    
	// dump SS and FS data mem to result directory.
	dmem_ss.outputDataMem(resultDir);
	dmem_fs.outputDataMem(resultDir);

	return 0;
}