/**
 * @file test_codegen.cpp
 * @brief LLVM IR 代码生成集成测试
 *
 * 本文件包含对编译器代码生成功能的完整集成测试。
 * 测试流程：运行 parser → 检查 output.ll → 用 lli 执行 → 验证输出
 *
 * ===================== 测试策略 =====================
 *
 * 本测试采用端到端集成测试方法：
 * 1. 编译阶段: 运行 parser 生成 output.ll
 * 2. IR 验证: 检查 output.ll 包含必要符号
 * 3. 执行阶段: 使用 lli 解释执行生成的 IR
 * 4. 结果验证: 对比实际输出与预期输出
 *
 * ===================== 测试用例 =====================
 *
 * 测试覆盖以下 SysY 特性：
 * - 基本输入输出: hello_getint, putint_putch, getch_builtin
 * - 算术运算: const_arith, unary_precedence
 * - 条件分支: if_else, short_circuit (短路求值)
 * - 循环结构: while_sum, break_continue
 * - 函数调用: function_call, builtin_call_in_initializer
 * - 数组操作: array_access
 * - 逻辑运算: logical_ops
 *
 * ===================== 环境要求 =====================
 *
 * 测试依赖以下工具：
 * - parser.exe / ./parser: 编译器可执行文件
 * - clang: 用于生成 LLVM IR (测试环境检查是否存在)
 * - lli: LLVM IR 解释器 (执行生成的 IR)
 *
 * 如果 clang 或 lli 不存在，测试会自动跳过。
 */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
/**
 * @brief 执行命令并返回是否成功
 * @param command 要执行的命令
 * @return 命令返回 0 返回 true，否则返回 false
 */
bool run_command(const std::string &command)
{
	return std::system(command.c_str()) == 0;
}

/**
 * @brief 执行命令并捕获输出到文件
 * @param command 要执行的命令
 * @param output_file 接收命令输出的文件路径
 * @return 命令执行成功返回 true
 */
bool run_command_capture(const std::string &command, const std::string &output_file)
{
#ifdef _WIN32
	const std::string full = command + " > " + output_file + " 2>&1";
#else
	const std::string full = command + " > " + output_file + " 2>&1";
#endif
	return run_command(full);
}

/**
 * @brief 检查文件是否存在
 * @param path 文件路径
 * @return 文件存在且可读返回 true
 */
bool file_exists(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	return in.good();
}

/**
 * @brief 读取文件的全部内容
 * @param path 文件路径
 * @return 文件内容的字符串形式
 */
std::string read_all(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

/**
 * @brief 规范化输出文本
 *
 * 处理内容：
 * - 移除 \r 字符 (Windows 行尾)
 * - 移除末尾的空白字符 (空格、换行、制表符)
 *
 * @param text 原始文本
 * @return 规范化后的文本
 */
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

/**
 * @brief 如果文件存在则删除
 * @param path 文件路径
 */
void remove_if_exists(const std::string &path)
{
	std::remove(path.c_str());
}

/**
 * @brief 设置优化等级环境变量
 * @param level 优化等级 (0-3)
 * @return 设置成功返回 true
 */
bool set_opt_level_env(int level)
{
	const std::string value = "MYCOMPILER_OPT_LEVEL=" + std::to_string(level);
#ifdef _WIN32
	return _putenv(value.c_str()) == 0;
#else
	return setenv("MYCOMPILER_OPT_LEVEL", std::to_string(level).c_str(), 1) == 0;
#endif
}

/**
 * @brief 清除优化等级环境变量
 * @return 清除成功返回 true
 */
bool clear_opt_level_env()
{
#ifdef _WIN32
	return _putenv("MYCOMPILER_OPT_LEVEL=") == 0;
#else
	return unsetenv("MYCOMPILER_OPT_LEVEL") == 0;
#endif
}

/**
 * @brief 写入文本内容到文件
 * @param path 文件路径
 * @param content 要写入的文本内容
 */
void write_text(const std::string &path, const std::string &content)
{
	std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
	out << content;
}

/**
 * @brief 运行 parser 并验证生成的 IR
 *
 * 检查项：
 * 1. parser 执行成功 (返回码为 0)
 * 2. output.ll 文件被生成
 * 3. output.ll 包含 @main 和 @getint 符号
 *
 * @return 验证通过返回 true
 */
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

/**
 * @brief 使用 lli 执行 IR 并验证输出
 *
 * 通过管道传递输入到 lli：
 * - Windows: echo input | lli output.ll
 * - Unix: printf "input\n" | lli output.ll
 *
 * @param input 标准输入内容 (可为空字符串)
 * @param expected_output 期望的输出内容
 * @return 验证通过返回 true
 */
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

/**
 * @brief 使用文件重定向方式运行 lli 并验证
 *
 * 模拟评测机环境：
 * 1. 输入写入 judge_input.txt
 * 2. lli < judge_input.txt > judge_stdout.log
 * 3. 捕获进程退出码
 *
 * @param input 标准输入内容
 * @param expected_output 期望的输出内容
 * @param expected_exit_code 期望的进程退出码
 * @return 验证通过返回 true
 */
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

/**
 * @brief 运行单个测试用例
 *
 * 完整流程：
 * 1. 写入测试源码到 testfile.txt
 * 2. 运行 parser 生成 output.ll
 * 3. 用 lli 执行并捕获输出
 * 4. 对比实际输出与期望输出
 *
 * @param name 测试用例名称
 * @param source SysY 源代码
 * @param input 标准输入内容
 * @param expected_output 期望的输出内容
 * @return 测试通过返回 true
 */
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

/**
 * @brief 主测试函数
 *
 * 测试执行流程：
 * 1. 检查 clang 和 lli 是否存在
 * 2. 定义所有测试用例
 * 3. 逐个运行测试用例
 * 4. 测试优化等级配置
 * 5. 测试文件重定向方式
 * 6. 清理临时文件
 */
int main()
{
	/** 检查测试环境是否具备 */
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

	/** 定义测试用例结构 */
	struct TestCase
	{
		std::string name;      /**< 测试名称 */
		std::string source;    /**< SysY 源代码 */
		std::string input;     /**< 标准输入 */
		std::string expected;  /**< 期望输出 */
	};

	/** 初始化测试用例列表 */
	std::vector<TestCase> cases;

	/** 用例 1: 基本输入输出测试 */
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

	/** 用例 2: 短路求值测试 (关键测试) */
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

	/** 用例 3: 常量算术运算 */
	cases.push_back(TestCase{"const_arith",
		"int main(){\n"
		"    const int a = 5;\n"
		"    const int b = 7;\n"
		"    printf(\"%d\\n\", a * b + 3);\n"
		"    return 0;\n"
		"}\n",
		"",
		"38\n"});

	/** 用例 4: if-else 条件分支 */
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

	/** 用例 5: while 循环求和 */
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

	/** 用例 6: break 和 continue */
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

	/** 用例 7: 函数调用 */
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

	/** 用例 8: 数组访问 */
	cases.push_back(TestCase{"array_access",
		"int main(){\n"
		"    int a[3] = {2,4,6};\n"
		"    printf(\"%d\\n\", a[0] + a[2]);\n"
		"    return 0;\n"
		"}\n",
		"",
		"8\n"});

	/** 用例 9: 逻辑运算 */
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

	/** 用例 10: 一元运算符优先级 */
	cases.push_back(TestCase{"unary_precedence",
		"int main(){\n"
		"    int x = -3 + +5 * 2;\n"
		"    printf(\"%d\\n\", x);\n"
		"    return 0;\n"
		"}\n",
		"",
		"7\n"});

	/** 用例 11: putint 和 putch */
	cases.push_back(TestCase{"putint_putch",
		"int main(){\n"
		"    putint(123);\n"
		"    putch(10);\n"
		"    return 0;\n"
		"}\n",
		"",
		"123\n"});

	/** 用例 12: getch 内置函数 */
	cases.push_back(TestCase{"getch_builtin",
		"int main(){\n"
		"    int c;\n"
		"    c = getch();\n"
		"    putint(c);\n"
		"    return 0;\n"
		"}\n",
		"A",
		"65"});

	/** 用例 13: 内置函数在初始化器中调用 */
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

	/** 运行所有测试用例 */
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

	/** 测试优化等级环境变量 */
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

	/** 测试文件重定向方式 (模拟评测机环境) */
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

	/** 清理所有临时文件 */
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
