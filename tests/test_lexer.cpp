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
	assert(token.type == TokenType::PUTFTK);

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

int main()
{
	std::cout << "Running Lexer tests..." << std::endl;

	test_lexer_basic();
	test_lexer_operators();
	test_lexer_strings();
	test_lexer_whitespace();

	std::cout << "All Lexer tests passed!" << std::endl;
	return 0;
}
