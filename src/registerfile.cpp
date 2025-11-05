#include "../include/registerfile.h"

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