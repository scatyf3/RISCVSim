#ifndef CORE_H
#define CORE_H

#include "common.h"
#include "insmem.h"
#include "datamem.h"
#include "registerfile.h"

class Core {
public:
    RegisterFile myRF;
    uint32_t cycle = 0;
    uint32_t instruction_count = 0;
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
    virtual void outputPerformanceMetrics(const string& outputDir);
    
protected:
    virtual string getStateOutputPath() const = 0;
    void printState(stateStruct state, int cycle);
    virtual string getCoreType() const = 0;
};

class SingleStageCore : public Core {
public:
    SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step();
    void printState();
    void setOutputDirectory(const string& outputDir);

protected:
    string getStateOutputPath() const override { return opFilePath; }
    string getCoreType() const override { return "Single Stage"; }

private:
    stateStruct state, nextState;
    string opFilePath;
    int nopCycles = 0;
};

#include <string>
#include <vector>
#include <cstdint>

// ==========================================
// PIPELINE STATE STRUCTS
// ==========================================

struct InstructionFetchState {
    bool nop = false;
    uint32_t PC = 0;
};

struct InstructionDecodeState {
    bool nop = true;
    bool hazard_nop = false;
    uint32_t PC = 0;
    uint32_t instr = 0;
};

struct ExecutionState {
    bool nop = true;
    uint32_t instr = 0;
    uint32_t read_data_1 = 0;
    uint32_t read_data_2 = 0;
    uint32_t imm = 0;
    uint32_t rs = 0;
    uint32_t rt = 0;
    uint32_t write_reg_addr = 0;
    bool is_I_type = false;
    bool read_mem = false;
    bool write_mem = false;
    string alu_op = "00";
    bool write_enable = false;
};

struct MemoryAccessState {
    bool nop = true;
    uint32_t alu_result = 0;
    uint32_t store_data = 0;
    uint32_t rs = 0;
    uint32_t rt = 0;
    uint32_t write_reg_addr = 0;
    bool read_mem = false;
    bool write_mem = false;
    bool write_enable = false;
};

struct WriteBackState {
    bool nop = true;
    uint32_t write_data = 0;
    uint32_t rs = 0;
    uint32_t rt = 0;
    uint32_t write_reg_addr = 0;
    bool write_enable = false;
};

class State_five {
public:
    InstructionFetchState IF;
    InstructionDecodeState ID;
    ExecutionState EX;
    MemoryAccessState MEM;
    WriteBackState WB;
};

// ==========================================
// STAGE CLASSES
// ==========================================

class InstructionFetchStage {
    State_five* state;
    InsMem* ins_mem;
public:
    InstructionFetchStage(State_five* s, InsMem* im);
    void run();
};

class InstructionDecodeStage {
    State_five* state;
    RegisterFile* rf;
public:
    InstructionDecodeStage(State_five* s, RegisterFile* r);
    int detect_hazard(uint32_t rs);
    uint32_t read_data(uint32_t rs, int forward_signal);
    void run();
};

class ExecutionStage {
    State_five* state;
public:
    ExecutionStage(State_five* s);
    void run();
};

class MemoryAccessStage {
    State_five* state;
    DataMem* data_mem;
public:
    MemoryAccessStage(State_five* s, DataMem* dm);
    void run();
};

class WriteBackStage {
    State_five* state;
    RegisterFile* rf;
public:
    WriteBackStage(State_five* s, RegisterFile* r);
    void run();
};

// ==========================================
// CORE CLASS
// ==========================================

class FiveStageCore {
    State_five state;
    string ioDir;
    string opFilePath;
    
    InsMem* ext_imem;
    DataMem* ext_dmem;
    RegisterFile myRF;

    InstructionFetchStage if_stage;
    InstructionDecodeStage id_stage;
    ExecutionStage ex_stage;
    MemoryAccessStage mem_stage;
    WriteBackStage wb_stage;

    int cycle;
    int num_instr;

public:
    bool halted;
    
    FiveStageCore(string ioDir, InsMem& imem, DataMem& dmem);
    void step();
    bool isHalted() const;
    void printState(State_five state, int cycle);
    void setOutputDirectory(const string& outputDir);
    void outputPerformanceMetrics(const string& outputDir);
};
#endif // CORE_H
