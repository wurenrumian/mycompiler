#pragma once

#include <string>

struct CodegenOptions
{
	bool enable_optimizations = false;
	int optimization_level = 0;
	bool keep_temporary_file = false;
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
