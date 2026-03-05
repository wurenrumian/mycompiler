#include "symtab/SymbolTable.h"
#include <unordered_map>

bool SymbolTable::insert(const std::shared_ptr<Symbol> &sym)
{
	// Phase 2 实现：检查重复，计算偏移量
	return symbols_.insert({sym->name, sym}).second;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string &name) const
{
	// Phase 2 实现：在当前表查找，找不到则向上查找 parent_
	auto it = symbols_.find(name);
	if (it != symbols_.end())
		return it->second;
	if (parent_)
		return parent_->lookup(name);
	return nullptr;
}

std::shared_ptr<Symbol> SymbolTable::create_variable(const std::string &name, DataType type, int size)
{
	// Phase 2 实现：创建 Symbol，计算 offset（基于 current_offset）
	auto sym = std::make_shared<Symbol>(name, SymbolKind::Variable, type);
	sym->size = size;
	// sym->offset = -current_offset() - size;  // 栈向下增长
	insert(sym);
	return sym;
}

std::shared_ptr<Symbol> SymbolTable::create_function(const std::string &name, DataType return_type)
{
	// Phase 2 实现：创建函数符号
	auto sym = std::make_shared<Symbol>(name, SymbolKind::Function, return_type);
	sym->size = 0; // 函数本身不占栈空间
	insert(sym);
	return sym;
}

std::shared_ptr<Symbol> SymbolTable::create_parameter(const std::string &name, DataType type, int position)
{
	// Phase 2 实现：创建参数符号（参数在栈上有固定位置）
	auto sym = std::make_shared<Symbol>(name, SymbolKind::Parameter, type);
	sym->size = 4; // 假设参数大小为 4 字节
	// offset 将在 Phase 3 计算
	insert(sym);
	return sym;
}