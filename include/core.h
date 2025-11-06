#ifndef CORE_HPP
#define CORE_HPP

#include "common.h"
#include "insmem.h"
#include "datamem.h"
#include "registerfile.h"

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

class Core {
    public:
        RegisterFile myRF;
        uint32_t cycle = 0;
        bool halted = false;
        string ioDir;
        struct stateStruct state, nextState;
        InsMem& ext_imem; // reference 
        DataMem& ext_dmem;
        
        Core(string ioDir, InsMem &imem, DataMem &dmem);

        virtual void step() {}
        virtual void printState() {}
        virtual void setOutputDirectory(const string& outputDir);
};

class SingleStageCore : public Core {
    public:
        SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem);
        void setOutputDirectory(const string& outputDir) override;
        void step() override;
        void printState(stateStruct state, int cycle);
        
    private:
        string opFilePath;
};

class FiveStageCore : public Core {
    public:
        FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem);
        void setOutputDirectory(const string& outputDir) override;
        void step() override;
        void printState(stateStruct state, int cycle);
        
    private:
        string opFilePath;
};

#endif // CORE_HPP
