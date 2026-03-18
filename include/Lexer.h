#pragma once

#include <memory>
#include "Stream.h"
#include "Token.h"

/**
 * @brief 词法分析器类
 *
 * 从 Stream<char> 读取字符流，生成 Token 流
 * 支持 CMM 语言的所有词法元素：
 * - 保留字（19个）
 * - 运算符（13个）
 * - 界符（8个）
 * - 整数常量、标识符、格式字符串
 *
 * 使用流式处理，一次只处理一个 token，内存效率高
 */
class Lexer
{
private:
	std::unique_ptr<Stream<char>> stream; // 字符流
	Token m_current_token;				  // 当前 token（m_ 前缀避免与成员函数冲突）
	bool has_current = false;			  // 是否已缓存当前 token

	// 跳过空白字符（空格、制表符、换行、回车）
	void skip_whitespace();

	// 读取标识符或保留字
	Token read_identifier(SourceLocation loc);

	// 读取整数常量（十进制）
	Token read_integer(SourceLocation loc);

	// 读取字符串常量（双引号包裹）
	Token read_string(SourceLocation loc);

	// 读取运算符或界符（处理多字符运算符如 >=、==、&&、||）
	Token read_operator_or_delimiter(SourceLocation loc, char first);

	// 处理注释
	bool skip_comment();

	// 获取下一个 token 的核心实现
	Token next_token_impl();

public:
	/**
	 * @brief 构造函数（从已创建的流）
	 * @param s 唯一指针指向 Stream<char>
	 */
	explicit Lexer(std::unique_ptr<Stream<char>> s);

	/**
	 * @brief 构造函数（从文件名）
	 * @param filename 输入文件路径
	 */
	explicit Lexer(const std::string &filename);

	/**
	 * @brief 析构函数
	 */
	~Lexer();

	/**
	 * @brief 获取下一个 token
	 * @return Token 对象
	 */
	Token next_token();

	/**
	 * @brief 获取当前 token（不移动流位置）
	 * @return 当前 token 的常量引用
	 */
	const Token &current_token() const { return m_current_token; }

	/**
	 * @brief 重置流位置到文件开头
	 */
	void reset();

	/**
	 * @brief 获取文件名
	 * @return 当前处理的文件名
	 */
	const std::string &get_filename() const { return stream->get_filename(); }
};
