#pragma once

#include "Ir.h"

#include <string>
#include <vector>

namespace irtext
{
struct TypeView
{
    std::string text;
    bool is_void;
    bool is_i1;
    bool is_i32;
    bool is_float;
    int pointer_depth;
    std::vector<int> array_dims;

    TypeView();
};

struct Operand
{
    std::string type;
    std::string value;
    bool is_constant;
    bool is_global;
    bool is_local;

    Operand();
};

enum class OpKind
{
    Alloca,
    Load,
    Store,
    Gep,
    Binary,
    Icmp,
    Fcmp,
    Convert,
    Call,
    Ret,
    Br,
    CondBr,
    Unreachable,
    Unknown
};

struct InstructionView
{
    OpKind kind;
    std::string raw;
    std::string result;
    std::string opcode;
    std::string result_type;
    std::string pointer_type;
    std::vector<Operand> operands;
    std::string callee;
    std::vector<Operand> args;
    std::string target_label;
    std::string true_label;
    std::string false_label;

    InstructionView();
};

struct GlobalView
{
    std::string name;
    TypeView type;
    std::string initializer;
    bool zero_initializer;

    GlobalView();
};

struct BlockView
{
    std::string label;
    std::vector<InstructionView> instructions;
};

struct FunctionView
{
    std::string name;
    TypeView return_type;
    std::vector<TypeView> params;
    std::vector<BlockView> blocks;
};

struct ModuleView
{
    std::vector<GlobalView> globals;
    std::vector<FunctionView> functions;
};

bool parse_type(const std::string &text, TypeView *type, std::string *error);
bool parse_instruction(const std::string &text, InstructionView *instruction, std::string *error);
bool parse_module(const ir::Module &module, ModuleView *view, std::string *error);
} // namespace irtext
