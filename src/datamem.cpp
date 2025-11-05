#include "../include/datamem.h"

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