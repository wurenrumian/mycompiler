#pragma once

#include "Ir.h"

#include <string>

namespace riscv
{
struct Options
{
    bool emit_comments;
    bool optimize_for_size;

    Options() : emit_comments(true), optimize_for_size(false) {}
};

struct Result
{
    bool ok;
    std::string message;
    std::string assembly;

    Result() : ok(false), message(), assembly() {}
};

Result emit_module(const ir::Module &module, const Options &options);
} // namespace riscv
