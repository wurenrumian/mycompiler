/**
 * @file IRBuilder.cpp
 * @brief LLVM IR 构建器实现
 *
 * 本文件实现了 LLVM IR 文本生成功能。
 *
 * 当前状态：
 * - 这是占位实现，用于自托管后端开发期间
 * - build_runtime_prelude() 已实现，生成运行时库函数声明
 * - build_placeholder_module() 生成带有统计信息的占位模块
 *
 * 未来计划：
 * - 将扩展为完整的 IR 发射器
 * - 支持表达式、语句、函数的完整 IR 生成
 * - 移除对 clang 的依赖
 */

#include "IRBuilder.h"

#include <sstream>

namespace ir
{
std::string build_runtime_prelude()
{
	/**
	 * @brief 生成运行时库函数声明
	 *
	 * 输出 SysY 运行时所需的 LLVM IR 函数声明：
	 * - @getchar: 从标准输入读取一个字符
	 * - @printf: 格式化输出函数
	 * - @scanf: 格式化输入函数
	 */
	std::ostringstream os;
	os << "declare i32 @getchar()\n";
	os << "declare i32 @printf(ptr, ...)\n";
	os << "declare i32 @scanf(ptr, ...)\n\n";
	return os.str();
}

std::string build_placeholder_module(const semantic::ProgramInfo &program_info)
{
	/**
	 * @brief 生成占位模块文本
	 *
	 * 用于自托管后端尚未完成时的占位输出：
	 * - 包含注释说明程序结构统计
	 * - 包含运行时库函数声明
	 */
	std::ostringstream os;
	os << "; self-hosted backend placeholder\n";
	os << "; globals=" << program_info.global_item_count << ", functions=" << program_info.function_count << "\n";
	os << build_runtime_prelude();
	return os.str();
}
} // namespace ir
