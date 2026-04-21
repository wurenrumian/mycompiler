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

Token Lexer::read_number(SourceLocation loc)
{
	/**
	 * @brief 读取数字常量（整数或浮点）
	 *
	 * 支持：
	 * - 整数：十进制、八进制风格、十六进制（0x...）
	 * - 浮点：3.14, .5, 1., 1e-3, 2.5E+2
	 */
	std::string lexeme;
	char c;

	stream->next(c);
	lexeme.push_back(c);

	// .5 形式
	if (c == '.')
	{
		while (char_util::is_digit(stream->peek(0)))
		{
			char d;
			stream->next(d);
			lexeme.push_back(d);
		}
	}
	else
	{
		// 整数部分
		while (char_util::is_digit(stream->peek(0)))
		{
			char d;
			stream->next(d);
			lexeme.push_back(d);
		}

		// 十六进制整数（仅 0x/0X）
		if (lexeme == "0" && (stream->peek(0) == 'x' || stream->peek(0) == 'X'))
		{
			char xchar;
			stream->next(xchar);
			lexeme.push_back(xchar);
			while (char_util::is_hex_digit(stream->peek(0)))
			{
				char hex_char;
				stream->next(hex_char);
				lexeme.push_back(hex_char);
			}
			return Token(TokenType::INTCON, lexeme, loc);
		}

		// 小数部分
		if (stream->peek(0) == '.')
		{
			char dot_char;
			stream->next(dot_char);
			lexeme.push_back(dot_char);
			while (char_util::is_digit(stream->peek(0)))
			{
				char frac_char;
				stream->next(frac_char);
				lexeme.push_back(frac_char);
			}
		}
	}

	// 指数部分
	if (stream->peek(0) == 'e' || stream->peek(0) == 'E')
	{
		char exponent_marker = stream->peek(0);
		char sign_char = stream->peek(1);
		size_t digit_offset = 1;
		if (sign_char == '+' || sign_char == '-')
		{
			digit_offset = 2;
		}
		if (char_util::is_digit(stream->peek(digit_offset)))
		{
			char e_char;
			stream->next(e_char);
			lexeme.push_back(e_char);
			if (sign_char == '+' || sign_char == '-')
			{
				char s_char;
				stream->next(s_char);
				lexeme.push_back(s_char);
			}
			while (char_util::is_digit(stream->peek(0)))
			{
				char exp_digit;
				stream->next(exp_digit);
				lexeme.push_back(exp_digit);
			}
			(void)exponent_marker;
		}
	}

	if (lexeme.find('.') != std::string::npos ||
		lexeme.find('e') != std::string::npos ||
		lexeme.find('E') != std::string::npos)
	{
		return Token(TokenType::FLOATCON, lexeme, loc);
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

		// 数字（整数/浮点）
		if (char_util::is_digit(c))
		{
			stream->unget();
			return read_number(loc);
		}

		// 以小数点开头的浮点数（.5）
		if (c == '.' && char_util::is_digit(stream->peek(0)))
		{
			stream->unget();
			return read_number(loc);
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
