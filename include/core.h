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

#endif // CORE_H