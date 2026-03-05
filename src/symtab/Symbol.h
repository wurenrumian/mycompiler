#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "common.h"

// 符号类型枚举
enum class SymbolKind
{
	Variable,  // 变量
	Function,  // 函数
	Parameter, // 函数参数
	Constant   // 常量
};

// 数据类型枚举（C-- 子集）
enum class DataType
{
	Int,
	Float,
	Void,
	Unknown // 未确定类型（语义分析阶段填充）
};

// Symbol: 符号表条目
struct Symbol
{
	std::string name;		// 符号名
	SymbolKind kind;		// 符号种类
	DataType type;			// 数据类型
	int offset = 0;			// 栈偏移量（相对于帧指针）
	int size = 0;			// 大小（字节）
	bool is_global = false; // 是否全局变量

	Symbol(std::string name, SymbolKind kind, DataType type)
		: name(std::move(name)), kind(kind), type(type) {}
};

// SymbolTable 前向声明（完整定义见 SymbolTable.h）
class SymbolTable;

// 为了方便，声明常用接口（实际定义在 SymbolTable.h）
// 注意：这里只声明不定义，避免重复
