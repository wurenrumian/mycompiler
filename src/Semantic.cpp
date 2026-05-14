#include "Semantic.h"

#include <map>
#include <string>
#include <vector>

namespace semantic
{
namespace
{
struct Symbol
{
	std::string name;
	ast::Type type;
	bool is_const;
	bool is_function;
	bool is_variadic;
	std::vector<ast::Type> params;
};

class ScopeStack
{
public:
	ScopeStack()
	{
		push();
	}

	void push()
	{
		scopes_.push_back(std::map<std::string, Symbol>());
	}

	void pop()
	{
		if (!scopes_.empty())
		{
			scopes_.pop_back();
		}
	}

	bool define(const Symbol &symbol)
	{
		if (scopes_.empty())
		{
			push();
		}
		std::map<std::string, Symbol> &scope = scopes_.back();
		if (scope.find(symbol.name) != scope.end())
		{
			return false;
		}
		scope[symbol.name] = symbol;
		return true;
	}

	const Symbol *lookup(const std::string &name) const
	{
		for (size_t i = scopes_.size(); i > 0; --i)
		{
			const std::map<std::string, Symbol> &scope = scopes_[i - 1];
			std::map<std::string, Symbol>::const_iterator found = scope.find(name);
			if (found != scope.end())
			{
				return &found->second;
			}
		}
		return 0;
	}

private:
	std::vector<std::map<std::string, Symbol> > scopes_;
};

struct Analyzer
{
	AnalyzeOptions options;
	ScopeStack scopes;
	ast::Type current_return_type;
	int loop_depth;
	int main_count;
	SemanticResult result;

	Analyzer() : options(), scopes(), current_return_type(ast::Type::void_type()), loop_depth(0), main_count(0), result() {}

	bool fail(const std::string &message)
	{
		result.ok = false;
		result.message = message;
		return false;
	}

	static ast::Type int_pointer_param()
	{
		ast::Type type = ast::Type::int_type();
		type.is_array_param = true;
		return type;
	}

	static ast::Type float_pointer_param()
	{
		ast::Type type = ast::Type::float_type();
		type.is_array_param = true;
		return type;
	}

	bool same_scalar_kind(const ast::Type &lhs, const ast::Type &rhs) const
	{
		return lhs.kind == rhs.kind;
	}

	bool is_numeric_scalar(const ast::Type &type) const
	{
		return !type.is_array_param && type.dimensions.empty() &&
		       (type.kind == ast::TypeKind::Int || type.kind == ast::TypeKind::Float);
	}

	bool is_assignable(const ast::Type &target, const ast::Type &source) const
	{
		if (target.is_array_param || !target.dimensions.empty())
		{
			return false;
		}
		if (!source.dimensions.empty() || source.is_array_param)
		{
			return false;
		}
		if (target.kind == source.kind)
		{
			return true;
		}
		return (target.kind == ast::TypeKind::Int && source.kind == ast::TypeKind::Float) ||
		       (target.kind == ast::TypeKind::Float && source.kind == ast::TypeKind::Int);
	}

	bool is_argument_compatible(const ast::Type &param, const ast::Type &arg) const
	{
		if (param.is_array_param)
		{
			return arg.is_array_param || !arg.dimensions.empty();
		}
		return is_assignable(param, arg);
	}

	void define_runtime(const std::string &name,
			    const ast::Type &return_type,
			    const std::vector<ast::Type> &params,
			    bool variadic)
	{
		Symbol symbol;
		symbol.name = name;
		symbol.type = return_type;
		symbol.is_const = false;
		symbol.is_function = true;
		symbol.is_variadic = variadic;
		symbol.params = params;
		scopes.define(symbol);
	}

	void define_runtime_functions()
	{
		define_runtime("getint", ast::Type::int_type(), std::vector<ast::Type>(), false);
		define_runtime("getch", ast::Type::int_type(), std::vector<ast::Type>(), false);
		define_runtime("getfloat", ast::Type::float_type(), std::vector<ast::Type>(), false);
		define_runtime("getarray", ast::Type::int_type(), std::vector<ast::Type>(1, int_pointer_param()), false);
		define_runtime("getfarray", ast::Type::int_type(), std::vector<ast::Type>(1, float_pointer_param()), false);
		define_runtime("putint", ast::Type::void_type(), std::vector<ast::Type>(1, ast::Type::int_type()), false);
		define_runtime("putch", ast::Type::void_type(), std::vector<ast::Type>(1, ast::Type::int_type()), false);
		define_runtime("putfloat", ast::Type::void_type(), std::vector<ast::Type>(1, ast::Type::float_type()), false);
		{
			std::vector<ast::Type> params;
			params.push_back(ast::Type::int_type());
			params.push_back(int_pointer_param());
			define_runtime("putarray", ast::Type::void_type(), params, false);
		}
		{
			std::vector<ast::Type> params;
			params.push_back(ast::Type::int_type());
			params.push_back(float_pointer_param());
			define_runtime("putfarray", ast::Type::void_type(), params, false);
		}
		define_runtime("putf", ast::Type::void_type(), std::vector<ast::Type>(), true);
		define_runtime("starttime", ast::Type::void_type(), std::vector<ast::Type>(), false);
		define_runtime("stoptime", ast::Type::void_type(), std::vector<ast::Type>(), false);
		define_runtime("_sysy_starttime", ast::Type::void_type(), std::vector<ast::Type>(1, ast::Type::int_type()), false);
		define_runtime("_sysy_stoptime", ast::Type::void_type(), std::vector<ast::Type>(1, ast::Type::int_type()), false);
	}

	bool analyze_init(const ast::InitVal &init)
	{
		if (!init.is_list)
		{
			if (init.expr.get() == 0)
			{
				return true;
			}
			ast::Type ignored;
			return analyze_expr(init.expr.get(), ignored);
		}

		for (size_t i = 0; i < init.elements.size(); ++i)
		{
			if (!analyze_init(init.elements[i]))
			{
				return false;
			}
		}
		return true;
	}

	bool analyze_decl(const ast::Decl *decl)
	{
		for (size_t i = 0; i < decl->defs.size(); ++i)
		{
			const ast::VarDef &def = decl->defs[i];
			for (size_t d = 0; d < def.dimensions.size(); ++d)
			{
				ast::Type ignored;
				if (!analyze_expr(def.dimensions[d].get(), ignored))
				{
					return false;
				}
			}

			Symbol symbol;
			symbol.name = def.name;
			symbol.type = decl->base_type;
			symbol.type.dimensions.resize(def.dimensions.size(), 0);
			symbol.is_const = decl->is_const;
			symbol.is_function = false;
			symbol.is_variadic = false;
			if (!scopes.define(symbol))
			{
				return fail("Duplicate definition: " + def.name);
			}

			if (def.has_init && !analyze_init(def.init))
			{
				return false;
			}
		}
		return true;
	}

	bool analyze_block(const ast::Block *block)
	{
		scopes.push();
		for (size_t i = 0; i < block->items.size(); ++i)
		{
			const ast::BlockItem *item = block->items[i].get();
			if (item->kind() == ast::NodeKind::Decl)
			{
				if (!analyze_decl(static_cast<const ast::Decl *>(item)))
				{
					scopes.pop();
					return false;
				}
				continue;
			}

			if (!analyze_stmt(static_cast<const ast::Stmt *>(item)))
			{
				scopes.pop();
				return false;
			}
		}
		scopes.pop();
		return true;
	}

	bool analyze_call(const ast::CallExpr *call, ast::Type &out_type)
	{
		const Symbol *symbol = scopes.lookup(call->callee);
		if (symbol == 0 || !symbol->is_function)
		{
			return fail("Undefined function: " + call->callee);
		}

		if (!symbol->is_variadic && call->args.size() != symbol->params.size())
		{
			return fail("Wrong argument count for function: " + call->callee);
		}

		if (symbol->is_variadic && call->args.size() < symbol->params.size())
		{
			return fail("Wrong argument count for variadic function: " + call->callee);
		}

		for (size_t i = 0; i < call->args.size(); ++i)
		{
			ast::Type arg_type;
			if (!analyze_expr(call->args[i].get(), arg_type))
			{
				return false;
			}
			if (i < symbol->params.size() && !is_argument_compatible(symbol->params[i], arg_type))
			{
				return fail("Argument type mismatch in call to: " + call->callee);
			}
		}

		out_type = symbol->type;
		return true;
	}

	bool analyze_expr(const ast::Expr *expr, ast::Type &out_type)
	{
		switch (expr->expr_kind())
		{
		case ast::ExprKind::IntLiteral:
			out_type = ast::Type::int_type();
			return true;
		case ast::ExprKind::FloatLiteral:
			out_type = ast::Type::float_type();
			return true;
		case ast::ExprKind::LValue:
		{
			const ast::LValueExpr *lvalue = static_cast<const ast::LValueExpr *>(expr);
			const Symbol *symbol = scopes.lookup(lvalue->name);
			if (symbol == 0)
			{
				return fail("Undefined identifier: " + lvalue->name);
			}
			for (size_t i = 0; i < lvalue->indices.size(); ++i)
			{
				ast::Type index_type;
				if (!analyze_expr(lvalue->indices[i].get(), index_type))
				{
					return false;
				}
			}
			out_type = symbol->type;
			const bool original_array_param = out_type.is_array_param;
			size_t available_indices = out_type.dimensions.size();
			if (out_type.is_array_param)
			{
				available_indices += 1;
			}
			if (lvalue->indices.size() > available_indices)
			{
				return fail("Too many indices for lvalue: " + lvalue->name);
			}

			if (out_type.is_array_param && !lvalue->indices.empty())
			{
				out_type.is_array_param = false;
				size_t remaining = lvalue->indices.size() - 1;
				if (remaining > 0 && !out_type.dimensions.empty())
				{
					out_type.dimensions.erase(out_type.dimensions.begin(),
								  out_type.dimensions.begin() + static_cast<std::ptrdiff_t>(remaining));
				}
			}
			else if (!out_type.dimensions.empty() && lvalue->indices.size() <= out_type.dimensions.size())
			{
				out_type.dimensions.erase(out_type.dimensions.begin(),
							  out_type.dimensions.begin() + static_cast<std::ptrdiff_t>(lvalue->indices.size()));
			}
			if (out_type.dimensions.empty())
			{
				out_type.is_array_param = original_array_param && lvalue->indices.empty();
			}
			return true;
		}
		case ast::ExprKind::Call:
			return analyze_call(static_cast<const ast::CallExpr *>(expr), out_type);
		case ast::ExprKind::Unary:
		{
			const ast::UnaryExpr *unary = static_cast<const ast::UnaryExpr *>(expr);
			ast::Type operand_type;
			if (!analyze_expr(unary->operand.get(), operand_type))
			{
				return false;
			}
			out_type = (unary->op == ast::UnaryOp::Not) ? ast::Type::int_type() : operand_type;
			return true;
		}
		case ast::ExprKind::Binary:
		{
			const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
			ast::Type lhs_type;
			ast::Type rhs_type;
			if (!analyze_expr(binary->lhs.get(), lhs_type) || !analyze_expr(binary->rhs.get(), rhs_type))
			{
				return false;
			}

			switch (binary->op)
			{
			case ast::BinaryOp::Add:
			case ast::BinaryOp::Sub:
			case ast::BinaryOp::Mul:
			case ast::BinaryOp::Div:
				out_type = (lhs_type.kind == ast::TypeKind::Float || rhs_type.kind == ast::TypeKind::Float)
					       ? ast::Type::float_type()
					       : ast::Type::int_type();
				return true;
			case ast::BinaryOp::Mod:
				if (lhs_type.kind != ast::TypeKind::Int || rhs_type.kind != ast::TypeKind::Int)
				{
					return fail("Modulo operands must be int.");
				}
				out_type = ast::Type::int_type();
				return true;
			case ast::BinaryOp::Lt:
			case ast::BinaryOp::Gt:
			case ast::BinaryOp::Le:
			case ast::BinaryOp::Ge:
			case ast::BinaryOp::Eq:
			case ast::BinaryOp::Ne:
			case ast::BinaryOp::And:
			case ast::BinaryOp::Or:
				out_type = ast::Type::int_type();
				return true;
			}
		}
		}

		return fail("Unknown expression kind.");
	}

	bool analyze_stmt(const ast::Stmt *stmt)
	{
		switch (stmt->stmt_kind())
		{
		case ast::StmtKind::Empty:
			return true;
		case ast::StmtKind::Expr:
		{
			const ast::ExprStmt *expr_stmt = static_cast<const ast::ExprStmt *>(stmt);
			if (expr_stmt->expr.get() == 0)
			{
				return true;
			}
			ast::Type ignored;
			return analyze_expr(expr_stmt->expr.get(), ignored);
		}
		case ast::StmtKind::Assign:
		{
			const ast::AssignStmt *assign = static_cast<const ast::AssignStmt *>(stmt);
			const Symbol *symbol = scopes.lookup(assign->target->name);
			if (symbol == 0)
			{
				return fail("Undefined identifier: " + assign->target->name);
			}
			if (symbol->is_const)
			{
				return fail("Cannot assign to const: " + assign->target->name);
			}
			ast::Type target_type = symbol->type;
			const bool original_array_param = target_type.is_array_param;
			size_t available_indices = target_type.dimensions.size();
			if (target_type.is_array_param)
			{
				available_indices += 1;
			}
			if (assign->target->indices.size() > available_indices)
			{
				return fail("Too many indices for assignment target: " + assign->target->name);
			}
			if (target_type.is_array_param && !assign->target->indices.empty())
			{
				target_type.is_array_param = false;
				size_t remaining = assign->target->indices.size() - 1;
				if (remaining > 0 && !target_type.dimensions.empty())
				{
					target_type.dimensions.erase(target_type.dimensions.begin(),
								     target_type.dimensions.begin() + static_cast<std::ptrdiff_t>(remaining));
				}
			}
			else if (!target_type.dimensions.empty() && assign->target->indices.size() <= target_type.dimensions.size())
			{
				target_type.dimensions.erase(target_type.dimensions.begin(),
							     target_type.dimensions.begin() + static_cast<std::ptrdiff_t>(assign->target->indices.size()));
			}
			if (target_type.dimensions.empty())
			{
				target_type.is_array_param = original_array_param && assign->target->indices.empty();
			}
			ast::Type value_type;
			if (!analyze_expr(assign->value.get(), value_type))
			{
				return false;
			}
			return is_assignable(target_type, value_type) || fail("Assignment type mismatch: " + assign->target->name);
		}
		case ast::StmtKind::Block:
			return analyze_block(static_cast<const ast::Block *>(stmt));
		case ast::StmtKind::If:
		{
			const ast::IfStmt *if_stmt = static_cast<const ast::IfStmt *>(stmt);
			ast::Type cond_type;
			if (!analyze_expr(if_stmt->condition.get(), cond_type))
			{
				return false;
			}
			if (!analyze_stmt(if_stmt->then_branch.get()))
			{
				return false;
			}
			return if_stmt->else_branch.get() == 0 || analyze_stmt(if_stmt->else_branch.get());
		}
		case ast::StmtKind::While:
		{
			const ast::WhileStmt *while_stmt = static_cast<const ast::WhileStmt *>(stmt);
			ast::Type cond_type;
			if (!analyze_expr(while_stmt->condition.get(), cond_type))
			{
				return false;
			}
			++loop_depth;
			const bool ok = analyze_stmt(while_stmt->body.get());
			--loop_depth;
			return ok;
		}
		case ast::StmtKind::Break:
		case ast::StmtKind::Continue:
			return loop_depth > 0 || fail("break/continue used outside loop.");
		case ast::StmtKind::Return:
		{
			const ast::ReturnStmt *ret = static_cast<const ast::ReturnStmt *>(stmt);
			if (ret->value.get() == 0)
			{
				return current_return_type.kind == ast::TypeKind::Void || fail("Non-void function must return a value.");
			}
			ast::Type value_type;
			if (!analyze_expr(ret->value.get(), value_type))
			{
				return false;
			}
			return is_assignable(current_return_type, value_type) || fail("Return type mismatch.");
		}
		case ast::StmtKind::FormatOutput:
		{
			const ast::FormatOutputStmt *output = static_cast<const ast::FormatOutputStmt *>(stmt);
			for (size_t i = 0; i < output->args.size(); ++i)
			{
				ast::Type arg_type;
				if (!analyze_expr(output->args[i].get(), arg_type))
				{
					return false;
				}
			}
			return true;
		}
		}

		return fail("Unknown statement kind.");
	}

	bool declare_function(const ast::FunctionDef *function)
	{
		Symbol symbol;
		symbol.name = function->name;
		symbol.type = function->return_type;
		symbol.is_const = false;
		symbol.is_function = true;
		symbol.is_variadic = false;
		for (size_t i = 0; i < function->params.size(); ++i)
		{
			symbol.params.push_back(function->params[i].type);
		}
		if (!scopes.define(symbol))
		{
			return fail("Duplicate function definition: " + function->name);
		}
		return true;
	}

	bool analyze_function(const ast::FunctionDef *function)
	{
		current_return_type = function->return_type;
		scopes.push();
		for (size_t i = 0; i < function->params.size(); ++i)
		{
			Symbol symbol;
			symbol.name = function->params[i].name;
			symbol.type = function->params[i].type;
			if (symbol.type.is_array_param)
			{
				symbol.type.dimensions.resize(function->params[i].array_dimensions_after_first.size(), 1);
			}
			symbol.is_const = false;
			symbol.is_function = false;
			symbol.is_variadic = false;
			if (!scopes.define(symbol))
			{
				scopes.pop();
				return fail("Duplicate parameter name: " + function->params[i].name);
			}
		}
		const bool ok = analyze_block(function->body.get());
		scopes.pop();
		return ok;
	}
};
} // namespace

SemanticResult analyze_program(const ast::CompUnit &root,
			       const AnalyzeOptions &options)
{
	Analyzer analyzer;
	analyzer.options = options;
	analyzer.define_runtime_functions();

	if (root.items.empty())
	{
		analyzer.fail("AST is empty.");
		return analyzer.result;
	}

	for (size_t i = 0; i < root.items.size(); ++i)
	{
		const ast::Node *node = root.items[i].get();
		if (node->kind() == ast::NodeKind::Decl)
		{
			if (!analyzer.analyze_decl(static_cast<const ast::Decl *>(node)))
			{
				return analyzer.result;
			}
			continue;
		}

		const ast::FunctionDef *function = static_cast<const ast::FunctionDef *>(node);
		if (function->name == "main")
		{
			++analyzer.main_count;
			if (function->return_type.kind != ast::TypeKind::Int || !function->params.empty())
			{
				analyzer.fail("main must have signature int main().");
				return analyzer.result;
			}
		}
		if (!analyzer.declare_function(function))
		{
			return analyzer.result;
		}
	}

	if (analyzer.main_count != 1)
	{
		analyzer.fail("Program must contain exactly one main function.");
		return analyzer.result;
	}

	for (size_t i = 0; i < root.items.size(); ++i)
	{
		const ast::Node *node = root.items[i].get();
		if (node->kind() != ast::NodeKind::FunctionDef)
		{
			continue;
		}
		if (!analyzer.analyze_function(static_cast<const ast::FunctionDef *>(node)))
		{
			return analyzer.result;
		}
	}

	analyzer.result.ok = true;
	return analyzer.result;
}

bool analyze_program(const ast::CompUnit &root,
		     std::string *error_message)
{
	SemanticResult result = analyze_program(root, AnalyzeOptions());
	if (!result.ok && error_message != 0)
	{
		*error_message = result.message;
	}
	return result.ok;
}
} // namespace semantic
