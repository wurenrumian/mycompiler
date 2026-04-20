/**
 * @file Semantic.cpp
 * @brief 语义分析模块实现
 *
 * 本文件实现了语义分析的核心功能。
 *
 * 分析流程：
 * 1. 空检查 - 确保 AST 非空
 * 2. main 函数验证 - 确保恰好有一个 main 函数
 *
 * 设计考量：
 * - 采用简单的一次遍历完成所有检查
 * - 错误信息详细，便于定位问题
 */

#include "Semantic.h"

#include <cstddef>

namespace semantic
{
bool analyze_program(const ast::CompUnit &root,
			 std::string *error_message)
{
	/** 第一步：检查 AST 是否为空 */
	if (root.items.empty())
	{
		if (error_message != nullptr)
		{
			*error_message = "AST is empty.";
		}
		return false;
	}

	/** 第二步：遍历所有顶层元素，检查 main 函数数量 */
	size_t main_function_count = 0;
	for (size_t i = 0; i < root.items.size(); ++i)
	{
		const ast::Item &item = root.items[i];
		if (item.kind == ast::ItemKind::MainFuncDef)
		{
			++main_function_count;
		}
	}

	/** 第三步：验证 main 函数唯一性 (SysY 语法要求恰好一个 main) */
	if (main_function_count != 1)
	{
		if (error_message != nullptr)
		{
			*error_message = "Program must contain exactly one main function.";
		}
		return false;
	}

	return true;
}
} // namespace semantic
