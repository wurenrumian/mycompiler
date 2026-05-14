#include "Ir.h"
#include "IrText.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    ir::Module module;
    module.declarations.push_back("@g = global i32 7");
    module.declarations.push_back("@arr = global [3 x i32] [i32 1, i32 2, i32 3]");

    ir::Function *fn = module.create_function("main", ir::Type::i32(), std::vector<ir::Type>());
    ir::BasicBlock *entry = fn->create_block("entry");
    entry->append(ir::Instruction::raw("%slot0 = alloca i32"));
    entry->append(ir::Instruction::raw("store i32 7, i32* %slot0"));
    entry->append(ir::Instruction::raw("%0 = load i32, i32* %slot0"));
    entry->append(ir::Instruction::raw("%1 = add i32 %0, 5"));
    entry->append(ir::Instruction::raw("%2 = icmp slt i32 %1, 20"));
    entry->append(ir::Instruction::raw("br i1 %2, label %then.0, label %end.1"));

    irtext::ModuleView view;
    std::string error;
    assert(irtext::parse_module(module, &view, &error));
    assert(error.empty());
    assert(view.globals.size() == 2);
    assert(view.globals[0].name == "g");
    assert(view.globals[1].type.text == "[3 x i32]");
    assert(view.functions.size() == 1);
    assert(view.functions[0].blocks.size() == 1);
    assert(view.functions[0].blocks[0].instructions.size() == 6);
    assert(view.functions[0].blocks[0].instructions[1].kind == irtext::OpKind::Store);
    assert(view.functions[0].blocks[0].instructions[3].opcode == "add");
    assert(view.functions[0].blocks[0].instructions[5].true_label == "then.0");
    return 0;
}
