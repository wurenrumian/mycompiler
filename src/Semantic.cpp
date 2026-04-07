#include "Semantic.h"

namespace semantic
{
bool analyze_program(const ast::CompUnit &root,
			 ProgramInfo *program_info,
			 std::string *error_message)
{
	if (root.items.empty())
	{
		if (error_message != nullptr)
		{
			*error_message = "AST is empty.";
		}
		return false;
	}

	ProgramInfo info;
	for (size_t i = 0; i < root.items.size(); ++i)
	{
		const ast::Item &item = root.items[i];
		if (item.kind == ast::ItemKind::Decl)
		{
			++info.global_item_count;
		}
		else if (item.kind == ast::ItemKind::FuncDef)
		{
			++info.function_count;
		}
		else if (item.kind == ast::ItemKind::MainFuncDef)
		{
			++info.main_function_count;
			++info.function_count;
		}
	}

	if (info.main_function_count != 1)
	{
		if (error_message != nullptr)
		{
			*error_message = "Program must contain exactly one main function.";
		}
		return false;
	}

	if (program_info != nullptr)
	{
		*program_info = info;
	}

	return true;
}
} // namespace semantic
