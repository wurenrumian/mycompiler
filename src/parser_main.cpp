#include <iostream>
#include <string>
#include "Codegen.h"

int main()
{
	const char *input_filename = "testfile.txt";
	const char *output_filename = "output.ll";

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
