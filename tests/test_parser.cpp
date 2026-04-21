/**
 * @file test_parser.cpp
 * @brief Parser 单元测试 - 详细版
 *
 * 测试覆盖：
 * 1. 基本程序结构
 * 2. 常量声明
 * 3. 变量声明
 * 4. 函数定义（普通函数和 main 函数）
 * 5. if/else 语句
 * 6. while 循环
 * 7. break/continue 语句
 * 8. return 语句
 * 9. 算术表达式
 * 10. 关系运算和逻辑运算
 * 11. 数组支持
 * 12. 嵌套语句块
 * 13. 函数调用
 * 14. printf/getint 等内置函数
 * 15. 边界情况
 */

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "../include/Ast.h"
#include "../include/Lexer.h"
#include "../include/ParserFrontend.h"

// 声明由 Bison 生成的函数和变量
extern int yyparse();
extern void set_lexer(Lexer *lexer);
extern std::ofstream output_file;

// ============================================================================
// 辅助函数
// ============================================================================

// 创建临时测试文件
void create_test_file(const std::string &filename, const std::string &content)
{
	std::ofstream file(filename);
	file << content;
	file.close();
}

// 删除临时文件
void delete_test_file(const std::string &filename)
{
	std::remove(filename.c_str());
}

void require_true(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "[FAIL] " << message << std::endl;
		std::abort();
	}
}

// 执行 parser 测试的辅助宏
#define PARSE_TEST(filename, content)    \
	create_test_file(filename, content); \
	Lexer lexer(filename);               \
	set_lexer(&lexer);                   \
	require_true(yyparse() == 0, "yyparse failed: " filename); \
	require_true(ast::peek_ast_root() != nullptr, "peek_ast_root is null: " filename); \
	std::unique_ptr<ast::CompUnit> parsed_ast = ast::take_ast_root(); \
	require_true(parsed_ast.get() != nullptr, "take_ast_root returned null: " filename); \
	require_true(!parsed_ast->items.empty(), "parsed ast items empty: " filename); \
	delete_test_file(filename)

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试1: 最基本的 main 函数
 */
void test_parser_basic_main()
{
	std::string content = "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_01.txt", content);
	std::cout << "[PASS] test_parser_basic_main" << std::endl;
}

/**
 * @brief 测试2: 带变量声明的程序
 */
void test_parser_var_decl()
{
	std::string content = "int a;\n"
						  "int b = 1;\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_02.txt", content);
	std::cout << "[PASS] test_parser_var_decl" << std::endl;
}

/**
 * @brief 测试3: 常量声明
 */
void test_parser_const_decl()
{
	std::string content = "const int a = 10;\n"
						  "const int b = 20;\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_03.txt", content);
	std::cout << "[PASS] test_parser_const_decl" << std::endl;
}

/**
 * @brief 测试4: 多变量声明（逗号分隔）
 */
void test_parser_multi_var_decl()
{
	std::string content = "int a, b, c;\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_04.txt", content);
	std::cout << "[PASS] test_parser_multi_var_decl" << std::endl;
}

/**
 * @brief 测试5: 带初始化的多变量声明
 */
void test_parser_multi_var_init()
{
	std::string content = "int a = 1, b = 2, c = 3;\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_05.txt", content);
	std::cout << "[PASS] test_parser_multi_var_init" << std::endl;
}

/**
 * @brief 测试6: 赋值语句
 */
void test_parser_assignment()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 10;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_06.txt", content);
	std::cout << "[PASS] test_parser_assignment" << std::endl;
}

/**
 * @brief 测试7: 算术表达式 - 加法
 */
void test_parser_expr_add()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 1 + 2;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_07.txt", content);
	std::cout << "[PASS] test_parser_expr_add" << std::endl;
}

/**
 * @brief 测试8: 算术表达式 - 减法
 */
void test_parser_expr_sub()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 5 - 3;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_08.txt", content);
	std::cout << "[PASS] test_parser_expr_sub" << std::endl;
}

/**
 * @brief 测试9: 算术表达式 - 乘法
 */
void test_parser_expr_mul()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 3 * 4;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_09.txt", content);
	std::cout << "[PASS] test_parser_expr_mul" << std::endl;
}

/**
 * @brief 测试10: 算术表达式 - 除法
 */
void test_parser_expr_div()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 10 / 2;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_10.txt", content);
	std::cout << "[PASS] test_parser_expr_div" << std::endl;
}

/**
 * @brief 测试11: 算术表达式 - 取模
 */
void test_parser_expr_mod()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 10 % 3;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_11.txt", content);
	std::cout << "[PASS] test_parser_expr_mod" << std::endl;
}

/**
 * @brief 测试12: 复杂算术表达式
 */
void test_parser_expr_complex()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = (1 + 2) * 3 - 4 / 2;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_12.txt", content);
	std::cout << "[PASS] test_parser_expr_complex" << std::endl;
}

/**
 * @brief 测试13: 一元正号
 */
void test_parser_unary_plus()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = +5;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_13.txt", content);
	std::cout << "[PASS] test_parser_unary_plus" << std::endl;
}

/**
 * @brief 测试14: 一元负号
 */
void test_parser_unary_minus()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = -5;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_14.txt", content);
	std::cout << "[PASS] test_parser_unary_minus" << std::endl;
}

/**
 * @brief 测试15: 关系运算 - 小于
 */
void test_parser_rel_lss()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 < 2) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_15.txt", content);
	std::cout << "[PASS] test_parser_rel_lss" << std::endl;
}

/**
 * @brief 测试16: 关系运算 - 大于
 */
void test_parser_rel_gre()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (3 > 2) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_16.txt", content);
	std::cout << "[PASS] test_parser_rel_gre" << std::endl;
}

/**
 * @brief 测试17: 关系运算 - 小于等于
 */
void test_parser_rel_leq()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (2 <= 2) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_17.txt", content);
	std::cout << "[PASS] test_parser_rel_leq" << std::endl;
}

/**
 * @brief 测试18: 关系运算 - 大于等于
 */
void test_parser_rel_geq()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (2 >= 2) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_18.txt", content);
	std::cout << "[PASS] test_parser_rel_geq" << std::endl;
}

/**
 * @brief 测试19: 关系运算 - 等于
 */
void test_parser_rel_eql()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 == 1) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_19.txt", content);
	std::cout << "[PASS] test_parser_rel_eql" << std::endl;
}

/**
 * @brief 测试20: 关系运算 - 不等于
 */
void test_parser_rel_neq()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 != 2) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_20.txt", content);
	std::cout << "[PASS] test_parser_rel_neq" << std::endl;
}

/**
 * @brief 测试21: 逻辑运算 - 逻辑与
 */
void test_parser_logic_and()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 && 1) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_21.txt", content);
	std::cout << "[PASS] test_parser_logic_and" << std::endl;
}

/**
 * @brief 测试22: 逻辑运算 - 逻辑或
 */
void test_parser_logic_or()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 || 0) a = 1;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_22.txt", content);
	std::cout << "[PASS] test_parser_logic_or" << std::endl;
}

/**
 * @brief 测试23: 逻辑非
 */
void test_parser_logic_not()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = !0;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_23.txt", content);
	std::cout << "[PASS] test_parser_logic_not" << std::endl;
}

/**
 * @brief 测试24: if 语句（无 else）
 */
void test_parser_if()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 < 2) {\n"
						  "        a = 1;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_24.txt", content);
	std::cout << "[PASS] test_parser_if" << std::endl;
}

/**
 * @brief 测试25: if-else 语句
 */
void test_parser_if_else()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 > 2) {\n"
						  "        a = 1;\n"
						  "    } else {\n"
						  "        a = 0;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_25.txt", content);
	std::cout << "[PASS] test_parser_if_else" << std::endl;
}

/**
 * @brief 测试26: 嵌套 if 语句
 */
void test_parser_nested_if()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    if (1 < 2) {\n"
						  "        if (2 < 3) {\n"
						  "            a = 1;\n"
						  "        }\n"
						  "    } else {\n"
						  "        a = 0;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_26.txt", content);
	std::cout << "[PASS] test_parser_nested_if" << std::endl;
}

/**
 * @brief 测试27: while 循环
 */
void test_parser_while()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 0;\n"
						  "    while (a < 10) {\n"
						  "        a = a + 1;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_27.txt", content);
	std::cout << "[PASS] test_parser_while" << std::endl;
}

/**
 * @brief 测试28: break 语句
 */
void test_parser_break()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 0;\n"
						  "    while (a < 10) {\n"
						  "        if (a == 5) {\n"
						  "            break;\n"
						  "        }\n"
						  "        a = a + 1;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_28.txt", content);
	std::cout << "[PASS] test_parser_break" << std::endl;
}

/**
 * @brief 测试29: continue 语句
 */
void test_parser_continue()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 0;\n"
						  "    while (a < 10) {\n"
						  "        a = a + 1;\n"
						  "        if (a < 5) {\n"
						  "            continue;\n"
						  "        }\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_29.txt", content);
	std::cout << "[PASS] test_parser_continue" << std::endl;
}

/**
 * @brief 测试30: return 语句（无表达式）
 */
void test_parser_return_void()
{
	std::string content = "void foo() {\n"
						  "    return;\n"
						  "}\n"
						  "int main() {\n"
						  "    foo();\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_30.txt", content);
	std::cout << "[PASS] test_parser_return_void" << std::endl;
}

/**
 * @brief 测试31: return 语句（带表达式）
 */
void test_parser_return_expr()
{
	std::string content = "int foo() {\n"
						  "    return 42;\n"
						  "}\n"
						  "int main() {\n"
						  "    return foo();\n"
						  "}";
	PARSE_TEST("test_parser_31.txt", content);
	std::cout << "[PASS] test_parser_return_expr" << std::endl;
}

/**
 * @brief 测试32: 空语句
 */
void test_parser_empty_stmt()
{
	std::string content = "int main() {\n"
						  "    ;\n"
						  "    ;\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_32.txt", content);
	std::cout << "[PASS] test_parser_empty_stmt" << std::endl;
}

/**
 * @brief 测试33: 表达式语句
 */
void test_parser_expr_stmt()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 1;\n"
						  "    a + 2;\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_33.txt", content);
	std::cout << "[PASS] test_parser_expr_stmt" << std::endl;
}

/**
 * @brief 测试34: 嵌套语句块
 */
void test_parser_nested_block()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 1;\n"
						  "    {\n"
						  "        int b = 2;\n"
						  "        {\n"
						  "            int c = 3;\n"
						  "            a = b + c;\n"
						  "        }\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_34.txt", content);
	std::cout << "[PASS] test_parser_nested_block" << std::endl;
}

/**
 * @brief 测试35: 普通函数定义（无参数）
 */
void test_parser_func_no_params()
{
	std::string content = "int foo() {\n"
						  "    return 0;\n"
						  "}\n"
						  "int main() {\n"
						  "    return foo();\n"
						  "}";
	PARSE_TEST("test_parser_35.txt", content);
	std::cout << "[PASS] test_parser_func_no_params" << std::endl;
}

/**
 * @brief 测试36: 普通函数定义（带参数）
 */
void test_parser_func_with_params()
{
	std::string content = "int add(int a, int b) {\n"
						  "    return a + b;\n"
						  "}\n"
						  "int main() {\n"
						  "    return add(1, 2);\n"
						  "}";
	PARSE_TEST("test_parser_36.txt", content);
	std::cout << "[PASS] test_parser_func_with_params" << std::endl;
}

/**
 * @brief 测试37: void 类型函数
 */
void test_parser_void_func()
{
	std::string content = "void print_hello() {\n"
						  "    return;\n"
						  "}\n"
						  "int main() {\n"
						  "    print_hello();\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_37.txt", content);
	std::cout << "[PASS] test_parser_void_func" << std::endl;
}

/**
 * @brief 测试38: printf 语句
 */
void test_parser_printf()
{
	std::string content = "int main() {\n"
						  "    printf(\"Hello\\n\");\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_38.txt", content);
	std::cout << "[PASS] test_parser_printf" << std::endl;
}

/**
 * @brief 测试39: printf 语句（带参数）
 */
void test_parser_printf_with_args()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 42;\n"
						  "    printf(\"%d\\n\", a);\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_39.txt", content);
	std::cout << "[PASS] test_parser_printf_with_args" << std::endl;
}

/**
 * @brief 测试40: getint 语句
 */
void test_parser_getint()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = getint();\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_40.txt", content);
	std::cout << "[PASS] test_parser_getint" << std::endl;
}

/**
 * @brief 测试41: 一维数组声明
 */
void test_parser_array_1d()
{
	std::string content = "int a[10];\n"
						  "int main() {\n"
						  "    a[0] = 1;\n"
						  "    return a[0];\n"
						  "}";
	PARSE_TEST("test_parser_41.txt", content);
	std::cout << "[PASS] test_parser_array_1d" << std::endl;
}

/**
 * @brief 测试42: 二维数组声明
 */
void test_parser_array_2d()
{
	std::string content = "int a[5][10];\n"
						  "int main() {\n"
						  "    a[0][0] = 1;\n"
						  "    return a[0][0];\n"
						  "}";
	PARSE_TEST("test_parser_42.txt", content);
	std::cout << "[PASS] test_parser_array_2d" << std::endl;
}

/**
 * @brief 测试43: 数组初始化
 */
void test_parser_array_init()
{
	std::string content = "int a[3] = {1, 2, 3};\n"
						  "int main() {\n"
						  "    return a[0];\n"
						  "}";
	PARSE_TEST("test_parser_43.txt", content);
	std::cout << "[PASS] test_parser_array_init" << std::endl;
}

/**
 * @brief 测试44: 空数组初始化
 */
void test_parser_array_init_empty()
{
	std::string content = "int a[3] = {};\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_44.txt", content);
	std::cout << "[PASS] test_parser_array_init_empty" << std::endl;
}

/**
 * @brief 测试45: 常量数组初始化
 */
void test_parser_const_array()
{
	std::string content = "const int a[3] = {1, 2, 3};\n"
						  "int main() {\n"
						  "    return a[0];\n"
						  "}";
	PARSE_TEST("test_parser_45.txt", content);
	std::cout << "[PASS] test_parser_const_array" << std::endl;
}

/**
 * @brief 测试46: 函数参数为数组
 */
void test_parser_func_array_param()
{
	std::string content = "int sum(int a[]) {\n"
						  "    return 0;\n"
						  "}\n"
						  "int main() {\n"
						  "    return sum(0);\n"
						  "}";
	PARSE_TEST("test_parser_46.txt", content);
	std::cout << "[PASS] test_parser_func_array_param" << std::endl;
}

/**
 * @brief 测试47: 变量作为数组索引
 */
void test_parser_array_var_index()
{
	std::string content = "int a[10];\n"
						  "int i;\n"
						  "int main() {\n"
						  "    i = 0;\n"
						  "    a[i] = 5;\n"
						  "    return a[i];\n"
						  "}";
	PARSE_TEST("test_parser_47.txt", content);
	std::cout << "[PASS] test_parser_array_var_index" << std::endl;
}

/**
 * @brief 测试48: 复杂条件表达式
 */
void test_parser_complex_cond()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 0;\n"
						  "    if (a >= 0 && a <= 10 && a != 5) {\n"
						  "        a = 1;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_48.txt", content);
	std::cout << "[PASS] test_parser_complex_cond" << std::endl;
}

/**
 * @brief 测试49: 函数调用表达式作为参数
 */
void test_parser_func_call_as_arg()
{
	std::string content = "int get_val() {\n"
						  "    return 10;\n"
						  "}\n"
						  "int add(int a, int b) {\n"
						  "    return a + b;\n"
						  "}\n"
						  "int main() {\n"
						  "    return add(get_val(), 5);\n"
						  "}";
	PARSE_TEST("test_parser_49.txt", content);
	std::cout << "[PASS] test_parser_func_call_as_arg" << std::endl;
}

/**
 * @brief 测试50: 多层嵌套函数调用
 */
void test_parser_nested_func_calls()
{
	std::string content = "int get_val() {\n"
						  "    return 1;\n"
						  "}\n"
						  "int add(int a, int b) {\n"
						  "    return a + b;\n"
						  "}\n"
						  "int main() {\n"
						  "    return add(add(get_val(), 2), 3);\n"
						  "}";
	PARSE_TEST("test_parser_50.txt", content);
	std::cout << "[PASS] test_parser_nested_func_calls" << std::endl;
}

/**
 * @brief 测试51: 多个全局变量
 */
void test_parser_multiple_globals()
{
	std::string content = "int a = 1;\n"
						  "int b = 2;\n"
						  "int c = 3;\n"
						  "const int d = 4;\n"
						  "int main() {\n"
						  "    return a + b + c + d;\n"
						  "}";
	PARSE_TEST("test_parser_51.txt", content);
	std::cout << "[PASS] test_parser_multiple_globals" << std::endl;
}

/**
 * @brief 测试52: 多个函数定义
 */
void test_parser_multiple_funcs()
{
	std::string content = "int foo() {\n"
						  "    return 1;\n"
						  "}\n"
						  "int bar() {\n"
						  "    return 2;\n"
						  "}\n"
						  "int main() {\n"
						  "    return foo() + bar();\n"
						  "}";
	PARSE_TEST("test_parser_52.txt", content);
	std::cout << "[PASS] test_parser_multiple_funcs" << std::endl;
}

/**
 * @brief 测试53: 表达式中的函数调用
 */
void test_parser_expr_with_func_call()
{
	std::string content = "int get_one() {\n"
						  "    return 1;\n"
						  "}\n"
						  "int main() {\n"
						  "    return get_one() * 2 + 3;\n"
						  "}";
	PARSE_TEST("test_parser_53.txt", content);
	std::cout << "[PASS] test_parser_expr_with_func_call" << std::endl;
}

/**
 * @brief 测试54: 括号表达式
 */
void test_parser_paren_expr()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = (1 + 2) * 3;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_54.txt", content);
	std::cout << "[PASS] test_parser_paren_expr" << std::endl;
}

/**
 * @brief 测试55: 赋值表达式作为子表达式
 */
void test_parser_assign_in_expr()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 5;\n"
						  "    a = a + 3;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_55.txt", content);
	std::cout << "[PASS] test_parser_assign_in_expr" << std::endl;
}

/**
 * @brief 测试56: 变量先声明后使用
 */
void test_parser_decl_before_use()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 1;\n"
						  "    {\n"
						  "        int b = a + 1;\n"
						  "        a = b;\n"
						  "    }\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_56.txt", content);
	std::cout << "[PASS] test_parser_decl_before_use" << std::endl;
}

/**
 * @brief 测试57: 完整的复杂程序
 */
void test_parser_full_program()
{
	std::string content = R"(
const int ARRAY_SIZE = 10;
int global_var = 100;

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int sum_array(int arr[], int size) {
    int sum = 0;
    int i = 0;
    while (i < size) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}

int main() {
    int a[ARRAY_SIZE];
    int i = 0;
    int result;
    
    while (i < ARRAY_SIZE) {
        a[i] = i;
        i = i + 1;
    }
    
    result = sum_array(a, ARRAY_SIZE);
    
    if (result > 0 && global_var > 0) {
        result = result + fibonacci(10);
    } else {
        result = 0;
    }
    
    printf("Result: %d\n", result);
    
    return result;
}
)";
	PARSE_TEST("test_parser_57.txt", content);
	std::cout << "[PASS] test_parser_full_program" << std::endl;
}

/**
 * @brief 测试58: 运算符优先级
 */
void test_parser_operator_precedence()
{
	std::string content = "int a;\n"
						  "int main() {\n"
						  "    a = 1 + 2 * 3 - 4 / 2;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_58.txt", content);
	std::cout << "[PASS] test_parser_operator_precedence" << std::endl;
}

/**
 * @brief 测试59: 赋值运算符优先级
 */
void test_parser_assign_precedence()
{
	std::string content = "int a, b, c;\n"
						  "int main() {\n"
						  "    c = 10;\n"
						  "    b = c;\n"
						  "    a = b;\n"
						  "    return a;\n"
						  "}";
	PARSE_TEST("test_parser_59.txt", content);
	std::cout << "[PASS] test_parser_assign_precedence" << std::endl;
}

/**
 * @brief 测试60: 负数常量
 */
void test_parser_negative_const()
{
	std::string content = "const int NEG = -100;\n"
						  "int main() {\n"
						  "    return NEG;\n"
						  "}";
	PARSE_TEST("test_parser_60.txt", content);
	std::cout << "[PASS] test_parser_negative_const" << std::endl;
}

/**
 * @brief 测试61: 验证输出格式
 */
void test_parser_output_format()
{
	// 测试 1: 普通表达式，不应有 RelExp 等标签
	std::string content1 = "int main() { return 1 + 2; }";
	create_test_file("test_parser_output1.txt", content1);
	Lexer lexer1("test_parser_output1.txt");
	set_lexer(&lexer1);
	output_file.open("output1.txt");
	require_true(yyparse() == 0, "yyparse failed for test_parser_output1.txt");
	output_file.close();

	// 验证 output1.txt 中是否包含 <RelExp>
	std::ifstream if1("output1.txt");
	std::string line;
	bool has_relexp = false;
	while (std::getline(if1, line))
	{
		if (line.find("<RelExp>") != std::string::npos)
			has_relexp = true;
	}
	if1.close();
	require_true(!has_relexp, "unexpected <RelExp> found in arithmetic expression output");
	std::cout << "[PASS] test_parser_output_format_arithmetic (No RelExp in Exp)" << std::endl;

	// 测试 2: 条件表达式，应包含 RelExp 等标签
	std::string content2 = "int main() { if (1 < 2) return 0; }";
	create_test_file("test_parser_output2.txt", content2);
	Lexer lexer2("test_parser_output2.txt");
	set_lexer(&lexer2);
	output_file.open("output2.txt");
	require_true(yyparse() == 0, "yyparse failed for test_parser_output2.txt");
	output_file.close();

	// 验证 output2.txt 中是否包含 <RelExp>
	std::ifstream if2("output2.txt");
	has_relexp = false;
	while (std::getline(if2, line))
	{
		if (line.find("<RelExp>") != std::string::npos)
			has_relexp = true;
	}
	if2.close();
	require_true(has_relexp, "expected <RelExp> not found in condition output");
	std::cout << "[PASS] test_parser_output_format_condition (Has RelExp in Cond)" << std::endl;

	delete_test_file("test_parser_output1.txt");
	delete_test_file("output1.txt");
	delete_test_file("test_parser_output2.txt");
	delete_test_file("output2.txt");

	// 测试 3: getint() 应包含 UnaryExp, MulExp, AddExp, Exp 标签
	std::string content3 = "int main() { int a; a = getint(); }";
	create_test_file("test_parser_output3.txt", content3);
	Lexer lexer3("test_parser_output3.txt");
	set_lexer(&lexer3);
	output_file.open("output3.txt");
	require_true(yyparse() == 0, "yyparse failed for test_parser_output3.txt");
	output_file.close();

	std::ifstream if3("output3.txt");
	bool has_exp = false;
	while (std::getline(if3, line))
	{
		if (line.find("<Exp>") != std::string::npos)
			has_exp = true;
	}
	if3.close();
	require_true(has_exp, "expected <Exp> not found in getint assignment output");
	std::cout << "[PASS] test_parser_output_format_getint (Has Exp in getint assignment)" << std::endl;
	delete_test_file("test_parser_output3.txt");
	delete_test_file("output3.txt");

	// 测试 4: FuncFParam 维度应包含 Exp 标签
	std::string content4 = "void foo(int a[][10]) {} int main() { return 0; }";
	create_test_file("test_parser_output4.txt", content4);
	Lexer lexer4("test_parser_output4.txt");
	set_lexer(&lexer4);
	output_file.open("output4.txt");
	require_true(yyparse() == 0, "yyparse failed for test_parser_output4.txt");
	output_file.close();

	std::ifstream if4("output4.txt");
	bool has_exp_in_param = false;
	while (std::getline(if4, line))
	{
		if (line.find("<Exp>") != std::string::npos)
			has_exp_in_param = true;
	}
	if4.close();
	require_true(has_exp_in_param, "expected <Exp> not found in function parameter dimension output");
	std::cout << "[PASS] test_parser_output_format_func_param_dim (Has Exp in FuncFParam dim)" << std::endl;
	delete_test_file("test_parser_output4.txt");
	delete_test_file("output4.txt");
}

void test_parser_frontend_api()
{
	const std::string filename = "test_parser_frontend.txt";
	const std::string content = "int g;\n"
					  "int main() {\n"
					  "    return 0;\n"
					  "}";
	create_test_file(filename, content);

	std::unique_ptr<ast::CompUnit> root;
	std::string error;
	require_true(parse_file_to_ast(filename, &root, &error), "parse_file_to_ast returned false");
	assert(error.empty());
	assert(root.get() != nullptr);
	assert(root->items.size() == 2);

	delete_test_file(filename);
	std::cout << "[PASS] test_parser_frontend_api" << std::endl;
}

void test_parser_float_decl_and_return()
{
	std::string content = "float addf(float a, float b) {\n"
						  "    float c = a + b;\n"
						  "    return c;\n"
						  "}\n"
						  "int main() {\n"
						  "    float x = 1.5;\n"
						  "    float y = 2.25;\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_float_01.txt", content);
	std::cout << "[PASS] test_parser_float_decl_and_return" << std::endl;
}

void test_parser_float_const_init()
{
	std::string content = "const float pi = 3.14159;\n"
						  "float g = 9.8;\n"
						  "int main() {\n"
						  "    return 0;\n"
						  "}";
	PARSE_TEST("test_parser_float_02.txt", content);
	std::cout << "[PASS] test_parser_float_const_init" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main()
{
	try
	{
		std::cout << "Running detailed Parser tests..." << std::endl;
		std::cout << "================================" << std::endl;
		std::cout << "test_parser_output_format" << std::endl;
		test_parser_output_format();
		test_parser_frontend_api();
		test_parser_float_decl_and_return();
		test_parser_float_const_init();

		std::cout << "basic program structure" << std::endl;

		// 基本程序结构测试
		test_parser_basic_main();
		test_parser_var_decl();
		test_parser_const_decl();
		test_parser_multi_var_decl();
		test_parser_multi_var_init();

		// 赋值语句测试
		test_parser_assignment();

		// 算术表达式测试
		test_parser_expr_add();
		test_parser_expr_sub();
		test_parser_expr_mul();
		test_parser_expr_div();
		test_parser_expr_mod();
		test_parser_expr_complex();
		test_parser_unary_plus();
		test_parser_unary_minus();
		test_parser_paren_expr();

		// 关系运算测试
		test_parser_rel_lss();
		test_parser_rel_gre();
		test_parser_rel_leq();
		test_parser_rel_geq();
		test_parser_rel_eql();
		test_parser_rel_neq();

		// 逻辑运算测试
		test_parser_logic_and();
		test_parser_logic_or();
		test_parser_logic_not();
		test_parser_complex_cond();

		// if 语句测试
		test_parser_if();
		test_parser_if_else();
		test_parser_nested_if();

		// while 循环测试
		test_parser_while();
		test_parser_break();
		test_parser_continue();

		// return 语句测试
		test_parser_return_void();
		test_parser_return_expr();

		// 语句测试
		test_parser_empty_stmt();
		test_parser_expr_stmt();
		test_parser_nested_block();

		// 函数测试
		test_parser_func_no_params();
		test_parser_func_with_params();
		test_parser_void_func();
		test_parser_func_call_as_arg();
		test_parser_nested_func_calls();
		test_parser_expr_with_func_call();
		test_parser_func_array_param();

		// 内置函数测试
		test_parser_printf();
		test_parser_printf_with_args();
		test_parser_getint();

		// 数组测试
		test_parser_array_1d();
		test_parser_array_2d();
		test_parser_array_init();
		test_parser_array_init_empty();
		test_parser_const_array();
		test_parser_array_var_index();

		// 复杂程序测试
		test_parser_multiple_globals();
		test_parser_multiple_funcs();
		test_parser_decl_before_use();
		test_parser_assign_in_expr();
		test_parser_full_program();

		// 运算符测试
		test_parser_operator_precedence();
		test_parser_assign_precedence();
		test_parser_negative_const();

		std::cout << "================================" << std::endl;
		std::cout << "All 60 Parser tests passed!" << std::endl;
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Test failed with exception: " << e.what() << std::endl;
		return 1;
	}
}
