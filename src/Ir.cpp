#include "Ir.h"

#include <sstream>

namespace ir
{
namespace
{
std::string render_value(const Value &value)
{
    return value.name;
}
} // namespace

Type::Type() : kind(Void), element(), array_count(0)
{
}

Type Type::void_ty()
{
    Type type;
    type.kind = Void;
    return type;
}

Type Type::i1()
{
    Type type;
    type.kind = I1;
    return type;
}

Type Type::i32()
{
    Type type;
    type.kind = I32;
    return type;
}

Type Type::float_ty()
{
    Type type;
    type.kind = Float;
    return type;
}

Type Type::ptr(const Type &element_type)
{
    Type type;
    type.kind = Pointer;
    type.element.reset(new Type(element_type));
    return type;
}

Type Type::array(int count, const Type &element_type)
{
    Type type;
    type.kind = Array;
    type.element.reset(new Type(element_type));
    type.array_count = count;
    return type;
}

std::string Type::str() const
{
    switch (kind)
    {
    case Void:
        return "void";
    case I1:
        return "i1";
    case I32:
        return "i32";
    case Float:
        return "float";
    case Pointer:
        return element.get() == 0 ? "ptr" : element->str() + "*";
    case Array:
    {
        std::ostringstream out;
        out << "[" << array_count << " x " << (element.get() == 0 ? "i32" : element->str()) << "]";
        return out.str();
    }
    }
    return "void";
}

Value::Value() : type(Type::void_ty()), name(), constant(false)
{
}

Value Value::constant_i32(int value)
{
    Value result;
    result.type = Type::i32();
    result.constant = true;
    std::ostringstream out;
    out << value;
    result.name = out.str();
    return result;
}

Value Value::named(const Type &type, const std::string &name)
{
    Value result;
    result.type = type;
    result.constant = false;
    result.name = name;
    return result;
}

Instruction::Instruction() : kind(Raw), result_type(Type::void_ty()), result(), text()
{
}

Instruction Instruction::ret(const Value &value)
{
    Instruction instruction;
    instruction.kind = Ret;
    instruction.result_type = Type::void_ty();
    instruction.text = "ret " + value.type.str() + " " + render_value(value);
    return instruction;
}

Instruction Instruction::raw(const std::string &text_value)
{
    Instruction instruction;
    instruction.kind = Raw;
    instruction.text = text_value;
    return instruction;
}

void BasicBlock::append(const Instruction &instruction)
{
    instructions.push_back(instruction);
}

Function::Function() : name(), return_type(Type::void_ty()), params(), blocks(), next_temp(0)
{
}

BasicBlock *Function::create_block(const std::string &label)
{
    BasicBlock block;
    block.label = label;
    blocks.push_back(block);
    return &blocks.back();
}

std::string Function::temp()
{
    std::ostringstream out;
    out << "%" << next_temp++;
    return out.str();
}

void Module::declare_runtime_functions()
{
    declarations.push_back("declare i32 @getint()");
    declarations.push_back("declare i32 @getch()");
    declarations.push_back("declare float @getfloat()");
    declarations.push_back("declare i32 @getarray(i32*)");
    declarations.push_back("declare i32 @getfarray(float*)");
    declarations.push_back("declare void @putint(i32)");
    declarations.push_back("declare void @putch(i32)");
    declarations.push_back("declare void @putfloat(float)");
    declarations.push_back("declare void @putarray(i32, i32*)");
    declarations.push_back("declare void @putfarray(i32, float*)");
    declarations.push_back("declare void @putf(i8*, ...)");
    declarations.push_back("declare void @_sysy_starttime(i32)");
    declarations.push_back("declare void @_sysy_stoptime(i32)");
}

Function *Module::create_function(const std::string &function_name,
                                  const Type &function_return_type,
                                  const std::vector<Type> &function_params)
{
    Function function;
    function.name = function_name;
    function.return_type = function_return_type;
    function.params = function_params;
    functions.push_back(function);
    return &functions.back();
}
} // namespace ir
