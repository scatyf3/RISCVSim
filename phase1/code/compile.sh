#!/bin/bash
# Simple compilation script
echo "Compiling RISC-V Simulator..."
g++ -std=c++17 -Wall -Wextra -o simulator main.cpp
if [ $? -eq 0 ]; then
    echo "✅ Compilation successful: simulator"
else
    echo "❌ Compilation failed"
    exit 1
fi
