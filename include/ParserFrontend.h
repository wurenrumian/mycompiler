#pragma once

/**
 * @file ParserFrontend.h
 * @brief 语法分析前端封装接口
 *
 * 本模块对 Bison 生成的语法分析器进行封装，
 * 提供简洁的 "文件 → AST" 接口。
 *
 * 设计目的：
 * - 隐藏 Bison 相关的 extern 声明细节
 * - 提供统一的错误处理机制
 * - 封装 Lexer 和 Parser 的初始化流程
 *
 * 使用流程：
 * 1. 调用 parse_file_to_ast() 解析源文件
 * 2. 检查返回值确认解析是否成功
 * 3. 通过 root 输出参数获取 AST 智能指针
 */

#include <memory>
#include <string>

#include "Ast.h"

/**
 * @brief 将源文件解析为 AST
 *
 * 封装词法分析 + 语法分析的完整流程：
 * 1. 创建 Lexer 对象并绑定到 Parser
 * 2. 调用 yyparse() 执行语法分析
 * 3. 从 Parser 获取构建好的 AST
 *
 * @param input_path 输入源文件路径 (testfile.txt)
 * @param root 输出参数，指向 AST 智能指针的引用
 * @param error_message 输出参数，存储错误信息 (解析失败时填充)
 * @return 解析成功返回 true，失败返回 false
 */
bool parse_file_to_ast(const std::string &input_path,
				   std::unique_ptr<ast::CompUnit> *root,
				   std::string *error_message);
