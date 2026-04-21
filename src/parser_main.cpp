#include <iostream>
#include <string>
#include "Codegen.h"

namespace
{
	void print_usage()
	{
		std::cerr << "Usage (RISC-V): parser -S -o <output.s> <input.sy>" << std::endl;
	}
}

int main(int argc, char **argv)
{
	bool emit_assembly = false;
	std::string input_filename;
	std::string output_filename;

	for (int index = 1; index < argc; ++index)
	{
		const std::string arg = argv[index];
		if (arg == "-S")
		{
			emit_assembly = true;
			continue;
		}
		if (arg == "-o")
		{
			if (index + 1 >= argc)
			{
				print_usage();
				return 1;
			}
			output_filename = argv[++index];
			continue;
		}
		if (!arg.empty() && arg[0] == '-')
		{
			print_usage();
			return 1;
		}
		if (!input_filename.empty())
		{
			print_usage();
			return 1;
		}
		input_filename = arg;
	}

	if (!emit_assembly || input_filename.empty() || output_filename.empty())
	{
		print_usage();
		return 1;
	}

	try
	{
		const CodegenOptions options = LLVMIRGenerator::from_environment();
		LLVMIRGenerator generator;
		std::string error;
		if (!generator.generate_from_file(input_filename, output_filename, options, &error))
		{
			return 1;
		}

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
