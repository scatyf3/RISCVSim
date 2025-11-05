#include <iostream>
#include "../include/registerfile.h"

using namespace std;

void testRegisterFile() {
    cout << "=== RegisterFile Test Suite ===" << endl;
    
    RegisterFile rf("./");
    
    cout << "\n1. Testing initial state..." << endl;
    cout << "Register 0 should always be 0: " << rf.debugGetRegister(0) << endl;
    cout << "Register 1 initial value: " << rf.debugGetRegister(1) << endl;
    
    cout << "\n2. Testing register write/read..." << endl;
    
    // Test write and read operations
    bitset<5> reg_addr_1(1);
    bitset<32> test_value_1(0x12345678);
    rf.writeRF(reg_addr_1, test_value_1);
    
    bitset<32> read_value_1 = rf.readRF(reg_addr_1);
    cout << "Written to R1: 0x" << hex << test_value_1.to_ulong() << endl;
    cout << "Read from R1:  0x" << hex << read_value_1.to_ulong() << dec << endl;
    
    if (read_value_1.to_ulong() == test_value_1.to_ulong()) {
        cout << "R1 Write/Read test: ✓" << endl;
    } else {
        cout << "R1 Write/Read test: ✗" << endl;
    }
    
    cout << "\n3. Testing register 0 protection..." << endl;
    
    // Try to write to register 0 (should fail)
    bitset<5> reg_addr_0(0);
    bitset<32> test_value_0(0xDEADBEEF);
    rf.writeRF(reg_addr_0, test_value_0);
    
    bitset<32> read_value_0 = rf.readRF(reg_addr_0);
    cout << "Attempted to write 0x" << hex << test_value_0.to_ulong() << " to R0" << endl;
    cout << "R0 actual value: 0x" << hex << read_value_0.to_ulong() << dec << endl;
    
    if (read_value_0.to_ulong() == 0) {
        cout << "R0 protection test: ✓" << endl;
    } else {
        cout << "R0 protection test: ✗" << endl;
    }
    
    cout << "\n4. Testing multiple registers..." << endl;
    
    // Write to multiple registers
    for (int i = 2; i <= 5; i++) {
        bitset<5> addr(i);
        bitset<32> value(i * 0x1000 + i);
        rf.writeRF(addr, value);
        
        bitset<32> read_val = rf.readRF(addr);
        cout << "R" << i << ": 0x" << hex << read_val.to_ulong() << dec << endl;
    }
    
    cout << "\n5. Testing boundary conditions..." << endl;
    
    // Test boundary register
    bitset<5> reg_addr_31(31);
    bitset<32> test_value_31(0xFFFFFFFF);
    rf.writeRF(reg_addr_31, test_value_31);
    
    bitset<32> read_value_31 = rf.readRF(reg_addr_31);
    cout << "R31: 0x" << hex << read_value_31.to_ulong() << dec << endl;
    
    cout << "\n=== RegisterFile Test Complete ===" << endl;
}

int main() {
    try {
        testRegisterFile();
    } catch (const exception& e) {
        cout << "Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}