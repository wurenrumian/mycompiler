@echo off
echo ========================================
echo Building Parser (Homework 2)
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

REM 编译 parser 目标
mingw32-make parser

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful! Parser executable created.
echo ========================================
echo.

REM 运行测试
if exist ..\testfile.txt (
    echo Running parser on testfile.txt...
    parser ..\testfile.txt
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
