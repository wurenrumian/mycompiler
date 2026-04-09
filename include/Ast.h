#pragma once

/**
 * @file Ast.h
 * @brief 抽象语法树 (Abstract Syntax Tree, AST) 节点定义
 *
 * 本模块定义了编译器前端的 AST 数据结构。
 * AST 是词法分析和语法分析完成后，源代码的树状中间表示形式。
 *
 * 设计说明：
 * - CompUnit (编译单元) 是 AST 的根节点，包含多个顶层声明项
 * - ItemKind 枚举区分声明、函数定义和主函数定义
 * - Item 结构体存储每种顶层元素的类型和名称信息
 *
 * 使用场景：
 * - Bison 语法分析器在归约过程中构建 AST 节点
 * - 语义分析器遍历 AST 进行类型检查和符号表管理
 * - 代码生成器基于 AST 生成目标代码
 */

#include <memory>
#include <string>
#include <vector>

namespace ast
{
/**
 * @brief 顶层元素的种类枚举
 *
 * 用于区分编译单元中的不同类型顶层元素：
 * - Decl: 全局变量/常量声明
 * - FuncDef: 普通函数定义
 * - MainFuncDef: 主函数定义 (必须恰好有一个)
 */
enum class ItemKind
{
	Decl,       /**< 全局声明 (变量/常量) */
	FuncDef,    /**< 函数定义 */
	MainFuncDef /**< 主函数定义 (int main) */
};

/**
 * @brief 顶层元素结构体
 *
 * 表示编译单元中的一个顶层元素，
 * 包含元素种类和名称信息。
 */
struct Item
{
	ItemKind kind;     /**< 元素种类 */
	std::string name;  /**< 元素名称 (MainFuncDef 固定为 "main") */

	Item(ItemKind item_kind, const std::string &item_name)
		: kind(item_kind), name(item_name)
	{
	}
};

/**
 * @brief 编译单元结构体 (AST 根节点)
 *
 * 表示一个完整的 SysY 程序，包含所有的顶层元素列表。
 * SysY 语法要求： CompUnit → {Decl} {FuncDef} MainFuncDef
 */
struct CompUnit
{
	std::vector<Item> items;  /**< 顶层元素列表 */
};

/**
 * @brief 重置 AST 根节点
 *
 * 在开始新的解析前调用，清空全局 AST 根节点，
 * 避免残留之前解析的结果。
 */
void reset_ast_root();

/**
 * @brief 向 AST 添加一个顶层元素
 *
 * Bison 语法分析器在归约 Unit 产生式时调用此函数，
 * 将新的声明、函数定义或主函数添加到 AST 中。
 *
 * @param kind 元素的种类
 * @param name 元素的名称 (主函数固定传 "main")
 */
void push_ast_item(ItemKind kind, const std::string &name);

/**
 * @brief 获取并转移 AST 根节点所有权
 *
 * 解析完成后调用此函数获取唯一的 AST 根节点指针。
 * 调用后内部状态被清空，调用者获得 AST 的完全所有权。
 *
 * @return 唯一的 AST 根节点智能指针
 */
std::unique_ptr<CompUnit> take_ast_root();

/**
 * @brief 窥视 AST 根节点 (不转移所有权)
 *
 * 用于调试和测试，在不转移所有权的情况下查看 AST。
 *
 * @return AST 根节点的原始指针，如果不存在则返回 nullptr
 */
const CompUnit *peek_ast_root();
} // namespace ast
