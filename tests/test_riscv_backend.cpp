#include "Ir.h"
#include "RiscVBackend.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    ir::Module module;
    ir::Function *main_fn = module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *entry = main_fn->create_block("entry");
    entry->append(ir::Instruction::ret(ir::Value::constant_i32(0)));

    riscv::Options options;
    riscv::Result result = riscv::emit_module(module, options);
    assert(result.ok);
    assert(result.message.empty());
    assert(result.assembly.find(".text") != std::string::npos);
    assert(result.assembly.find(".globl main") != std::string::npos);
    assert(result.assembly.find("li a0, 0") != std::string::npos);
    assert(result.assembly.find("ret") != std::string::npos);

    ir::Module scalar_module;
    ir::Function *scalar = scalar_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *scalar_entry = scalar->create_block("entry");
    scalar_entry->append(ir::Instruction::raw("%slot0 = alloca i32"));
    scalar_entry->append(ir::Instruction::raw("store i32 8, i32* %slot0"));
    scalar_entry->append(ir::Instruction::raw("%0 = load i32, i32* %slot0"));
    scalar_entry->append(ir::Instruction::raw("%1 = add i32 %0, 4"));
    scalar_entry->append(ir::Instruction::raw("%2 = mul i32 %1, 3"));
    scalar_entry->append(ir::Instruction::raw("%3 = sdiv i32 %2, 2"));
    scalar_entry->append(ir::Instruction::raw("%4 = srem i32 %3, 5"));
    scalar_entry->append(ir::Instruction::raw("%5 = icmp sgt i32 %4, 0"));
    scalar_entry->append(ir::Instruction::raw("%6 = zext i1 %5 to i32"));
    scalar_entry->append(ir::Instruction::raw("ret i32 %6"));
    riscv::Result scalar_result = riscv::emit_module(scalar_module, options);
    assert(scalar_result.ok);
    assert(scalar_result.assembly.find("addw") != std::string::npos);
    assert(scalar_result.assembly.find("mulw") != std::string::npos);
    assert(scalar_result.assembly.find("divw") != std::string::npos);
    assert(scalar_result.assembly.find("remw") != std::string::npos);
    assert(scalar_result.assembly.find("slt") != std::string::npos);

    ir::Module float_store_module;
    ir::Function *float_store_fn = float_store_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *float_store_entry = float_store_fn->create_block("entry");
    float_store_entry->append(ir::Instruction::raw("%slot0 = alloca float"));
    float_store_entry->append(ir::Instruction::raw("store float 1.0, float* %slot0"));
    float_store_entry->append(ir::Instruction::raw("%0 = load float, float* %slot0"));
    float_store_entry->append(ir::Instruction::raw("%1 = fptosi float %0 to i32"));
    float_store_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result float_store_result = riscv::emit_module(float_store_module, options);
    assert(float_store_result.ok);
    assert(float_store_result.assembly.find("li t0, 1.0") == std::string::npos);
    assert(float_store_result.assembly.find("fmv.w.x") != std::string::npos);
    assert(float_store_result.assembly.find("fsw") != std::string::npos);

    ir::Module float_compare_module;
    ir::Function *float_compare_fn = float_compare_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *float_compare_entry = float_compare_fn->create_block("entry");
    float_compare_entry->append(ir::Instruction::raw("%0 = fcmp olt float 1.0, 2.0"));
    float_compare_entry->append(ir::Instruction::raw("%1 = zext i1 %0 to i32"));
    float_compare_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result float_compare_result = riscv::emit_module(float_compare_module, options);
    assert(float_compare_result.ok);
    assert(float_compare_result.assembly.find("flt.s") != std::string::npos);

    ir::Module mixed_arg_module;
    std::vector<ir::Type> mixed_param_types;
    mixed_param_types.push_back(ir::Type::i32());
    mixed_param_types.push_back(ir::Type::float_ty());
    mixed_param_types.push_back(ir::Type::i32());
    mixed_param_types.push_back(ir::Type::float_ty());
    ir::Function *mixed_callee = mixed_arg_module.create_function("mix", ir::Type::float_ty(), mixed_param_types);
    ir::BasicBlock *mixed_callee_entry = mixed_callee->create_block("entry");
    mixed_callee_entry->append(ir::Instruction::raw("%4 = sitofp i32 %0 to float"));
    mixed_callee_entry->append(ir::Instruction::raw("%5 = fadd float %4, %1"));
    mixed_callee_entry->append(ir::Instruction::raw("%6 = sitofp i32 %2 to float"));
    mixed_callee_entry->append(ir::Instruction::raw("%7 = fadd float %5, %6"));
    mixed_callee_entry->append(ir::Instruction::raw("%8 = fadd float %7, %3"));
    mixed_callee_entry->append(ir::Instruction::raw("ret float %8"));
    ir::Function *mixed_caller = mixed_arg_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *mixed_caller_entry = mixed_caller->create_block("entry");
    mixed_caller_entry->append(ir::Instruction::raw("%0 = call float @mix(i32 1, float 2.0, i32 3, float 4.0)"));
    mixed_caller_entry->append(ir::Instruction::raw("%1 = fptosi float %0 to i32"));
    mixed_caller_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result mixed_arg_result = riscv::emit_module(mixed_arg_module, options);
    assert(mixed_arg_result.ok);
    const size_t mix_label = mixed_arg_result.assembly.find("mix:");
    const size_t main_label = mixed_arg_result.assembly.find("main:");
    assert(mix_label != std::string::npos);
    assert(main_label != std::string::npos);
    const std::string mix_slice = mixed_arg_result.assembly.substr(mix_label, main_label - mix_label);
    const std::string main_slice = mixed_arg_result.assembly.substr(main_label);
    assert(mix_slice.find("fsw fa0") != std::string::npos);
    assert(mix_slice.find("fsw fa1") != std::string::npos);
    assert(mix_slice.find("sw a0") != std::string::npos);
    assert(mix_slice.find("sw a1") != std::string::npos);
    assert(main_slice.find("fmv.w.x fa0") != std::string::npos);
    assert(main_slice.find("fmv.w.x fa1") != std::string::npos);
    assert(main_slice.find("li a0, 1") != std::string::npos);
    assert(main_slice.find("li a1, 3") != std::string::npos);

    ir::Module branch_module;
    ir::Function *branch_fn = branch_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *branch_entry = branch_fn->create_block("entry");
    branch_entry->append(ir::Instruction::raw("br i1 1, label %then.0, label %else.1"));
    ir::BasicBlock *then_block = branch_fn->create_block("then.0");
    then_block->append(ir::Instruction::raw("ret i32 7"));
    ir::BasicBlock *else_block = branch_fn->create_block("else.1");
    else_block->append(ir::Instruction::raw("ret i32 9"));
    riscv::Result branch_result = riscv::emit_module(branch_module, options);
    assert(branch_result.ok);
    assert(branch_result.assembly.find("bnez") != std::string::npos);
    assert(branch_result.assembly.find(".Lmain_then_0") != std::string::npos);
    assert(branch_result.assembly.find(".Lmain_else_1") != std::string::npos);

    ir::Module array_module;
    array_module.declarations.push_back("@g = global i32 7");
    array_module.declarations.push_back("@z = global [4 x i32] zeroinitializer");
    array_module.declarations.push_back("@arr = global [3 x i32] [i32 1, i32 2, i32 3]");
    ir::Function *array_fn = array_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *array_entry = array_fn->create_block("entry");
    array_entry->append(ir::Instruction::raw("%0 = load i32, i32* @g"));
    array_entry->append(ir::Instruction::raw("%1 = getelementptr [3 x i32], [3 x i32]* @arr, i32 0, i32 2"));
    array_entry->append(ir::Instruction::raw("%2 = load i32, i32* %1"));
    array_entry->append(ir::Instruction::raw("%3 = add i32 %0, %2"));
    array_entry->append(ir::Instruction::raw("ret i32 %3"));
    riscv::Result array_result = riscv::emit_module(array_module, options);
    assert(array_result.ok);
    assert(array_result.assembly.find(".globl g") != std::string::npos);
    assert(array_result.assembly.find(".word 7") != std::string::npos);
    assert(array_result.assembly.find(".zero 16") != std::string::npos);
    assert(array_result.assembly.find("lla") != std::string::npos);
    const size_t arr_label = array_result.assembly.find("arr:");
    assert(arr_label != std::string::npos);
    const std::string arr_slice = array_result.assembly.substr(arr_label, 64);
    assert(arr_slice.find(".word 1") != std::string::npos);
    assert(arr_slice.find(".word 2") != std::string::npos);
    assert(arr_slice.find(".word 3") != std::string::npos);
    assert(arr_slice.find(".word 32") == std::string::npos);

    ir::Module local_array_module;
    ir::Function *local_array_fn = local_array_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *local_array_entry = local_array_fn->create_block("entry");
    local_array_entry->append(ir::Instruction::raw("%slot0 = alloca [4 x i32]"));
    local_array_entry->append(ir::Instruction::raw("%0 = getelementptr [4 x i32], [4 x i32]* %slot0, i32 0, i32 2"));
    local_array_entry->append(ir::Instruction::raw("store i32 11, i32* %0"));
    local_array_entry->append(ir::Instruction::raw("%1 = load i32, i32* %0"));
    local_array_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result local_array_result = riscv::emit_module(local_array_module, options);
    assert(local_array_result.ok);
    assert(local_array_result.assembly.find("slli") != std::string::npos || local_array_result.assembly.find("addi") != std::string::npos);
    assert(local_array_result.assembly.find("sw") != std::string::npos);
    assert(local_array_result.assembly.find("lw") != std::string::npos);
    assert(local_array_result.assembly.find("sd t0, -") != std::string::npos);
    assert(local_array_result.assembly.find("ld t1, -") != std::string::npos ||
           local_array_result.assembly.find("ld t6, -") != std::string::npos);

    ir::Module gep_load_module;
    gep_load_module.declarations.push_back("@arr2 = global [3 x i32] [i32 1, i32 2, i32 3]");
    ir::Function *gep_load_fn = gep_load_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *gep_load_entry = gep_load_fn->create_block("entry");
    gep_load_entry->append(ir::Instruction::raw("%0 = getelementptr [3 x i32], [3 x i32]* @arr2, i32 0, i32 2"));
    gep_load_entry->append(ir::Instruction::raw("%1 = load i32, i32* %0"));
    gep_load_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result gep_load_result = riscv::emit_module(gep_load_module, options);
    assert(gep_load_result.ok);
    assert(gep_load_result.assembly.find("sd t0, -") != std::string::npos);
    assert(gep_load_result.assembly.find("ld t1, -") != std::string::npos ||
           gep_load_result.assembly.find("ld t6, -") != std::string::npos);

    ir::Module matrix_module;
    ir::Function *matrix_fn = matrix_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *matrix_entry = matrix_fn->create_block("entry");
    matrix_entry->append(ir::Instruction::raw("%slot0 = alloca [4 x [2 x i32]]"));
    matrix_entry->append(ir::Instruction::raw("%0 = getelementptr [4 x [2 x i32]], [4 x [2 x i32]]* %slot0, i32 0, i32 2, i32 1"));
    matrix_entry->append(ir::Instruction::raw("store i32 14, i32* %0"));
    matrix_entry->append(ir::Instruction::raw("%1 = load i32, i32* %0"));
    matrix_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result matrix_result = riscv::emit_module(matrix_module, options);
    assert(matrix_result.ok);
    assert(matrix_result.assembly.find("sd t0, -") != std::string::npos);
    assert(matrix_result.assembly.find("ld t1, -") != std::string::npos ||
           matrix_result.assembly.find("ld t6, -") != std::string::npos);

    ir::Module param_gep_module;
    ir::Function *param_gep_fn = param_gep_module.create_function(
        "row_load", ir::Type::i32(), std::vector<ir::Type>(1, ir::Type::ptr(ir::Type::array(4, ir::Type::i32()))));
    ir::BasicBlock *param_gep_entry = param_gep_fn->create_block("entry");
    param_gep_entry->append(ir::Instruction::raw("%1 = getelementptr [4 x i32], [4 x i32]* %0, i32 2, i32 1"));
    param_gep_entry->append(ir::Instruction::raw("%2 = load i32, i32* %1"));
    param_gep_entry->append(ir::Instruction::raw("ret i32 %2"));
    riscv::Result param_gep_result = riscv::emit_module(param_gep_module, options);
    assert(param_gep_result.ok);
    assert(param_gep_result.assembly.find("li t2, 16") != std::string::npos ||
           param_gep_result.assembly.find("slli t1, t1, 4") != std::string::npos ||
           param_gep_result.assembly.find("li t6, 16") != std::string::npos ||
           param_gep_result.assembly.find("addi t0, t0, 36") != std::string::npos);

    ir::Module call_module;
    ir::Function *callee = call_module.create_function("sum10", ir::Type::i32(),
        std::vector<ir::Type>(10, ir::Type::i32()));
    ir::BasicBlock *callee_entry = callee->create_block("entry");
    callee_entry->append(ir::Instruction::raw("%10 = add i32 %0, %9"));
    callee_entry->append(ir::Instruction::raw("ret i32 %10"));

    ir::Function *caller = call_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *caller_entry = caller->create_block("entry");
    caller_entry->append(ir::Instruction::raw("%0 = call i32 @sum10(i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9)"));
    caller_entry->append(ir::Instruction::raw("call void @putint(i32 %0)"));
    caller_entry->append(ir::Instruction::raw("ret i32 %0"));
    riscv::Result call_result = riscv::emit_module(call_module, options);
    assert(call_result.ok);
    assert(call_result.assembly.find("mv a0") != std::string::npos || call_result.assembly.find("li a0") != std::string::npos);
    assert(call_result.assembly.find("call sum10") != std::string::npos);
    assert(call_result.assembly.find("call putint") != std::string::npos);
    const size_t sum10_label = call_result.assembly.find("sum10:");
    assert(sum10_label != std::string::npos);
    const std::string sum10_slice = call_result.assembly.substr(sum10_label, 120);
    assert(sum10_slice.find("li a0, 0") == std::string::npos);

    std::vector<ir::Type> many_param_types(300, ir::Type::i32());
    ir::Module many_call_module;
    ir::Function *many_callee = many_call_module.create_function("manysum", ir::Type::i32(), many_param_types);
    ir::BasicBlock *many_callee_entry = many_callee->create_block("entry");
    many_callee_entry->append(ir::Instruction::raw("%300 = add i32 %0, %299"));
    many_callee_entry->append(ir::Instruction::raw("ret i32 %300"));
    ir::Function *many_caller = many_call_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *many_caller_entry = many_caller->create_block("entry");
    std::ostringstream many_call;
    many_call << "%0 = call i32 @manysum(";
    for (int i = 0; i < 300; ++i)
    {
        if (i != 0)
        {
            many_call << ", ";
        }
        many_call << "i32 " << i;
    }
    many_call << ")";
    many_caller_entry->append(ir::Instruction::raw(many_call.str()));
    many_caller_entry->append(ir::Instruction::raw("ret i32 %0"));
    riscv::Result many_call_result = riscv::emit_module(many_call_module, options);
    assert(many_call_result.ok);
    assert(many_call_result.assembly.find("sw t0, 2048(sp)") == std::string::npos);
    assert(many_call_result.assembly.find("addi sp, sp, -2336") == std::string::npos);
    assert(many_call_result.assembly.find("lw t0, 2328(s0)") == std::string::npos);
    assert(many_call_result.assembly.find("call manysum") != std::string::npos);

    ir::Module large_frame_module;
    ir::Function *large_frame_fn = large_frame_module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *large_frame_entry = large_frame_fn->create_block("entry");
    large_frame_entry->append(ir::Instruction::raw("%slot0 = alloca [2048 x i32]"));
    large_frame_entry->append(ir::Instruction::raw("%0 = getelementptr [2048 x i32], [2048 x i32]* %slot0, i32 0, i32 1024"));
    large_frame_entry->append(ir::Instruction::raw("store i32 5, i32* %0"));
    large_frame_entry->append(ir::Instruction::raw("%1 = load i32, i32* %0"));
    large_frame_entry->append(ir::Instruction::raw("ret i32 %1"));
    riscv::Result large_frame_result = riscv::emit_module(large_frame_module, options);
    assert(large_frame_result.ok);
    assert(large_frame_result.assembly.find("addi sp, sp, -8224") == std::string::npos);
    assert(large_frame_result.assembly.find("sd ra, 8216(sp)") == std::string::npos);
    assert(large_frame_result.assembly.find("sd s0, 8208(sp)") == std::string::npos);
    assert(large_frame_result.assembly.find("addi s0, sp, 8224") == std::string::npos);
    assert(large_frame_result.assembly.find("ld ra, 8216(sp)") == std::string::npos);
    assert(large_frame_result.assembly.find("ld s0, 8208(sp)") == std::string::npos);
    assert(large_frame_result.assembly.find("addi sp, sp, 8224") == std::string::npos);
    assert(large_frame_result.assembly.find("addi t0, t0, 4096") == std::string::npos);
    return 0;
}
