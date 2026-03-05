// tests/test_symbol.cpp
// 测试 Symbol 结构和相关功能 - 使用 assert 的简单测试

#include <iostream>
#include <cassert>
#include "symtab/Symbol.h"

using namespace std;

void test_symbol_default_constructor() {
    Symbol sym("x", SymbolKind::Variable, DataType::Int);
    assert(sym.name == "x");
    assert(sym.kind == SymbolKind::Variable);
    assert(sym.type == DataType::Int);
    assert(sym.offset == 0);
    assert(sym.size == 0);
    assert(!sym.is_global);
    std::cout << "[PASS] Symbol default constructor\n";
}

void test_symbol_with_all_fields() {
    Symbol sym("global_var", SymbolKind::Variable, DataType::Float);
    sym.offset = 8;
    sym.size = 4;
    sym.is_global = true;

    assert(sym.name == "global_var");
    assert(sym.kind == SymbolKind::Variable);
    assert(sym.type == DataType::Float);
    assert(sym.offset == 8);
    assert(sym.size == 4);
    assert(sym.is_global);
    std::cout << "[PASS] Symbol with all fields\n";
}

void test_symbol_different_kinds() {
    Symbol var("a", SymbolKind::Variable, DataType::Int);
    Symbol func("main", SymbolKind::Function, DataType::Int);
    Symbol param("x", SymbolKind::Parameter, DataType::Float);
    Symbol constant("PI", SymbolKind::Constant, DataType::Float);

    assert(var.kind == SymbolKind::Variable);
    assert(func.kind == SymbolKind::Function);
    assert(param.kind == SymbolKind::Parameter);
    assert(constant.kind == SymbolKind::Constant);
    std::cout << "[PASS] Symbol different kinds\n";
}

void test_symbol_different_data_types() {
    Symbol int_sym("i", SymbolKind::Variable, DataType::Int);
    Symbol float_sym("f", SymbolKind::Variable, DataType::Float);
    Symbol void_sym("v", SymbolKind::Function, DataType::Void);
    Symbol unknown_sym("u", SymbolKind::Variable, DataType::Unknown);

    assert(int_sym.type == DataType::Int);
    assert(float_sym.type == DataType::Float);
    assert(void_sym.type == DataType::Void);
    assert(unknown_sym.type == DataType::Unknown);
    std::cout << "[PASS] Symbol different data types\n";
}

int main() {
    cout << "Running Symbol tests...\n\n";

    test_symbol_default_constructor();
    test_symbol_with_all_fields();
    test_symbol_different_kinds();
    test_symbol_different_data_types();

    cout << "\nAll tests passed!\n";
    return 0;
}
