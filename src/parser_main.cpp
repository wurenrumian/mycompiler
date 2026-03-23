#include <iostream>
#include <fstream>
#include "Lexer.h"

// 声明由 Bison 生成的函数和变量
extern int yyparse();
extern void set_lexer(Lexer *lexer);
extern std::ofstream output_file;

int main()
{
	const char *input_filename = "testfile.txt";
	const char *output_filename = "output.txt";

	try
	{
		std::cout << "Opening input file: " << input_filename << std::endl;
		// 创建词法分析器
		Lexer current_lexer(input_filename);
		set_lexer(&current_lexer);

		std::cout << "Opening output file: " << output_filename << std::endl;
		// 打开输出文件
		output_file.open(output_filename);
		if (!output_file.is_open())
		{
			std::cerr << "Failed to open output file: " << output_filename << std::endl;
			return 1;
		}

		std::cout << "Starting parsing..." << std::endl;
		// 执行语法分析
		int result = yyparse();
		if (result == 0)
		{
			std::cout << "Parsing completed successfully." << std::endl;
		}
		else
		{
			std::cerr << "Parsing failed with code: " << result << std::endl;
			output_file.close();
			return 1;
		}

		output_file.close();
		std::cout << "Output file closed." << std::endl;
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
