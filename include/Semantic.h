#pragma once

/**
 * @file Semantic.h
 * @brief 语义分析模块接口定义
 *
 * 本模块负责对 AST 进行必要的语义检查。
 *
 * 主要功能：
 * - 验证程序结构的合法性 (如检查 main 函数是否存在且唯一)
 * - 采用错误信息字符串返回机制，便于诊断和问题定位
 */

#include <string>

#include "Ast.h"

namespace semantic
{
/**
 * @brief 对 AST 进行语义分析
 *
 * 执行完整的语义检查流程：
 * 1. 检查 AST 是否为空
 * 2. 验证 main 函数存在且唯一
 *
 * @param root AST 根节点引用
 * @param error_message 输出参数，存储错误信息 (分析失败时填充)
 * @return 分析成功返回 true，失败返回 false
 */
bool analyze_program(const ast::CompUnit &root,
			 std::string *error_message);
} // namespace semantic
