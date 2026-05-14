#include "IrText.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace irtext
{
namespace
{
std::string trim_copy(const std::string &text)
{
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0)
    {
        ++begin;
    }

    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
    {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool starts_with(const std::string &text, const std::string &prefix)
{
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> split_top_level_commas(const std::string &text)
{
    std::vector<std::string> parts;
    size_t start = 0;
    int square_depth = 0;
    int paren_depth = 0;
    for (size_t index = 0; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (ch == '[')
        {
            ++square_depth;
        }
        else if (ch == ']')
        {
            --square_depth;
        }
        else if (ch == '(')
        {
            ++paren_depth;
        }
        else if (ch == ')')
        {
            --paren_depth;
        }
        else if (ch == ',' && square_depth == 0 && paren_depth == 0)
        {
            parts.push_back(trim_copy(text.substr(start, index - start)));
            start = index + 1;
        }
    }
    parts.push_back(trim_copy(text.substr(start)));
    return parts;
}

size_t find_last_top_level_space(const std::string &text)
{
    int square_depth = 0;
    int paren_depth = 0;
    for (size_t index = text.size(); index > 0; --index)
    {
        const char ch = text[index - 1];
        if (ch == ']')
        {
            ++square_depth;
        }
        else if (ch == '[')
        {
            --square_depth;
        }
        else if (ch == ')')
        {
            ++paren_depth;
        }
        else if (ch == '(')
        {
            --paren_depth;
        }
        else if (std::isspace(static_cast<unsigned char>(ch)) != 0 && square_depth == 0 && paren_depth == 0)
        {
            return index - 1;
        }
    }
    return std::string::npos;
}

bool parse_typed_operand(const std::string &text, Operand *operand)
{
    if (operand == 0)
    {
        return false;
    }
    const std::string trimmed = trim_copy(text);
    const size_t split = find_last_top_level_space(trimmed);
    if (split == std::string::npos)
    {
        return false;
    }

    operand->type = trim_copy(trimmed.substr(0, split));
    operand->value = trim_copy(trimmed.substr(split + 1));
    operand->is_global = !operand->value.empty() && operand->value[0] == '@';
    operand->is_local = !operand->value.empty() && operand->value[0] == '%';
    operand->is_constant = !operand->is_global && !operand->is_local;
    return !operand->type.empty() && !operand->value.empty();
}

bool parse_global(const std::string &text, GlobalView *global, std::string *error)
{
    const size_t equal_pos = text.find(" = global ");
    if (equal_pos == std::string::npos)
    {
        if (error != 0)
        {
            *error = "Unsupported global declaration: " + text;
        }
        return false;
    }

    const std::string raw_name = trim_copy(text.substr(0, equal_pos));
    if (raw_name.empty() || raw_name[0] != '@')
    {
        if (error != 0)
        {
            *error = "Invalid global name: " + text;
        }
        return false;
    }
    global->name = raw_name.substr(1);

    const std::string tail = text.substr(equal_pos + 10);
    size_t split = std::string::npos;
    int square_depth = 0;
    for (size_t index = 0; index < tail.size(); ++index)
    {
        const char ch = tail[index];
        if (ch == '[')
        {
            ++square_depth;
        }
        else if (ch == ']')
        {
            --square_depth;
        }
        else if (std::isspace(static_cast<unsigned char>(ch)) != 0 && square_depth == 0)
        {
            split = index;
            break;
        }
    }
    if (split == std::string::npos)
    {
        if (error != 0)
        {
            *error = "Global declaration missing initializer: " + text;
        }
        return false;
    }

    const std::string type_text = trim_copy(tail.substr(0, split));
    const std::string init_text = trim_copy(tail.substr(split + 1));
    if (!parse_type(type_text, &global->type, error))
    {
        return false;
    }
    global->initializer = init_text;
    global->zero_initializer = init_text == "zeroinitializer";
    return true;
}

bool parse_call(const std::string &text, const std::string &result, InstructionView *instruction, std::string *error)
{
    const std::string body = trim_copy(text.substr(5));
    const size_t at_pos = body.find(" @");
    if (at_pos == std::string::npos)
    {
        if (error != 0)
        {
            *error = "Invalid call instruction: " + text;
        }
        return false;
    }

    instruction->kind = OpKind::Call;
    instruction->result = result;
    instruction->result_type = trim_copy(body.substr(0, at_pos));

    const std::string callee_and_args = trim_copy(body.substr(at_pos + 1));
    const size_t open_paren = callee_and_args.find('(');
    const size_t close_paren = callee_and_args.rfind(')');
    if (open_paren == std::string::npos || close_paren == std::string::npos || close_paren < open_paren)
    {
        if (error != 0)
        {
            *error = "Invalid call argument list: " + text;
        }
        return false;
    }

    const std::string callee_name = trim_copy(callee_and_args.substr(0, open_paren));
    instruction->callee = !callee_name.empty() && callee_name[0] == '@' ? callee_name.substr(1) : callee_name;

    const std::string args_text = trim_copy(callee_and_args.substr(open_paren + 1, close_paren - open_paren - 1));
    if (!args_text.empty())
    {
        const std::vector<std::string> parts = split_top_level_commas(args_text);
        for (size_t index = 0; index < parts.size(); ++index)
        {
            Operand operand;
            if (!parse_typed_operand(parts[index], &operand))
            {
                if (error != 0)
                {
                    *error = "Invalid call operand: " + parts[index];
                }
                return false;
            }
            instruction->args.push_back(operand);
        }
    }
    return true;
}

bool parse_branch_label(const std::string &text, std::string *label)
{
    const std::string trimmed = trim_copy(text);
    if (!starts_with(trimmed, "label %"))
    {
        return false;
    }
    *label = trim_copy(trimmed.substr(7));
    return true;
}

void fill_type_flags(TypeView *type)
{
    type->is_void = type->text == "void";
    type->is_i1 = type->text == "i1";
    type->is_i32 = type->text == "i32";
    type->is_float = type->text == "float";
}
} // namespace

TypeView::TypeView()
    : text(), is_void(false), is_i1(false), is_i32(false), is_float(false), pointer_depth(0), array_dims()
{
}

Operand::Operand() : type(), value(), is_constant(false), is_global(false), is_local(false)
{
}

InstructionView::InstructionView()
    : kind(OpKind::Unknown),
      raw(),
      result(),
      opcode(),
      result_type(),
      pointer_type(),
      operands(),
      callee(),
      args(),
      target_label(),
      true_label(),
      false_label()
{
}

GlobalView::GlobalView() : name(), type(), initializer(), zero_initializer(false)
{
}

bool parse_type(const std::string &text, TypeView *type, std::string *error)
{
    if (type == 0)
    {
        return false;
    }

    *type = TypeView();
    type->text = trim_copy(text);
    if (type->text.empty())
    {
        if (error != 0)
        {
            *error = "Empty type text.";
        }
        return false;
    }

    std::string base = type->text;
    while (!base.empty() && base[base.size() - 1] == '*')
    {
        ++type->pointer_depth;
        base.erase(base.size() - 1);
        base = trim_copy(base);
    }

    while (starts_with(base, "["))
    {
        const size_t x_pos = base.find(" x ");
        const size_t close_pos = base.rfind(']');
        if (x_pos == std::string::npos || close_pos == std::string::npos || close_pos <= x_pos + 3)
        {
            if (error != 0)
            {
                *error = "Unsupported array type: " + text;
            }
            return false;
        }
        const std::string count_text = trim_copy(base.substr(1, x_pos - 1));
        type->array_dims.push_back(std::atoi(count_text.c_str()));
        base = trim_copy(base.substr(x_pos + 3, close_pos - (x_pos + 3)));
    }

    fill_type_flags(type);
    if (type->pointer_depth == 0 && type->array_dims.empty())
    {
        fill_type_flags(type);
    }
    if (base == "void")
    {
        type->is_void = true;
    }
    else if (base == "i1")
    {
        type->is_i1 = true;
    }
    else if (base == "i32")
    {
        type->is_i32 = true;
    }
    else if (base == "float")
    {
        type->is_float = true;
    }
    return true;
}

bool parse_instruction(const std::string &text, InstructionView *instruction, std::string *error)
{
    if (instruction == 0)
    {
        return false;
    }

    *instruction = InstructionView();
    instruction->raw = trim_copy(text);
    std::string body = instruction->raw;
    std::string result;

    const size_t equal_pos = body.find(" = ");
    if (equal_pos != std::string::npos)
    {
        result = trim_copy(body.substr(0, equal_pos));
        body = trim_copy(body.substr(equal_pos + 3));
    }

    if (starts_with(body, "alloca "))
    {
        instruction->kind = OpKind::Alloca;
        instruction->result = result;
        instruction->result_type = trim_copy(body.substr(7));
        return true;
    }

    if (starts_with(body, "load "))
    {
        const std::vector<std::string> parts = split_top_level_commas(trim_copy(body.substr(5)));
        if (parts.size() != 2)
        {
            if (error != 0)
            {
                *error = "Invalid load instruction: " + text;
            }
            return false;
        }
        instruction->kind = OpKind::Load;
        instruction->result = result;
        instruction->result_type = parts[0];
        instruction->pointer_type = parts[1];
        Operand pointer_operand;
        if (!parse_typed_operand(parts[1], &pointer_operand))
        {
            if (error != 0)
            {
                *error = "Invalid load pointer operand: " + text;
            }
            return false;
        }
        instruction->operands.push_back(pointer_operand);
        return true;
    }

    if (starts_with(body, "store "))
    {
        const std::vector<std::string> parts = split_top_level_commas(trim_copy(body.substr(6)));
        if (parts.size() != 2)
        {
            if (error != 0)
            {
                *error = "Invalid store instruction: " + text;
            }
            return false;
        }
        instruction->kind = OpKind::Store;
        for (size_t index = 0; index < parts.size(); ++index)
        {
            Operand operand;
            if (!parse_typed_operand(parts[index], &operand))
            {
                if (error != 0)
                {
                    *error = "Invalid store operand: " + parts[index];
                }
                return false;
            }
            instruction->operands.push_back(operand);
        }
        instruction->pointer_type = instruction->operands[1].type;
        return true;
    }

    if (starts_with(body, "getelementptr "))
    {
        const std::vector<std::string> parts = split_top_level_commas(trim_copy(body.substr(14)));
        if (parts.size() < 2)
        {
            if (error != 0)
            {
                *error = "Invalid getelementptr instruction: " + text;
            }
            return false;
        }
        instruction->kind = OpKind::Gep;
        instruction->result = result;
        instruction->result_type = parts[0];
        instruction->pointer_type = parts[1];
        for (size_t index = 1; index < parts.size(); ++index)
        {
            Operand operand;
            if (!parse_typed_operand(parts[index], &operand))
            {
                if (error != 0)
                {
                    *error = "Invalid getelementptr operand: " + parts[index];
                }
                return false;
            }
            instruction->operands.push_back(operand);
        }
        return true;
    }

    if (starts_with(body, "icmp ") || starts_with(body, "fcmp "))
    {
        const bool is_icmp = starts_with(body, "icmp ");
        const size_t opcode_space = body.find(' ', 5);
        if (opcode_space == std::string::npos)
        {
            if (error != 0)
            {
                *error = "Invalid compare instruction: " + text;
            }
            return false;
        }
        instruction->kind = is_icmp ? OpKind::Icmp : OpKind::Fcmp;
        instruction->result = result;
        instruction->opcode = trim_copy(body.substr(5, opcode_space - 5));
        const std::string tail = trim_copy(body.substr(opcode_space + 1));
        const size_t type_space = tail.find(' ');
        const size_t comma_pos = tail.find(", ");
        if (type_space == std::string::npos || comma_pos == std::string::npos)
        {
            if (error != 0)
            {
                *error = "Invalid compare operands: " + text;
            }
            return false;
        }
        instruction->result_type = trim_copy(tail.substr(0, type_space));
        Operand lhs;
        lhs.type = instruction->result_type;
        lhs.value = trim_copy(tail.substr(type_space + 1, comma_pos - type_space - 1));
        lhs.is_global = !lhs.value.empty() && lhs.value[0] == '@';
        lhs.is_local = !lhs.value.empty() && lhs.value[0] == '%';
        lhs.is_constant = !lhs.is_global && !lhs.is_local;
        instruction->operands.push_back(lhs);

        Operand rhs;
        rhs.type = instruction->result_type;
        rhs.value = trim_copy(tail.substr(comma_pos + 2));
        rhs.is_global = !rhs.value.empty() && rhs.value[0] == '@';
        rhs.is_local = !rhs.value.empty() && rhs.value[0] == '%';
        rhs.is_constant = !rhs.is_global && !rhs.is_local;
        instruction->operands.push_back(rhs);
        return true;
    }

    if (starts_with(body, "zext ") || starts_with(body, "sext ") || starts_with(body, "trunc ") ||
        starts_with(body, "sitofp ") || starts_with(body, "fptosi "))
    {
        const size_t first_space = body.find(' ');
        const size_t to_pos = body.rfind(" to ");
        if (first_space == std::string::npos || to_pos == std::string::npos || to_pos <= first_space + 1)
        {
            if (error != 0)
            {
                *error = "Invalid convert instruction: " + text;
            }
            return false;
        }
        instruction->kind = OpKind::Convert;
        instruction->result = result;
        instruction->opcode = trim_copy(body.substr(0, first_space));
        instruction->result_type = trim_copy(body.substr(to_pos + 4));

        Operand operand;
        if (!parse_typed_operand(body.substr(first_space + 1, to_pos - first_space - 1), &operand))
        {
            if (error != 0)
            {
                *error = "Invalid convert operand: " + text;
            }
            return false;
        }
        instruction->operands.push_back(operand);
        return true;
    }

    if (starts_with(body, "call "))
    {
        return parse_call(body, result, instruction, error);
    }

    if (starts_with(body, "br label "))
    {
        instruction->kind = OpKind::Br;
        if (!parse_branch_label(trim_copy(body.substr(3)), &instruction->target_label))
        {
            if (error != 0)
            {
                *error = "Invalid branch target: " + text;
            }
            return false;
        }
        return true;
    }

    if (starts_with(body, "br i1 "))
    {
        const std::vector<std::string> parts = split_top_level_commas(trim_copy(body.substr(3)));
        if (parts.size() != 3)
        {
            if (error != 0)
            {
                *error = "Invalid conditional branch: " + text;
            }
            return false;
        }
        instruction->kind = OpKind::CondBr;
        Operand condition;
        if (!parse_typed_operand(parts[0], &condition))
        {
            if (error != 0)
            {
                *error = "Invalid branch condition: " + text;
            }
            return false;
        }
        instruction->operands.push_back(condition);
        if (!parse_branch_label(parts[1], &instruction->true_label) ||
            !parse_branch_label(parts[2], &instruction->false_label))
        {
            if (error != 0)
            {
                *error = "Invalid conditional branch labels: " + text;
            }
            return false;
        }
        return true;
    }

    if (starts_with(body, "ret "))
    {
        instruction->kind = OpKind::Ret;
        if (body == "ret void")
        {
            instruction->result_type = "void";
            return true;
        }
        Operand operand;
        if (!parse_typed_operand(trim_copy(body.substr(4)), &operand))
        {
            if (error != 0)
            {
                *error = "Invalid return operand: " + text;
            }
            return false;
        }
        instruction->result_type = operand.type;
        instruction->operands.push_back(operand);
        return true;
    }

    if (body == "unreachable")
    {
        instruction->kind = OpKind::Unreachable;
        return true;
    }

    const char *binary_ops[] = {"add", "sub", "mul", "sdiv", "srem", "and", "or", "xor", "fadd", "fsub", "fmul", "fdiv"};
    for (size_t index = 0; index < sizeof(binary_ops) / sizeof(binary_ops[0]); ++index)
    {
        const std::string prefix = std::string(binary_ops[index]) + " ";
        if (starts_with(body, prefix))
        {
            instruction->kind = OpKind::Binary;
            instruction->result = result;
            instruction->opcode = binary_ops[index];
            const std::string tail = trim_copy(body.substr(prefix.size()));
            const size_t type_space = tail.find(' ');
            const size_t comma_pos = tail.find(", ");
            if (type_space == std::string::npos || comma_pos == std::string::npos)
            {
                if (error != 0)
                {
                    *error = "Invalid binary instruction: " + text;
                }
                return false;
            }
            instruction->result_type = trim_copy(tail.substr(0, type_space));

            Operand lhs;
            lhs.type = instruction->result_type;
            lhs.value = trim_copy(tail.substr(type_space + 1, comma_pos - type_space - 1));
            lhs.is_global = !lhs.value.empty() && lhs.value[0] == '@';
            lhs.is_local = !lhs.value.empty() && lhs.value[0] == '%';
            lhs.is_constant = !lhs.is_global && !lhs.is_local;
            instruction->operands.push_back(lhs);

            Operand rhs;
            rhs.type = instruction->result_type;
            rhs.value = trim_copy(tail.substr(comma_pos + 2));
            rhs.is_global = !rhs.value.empty() && rhs.value[0] == '@';
            rhs.is_local = !rhs.value.empty() && rhs.value[0] == '%';
            rhs.is_constant = !rhs.is_global && !rhs.is_local;
            instruction->operands.push_back(rhs);
            return true;
        }
    }

    instruction->kind = OpKind::Unknown;
    return true;
}

bool parse_module(const ir::Module &module, ModuleView *view, std::string *error)
{
    if (view == 0)
    {
        return false;
    }

    *view = ModuleView();

    for (size_t index = 0; index < module.declarations.size(); ++index)
    {
        const std::string &declaration = module.declarations[index];
        if (!starts_with(trim_copy(declaration), "@"))
        {
            continue;
        }
        GlobalView global;
        if (!parse_global(declaration, &global, error))
        {
            return false;
        }
        view->globals.push_back(global);
    }

    for (size_t fn_index = 0; fn_index < module.functions.size(); ++fn_index)
    {
        const ir::Function &fn = module.functions[fn_index];
        FunctionView function_view;
        function_view.name = fn.name;
        if (!parse_type(fn.return_type.str(), &function_view.return_type, error))
        {
            return false;
        }
        for (size_t param_index = 0; param_index < fn.params.size(); ++param_index)
        {
            TypeView param_type;
            if (!parse_type(fn.params[param_index].str(), &param_type, error))
            {
                return false;
            }
            function_view.params.push_back(param_type);
        }

        for (size_t block_index = 0; block_index < fn.blocks.size(); ++block_index)
        {
            const ir::BasicBlock &block = fn.blocks[block_index];
            BlockView block_view;
            block_view.label = block.label;
            for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
            {
                InstructionView instruction;
                if (!parse_instruction(block.instructions[inst_index].text, &instruction, error))
                {
                    return false;
                }
                block_view.instructions.push_back(instruction);
            }
            function_view.blocks.push_back(block_view);
        }
        view->functions.push_back(function_view);
    }
    return true;
}
} // namespace irtext
