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

# 查找 flex/bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

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
    ${FLEX_INCLUDE_DIRS}
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