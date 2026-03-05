#include <cstdio>
#include <iostream>

// Bison 生成的 C 头文件（包含 token 定义和 yyparse 声明）
#include "cmm.tab.h"
// Flex 生成的 C 头文件（包含 yylex 声明）
#include "lex.yy.h"

// 错误处理函数（bison 调用）
void yyerror(const char *msg);

int main(int argc, char **argv)
{
	// 1. 设置输入文件
	if (argc > 1)
	{
		yyin = std::fopen(argv[1], "r");
		if (!yyin)
		{
			std::cerr << "Error: cannot open file '" << argv[1] << "'" << std::endl;
			return 1;
		}
	}
	else
	{
		yyin = stdin; // 从标准输入读取
	}

	// 2. 调用 bison 生成的解析器（C 风格 API）
	std::cout << "Starting parser..." << std::endl;
	int result = yyparse();

	// 3. 输出结果
	if (result == 0)
	{
		std::cout << "Parsing completed successfully." << std::endl;
	}
	else
	{
		std::cerr << "Parsing failed with error code " << result << std::endl;
	}

	// 4. 清理资源
	if (argc > 1)
		std::fclose(yyin);

	return result;
}

// 错误处理函数
void yyerror(const char *msg)
{
	std::cerr << "Parse error: " << msg << std::endl;
}