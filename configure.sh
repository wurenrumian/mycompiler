#!/bin/bash

# 环境检测脚本：检查 flex/bison/g++ 版本

echo "=== C-- 编译器环境检测 ==="
echo ""

# 检测 g++
echo -n "检测 g++: "
if command -v g++ &> /dev/null; then
    g++_version=$(g++ --version | head -n1)
    echo "✓ $g++_version"
else
    echo "✗ 未找到 g++，请安装"
    echo "  Ubuntu/Debian: sudo apt install g++"
    echo "  macOS: brew install gcc"
    echo "  Windows MSYS2: pacman -S mingw-w64-x86_64-gcc"
    exit 1
fi

# 检测 flex
echo -n "检测 flex: "
if command -v flex &> /dev/null; then
    flex_version=$(flex --version | head -n1)
    echo "✓ $flex_version"
else
    echo "✗ 未找到 flex，请安装"
    echo "  Ubuntu/Debian: sudo apt install flex"
    echo "  macOS: brew install flex"
    echo "  Windows MSYS2: pacman -S flex"
    exit 1
fi

# 检测 bison
echo -n "检测 bison: "
if command -v bison &> /dev/null; then
    bison_version=$(bison --version | head -n1)
    echo "✓ $bison_version"
else
    echo "✗ 未找到 bison，请安装"
    echo "  Ubuntu/Debian: sudo apt install bison"
    echo "  macOS: brew install bison"
    echo "  Windows MSYS2: pacman -S bison"
    exit 1
fi

# 检测 CMake
echo -n "检测 CMake: "
if command -v cmake &> /dev/null; then
    cmake_version=$(cmake --version | head -n1)
    echo "✓ $cmake_version"
else
    echo "⚠ CMake 未安装，将使用 Makefile 构建"
fi

# 检测操作系统
echo ""
echo "操作系统检测:"
if [[ "$OSTYPE" == "msys"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ "$OSTYPE" == "win32"* ]]; then
    echo "  检测到 Windows 环境 (MSYS2/Cygwin)"
    echo "  推荐使用 MinGW Makefiles 生成器: cmake .. -G \"MinGW Makefiles\""
else
    echo "  检测到 Unix-like 环境"
    echo "  使用默认生成器即可"
fi

echo ""
echo "环境检测完成！"
echo "构建方式："
echo "  1. 使用 CMake: mkdir build && cd build && cmake .. && make"
echo "  2. 使用 Makefile: make"
echo "  3. Windows 批处理: build_simple.bat 或 build_msys2.bat"