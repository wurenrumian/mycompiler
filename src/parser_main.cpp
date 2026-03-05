/**
 * @file parser_main.cpp
 * @brief Homework 2: 语法分析器主程序
 */

#include <cstdio>
#include <iostream>
#include "Lexer.h"

// 声明由 Bison 生成的函数和变量
extern int yyparse();
extern void set_lexer(Lexer *lexer);

int main(int argc, char **argv)
{
	const char *input_file = "testfile.txt";
	if (argc >= 2)
	{
		input_file = argv[1];
	}

	try
	{
		// 创建词法分析器
		Lexer lexer(input_file);

		// 设置全局 lexer 供 yylex() 使用
		set_lexer(&lexer);

		// 打开输出文件（如果 cmm.y 中需要）
		// 注意：cmm.y 中使用了 out_file，我们需要确保它被正确初始化或在 parser_main 中处理

		// 执行语法分析
		if (yyparse() == 0)
		{
			std::cout << "Parsing completed successfully." << std::endl;
		}
		else
		{
			std::cerr << "Parsing failed." << std::endl;
			return 1;
		}

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
