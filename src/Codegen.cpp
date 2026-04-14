/**
 * @file Codegen.cpp
 * @brief LLVM IR 代码生成器实现
 *
 * 本文件是编译器的核心代码生成模块，
 * 负责将 SysY 源代码转换为 LLVM IR 中间代码。
 *
 * ===================== 重要设计说明 =====================
 *
 * 当前实现采用 clang 辅助模式：
 * - 读取源文件 → 解析为 AST → 语义分析
 * - 生成临时 C 代码 → 调用 clang 发射 LLVM IR
 * - 清理和规范化生成的 IR
 *
 * ===================== 环境变量配置 =====================
 *
 * - LLVM_CLANG: 指定 clang 可执行文件路径
 * - MYCOMPILER_OPT_LEVEL: 优化等级 (0-3)
 * - MYCOMPILER_KEEP_TEMP: 设为 1 则保留临时文件
 */

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
/**
 * @brief 执行系统命令
 * @param command 要执行的命令字符串
 * @return 命令的退出码
 */
int run_command(const std::string &command)
{
	return std::system(command.c_str());
}

/**
 * @brief 检查命令是否存在
 * @param exe 可执行文件名或完整路径
 * @return 存在返回 true，否则返回 false
 */
bool command_exists(const std::string &exe)
{
#ifdef _WIN32
	const std::string cmd = exe + " --version >NUL 2>&1";
#else
	const std::string cmd = exe + " --version >/dev/null 2>&1";
#endif
	return run_command(cmd) == 0;
}

/**
 * @brief 拼接目录和文件名
 * @param dir 目录路径
 * @param name 文件名
 * @return 拼接后的完整路径
 */
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

/**
 * @brief 在 lli 所在目录查找同版本的 clang
 *
 * 查找策略：
 * - 遍历 PATH 环境变量中的每个目录
 * - 检查是否存在 lli.exe 和 clang.exe
 * - 返回找到的 clang.exe 路径
 *
 * @return 找到的 clang 路径，未找到则返回空字符串
 */
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

/**
 * @brief 查找可用的 clang 可执行文件
 *
 * 查找优先级：
 * 1. LLVM_CLANG 环境变量指定的值
 * 2. lli 同目录的 clang
 * 3. PATH 中的 clang, clang-18, clang-17, clang-16
 *
 * @return 找到的 clang 路径，未找到则返回空字符串
 */
std::string find_clang_executable()
{
	/** 优先级 1: 环境变量指定 */
	const char *from_env = std::getenv("LLVM_CLANG");
	if (from_env != nullptr && from_env[0] != '\0' && command_exists(from_env))
	{
		return std::string(from_env);
	}

	/** 优先级 2: lli 同目录 */
	const std::string sibling = find_lli_sibling_clang();
	if (!sibling.empty())
	{
		return sibling;
	}

	/** 优先级 3: PATH 中的常见 clang 版本 */
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

/**
 * @brief 生成 SysY 运行时库函数实现
 *
 * 输出 C 代码形式的运行时库：
 * - getint/getch: 从标准输入读取
 * - putint/putch: 输出到标准输出
 * - getarray/putarray: 数组输入输出
 * - starttime/stoptime: 性能计时 (空实现)
 */
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

/**
 * @brief 去除字符串首尾空白字符
 * @param text 输入字符串
 * @return 去除空白后的字符串
 */
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

/**
 * @brief 判断一行是否为内置函数声明
 *
 * SysY 内置函数包括：
 * getint, getch, getarray, putint, putch, putarray, putf,
 * starttime, stoptime, _sysy_starttime, _sysy_stoptime
 *
 * @param line 源代码行
 * @return 是内置函数声明返回 true
 */
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

/**
 * @brief 从源代码中移除内置函数声明
 *
 * 因为内置函数会在 driver_prelude 中提供实现，
 * 所以源文件中的声明需要移除以避免重复定义。
 *
 * @param source 原始源代码
 * @return 移除内置函数声明后的源代码
 */
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

/**
 * @brief 移除 LLVM IR 中的主机特定行
 *
 * 移除的内容：
 * - target datalayout: 目标数据布局 (包含主机特定信息)
 * - target triple: 目标三元组 (包含主机特定信息)
 *
 * 这些信息会导致生成的 IR 不可移植，
 * 移除后 IR 可以在不同的 LLVM 环境下运行。
 *
 * @param ir 原始 LLVM IR 文本
 * @return 清理后的 LLVM IR 文本
 */
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

/**
 * @brief 从环境变量读取代码生成配置
 *
 * 支持的环境变量：
 * - MYCOMPILER_OPT_LEVEL: 优化等级 (0-3)
 * - MYCOMPILER_KEEP_TEMP: 设为 1 则保留临时文件
 *
 * @return 解析后的配置结构体
 */
CodegenOptions LLVMIRGenerator::from_environment()
{
	CodegenOptions options;

	/** 读取优化等级 */
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

	/** 读取临时文件保留标志 */
	const char *keep_temp_env = std::getenv("MYCOMPILER_KEEP_TEMP");
	if (keep_temp_env != nullptr && keep_temp_env[0] == '1')
	{
		options.keep_temporary_file = true;
	}

	return options;
}

/**
 * @brief 将源文件编译为 LLVM IR
 *
 * 完整的编译流程：
 * 1. 读取源文件
 * 2. 预处理: 移除内置函数声明
 * 3. 语法分析: 解析为 AST
 * 4. 语义分析: 检查程序结构
 * 5. 生成临时 C 代码
 * 6. 调用 clang 发射 LLVM IR
 * 7. 清理和规范化 IR
 * 8. 输出到目标文件
 *
 * @param input_path 输入源文件路径 (testfile.txt)
 * @param output_path 输出 IR 文件路径 (output.ll)
 * @param options 代码生成选项
 * @param error_message 错误信息输出参数
 * @return 成功返回 true，失败返回 false
 */
bool LLVMIRGenerator::generate_from_file(const std::string &input_path,
					 const std::string &output_path,
					 const CodegenOptions &options,
					 std::string *error_message) const
{
	/** 步骤 1: 打开源文件 */
	std::ifstream input(input_path.c_str(), std::ios::binary);
	if (!input.is_open())
	{
		if (error_message != nullptr)
		{
			*error_message = "Failed to open source file: " + input_path;
		}
		return false;
	}

	/** 步骤 2: 读取并预处理源文件 */
	std::ostringstream source_buffer;
	source_buffer << input.rdbuf();
	const std::string stripped_source = strip_builtin_declarations(source_buffer.str());

	/** 步骤 3: 创建临时文件用于解析 */
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

	/** 步骤 4: 语法分析 - 解析为 AST */
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

	/** 检查 AST 是否为空 */
	if (!ast_root || ast_root->items.empty())
	{
		if (error_message != nullptr)
		{
			*error_message = "Parser produced an empty AST.";
		}
		return false;
	}

	/** 步骤 5: 语义分析 */
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

	/** 步骤 6: 生成临时 C 源代码文件 */
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

	/** 步骤 7: 查找 clang 可执行文件 */
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

	/** 步骤 8: 调用 clang 编译为 LLVM IR */
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

	/** 步骤 9: 读取生成的 IR */
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

	/** 步骤 10: 规范化 IR - 移除主机特定信息 */
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
