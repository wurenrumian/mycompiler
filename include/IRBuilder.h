#pragma once

/**
 * @file IRBuilder.h
 * @brief LLVM IR 构建器接口定义
 *
 * 本模块负责生成 LLVM IR (Intermediate Representation) 文本格式代码。
 *
 * LLVM IR 是 LLVM 编译框架的核心中间表示形式，
 * 采用 SSA (Static Single Assignment) 形式，
 * 是连接高级语言源码和目标机器码的桥梁。
 *
 * 设计说明：
 * - build_runtime_prelude: 生成运行时库函数的声明
 * - build_placeholder_module: 生成占位模块信息 (用于自托管后端尚未完成时)
 *
 * 未来扩展：
 * - 此模块将发展为完整的自托管 IR 发射器，
 *   替代当前依赖 clang 的代码生成方式
 */

#include <string>

#include "Semantic.h"

namespace ir
{
/**
 * @brief 生成运行时库函数声明
 *
 * 输出 SysY 内置函数的 LLVM IR 声明，
 * 包括 getchar、printf、scanf 等。
 *
 * @return 包含函数声明的 LLVM IR 文本
 */
std::string build_runtime_prelude();

/**
 * @brief 生成占位模块信息
 *
 * 用于自托管后端尚未完成时的占位实现，
 * 输出程序结构注释和运行时声明。
 *
 * @param program_info 程序结构信息
 * @return 占位模块的 LLVM IR 文本
 */
std::string build_placeholder_module(const semantic::ProgramInfo &program_info);
} // namespace ir
