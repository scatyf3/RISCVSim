#!/bin/bash
# 简单编译脚本
echo "编译 RISC-V 模拟器..."
g++ -std=c++17 -Wall -Wextra -o simulator main.cpp
if [ $? -eq 0 ]; then
    echo "✅ 编译成功: simulator"
else
    echo "❌ 编译失败"
    exit 1
fi
