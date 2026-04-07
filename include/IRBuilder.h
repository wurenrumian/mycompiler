#pragma once

#include <string>

#include "Semantic.h"

namespace ir
{
std::string build_runtime_prelude();
std::string build_placeholder_module(const semantic::ProgramInfo &program_info);
} // namespace ir
