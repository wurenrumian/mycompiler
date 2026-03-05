#pragma once

#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>

// ============================================
// SourceLocation: 源码位置追踪（用于错误报告）
// ============================================
struct SourceLocation {
    int line = 1;           // 行号（从 1 开始）
    int column = 1;         // 列号（从 1 开始）
    std::string filename;   // 文件名（可选）

    SourceLocation() = default;
    SourceLocation(int line, int column, std::string filename = "")
        : line(line), column(column), filename(std::move(filename)) {}

    // 转换为可读字符串，如 "file.cmm:10:5"
    std::string to_string() const {
        std::ostringstream oss;
        if (!filename.empty()) oss << filename << ":";
        oss << line << ":" << column;
        return oss.str();
    }
};

// ============================================
// SourceSpan: 位置范围（token 起始-结束）
// ============================================
struct SourceSpan {
    SourceLocation start;
    SourceLocation end;

    std::string to_string() const {
        if (start.filename != end.filename) {
            return start.to_string() + "-" + end.to_string();
        }
        if (start.line == end.line) {
            return start.filename + ":" + std::to_string(start.line) +
                   ":" + std::to_string(start.column) + "-" + std::to_string(end.column);
        }
        return start.to_string() + "-" + end.to_string();
    }
};

// ============================================
// char_util: 字符分类工具（constexpr 版本）
// ============================================
namespace char_util {

// 判断是否为数字字符（0-9）
constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

// 判断是否为字母（a-z, A-Z）
constexpr bool is_letter(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// 判断是否为标识符首字符（字母或下划线）
constexpr bool is_identifier_start(char c) noexcept {
    return is_letter(c) || c == '_';
}

// 判断是否为标识符后续字符（字母、数字、下划线）
constexpr bool is_identifier_part(char c) noexcept {
    return is_identifier_start(c) || is_digit(c);
}

// 判断是否为空白字符（空格、制表符、换行、回车）
constexpr bool is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// 判断是否为十六进制数字
constexpr bool is_hex_digit(char c) noexcept {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// 字符转小写（constexpr）
constexpr char to_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// 字符串转小写（返回新字符串）
inline std::string to_lower_string(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) result.push_back(to_lower(c));
    return result;
}

} // namespace char_util

// ============================================
// 错误报告辅助函数
// ============================================
// 格式化错误消息：位置 + 消息
inline std::string format_error_message(const SourceLocation& loc, const std::string& msg) {
    return loc.to_string() + ": error: " + msg;
}

// 格式化警告消息
inline std::string format_warning_message(const SourceLocation& loc, const std::string& msg) {
    return loc.to_string() + ": warning: " + msg;
}