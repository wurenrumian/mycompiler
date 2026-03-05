cmake_minimum_required(VERSION 3.15)
project(CMMCompiler LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 针对 MSVC 的设置
if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 启用测试
enable_testing()

# 查找 flex/bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 手动设置 FLEX 头文件路径（MSYS2 MinGW 环境）
if(NOT FLEX_INCLUDE_DIR)
    # 尝试 mingw64 路径
    if(EXISTS "D:/msys64/mingw64/include/FlexLexer.h")
        set(FLEX_INCLUDE_DIR "D:/msys64/mingw64/include")
    else()
        # 回退到 usr 路径
        set(FLEX_INCLUDE_DIR "D:/msys64/usr/include")
    endif()
endif()

# 生成词法/语法分析器
FLEX_TARGET(CMMLexer src/cmm.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc
            COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/lex.yy.h")
BISON_TARGET(CMMParser src/cmm.y ${CMAKE_CURRENT_BINARY_DIR}/cmm.tab.cc
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/cmm.tab.h)

# 添加可执行文件
add_executable(cmm_compiler
    src/main.cpp
    ${FLEX_CMMLexer_OUTPUTS}
    ${BISON_CMMParser_OUTPUTS}
    src/symtab/SymbolTable.cpp
)

# 包含目录
target_include_directories(cmm_compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FLEX_INCLUDE_DIR}
)

# Windows 兼容性
if(MSVC)
    target_compile_definitions(cmm_compiler PRIVATE YY_NO_UNISTD_H)
endif()

# 链接 flex/bison 库（如果找到）
if(FLEX_FOUND AND BISON_FOUND)
    if(FLEX_LIBRARIES)
        target_link_libraries(cmm_compiler PRIVATE ${FLEX_LIBRARIES})
    endif()
    if(BISON_LIBRARIES)
        target_link_libraries(cmm_compiler PRIVATE ${BISON_LIBRARIES})
    endif()
endif()

# ============================================
# 测试目标
# ============================================

# 测试公共功能（common.h）
add_executable(test_common tests/test_common.cpp)
target_include_directories(test_common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(test_common PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_common COMMAND test_common)

# 测试 Symbol 结构
add_executable(test_symbol tests/test_symbol.cpp)
target_include_directories(test_symbol PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(test_symbol PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_symbol COMMAND test_symbol)

# 测试 SymbolTable（需要链接 SymbolTable.cpp）
add_executable(test_symbol_table
    tests/test_symbol_table.cpp
    src/symtab/SymbolTable.cpp
)
target_include_directories(test_symbol_table PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(test_symbol_table PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_symbol_table COMMAND test_symbol_table)

# 运行所有测试的便捷目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_common test_symbol test_symbol_table
    COMMENT "Running all tests..."
)

