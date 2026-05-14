cmake_minimum_required(VERSION 3.15)
project(CMMCompiler LANGUAGES CXX C)

enable_testing()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 针对 MSVC 的设置
if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 查找 bison
find_package(BISON REQUIRED)

# 生成语法分析器
BISON_TARGET(SysYParser src/sysy.y ${CMAKE_CURRENT_BINARY_DIR}/sysy.tab.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/sysy.tab.h)

# ============================================
# Homework 1: 独立词法分析器（不依赖 flex/bison）
# ============================================
add_executable(lexer
    src/lexer_main.cpp
    src/Token.cpp
    src/Lexer.cpp
)
target_include_directories(lexer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(lexer PRIVATE YY_NO_UNISTD_H)

set(COMPILER_COMMON_SOURCES
    src/Ast.cpp
    src/ParserFrontend.cpp
    src/Semantic.cpp
    src/Ir.cpp
    src/IrPrinter.cpp
    src/IrBuilder.cpp
    src/IrText.cpp
    src/RiscVBackend.cpp
    src/Codegen.cpp
    ${BISON_SysYParser_OUTPUTS}
    src/Token.cpp
    src/Lexer.cpp
)

# ============================================
# Homework 2/3: 语法分析+代码生成器
# ============================================
add_executable(parser
    src/parser_main.cpp
    ${COMPILER_COMMON_SOURCES}
)
target_include_directories(parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(parser PRIVATE YY_NO_UNISTD_H ENABLE_PARSER_OUTPUT=1)

add_executable(compiler
    src/compiler_main.cpp
    ${COMPILER_COMMON_SOURCES}
)
target_include_directories(compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(compiler PRIVATE YY_NO_UNISTD_H ENABLE_PARSER_OUTPUT=1)

# 链接 bison 库（如果找到）
if(BISON_FOUND)
    if(BISON_LIBRARIES)
        target_link_libraries(parser PRIVATE ${BISON_LIBRARIES})
        target_link_libraries(compiler PRIVATE ${BISON_LIBRARIES})
    endif()
endif()

# ============================================
# 单元测试
# ============================================

# Stream 测试
add_executable(test_stream
    tests/test_stream.cpp
)
target_include_directories(test_stream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(test_stream PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_stream COMMAND test_stream)

# Token 测试
add_executable(test_token
    tests/test_token.cpp
    src/Token.cpp
)
target_include_directories(test_token PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(test_token PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_token COMMAND test_token)

# Parser 测试
add_executable(test_parser
    tests/test_parser.cpp
    src/Ast.cpp
    src/ParserFrontend.cpp
    ${BISON_SysYParser_OUTPUTS}
    src/Token.cpp
    src/Lexer.cpp
)
target_include_directories(test_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(test_parser PRIVATE YY_NO_UNISTD_H ENABLE_PARSER_OUTPUT=0)
add_test(NAME test_parser COMMAND test_parser)

# Lexer 测试
add_executable(test_lexer
    tests/test_lexer.cpp
    src/Token.cpp
    src/Lexer.cpp
)
target_include_directories(test_lexer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(test_lexer PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_lexer COMMAND test_lexer)

# LLVM IR 生成集成测试（clang 不存在时会自动 skip）
add_executable(test_codegen
    tests/test_codegen.cpp
)
target_include_directories(test_codegen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(test_codegen PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_codegen COMMAND test_codegen)

add_executable(test_ir_generation
    tests/test_ir_generation.cpp
    src/Ir.cpp
    src/IrPrinter.cpp
    src/IrBuilder.cpp
    src/ParserFrontend.cpp
    ${BISON_SysYParser_OUTPUTS}
    src/Ast.cpp
    src/Token.cpp
    src/Lexer.cpp
)
target_include_directories(test_ir_generation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(test_ir_generation PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_ir_generation COMMAND test_ir_generation)
add_executable(test_ir_text
    tests/test_ir_text.cpp
    src/Ir.cpp
    src/IrText.cpp
)
target_include_directories(test_ir_text PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(test_ir_text PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_ir_text COMMAND test_ir_text)
add_executable(test_riscv_backend
    tests/test_riscv_backend.cpp
    src/Ir.cpp
    src/IrText.cpp
    src/RiscVBackend.cpp
)
target_include_directories(test_riscv_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(test_riscv_backend PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_riscv_backend COMMAND test_riscv_backend)
add_executable(test_ast_semantic
    tests/test_ast_semantic.cpp
    src/Ast.cpp
    src/ParserFrontend.cpp
    src/Semantic.cpp
    ${BISON_SysYParser_OUTPUTS}
    src/Token.cpp
    src/Lexer.cpp
)
target_include_directories(test_ast_semantic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_definitions(test_ast_semantic PRIVATE YY_NO_UNISTD_H)
add_test(NAME test_ast_semantic COMMAND test_ast_semantic)


# 运行所有测试的便捷目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_stream test_token test_lexer test_parser test_codegen test_ast_semantic test_ir_generation
    COMMENT "Running all unit tests..."
)
