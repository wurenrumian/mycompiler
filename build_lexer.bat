@echo off
echo ========================================
echo Building Lexer (Homework 1)
echo ========================================

REM 创建构建目录
if not exist build mkdir build
cd build

REM 生成 Makefile
cmake -G "MinGW Makefiles" ..

if errorlevel 1 (
    echo CMake generation failed!
    pause
    exit /b 1
)

REM 编译 lexer 目标
mingw32-make lexer

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful! Lexer executable created.
echo ========================================
echo.

REM 运行测试
if exist ..\testfile.txt (
    echo Running lexer on testfile.txt...
    lexer ..\testfile.txt
    echo.
    echo === Output.txt content ===
    if exist output.txt (
        type output.txt
    ) else (
        echo output.txt not found!
    )
    echo === End of output ===
) else (
    echo testfile.txt not found!
)

pause
