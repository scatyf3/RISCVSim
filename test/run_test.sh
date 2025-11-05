#!/bin/bash

echo "=== Building RISC-V Simulator Tests ==="
cd /home/scatyf3/RISCVSim

# 编译源文件
echo "Compiling source files..."
g++ -std=c++17 -c src/insmem.cpp -o src/insmem.o
g++ -std=c++17 -c src/datamem.cpp -o src/datamem.o  
g++ -std=c++17 -c src/registerfile.cpp -o src/registerfile.o

if [ $? -ne 0 ]; then
    echo "Source compilation failed!"
    exit 1
fi

cd test

echo ""
echo "=== Running InsMem Test ==="
g++ -std=c++17 -o test_imem test_imem.cpp ../src/insmem.o
if [ $? -eq 0 ]; then
    ./test_imem
    echo ""
else
    echo "InsMem test build failed!"
fi

echo "=== Running DataMem Test ==="
g++ -std=c++17 -o test_datamem test_datamem.cpp ../src/datamem.o
if [ $? -eq 0 ]; then
    ./test_datamem
    echo ""
else
    echo "DataMem test build failed!"
fi

echo "=== Running RegisterFile Test ==="
g++ -std=c++17 -o test_registerfile test_registerfile.cpp ../src/registerfile.o
if [ $? -eq 0 ]; then
    ./test_registerfile
    echo ""
else
    echo "RegisterFile test build failed!"
fi

echo "=== All Tests Complete ==="