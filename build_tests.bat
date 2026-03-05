@echo off
echo ========================================
echo Building and Running Unit Tests
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

REM 编译所有测试
mingw32-make test_stream test_token test_lexer

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Tests built successfully!
echo ========================================
echo.

REM 运行所有测试
echo Running all unit tests...
echo.

ctest --output-on-failure

if errorlevel 1 (
    echo.
    echo Some tests failed!
) else (
    echo.
    echo All tests passed!
)

echo.
pause
