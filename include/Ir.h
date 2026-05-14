#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ir
{
struct Type
{
    enum Kind
    {
        Void,
        I1,
        I32,
        Float,
        Pointer,
        Array
    } kind;

    std::shared_ptr<Type> element;
    int array_count;

    Type();

    static Type void_ty();
    static Type i1();
    static Type i32();
    static Type float_ty();
    static Type ptr(const Type &element_type);
    static Type array(int count, const Type &element_type);

    std::string str() const;
};

struct Value
{
    Type type;
    std::string name;
    bool constant;

    Value();

    static Value constant_i32(int value);
    static Value named(const Type &type, const std::string &name);
};

struct Instruction
{
    enum Kind
    {
        Ret,
        Br,
        CondBr,
        Call,
        Alloca,
        Load,
        Store,
        Gep,
        Binary,
        Icmp,
        Fcmp,
        Convert,
        Raw
    } kind;

    Type result_type;
    std::string result;
    std::string text;

    Instruction();

    static Instruction ret(const Value &value);
    static Instruction raw(const std::string &text);
};

struct BasicBlock
{
    std::string label;
    std::vector<Instruction> instructions;

    void append(const Instruction &instruction);
};

struct Function
{
    std::string name;
    Type return_type;
    std::vector<Type> params;
    std::vector<BasicBlock> blocks;
    int next_temp;

    Function();

    BasicBlock *create_block(const std::string &label);
    std::string temp();
};

struct Module
{
    std::vector<std::string> declarations;
    std::vector<Function> functions;

    void declare_runtime_functions();
    Function *create_function(const std::string &name,
                              const Type &return_type,
                              const std::vector<Type> &params);
};
} // namespace ir
