/**
 * @file lexer_main.cpp
 * @brief Homework 1: 词法分析器主程序
 *
 * 功能：
 * 1. 从命令行参数获取输入文件名
 * 2. 创建 Lexer 对象进行词法分析
 * 3. 将 token 流输出到 output.txt
 *
 * 输出格式：每行一个 token，格式为 "TokenType  lexeme"
 * 示例：CONSTTK  const
 *
 * 编译：g++ -std=c++11 -Iinclude src/lexer_main.cpp src/Token.cpp src/Lexer.cpp -o lexer
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Lexer.h"

// ============================================================================
// 输出控制宏
// ============================================================================
#ifndef ENABLE_OUTPUT
#define ENABLE_OUTPUT 1 // 默认开启输出到文件
#endif

int main(int argc, char **argv)
{
	// 检查命令行参数
	const char *input_file = "testfile.txt";
	if (argc >= 2)
	{
		input_file = argv[1];
	}

	try
	{
		// 创建词法分析器
		Lexer lexer(input_file);

#if ENABLE_OUTPUT
		// 打开输出文件
		std::ofstream out("output.txt");
		if (!out.is_open())
		{
			std::cerr << "Error: cannot create output.txt" << std::endl;
			return 1;
		}
#endif

		// 循环读取 token 并输出
		Token token;
		do
		{
			token = lexer.next_token();

			if (token.is_valid())
			{
#if ENABLE_OUTPUT
				out << TokenUtils::to_string(token.type) << " " << token.lexeme << std::endl;
#else
				std::cout << TokenUtils::to_string(token.type) << " " << token.lexeme << std::endl;
#endif
			}

			if (token.type == TokenType::END_OF_FILE)
				break;

		} while (true);

#if ENABLE_OUTPUT
		out.close();
		std::cout << "Lexical analysis completed. Output written to output.txt" << std::endl;
#endif

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
