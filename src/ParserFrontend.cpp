/**
 * @file ParserFrontend.cpp
 * @brief 语法分析前端封装实现
 *
 * 本文件封装了 Bison 语法分析器的调用细节，
 * 提供简洁的文件 → AST 转换接口。
 *
 * 封装内容：
 * - Lexer 和 Parser 的初始化绑定
 * - 错误信息的收集和返回
 * - 异常安全处理
 *
 * 解析流程：
 * 1. 参数校验 - 确保输出指针有效
 * 2. 重置状态 - 清空之前的解析残留
 * 3. 创建 Lexer - 绑定到输入文件
 * 4. 绑定 Lexer 到 Parser - 通过 set_lexer()
 * 5. 调用 yyparse() - 执行语法分析
 * 6. 提取 AST - 通过 take_ast_root()
 */

#include "ParserFrontend.h"

#include <exception>

#include "Lexer.h"

/** @brief 声明 Bison 生成的解析器入口函数 */
extern int yyparse();
/** @brief 声明设置当前 Lexer 的函数 (由 sysy.y 提供) */
extern void set_lexer(Lexer *lexer);
/** @brief 声明重置解析器错误状态的函数 */
extern void reset_parser_error();
/** @brief 声明获取解析器错误消息的函数 */
extern std::string take_parser_error();

bool parse_file_to_ast(const std::string &input_path,
				   std::unique_ptr<ast::CompUnit> *root,
				   std::string *error_message)
{
	/** 参数校验：确保输出指针有效 */
	if (root == nullptr)
	{
		if (error_message != nullptr)
		{
			*error_message = "AST output pointer is null.";
		}
		return false;
	}

	/** 重置状态：清空之前的 AST 和错误消息 */
	root->reset();
	reset_parser_error();

	try
	{
		/** 创建 Lexer 并绑定到 Parser */
		Lexer lexer(input_path);
		set_lexer(&lexer);

		/** 执行语法分析 */
		if (yyparse() != 0)
		{
			if (error_message != nullptr)
			{
				*error_message = take_parser_error();
				if (error_message->empty())
				{
					*error_message = "Parser failed without diagnostic.";
				}
			}
			return false;
		}

		/** 提取构建好的 AST */
		*root = ast::take_ast_root();
		if ((*root).get() == nullptr)
		{
			if (error_message != nullptr)
			{
				*error_message = "Parser succeeded but AST root is missing.";
			}
			return false;
		}
	}
	catch (const std::exception &e)
	{
		/** 捕获并传播异常信息 */
		if (error_message != nullptr)
		{
			*error_message = e.what();
		}
		return false;
	}

	return true;
}
