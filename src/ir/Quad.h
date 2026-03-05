#pragma once

#include <string>
#include <vector>
#include <variant>
#include "symtab/Symbol.h"

// 四元式操作符类型（简化版）
enum class QuadOp
{
	// 赋值
	Assign,

	// 算术
	Add,
	Sub,
	Mul,
	Div,
	Mod,

	// 比较
	Eq,
	Neq,
	Lt,
	Le,
	Gt,
	Ge,

	// 控制流
	Jump,
	JumpIfTrue,
	JumpIfFalse,

	// 函数
	Call,
	Return,

	// 地址运算
	LoadAddress,
	LoadValue,
	StoreValue
};

// 操作数类型：可以是符号、立即数、临时变量
using Operand = std::variant<
	std::shared_ptr<Symbol>, // 变量/参数
	int,					 // 整型立即数
	double,					 // 浮点立即数
	std::string				 // 临时变量名（如 t1, t2）
	>;

// Quad: 四元式结构
struct Quad
{
	QuadOp op;
	Operand arg1;
	Operand arg2;
	Operand result;

	Quad(QuadOp op, Operand arg1 = {}, Operand arg2 = {}, Operand result = {})
		: op(op), arg1(std::move(arg1)), arg2(std::move(arg2)), result(std::move(result)) {}
};

// IR 程序：四元式列表
class IRProgram
{
private:
	std::vector<Quad> quads_;

public:
	void emit(const Quad &quad) { quads_.push_back(quad); }
	const std::vector<Quad> &quads() const { return quads_; }
	size_t size() const { return quads_.size(); }

	// Phase 2 实现：标签管理（用于跳转）
	int new_label() { /* TODO */ return 0; }
	void mark_label(int label) { /* TODO */ }
	void patch_label(int label, int quad_index) { /* TODO */ }
};