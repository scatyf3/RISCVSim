#ifndef INSMEM_H
#define INSMEM_H

#include "common.h"

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

#endif // INSMEM_H