#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <functional>
#include "common.h"

/**
 * @brief 通用流式缓冲区模板类
 *
 * 提供向前查看、回退、位置追踪的流式数据访问
 * 支持文件流、字符串流等多种数据源
 *
 * @tparam T 缓冲区元素类型（通常为 char 或 unsigned char）
 */
template <typename T>
class Stream
{
private:
	std::string filename;
	std::basic_ifstream<T> file;
	std::vector<T> buffer;
	size_t pos = 0; // 当前读取位置
	int line = 1;	// 当前行号（仅对字符流有意义）
	int column = 1; // 当前列号（仅对字符流有意义）
	bool eof = false;
	static constexpr size_t BUFFER_SIZE = 65536; // 增大缓冲区以减少填充次数，并支持更长的 peek

	// 填充缓冲区
	bool fill_buffer()
	{
		if (eof)
			return false;
		buffer.clear();
		buffer.resize(BUFFER_SIZE);
		file.read(&buffer[0], BUFFER_SIZE);
		size_t count = file.gcount();
		buffer.resize(count);
		pos = 0;

		if (count == 0)
		{
			eof = true;
			return false;
		}
		return true;
	}

public:
	/**
	 * @brief 构造函数（从文件）
	 */
	explicit Stream(const std::string &fname)
		: filename(fname)
	{
		file.open(filename, std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Cannot open file: " + filename);
		}
	}

	/**
	 * @brief 构造函数（从向量数据）
	 */
	explicit Stream(const std::vector<T> &data)
		: buffer(data), eof(data.empty())
	{
	}

	/**
	 * @brief 析构函数
	 */
	~Stream()
	{
		if (file.is_open())
			file.close();
	}

	// 禁止拷贝
	Stream(const Stream &) = delete;
	Stream &operator=(const Stream &) = delete;

	// 允许移动
	Stream(Stream &&) = default;
	Stream &operator=(Stream &&) = default;

	/**
	 * @brief 获取当前位置信息
	 */
	SourceLocation current_location() const
	{
		return SourceLocation(line, column, filename);
	}

	/**
	 * @brief 读取下一个元素
	 * @return true 成功读取，false 到达末尾
	 */
	bool next(T &out)
	{
		if (eof)
			return false;

		// 缓冲区已读完，重新填充
		if (pos >= buffer.size())
		{
			if (!fill_buffer())
				return false;
		}

		out = buffer[pos++];

		// 仅对字符类型更新行号列号
		if (std::is_same<T, char>::value || std::is_same<T, unsigned char>::value)
		{
			// 处理 \r\n 作为单个换行符（Windows 换行符）
			if (out == '\r')
			{
				// 检查下一个字符是否为 \n
				if (pos < buffer.size() && buffer[pos] == '\n')
				{
					pos++;		// 跳过 \n
					out = '\n'; // 将 \r\n 统一视为 \n
				}
				line++;
				column = 1;
			}
			else if (out == '\n')
			{
				line++;
				column = 1;
			}
			else
			{
				column++;
			}
		}

		return true;
	}

	/**
	 * @brief 向前查看 n 个元素（不移动位置）
	 */
	T peek(size_t n = 0)
	{
		if (eof)
			return T{};

		// 如果请求的位置超出当前缓冲区，尝试填充
		if (pos + n >= buffer.size())
		{
			// 如果已经到达文件末尾，无法再填充
			if (file.eof())
			{
				return T{};
			}

			// 简单的跨缓冲区 peek 实现：
			// 将当前缓冲区未读部分移动到开头，并读取更多数据
			size_t remaining = buffer.size() - pos;
			std::vector<T> new_buffer;
			new_buffer.resize(BUFFER_SIZE + remaining);

			if (remaining > 0)
			{
				for (size_t i = 0; i < remaining; ++i)
				{
					new_buffer[i] = buffer[pos + i];
				}
			}

			file.read(&new_buffer[remaining], BUFFER_SIZE);
			size_t count = file.gcount();
			new_buffer.resize(remaining + count);

			buffer = std::move(new_buffer);
			pos = 0;

			if (pos + n >= buffer.size())
			{
				return T{};
			}
		}

		return buffer[pos + n];
	}

	/**
	 * @brief 回退一个元素
	 * @note 仅支持单次回退，不支持连续回退
	 */
	void unget()
	{
		if (pos > 0)
		{
			pos--;
			// 更新位置信息（仅对字符类型）
			if (std::is_same<T, char>::value || std::is_same<T, unsigned char>::value)
			{
				if (pos < buffer.size())
				{
					T c = buffer[pos];
					if (c == '\n')
					{
						line--;
						// 计算上一行列号
						size_t line_start = 0;
						for (size_t i = 0; i < pos; i++)
						{
							if (buffer[i] == '\n')
								line_start = i + 1;
						}
						column = (pos - line_start) + 1;
					}
					else
					{
						column--;
					}
				}
			}
		}
	}

	/**
	 * @brief 判断是否到达末尾
	 */
	bool is_eof() const { return eof; }

	/**
	 * @brief 获取文件名
	 */
	const std::string &get_filename() const { return filename; }

	/**
	 * @brief 重置行号
	 */
	void reset_line(int line_no = 1)
	{
		line = line_no;
		column = 1;
	}

	/**
	 * @brief 获取当前行号
	 */
	int get_line() const { return line; }

	/**
	 * @brief 获取当前列号
	 */
	int get_column() const { return column; }
};

// 类型别名
using CharStream = Stream<char>;
using ByteStream = Stream<unsigned char>;
