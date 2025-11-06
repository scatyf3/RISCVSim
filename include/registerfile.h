#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "common.h"

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

#endif // REGISTERFILE_H