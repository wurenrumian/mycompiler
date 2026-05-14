#include "IrPrinter.h"

#include <sstream>

namespace ir
{
std::string print_module(const Module &module)
{
    std::ostringstream out;

    for (size_t i = 0; i < module.declarations.size(); ++i)
    {
        out << module.declarations[i] << "\n";
    }

    if (!module.declarations.empty())
    {
        out << "\n";
    }

    for (size_t function_index = 0; function_index < module.functions.size(); ++function_index)
    {
        const Function &function = module.functions[function_index];
        out << "define " << function.return_type.str() << " @" << function.name << "(";
        for (size_t param_index = 0; param_index < function.params.size(); ++param_index)
        {
            if (param_index != 0)
            {
                out << ", ";
            }
            out << function.params[param_index].str();
        }
        out << ") {\n";

        for (size_t block_index = 0; block_index < function.blocks.size(); ++block_index)
        {
            const BasicBlock &block = function.blocks[block_index];
            out << block.label << ":\n";
            for (size_t instruction_index = 0; instruction_index < block.instructions.size(); ++instruction_index)
            {
                out << "  " << block.instructions[instruction_index].text << "\n";
            }
        }

        out << "}\n";
        if (function_index + 1 != module.functions.size())
        {
            out << "\n";
        }
    }

    return out.str();
}
} // namespace ir
