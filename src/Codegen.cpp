#include "Codegen.h"

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <fstream>
#include <cstring>
#include <sstream>
#include <vector>

#include "ParserFrontend.h"
#include "Semantic.h"

namespace
{
int run_command(const std::string &command)
{
	return std::system(command.c_str());
}

bool command_exists(const std::string &exe)
{
#ifdef _WIN32
	const std::string cmd = exe + " --version >NUL 2>&1";
#else
	const std::string cmd = exe + " --version >/dev/null 2>&1";
#endif
	return run_command(cmd) == 0;
}

std::string join_path(const std::string &dir, const std::string &name)
{
	if (dir.empty())
	{
		return name;
	}
	const char last = dir[dir.size() - 1];
	if (last == '\\' || last == '/')
	{
		return dir + name;
	}
	return dir + "\\" + name;
}

std::string find_lli_sibling_clang()
{
	const char *path_env = std::getenv("PATH");
	if (path_env == nullptr)
	{
		return std::string();
	}

	const std::string path_value(path_env);
	size_t start = 0;
	while (start <= path_value.size())
	{
		size_t end = path_value.find(';', start);
		const std::string entry = path_value.substr(start, end == std::string::npos ? std::string::npos : end - start);
		if (!entry.empty())
		{
			const std::string lli_path = join_path(entry, "lli.exe");
			const std::string clang_path = join_path(entry, "clang.exe");
			std::ifstream lli_file(lli_path.c_str(), std::ios::binary);
			std::ifstream clang_file(clang_path.c_str(), std::ios::binary);
			if (lli_file.good() && clang_file.good() && command_exists(clang_path))
			{
				return clang_path;
			}
		}
		if (end == std::string::npos)
		{
			break;
		}
		start = end + 1;
	}

	return std::string();
}

std::string find_clang_executable()
{
	const char *from_env = std::getenv("LLVM_CLANG");
	if (from_env != nullptr && from_env[0] != '\0' && command_exists(from_env))
	{
		return std::string(from_env);
	}

	const std::string sibling = find_lli_sibling_clang();
	if (!sibling.empty())
	{
		return sibling;
	}

	const char *candidates[] = {"clang", "clang-18", "clang-17", "clang-16"};
	for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
	{
		if (command_exists(candidates[i]))
		{
			return std::string(candidates[i]);
		}
	}

	return std::string();
}

std::string build_driver_prelude()
{
	std::ostringstream os;
	os << "int getint(void);\n";
	os << "int getch(void);\n";
	os << "int getarray(int a[]);\n";
	os << "void putint(int a);\n";
	os << "void putch(int a);\n";
	os << "void putarray(int n, int a[]);\n";
	os << "void putf(char a[], ...);\n";
	os << "void _sysy_starttime(int lineno);\n";
	os << "void _sysy_stoptime(int lineno);\n";
	os << "#define starttime() _sysy_starttime(__LINE__)\n";
	os << "#define stoptime() _sysy_stoptime(__LINE__)\n";
	os << "#define printf putf\n\n";
	return os.str();
}

std::string trim_copy(const std::string &text)
{
	size_t begin = 0;
	while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0)
	{
		++begin;
	}

	size_t end = text.size();
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
	{
		--end;
	}

	return text.substr(begin, end - begin);
}

bool is_builtin_decl_line(const std::string &line)
{
	static const char *names[] = {
		"getint", "getch", "getarray", "putint", "putch", "putarray", "putf",
		"starttime", "stoptime", "_sysy_starttime", "_sysy_stoptime"};

	const std::string t = trim_copy(line);
	if (t.empty() || t[t.size() - 1] != ';')
	{
		return false;
	}

	if (!(t.find("int ") == 0 || t.find("void ") == 0 || t.find("const int ") == 0))
	{
		return false;
	}

	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
	{
		const std::string needle = std::string(names[i]) + "(";
		const size_t pos = t.find(needle);
		if (pos != std::string::npos)
		{
			const std::string prefix = trim_copy(t.substr(0, pos));
			if (prefix == "int" || prefix == "void" || prefix == "const int")
			{
				return true;
			}
		}
	}

	return false;
}

std::string strip_builtin_declarations(const std::string &source)
{
	std::istringstream in(source);
	std::ostringstream out;
	std::string line;

	while (std::getline(in, line))
	{
		if (!is_builtin_decl_line(line))
		{
			out << line << '\n';
		}
	}

	return out.str();
}

std::string strip_host_specific_llvm_module_lines(const std::string &ir)
{
	std::istringstream in(ir);
	std::ostringstream out;
	std::string line;

	while (std::getline(in, line))
	{
		const std::string trimmed = trim_copy(line);
		if (trimmed.find("target datalayout = ") == 0 || trimmed.find("target triple = ") == 0)
		{
			continue;
		}
		out << line << '\n';
	}

	return out.str();
}
} // namespace

CodegenOptions LLVMIRGenerator::from_environment()
{
	CodegenOptions options;

	const char *opt_level_env = std::getenv("MYCOMPILER_OPT_LEVEL");
	if (opt_level_env != nullptr && opt_level_env[0] != '\0')
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
	if (keep_temp_env != nullptr && keep_temp_env[0] == '1')
	{
		options.keep_temporary_file = true;
	}

	return options;
}

bool LLVMIRGenerator::generate_from_file(const std::string &input_path,
					 const std::string &output_path,
					 const CodegenOptions &options,
					 std::string *error_message) const
{
	std::ifstream input(input_path.c_str(), std::ios::binary);
	if (!input.is_open())
	{
		if (error_message != nullptr)
		{
			*error_message = "Failed to open source file: " + input_path;
		}
		return false;
	}

	std::ostringstream source_buffer;
	source_buffer << input.rdbuf();
	const std::string stripped_source = strip_builtin_declarations(source_buffer.str());

	const std::string temp_parse_file = "__tmp_parse_input.sy";
	{
		std::ofstream parse_input(temp_parse_file.c_str(), std::ios::binary | std::ios::trunc);
		if (!parse_input.is_open())
		{
			if (error_message != nullptr)
			{
				*error_message = "Failed to create parser input file: " + temp_parse_file;
			}
			return false;
		}
		parse_input << stripped_source;
	}

	std::unique_ptr<ast::CompUnit> ast_root;
	std::string parse_error;
	const bool parsed = parse_file_to_ast(temp_parse_file, &ast_root, &parse_error);
	if (!options.keep_temporary_file)
	{
		std::remove(temp_parse_file.c_str());
	}
	if (!parsed)
	{
		if (error_message != nullptr)
		{
			*error_message = parse_error;
		}
		return false;
	}

	if (!ast_root || ast_root->items.empty())
	{
		if (error_message != nullptr)
		{
			*error_message = "Parser produced an empty AST.";
		}
		return false;
	}

	semantic::ProgramInfo program_info;
	std::string semantic_error;
	if (!semantic::analyze_program(*ast_root, &program_info, &semantic_error))
	{
		if (error_message != nullptr)
		{
			*error_message = semantic_error;
		}
		return false;
	}

	const std::string temp_source_file = "__tmp_codegen_input.c";
	std::ofstream temp(temp_source_file.c_str(), std::ios::binary | std::ios::trunc);
	if (!temp.is_open())
	{
		if (error_message != nullptr)
		{
			*error_message = "Failed to create temporary source file: " + temp_source_file;
		}
		return false;
	}

	temp << build_driver_prelude();
	temp << stripped_source;
	temp.close();

	const std::string clang_exe = find_clang_executable();
	if (clang_exe.empty())
	{
		if (!options.keep_temporary_file)
		{
			std::remove(temp_source_file.c_str());
		}
		if (error_message != nullptr)
		{
			*error_message = "No clang executable found. Install LLVM/clang or set LLVM_CLANG.";
		}
		return false;
	}

	std::ostringstream cmd;
	cmd << clang_exe
		<< " -S -emit-llvm -x c"
		<< " " << temp_source_file
		<< " -o " << output_path
		<< " -O" << (options.enable_optimizations ? options.optimization_level : 0);

	const int status = run_command(cmd.str());

	if (!options.keep_temporary_file)
	{
		std::remove(temp_source_file.c_str());
	}

	if (status != 0)
	{
		if (error_message != nullptr)
		{
			*error_message = "clang failed while generating LLVM IR from temporary source: " + temp_source_file +
				" (clang=" + clang_exe + ")";
		}
		return false;
	}

	std::ifstream generated_ir(output_path.c_str(), std::ios::binary);
	if (!generated_ir.is_open())
	{
		if (error_message != nullptr)
		{
			*error_message = "Generated LLVM IR file is missing: " + output_path;
		}
		return false;
	}

	std::ostringstream ir_buffer;
	ir_buffer << generated_ir.rdbuf();
	generated_ir.close();

	std::ofstream normalized_ir(output_path.c_str(), std::ios::binary | std::ios::trunc);
	if (!normalized_ir.is_open())
	{
		if (error_message != nullptr)
		{
			*error_message = "Failed to normalize LLVM IR file: " + output_path;
		}
		return false;
	}

	normalized_ir << strip_host_specific_llvm_module_lines(ir_buffer.str());
	normalized_ir.close();

	return true;
}
