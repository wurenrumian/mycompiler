#include "Codegen.h"

#include <cstdlib>
#include <fstream>
#include <memory>

#include "IrBuilder.h"
#include "IrPrinter.h"
#include "ParserFrontend.h"
#include "RiscVBackend.h"
#include "Semantic.h"

namespace
{
bool parse_and_analyze(const std::string &input_path,
                       std::unique_ptr<ast::CompUnit> *root,
                       std::string *error_message)
{
    std::string parse_error;
    if (!parse_file_to_ast(input_path, root, &parse_error))
    {
        if (error_message != 0)
        {
            *error_message = parse_error.empty() ? "Parser failed." : parse_error;
        }
        return false;
    }

    semantic::SemanticResult semantic_result =
        semantic::analyze_program(**root, semantic::AnalyzeOptions());
    if (!semantic_result.ok)
    {
        if (error_message != 0)
        {
            *error_message = semantic_result.message.empty() ? "Semantic analysis failed." : semantic_result.message;
        }
        return false;
    }
    return true;
}

bool write_file(const std::string &output_path,
                const std::string &content,
                std::string *error_message)
{
    std::ofstream out(output_path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        if (error_message != 0)
        {
            *error_message = "Failed to open output file: " + output_path;
        }
        return false;
    }
    out << content;
    return true;
}
} // namespace

CodegenOptions CompilerPipeline::from_environment()
{
    CodegenOptions options;

    const char *opt_level_env = std::getenv("MYCOMPILER_OPT_LEVEL");
    if (opt_level_env != 0 && opt_level_env[0] != '\0')
    {
        int value = std::atoi(opt_level_env);
        if (value < 0)
        {
            value = 0;
        }
        if (value > 3)
        {
            value = 3;
        }
        options.optimization_level = value;
        options.enable_optimizations = value > 0;
    }

    const char *keep_temp_env = std::getenv("MYCOMPILER_KEEP_TEMP");
    if (keep_temp_env != 0 && keep_temp_env[0] == '1')
    {
        options.keep_temporary_file = true;
    }

    return options;
}

bool CompilerPipeline::emit_llvm_ir_from_file(const std::string &input_path,
                                              const std::string &output_path,
                                              const CodegenOptions &options,
                                              std::string *error_message) const
{
    (void)options;

    std::unique_ptr<ast::CompUnit> root;
    if (!parse_and_analyze(input_path, &root, error_message))
    {
        return false;
    }

    irgen::Options ir_options;
    ir_options.emit_main_return_value = true;
    irgen::Result ir_result = irgen::build_module(*root, ir_options);
    if (!ir_result.ok)
    {
        if (error_message != 0)
        {
            *error_message = ir_result.message.empty() ? "IR generation failed." : ir_result.message;
        }
        return false;
    }

    return write_file(output_path, ir::print_module(ir_result.module), error_message);
}

bool CompilerPipeline::emit_assembly_from_file(const std::string &input_path,
                                               const std::string &output_path,
                                               const CodegenOptions &options,
                                               std::string *error_message) const
{
    (void)options;

    std::unique_ptr<ast::CompUnit> root;
    if (!parse_and_analyze(input_path, &root, error_message))
    {
        return false;
    }

    irgen::Options ir_options;
    ir_options.emit_main_return_value = false;
    irgen::Result ir_result = irgen::build_module(*root, ir_options);
    if (!ir_result.ok)
    {
        if (error_message != 0)
        {
            *error_message = ir_result.message.empty() ? "IR generation failed." : ir_result.message;
        }
        return false;
    }

    riscv::Options backend_options;
    riscv::Result asm_result = riscv::emit_module(ir_result.module, backend_options);
    if (!asm_result.ok)
    {
        if (error_message != 0)
        {
            *error_message = asm_result.message.empty() ? "RISC-V backend failed." : asm_result.message;
        }
        return false;
    }

    return write_file(output_path, asm_result.assembly, error_message);
}

CodegenOptions LLVMIRGenerator::from_environment()
{
    return CompilerPipeline::from_environment();
}

bool LLVMIRGenerator::generate_from_file(const std::string &input_path,
                                         const std::string &output_path,
                                         const CodegenOptions &options,
                                         std::string *error_message) const
{
    CompilerPipeline pipeline;
    return pipeline.emit_assembly_from_file(input_path, output_path, options, error_message);
}
