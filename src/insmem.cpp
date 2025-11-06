#include "../include/insmem.h"

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