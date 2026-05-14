#include "Ir.h"
#include "IrBuilder.h"
#include "IrPrinter.h"
#include "ParserFrontend.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void write_file(const std::string &path, const std::string &content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    out << content;
}

void remove_file(const std::string &path)
{
    std::remove(path.c_str());
}

std::unique_ptr<ast::CompUnit> parse_program(const std::string &path,
                                             const std::string &content)
{
    write_file(path, content);
    std::unique_ptr<ast::CompUnit> root;
    std::string error;
    const bool parsed = parse_file_to_ast(path, &root, &error);
    remove_file(path);
    assert(parsed);
    assert(error.empty());
    assert(root.get() != 0);
    return root;
}
} // namespace

int main()
{
    ir::Module module;
    module.declare_runtime_functions();
    ir::Function *main_fn = module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *entry = main_fn->create_block("entry");
    entry->append(ir::Instruction::ret(ir::Value::constant_i32(0)));

    const std::string text = ir::print_module(module);
    assert(text.find("declare void @putint(i32)") != std::string::npos);
    assert(text.find("define i32 @main()") != std::string::npos);
    assert(text.find("ret i32 0") != std::string::npos);

    std::unique_ptr<ast::CompUnit> root = parse_program("test_ir_builder.sy", "int main(){ return 3; }\n");

    irgen::Options options;
    options.emit_main_return_value = true;
    irgen::Result built = irgen::build_module(*root, options);
    assert(built.ok);

    const std::string built_text = ir::print_module(built.module);
    assert(built_text.find("call void @putint(i32 3)") != std::string::npos);
    assert(built_text.find("ret i32 3") != std::string::npos);

    std::unique_ptr<ast::CompUnit> wrapped_root = parse_program(
        "test_ir_builder_wrap.sy",
        "int main(){ return 1024; }\n");
    irgen::Result wrapped = irgen::build_module(*wrapped_root, options);
    assert(wrapped.ok);
    const std::string wrapped_text = ir::print_module(wrapped.module);
    assert(wrapped_text.find("call void @putint(i32 0)") != std::string::npos);
    assert(wrapped_text.find("ret i32 1024") != std::string::npos);

    std::unique_ptr<ast::CompUnit> local_arith_root = parse_program(
        "test_ir_builder_local.sy",
        "int main() {\n"
        "  int a, b = 8, c = 12;\n"
        "  a = b + c;\n"
        "  return a;\n"
        "}\n");
    irgen::Result local_arith = irgen::build_module(*local_arith_root, options);
    assert(local_arith.ok);
    const std::string local_arith_text = ir::print_module(local_arith.module);
    assert(local_arith_text.find("alloca i32") != std::string::npos);
    assert(local_arith_text.find("add i32") != std::string::npos);
    assert(local_arith_text.find("call void @putint(i32") != std::string::npos);

    std::unique_ptr<ast::CompUnit> global_root = parse_program(
        "test_ir_builder_global.sy",
        "int a = 3;\n"
        "int b = 5;\n"
        "int main(){\n"
        "    int a = 5;\n"
        "    return a + b;\n"
        "}\n");
    irgen::Result global_result = irgen::build_module(*global_root, options);
    assert(global_result.ok);
    const std::string global_text = ir::print_module(global_result.module);
    assert(global_text.find("@a = global i32 3") != std::string::npos);
    assert(global_text.find("@b = global i32 5") != std::string::npos);
    assert(global_text.find("load i32, i32* @b") != std::string::npos);

    std::unique_ptr<ast::CompUnit> hidden_init_root = parse_program(
        "test_ir_builder_hidden_init.sy",
        "int x = 330;\n"
        "int main(){\n"
        "  int x = x + 1;\n"
        "  return x;\n"
        "}\n");
    irgen::Result hidden_init_result = irgen::build_module(*hidden_init_root, options);
    assert(hidden_init_result.ok);
    const std::string hidden_init_text = ir::print_module(hidden_init_result.module);
    assert(hidden_init_text.find("load i32, i32* @x") != std::string::npos);

    std::unique_ptr<ast::CompUnit> format_output_root = parse_program(
        "test_ir_builder_format_output.sy",
        "int main(){\n"
        "    printf(\"%d %d\\n\", 331, 1);\n"
        "    return 0;\n"
        "}\n");
    irgen::Result format_output_result = irgen::build_module(*format_output_root, options);
    assert(format_output_result.ok);
    const std::string format_output_text = ir::print_module(format_output_result.module);
    assert(format_output_text.find("call void @putint(i32 331)") != std::string::npos);
    assert(format_output_text.find("call void @putch(i32 32)") != std::string::npos);
    assert(format_output_text.find("call void @putint(i32 1)") != std::string::npos);
    assert(format_output_text.find("call void @putch(i32 10)") != std::string::npos);

    std::unique_ptr<ast::CompUnit> array_root = parse_program(
        "test_ir_builder_array.sy",
        "int a[10][10];\n"
        "int main(){\n"
        "    return 0;\n"
        "}\n");
    irgen::Result array_result = irgen::build_module(*array_root, options);
    assert(array_result.ok);
    const std::string array_text = ir::print_module(array_result.module);
    assert(array_text.find("@a = global [10 x [10 x i32]] zeroinitializer") != std::string::npos);

    std::unique_ptr<ast::CompUnit> const_array_root = parse_program(
        "test_ir_builder_const_array.sy",
        "const int a[5]={0,1,2,3,4};\n"
        "int main(){\n"
        "    return a[4];\n"
        "}\n");
    irgen::Result const_array_result = irgen::build_module(*const_array_root, options);
    assert(const_array_result.ok);
    const std::string const_array_text = ir::print_module(const_array_result.module);
    assert(const_array_text.find("@a = global [5 x i32]") != std::string::npos);
    assert(const_array_text.find("call void @putint(i32 4)") != std::string::npos);

    std::unique_ptr<ast::CompUnit> add_root = parse_program(
        "test_ir_builder_add.sy",
        "int main(){\n"
        "    int a, b;\n"
        "    a = 10;\n"
        "    b = -1;\n"
        "    return a + b;\n"
        "}\n");
    irgen::Result add_result = irgen::build_module(*add_root, options);
    assert(add_result.ok);

    std::unique_ptr<ast::CompUnit> add_const_root = parse_program(
        "test_ir_builder_addc.sy",
        "const int a = 10;\n"
        "int main(){\n"
        "    return a + 5;\n"
        "}\n");
    irgen::Result add_const_result = irgen::build_module(*add_const_root, options);
    assert(add_const_result.ok);

    std::unique_ptr<ast::CompUnit> sub_root = parse_program(
        "test_ir_builder_sub.sy",
        "const int a = 10;\n"
        "int main(){\n"
        "    int b;\n"
        "    b = 2;\n"
        "    return b - a;\n"
        "}\n");
    irgen::Result sub_result = irgen::build_module(*sub_root, options);
    assert(sub_result.ok);

    std::unique_ptr<ast::CompUnit> while_array_root = parse_program(
        "test_ir_builder_while_array.sy",
        "int arr[-1 + 2 * 4 - 99 / 99] = {1, 2, 33, 4, 5, 6};\n"
        "int main() {\n"
        "  int i = 0, sum = 0;\n"
        "  while (i < 6) {\n"
        "    sum = sum + arr[i];\n"
        "    i = i + 1;\n"
        "  }\n"
        "  return sum;\n"
        "}\n");
    irgen::Result while_array_result = irgen::build_module(*while_array_root, options);
    assert(while_array_result.ok);
    const std::string while_array_text = ir::print_module(while_array_result.module);
    assert(while_array_text.find("@arr = global [6 x i32]") != std::string::npos);
    assert(while_array_text.find("getelementptr [6 x i32], [6 x i32]* @arr, i32 0, i32") != std::string::npos);
    assert(while_array_text.find("br i1") != std::string::npos);

    std::unique_ptr<ast::CompUnit> if_root = parse_program(
        "test_ir_builder_if.sy",
        "int main() {\n"
        "    int a;\n"
        "    a = 10;\n"
        "    if (+-!!!a) {\n"
        "        a = - - -1;\n"
        "    }\n"
        "    else {\n"
        "        a = 0;\n"
        "    }\n"
        "    return a;\n"
        "}\n");
    irgen::Result if_result = irgen::build_module(*if_root, options);
    assert(if_result.ok);
    const std::string if_text = ir::print_module(if_result.module);
    assert(if_text.find("br i1") != std::string::npos);
    assert(if_text.find("if.then.") != std::string::npos);
    assert(if_text.find("if.else.") != std::string::npos);

    std::unique_ptr<ast::CompUnit> local_array_rw_root = parse_program(
        "test_ir_builder_local_array_rw.sy",
        "int main() {\n"
        "  int arr[4] = {1, 2};\n"
        "  int i = 2;\n"
        "  arr[i] = arr[i - 1] + arr[i - 2];\n"
        "  return arr[i];\n"
        "}\n");
    irgen::Result local_array_rw_result = irgen::build_module(*local_array_rw_root, options);
    assert(local_array_rw_result.ok);
    const std::string local_array_rw_text = ir::print_module(local_array_rw_result.module);
    assert(local_array_rw_text.find("alloca [4 x i32]") != std::string::npos);
    assert(local_array_rw_text.find("getelementptr [4 x i32], [4 x i32]*") != std::string::npos);

    std::unique_ptr<ast::CompUnit> func_call_root = parse_program(
        "test_ir_builder_func_call.sy",
        "int add(int x, int y) {\n"
        "  return x + y;\n"
        "}\n"
        "int main() {\n"
        "  return add(2, 3);\n"
        "}\n");
    irgen::Result func_call_result = irgen::build_module(*func_call_root, options);
    assert(func_call_result.ok);
    const std::string func_call_text = ir::print_module(func_call_result.module);
    assert(func_call_text.find("define i32 @add(i32, i32)") != std::string::npos);
    assert(func_call_text.find("call i32 @add(i32 2, i32 3)") != std::string::npos);

    std::unique_ptr<ast::CompUnit> runtime_root = parse_program(
        "test_ir_builder_runtime_public.sy",
        "int a[8];\n"
        "int main(){\n"
        "  int n = getarray(a);\n"
        "  starttime();\n"
        "  putarray(n, a);\n"
        "  stoptime();\n"
        "  return n;\n"
        "}\n");
    irgen::Options final_ir_options;
    final_ir_options.emit_main_return_value = false;
    irgen::Result runtime_result = irgen::build_module(*runtime_root, final_ir_options);
    assert(runtime_result.ok);
    const std::string runtime_text = ir::print_module(runtime_result.module);
    assert(runtime_text.find("call i32 @getarray(i32*") != std::string::npos);
    assert(runtime_text.find("call void @putarray(i32") != std::string::npos);
    assert(runtime_text.find("call void @_sysy_starttime(i32 0)") != std::string::npos);
    assert(runtime_text.find("call void @_sysy_stoptime(i32 0)") != std::string::npos);

    std::unique_ptr<ast::CompUnit> partial_decay_root = parse_program(
        "test_ir_builder_partial_decay.sy",
        "int x[2][3];\n"
        "int main(){\n"
        "  putarray(3, x[1]);\n"
        "  return 0;\n"
        "}\n");
    irgen::Result partial_decay_result = irgen::build_module(*partial_decay_root, final_ir_options);
    assert(partial_decay_result.ok);
    const std::string partial_decay_text = ir::print_module(partial_decay_result.module);
    assert(partial_decay_text.find("getelementptr [2 x [3 x i32]], [2 x [3 x i32]]* @x, i32 0, i32 1, i32 0") != std::string::npos);
    assert(partial_decay_text.find("call void @putarray(i32 3, i32*") != std::string::npos);

    std::unique_ptr<ast::CompUnit> dynamic_partial_decay_root = parse_program(
        "test_ir_builder_dynamic_partial_decay.sy",
        "int x[4][5][6];\n"
        "int main(){\n"
        "  int i = 2;\n"
        "  int j = 3;\n"
        "  putarray(6, x[i - 1][j - 1]);\n"
        "  return 0;\n"
        "}\n");
    irgen::Result dynamic_partial_decay_result = irgen::build_module(*dynamic_partial_decay_root, final_ir_options);
    assert(dynamic_partial_decay_result.ok);
    const std::string dynamic_partial_decay_text = ir::print_module(dynamic_partial_decay_result.module);
    assert(dynamic_partial_decay_text.find("getelementptr [4 x [5 x [6 x i32]]], [4 x [5 x [6 x i32]]]* @x") != std::string::npos);
    assert(dynamic_partial_decay_text.find("call void @putarray(i32 6, i32*") != std::string::npos);

    std::unique_ptr<ast::CompUnit> float_root = parse_program(
        "test_ir_builder_float.sy",
        "float a = 3.5;\n"
        "int main(){\n"
        "  float x = 1.25;\n"
        "  float y = 2.0;\n"
        "  float z = x + y;\n"
        "  putfloat(z);\n"
        "  return z;\n"
        "}\n");
    irgen::Result float_ir = irgen::build_module(*float_root, final_ir_options);
    assert(float_ir.ok);
    const std::string float_text = ir::print_module(float_ir.module);
    assert(float_text.find("@a = global float") != std::string::npos);
    assert(float_text.find("fadd float") != std::string::npos);
    assert(float_text.find("call void @putfloat(float") != std::string::npos);
    assert(float_text.find("fptosi float") != std::string::npos);

    std::unique_ptr<ast::CompUnit> precise_float_global_root = parse_program(
        "test_ir_builder_precise_float_global.sy",
        "float table[2] = {0.123456789, 0.987654321};\n"
        "int main(){ putfloat(table[0]); return 0; }\n");
    irgen::Result precise_float_global_ir = irgen::build_module(*precise_float_global_root, final_ir_options);
    assert(precise_float_global_ir.ok);
    const std::string precise_float_global_text = ir::print_module(precise_float_global_ir.module);
    assert(precise_float_global_text.find("0.123456789") != std::string::npos);
    assert(precise_float_global_text.find("0.987654321") != std::string::npos);

    std::unique_ptr<ast::CompUnit> float_compare_root = parse_program(
        "test_ir_builder_float_compare.sy",
        "int main(){\n"
        "  float x = 1.0;\n"
        "  float y = 2.0;\n"
        "  if (x < y) {\n"
        "    if (x != y) {\n"
        "      return 1;\n"
        "    }\n"
        "  }\n"
        "  return 0;\n"
        "}\n");
    irgen::Result float_compare_ir = irgen::build_module(*float_compare_root, final_ir_options);
    assert(float_compare_ir.ok);
    const std::string float_compare_text = ir::print_module(float_compare_ir.module);
    assert(float_compare_text.find("fcmp olt float") != std::string::npos);
    assert(float_compare_text.find("fcmp one float") != std::string::npos);

    std::unique_ptr<ast::CompUnit> float_chained_compare_root = parse_program(
        "test_ir_builder_float_chained_compare.sy",
        "float id(float x) { return x; }\n"
        "int main(){\n"
        "  float x = id(1.0);\n"
        "  float y = id(2.0);\n"
        "  if (x < y < 3.0) {\n"
        "    return 1;\n"
        "  }\n"
        "  return 0;\n"
        "}\n");
    irgen::Result float_chained_compare_ir = irgen::build_module(*float_chained_compare_root, final_ir_options);
    assert(float_chained_compare_ir.ok);
    const std::string float_chained_compare_text = ir::print_module(float_chained_compare_ir.module);
    assert(float_chained_compare_text.find("fcmp olt float") != std::string::npos);
    assert(float_chained_compare_text.find("zext i1") != std::string::npos);
    return 0;
}
