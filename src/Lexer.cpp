#include "Lexer.h"
#include <cctype>
#include <stdexcept>

// ============================================================================
// Lexer 构造函数/析构函数
// ============================================================================

Lexer::Lexer(std::unique_ptr<Stream<char>> s)
	: stream(std::move(s))
{
}

Lexer::Lexer(const std::string &filename)
	: stream(std::make_unique<Stream<char>>(filename))
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
		return Token(type, lexeme, loc);
	}
	else
	{
		return Token(TokenType::IDENFR, lexeme, loc);
	}
}

Token Lexer::read_integer(SourceLocation loc)
{
	/**
	 * @brief 读取整数常量
	 *
	 * 格式：连续的数字字符 [0-9]+
	 * 注意：当前仅支持十进制整数
	 */
	std::string lexeme;
	char c;

	while (true)
	{
		c = stream->peek(0); // 查看下一个字符
		if (char_util::is_digit(c))
		{
			lexeme.push_back(c);
			stream->next(c);
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
			// 如果有 NEQ 类型则返回，否则根据 html 表格，! 只有 NOT
			// 检查 html 表格发现没有 != 的定义，所以这里可能不需要处理 != 或者 NEQ
			// 但为了健壮性，如果文法没定义，我们先保持原样
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
		return Token(TokenType::MIUN, lexeme, loc);
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
	stream = std::make_unique<Stream<char>>(stream->get_filename());
	has_current = false;
}
