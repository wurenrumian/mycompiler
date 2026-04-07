#include "Ast.h"

namespace ast
{
namespace
{
std::unique_ptr<CompUnit> g_root;

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
	g_root.reset();
}

void push_ast_item(ItemKind kind, const std::string &name)
{
	ensure_root().items.push_back(Item(kind, name));
}

std::unique_ptr<CompUnit> take_ast_root()
{
	return std::move(g_root);
}

const CompUnit *peek_ast_root()
{
	return g_root.get();
}
} // namespace ast
