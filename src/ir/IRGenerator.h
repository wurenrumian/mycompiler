#pragma once

#include "ir/Quad.h"
#include "symtab/Symbol.h"

// IRGenerator: 遍历 AST 生成四元式
class IRGenerator
{
private:
	IRProgram program_;
	SymbolTable *current_scope_ = nullptr;

public:
	IRGenerator() = default;

	// 主入口：生成 IR
	void generate();

	// 获取生成的 IR 程序
	const IRProgram &program() const { return program_; }

	// 辅助方法（Phase 3 实现）
	void enter_scope(SymbolTable *scope);
	void exit_scope();
	SymbolTable *current_scope() const { return current_scope_; }
};