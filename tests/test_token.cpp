/**
 * @file test_token.cpp
 * @brief Token 和 TokenUtils 单元测试
 */

#include <cassert>
#include <iostream>
#include "../include/Token.h"
#include "../include/common.h"

void test_token_creation()
{
	// 测试 Token 构造
	SourceLocation loc(1, 5, "test.cmm");
	Token token(TokenType::INTTK, "int", loc);

	assert(token.type == TokenType::INTTK);
	assert(token.lexeme == "int");
	assert(token.location.line == 1);
	assert(token.location.column == 5);
	assert(token.location.filename == "test.cmm");
	assert(token.is_valid());

	// 测试默认构造（无效 token）
	Token empty;
	assert(!empty.is_valid());

	std::cout << "[PASS] test_token_creation" << std::endl;
}

void test_tokenutils_keywords()
{
	// 测试保留字识别
	TokenType type;

	assert(TokenUtils::is_keyword("int", type));
	assert(type == TokenType::INTTK);

	assert(TokenUtils::is_keyword("if", type));
	assert(type == TokenType::IFTK);

	assert(TokenUtils::is_keyword("while", type));
	assert(type == TokenType::WHILETK);

	assert(TokenUtils::is_keyword("return", type));
	assert(type == TokenType::RETURNTK);

	// 测试非保留字
	assert(!TokenUtils::is_keyword("myvar", type));
	assert(!TokenUtils::is_keyword("x123", type));

	std::cout << "[PASS] test_tokenutils_keywords" << std::endl;
}

void test_tokenutils_to_string()
{
	// 测试类型字符串转换
	assert(TokenUtils::to_string(TokenType::CONSTTK) == "CONSTTK");
	assert(TokenUtils::to_string(TokenType::INTTK) == "INTTK");
	assert(TokenUtils::to_string(TokenType::PLUS) == "PLUS");
	assert(TokenUtils::to_string(TokenType::SEMICN) == "SEMICN");
	assert(TokenUtils::to_string(TokenType::IDENFR) == "IDENFR");
	assert(TokenUtils::to_string(TokenType::INTCON) == "INTCON");
	assert(TokenUtils::to_string(TokenType::END_OF_FILE) == "EOF");
	assert(TokenUtils::to_string(static_cast<TokenType>(999)) == "UNKNOWN");

	std::cout << "[PASS] test_tokenutils_to_string" << std::endl;
}

int main()
{
	std::cout << "Running Token tests..." << std::endl;

	test_token_creation();
	test_tokenutils_keywords();
	test_tokenutils_to_string();

	std::cout << "All Token tests passed!" << std::endl;
	return 0;
}
