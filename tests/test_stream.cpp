/**
 * @file test_stream.cpp
 * @brief Stream 类单元测试
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include "../include/Stream.h"

void test_stream_from_file()
{
	// 创建临时测试文件
	std::ofstream temp("temp_test.txt", std::ios::binary);
	temp << "Hello\nWorld\n123";
	temp.close();

	// 测试文件流
	Stream<char> stream(std::string("temp_test.txt"));

	char c;
	assert(stream.next(c) && c == 'H');
	assert(stream.next(c) && c == 'e');
	assert(stream.next(c) && c == 'l');
	assert(stream.next(c) && c == 'l');
	assert(stream.next(c) && c == 'o');

	// 测试位置追踪
	auto loc = stream.current_location();
	assert(loc.line == 1);
	assert(loc.column == 6);

	// 跳过换行
	stream.next(c); // '\n'
	loc = stream.current_location();
	assert(loc.line == 2);
	assert(loc.column == 1);

	// 测试 peek
	assert(stream.peek(0) == 'W');
	assert(stream.peek(1) == 'o');

	// 测试 unget
	stream.unget();
	loc = stream.current_location();
	assert(loc.line == 1);
	assert(loc.column == 6);

	// 清理
	std::remove("temp_test.txt");

	std::cout << "[PASS] test_stream_from_file" << std::endl;
}

void test_stream_from_string()
{
	// 测试字符串流
	std::string data = "Test 123";
	Stream<char> stream{std::string_view(data)};

	char c;
	std::string result;
	while (stream.next(c))
	{
		result.push_back(c);
	}

	assert(result == data);

	std::cout << "[PASS] test_stream_from_string" << std::endl;
}

void test_stream_peek_unget()
{
	std::string data = "ABC";
	Stream<char> stream{std::string_view(data)};

	char c;

	// 读取 A
	assert(stream.next(c) && c == 'A');

	// peek B
	assert(stream.peek(0) == 'B');
	assert(stream.peek(1) == 'C');

	// 读取 B
	assert(stream.next(c) && c == 'B');

	// unget 回到 B
	stream.unget();
	assert(stream.peek(0) == 'B');

	// 读取 B 再次
	assert(stream.next(c) && c == 'B');

	// 读取 C
	assert(stream.next(c) && c == 'C');

	// EOF
	assert(!stream.next(c));
	assert(stream.is_eof());

	std::cout << "[PASS] test_stream_peek_unget" << std::endl;
}

int main()
{
	std::cout << "Running Stream tests..." << std::endl;

	test_stream_from_file();
	test_stream_from_string();
	test_stream_peek_unget();

	std::cout << "All Stream tests passed!" << std::endl;
	return 0;
}
