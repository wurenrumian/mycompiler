#include "IrBuilder.h"

#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

namespace irgen
{
namespace
{
struct Binding
{
    ast::Type type;
    std::string storage;
    bool is_global;
    bool is_const;
    bool has_const_int;
    long long const_int;
    bool has_const_array;
    std::vector<long long> const_array_values;
    std::vector<int> array_dimensions;

    Binding()
        : type(ast::Type::int_type()),
          storage(),
          is_global(false),
          is_const(false),
          has_const_int(false),
          const_int(0),
          has_const_array(false),
          const_array_values(),
          array_dimensions()
    {
    }
};

class Builder
{
public:
    explicit Builder(const Options &options_value)
        : options_(options_value), current_function_(0), current_block_(0), next_slot_id_(0), failed_(false), message_()
    {
        module_.declare_runtime_functions();
    }

    Result build(const ast::CompUnit &unit)
    {
        for (size_t index = 0; index < unit.items.size(); ++index)
        {
            const ast::Node *node = unit.items[index].get();
            if (node->kind() == ast::NodeKind::Decl)
            {
                if (!lower_global_decl(static_cast<const ast::Decl *>(node)))
                {
                    return finish();
                }
            }
        }

        FunctionSignature getarray_sig;
        getarray_sig.return_type = ast::Type::int_type();
        ast::Type int_array_param = ast::Type::int_type();
        int_array_param.is_array_param = true;
        getarray_sig.params.push_back(int_array_param);
        getarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        functions_["getarray"] = getarray_sig;

        FunctionSignature putarray_sig;
        putarray_sig.return_type = ast::Type::void_type();
        putarray_sig.params.push_back(ast::Type::int_type());
        putarray_sig.params.push_back(int_array_param);
        putarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        putarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        functions_["putarray"] = putarray_sig;

        FunctionSignature getfarray_sig;
        getfarray_sig.return_type = ast::Type::int_type();
        ast::Type float_array_param = ast::Type::float_type();
        float_array_param.is_array_param = true;
        getfarray_sig.params.push_back(float_array_param);
        getfarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        functions_["getfarray"] = getfarray_sig;

        FunctionSignature putfarray_sig;
        putfarray_sig.return_type = ast::Type::void_type();
        putfarray_sig.params.push_back(ast::Type::int_type());
        putfarray_sig.params.push_back(float_array_param);
        putfarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        putfarray_sig.param_array_dimensions_after_first.push_back(std::vector<int>());
        functions_["putfarray"] = putfarray_sig;

        for (size_t index = 0; index < unit.items.size(); ++index)
        {
            const ast::Node *node = unit.items[index].get();
            if (node->kind() == ast::NodeKind::FunctionDef)
            {
                const ast::FunctionDef *function = static_cast<const ast::FunctionDef *>(node);
                FunctionSignature signature;
                signature.return_type = function->return_type;
                for (size_t param_index = 0; param_index < function->params.size(); ++param_index)
                {
                    signature.params.push_back(function->params[param_index].type);
                    std::vector<int> dims;
                    if (!collect_param_dimensions(function->params[param_index], dims))
                    {
                        return finish();
                    }
                    signature.param_array_dimensions_after_first.push_back(dims);
                }
                functions_[function->name] = signature;
                if (!lower_function(static_cast<const ast::FunctionDef *>(node)))
                {
                    return finish();
                }
            }
        }

        return finish();
    }

private:
    struct ScopeFrame
    {
        std::map<std::string, Binding> bindings;
    };

    struct FunctionSignature
    {
        ast::Type return_type;
        std::vector<ast::Type> params;
        std::vector<std::vector<int> > param_array_dimensions_after_first;
    };

    Options options_;
    ir::Module module_;
    ir::Function *current_function_;
    ir::BasicBlock *current_block_;
    std::vector<ScopeFrame> scopes_;
    std::map<std::string, Binding> globals_;
    std::map<std::string, FunctionSignature> functions_;
    std::vector<std::string> loop_continue_labels_;
    std::vector<std::string> loop_break_labels_;
    int next_slot_id_;
    bool failed_;
    std::string message_;

    Result finish()
    {
        Result result;
        result.ok = !failed_;
        result.message = message_;
        result.module = module_;
        return result;
    }

    bool fail(const std::string &message)
    {
        failed_ = true;
        message_ = message;
        return false;
    }

    bool current_block_has_direct_output() const
    {
        if (current_block_ == 0)
        {
            return false;
        }
        for (size_t index = 0; index < current_block_->instructions.size(); ++index)
        {
            const std::string &text = current_block_->instructions[index].text;
            if (text.find("call void @putint(") != std::string::npos ||
                text.find("call void @putch(") != std::string::npos ||
                text.find("call void @putfloat(") != std::string::npos ||
                text.find("call void @putarray(") != std::string::npos ||
                text.find("call void @putfarray(") != std::string::npos ||
                text.find("call void @putf(") != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool current_block_needs_newline_before_exit_output() const
    {
        if (current_block_ == 0)
        {
            return false;
        }
        bool saw_output = false;
        for (size_t index = 0; index < current_block_->instructions.size(); ++index)
        {
            const std::string &text = current_block_->instructions[index].text;
            if (text.find("call void @putint(") != std::string::npos ||
                text.find("call void @putfloat(") != std::string::npos ||
                text.find("call void @putarray(") != std::string::npos ||
                text.find("call void @putfarray(") != std::string::npos ||
                text.find("call void @putf(") != std::string::npos)
            {
                saw_output = true;
                continue;
            }
            if (text.find("call void @putch(i32 10)") != std::string::npos)
            {
                saw_output = false;
                continue;
            }
            if (text.find("call void @putch(") != std::string::npos)
            {
                saw_output = true;
            }
        }
        return saw_output;
    }

    void push_scope()
    {
        scopes_.push_back(ScopeFrame());
    }

    void pop_scope()
    {
        if (!scopes_.empty())
        {
            scopes_.pop_back();
        }
    }

    Binding *lookup(const std::string &name)
    {
        for (size_t index = scopes_.size(); index > 0; --index)
        {
            std::map<std::string, Binding> &bindings = scopes_[index - 1].bindings;
            std::map<std::string, Binding>::iterator found = bindings.find(name);
            if (found != bindings.end())
            {
                return &found->second;
            }
        }

        std::map<std::string, Binding>::iterator global_found = globals_.find(name);
        if (global_found != globals_.end())
        {
            return &global_found->second;
        }
        return 0;
    }

    bool define_local(const std::string &name, const Binding &binding)
    {
        if (scopes_.empty())
        {
            push_scope();
        }
        scopes_.back().bindings[name] = binding;
        return true;
    }

    static ir::Type lower_scalar_type(const ast::Type &type)
    {
        switch (type.kind)
        {
        case ast::TypeKind::Float:
            return ir::Type::float_ty();
        case ast::TypeKind::Void:
            return ir::Type::void_ty();
        case ast::TypeKind::Int:
        default:
            return ir::Type::i32();
        }
    }

    static ir::Type lower_array_type(ast::TypeKind kind, const std::vector<int> &dimensions)
    {
        ir::Type result = (kind == ast::TypeKind::Float) ? ir::Type::float_ty() : ir::Type::i32();
        for (size_t index = dimensions.size(); index > 0; --index)
        {
            result = ir::Type::array(dimensions[index - 1], result);
        }
        return result;
    }

    static ir::Type lower_param_type(const ast::Type &type, const std::vector<int> &array_dimensions_after_first)
    {
        if (!type.is_array_param)
        {
            return lower_scalar_type(type);
        }
        if (array_dimensions_after_first.empty())
        {
            return ir::Type::ptr(lower_scalar_type(type));
        }
        return ir::Type::ptr(lower_array_type(type.kind, array_dimensions_after_first));
    }

    std::string next_label(const std::string &prefix)
    {
        std::ostringstream out;
        out << prefix << "." << current_function_->next_temp++;
        return out.str();
    }

    std::string next_slot()
    {
        std::ostringstream out;
        out << "%slot" << next_slot_id_++;
        return out.str();
    }

    bool eval_const_int_expr(const ast::Expr *expr, long long &value)
    {
        switch (expr->expr_kind())
        {
        case ast::ExprKind::IntLiteral:
            value = static_cast<const ast::IntLiteral *>(expr)->value;
            return true;
        case ast::ExprKind::LValue:
        {
            const ast::LValueExpr *lvalue = static_cast<const ast::LValueExpr *>(expr);
            Binding *binding = lookup(lvalue->name);
            if (binding == 0)
            {
                return false;
            }
            if (lvalue->indices.empty())
            {
                if (!binding->has_const_int)
                {
                    return false;
                }
                value = binding->const_int;
                return true;
            }
            if (!binding->has_const_array || lvalue->indices.size() != binding->array_dimensions.size())
            {
                return false;
            }
            long long flat_index = 0;
            for (size_t index = 0; index < lvalue->indices.size(); ++index)
            {
                long long index_value = 0;
                if (!eval_const_int_expr(lvalue->indices[index].get(), index_value))
                {
                    return false;
                }
                flat_index *= binding->array_dimensions[index];
                flat_index += index_value;
            }
            if (flat_index < 0 || static_cast<size_t>(flat_index) >= binding->const_array_values.size())
            {
                return false;
            }
            value = binding->const_array_values[static_cast<size_t>(flat_index)];
            return true;
        }
        case ast::ExprKind::Unary:
        {
            const ast::UnaryExpr *unary = static_cast<const ast::UnaryExpr *>(expr);
            long long operand = 0;
            if (!eval_const_int_expr(unary->operand.get(), operand))
            {
                return false;
            }
            switch (unary->op)
            {
            case ast::UnaryOp::Plus:
                value = operand;
                return true;
            case ast::UnaryOp::Minus:
                value = -operand;
                return true;
            case ast::UnaryOp::Not:
                value = operand ? 0 : 1;
                return true;
            }
            return false;
        }
        case ast::ExprKind::Binary:
        {
            const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
            long long lhs = 0;
            long long rhs = 0;
            if (!eval_const_int_expr(binary->lhs.get(), lhs) ||
                !eval_const_int_expr(binary->rhs.get(), rhs))
            {
                return false;
            }
            switch (binary->op)
            {
            case ast::BinaryOp::Add:
                value = lhs + rhs;
                return true;
            case ast::BinaryOp::Sub:
                value = lhs - rhs;
                return true;
            case ast::BinaryOp::Mul:
                value = lhs * rhs;
                return true;
            case ast::BinaryOp::Div:
                value = lhs / rhs;
                return true;
            case ast::BinaryOp::Mod:
                value = lhs % rhs;
                return true;
            default:
                return false;
            }
        }
        default:
            return false;
        }
    }

    bool collect_dimensions(const ast::VarDef &def, std::vector<int> &dimensions)
    {
        for (size_t index = 0; index < def.dimensions.size(); ++index)
        {
            long long value = 0;
            if (!eval_const_int_expr(def.dimensions[index].get(), value))
            {
                return fail("Unsupported array dimension expression.");
            }
            dimensions.push_back(static_cast<int>(value));
        }
        return true;
    }

    bool eval_const_float_expr(const ast::Expr *expr, double &value)
    {
        switch (expr->expr_kind())
        {
        case ast::ExprKind::FloatLiteral:
            value = static_cast<const ast::FloatLiteral *>(expr)->value;
            return true;
        case ast::ExprKind::IntLiteral:
            value = static_cast<double>(static_cast<const ast::IntLiteral *>(expr)->value);
            return true;
        case ast::ExprKind::Unary:
        {
            const ast::UnaryExpr *unary = static_cast<const ast::UnaryExpr *>(expr);
            double operand = 0.0;
            if (!eval_const_float_expr(unary->operand.get(), operand))
            {
                return false;
            }
            switch (unary->op)
            {
            case ast::UnaryOp::Plus:
                value = operand;
                return true;
            case ast::UnaryOp::Minus:
                value = -operand;
                return true;
            default:
                return false;
            }
        }
        case ast::ExprKind::Binary:
        {
            const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
            double lhs = 0.0;
            double rhs = 0.0;
            if (!eval_const_float_expr(binary->lhs.get(), lhs) ||
                !eval_const_float_expr(binary->rhs.get(), rhs))
            {
                return false;
            }
            switch (binary->op)
            {
            case ast::BinaryOp::Add:
                value = lhs + rhs;
                return true;
            case ast::BinaryOp::Sub:
                value = lhs - rhs;
                return true;
            case ast::BinaryOp::Mul:
                value = lhs * rhs;
                return true;
            case ast::BinaryOp::Div:
                value = lhs / rhs;
                return true;
            default:
                return false;
            }
        }
        default:
            return false;
        }
    }

    bool collect_param_dimensions(const ast::Param &param, std::vector<int> &dimensions)
    {
        for (size_t index = 0; index < param.array_dimensions_after_first.size(); ++index)
        {
            long long value = 0;
            if (!eval_const_int_expr(param.array_dimensions_after_first[index].get(), value))
            {
                return fail("Unsupported array parameter dimension expression.");
            }
            dimensions.push_back(static_cast<int>(value));
        }
        return true;
    }

    size_t total_element_count(const std::vector<int> &dimensions) const
    {
        size_t total = 1;
        for (size_t index = 0; index < dimensions.size(); ++index)
        {
            total *= static_cast<size_t>(dimensions[index]);
        }
        return total;
    }

    bool flatten_init_values(const ast::InitVal &init, std::vector<long long> &values)
    {
        if (!init.is_list)
        {
            if (init.expr.get() == 0)
            {
                values.push_back(0);
                return true;
            }

            long long value = 0;
            if (!eval_const_int_expr(init.expr.get(), value))
            {
                return false;
            }
            values.push_back(value);
            return true;
        }

        for (size_t index = 0; index < init.elements.size(); ++index)
        {
            if (!flatten_init_values(init.elements[index], values))
            {
                return false;
            }
        }
        return true;
    }

    size_t subarray_size(const std::vector<int> &dimensions, size_t dim_index) const
    {
        size_t size = 1;
        for (size_t index = dim_index + 1; index < dimensions.size(); ++index)
        {
            size *= static_cast<size_t>(dimensions[index]);
        }
        return size;
    }

    void fill_initializer_slots(const ast::InitVal &init,
                                const std::vector<int> &dimensions,
                                size_t dim_index,
                                size_t start,
                                std::vector<const ast::Expr *> &slots) const
    {
        if (dimensions.empty())
        {
            if (!init.is_list)
            {
                slots[start] = init.expr.get();
            }
            return;
        }

        const size_t stride = subarray_size(dimensions, dim_index);
        const size_t capacity = static_cast<size_t>(dimensions[dim_index]) * stride;

        if (!init.is_list)
        {
            slots[start] = init.expr.get();
            return;
        }

        size_t current = start;
        for (size_t index = 0; index < init.elements.size(); ++index)
        {
            if (current >= start + capacity)
            {
                break;
            }

            const ast::InitVal &child = init.elements[index];
            if (dim_index + 1 < dimensions.size() && child.is_list)
            {
                const size_t relative = current - start;
                if ((relative % stride) != 0)
                {
                    current += stride - (relative % stride);
                }
                if (current >= start + capacity)
                {
                    break;
                }
                fill_initializer_slots(child, dimensions, dim_index + 1, current, slots);
                current += stride;
                continue;
            }

            if (child.is_list)
            {
                fill_initializer_slots(child, dimensions, dim_index, current, slots);
                continue;
            }

            slots[current++] = child.expr.get();
        }
    }

    std::vector<const ast::Expr *> flatten_initializer_exprs(const ast::InitVal &init,
                                                             const std::vector<int> &dimensions) const
    {
        std::vector<const ast::Expr *> slots(total_element_count(dimensions), static_cast<const ast::Expr *>(0));
        fill_initializer_slots(init, dimensions, 0, 0, slots);
        return slots;
    }

    std::string render_array_constant(const std::vector<int> &dimensions,
                                      const std::vector<long long> &values,
                                      size_t dimension_index,
                                      size_t &value_index) const
    {
        std::ostringstream out;
        out << "[";
        if (dimension_index + 1 == dimensions.size())
        {
            for (int i = 0; i < dimensions[dimension_index]; ++i)
            {
                if (i != 0)
                {
                    out << ", ";
                }
                out << "i32 " << values[value_index++];
            }
            out << "]";
            return out.str();
        }

        std::vector<int> child_dimensions(dimensions.begin() + static_cast<std::ptrdiff_t>(dimension_index + 1),
                                          dimensions.end());
        const ir::Type child_type = lower_array_type(ast::TypeKind::Int, child_dimensions);
        for (int i = 0; i < dimensions[dimension_index]; ++i)
        {
            if (i != 0)
            {
                out << ", ";
            }
            out << child_type.str() << " "
                << render_array_constant(dimensions, values, dimension_index + 1, value_index);
        }
        out << "]";
        return out.str();
    }

    std::string render_float_array_constant(const std::vector<int> &dimensions,
                                            const std::vector<double> &values,
                                            size_t dimension_index,
                                            size_t &value_index) const
    {
        std::ostringstream out;
        out << "[";
        if (dimension_index + 1 == dimensions.size())
        {
            for (int i = 0; i < dimensions[dimension_index]; ++i)
            {
                if (i != 0)
                {
                    out << ", ";
                }
                out << "float " << format_float_constant(values[value_index++]);
            }
            out << "]";
            return out.str();
        }

        std::vector<int> child_dimensions(dimensions.begin() + static_cast<std::ptrdiff_t>(dimension_index + 1),
                                          dimensions.end());
        const ir::Type child_type = lower_array_type(ast::TypeKind::Float, child_dimensions);
        for (int i = 0; i < dimensions[dimension_index]; ++i)
        {
            if (i != 0)
            {
                out << ", ";
            }
            out << child_type.str() << " "
                << render_float_array_constant(dimensions, values, dimension_index + 1, value_index);
        }
        out << "]";
        return out.str();
    }

    bool lower_global_decl(const ast::Decl *decl)
    {
        for (size_t index = 0; index < decl->defs.size(); ++index)
        {
            const ast::VarDef &def = decl->defs[index];
            Binding binding;
            binding.type = decl->base_type;
            binding.is_global = true;
            binding.is_const = decl->is_const;
            binding.storage = "@" + def.name;

            std::ostringstream out;
            if (def.dimensions.empty())
            {
                std::string initializer_text = "0";
                if (def.has_init)
                {
                    if (binding.type.kind == ast::TypeKind::Float)
                    {
                        double value = 0.0;
                        if (!eval_const_float_expr(def.init.expr.get(), value))
                        {
                            return fail("Unsupported global initializer.");
                        }
                        initializer_text = format_float_constant(value);
                    }
                    else
                    {
                        long long value = 0;
                        if (!eval_const_int_expr(def.init.expr.get(), value))
                        {
                            return fail("Unsupported global initializer.");
                        }
                        initializer_text = std::to_string(value);
                        if (decl->is_const)
                        {
                            binding.has_const_int = true;
                            binding.const_int = value;
                        }
                    }
                }
                out << binding.storage << " = global " << lower_scalar_type(binding.type).str() << " " << initializer_text;
            }
            else
            {
                std::vector<int> dimensions;
                if (!collect_dimensions(def, dimensions))
                {
                    return false;
                }
                binding.type.dimensions = dimensions;
                binding.array_dimensions = dimensions;
                if (def.has_init)
                {
                    std::vector<const ast::Expr *> expr_slots = flatten_initializer_exprs(def.init, dimensions);
                    if (binding.type.kind == ast::TypeKind::Float)
                    {
                        std::vector<double> values(expr_slots.size(), 0.0);
                        for (size_t slot = 0; slot < expr_slots.size(); ++slot)
                        {
                            if (expr_slots[slot] == 0)
                            {
                                continue;
                            }
                            if (!eval_const_float_expr(expr_slots[slot], values[slot]))
                            {
                                return fail("Unsupported global array initializer.");
                            }
                        }
                        size_t value_index = 0;
                        out << binding.storage << " = global "
                            << lower_array_type(binding.type.kind, dimensions).str() << " "
                            << render_float_array_constant(dimensions, values, 0, value_index);
                    }
                    else
                    {
                        std::vector<long long> values(expr_slots.size(), 0);
                        for (size_t slot = 0; slot < expr_slots.size(); ++slot)
                        {
                            if (expr_slots[slot] == 0)
                            {
                                continue;
                            }
                            if (!eval_const_int_expr(expr_slots[slot], values[slot]))
                            {
                                return fail("Unsupported global array initializer.");
                            }
                        }
                        binding.has_const_array = true;
                        binding.const_array_values = values;
                        size_t value_index = 0;
                        out << binding.storage << " = global "
                            << lower_array_type(binding.type.kind, dimensions).str() << " "
                            << render_array_constant(dimensions, values, 0, value_index);
                    }
                }
                else
                {
                    out << binding.storage << " = global " << lower_array_type(binding.type.kind, dimensions).str()
                        << " zeroinitializer";
                }
            }

            module_.declarations.push_back(out.str());
            globals_[def.name] = binding;
        }
        return true;
    }

    ir::Value constant_value(long long value) const
    {
        return ir::Value::constant_i32(static_cast<int>(value));
    }

    ir::Value constant_float_value(const std::string &raw) const
    {
        return ir::Value::named(ir::Type::float_ty(), raw);
    }

    std::string format_float_constant(double value) const
    {
        std::ostringstream out;
        out << std::setprecision(9);
        out << value;
        return out.str();
    }

    bool cast_value(const ast::Type &target_type, const ir::Value &value, ir::Value &out_value)
    {
        if ((target_type.kind == ast::TypeKind::Float && value.type.kind == ir::Type::Float) ||
            (target_type.kind == ast::TypeKind::Int && value.type.kind == ir::Type::I32))
        {
            out_value = value;
            return true;
        }

        if (target_type.kind == ast::TypeKind::Float && value.type.kind == ir::Type::I32)
        {
            const std::string temp = current_function_->temp();
            current_block_->append(ir::Instruction::raw(
                temp + " = sitofp i32 " + value.name + " to float"));
            out_value = ir::Value::named(ir::Type::float_ty(), temp);
            return true;
        }

        if (target_type.kind == ast::TypeKind::Int && value.type.kind == ir::Type::Float)
        {
            const std::string temp = current_function_->temp();
            current_block_->append(ir::Instruction::raw(
                temp + " = fptosi float " + value.name + " to i32"));
            out_value = ir::Value::named(ir::Type::i32(), temp);
            return true;
        }

        out_value = value;
        return true;
    }

    ir::Value normalize_exit_value_for_output(const ir::Value &value)
    {
        if (value.constant)
        {
            const int normalized = static_cast<int>(static_cast<unsigned int>(std::atoi(value.name.c_str())) & 255U);
            return ir::Value::constant_i32(normalized);
        }

        const std::string temp = current_function_->temp();
        current_block_->append(ir::Instruction::raw(
            temp + " = and i32 " + value.name + ", 255"));
        return ir::Value::named(ir::Type::i32(), temp);
    }

    ir::Value emit_load(const Binding &binding)
    {
        const std::string temp = current_function_->temp();
        const std::string type_text = lower_scalar_type(binding.type).str();
        current_block_->append(ir::Instruction::raw(
            temp + " = load " + type_text + ", " + type_text + "* " + binding.storage));
        return ir::Value::named(lower_scalar_type(binding.type), temp);
    }

    void append_alloca_to_entry(const std::string &text)
    {
        if (current_function_ == 0 || current_function_->blocks.empty())
        {
            return;
        }
        current_function_->blocks.front().instructions.insert(
            current_function_->blocks.front().instructions.begin(),
            ir::Instruction::raw(text));
    }

    bool emit_array_address(const Binding &binding,
                            const std::vector<std::unique_ptr<ast::Expr> > &indices,
                            std::string &address_name)
    {
        if (binding.type.is_array_param)
        {
            std::vector<ir::Value> index_values;
            for (size_t index = 0; index < indices.size(); ++index)
            {
                ir::Value index_value;
                if (!emit_expr(indices[index].get(), index_value))
                {
                    return false;
                }
                index_values.push_back(index_value);
            }

            if (binding.array_dimensions.empty())
            {
                if (index_values.size() != 1)
                {
                    return fail("Unsupported indexed assignment in minimal IR builder. dims=0 indices=" + std::to_string(index_values.size()));
                }
                const std::string gep_temp = current_function_->temp();
                const std::string element_type = lower_scalar_type(binding.type).str();
                current_block_->append(ir::Instruction::raw(
                    gep_temp + " = getelementptr " + element_type + ", " + element_type + "* " +
                    binding.storage + ", i32 " + index_values[0].name));
                address_name = gep_temp;
                return true;
            }

            if (index_values.size() != binding.array_dimensions.size() + 1)
            {
                return fail("Unsupported indexed assignment in minimal IR builder. dims=" +
                            std::to_string(binding.array_dimensions.size()) +
                            " indices=" + std::to_string(index_values.size()));
            }

            const std::string pointee_type = lower_array_type(binding.type.kind, binding.array_dimensions).str();
            const std::string gep_temp = current_function_->temp();
            std::ostringstream text;
            text << gep_temp << " = getelementptr " << pointee_type << ", " << pointee_type
                 << "* " << binding.storage;
            for (size_t index = 0; index < index_values.size(); ++index)
            {
                text << ", i32 " << index_values[index].name;
            }
            current_block_->append(ir::Instruction::raw(text.str()));
            address_name = gep_temp;
            return true;
        }

        if (binding.array_dimensions.empty() || indices.size() != binding.array_dimensions.size())
        {
            std::ostringstream message;
            message << "Unsupported indexed assignment in minimal IR builder."
                    << " dims=" << binding.array_dimensions.size()
                    << " indices=" << indices.size();
            return fail(message.str());
        }

        std::vector<ir::Value> index_values;
        for (size_t index = 0; index < indices.size(); ++index)
        {
            ir::Value index_value;
            if (!emit_expr(indices[index].get(), index_value))
            {
                return false;
            }
            index_values.push_back(index_value);
        }

        const std::string array_type = lower_array_type(binding.type.kind, binding.array_dimensions).str();
        const std::string gep_temp = current_function_->temp();
        std::ostringstream text;
        text << gep_temp << " = getelementptr " << array_type << ", " << array_type
             << "* " << binding.storage << ", i32 0";
        for (size_t index = 0; index < index_values.size(); ++index)
        {
            text << ", i32 " << index_values[index].name;
        }
        current_block_->append(ir::Instruction::raw(text.str()));
        address_name = gep_temp;
        return true;
    }

    bool current_block_terminated() const
    {
        if (current_block_ == 0 || current_block_->instructions.empty())
        {
            return false;
        }
        const std::string &text = current_block_->instructions.back().text;
        return text.find("ret ") == 0 || text.find("br ") == 0;
    }

    bool emit_indexed_global_load(const Binding &binding,
                                  const ast::LValueExpr *lvalue,
                                  ir::Value &out_value)
    {
        std::string gep_temp;
        if (!emit_array_address(binding, lvalue->indices, gep_temp))
        {
            return false;
        }
        const std::string load_temp = current_function_->temp();
        const std::string type_text = lower_scalar_type(binding.type).str();
        current_block_->append(ir::Instruction::raw(
            load_temp + " = load " + type_text + ", " + type_text + "* " + gep_temp));
        out_value = ir::Value::named(lower_scalar_type(binding.type), load_temp);
        return true;
    }

    bool emit_array_decay(const Binding &binding,
                          ir::Value &out_value)
    {
        if (binding.type.is_array_param)
        {
            if (binding.array_dimensions.empty())
            {
                out_value = ir::Value::named(ir::Type::ptr(lower_scalar_type(binding.type)), binding.storage);
                return true;
            }
            out_value = ir::Value::named(ir::Type::ptr(lower_array_type(binding.type.kind, binding.array_dimensions)), binding.storage);
            return true;
        }

        if (binding.array_dimensions.empty())
        {
            return fail("Cannot decay scalar value as array.");
        }

        const std::string base_type = lower_array_type(binding.type.kind, binding.array_dimensions).str();
        const std::string gep_temp = current_function_->temp();
        std::ostringstream text;
        text << gep_temp << " = getelementptr " << base_type << ", " << base_type << "* " << binding.storage
             << ", i32 0, i32 0";
        current_block_->append(ir::Instruction::raw(text.str()));

        if (binding.array_dimensions.size() == 1)
        {
            out_value = ir::Value::named(ir::Type::ptr(lower_scalar_type(binding.type)), gep_temp);
        }
        else
        {
            std::vector<int> child_dims(binding.array_dimensions.begin() + 1, binding.array_dimensions.end());
            out_value = ir::Value::named(ir::Type::ptr(lower_array_type(binding.type.kind, child_dims)), gep_temp);
        }
        return true;
    }

    bool emit_partial_array_decay(const Binding &binding,
                                  const std::vector<std::unique_ptr<ast::Expr> > &indices,
                                  ir::Value &out_value)
    {
        if (binding.array_dimensions.empty())
        {
            return fail("Cannot partially decay a scalar value.");
        }
        if (indices.empty())
        {
            return emit_array_decay(binding, out_value);
        }
        if (indices.size() >= binding.array_dimensions.size())
        {
            return fail("Partial array decay requires fewer indices than array rank.");
        }

        std::vector<ir::Value> index_values;
        for (size_t index = 0; index < indices.size(); ++index)
        {
            ir::Value index_value;
            if (!emit_expr(indices[index].get(), index_value))
            {
                return false;
            }
            index_values.push_back(index_value);
        }

        const std::string array_type = lower_array_type(binding.type.kind, binding.array_dimensions).str();
        const std::string gep_temp = current_function_->temp();
        std::ostringstream text;
        text << gep_temp << " = getelementptr " << array_type << ", " << array_type
             << "* " << binding.storage << ", i32 0";
        for (size_t index = 0; index < index_values.size(); ++index)
        {
            text << ", i32 " << index_values[index].name;
        }
        text << ", i32 0";
        current_block_->append(ir::Instruction::raw(text.str()));

        std::vector<int> remaining_dims(binding.array_dimensions.begin() +
                                            static_cast<std::ptrdiff_t>(indices.size() + 1),
                                        binding.array_dimensions.end());
        if (remaining_dims.empty())
        {
            out_value = ir::Value::named(ir::Type::ptr(lower_scalar_type(binding.type)), gep_temp);
        }
        else
        {
            out_value = ir::Value::named(
                ir::Type::ptr(lower_array_type(binding.type.kind, remaining_dims)),
                gep_temp);
        }
        return true;
    }

    bool emit_condition(const ast::Expr *expr, std::string &condition_name)
    {
        if (expr->expr_kind() == ast::ExprKind::Binary)
        {
            const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
            const char *predicate = 0;
            const char *float_predicate = 0;
            switch (binary->op)
            {
            case ast::BinaryOp::Lt: predicate = "slt"; float_predicate = "olt"; break;
            case ast::BinaryOp::Gt: predicate = "sgt"; float_predicate = "ogt"; break;
            case ast::BinaryOp::Le: predicate = "sle"; float_predicate = "ole"; break;
            case ast::BinaryOp::Ge: predicate = "sge"; float_predicate = "oge"; break;
            case ast::BinaryOp::Eq: predicate = "eq"; float_predicate = "oeq"; break;
            case ast::BinaryOp::Ne: predicate = "ne"; float_predicate = "one"; break;
            default: break;
            }

            if (predicate != 0)
            {
                ir::Value lhs;
                ir::Value rhs;
                if (!emit_expr(binary->lhs.get(), lhs) || !emit_expr(binary->rhs.get(), rhs))
                {
                    return false;
                }
                condition_name = current_function_->temp();
                if (lhs.type.kind == ir::Type::Float || rhs.type.kind == ir::Type::Float)
                {
                    ast::Type float_type = ast::Type::float_type();
                    if (!cast_value(float_type, lhs, lhs) || !cast_value(float_type, rhs, rhs))
                    {
                        return false;
                    }
                    current_block_->append(ir::Instruction::raw(
                        condition_name + " = fcmp " + float_predicate + " float " + lhs.name + ", " + rhs.name));
                    return true;
                }
                current_block_->append(ir::Instruction::raw(
                    condition_name + " = icmp " + predicate + " i32 " + lhs.name + ", " + rhs.name));
                return true;
            }
        }

        ir::Value value;
        if (!emit_expr(expr, value))
        {
            return false;
        }
        condition_name = current_function_->temp();
        if (value.type.kind == ir::Type::Float)
        {
            current_block_->append(ir::Instruction::raw(
                condition_name + " = fcmp one float " + value.name + ", 0.0"));
            return true;
        }
        current_block_->append(ir::Instruction::raw(
            condition_name + " = icmp ne i32 " + value.name + ", 0"));
        return true;
    }

    bool emit_branch_on_condition(const ast::Expr *expr,
                                  const std::string &true_label,
                                  const std::string &false_label)
    {
        if (expr->expr_kind() == ast::ExprKind::Binary)
        {
            const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
            if (binary->op == ast::BinaryOp::And)
            {
                const std::string rhs_label = next_label("land.rhs");
                if (!emit_branch_on_condition(binary->lhs.get(), rhs_label, false_label))
                {
                    return false;
                }
                current_block_ = current_function_->create_block(rhs_label);
                return emit_branch_on_condition(binary->rhs.get(), true_label, false_label);
            }
            if (binary->op == ast::BinaryOp::Or)
            {
                const std::string rhs_label = next_label("lor.rhs");
                if (!emit_branch_on_condition(binary->lhs.get(), true_label, rhs_label))
                {
                    return false;
                }
                current_block_ = current_function_->create_block(rhs_label);
                return emit_branch_on_condition(binary->rhs.get(), true_label, false_label);
            }
        }

        std::string cond_name;
        if (!emit_condition(expr, cond_name))
        {
            return false;
        }
            current_block_->append(ir::Instruction::raw(
                "br i1 " + cond_name + ", label %" + true_label + ", label %" + false_label));
        return true;
    }

    bool emit_expr(const ast::Expr *expr, ir::Value &out_value)
    {
        long long constant = 0;
        if (eval_const_int_expr(expr, constant))
        {
            out_value = constant_value(constant);
            return true;
        }

        switch (expr->expr_kind())
        {
        case ast::ExprKind::IntLiteral:
            out_value = constant_value(static_cast<const ast::IntLiteral *>(expr)->value);
            return true;
        case ast::ExprKind::FloatLiteral:
        {
            const ast::FloatLiteral *literal = static_cast<const ast::FloatLiteral *>(expr);
            out_value = constant_float_value(literal->raw.empty() ? format_float_constant(literal->value) : literal->raw);
            return true;
        }
        case ast::ExprKind::LValue:
        {
            const ast::LValueExpr *lvalue = static_cast<const ast::LValueExpr *>(expr);
            Binding *binding = lookup(lvalue->name);
            if (binding == 0)
            {
                return fail("Unknown identifier in IR builder: " + lvalue->name);
            }
            if (!lvalue->indices.empty())
            {
                return emit_indexed_global_load(*binding, lvalue, out_value);
            }
            if (binding->type.is_array_param || !binding->array_dimensions.empty())
            {
                return emit_array_decay(*binding, out_value);
            }
            out_value = emit_load(*binding);
            return true;
        }
        case ast::ExprKind::Call:
        {
            const ast::CallExpr *call = static_cast<const ast::CallExpr *>(expr);
            if (call->callee == "starttime" || call->callee == "stoptime")
            {
                const std::string runtime_name =
                    call->callee == "starttime" ? "_sysy_starttime" : "_sysy_stoptime";
                current_block_->append(ir::Instruction::raw(
                    "call void @" + runtime_name + "(i32 0)"));
                out_value = ir::Value::constant_i32(0);
                return true;
            }
            if (call->callee == "getint" || call->callee == "getch")
            {
                const std::string temp = current_function_->temp();
                current_block_->append(ir::Instruction::raw(
                    temp + " = call i32 @" + call->callee + "()"));
                out_value = ir::Value::named(ir::Type::i32(), temp);
                return true;
            }
            if (call->callee == "getfloat")
            {
                const std::string temp = current_function_->temp();
                current_block_->append(ir::Instruction::raw(
                    temp + " = call float @getfloat()"));
                out_value = ir::Value::named(ir::Type::float_ty(), temp);
                return true;
            }
            if (call->callee == "putint" || call->callee == "putch")
            {
                if (call->args.size() != 1)
                {
                    return fail("Unsupported runtime call arity.");
                }
                ir::Value arg;
                if (!emit_expr(call->args[0].get(), arg))
                {
                    return false;
                }
                current_block_->append(ir::Instruction::raw(
                    "call void @" + call->callee + "(i32 " + arg.name + ")"));
                out_value = ir::Value::constant_i32(0);
                return true;
            }
            if (call->callee == "putfloat")
            {
                if (call->args.size() != 1)
                {
                    return fail("Unsupported runtime call arity.");
                }
                ir::Value arg;
                if (!emit_expr(call->args[0].get(), arg))
                {
                    return false;
                }
                ast::Type float_type = ast::Type::float_type();
                if (!cast_value(float_type, arg, arg))
                {
                    return false;
                }
                current_block_->append(ir::Instruction::raw(
                    "call void @putfloat(float " + arg.name + ")"));
                out_value = ir::Value::constant_i32(0);
                return true;
            }

            std::map<std::string, FunctionSignature>::const_iterator found = functions_.find(call->callee);
            if (found == functions_.end())
            {
                return fail("Unsupported call expression in minimal IR builder.");
            }

            std::vector<std::string> rendered_args;
            for (size_t index = 0; index < call->args.size(); ++index)
            {
                ir::Value arg;
                const ast::Type &param_type = found->second.params[index];
                if (param_type.is_array_param)
                {
                    const ast::Expr *arg_expr = call->args[index].get();
                    if (arg_expr->expr_kind() != ast::ExprKind::LValue)
                    {
                        return fail("Array argument must be an lvalue.");
                    }
                    const ast::LValueExpr *arg_lvalue = static_cast<const ast::LValueExpr *>(arg_expr);
                    Binding *binding = lookup(arg_lvalue->name);
                    if (binding == 0)
                    {
                        return fail("Unknown array argument in call: " + arg_lvalue->name);
                    }
                    if (!arg_lvalue->indices.empty())
                    {
                        if (!emit_partial_array_decay(*binding, arg_lvalue->indices, arg))
                        {
                            return false;
                        }
                    }
                    else if (!emit_array_decay(*binding, arg))
                    {
                        return false;
                    }
                    rendered_args.push_back(
                        lower_param_type(param_type, found->second.param_array_dimensions_after_first[index]).str() +
                        " " + arg.name);
                }
                else
                {
                    if (!emit_expr(call->args[index].get(), arg))
                    {
                        return false;
                    }
                    if (!cast_value(param_type, arg, arg))
                    {
                        return false;
                    }
                    rendered_args.push_back(lower_scalar_type(param_type).str() + " " + arg.name);
                }
            }

            std::ostringstream call_text;
            bool returns_void = found->second.return_type.kind == ast::TypeKind::Void;
            if (returns_void)
            {
                call_text << "call void @" << call->callee << "(";
            }
            else
            {
                const std::string temp = current_function_->temp();
                call_text << temp << " = call " << lower_scalar_type(found->second.return_type).str()
                          << " @" << call->callee << "(";
                out_value = ir::Value::named(lower_scalar_type(found->second.return_type), temp);
            }
            for (size_t index = 0; index < rendered_args.size(); ++index)
            {
                if (index != 0)
                {
                    call_text << ", ";
                }
                call_text << rendered_args[index];
            }
            call_text << ")";
            current_block_->append(ir::Instruction::raw(call_text.str()));
            if (returns_void)
            {
                out_value = ir::Value::constant_i32(0);
            }
            return true;
        }
        case ast::ExprKind::Unary:
        {
            const ast::UnaryExpr *unary = static_cast<const ast::UnaryExpr *>(expr);
            ir::Value operand;
            if (!emit_expr(unary->operand.get(), operand))
            {
                return false;
            }
            if (unary->op == ast::UnaryOp::Plus)
            {
                out_value = operand;
                return true;
            }

            const std::string temp = current_function_->temp();
            if (operand.type.kind == ir::Type::Float)
            {
                if (unary->op == ast::UnaryOp::Minus)
                {
                    current_block_->append(ir::Instruction::raw(
                        temp + " = fsub float 0.0, " + operand.name));
                    out_value = ir::Value::named(ir::Type::float_ty(), temp);
                    return true;
                }
                return fail("Unsupported float unary expression in minimal IR builder.");
            }
            if (unary->op == ast::UnaryOp::Minus)
            {
                current_block_->append(ir::Instruction::raw(
                    temp + " = sub i32 0, " + operand.name));
            }
            else
            {
                current_block_->append(ir::Instruction::raw(
                    temp + " = icmp eq i32 " + operand.name + ", 0"));
                const std::string zext = current_function_->temp();
                current_block_->append(ir::Instruction::raw(
                    zext + " = zext i1 " + temp + " to i32"));
                out_value = ir::Value::named(ir::Type::i32(), zext);
                return true;
            }
            out_value = ir::Value::named(ir::Type::i32(), temp);
            return true;
        }
        case ast::ExprKind::Binary:
        {
            const ast::BinaryExpr *binary = static_cast<const ast::BinaryExpr *>(expr);
            ir::Value lhs;
            ir::Value rhs;
            if (!emit_expr(binary->lhs.get(), lhs) || !emit_expr(binary->rhs.get(), rhs))
            {
                return false;
            }
            const std::string temp = current_function_->temp();
            if (lhs.type.kind == ir::Type::Float || rhs.type.kind == ir::Type::Float)
            {
                ast::Type float_type = ast::Type::float_type();
                if (!cast_value(float_type, lhs, lhs) || !cast_value(float_type, rhs, rhs))
                {
                    return false;
                }
                const char *float_opcode = 0;
                switch (binary->op)
                {
                case ast::BinaryOp::Add: float_opcode = "fadd"; break;
                case ast::BinaryOp::Sub: float_opcode = "fsub"; break;
                case ast::BinaryOp::Mul: float_opcode = "fmul"; break;
                case ast::BinaryOp::Div: float_opcode = "fdiv"; break;
                case ast::BinaryOp::Lt: float_opcode = "olt"; break;
                case ast::BinaryOp::Gt: float_opcode = "ogt"; break;
                case ast::BinaryOp::Le: float_opcode = "ole"; break;
                case ast::BinaryOp::Ge: float_opcode = "oge"; break;
                case ast::BinaryOp::Eq: float_opcode = "oeq"; break;
                case ast::BinaryOp::Ne: float_opcode = "one"; break;
                default:
                    return fail("Unsupported float binary expression in minimal IR builder.");
                }
                if (binary->op == ast::BinaryOp::Lt || binary->op == ast::BinaryOp::Gt ||
                    binary->op == ast::BinaryOp::Le || binary->op == ast::BinaryOp::Ge ||
                    binary->op == ast::BinaryOp::Eq || binary->op == ast::BinaryOp::Ne)
                {
                    current_block_->append(ir::Instruction::raw(
                        temp + " = fcmp " + float_opcode + std::string(" float ") + lhs.name + ", " + rhs.name));
                    const std::string zext = current_function_->temp();
                    current_block_->append(ir::Instruction::raw(
                        zext + " = zext i1 " + temp + " to i32"));
                    out_value = ir::Value::named(ir::Type::i32(), zext);
                    return true;
                }
                current_block_->append(ir::Instruction::raw(
                    temp + " = " + float_opcode + std::string(" float ") + lhs.name + ", " + rhs.name));
                out_value = ir::Value::named(ir::Type::float_ty(), temp);
                return true;
            }
            const char *opcode = 0;
            switch (binary->op)
            {
            case ast::BinaryOp::Add: opcode = "add"; break;
            case ast::BinaryOp::Sub: opcode = "sub"; break;
            case ast::BinaryOp::Mul: opcode = "mul"; break;
            case ast::BinaryOp::Div: opcode = "sdiv"; break;
            case ast::BinaryOp::Mod: opcode = "srem"; break;
            case ast::BinaryOp::Lt:
            case ast::BinaryOp::Gt:
            case ast::BinaryOp::Le:
            case ast::BinaryOp::Ge:
            case ast::BinaryOp::Eq:
            case ast::BinaryOp::Ne:
            {
                std::string cond_name;
                if (!emit_condition(expr, cond_name))
                {
                    return false;
                }
                const std::string zext = current_function_->temp();
                current_block_->append(ir::Instruction::raw(
                    zext + " = zext i1 " + cond_name + " to i32"));
                out_value = ir::Value::named(ir::Type::i32(), zext);
                return true;
            }
            case ast::BinaryOp::And:
            case ast::BinaryOp::Or:
                return fail("Unsupported logical expression outside condition context.");
            default:
                return fail("Unsupported binary expression in minimal IR builder.");
            }
            current_block_->append(ir::Instruction::raw(
                temp + " = " + opcode + std::string(" i32 ") + lhs.name + ", " + rhs.name));
            out_value = ir::Value::named(ir::Type::i32(), temp);
            return true;
        }
        default:
            return fail("Unsupported expression in minimal IR builder.");
        }
    }

    bool lower_decl(const ast::Decl *decl)
    {
        for (size_t index = 0; index < decl->defs.size(); ++index)
        {
            const ast::VarDef &def = decl->defs[index];
            Binding binding;
            binding.type = decl->base_type;
            binding.is_global = false;
            binding.is_const = decl->is_const;
            if (!def.dimensions.empty())
            {
                if (!collect_dimensions(def, binding.array_dimensions))
                {
                    return false;
                }
                binding.type.dimensions = binding.array_dimensions;
                binding.storage = next_slot();
                append_alloca_to_entry(
                    binding.storage + " = alloca " + lower_array_type(binding.type.kind, binding.array_dimensions).str());
                if (def.has_init)
                {
                    std::vector<const ast::Expr *> expr_slots = flatten_initializer_exprs(def.init, binding.array_dimensions);
                    for (size_t slot = 0; slot < expr_slots.size(); ++slot)
                    {
                        std::vector<std::unique_ptr<ast::Expr> > indices;
                        size_t remainder = slot;
                        for (size_t dim = 0; dim < binding.array_dimensions.size(); ++dim)
                        {
                            size_t divisor = 1;
                            for (size_t next = dim + 1; next < binding.array_dimensions.size(); ++next)
                            {
                                divisor *= static_cast<size_t>(binding.array_dimensions[next]);
                            }
                            const size_t idx = remainder / divisor;
                            remainder %= divisor;
                            std::unique_ptr<ast::IntLiteral> literal(new ast::IntLiteral());
                            literal->value = static_cast<long long>(idx);
                            indices.push_back(std::unique_ptr<ast::Expr>(literal.release()));
                        }

                        std::string address;
                        if (!emit_array_address(binding, indices, address))
                        {
                            return false;
                        }

                        ir::Value init_value = constant_value(0);
                        if (expr_slots[slot] != 0)
                        {
                            if (!emit_expr(expr_slots[slot], init_value))
                            {
                                return false;
                            }
                            if (!cast_value(binding.type, init_value, init_value))
                            {
                                return false;
                            }
                        }
                        current_block_->append(ir::Instruction::raw(
                            "store " + lower_scalar_type(binding.type).str() + " " + init_value.name + ", " +
                            lower_scalar_type(binding.type).str() + "* " + address));
                    }
                }
                define_local(def.name, binding);
                continue;
            }

            binding.storage = next_slot();
            append_alloca_to_entry(binding.storage + " = alloca " + lower_scalar_type(binding.type).str());
            if (def.has_init)
            {
                ir::Value init_value;
                if (!emit_expr(def.init.expr.get(), init_value))
                {
                    return false;
                }
                if (!cast_value(binding.type, init_value, init_value))
                {
                    return false;
                }
                current_block_->append(ir::Instruction::raw(
                    "store " + lower_scalar_type(binding.type).str() + " " + init_value.name + ", " +
                    lower_scalar_type(binding.type).str() + "* " + binding.storage));

                long long constant = 0;
                if (decl->is_const && eval_const_int_expr(def.init.expr.get(), constant))
                {
                    binding.has_const_int = true;
                    binding.const_int = constant;
                }
            }
            else if (decl->is_const)
            {
                binding.has_const_int = true;
                binding.const_int = 0;
            }
            define_local(def.name, binding);
        }
        return true;
    }

    bool lower_stmt(const ast::Stmt *stmt)
    {
        switch (stmt->stmt_kind())
        {
        case ast::StmtKind::Block:
        {
            const ast::Block *block = static_cast<const ast::Block *>(stmt);
            push_scope();
            for (size_t index = 0; index < block->items.size(); ++index)
            {
                const ast::BlockItem *item = block->items[index].get();
                if (item->kind() == ast::NodeKind::Decl)
                {
                    if (!lower_decl(static_cast<const ast::Decl *>(item)))
                    {
                        pop_scope();
                        return false;
                    }
                }
                else
                {
                    if (!lower_stmt(static_cast<const ast::Stmt *>(item)))
                    {
                        pop_scope();
                        return false;
                    }
                }
            }
            pop_scope();
            return true;
        }
        case ast::StmtKind::Assign:
        {
            const ast::AssignStmt *assign = static_cast<const ast::AssignStmt *>(stmt);
            Binding *binding = lookup(assign->target->name);
            if (binding == 0)
            {
                return fail("Unknown assignment target in IR builder: " + assign->target->name);
            }
            ir::Value value;
            if (!emit_expr(assign->value.get(), value))
            {
                return false;
            }
            if (!cast_value(binding->type, value, value))
            {
                return false;
            }
            if (!assign->target->indices.empty())
            {
                std::string address;
                if (!emit_array_address(*binding, assign->target->indices, address))
                {
                    return false;
                }
                current_block_->append(ir::Instruction::raw(
                    "store " + lower_scalar_type(binding->type).str() + " " + value.name + ", " +
                    lower_scalar_type(binding->type).str() + "* " + address));
                return true;
            }
            current_block_->append(ir::Instruction::raw(
                "store " + lower_scalar_type(binding->type).str() + " " + value.name + ", " +
                lower_scalar_type(binding->type).str() + "* " + binding->storage));
            return true;
        }
        case ast::StmtKind::Expr:
        {
            const ast::ExprStmt *expr_stmt = static_cast<const ast::ExprStmt *>(stmt);
            if (expr_stmt->expr.get() == 0)
            {
                return true;
            }
            ir::Value ignored;
            return emit_expr(expr_stmt->expr.get(), ignored);
        }
        case ast::StmtKind::FormatOutput:
        {
            const ast::FormatOutputStmt *output = static_cast<const ast::FormatOutputStmt *>(stmt);
            std::string format = output->format;
            if (format.size() >= 2 && format.front() == '"' && format.back() == '"')
            {
                format = format.substr(1, format.size() - 2);
            }

            size_t arg_index = 0;
            for (size_t index = 0; index < format.size(); ++index)
            {
                const char ch = format[index];
                if (ch == '\\' && index + 1 < format.size())
                {
                    ++index;
                    int value = static_cast<unsigned char>(format[index]);
                    if (format[index] == 'n')
                    {
                        value = 10;
                    }
                    else if (format[index] == 't')
                    {
                        value = 9;
                    }
                    current_block_->append(ir::Instruction::raw(
                        "call void @putch(i32 " + std::to_string(value) + ")"));
                    continue;
                }

                if (ch == '%' && index + 1 < format.size())
                {
                    const char specifier = format[++index];
                    if (arg_index >= output->args.size())
                    {
                        return fail("Not enough arguments for format output.");
                    }

                    ir::Value arg;
                    if (!emit_expr(output->args[arg_index++].get(), arg))
                    {
                        return false;
                    }

                    if (specifier == 'f')
                    {
                        ast::Type float_type = ast::Type::float_type();
                        if (!cast_value(float_type, arg, arg))
                        {
                            return false;
                        }
                        current_block_->append(ir::Instruction::raw(
                            "call void @putfloat(float " + arg.name + ")"));
                        continue;
                    }

                    if (specifier == 'd' || specifier == 'c')
                    {
                        ast::Type int_type = ast::Type::int_type();
                        if (!cast_value(int_type, arg, arg))
                        {
                            return false;
                        }
                        current_block_->append(ir::Instruction::raw(
                            "call void @" + std::string(specifier == 'c' ? "putch" : "putint") +
                            "(i32 " + arg.name + ")"));
                        continue;
                    }

                    current_block_->append(ir::Instruction::raw(
                        "call void @putch(i32 " + std::to_string(static_cast<int>(specifier)) + ")"));
                    continue;
                }

                current_block_->append(ir::Instruction::raw(
                    "call void @putch(i32 " + std::to_string(static_cast<int>(static_cast<unsigned char>(ch))) + ")"));
            }

            if (arg_index != output->args.size())
            {
                return fail("Too many arguments for format output.");
            }
            return true;
        }
        case ast::StmtKind::Return:
        {
            const ast::ReturnStmt *ret = static_cast<const ast::ReturnStmt *>(stmt);
            if (ret->value.get() == 0)
            {
                current_block_->append(ir::Instruction::raw("ret void"));
                return true;
            }
            ir::Value value;
            if (!emit_expr(ret->value.get(), value))
            {
                return false;
            }
            ast::Type return_type = ast::Type::int_type();
            if (current_function_->return_type.kind == ir::Type::Float)
            {
                return_type = ast::Type::float_type();
            }
            else if (current_function_->return_type.kind == ir::Type::Void)
            {
                return_type = ast::Type::void_type();
            }
            if (!cast_value(return_type, value, value))
            {
                return false;
            }
            if (options_.emit_main_return_value && current_function_->name == "main")
            {
                if (current_block_needs_newline_before_exit_output())
                {
                    current_block_->append(ir::Instruction::raw("call void @putch(i32 10)"));
                }
                ir::Value output_value = normalize_exit_value_for_output(value);
                current_block_->append(ir::Instruction::raw(
                    "call void @putint(i32 " + output_value.name + ")"));
            }
            current_block_->append(ir::Instruction::ret(value));
            return true;
        }
        case ast::StmtKind::While:
        {
            const ast::WhileStmt *while_stmt = static_cast<const ast::WhileStmt *>(stmt);
            const std::string cond_label = next_label("while.cond");
            const std::string body_label = next_label("while.body");
            const std::string end_label = next_label("while.end");

            current_block_->append(ir::Instruction::raw("br label %" + cond_label));

            current_block_ = current_function_->create_block(cond_label);
            if (!emit_branch_on_condition(while_stmt->condition.get(), body_label, end_label))
            {
                return false;
            }

            current_block_ = current_function_->create_block(body_label);
            loop_continue_labels_.push_back(cond_label);
            loop_break_labels_.push_back(end_label);
            if (!lower_stmt(while_stmt->body.get()))
            {
                loop_continue_labels_.pop_back();
                loop_break_labels_.pop_back();
                return false;
            }
            loop_continue_labels_.pop_back();
            loop_break_labels_.pop_back();
            if (!current_block_terminated())
            {
                current_block_->append(ir::Instruction::raw("br label %" + cond_label));
            }

            current_block_ = current_function_->create_block(end_label);
            return true;
        }
        case ast::StmtKind::If:
        {
            const ast::IfStmt *if_stmt = static_cast<const ast::IfStmt *>(stmt);
            const std::string then_label = next_label("if.then");
            const std::string else_label = next_label("if.else");

            if (!emit_branch_on_condition(if_stmt->condition.get(), then_label, else_label))
            {
                return false;
            }

            current_block_ = current_function_->create_block(then_label);
            if (!lower_stmt(if_stmt->then_branch.get()))
            {
                return false;
            }
            const std::string then_exit_label = current_block_->label;
            const bool then_terminated = current_block_terminated();

            current_block_ = current_function_->create_block(else_label);
            if (if_stmt->else_branch.get() != 0)
            {
                if (!lower_stmt(if_stmt->else_branch.get()))
                {
                    return false;
                }
            }
            const bool else_terminated = current_block_terminated();

            if (then_terminated && else_terminated)
            {
                return true;
            }

            const std::string end_label = next_label("if.end");
            if (!then_terminated)
            {
                for (size_t block_index = 0; block_index < current_function_->blocks.size(); ++block_index)
                {
                    if (current_function_->blocks[block_index].label == then_exit_label)
                    {
                        current_function_->blocks[block_index].append(
                            ir::Instruction::raw("br label %" + end_label));
                        break;
                    }
                }
            }
            if (!else_terminated)
            {
                current_block_->append(ir::Instruction::raw("br label %" + end_label));
            }

            current_block_ = current_function_->create_block(end_label);
            return true;
        }
        case ast::StmtKind::Empty:
            return true;
        case ast::StmtKind::Continue:
            if (loop_continue_labels_.empty())
            {
                return fail("continue used outside loop in IR builder.");
            }
            current_block_->append(ir::Instruction::raw("br label %" + loop_continue_labels_.back()));
            return true;
        case ast::StmtKind::Break:
            if (loop_break_labels_.empty())
            {
                return fail("break used outside loop in IR builder.");
            }
            current_block_->append(ir::Instruction::raw("br label %" + loop_break_labels_.back()));
            return true;
        default:
            return fail("Unsupported statement in minimal IR builder.");
        }
    }

    bool lower_function(const ast::FunctionDef *function)
    {
        std::vector<ir::Type> param_types;
        for (size_t param_index = 0; param_index < function->params.size(); ++param_index)
        {
            std::vector<int> dims;
            if (!collect_param_dimensions(function->params[param_index], dims))
            {
                return false;
            }
            param_types.push_back(lower_param_type(function->params[param_index].type, dims));
        }

        current_function_ = module_.create_function(function->name,
                                                    lower_scalar_type(function->return_type),
                                                    param_types);
        current_function_->next_temp = static_cast<int>(function->params.size());
        next_slot_id_ = 0;
        current_block_ = current_function_->create_block("entry");
        scopes_.clear();
        push_scope();

        for (size_t param_index = 0; param_index < function->params.size(); ++param_index)
        {
            const ast::Param &param = function->params[param_index];
            if (param.type.is_array_param)
            {
                Binding binding;
                binding.type = param.type;
                binding.is_global = false;
                binding.is_const = false;
                binding.storage = "%" + std::to_string(static_cast<int>(param_index));
                if (!collect_param_dimensions(param, binding.array_dimensions))
                {
                    return false;
                }
                binding.type.dimensions = binding.array_dimensions;
                define_local(param.name, binding);
                continue;
            }

            Binding binding;
            binding.type = param.type;
            binding.is_global = false;
            binding.is_const = false;
            binding.storage = next_slot();
            append_alloca_to_entry(
                binding.storage + " = alloca " + lower_scalar_type(param.type).str());

            std::ostringstream incoming;
            incoming << "%" << param_index;
            current_block_->append(ir::Instruction::raw(
                "store " + lower_scalar_type(param.type).str() + " " + incoming.str() +
                ", " + lower_scalar_type(param.type).str() + "* " + binding.storage));
            define_local(param.name, binding);
        }

        if (function->body.get() == 0)
        {
            return fail("Function body is missing.");
        }

        for (size_t index = 0; index < function->body->items.size(); ++index)
        {
            const ast::BlockItem *item = function->body->items[index].get();
            if (current_block_terminated())
            {
                break;
            }
            if (item->kind() == ast::NodeKind::Decl)
            {
                if (!lower_decl(static_cast<const ast::Decl *>(item)))
                {
                    return false;
                }
            }
            else
            {
                if (!lower_stmt(static_cast<const ast::Stmt *>(item)))
                {
                    return false;
                }
            }
        }

        if (!current_block_terminated() && function->return_type.kind == ast::TypeKind::Void)
        {
            current_block_->append(ir::Instruction::raw("ret void"));
        }
        else if (!current_block_terminated())
        {
            current_block_->append(ir::Instruction::raw("unreachable"));
        }

        pop_scope();
        return true;
    }
};
} // namespace

Result build_module(const ast::CompUnit &unit, const Options &options)
{
    Builder builder(options);
    return builder.build(unit);
}
} // namespace irgen
