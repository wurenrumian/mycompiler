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

enum class NodeKind
{
	Decl,
	FunctionDef,
	Block,
	Stmt,
	Expr
};

enum class TypeKind
{
	Void,
	Int,
	Float
};

enum class UnaryOp
{
	Plus,
	Minus,
	Not
};

enum class BinaryOp
{
	Add,
	Sub,
	Mul,
	Div,
	Mod,
	Lt,
	Gt,
	Le,
	Ge,
	Eq,
	Ne,
	And,
	Or
};

enum class StmtKind
{
	Empty,
	Expr,
	Assign,
	Block,
	If,
	While,
	Break,
	Continue,
	Return,
	FormatOutput
};

enum class ExprKind
{
	IntLiteral,
	FloatLiteral,
	LValue,
	Call,
	Unary,
	Binary
};

struct Type
{
	TypeKind kind;
	std::vector<int> dimensions;
	bool is_array_param;

	Type();

	static Type void_type();
	static Type int_type();
	static Type float_type();
};

struct Node
{
	virtual ~Node() {}
	virtual NodeKind kind() const = 0;
};

struct Expr : Node
{
	Type inferred_type;

	NodeKind kind() const override
	{
		return NodeKind::Expr;
	}

	virtual ExprKind expr_kind() const = 0;
};

struct BlockItem : Node
{
	virtual ~BlockItem() {}
};

struct Stmt : BlockItem
{
	NodeKind kind() const override
	{
		return NodeKind::Stmt;
	}

	virtual StmtKind stmt_kind() const = 0;
};

struct InitVal
{
	bool is_list;
	std::unique_ptr<Expr> expr;
	std::vector<InitVal> elements;

	InitVal();
};

struct VarDef
{
	std::string name;
	std::vector<std::unique_ptr<Expr> > dimensions;
	bool has_init;
	InitVal init;

	VarDef();
};

struct Decl : BlockItem
{
	bool is_const;
	Type base_type;
	std::vector<VarDef> defs;

	Decl();

	NodeKind kind() const override
	{
		return NodeKind::Decl;
	}
};

struct Param
{
	Type type;
	std::string name;
	std::vector<std::unique_ptr<Expr> > array_dimensions_after_first;

	Param();
};

struct Block : Stmt
{
	std::vector<std::unique_ptr<BlockItem> > items;

	StmtKind stmt_kind() const override
	{
		return StmtKind::Block;
	}

	NodeKind kind() const override
	{
		return NodeKind::Block;
	}
};

struct FunctionDef : Node
{
	Type return_type;
	std::string name;
	std::vector<Param> params;
	std::unique_ptr<Block> body;

	FunctionDef();

	NodeKind kind() const override
	{
		return NodeKind::FunctionDef;
	}
};

struct IntLiteral : Expr
{
	long long value;

	IntLiteral();

	ExprKind expr_kind() const override
	{
		return ExprKind::IntLiteral;
	}
};

struct FloatLiteral : Expr
{
	double value;
	std::string raw;

	FloatLiteral();

	ExprKind expr_kind() const override
	{
		return ExprKind::FloatLiteral;
	}
};

struct LValueExpr : Expr
{
	std::string name;
	std::vector<std::unique_ptr<Expr> > indices;

	ExprKind expr_kind() const override
	{
		return ExprKind::LValue;
	}
};

struct CallExpr : Expr
{
	std::string callee;
	std::vector<std::unique_ptr<Expr> > args;

	ExprKind expr_kind() const override
	{
		return ExprKind::Call;
	}
};

struct UnaryExpr : Expr
{
	UnaryOp op;
	std::unique_ptr<Expr> operand;

	UnaryExpr();

	ExprKind expr_kind() const override
	{
		return ExprKind::Unary;
	}
};

struct BinaryExpr : Expr
{
	BinaryOp op;
	std::unique_ptr<Expr> lhs;
	std::unique_ptr<Expr> rhs;

	BinaryExpr();

	ExprKind expr_kind() const override
	{
		return ExprKind::Binary;
	}
};

struct ExprStmt : Stmt
{
	std::unique_ptr<Expr> expr;

	StmtKind stmt_kind() const override
	{
		return StmtKind::Expr;
	}
};

struct AssignStmt : Stmt
{
	std::unique_ptr<LValueExpr> target;
	std::unique_ptr<Expr> value;

	StmtKind stmt_kind() const override
	{
		return StmtKind::Assign;
	}
};

struct IfStmt : Stmt
{
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Stmt> then_branch;
	std::unique_ptr<Stmt> else_branch;

	StmtKind stmt_kind() const override
	{
		return StmtKind::If;
	}
};

struct WhileStmt : Stmt
{
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Stmt> body;

	StmtKind stmt_kind() const override
	{
		return StmtKind::While;
	}
};

struct BreakStmt : Stmt
{
	StmtKind stmt_kind() const override
	{
		return StmtKind::Break;
	}
};

struct ContinueStmt : Stmt
{
	StmtKind stmt_kind() const override
	{
		return StmtKind::Continue;
	}
};

struct ReturnStmt : Stmt
{
	std::unique_ptr<Expr> value;

	StmtKind stmt_kind() const override
	{
		return StmtKind::Return;
	}
};

struct EmptyStmt : Stmt
{
	StmtKind stmt_kind() const override
	{
		return StmtKind::Empty;
	}
};

struct FormatOutputStmt : Stmt
{
	std::string format;
	std::vector<std::unique_ptr<Expr> > args;

	StmtKind stmt_kind() const override
	{
		return StmtKind::FormatOutput;
	}
};

struct CompUnit
{
	std::vector<std::unique_ptr<Node> > items;
};

std::unique_ptr<FunctionDef> make_function(const Type &return_type,
					   const std::string &name,
					   std::vector<Param> params,
					   std::unique_ptr<Block> body);
std::unique_ptr<Block> make_block(std::vector<std::unique_ptr<BlockItem> > items);

Expr *make_int_literal(const std::string &text);
Expr *make_float_literal(const std::string &text);
LValueExpr *make_lvalue(const std::string &name, std::vector<Expr *> *indices);
Expr *make_call(const std::string &callee, std::vector<Expr *> *args);
Expr *make_unary(UnaryOp op, Expr *operand);
Expr *make_binary(BinaryOp op, Expr *lhs, Expr *rhs);
InitVal *make_init_expr(Expr *expr);
InitVal *make_init_list(std::vector<InitVal *> *elements);
VarDef *make_var_def(const std::string &name, std::vector<Expr *> *dimensions, InitVal *init);
Decl *make_decl(bool is_const, Type *base_type, std::vector<VarDef *> *defs);
Param *make_param(Type *type, const std::string &name, bool is_array_param, std::vector<Expr *> *dimensions_after_first);
Block *make_block_node(std::vector<BlockItem *> *items);
Stmt *make_expr_stmt(Expr *expr);
Stmt *make_assign_stmt(LValueExpr *target, Expr *value);
Stmt *make_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch);
Stmt *make_while_stmt(Expr *condition, Stmt *body);
Stmt *make_break_stmt();
Stmt *make_continue_stmt();
Stmt *make_return_stmt(Expr *value);
Stmt *make_empty_stmt();
Stmt *make_format_output_stmt(const std::string &format, std::vector<Expr *> *args);
FunctionDef *make_function_node(Type *return_type, const std::string &name, std::vector<Param *> *params, Block *body);
void push_ast_node(Node *node);

void reset_ast_root();
void push_ast_item(ItemKind kind, const std::string &name);
std::unique_ptr<CompUnit> take_ast_root();
const CompUnit *peek_ast_root();
} // namespace ast
