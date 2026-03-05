/**
 * @file test_parser.cpp
 * @brief Parser 单元测试
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include "../include/Lexer.h"

// 声明由 Bison 生成的函数和变量
extern int yyparse();
extern void set_lexer(Lexer *lexer);

// 创建临时测试文件
void create_test_file(const std::string &filename, const std::string &content)
{
	std::ofstream file(filename);
	file << content;
	file.close();
}

// 删除临时文件
void delete_test_file(const std::string &filename)
{
	std::remove(filename.c_str());
}

void test_parser_basic()
{
	std::string content = "int main() {\n"
						  "    return 0;\n"
						  "}";
	create_test_file("test_parser1.txt", content);

	Lexer lexer("test_parser1.txt");
	set_lexer(&lexer);

	int result = yyparse();
	assert(result == 0);

	delete_test_file("test_parser1.txt");
	std::cout << "[PASS] test_parser_basic" << std::endl;
}

void test_parser_decl()
{
	std::string content = "int a = 1;\n"
						  "int b = 2;\n"
						  "int main() {\n"
						  "    a = a + b;\n"
						  "    return a;\n"
						  "}";
	create_test_file("test_parser2.txt", content);

	Lexer lexer("test_parser2.txt");
	set_lexer(&lexer);

	int result = yyparse();
	assert(result == 0);

	delete_test_file("test_parser2.txt");
	std::cout << "[PASS] test_parser_decl" << std::endl;
}

int main()
{
	try
	{
		test_parser_basic();
		test_parser_decl();
		std::cout << "All parser tests passed!" << std::endl;
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Test failed with exception: " << e.what() << std::endl;
		return 1;
	}
}
