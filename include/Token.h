#pragma once

#include <string>
#include <cstdint>
#include "common.h"

/**
 * @brief Token 类型枚举
 *
 * 对应 CMM 语言的所有类别码，包括：
 * - 保留字（CONSTTK, INTTK, IFTK 等）
 * - 运算符（GEQ, EQL, AND, OR 等）
 * - 界符（SEMICN, LPARENT, RBRACK 等）
 * - 其他（INTCON, IDENFR, STRCON）
 */
enum class TokenType
{
	// 保留字
	CONSTTK,	 // const
	INTTK,		 // int
	ELSETK,		 // else
	BREAKTK,	 // break
	RETURNTK,	 // return
	PUTARRAYTK,	 // putarray
	STOPTIMETK,	 // stoptime
	GETINTTK,	 // getint
	GETCHTK,	 // getch
	GETARRAYTK,	 // getarray
	PUTINTTK,	 // putint
	PUTCHTK,	 // putch
	PUTFTK,		 // putf
	STARTTIMETK, // starttime
	VOIDTK,		 // void
	IFTK,		 // if
	WHILETK,	 // while
	CONTINUETK,	 // continue
	MAINTK,		 // main

	// 运算符
	GEQ,	// >=
	EQL,	// ==
	AND,	// &&
	OR,		// ||
	LEQ,	// <=
	LSS,	// <
	GRE,	// >
	PLUS,	// +
	MIUN,	// -
	MULT,	// *
	DIV,	// /
	MOD,	// %
	ASSIGN, // =
	NOT,	// !

	// 界符
	SEMICN,	 // ;
	LPARENT, // (
	RPARENT, // )
	LBRACK,	 // [
	RBRACK,	 // ]
	LBRACE,	 // {
	RBRACE,	 // }
	COMMA,	 // ,

	// 其他
	INTCON, // 整数常量
	IDENFR, // 标识符
	STRCON, // 格式字符串

	// 特殊
	END_OF_FILE, // 文件结束
	INVALID		 // 无效 token
};

/**
 * @brief Token 结构体
 *
 * 表示词法分析器输出的一个 token，包含：
 * - type: Token 类型
 * - lexeme: 原始字符串（从输入中直接读取）
 * - location: token 在源文件中的位置（行号、列号、文件名）
 */
struct Token
{
	TokenType type;			 // Token 类型
	std::string lexeme;		 // 原始字符串（原样保留）
	SourceLocation location; // token 起始位置

	/**
	 * @brief 默认构造函数（创建无效 token）
	 */
	Token() : type(TokenType::INVALID), lexeme(""), location() {}

	/**
	 * @brief 构造函数
	 * @param t Token 类型
	 * @param s 原始字符串
	 * @param loc 位置信息
	 */
	Token(TokenType t, const std::string &s, const SourceLocation &loc)
		: type(t), lexeme(s), location(loc) {}

	/**
	 * @brief 判断 token 是否有效
	 * @return true 如果 type 不是 INVALID 或 END_OF_FILE
	 */
	bool is_valid() const { return type != TokenType::INVALID && type != TokenType::END_OF_FILE; }
};

/**
 * @brief Token 工具类
 *
 * 提供保留字识别和类型字符串转换功能
 */
class TokenUtils
{
public:
	/**
	 * @brief 判断是否为保留字
	 * @param word 待检查的字符串
	 * @param out_type 输出对应的 TokenType
	 * @return true 如果是保留字
	 */
	static bool is_keyword(const std::string &word, TokenType &out_type);

	/**
	 * @brief 获取 Token 类型的字符串表示
	 * @param type Token 类型
	 * @return 类型名称字符串（如 "CONSTTK"、"INTTK"）
	 */
	static std::string to_string(TokenType type);
};
