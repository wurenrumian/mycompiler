#pragma once

#include <string>

#include "Ast.h"
#include "Ir.h"

namespace irgen
{
struct Options
{
    bool emit_main_return_value;
    bool optimize;

    Options() : emit_main_return_value(false), optimize(false) {}
};

struct Result
{
    bool ok;
    std::string message;
    ir::Module module;

    Result() : ok(false), message(), module() {}
};

Result build_module(const ast::CompUnit &unit, const Options &options);
} // namespace irgen
