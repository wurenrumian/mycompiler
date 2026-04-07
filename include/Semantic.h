#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "Ast.h"

namespace semantic
{
struct ProgramInfo
{
	size_t global_item_count = 0;
	size_t function_count = 0;
	size_t main_function_count = 0;
};

bool analyze_program(const ast::CompUnit &root,
			 ProgramInfo *program_info,
			 std::string *error_message);
} // namespace semantic
