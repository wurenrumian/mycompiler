#pragma once

#include <string>
#include <vector>
#include "ir/Quad.h"

// MIPSAssembler: 将四元式转换为 MIPS 汇编代码
class MIPSAssembler
{
private:
	std::vector<std::string> assembly_lines_;
	int temp_var_counter_ = 0;

public:
	MIPSAssembler() = default;

	// 生成 MIPS 汇编代码
	std::string generate(const IRProgram &program);

	// 获取汇编代码行
	const std::vector<std::string> &lines() const { return assembly_lines_; }

private:
	// 辅助方法（Phase 4 实现）
	std::string emit_instruction(const std::string &op, const std::string &args = "");
	std::string operand_to_register(const Operand &op);
	int next_temp_var();
};