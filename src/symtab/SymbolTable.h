#pragma once

#include "symtab/Symbol.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// SymbolTable: 符号表（支持嵌套作用域）
class SymbolTable
{
private:
	// 符号存储：name -> Symbol
	std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols_;

	// 父作用域
	SymbolTable *parent_ = nullptr;

	// 当前作用域的变量偏移
	int current_offset_ = 0;

public:
	SymbolTable() = default;
	explicit SymbolTable(SymbolTable *parent) : parent_(parent) {}

	// 插入符号（返回是否成功）
	bool insert(const std::shared_ptr<Symbol> &sym);

	// 查找符号（当前作用域 + 父作用域）
	std::shared_ptr<Symbol> lookup(const std::string &name) const;

	// 创建变量符号
	std::shared_ptr<Symbol> create_variable(const std::string &name, DataType type, int size = 4);

	// 创建函数符号
	std::shared_ptr<Symbol> create_function(const std::string &name, DataType return_type);

	// 创建参数符号
	std::shared_ptr<Symbol> create_parameter(const std::string &name, DataType type, int position);

	// 获取父作用域
	SymbolTable *parent() const { return parent_; }

	// 获取当前偏移
	int current_offset() const { return current_offset_; }

	// 更新偏移
	void set_offset(int offset) { current_offset_ = offset; }
};