#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ast
{
enum class ItemKind
{
	Decl,
	FuncDef,
	MainFuncDef
};

struct Item
{
	ItemKind kind;
	std::string name;

	Item(ItemKind item_kind, const std::string &item_name)
		: kind(item_kind), name(item_name)
	{
	}
};

struct CompUnit
{
	std::vector<Item> items;
};

void reset_ast_root();
void push_ast_item(ItemKind kind, const std::string &name);
std::unique_ptr<CompUnit> take_ast_root();
const CompUnit *peek_ast_root();
} // namespace ast
