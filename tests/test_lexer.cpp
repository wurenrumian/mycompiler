/**
 * @file test_lexer.cpp
 * @brief Lexer 单元测试
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include "../include/Lexer.h"
#include "../include/Token.h"

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

void test_lexer_basic()
{
	// 创建测试文件
	create_test_file("test_lexer1.txt", "int x = 5;");

	Lexer lexer("test_lexer1.txt");
	Token token;

	// 读取 int
	token = lexer.next_token();
	std::cout << "Token 1: type=" << static_cast<int>(token.type)
			  << ", lexeme='" << token.lexeme << "'"
			  << ", INTTK=" << static_cast<int>(TokenType::INTTK) << std::endl;
	assert(token.is_valid());
	assert(token.type == TokenType::INTTK);
	assert(token.lexeme == "int");

	// 读取标识符 x
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR);
	assert(token.lexeme == "x");

	// 读取 =
	token = lexer.next_token();
	assert(token.type == TokenType::ASSIGN);
	assert(token.lexeme == "=");

	// 读取整数 5
	token = lexer.next_token();
	assert(token.type == TokenType::INTCON);
	assert(token.lexeme == "5");

	// 读取 ;
	token = lexer.next_token();
	assert(token.type == TokenType::SEMICN);
	assert(token.lexeme == ";");

	// EOF
	token = lexer.next_token();
	assert(token.type == TokenType::END_OF_FILE);

	delete_test_file("test_lexer1.txt");
	std::cout << "[PASS] test_lexer_basic" << std::endl;
}

void test_lexer_operators()
{
	// 测试多字符运算符
	create_test_file("test_lexer2.txt", "a >= b && c == d || e <= f");

	Lexer lexer("test_lexer2.txt");
	Token token;

	// a
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "a");

	// >=
	token = lexer.next_token();
	assert(token.type == TokenType::GEQ && token.lexeme == ">=");

	// b
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "b");

	// &&
	token = lexer.next_token();
	assert(token.type == TokenType::AND && token.lexeme == "&&");

	// c
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "c");

	// ==
	token = lexer.next_token();
	assert(token.type == TokenType::EQL && token.lexeme == "==");

	// d
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "d");

	// ||
	token = lexer.next_token();
	assert(token.type == TokenType::OR && token.lexeme == "||");

	// e
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "e");

	// <=
	token = lexer.next_token();
	assert(token.type == TokenType::LEQ && token.lexeme == "<=");

	// f
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "f");

	delete_test_file("test_lexer2.txt");
	std::cout << "[PASS] test_lexer_operators" << std::endl;
}

void test_lexer_strings()
{
	// 测试字符串
	create_test_file("test_lexer3.txt", "putf(\"Hello %d\\n\", x);");

	Lexer lexer("test_lexer3.txt");
	Token token;

	// putf
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "putf");

	// (
	token = lexer.next_token();
	assert(token.type == TokenType::LPARENT);

	// "Hello %d\n"
	token = lexer.next_token();
	assert(token.type == TokenType::STRCON);
	assert(token.lexeme == "\"Hello %d\\n\"");

	// ,
	token = lexer.next_token();
	assert(token.type == TokenType::COMMA);

	// x
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "x");

	// )
	token = lexer.next_token();
	assert(token.type == TokenType::RPARENT);

	// ;
	token = lexer.next_token();
	assert(token.type == TokenType::SEMICN);

	delete_test_file("test_lexer3.txt");
	std::cout << "[PASS] test_lexer_strings" << std::endl;
}

void test_lexer_whitespace()
{
	// 测试空白字符处理
	create_test_file("test_lexer4.txt", "  int   x\n\t=  5  ;");

	Lexer lexer("test_lexer4.txt");
	Token token;

	// int
	token = lexer.next_token();
	assert(token.type == TokenType::INTTK);

	// x
	token = lexer.next_token();
	assert(token.type == TokenType::IDENFR && token.lexeme == "x");

	// =
	token = lexer.next_token();
	assert(token.type == TokenType::ASSIGN);

	// 5
	token = lexer.next_token();
	assert(token.type == TokenType::INTCON && token.lexeme == "5");

	// ;
	token = lexer.next_token();
	assert(token.type == TokenType::SEMICN);

	delete_test_file("test_lexer4.txt");
	std::cout << "[PASS] test_lexer_whitespace" << std::endl;
}

void test_lexer_long_identifier()
{
	// 测试超长标识符
	std::string long_id = "Cy92k8jOyGwpymrp_aZeu_vYwQPeRGHYo0nKXgxMsU3ALykomjoy0v7PrBR5MLWuTcPITZzLajYN3kAhROsMQ6uUe3NqQ6QJyFKvM2G1h6mT3QX9DYbJQsDjb_qKv7vEp6fZLUYJbP4BQnFuisvKlzY8Ym0f_IYchIbJQCvl5R0X3cujCWAWQmNXr8sWoyCtt8ghUridaVa8TcycXdrhLOXI4akvL4wY1B3BCAX4nTZsUGemmDbrjvz_XQ37Hw5lRNDM3AuNOb_oeTlLhRcIjoby7T1ozDgHQbUJFDKlD7D999ynqfEwOeJ1UTkX0elvwrn_Cem72IEwj0Kx_bCapCVe5JSCtVSKmoHhm0wlescYbATn21EnuIjiP_bVvzEvjKY9d7azInHGIwWjZJW_I8EgXj0rP9adIEZxSMJ6BLnuegBp71xWiZCJh_s4efmZUatbxHChtdHkY_VnLFku8X29hgVTjgPdTqjbP2Jcu2bV58lnZIt5wQk3rqeTInqkxNW4OvSbvo6np8PVtL0zjbJ5Sx2oSU1KcQTAG4Dgx14Eb8zGbDvs19ErvuUl48YOguAP2tJhbWBZ5Zv7FSg5V6BsF0ABLFmcT1LFgbgI0af8OK8eOrCzbQgG9SLnx9UTJx3_fCGpFOe5TtAqRvc4tqQglgYgfbGUpRT8X3yGln8jnUt6uR2Xgacd0JfjQg8CWLBihfD9JlE2hPUyoNzhaLmiZleiA4dg1I1q_5ugfO5PyfWwEVVwyTAlpaza1fOCAUCJW5Fy7g_fHFBfDNdUf98CJRmULkQ_qKCBxqVTV1JYDHm1vkKFt9qzr9sGxZWyLhQwTaBt2X_diibJJRHPEHGqcyla_O5MOy5VOu5tvpkw3V9Nn0UwUBzWEa32i2ekbzyGHjwsjoCm9q1IRDsunPKOgXpLmo21aUPPjpKH0nwKJWSfOTpa03CYMBjAZAxxLXNyH2wuTPt0ievT3BLmixYFifby5cqUHHZx7oaK6PnfHMoTh8PIF7KUQY2lkO3zgr9TnRNKtra_BbFArmWlWpvBVBBGuXnKuhJXSWRdARZi00QIMeApsc9hPXeV0OT9UiauD3mgxKTvWieLmXXg0ccoUyTkghCWRbA_SVbTEwHvmX8M_987cngFjVGdz7jYuWcTprOvoYc5qzNK8HZnP0iKHka2ysn6qvr4sheXnyybWMTdrMH6ej26BhzU2bHDgLZISeWIdu_mIQIKmgPNY7A6LujG1lp2K86YJ57RGFyTy5lPyV3yTRc3b5MEFMExLrqSx3m_vSNy1jr73sw6qU4wsBDTdhLvfqtR3FfKhcIdYNhz6BzzF50J42BSKYvk5x4oMhs7OSzPwPdMtEDt1BEkciRTt5LZgGBWYTsWAdXV3gbLKz0y2mOked_wgPwcu7dIuid2p1a0pq5SyV0aSYs13abh_ddAoNTiB7ZTwL3mw05WISorBytfJexyq0534kbAnp5DQwnGiAuM5GMVdE0mAnMr";
	create_test_file("test_lexer5.txt", long_id);

	Lexer lexer("test_lexer5.txt");
	Token token = lexer.next_token();

	assert(token.type == TokenType::IDENFR);
	assert(token.lexeme == long_id);

	delete_test_file("test_lexer5.txt");
	std::cout << "[PASS] test_lexer_long_identifier" << std::endl;
}

int main()
{
	std::cout << "Running Lexer tests..." << std::endl;

	test_lexer_basic();
	test_lexer_operators();
	test_lexer_strings();
	test_lexer_whitespace();
	test_lexer_long_identifier();

	std::cout << "All Lexer tests passed!" << std::endl;
	return 0;
}
