#pragma once

#include <string>

struct CodegenOptions
{
    bool enable_optimizations;
    int optimization_level;
    bool keep_temporary_file;

    CodegenOptions() : enable_optimizations(false), optimization_level(0), keep_temporary_file(false) {}
};

class CompilerPipeline
{
public:
    static CodegenOptions from_environment();

    bool emit_llvm_ir_from_file(const std::string &input_path,
                                const std::string &output_path,
                                const CodegenOptions &options,
                                std::string *error_message) const;

    bool emit_assembly_from_file(const std::string &input_path,
                                 const std::string &output_path,
                                 const CodegenOptions &options,
                                 std::string *error_message) const;
};

class LLVMIRGenerator
{
public:
    static CodegenOptions from_environment();

    bool generate_from_file(const std::string &input_path,
                            const std::string &output_path,
                            const CodegenOptions &options,
                            std::string *error_message) const;
};
