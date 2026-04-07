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
	while (!out.empty() && (out.back() == '\n' || out.back() == ' ' || out.back() == '\t'))
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
	if (ir.find("@main") == std::string::npos || ir.find("@getint") == std::string::npos)
	{
		std::cerr << "[FAIL] output.ll misses required symbols (@main/@getint)." << std::endl;
		return false;
	}

	return true;
}

bool run_lli_and_expect(const std::string &input, const std::string &expected_output)
{
	remove_if_exists("lli_output.log");
#ifdef _WIN32
	const std::string cmd = input.empty()
						? "lli output.ll"
						: "cmd /C \"echo " + input + " | lli output.ll\"";
#else
	const std::string cmd = input.empty()
						? "lli output.ll"
						: "printf \"" + input + "\\n\" | lli output.ll";
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
		"cmd /C \"lli output.ll < judge_input.txt > judge_stdout.log 2>&1 & echo %ERRORLEVEL% > judge_exit.log\"";
#else
	const std::string cmd =
		"sh -c 'lli output.ll < judge_input.txt > judge_stdout.log 2>&1; printf \"%s\" $? > judge_exit.log'";
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
	if (!run_lli_and_expect(input, expected_output))
	{
		std::cerr << "[FAIL] case: " << name << std::endl;
		return false;
	}
	std::cout << "[PASS] case: " << name << std::endl;
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

	struct TestCase
	{
		std::string name;
		std::string source;
		std::string input;
		std::string expected;
	};

	std::vector<TestCase> cases;
	cases.push_back(TestCase{"hello_getint",
		"int test;\n"
		"int main(){\n"
		"    printf(\"Hello World\\n\");\n"
		"    test = getint();\n"
		"    printf(\"%d\",test);\n"
		"    return 0;\n"
		"}\n",
		"1906",
		"Hello World\n1906"});

	cases.push_back(TestCase{"short_circuit",
		"int count = 0;\n"
		"int f () {\n"
		"    count = count + 1;\n"
		"    return count;\n"
		"}\n"
		"int main() {\n"
		"    int arr1[2] = {1,1};\n"
		"    int i = 0;\n"
		"    int k = 0;\n"
		"    if (i < -1 && f() < 1) {\n"
		"        printf(\"count: %d\\n\", count);\n"
		"    }\n"
		"    if (i < -1 && f() < 1 || k < 2) {\n"
		"        printf(\"count: %d\\n\", count);\n"
		"    }\n"
		"    printf(\"count: %d\\n\", count);\n"
		"    return 0;\n"
		"}\n",
		"",
		"count: 0\ncount: 0\n"});

	cases.push_back(TestCase{"const_arith",
		"int main(){\n"
		"    const int a = 5;\n"
		"    const int b = 7;\n"
		"    printf(\"%d\\n\", a * b + 3);\n"
		"    return 0;\n"
		"}\n",
		"",
		"38\n"});

	cases.push_back(TestCase{"if_else",
		"int main(){\n"
		"    int x = 3;\n"
		"    if (x > 5) {\n"
		"        printf(\"big\\n\");\n"
		"    } else {\n"
		"        printf(\"small\\n\");\n"
		"    }\n"
		"    return 0;\n"
		"}\n",
		"",
		"small\n"});

	cases.push_back(TestCase{"while_sum",
		"int main(){\n"
		"    int i = 1;\n"
		"    int s = 0;\n"
		"    while (i <= 5) {\n"
		"        s = s + i;\n"
		"        i = i + 1;\n"
		"    }\n"
		"    printf(\"%d\\n\", s);\n"
		"    return 0;\n"
		"}\n",
		"",
		"15\n"});

	cases.push_back(TestCase{"break_continue",
		"int main(){\n"
		"    int i = 0;\n"
		"    int s = 0;\n"
		"    while (i < 10) {\n"
		"        i = i + 1;\n"
		"        if (i == 3) continue;\n"
		"        if (i == 7) break;\n"
		"        s = s + i;\n"
		"    }\n"
		"    printf(\"%d\\n\", s);\n"
		"    return 0;\n"
		"}\n",
		"",
		"18\n"});

	cases.push_back(TestCase{"function_call",
		"int add(int a, int b){\n"
		"    return a + b;\n"
		"}\n"
		"int main(){\n"
		"    printf(\"%d\\n\", add(10, 32));\n"
		"    return 0;\n"
		"}\n",
		"",
		"42\n"});

	cases.push_back(TestCase{"array_access",
		"int main(){\n"
		"    int a[3] = {2,4,6};\n"
		"    printf(\"%d\\n\", a[0] + a[2]);\n"
		"    return 0;\n"
		"}\n",
		"",
		"8\n"});

	cases.push_back(TestCase{"logical_ops",
		"int main(){\n"
		"    int a = 0;\n"
		"    int b = 1;\n"
		"    if (!a && b) {\n"
		"        printf(\"ok\\n\");\n"
		"    }\n"
		"    return 0;\n"
		"}\n",
		"",
		"ok\n"});

	cases.push_back(TestCase{"unary_precedence",
		"int main(){\n"
		"    int x = -3 + +5 * 2;\n"
		"    printf(\"%d\\n\", x);\n"
		"    return 0;\n"
		"}\n",
		"",
		"7\n"});

	cases.push_back(TestCase{"putint_putch",
		"int main(){\n"
		"    putint(123);\n"
		"    putch(10);\n"
		"    return 0;\n"
		"}\n",
		"",
		"123\n"});

	cases.push_back(TestCase{"getch_builtin",
		"int main(){\n"
		"    int c;\n"
		"    c = getch();\n"
		"    putint(c);\n"
		"    return 0;\n"
		"}\n",
		"A",
		"65"});

	cases.push_back(TestCase{"builtin_call_in_initializer",
		"int readint(){\n"
		"    int x = 0, f = 0, ch = getch();\n"
		"    while (ch < 48 || ch > 57) {\n"
		"        if (ch == 45) f = 1;\n"
		"        ch = getch();\n"
		"    }\n"
		"    while (ch >= 48 && ch <= 57) {\n"
		"        x = x * 10 + ch - 48;\n"
		"        ch = getch();\n"
		"    }\n"
		"    if (f) return -x;\n"
		"    return x;\n"
		"}\n"
		"int main(){\n"
		"    printf(\"%d\\n\", readint());\n"
		"    return 0;\n"
		"}\n",
		"-120",
		"-120\n"});

	for (size_t i = 0; i < cases.size(); ++i)
	{
		if (!run_case(cases[i].name, cases[i].source, cases[i].input, cases[i].expected))
		{
			remove_if_exists("testfile.txt");
			remove_if_exists("output.ll");
			remove_if_exists("codegen_test.log");
			remove_if_exists("lli_output.log");
			return 1;
		}
	}

	if (!set_opt_level_env(2))
	{
		std::cerr << "[FAIL] failed to set MYCOMPILER_OPT_LEVEL." << std::endl;
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		remove_if_exists("lli_output.log");
		return 1;
	}
	write_text("testfile.txt",
			  "int main(){\n"
			  "    return 0;\n"
			  "}\n");
	if (!run_parser_and_check_ir())
	{
		clear_opt_level_env();
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		remove_if_exists("lli_output.log");
		return 1;
	}
	clear_opt_level_env();

	write_text("testfile.txt",
			  "int main(){\n"
			  "    printf(\"42\");\n"
			  "    return 0;\n"
			  "}\n");
	if (!run_parser_and_check_ir())
	{
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		remove_if_exists("lli_output.log");
		return 1;
	}
	if (!run_lli_with_file_redirect_and_expect("", "42", 0))
	{
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		remove_if_exists("lli_output.log");
		remove_if_exists("judge_input.txt");
		remove_if_exists("judge_stdout.log");
		remove_if_exists("judge_exit.log");
		return 1;
	}

	remove_if_exists("testfile.txt");
	remove_if_exists("output.ll");
	remove_if_exists("codegen_test.log");
	remove_if_exists("lli_output.log");
	remove_if_exists("judge_input.txt");
	remove_if_exists("judge_stdout.log");
	remove_if_exists("judge_exit.log");

	std::cout << "[PASS] codegen+lli integration tests." << std::endl;
	return 0;
}
