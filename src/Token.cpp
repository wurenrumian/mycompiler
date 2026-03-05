#include "Token.h"
#include <unordered_map>

// ============================================================================
// 保留字映射表（静态常量）
// ============================================================================
static const std::unordered_map<std::string, TokenType> keyword_map = {
	// 关键字
	{"const", TokenType::CONSTTK},
	{"int", TokenType::INTTK},
	{"else", TokenType::ELSETK},
	{"break", TokenType::BREAKTK},
	{"return", TokenType::RETURNTK},
	{"putarray", TokenType::PUTARRAYTK},
	{"stoptime", TokenType::STOPTIMETK},
	{"getint", TokenType::GETINTTK},
	{"getch", TokenType::GETCHTK},
	{"getarray", TokenType::GETARRAYTK},
	{"putint", TokenType::PUTINTTK},
	{"putch", TokenType::PUTCHTK},
	{"putf", TokenType::PUTFTK},
	{"starttime", TokenType::STARTTIMETK},
	{"void", TokenType::VOIDTK},
	{"if", TokenType::IFTK},
	{"while", TokenType::WHILETK},
	{"continue", TokenType::CONTINUETK},
	{"main", TokenType::MAINTK}};

// ============================================================================
// Token 类型字符串映射（用于输出）
// ============================================================================
static const std::unordered_map<TokenType, std::string> token_type_strings = {
	// 保留字
	{TokenType::CONSTTK, "CONSTTK"},
	{TokenType::INTTK, "INTTK"},
	{TokenType::ELSETK, "ELSETK"},
	{TokenType::BREAKTK, "BREAKTK"},
	{TokenType::RETURNTK, "RETURNTK"},
	{TokenType::PUTARRAYTK, "PUTARRAYTK"},
	{TokenType::STOPTIMETK, "STOPTIMETK"},
	{TokenType::GETINTTK, "GETINTTK"},
	{TokenType::GETCHTK, "GETCHTK"},
	{TokenType::GETARRAYTK, "GETARRAYTK"},
	{TokenType::PUTINTTK, "PUTINTTK"},
	{TokenType::PUTCHTK, "PUTCHTK"},
	{TokenType::PUTFTK, "PUTFTK"},
	{TokenType::STARTTIMETK, "STARTTIMETK"},
	{TokenType::VOIDTK, "VOIDTK"},
	{TokenType::IFTK, "IFTK"},
	{TokenType::WHILETK, "WHILETK"},
	{TokenType::CONTINUETK, "CONTINUETK"},
	{TokenType::MAINTK, "MAINTK"},

	// 运算符
	{TokenType::GEQ, "GEQ"},
	{TokenType::EQL, "EQL"},
	{TokenType::AND, "AND"},
	{TokenType::OR, "OR"},
	{TokenType::LEQ, "LEQ"},
	{TokenType::LSS, "LSS"},
	{TokenType::GRE, "GRE"},
	{TokenType::PLUS, "PLUS"},
	{TokenType::MIUN, "MIUN"},
	{TokenType::MULT, "MULT"},
	{TokenType::DIV, "DIV"},
	{TokenType::MOD, "MOD"},
	{TokenType::ASSIGN, "ASSIGN"},
	{TokenType::NOT, "NOT"},

	// 界符
	{TokenType::SEMICN, "SEMICN"},
	{TokenType::LPARENT, "LPARENT"},
	{TokenType::RPARENT, "RPARENT"},
	{TokenType::LBRACK, "LBRACK"},
	{TokenType::RBRACK, "RBRACK"},
	{TokenType::LBRACE, "LBRACE"},
	{TokenType::RBRACE, "RBRACE"},
	{TokenType::COMMA, "COMMA"},

	// 其他
	{TokenType::INTCON, "INTCON"},
	{TokenType::IDENFR, "IDENFR"},
	{TokenType::STRCON, "STRCON"},

	// 特殊
	{TokenType::END_OF_FILE, "EOF"},
	{TokenType::INVALID, "INVALID"}};

// ============================================================================
// TokenUtils 实现
// ============================================================================

bool TokenUtils::is_keyword(const std::string &word, TokenType &out_type)
{
	// 在保留字映射表中查找
	auto it = keyword_map.find(word);
	if (it != keyword_map.end())
	{
		out_type = it->second;
		return true;
	}
	return false;
}

std::string TokenUtils::to_string(TokenType type)
{
	// 获取类型字符串表示
	auto it = token_type_strings.find(type);
	if (it != token_type_strings.end())
		return it->second;
	return "UNKNOWN";
}
