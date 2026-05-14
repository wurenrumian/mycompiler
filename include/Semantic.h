#pragma once

#include <string>

#include "Ast.h"

namespace semantic
{
enum class ValueCategory
{
	Scalar,
	Array,
	Function
};

struct SemanticResult
{
	bool ok;
	std::string message;

	SemanticResult() : ok(false), message() {}
};

struct AnalyzeOptions
{
	bool allow_main_return_output;

	AnalyzeOptions() : allow_main_return_output(false) {}
};

SemanticResult analyze_program(const ast::CompUnit &root,
			       const AnalyzeOptions &options);

bool analyze_program(const ast::CompUnit &root,
		     std::string *error_message);
} // namespace semantic
