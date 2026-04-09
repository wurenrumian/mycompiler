/**
 * @file Ast.cpp
 * @brief 抽象语法树 (AST) 节点实现
 *
 * 本文件实现了 Ast.h 中声明的 AST 管理函数。
 *
 * 实现细节：
 * - 使用文件级静态变量 g_root 作为全局唯一的 AST 根节点
 * - 通过智能指针管理内存，避免内存泄漏
 * - ensure_root() 辅助函数确保根节点在首次访问时被创建
 *
 * 线程安全说明：
 * - 当前实现是非线程安全的
 * - 如需多线程支持，需添加同步机制或使用 thread_local
 */

#include "Ast.h"

namespace ast
{
namespace
{
/** @brief 全局 AST 根节点智能指针 */
std::unique_ptr<CompUnit> g_root;

/**
 * @brief 确保根节点存在
 *
 * 如果根节点尚未创建，则创建一个新的空 CompUnit。
 * 此函数保证后续对 g_root 的访问是安全的。
 *
 * @return 对根节点的引用
 */
CompUnit &ensure_root()
{
	if (!g_root)
	{
		g_root.reset(new CompUnit());
	}
	return *g_root;
}
} // namespace

void reset_ast_root()
{
	/** 重置根节点，释放之前的 AST 内存 */
	g_root.reset();
}

void push_ast_item(ItemKind kind, const std::string &name)
{
	/** 将新的顶层元素添加到 AST 列表末尾 */
	ensure_root().items.push_back(Item(kind, name));
}

std::unique_ptr<CompUnit> take_ast_root()
{
	/** 转移所有权，返回智能指针并清空内部状态 */
	return std::move(g_root);
}

const CompUnit *peek_ast_root()
{
	/** 返回原始指针，不转移所有权 (供调试/测试使用) */
	return g_root.get();
}
} // namespace ast
