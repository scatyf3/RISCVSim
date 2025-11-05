#!/bin/bash

# Set the compiler and flags
CXX=g++
CXXFLAGS="-g -fsanitize=address,undefined -Wall -Wextra -std=c++17"

# Source file and output binary
SRC_FILE="sim.cpp"
OUTPUT="simulator"

# Build the project
$CXX $CXXFLAGS $SRC_FILE -o $OUTPUT

# Notify the user
if [ $? -eq 0 ]; then
    echo "Build successful. Run ./$OUTPUT to execute."
else
    echo "Build failed. Check the errors above."
fi