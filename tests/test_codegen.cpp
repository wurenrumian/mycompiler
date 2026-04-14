#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
bool run_command(const std::string &command)
{
	return std::system(command.c_str()) == 0;
}

std::string quote_arg(const std::string &value)
{
	if (value.find('\\') == std::string::npos && value.find('/') == std::string::npos && value.find(':') == std::string::npos)
	{
		return value;
	}
	return std::string("\"") + value + "\"";
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

#ifdef _WIN32
	return dir + "\\" + name;
#else
	return dir + "/" + name;
#endif
}

char path_list_separator()
{
#ifdef _WIN32
	return ';';
#else
	return ':';
#endif
}

const char *lli_executable_name()
{
#ifdef _WIN32
	return "lli.exe";
#else
	return "lli";
#endif
}

std::string find_tool_near_lli(const std::string &tool_name)
{
	const char *path_env = std::getenv("PATH");
	if (path_env == nullptr)
	{
		return std::string();
	}

	const std::string path_value(path_env);
	const char separator = path_list_separator();
	for (size_t start = 0; start <= path_value.size(); )
	{
		const size_t end = path_value.find(separator, start);
		const std::string entry = path_value.substr(start, end == std::string::npos ? std::string::npos : end - start);
		if (!entry.empty())
		{
			const std::string lli_path = join_path(entry, lli_executable_name());
			const std::string tool_path = join_path(entry, tool_name);
			std::ifstream lli_file(lli_path.c_str(), std::ios::binary);
			std::ifstream tool_file(tool_path.c_str(), std::ios::binary);
			if (lli_file.good() && tool_file.good())
			{
				return tool_path;
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

std::string preferred_clang()
{
	const char *from_env = std::getenv("LLVM_CLANG");
	if (from_env != nullptr && from_env[0] != '\0')
	{
		return from_env;
	}

#ifdef _WIN32
	const std::string sibling = find_tool_near_lli("clang.exe");
#else
	const std::string sibling = find_tool_near_lli("clang");
#endif
	if (!sibling.empty())
	{
		return sibling;
	}
	return "clang";
}

std::string preferred_llvm_link()
{
	#ifdef _WIN32
	const std::string sibling = find_tool_near_lli("llvm-link.exe");
	#else
	const std::string sibling = find_tool_near_lli("llvm-link");
	#endif
	if (!sibling.empty())
	{
		return sibling;
	}
	return "llvm-link";
}

bool run_command_capture(const std::string &command, const std::string &output_file)
{
#ifdef _WIN32
	const std::string full = command + " > " + output_file + " 2>&1";
#else
	const std::string full = command + " > " + output_file + " 2>&1";
#endif
	return run_command(full);
}

bool file_exists(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	return in.good();
}

std::string read_all(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

std::string normalize_output(std::string text)
{
	std::string out;
	out.reserve(text.size());
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (text[i] != '\r')
		{
			out.push_back(text[i]);
		}
	}
	while (!out.empty() && (out.back() == '\n' || out.back() == '\t'))
	{
		out.pop_back();
	}
	return out;
}

void remove_if_exists(const std::string &path)
{
	std::remove(path.c_str());
}

bool set_opt_level_env(int level)
{
	const std::string value = "MYCOMPILER_OPT_LEVEL=" + std::to_string(level);
#ifdef _WIN32
	return _putenv(value.c_str()) == 0;
#else
	return setenv("MYCOMPILER_OPT_LEVEL", std::to_string(level).c_str(), 1) == 0;
#endif
}

bool clear_opt_level_env()
{
#ifdef _WIN32
	return _putenv("MYCOMPILER_OPT_LEVEL=") == 0;
#else
	return unsetenv("MYCOMPILER_OPT_LEVEL") == 0;
#endif
}

void write_text(const std::string &path, const std::string &content)
{
	std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
	out << content;
}

bool run_parser_and_check_ir()
{
#ifdef _WIN32
	const std::string run_parser = "parser.exe";
#else
	const std::string run_parser = "./parser";
#endif
	if (!run_command_capture(run_parser, "codegen_test.log"))
	{
		std::cerr << "[FAIL] parser execution failed." << std::endl;
		return false;
	}

	if (!file_exists("output.ll"))
	{
		std::cerr << "[FAIL] output.ll was not generated." << std::endl;
		return false;
	}

	const std::string ir = read_all("output.ll");
	if (ir.find("@main") == std::string::npos)
	{
		std::cerr << "[FAIL] output.ll misses required symbol @main." << std::endl;
		return false;
	}

	const char *runtime_symbols[] = {
		"getint", "getch", "getarray", "putint", "putch", "putarray", "putf", "_sysy_starttime", "_sysy_stoptime"
	};
	for (size_t i = 0; i < sizeof(runtime_symbols) / sizeof(runtime_symbols[0]); ++i)
	{
		const std::string name = runtime_symbols[i];
		if (ir.find("define dso_local i32 @" + name) != std::string::npos ||
			ir.find("define dso_local void @" + name) != std::string::npos ||
			ir.find("define i32 @" + name) != std::string::npos ||
			ir.find("define void @" + name) != std::string::npos)
		{
			std::cerr << "[FAIL] output.ll still defines runtime symbol @" << name
				  << " instead of declaring it from sylib." << std::endl;
			return false;
		}
	}

	return true;
}

bool prepare_linked_runtime_ir()
{
	remove_if_exists("runtime_support.c");
	remove_if_exists("runtime_sylib.ll");
	remove_if_exists("linked_output.ll");

	const std::string clang_path = preferred_clang();
	const std::string llvm_link_path = preferred_llvm_link();
	write_text("runtime_support.c",
		      "extern int scanf(const char *, ...);\n"
		      "extern int printf(const char *, ...);\n"
		      "extern int vprintf(const char *, __builtin_va_list);\n"
		      "extern int getchar(void);\n"
		      "int getint(void) { int x = 0; scanf(\"%d\", &x); return x; }\n"
		      "int getch(void) { return getchar(); }\n"
		      "int getarray(int a[]) { int n = 0; scanf(\"%d\", &n); for (int i = 0; i < n; ++i) scanf(\"%d\", &a[i]); return n; }\n"
		      "void putint(int a) { printf(\"%d\", a); }\n"
		      "void putch(int a) { printf(\"%c\", a); }\n"
		      "void putarray(int n, int a[]) { printf(\"%d:\", n); for (int i = 0; i < n; ++i) printf(\" %d\", a[i]); printf(\"\\n\"); }\n"
		      "void putf(char a[], ...) { __builtin_va_list args; __builtin_va_start(args, a); vprintf(a, args); __builtin_va_end(args); }\n"
		      "void _sysy_starttime(int lineno) { (void)lineno; }\n"
		      "void _sysy_stoptime(int lineno) { (void)lineno; }\n");

	const std::string compile_runtime = quote_arg(clang_path) +
		" -S -emit-llvm runtime_support.c -o runtime_sylib.ll -O0";
	if (!run_command_capture(compile_runtime, "runtime_build.log"))
	{
		std::cerr << "[FAIL] failed to compile local runtime support into LLVM IR." << std::endl;
		return false;
	}

	const std::string link_modules = quote_arg(llvm_link_path) +
		" runtime_sylib.ll output.ll -S -o linked_output.ll";
	if (!run_command_capture(link_modules, "runtime_link.log"))
	{
		std::cerr << "[FAIL] failed to link runtime_sylib.ll with output.ll." << std::endl;
		return false;
	}

	return file_exists("linked_output.ll");
}

bool run_lli_and_expect(const std::string &input, const std::string &expected_output)
{
	remove_if_exists("lli_output.log");
#ifdef _WIN32
	const std::string cmd = input.empty()
						? "lli linked_output.ll"
						: "cmd /C \"echo " + input + " | lli linked_output.ll\"";
#else
	const std::string cmd = input.empty()
						? "lli linked_output.ll"
						: "printf \"" + input + "\\n\" | lli linked_output.ll";
#endif
	if (!run_command_capture(cmd, "lli_output.log"))
	{
		std::cerr << "[FAIL] lli execution failed." << std::endl;
		return false;
	}

	const std::string actual = normalize_output(read_all("lli_output.log"));
	const std::string expected = normalize_output(expected_output);
	if (actual != expected)
	{
		std::cerr << "[FAIL] lli output mismatch." << std::endl;
		std::cerr << "Expected:\n"
				  << expected
				  << "Actual:\n"
				  << actual << std::endl;
		return false;
	}

	return true;
}

bool run_lli_public_style_and_expect(const std::string &input,
					 const std::string &expected_output)
{
	remove_if_exists("judge_input.txt");
	remove_if_exists("judge_stdout.log");
	remove_if_exists("judge_exit.log");
	write_text("judge_input.txt", input);

#ifdef _WIN32
	const std::string cmd =
		"pwsh -NoProfile -Command \"$inputText = Get-Content 'judge_input.txt' -Raw; "
		"if ($inputText.Length -gt 0) { $inputText | lli linked_output.ll *> judge_stdout.log } else { lli linked_output.ll *> judge_stdout.log }; "
		"Set-Content -Path judge_exit.log -Value $LASTEXITCODE\"";
#else
	const std::string cmd =
		"sh -c 'lli linked_output.ll < judge_input.txt > judge_stdout.log 2>&1; printf \"%s\" $? > judge_exit.log'";
#endif

	if (!run_command(cmd))
	{
		std::cerr << "[FAIL] lli public-style execution failed." << std::endl;
		return false;
	}

	std::string actual = read_all("judge_stdout.log");
	const std::string exit_text = normalize_output(read_all("judge_exit.log"));
	int exit_code = std::atoi(exit_text.c_str());
	exit_code = ((exit_code % 256) + 256) % 256;
	if (!actual.empty() && actual[actual.size() - 1] != '\n')
	{
		actual.push_back('\n');
	}
	actual += std::to_string(exit_code);

	const std::string normalized_actual = normalize_output(actual);
	const std::string normalized_expected = normalize_output(expected_output);
	if (normalized_actual != normalized_expected)
	{
		std::cerr << "[FAIL] public-style output mismatch." << std::endl;
		std::cerr << "Expected:\n" << normalized_expected << "\nActual:\n" << normalized_actual << std::endl;
		return false;
	}

	return true;
}

bool run_lli_with_file_redirect_and_expect(const std::string &input,
					   const std::string &expected_output,
					   int expected_exit_code)
{
	remove_if_exists("judge_input.txt");
	remove_if_exists("judge_stdout.log");
	remove_if_exists("judge_exit.log");
	write_text("judge_input.txt", input.empty() ? std::string() : input + "\n");

#ifdef _WIN32
	const std::string cmd =
		"cmd /C \"lli linked_output.ll < judge_input.txt > judge_stdout.log 2>&1 & echo %ERRORLEVEL% > judge_exit.log\"";
#else
	const std::string cmd =
		"sh -c 'lli linked_output.ll < judge_input.txt > judge_stdout.log 2>&1; printf \"%s\" $? > judge_exit.log'";
#endif

	if (!run_command(cmd))
	{
		std::cerr << "[FAIL] lli judge-style execution failed." << std::endl;
		return false;
	}

	const std::string actual = normalize_output(read_all("judge_stdout.log"));
	const std::string expected = normalize_output(expected_output);
	if (actual != expected)
	{
		std::cerr << "[FAIL] judge-style lli output mismatch." << std::endl;
		std::cerr << "Expected:\n" << expected << "\nActual:\n" << actual << std::endl;
		return false;
	}

	const std::string exit_text = normalize_output(read_all("judge_exit.log"));
	if (exit_text != std::to_string(expected_exit_code))
	{
		std::cerr << "[FAIL] judge-style lli exit code mismatch. expected "
				  << expected_exit_code << ", actual " << exit_text << std::endl;
		return false;
	}

	return true;
}

bool run_case(const std::string &name,
			  const std::string &source,
			  const std::string &input,
			  const std::string &expected_output)
{
	write_text("testfile.txt", source);
	if (!run_parser_and_check_ir())
	{
		std::cerr << "[FAIL] case: " << name << std::endl;
		return false;
	}
	if (!prepare_linked_runtime_ir())
	{
		std::cerr << "[FAIL] case: " << name << std::endl;
		return false;
	}
	if (!run_lli_and_expect(input, expected_output))
	{
		std::cerr << "[FAIL] case: " << name << std::endl;
		return false;
	}
	std::cout << "[PASS] case: " << name << std::endl;
	return true;
}

bool run_public_case(const std::string &name,
			 const std::string &source_path,
			 const std::string &input_path,
			 const std::string &output_path)
{
	write_text("testfile.txt", read_all(source_path));
	if (!run_parser_and_check_ir())
	{
		std::cerr << "[FAIL] public case: " << name << std::endl;
		return false;
	}
	if (!prepare_linked_runtime_ir())
	{
		std::cerr << "[FAIL] public case: " << name << std::endl;
		return false;
	}
	const std::string input = file_exists(input_path) ? read_all(input_path) : std::string();
	const std::string expected = read_all(output_path);
	if (!run_lli_public_style_and_expect(input, expected))
	{
		std::cerr << "[FAIL] public case: " << name << std::endl;
		return false;
	}
	std::cout << "[PASS] public case: " << name << std::endl;
	return true;
}
} // namespace

int main()
{
#ifdef _WIN32
	const std::string check_clang = "clang --version >NUL 2>&1";
	const std::string check_lli = "lli --version >NUL 2>&1";
#else
	const std::string check_clang = "clang --version >/dev/null 2>&1";
	const std::string check_lli = "lli --version >/dev/null 2>&1";
#endif

	if (!run_command(check_clang))
	{
		std::cout << "[SKIP] clang not found, skip codegen integration test." << std::endl;
		return 0;
	}

	if (!run_command(check_lli))
	{
		std::cout << "[SKIP] lli not found, skip runtime validation." << std::endl;
		return 0;
	}

	if (!file_exists("..\\public\\testfile1.txt"))
	{
		std::cout << "[SKIP] public cases not found, skip public-case validation." << std::endl;
		return 0;
	}

	for (int i = 1; i <= 40; ++i)
	{
		const std::string id = std::to_string(i);
		const std::string source_path = std::string("..\\public\\testfile") + id + ".txt";
		const std::string input_path = std::string("..\\public\\input") + id + ".txt";
		const std::string output_path = std::string("..\\public\\output") + id + ".txt";
		if (!file_exists(source_path) || !file_exists(output_path))
		{
			continue;
		}
		if (!run_public_case(std::string("public_") + id, source_path, input_path, output_path))
		{
			remove_if_exists("testfile.txt");
			remove_if_exists("output.ll");
			remove_if_exists("codegen_test.log");
			remove_if_exists("lli_output.log");
			return 1;
		}
	}

	remove_if_exists("testfile.txt");
	remove_if_exists("output.ll");
	remove_if_exists("codegen_test.log");
	remove_if_exists("lli_output.log");
	remove_if_exists("judge_input.txt");
	remove_if_exists("judge_stdout.log");
	remove_if_exists("judge_exit.log");
	remove_if_exists("runtime_support.c");
	remove_if_exists("runtime_sylib.ll");
	remove_if_exists("linked_output.ll");
	remove_if_exists("runtime_build.log");
	remove_if_exists("runtime_link.log");

	std::cout << "[PASS] codegen+lli integration tests." << std::endl;
	return 0;
}
