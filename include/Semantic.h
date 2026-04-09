#pragma once

/**
 * @file Semantic.h
 * @brief 语义分析模块接口定义
 *
 * 本模块负责对 AST 进行语义检查和程序信息统计。
 *
 * 主要功能：
 * - 验证程序结构的合法性 (如检查 main 函数是否存在且唯一)
 * - 统计程序的组成信息 (全局变量数量、函数数量等)
 * - 为后续代码生成阶段提供程序结构信息
 *
 * 设计说明：
 * - analyze_program 是核心入口函数，执行完整的语义检查流程
 * - ProgramInfo 结构体存储分析结果，供其他模块使用
 * - 采用错误信息字符串返回机制，便于诊断和问题定位
 */

#include <cstddef>
#include <memory>
#include <string>

#include "Ast.h"

namespace semantic
{
/**
 * @brief 程序信息结构体
 *
 * 存储经过语义分析后的程序统计信息，
 * 用于代码生成阶段了解程序的整体结构。
 */
struct ProgramInfo
{
	size_t global_item_count = 0;    /**< 全局声明项的数量 */
	size_t function_count = 0;        /**< 函数定义的数量 (不含 main) */
	size_t main_function_count = 0;  /**< main 函数数量 (应为 1) */
};

/**
 * @brief 对 AST 进行语义分析
 *
 * 执行完整的语义检查流程：
 * 1. 检查 AST 是否为空
 * 2. 遍历所有顶层元素，统计各类数量
 * 3. 验证 main 函数存在且唯一
 *
 * @param root AST 根节点引用
 * @param program_info 输出参数，存储分析结果
 * @param error_message 输出参数，存储错误信息 (分析失败时填充)
 * @return 分析成功返回 true，失败返回 false
 */
bool analyze_program(const ast::CompUnit &root,
			 ProgramInfo *program_info,
			 std::string *error_message);
} // namespace semantic
