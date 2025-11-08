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

class FiveStageCore : public Core {
public:
    FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem);
    void step();
    void printState();
    void setOutputDirectory(const string& outputDir);

protected:
    string getStateOutputPath() const override { return opFilePath; }
    string getCoreType() const override { return "Five Stage"; }

private:
    stateStruct state, nextState;
    string opFilePath;
};

#endif // CORE_H
