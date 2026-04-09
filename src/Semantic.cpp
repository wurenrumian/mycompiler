/**
 * @file Semantic.cpp
 * @brief 语义分析模块实现
 *
 * 本文件实现了语义分析的核心功能。
 *
 * 分析流程：
 * 1. 空检查 - 确保 AST 非空
 * 2. 遍历统计 - 统计全局声明、函数定义、main 函数数量
 * 3. main 函数验证 - 确保恰好有一个 main 函数
 *
 * 设计考量：
 * - 采用简单的一次遍历完成所有检查
 * - 错误信息详细，便于定位问题
 * - ProgramInfo 结构体记录分析结果，供后续阶段使用
 */

#include "Semantic.h"

namespace semantic
{
bool analyze_program(const ast::CompUnit &root,
			 ProgramInfo *program_info,
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

	/** 第二步：遍历所有顶层元素，统计各类数量 */
	ProgramInfo info;
	for (size_t i = 0; i < root.items.size(); ++i)
	{
		const ast::Item &item = root.items[i];
		if (item.kind == ast::ItemKind::Decl)
		{
			++info.global_item_count;   /**< 全局声明项计数 */
		}
		else if (item.kind == ast::ItemKind::FuncDef)
		{
			++info.function_count;      /**< 普通函数定义计数 */
		}
		else if (item.kind == ast::ItemKind::MainFuncDef)
		{
			++info.main_function_count; /**< main 函数计数 */
			++info.function_count;      /**< main 也算作函数 */
		}
	}

	/** 第三步：验证 main 函数唯一性 (SysY 语法要求恰好一个 main) */
	if (info.main_function_count != 1)
	{
		if (error_message != nullptr)
		{
			*error_message = "Program must contain exactly one main function.";
		}
		return false;
	}

	/** 第四步：输出分析结果 */
	if (program_info != nullptr)
	{
		*program_info = info;
	}

	return true;
}
} // namespace semantic
