#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include <cassert>
#include <filesystem>
#include "../include/insmem.h"

using namespace std;

// Create test imem.txt file
void createTestImemFile(const string& testDir) {
    filesystem::create_directory(testDir);
    ofstream imem_file(testDir + "/imem.txt");
    if (imem_file.is_open()) {
        // Create test instructions
        // Instruction 1: 0x83002000 (little endian storage)
        imem_file << "00000000" << endl;  // byte 0: 0x00
        imem_file << "00100000" << endl;  // byte 1: 0x20  
        imem_file << "00000000" << endl;  // byte 2: 0x00
        imem_file << "10000011" << endl;  // byte 3: 0x83
        
        // Instruction 2: 0x13010F00 (little endian storage)
        imem_file << "00000000" << endl;  // byte 4: 0x00
        imem_file << "00001111" << endl;  // byte 5: 0x0F
        imem_file << "00000001" << endl;  // byte 6: 0x01
        imem_file << "00010011" << endl;  // byte 7: 0x13
        
        // Instruction 3: 0x33021040 (little endian storage)
        imem_file << "01000000" << endl;  // byte 8: 0x40
        imem_file << "00010000" << endl;  // byte 9: 0x10
        imem_file << "00000010" << endl;  // byte 10: 0x02
        imem_file << "00110011" << endl;  // byte 11: 0x33
        
        // Instruction 4: 0x23204000 (little endian storage)
        imem_file << "00000000" << endl;  // byte 12: 0x00
        imem_file << "01000000" << endl;  // byte 13: 0x40
        imem_file << "00100000" << endl;  // byte 14: 0x20
        imem_file << "00100011" << endl;  // byte 15: 0x23
        
        // Fill with alternating 0s and 1s
        for (int i = 16; i < 32; i++) {
            if (i % 2 == 0) {
                imem_file << "00000000" << endl;
            } else {
                imem_file << "11111111" << endl;
            }
        }
        
        imem_file.close();
        cout << "Test imem.txt file created successfully." << endl;
    } else {
        cout << "Failed to create test imem.txt file." << endl;
    }
}

// Test function
void testInsMem() {
    cout << "=== InsMem Test Suite ===" << endl;
    
    // Create test directory
    string testDir = "test_data";
    
    // Create test file
    createTestImemFile(testDir);
    
    // Create InsMem object
    InsMem imem("TestImem", testDir);
    
    cout << "\n1. Testing memory loading..." << endl;
    imem.debugPrintMemory(0, 15);
    
    cout << "\n2. Testing instruction reading..." << endl;
    
    // Test reading first instruction (address 0)
    bitset<32> addr0(0);
    bitset<32> instr0 = imem.readInstr(addr0);
    cout << "Instruction at address 0: " << instr0 << " (0x" << hex << instr0.to_ulong() << dec << ")" << endl;
    
    // Test reading second instruction (address 4)
    bitset<32> addr4(4);
    bitset<32> instr4 = imem.readInstr(addr4);
    cout << "Instruction at address 4: " << instr4 << " (0x" << hex << instr4.to_ulong() << dec << ")" << endl;
    
    // Test reading third instruction (address 8)
    bitset<32> addr8(8);
    bitset<32> instr8 = imem.readInstr(addr8);
    cout << "Instruction at address 8: " << instr8 << " (0x" << hex << instr8.to_ulong() << dec << ")" << endl;
    
    // Test reading fourth instruction (address 12)
    bitset<32> addr12(12);
    bitset<32> instr12 = imem.readInstr(addr12);
    cout << "Instruction at address 12: " << instr12 << " (0x" << hex << instr12.to_ulong() << dec << ")" << endl;
    
    cout << "\n3. Testing expected values..." << endl;
    
    // Verify expected values
    uint32_t expected0 = 0x83002000;
    uint32_t expected4 = 0x13010F00;
    uint32_t expected8 = 0x33021040;
    uint32_t expected12 = 0x23204000;
    
    cout << "Expected vs Actual:" << endl;
    cout << "Addr 0:  Expected 0x" << hex << expected0 << ", Got 0x" << instr0.to_ulong() << dec;
    if (instr0.to_ulong() == expected0) cout << " ✓" << endl; else cout << " ✗" << endl;
    
    cout << "Addr 4:  Expected 0x" << hex << expected4 << ", Got 0x" << instr4.to_ulong() << dec;
    if (instr4.to_ulong() == expected4) cout << " ✓" << endl; else cout << " ✗" << endl;
    
    cout << "Addr 8:  Expected 0x" << hex << expected8 << ", Got 0x" << instr8.to_ulong() << dec;
    if (instr8.to_ulong() == expected8) cout << " ✓" << endl; else cout << " ✗" << endl;
    
    cout << "Addr 12: Expected 0x" << hex << expected12 << ", Got 0x" << instr12.to_ulong() << dec;
    if (instr12.to_ulong() == expected12) cout << " ✓" << endl; else cout << " ✗" << endl;
    
    cout << "\n4. Testing boundary conditions..." << endl;
    
    // Test boundary address
    bitset<32> addr16(16);
    bitset<32> instr16 = imem.readInstr(addr16);
    cout << "Instruction at address 16: " << instr16 << " (0x" << hex << instr16.to_ulong() << dec << ")" << endl;
    
    cout << "\n=== Test Complete ===" << endl;
}

int main() {
    try {
        testInsMem();
    } catch (const exception& e) {
        cout << "Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
