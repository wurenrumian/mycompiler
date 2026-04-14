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
		for (size_t start = 0; start <= path_value.size();)
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
		remove_if_exists("judge_input.txt");
		write_text("judge_input.txt", input);
#ifdef _WIN32
		const std::string cmd =
			"pwsh -NoProfile -Command \"$inputText = Get-Content 'judge_input.txt' -Raw; "
			"if ($inputText.Length -gt 0) { $inputText | lli linked_output.ll *> lli_output.log } else { lli linked_output.ll *> lli_output.log }; "
			"exit 0\"";
#else
		const std::string cmd =
			"sh -c 'lli linked_output.ll < judge_input.txt > lli_output.log 2>&1; true'";
#endif
		if (!run_command(cmd))
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

	const std::string complex_test_source = R"(
const int G_CONST_A = 10;
const int G_CONST_B = G_CONST_A * 2 + 5;
const int G_CONST_C = - + - G_CONST_A;

int global_var = 100;
int global_arr[2][3] = {{1, 2, 3}, {4, 5, 6}};

int test_complex_math(int arr[][3], int x, int y) {
    int a = arr[x % 2][y % 3];
    int b = arr[(x + 1) % 2][(y + 2) % 3];
    int result = (a + b * G_CONST_B - x / y % 2) + (- + - x) - (-(-y)) + a * (b - x) / (y + 1);
    int i = 0;
    while (i < 5) {
        result = result + i * 2 - (result % (i + 1));
        i = i + 1;
    }
    return result; 
}

void test_deep_scopes() {
    int val = 1;
    int shadow_var = 10;
    
    printf("Scope 0: val=%d, shadow_var=%d\n", val, shadow_var);
    
    {
        int val = 2;
        shadow_var = 20;
        int hidden = 100;
        printf("Scope 1: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
        
        if (val != 1) {
            int shadow_var = 30;
            int hidden = 200;
            val = val + 1;
            
            printf("Scope 2 (if): val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
            
            {
                int val = shadow_var + hidden;
                printf("Scope 3 (block): val=%d\n", val);
            }
            
            printf("Scope 2 (if) after: val=%d\n", val);
        }
        
        printf("Scope 1 after: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
    }
    
    printf("Scope 0 after: val=%d, shadow_var=%d\n", val, shadow_var);
}

void test_add_sub_scope_stress() {
    int base = 40;
    int delta = 3;
    int mix = base - delta + (base - (delta + 2)) - (- + - delta);
    printf("Stress A: mix=%d, base=%d, delta=%d\n", mix, base, delta);

    {
        int base = 5;
        int local = base + delta - (- + - base) + (base - delta);
        printf("Stress B: local=%d, base=%d, delta=%d\n", local, base, delta);

        if (local == 5) {
            int delta = 7;
            base = base + delta - (- + - delta);

            {
                int snap = base + delta + mix - (delta - base);
                printf("Stress C: snap=%d, base=%d, delta=%d, mix=%d\n", snap, base, delta, mix);
            }

            printf("Stress D: base=%d, local=%d, delta=%d\n", base, local, delta);
        }
    }

    printf("Stress E: base=%d, delta=%d\n", base, delta);

    int chain = 0;
    int i = 0;
    while (i < 4) {
        int step = i - 1;
        chain = chain + base - step - (- + - i);
        if (chain > 140) {
            break;
        }
        i = i + 1;
    }

    printf("Stress F: chain=%d, i=%d, base=%d\n", chain, i, base);
}

int test_loops_and_control() {
    int sum = 0;
    int i = 0;
    
    while (i < 10) {
        int step = i % 3;
        
        if (step == 0) {
            int sum = 100;
            sum = sum + i;
        } else if (step == 1) {
            sum = sum + i;
            i = i + 1;
            continue;
        } else {
            {
                int shadow = i * i;
                sum = sum + shadow - (- + - step);
                if (sum > 50) {
                    break;
                }
            }
        }
        i = i + 1;
    }
    
    return sum;
}

int main() {
    int in_x;
    int in_y;
    
    in_x = getint();
    in_y = getint();
    
    if (in_y == 0) {
        in_y = 1;
    }

    int res1 = test_complex_math(global_arr, in_x, in_y);
    printf("Math Result: %d\n", res1);
    
    test_deep_scopes();

    test_add_sub_scope_stress();
    
    int res2 = test_loops_and_control();
    printf("Loop Result: %d\n", res2);
    
    return 0;
}
)";

} // namespace

void cleanup();

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

	write_text("testfile.txt", complex_test_source);
	if (!run_parser_and_check_ir())
	{
		std::cerr << "[FAIL] complex test case: parser failed" << std::endl;
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		return 1;
	}

	if (!prepare_linked_runtime_ir())
	{
		std::cerr << "[FAIL] complex test case: linking failed" << std::endl;
		remove_if_exists("testfile.txt");
		remove_if_exists("output.ll");
		remove_if_exists("codegen_test.log");
		remove_if_exists("runtime_support.c");
		remove_if_exists("runtime_sylib.ll");
		remove_if_exists("linked_output.ll");
		return 1;
	}

	const std::string input = "7\n5\n";
	const std::string expected_output =
		"Math Result: 68\n"
		"Scope 0: val=1, shadow_var=10\n"
		"Scope 1: val=2, shadow_var=20, hidden=100\n"
		"Scope 2 (if): val=3, shadow_var=30, hidden=200\n"
		"Scope 3 (block): val=230\n"
		"Scope 2 (if) after: val=3\n"
		"Scope 1 after: val=3, shadow_var=20, hidden=100\n"
		"Scope 0 after: val=1, shadow_var=20\n"
		"Stress A: mix=69, base=40, delta=3\n"
		"Stress B: local=5, base=5, delta=3\n"
		"Stress C: snap=79, base=5, delta=7, mix=69\n"
		"Stress D: base=5, local=5, delta=7\n"
		"Stress E: base=40, delta=3\n"
		"Stress F: chain=152, i=3, base=40\n"
		"Loop Result: 99\n"
		"0\n";

	if (!run_lli_and_expect(input, expected_output))
	{
		std::cerr << "[FAIL] complex test case: output mismatch" << std::endl;
		cleanup();
		return 1;
	}

	remove_if_exists("testfile.txt");
	remove_if_exists("output.ll");
	remove_if_exists("codegen_test.log");
	remove_if_exists("lli_output.log");
	remove_if_exists("judge_input.txt");
	remove_if_exists("runtime_support.c");
	remove_if_exists("runtime_sylib.ll");
	remove_if_exists("linked_output.ll");
	remove_if_exists("runtime_build.log");
	remove_if_exists("runtime_link.log");

	std::cout << "[PASS] complex test case passed." << std::endl;
	return 0;
}

void cleanup()
{
	remove_if_exists("testfile.txt");
	remove_if_exists("output.ll");
	remove_if_exists("codegen_test.log");
	remove_if_exists("lli_output.log");
	remove_if_exists("judge_input.txt");
	remove_if_exists("runtime_support.c");
	remove_if_exists("runtime_sylib.ll");
	remove_if_exists("linked_output.ll");
	remove_if_exists("runtime_build.log");
	remove_if_exists("runtime_link.log");
}
