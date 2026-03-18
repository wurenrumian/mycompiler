/**
 * @file yylex.cpp
 * @brief Bison 词法分析器接口
 *
 * 将 Lexer 封装为 Bison 需要的 yylex() 函数
 */

#include "cmm.tab.h"
#include "Lexer.h"
#include <cstring>

// 全局 lexer 实例（Bison 单线程）
static Lexer *g_lexer = nullptr;

// Bison 行号变量
extern int yylineno;

// 当前 token 的语义值（由 Bison 声明）
extern YYSTYPE yylval;

// 设置 lexer 实例
void set_lexer(Lexer *lexer)
{
	g_lexer = lexer;
}

// Bison 调用的词法分析函数
int yylex()
{
	if (!g_lexer)
	{
		return 0; // 错误：未设置 lexer
	}

	Token token = g_lexer->next_token();

	// 设置行号（Bison 使用 yylineno）
	yylineno = token.location.line;

	// 处理语义值
	if (token.type == TokenType::INTCON)
	{
		yylval.ival = std::stoi(token.lexeme);
	}
	else if (token.type == TokenType::IDENFR)
	{
		yylval.sval = _strdup(token.lexeme.c_str());
	}
	else if (token.type == TokenType::STRCON)
	{
		yylval.sval = _strdup(token.lexeme.c_str());
	}

	switch (token.type)
	{
	// 保留字
	case TokenType::CONSTTK:
		return CONST;
	case TokenType::INTTK:
		return INT_TYPE;
	case TokenType::ELSETK:
		return ELSE;
	case TokenType::BREAKTK:
		return BREAK;
	case TokenType::RETURNTK:
		return RETURN;
	case TokenType::VOIDTK:
		return VOID_TYPE;
	case TokenType::IFTK:
		return IF;
	case TokenType::WHILETK:
		return WHILE;
	case TokenType::CONTINUETK:
		return CONTINUE;

	// 运算符
	case TokenType::GEQ:
		return GE;
	case TokenType::EQL:
		return EQ;
	case TokenType::AND:
		return AND;
	case TokenType::OR:
		return OR;
	case TokenType::LEQ:
		return LE;
	case TokenType::LSS:
		return LT;
	case TokenType::GRE:
		return GT;
	case TokenType::PLUS:
		return PLUS;
	case TokenType::MINU:
		return MINU;
	case TokenType::MULT:
		return MUL;
	case TokenType::DIV:
		return DIV;
	case TokenType::MOD:
		return MOD;
	case TokenType::ASSIGN:
		return ASSIGN;
	case TokenType::NOT:
		return NOT;

	// 界符
	case TokenType::SEMICN:
		return SEMICOLON;
	case TokenType::LPARENT:
		return LPARENT;
	case TokenType::RPARENT:
		return RPARENT;
	case TokenType::LBRACK:
		return LBRACK;
	case TokenType::RBRACK:
		return RBRACK;
	case TokenType::LBRACE:
		return LBRACE;
	case TokenType::RBRACE:
		return RBRACE;
	case TokenType::COMMA:
		return COMMA;

	// 其他
	case TokenType::INTCON:
		return INTEGER_LITERAL;
	case TokenType::IDENFR:
		return IDENTIFIER;
	case TokenType::STRCON:
		return STRCON;

	case TokenType::END_OF_FILE:
		return 0;

	case TokenType::INVALID:
	default:
		return -1; // 错误
	}
}
