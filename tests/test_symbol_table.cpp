// tests/test_symbol_table.cpp
// 测试 SymbolTable 的符号表管理功能 - 使用 assert 的简单测试

#include <iostream>
#include <cassert>
#include "symtab/SymbolTable.h"
#include "symtab/Symbol.h"

using namespace std;

void test_default_construction() {
    SymbolTable table;
    assert(table.parent() == nullptr);
    assert(table.current_offset() == 0);
    cout << "[PASS] Default construction\n";
}

void test_construction_with_parent() {
    SymbolTable parent;
    SymbolTable child(&parent);
    assert(child.parent() == &parent);
    assert(child.current_offset() == 0);
    cout << "[PASS] Construction with parent\n";
}

void test_insert_variable() {
    SymbolTable table;
    auto sym = table.create_variable("x", DataType::Int, 4);
    assert(sym != nullptr);
    assert(sym->name == "x");
    assert(sym->kind == SymbolKind::Variable);
    assert(sym->type == DataType::Int);
    assert(sym->size == 4);
    cout << "[PASS] Insert variable\n";
}

void test_insert_function() {
    SymbolTable table;
    auto sym = table.create_function("main", DataType::Int);
    assert(sym != nullptr);
    assert(sym->name == "main");
    assert(sym->kind == SymbolKind::Function);
    assert(sym->type == DataType::Int);
    assert(sym->size == 0);
    cout << "[PASS] Insert function\n";
}

void test_insert_parameter() {
    SymbolTable table;
    auto sym = table.create_parameter("arg", DataType::Float, 0);
    assert(sym != nullptr);
    assert(sym->name == "arg");
    assert(sym->kind == SymbolKind::Parameter);
    assert(sym->type == DataType::Float);
    assert(sym->size == 4);
    cout << "[PASS] Insert parameter\n";
}

void test_lookup_in_same_scope() {
    SymbolTable table;
    table.create_variable("local", DataType::Int, 4);
    auto sym = table.lookup("local");
    assert(sym != nullptr);
    assert(sym->name == "local");
    cout << "[PASS] Lookup in same scope\n";
}

void test_lookup_in_parent_scope() {
    SymbolTable parent;
    SymbolTable child(&parent);

    parent.create_variable("global", DataType::Int, 4);

    // 从子作用域查找父作用域的变量
    auto sym = child.lookup("global");
    assert(sym != nullptr);
    assert(sym->name == "global");

    // 从父作用域查找子作用域的变量应该找不到
    auto sym2 = parent.lookup("nonexistent");
    assert(sym2 == nullptr);
    cout << "[PASS] Lookup in parent scope\n";
}

void test_lookup_not_found() {
    SymbolTable table;
    auto sym = table.lookup("nonexistent");
    assert(sym == nullptr);
    cout << "[PASS] Lookup not found\n";
}

void test_nested_scopes() {
    SymbolTable global;
    SymbolTable func(&global);
    SymbolTable block(&func);

    // 在不同作用域插入同名的变量
    global.create_variable("x", DataType::Int, 4);
    func.create_variable("x", DataType::Float, 4);
    block.create_variable("x", DataType::Int, 4);

    // 从 block 查找应该找到最近的
    auto sym1 = block.lookup("x");
    assert(sym1 != nullptr);
    assert(sym1->type == DataType::Int); // block 层

    // 从 func 查找应该找到 func 层的
    auto sym2 = func.lookup("x");
    assert(sym2 != nullptr);
    assert(sym2->type == DataType::Float); // func 层

    // 从 global 查找应该找到 global 层的
    auto sym3 = global.lookup("x");
    assert(sym3 != nullptr);
    assert(sym3->type == DataType::Int); // global 层
    cout << "[PASS] Nested scopes\n";
}

void test_insert_duplicate() {
    SymbolTable table;
    table.create_variable("x", DataType::Int, 4);
    auto sym2 = table.create_variable("x", DataType::Float, 4);
    // 应该插入失败，返回已存在的符号
    assert(sym2->name == "x");
    assert(sym2->type == DataType::Int); // 保持原来的类型
    cout << "[PASS] Insert duplicate\n";
}

void test_offset_tracking() {
    SymbolTable table;

    assert(table.current_offset() == 0);

    table.create_variable("a", DataType::Int, 4);
    assert(table.current_offset() == 4);

    table.create_variable("b", DataType::Int, 4);
    assert(table.current_offset() == 8);

    table.set_offset(100);
    assert(table.current_offset() == 100);
    cout << "[PASS] Offset tracking\n";
}

void test_multiple_types() {
    SymbolTable table;

    table.create_variable("int_var", DataType::Int, 4);
    table.create_variable("float_var", DataType::Float, 4);
    table.create_function("func1", DataType::Int);
    table.create_parameter("param1", DataType::Float, 0);

    auto int_var = table.lookup("int_var");
    auto float_var = table.lookup("float_var");
    auto func1 = table.lookup("func1");
    auto param1 = table.lookup("param1");

    assert(int_var != nullptr);
    assert(float_var != nullptr);
    assert(func1 != nullptr);
    assert(param1 != nullptr);

    assert(int_var->type == DataType::Int);
    assert(float_var->type == DataType::Float);
    assert(func1->type == DataType::Int);
    assert(param1->type == DataType::Float);
    cout << "[PASS] Multiple types\n";
}

int main() {
    cout << "Running SymbolTable tests...\n\n";

    test_default_construction();
    test_construction_with_parent();
    test_insert_variable();
    test_insert_function();
    test_insert_parameter();
    test_lookup_in_same_scope();
    test_lookup_in_parent_scope();
    test_lookup_not_found();
    test_nested_scopes();
    test_insert_duplicate();
    test_offset_tracking();
    test_multiple_types();

    cout << "\nAll tests passed!\n";
    return 0;
}
