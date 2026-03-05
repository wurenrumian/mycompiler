// tests/test_common.cpp
// 测试 common.h 中的功能 - 使用 assert 的简单测试

#include <iostream>
#include <cassert>
#include "common.h"

// ============================================
// SourceLocation 测试
// ============================================

void test_source_location_default() {
    SourceLocation loc;
    assert(loc.line == 1);
    assert(loc.column == 1);
    assert(loc.filename.empty());
    std::cout << "[PASS] SourceLocation default construction\n";
}

void test_source_location_param() {
    SourceLocation loc(10, 20, "test.cmm");
    assert(loc.line == 10);
    assert(loc.column == 20);
    assert(loc.filename == "test.cmm");
    std::cout << "[PASS] SourceLocation parameterized construction\n";
}

void test_source_location_to_string_no_filename() {
    SourceLocation loc(5, 3);
    assert(loc.to_string() == "5:3");
    std::cout << "[PASS] Location to_string without filename\n";
}

void test_source_location_to_string_with_filename() {
    SourceLocation loc(10, 15, "example.cmm");
    assert(loc.to_string() == "example.cmm:10:15");
    std::cout << "[PASS] Location to_string with filename\n";
}

// ============================================
// SourceSpan 测试
// ============================================

void test_source_span_same_line() {
    SourceLocation start(5, 1, "test.cmm");
    SourceLocation end(5, 10, "test.cmm");
    SourceSpan span{start, end};
    assert(span.to_string() == "test.cmm:5:1-10");
    std::cout << "[PASS] SourceSpan same line\n";
}

void test_source_span_different_lines() {
    SourceLocation start(5, 1, "test.cmm");
    SourceLocation end(7, 3, "test.cmm");
    SourceSpan span{start, end};
    assert(span.to_string() == "test.cmm:5:1-7:3");
    std::cout << "[PASS] SourceSpan different lines\n";
}

void test_source_span_different_filenames() {
    SourceLocation start(1, 1, "file1.cmm");
    SourceLocation end(10, 5, "file2.cmm");
    SourceSpan span{start, end};
    assert(span.to_string() == "file1.cmm:1:1-file2.cmm:10:5");
    std::cout << "[PASS] SourceSpan different filenames\n";
}

// ============================================
// char_util 命名空间测试
// ============================================

void test_char_util_is_digit() {
    assert(char_util::is_digit('0'));
    assert(char_util::is_digit('5'));
    assert(char_util::is_digit('9'));
    assert(!char_util::is_digit('a'));
    assert(!char_util::is_digit('-'));
    std::cout << "[PASS] char_util::is_digit\n";
}

void test_char_util_is_letter() {
    assert(char_util::is_letter('a'));
    assert(char_util::is_letter('z'));
    assert(char_util::is_letter('A'));
    assert(char_util::is_letter('Z'));
    assert(!char_util::is_letter('0'));
    assert(!char_util::is_letter('_'));
    std::cout << "[PASS] char_util::is_letter\n";
}

void test_char_util_is_identifier_start() {
    assert(char_util::is_identifier_start('a'));
    assert(char_util::is_identifier_start('Z'));
    assert(char_util::is_identifier_start('_'));
    assert(!char_util::is_identifier_start('0'));
    assert(!char_util::is_identifier_start('9'));
    std::cout << "[PASS] char_util::is_identifier_start\n";
}

void test_char_util_is_identifier_part() {
    assert(char_util::is_identifier_part('a'));
    assert(char_util::is_identifier_part('Z'));
    assert(char_util::is_identifier_part('_'));
    assert(char_util::is_identifier_part('0'));
    assert(char_util::is_identifier_part('9'));
    assert(!char_util::is_identifier_part('-'));
    assert(!char_util::is_identifier_part(' '));
    std::cout << "[PASS] char_util::is_identifier_part\n";
}

void test_char_util_is_whitespace() {
    assert(char_util::is_whitespace(' '));
    assert(char_util::is_whitespace('\t'));
    assert(char_util::is_whitespace('\n'));
    assert(char_util::is_whitespace('\r'));
    assert(!char_util::is_whitespace('a'));
    assert(!char_util::is_whitespace('0'));
    std::cout << "[PASS] char_util::is_whitespace\n";
}

void test_char_util_is_hex_digit() {
    assert(char_util::is_hex_digit('0'));
    assert(char_util::is_hex_digit('a'));
    assert(char_util::is_hex_digit('f'));
    assert(char_util::is_hex_digit('A'));
    assert(char_util::is_hex_digit('F'));
    assert(char_util::is_hex_digit('9'));
    assert(!char_util::is_hex_digit('g'));
    assert(!char_util::is_hex_digit('z'));
    std::cout << "[PASS] char_util::is_hex_digit\n";
}

void test_char_util_to_lower() {
    assert(char_util::to_lower('A') == 'a');
    assert(char_util::to_lower('Z') == 'z');
    assert(char_util::to_lower('a') == 'a');
    assert(char_util::to_lower('0') == '0');
    std::cout << "[PASS] char_util::to_lower\n";
}

void test_char_util_to_lower_string() {
    std::string s1 = "Hello World";
    std::string r1 = char_util::to_lower_string(s1);
    assert(r1 == "hello world");

    std::string s2 = "ABC123";
    std::string r2 = char_util::to_lower_string(s2);
    assert(r2 == "abc123");
    std::cout << "[PASS] char_util::to_lower_string\n";
}

// ============================================
// 错误消息格式化测试
// ============================================

void test_format_error_message() {
    SourceLocation loc(10, 5, "test.cmm");
    std::string msg = format_error_message(loc, "syntax error");
    assert(msg == "test.cmm:10:5: error: syntax error");
    std::cout << "[PASS] format_error_message\n";
}

void test_format_warning_message() {
    SourceLocation loc(3, 1);
    std::string msg = format_warning_message(loc, "unused variable");
    assert(msg == "3:1: warning: unused variable");
    std::cout << "[PASS] format_warning_message\n";
}

// ============================================
// 主函数
// ============================================

int main() {
    std::cout << "Running common.h tests...\n\n";

    test_source_location_default();
    test_source_location_param();
    test_source_location_to_string_no_filename();
    test_source_location_to_string_with_filename();

    test_source_span_same_line();
    test_source_span_different_lines();
    test_source_span_different_filenames();

    test_char_util_is_digit();
    test_char_util_is_letter();
    test_char_util_is_identifier_start();
    test_char_util_is_identifier_part();
    test_char_util_is_whitespace();
    test_char_util_is_hex_digit();
    test_char_util_to_lower();
    test_char_util_to_lower_string();

    test_format_error_message();
    test_format_warning_message();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
