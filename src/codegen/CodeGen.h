#pragma once

#include "ir/Quad.h"
#include <string>

class CodeGen
{
private:
	// 输出流
	std::ostream &out_;

	// 标签映射（四元式索引 → 汇编标签）
	std::unordered_map<int, std::string> label_map_;

public:
	explicit CodeGen(std::ostream &out) : out_(out) {}

	// 生成汇编代码（主入口）
	void generate(const IRProgram &program);

	// 辅助方法（Phase 4 实现）
	void emit_prologue();
	void emit_epilogue();
	void emit_data_section();
	void emit_text_section();
	std::string quad_to_assembly(const Quad &quad);
};