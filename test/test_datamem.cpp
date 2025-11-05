#include <iostream>
#include <filesystem>
#include "../include/datamem.h"

using namespace std;

void createTestDmemFile(const string& testDir) {
    filesystem::create_directory(testDir);
    ofstream dmem_file(testDir + "/dmem.txt");
    if (dmem_file.is_open()) {
        // Create test data
        // Data 1: 0x12345678 (little endian storage)
        dmem_file << "01111000" << endl;  // byte 0: 0x78
        dmem_file << "01010110" << endl;  // byte 1: 0x56  
        dmem_file << "00110100" << endl;  // byte 2: 0x34
        dmem_file << "00010010" << endl;  // byte 3: 0x12
        
        // Data 2: 0xABCDEF00 (little endian storage)
        dmem_file << "00000000" << endl;  // byte 4: 0x00
        dmem_file << "11101111" << endl;  // byte 5: 0xEF
        dmem_file << "11001101" << endl;  // byte 6: 0xCD
        dmem_file << "10101011" << endl;  // byte 7: 0xAB
        
        // Fill with more data
        for (int i = 8; i < 32; i++) {
            dmem_file << "00000000" << endl;
        }
        
        dmem_file.close();
        cout << "Test dmem.txt file created successfully." << endl;
    } else {
        cout << "Failed to create test dmem.txt file." << endl;
    }
}

void testDataMem() {
    cout << "=== DataMem Test Suite ===" << endl;
    
    string testDir = "test_data";
    createTestDmemFile(testDir);
    
    DataMem dmem("TestDMem", testDir);
    
    cout << "\n1. Testing memory loading..." << endl;
    dmem.debugPrintMemory(0, 15);
    
    cout << "\n2. Testing data reading..." << endl;
    
    // Test reading data
    bitset<32> addr0(0);
    bitset<32> data0 = dmem.readDataMem(addr0);
    cout << "Data at address 0: " << data0 << " (0x" << hex << data0.to_ulong() << dec << ")" << endl;
    
    bitset<32> addr4(4);
    bitset<32> data4 = dmem.readDataMem(addr4);
    cout << "Data at address 4: " << data4 << " (0x" << hex << data4.to_ulong() << dec << ")" << endl;
    
    cout << "\n3. Testing data writing..." << endl;
    
    // Write new data
    bitset<32> write_addr(8);
    bitset<32> write_data(0xDEADBEEF);
    dmem.writeDataMem(write_addr, write_data);
    
    // Read written data
    bitset<32> read_data = dmem.readDataMem(write_addr);
    cout << "Written: 0x" << hex << write_data.to_ulong() << ", Read: 0x" << read_data.to_ulong() << dec << endl;
    
    if (read_data.to_ulong() == write_data.to_ulong()) {
        cout << "Write/Read test: ✓" << endl;
    } else {
        cout << "Write/Read test: ✗" << endl;
    }
    
    cout << "\n4. Memory after write:" << endl;
    dmem.debugPrintMemory(8, 11);
    
    cout << "\n=== DataMem Test Complete ===" << endl;
}

int main() {
    try {
        testDataMem();
    } catch (const exception& e) {
        cout << "Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}