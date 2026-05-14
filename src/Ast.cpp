#include "Ast.h"

#include <cstdlib>

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

template <typename T>
std::unique_ptr<T> take_ptr(T *value)
{
	return std::unique_ptr<T>(value);
}
} // namespace

Type::Type() : kind(TypeKind::Void), dimensions(), is_array_param(false)
{
}

InitVal::InitVal() : is_list(false), expr(), elements()
{
}

VarDef::VarDef() : name(), dimensions(), has_init(false), init()
{
}

Decl::Decl() : is_const(false), base_type(Type::int_type()), defs()
{
}

Param::Param() : type(Type::int_type()), name(), array_dimensions_after_first()
{
}

FunctionDef::FunctionDef() : return_type(Type::int_type()), name(), params(), body()
{
}

IntLiteral::IntLiteral() : value(0)
{
	inferred_type = Type::int_type();
}

FloatLiteral::FloatLiteral() : value(0.0), raw()
{
	inferred_type = Type::float_type();
}

UnaryExpr::UnaryExpr() : op(UnaryOp::Plus), operand()
{
}

BinaryExpr::BinaryExpr() : op(BinaryOp::Add), lhs(), rhs()
{
}

Type Type::void_type()
{
	Type type;
	type.kind = TypeKind::Void;
	return type;
}

Type Type::int_type()
{
	Type type;
	type.kind = TypeKind::Int;
	return type;
}

Type Type::float_type()
{
	Type type;
	type.kind = TypeKind::Float;
	return type;
}

std::unique_ptr<FunctionDef> make_function(const Type &return_type,
					   const std::string &name,
					   std::vector<Param> params,
					   std::unique_ptr<Block> body)
{
	std::unique_ptr<FunctionDef> function(new FunctionDef());
	function->return_type = return_type;
	function->name = name;
	function->params = std::move(params);
	function->body = std::move(body);
	return function;
}

std::unique_ptr<Block> make_block(std::vector<std::unique_ptr<BlockItem> > items)
{
	std::unique_ptr<Block> block(new Block());
	block->items = std::move(items);
	return block;
}

Expr *make_int_literal(const std::string &text)
{
	std::unique_ptr<IntLiteral> literal(new IntLiteral());
	literal->value = std::strtoll(text.c_str(), static_cast<char **>(0), 0);
	return literal.release();
}

Expr *make_float_literal(const std::string &text)
{
	std::unique_ptr<FloatLiteral> literal(new FloatLiteral());
	literal->raw = text;
	literal->value = std::strtod(text.c_str(), static_cast<char **>(0));
	return literal.release();
}

LValueExpr *make_lvalue(const std::string &name, std::vector<Expr *> *indices)
{
	std::unique_ptr<LValueExpr> lvalue(new LValueExpr());
	lvalue->name = name;
	if (indices != 0)
	{
		for (size_t i = 0; i < indices->size(); ++i)
		{
			lvalue->indices.push_back(take_ptr((*indices)[i]));
		}
		delete indices;
	}
	return lvalue.release();
}

Expr *make_call(const std::string &callee, std::vector<Expr *> *args)
{
	std::unique_ptr<CallExpr> call(new CallExpr());
	call->callee = callee;
	if (args != 0)
	{
		for (size_t i = 0; i < args->size(); ++i)
		{
			call->args.push_back(take_ptr((*args)[i]));
		}
		delete args;
	}
	return call.release();
}

Expr *make_unary(UnaryOp op, Expr *operand)
{
	std::unique_ptr<UnaryExpr> expression(new UnaryExpr());
	expression->op = op;
	expression->operand.reset(operand);
	return expression.release();
}

Expr *make_binary(BinaryOp op, Expr *lhs, Expr *rhs)
{
	std::unique_ptr<BinaryExpr> expression(new BinaryExpr());
	expression->op = op;
	expression->lhs.reset(lhs);
	expression->rhs.reset(rhs);
	return expression.release();
}

InitVal *make_init_expr(Expr *expr)
{
	std::unique_ptr<InitVal> init(new InitVal());
	init->is_list = false;
	init->expr.reset(expr);
	return init.release();
}

InitVal *make_init_list(std::vector<InitVal *> *elements)
{
	std::unique_ptr<InitVal> init(new InitVal());
	init->is_list = true;
	if (elements != 0)
	{
		for (size_t i = 0; i < elements->size(); ++i)
		{
			init->elements.push_back(std::move(*(*elements)[i]));
			delete (*elements)[i];
		}
		delete elements;
	}
	return init.release();
}

VarDef *make_var_def(const std::string &name, std::vector<Expr *> *dimensions, InitVal *init)
{
	std::unique_ptr<VarDef> def(new VarDef());
	def->name = name;
	if (dimensions != 0)
	{
		for (size_t i = 0; i < dimensions->size(); ++i)
		{
			def->dimensions.push_back(take_ptr((*dimensions)[i]));
		}
		delete dimensions;
	}
	if (init != 0)
	{
		def->has_init = true;
		def->init = std::move(*init);
		delete init;
	}
	return def.release();
}

Decl *make_decl(bool is_const, Type *base_type, std::vector<VarDef *> *defs)
{
	std::unique_ptr<Decl> decl(new Decl());
	decl->is_const = is_const;
	if (base_type != 0)
	{
		decl->base_type = *base_type;
		delete base_type;
	}
	if (defs != 0)
	{
		for (size_t i = 0; i < defs->size(); ++i)
		{
			decl->defs.push_back(std::move(*(*defs)[i]));
			delete (*defs)[i];
		}
		delete defs;
	}
	return decl.release();
}

Param *make_param(Type *type, const std::string &name, bool is_array_param, std::vector<Expr *> *dimensions_after_first)
{
	std::unique_ptr<Param> param(new Param());
	if (type != 0)
	{
		param->type = *type;
		delete type;
	}
	param->type.is_array_param = is_array_param;
	param->name = name;
	if (dimensions_after_first != 0)
	{
		for (size_t i = 0; i < dimensions_after_first->size(); ++i)
		{
			param->array_dimensions_after_first.push_back(take_ptr((*dimensions_after_first)[i]));
		}
		delete dimensions_after_first;
	}
	return param.release();
}

Block *make_block_node(std::vector<BlockItem *> *items)
{
	std::unique_ptr<Block> block(new Block());
	if (items != 0)
	{
		for (size_t i = 0; i < items->size(); ++i)
		{
			block->items.push_back(take_ptr((*items)[i]));
		}
		delete items;
	}
	return block.release();
}

Stmt *make_expr_stmt(Expr *expr)
{
	std::unique_ptr<ExprStmt> stmt(new ExprStmt());
	stmt->expr.reset(expr);
	return stmt.release();
}

Stmt *make_assign_stmt(LValueExpr *target, Expr *value)
{
	std::unique_ptr<AssignStmt> stmt(new AssignStmt());
	stmt->target.reset(target);
	stmt->value.reset(value);
	return stmt.release();
}

Stmt *make_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch)
{
	std::unique_ptr<IfStmt> stmt(new IfStmt());
	stmt->condition.reset(condition);
	stmt->then_branch.reset(then_branch);
	stmt->else_branch.reset(else_branch);
	return stmt.release();
}

Stmt *make_while_stmt(Expr *condition, Stmt *body)
{
	std::unique_ptr<WhileStmt> stmt(new WhileStmt());
	stmt->condition.reset(condition);
	stmt->body.reset(body);
	return stmt.release();
}

Stmt *make_break_stmt()
{
	return new BreakStmt();
}

Stmt *make_continue_stmt()
{
	return new ContinueStmt();
}

Stmt *make_return_stmt(Expr *value)
{
	std::unique_ptr<ReturnStmt> stmt(new ReturnStmt());
	stmt->value.reset(value);
	return stmt.release();
}

Stmt *make_empty_stmt()
{
	return new EmptyStmt();
}

Stmt *make_format_output_stmt(const std::string &format, std::vector<Expr *> *args)
{
	std::unique_ptr<FormatOutputStmt> stmt(new FormatOutputStmt());
	stmt->format = format;
	if (args != 0)
	{
		for (size_t i = 0; i < args->size(); ++i)
		{
			stmt->args.push_back(take_ptr((*args)[i]));
		}
		delete args;
	}
	return stmt.release();
}

FunctionDef *make_function_node(Type *return_type, const std::string &name, std::vector<Param *> *params, Block *body)
{
	std::unique_ptr<FunctionDef> function(new FunctionDef());
	if (return_type != 0)
	{
		function->return_type = *return_type;
		delete return_type;
	}
	function->name = name;
	if (params != 0)
	{
		for (size_t i = 0; i < params->size(); ++i)
		{
			function->params.push_back(std::move(*(*params)[i]));
			delete (*params)[i];
		}
		delete params;
	}
	function->body.reset(body);
	return function.release();
}

void push_ast_node(Node *node)
{
	if (node == 0)
	{
		return;
	}
	ensure_root().items.push_back(take_ptr(node));
}

void reset_ast_root()
{
	g_root.reset();
}

void push_ast_item(ItemKind kind, const std::string &name)
{
	if (kind == ItemKind::Decl)
	{
		push_ast_node(new Decl());
		return;
	}

	push_ast_node(make_function_node(0, name, 0, make_block_node(0)));
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
