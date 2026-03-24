#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <iostream>

// ============================================================================
// Lexer 构造函数/析构函数
// ============================================================================

Lexer::Lexer(std::unique_ptr<Stream<char>> s)
	: stream(std::move(s))
{
}

Lexer::Lexer(const std::string &filename)
	: stream(std::unique_ptr<Stream<char>>(new Stream<char>(filename)))
{
}

Lexer::~Lexer() = default;

// ============================================================================
// 辅助函数实现
// ============================================================================

void Lexer::skip_whitespace()
{
	/**
	 * @brief 跳过连续的空白字符
	 *
	 * 空白字符包括：空格、制表符、换行、回车
	 * 使用 stream->next() 读取，遇到非空白字符时回退
	 */
	char c;
	while (stream->next(c))
	{
		if (!char_util::is_whitespace(c))
		{
			stream->unget();
			break;
		}
	}
}

Token Lexer::read_identifier(SourceLocation loc)
{
	/**
	 * @brief 读取标识符或保留字
	 *
	 * 标识符格式：[a-zA-Z_][a-zA-Z0-9_]*
	 * 读取后检查是否为保留字
	 */
	std::string lexeme;
	char c;

	// 读取第一个字符（已确定是标识符首字符）
	stream->next(c);
	lexeme.push_back(c);

	// 读取后续字符（字母、数字、下划线）
	while (true)
	{
		char next_c = stream->peek(0); // 查看下一个字符
		if (char_util::is_identifier_part(next_c))
		{
			lexeme.push_back(next_c);
			stream->next(c); // 消耗字符
		}
		else
		{
			break;
		}
	}

	// 判断是否为保留字
	TokenType type;
	if (TokenUtils::is_keyword(lexeme, type))
	{
		Token t(type, lexeme, loc);
		// std::cout << "Token: " << TokenUtils::to_string(t.type) << " " << t.lexeme << std::endl;
		return t;
	}
	else
	{
		Token t(TokenType::IDENFR, lexeme, loc);
		// std::cout << "Token: " << TokenUtils::to_string(t.type) << " " << t.lexeme << std::endl;
		return t;
	}
}

Token Lexer::read_integer(SourceLocation loc)
{
	/**
	 * @brief 读取整数常量
	 *
	 * 格式：
	 * - 十六进制：0x[0-9a-fA-F]+ 或 0X[0-9a-fA-F]+
	 * - 十进制：[0-9]+
	 */
	std::string lexeme;
	char c;

	stream->next(c);
	lexeme.push_back(c);

	if (c == '0')
	{
		char next = stream->peek(0);
		if (next == 'x' || next == 'X')
		{
			stream->next(next);
			lexeme.push_back(next);
			while (true)
			{
				char hex_c = stream->peek(0);
				if (char_util::is_hex_digit(hex_c))
				{
					lexeme.push_back(hex_c);
					stream->next(hex_c);
				}
				else
				{
					break;
				}
			}
			return Token(TokenType::INTCON, lexeme, loc);
		}
	}

	// 十进制
	while (true)
	{
		char next_c = stream->peek(0);
		if (char_util::is_digit(next_c))
		{
			lexeme.push_back(next_c);
			stream->next(next_c);
		}
		else
		{
			break;
		}
	}

	return Token(TokenType::INTCON, lexeme, loc);
}

Token Lexer::read_string(SourceLocation loc)
{
	/**
	 * @brief 读取字符串常量
	 *
	 * 格式：双引号包裹的内容，如 "hello %d"
	 * 包含开始和结束的双引号
	 */
	std::string lexeme;
	char c;

	// 读取开头的双引号
	stream->next(c);
	lexeme.push_back(c);

	// 读取字符串内容，直到遇到结束双引号或 EOF
	while (stream->next(c))
	{
		lexeme.push_back(c);
		if (c == '"')
			break;
	}

	return Token(TokenType::STRCON, lexeme, loc);
}

Token Lexer::read_operator_or_delimiter(SourceLocation loc, char first)
{
	/**
	 * @brief 读取运算符或界符
	 *
	 * 处理单字符和双字符运算符：
	 * - 双字符：>=、==、&&、||
	 * - 单字符：+、-、*、/、%、!、;、(、)、[、]、{、}、,
	 */
	std::string lexeme(1, first);

	switch (first)
	{
	case '>':
		if (stream->peek(0) == '=')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::GEQ, lexeme, loc);
		}
		return Token(TokenType::GRE, lexeme, loc);

	case '<':
		if (stream->peek(0) == '=')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::LEQ, lexeme, loc);
		}
		return Token(TokenType::LSS, lexeme, loc);

	case '!':
		if (stream->peek(0) == '=')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::NEQ, lexeme, loc);
		}
		return Token(TokenType::NOT, lexeme, loc);
	case '=':
		if (stream->peek(0) == '=')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::EQL, lexeme, loc);
		}
		return Token(TokenType::ASSIGN, lexeme, loc);

	case '&':
		if (stream->peek(0) == '&')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::AND, lexeme, loc);
		}
		return Token(TokenType::INVALID, lexeme, loc);

	case '|':
		if (stream->peek(0) == '|')
		{
			char next_c;
			stream->next(next_c);
			lexeme.push_back(next_c);
			return Token(TokenType::OR, lexeme, loc);
		}
		return Token(TokenType::INVALID, lexeme, loc);

	case '+':
		return Token(TokenType::PLUS, lexeme, loc);
	case '-':
		return Token(TokenType::MINU, lexeme, loc);
	case '*':
		return Token(TokenType::MULT, lexeme, loc);
	case '/':
		return Token(TokenType::DIV, lexeme, loc);
	case '%':
		return Token(TokenType::MOD, lexeme, loc);
	case ';':
		return Token(TokenType::SEMICN, lexeme, loc);
	case '(':
		return Token(TokenType::LPARENT, lexeme, loc);
	case ')':
		return Token(TokenType::RPARENT, lexeme, loc);
	case '[':
		return Token(TokenType::LBRACK, lexeme, loc);
	case ']':
		return Token(TokenType::RBRACK, lexeme, loc);
	case '{':
		return Token(TokenType::LBRACE, lexeme, loc);
	case '}':
		return Token(TokenType::RBRACE, lexeme, loc);
	case ',':
		return Token(TokenType::COMMA, lexeme, loc);

	default:
		return Token(TokenType::INVALID, lexeme, loc);
	}
}

// ============================================================================
// 核心词法分析逻辑
// ============================================================================

bool Lexer::skip_comment()
{
	char next_c = stream->peek(0);
	if (next_c == '/')
	{
		// 单行注释
		char c;
		stream->next(c); // 消耗第二个 /
		while (stream->next(c))
		{
			if (c == '\n')
				break;
		}
		return true;
	}
	else if (next_c == '*')
	{
		// 多行注释
		char c;
		stream->next(c); // 消耗 *
		while (stream->next(c))
		{
			if (c == '*' && stream->peek(0) == '/')
			{
				stream->next(c); // 消耗 /
				break;
			}
		}
		return true;
	}
	return false;
}

Token Lexer::next_token_impl()
{
	/**
	 * @brief 获取下一个 token 的核心实现
	 *
	 * 状态机流程：
	 * 1. 读取字符
	 * 2. 跳过空白
	 * 3. 根据首字符判断 token 类型
	 * 4. 调用相应的读取函数
	 */
	char c;
	SourceLocation loc;

	while (stream->next(c))
	{
		loc = stream->current_location();

		// 跳过空白字符
		if (char_util::is_whitespace(c))
			continue;

		// 标识符或保留字（以字母或下划线开头）
		if (char_util::is_identifier_start(c))
		{
			stream->unget();
			return read_identifier(loc);
		}

		// 数字（整数常量）
		if (char_util::is_digit(c))
		{
			stream->unget();
			return read_integer(loc);
		}

		// 字符串常量（以双引号开头）
		if (c == '"')
		{
			stream->unget();
			return read_string(loc);
		}

		// 运算符和界符
		if (c == '/' && skip_comment())
		{
			continue;
		}

		return read_operator_or_delimiter(loc, c);
	}

	// EOF
	return Token(TokenType::END_OF_FILE, "", stream->current_location());
}

Token Lexer::next_token()
{
	/**
	 * @brief 获取下一个 token（公共接口）
	 *
	 * 缓存结果到 m_current_token，支持多次访问当前 token
	 */
	m_current_token = next_token_impl();
	has_current = true;
	return m_current_token;
}

void Lexer::reset()
{
	/**
	 * @brief 重置词法分析器
	 *
	 * 重新打开文件，将流位置重置到开头
	 * 用于需要重新扫描的场景
	 */
	stream = std::unique_ptr<Stream<char>>(new Stream<char>(stream->get_filename()));
	has_current = false;
}
