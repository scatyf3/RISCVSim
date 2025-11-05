#ifndef DATAMEM_H
#define DATAMEM_H

#include "common.h"

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

#endif // DATAMEM_H