#include "RiscVBackend.h"

#include "IrText.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <vector>

namespace riscv
{
namespace
{
class AsmEmitter
{
public:
    void line(const std::string &text)
    {
        out_ << text << "\n";
    }

    void inst(const std::string &text)
    {
        out_ << "    " << text << "\n";
    }

    std::string str() const
    {
        return out_.str();
    }

private:
    std::ostringstream out_;
};

Result fail(const std::string &message)
{
    Result result;
    result.ok = false;
    result.message = message;
    return result;
}

int align_up(int value, int alignment)
{
    const int remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}

struct StackSlot
{
    int offset;
    int size;

    StackSlot() : offset(0), size(0) {}
    StackSlot(int offset_value, int size_value) : offset(offset_value), size(size_value) {}
};

class Frame
{
public:
    Frame() : slots_(), next_offset_(16), direct_address_values_() {}

    void reserve_value(const std::string &name, int size, bool is_direct_address)
    {
        if (name.empty() || has_value(name))
        {
            if (is_direct_address)
            {
                direct_address_values_[name] = true;
            }
            return;
        }

        const int slot_size = align_up(size < 8 ? 8 : size, 8);
        next_offset_ += slot_size;
        slots_[name] = StackSlot(-next_offset_, slot_size);
        if (is_direct_address)
        {
            direct_address_values_[name] = true;
        }
    }

    bool has_value(const std::string &name) const
    {
        return slots_.find(name) != slots_.end();
    }

    StackSlot value_slot(const std::string &name) const
    {
        std::map<std::string, StackSlot>::const_iterator found = slots_.find(name);
        if (found == slots_.end())
        {
            return StackSlot();
        }
        return found->second;
    }

    bool is_direct_address_value(const std::string &name) const
    {
        std::map<std::string, bool>::const_iterator found = direct_address_values_.find(name);
        return found != direct_address_values_.end() && found->second;
    }

    int frame_size() const
    {
        return align_up(next_offset_, 16);
    }

private:
    std::map<std::string, StackSlot> slots_;
    int next_offset_;
    std::map<std::string, bool> direct_address_values_;
};

int type_size_bytes(const irtext::TypeView &type)
{
    int size = type.is_void ? 0 : 4;
    for (size_t index = 0; index < type.array_dims.size(); ++index)
    {
        size *= type.array_dims[index];
    }
    return size == 0 ? 4 : size;
}

std::string sanitize_label(const std::string &text)
{
    std::string out = text;
    for (size_t index = 0; index < out.size(); ++index)
    {
        const char ch = out[index];
        if (!(ch >= 'a' && ch <= 'z') &&
            !(ch >= 'A' && ch <= 'Z') &&
            !(ch >= '0' && ch <= '9'))
        {
            out[index] = '_';
        }
    }
    return out;
}

std::string block_label(const std::string &function_name, const std::string &block_name)
{
    return ".L" + function_name + "_" + sanitize_label(block_name);
}

bool fits_i12(int value)
{
    return value >= -2048 && value <= 2047;
}

void emit_add_immediate(AsmEmitter &out, const char *dst, const char *base, int value, const char *scratch)
{
    if (fits_i12(value))
    {
        std::ostringstream text;
        text << "addi " << dst << ", " << base << ", " << value;
        out.inst(text.str());
        return;
    }

    std::ostringstream text;
    text << "li " << scratch << ", " << value;
    out.inst(text.str());
    std::ostringstream add_text;
    add_text << "add " << dst << ", " << base << ", " << scratch;
    out.inst(add_text.str());
}

void emit_stack_address(AsmEmitter &out, int offset, const char *reg)
{
    emit_add_immediate(out, reg, "s0", offset, "t6");
}

void emit_memory_store(AsmEmitter &out, const char *opcode, const char *src, const char *base, int offset, const char *scratch)
{
    if (fits_i12(offset))
    {
        std::ostringstream text;
        text << opcode << " " << src << ", " << offset << "(" << base << ")";
        out.inst(text.str());
        return;
    }

    emit_add_immediate(out, scratch, base, offset, scratch);
    std::ostringstream text;
    text << opcode << " " << src << ", 0(" << scratch << ")";
    out.inst(text.str());
}

void emit_memory_load(AsmEmitter &out, const char *opcode, const char *dst, const char *base, int offset, const char *scratch)
{
    if (fits_i12(offset))
    {
        std::ostringstream text;
        text << opcode << " " << dst << ", " << offset << "(" << base << ")";
        out.inst(text.str());
        return;
    }

    emit_add_immediate(out, scratch, base, offset, scratch);
    std::ostringstream text;
    text << opcode << " " << dst << ", 0(" << scratch << ")";
    out.inst(text.str());
}

void load_word_from_slot(AsmEmitter &out, const StackSlot &slot, const char *reg)
{
    emit_memory_load(out, "lw", reg, "s0", slot.offset, "t6");
}

void store_word_to_slot(AsmEmitter &out, const StackSlot &slot, const char *reg)
{
    emit_memory_store(out, "sw", reg, "s0", slot.offset, "t6");
}

void load_doubleword_from_slot(AsmEmitter &out, const StackSlot &slot, const char *reg)
{
    emit_memory_load(out, "ld", reg, "s0", slot.offset, "t6");
}

void store_doubleword_to_slot(AsmEmitter &out, const StackSlot &slot, const char *reg)
{
    emit_memory_store(out, "sd", reg, "s0", slot.offset, "t6");
}

void load_i32(AsmEmitter &out, const irtext::Operand &operand, const Frame &frame, const char *reg)
{
    if (operand.is_constant)
    {
        out.inst(std::string("li ") + reg + ", " + operand.value);
        return;
    }

    if (operand.is_global)
    {
        out.inst(std::string("lla t6, ") + operand.value.substr(1));
        out.inst(std::string("lw ") + reg + ", 0(t6)");
        return;
    }

    const StackSlot slot = frame.value_slot(operand.value);
    load_word_from_slot(out, slot, reg);
}

void store_i32(AsmEmitter &out, const std::string &value_name, const Frame &frame, const char *reg)
{
    const StackSlot slot = frame.value_slot(value_name);
    store_word_to_slot(out, slot, reg);
}

void load_address(AsmEmitter &out, const irtext::Operand &operand, const Frame &frame, const char *reg)
{
    if (operand.is_global)
    {
        out.inst(std::string("lla ") + reg + ", " + operand.value.substr(1));
        return;
    }

    const StackSlot slot = frame.value_slot(operand.value);
    if (frame.is_direct_address_value(operand.value))
    {
        emit_stack_address(out, slot.offset, reg);
        return;
    }

    load_doubleword_from_slot(out, slot, reg);
}

int array_stride_bytes(const std::vector<int> &dims, size_t index)
{
    int stride = 4;
    for (size_t next = index + 1; next < dims.size(); ++next)
    {
        stride *= dims[next];
    }
    return stride;
}

std::vector<int> parse_initializer_words(const std::string &initializer)
{
    std::vector<int> values;
    const std::string needle = "i32 ";
    size_t start = 0;
    while (true)
    {
        const size_t pos = initializer.find(needle, start);
        if (pos == std::string::npos)
        {
            break;
        }

        size_t index = pos + needle.size();
        std::string current;
        if (index < initializer.size() && initializer[index] == '-')
        {
            current.push_back(initializer[index]);
            ++index;
        }
        while (index < initializer.size() &&
               initializer[index] >= '0' && initializer[index] <= '9')
        {
            current.push_back(initializer[index]);
            ++index;
        }
        if (!current.empty() && current != "-")
        {
            values.push_back(std::atoi(current.c_str()));
        }
        start = index;
    }
    return values;
}

unsigned int float_bits(float value)
{
    unsigned int bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

std::vector<float> parse_float_initializer_words(const std::string &initializer)
{
    std::vector<float> values;
    const std::string needle = "float ";
    size_t start = 0;
    while (true)
    {
        const size_t pos = initializer.find(needle, start);
        if (pos == std::string::npos)
        {
            break;
        }

        size_t index = pos + needle.size();
        std::string current;
        if (index < initializer.size() && (initializer[index] == '-' || initializer[index] == '+'))
        {
            current.push_back(initializer[index]);
            ++index;
        }
        while (index < initializer.size())
        {
            const char ch = initializer[index];
            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E' || ch == '-' || ch == '+')
            {
                current.push_back(ch);
                ++index;
                continue;
            }
            break;
        }
        if (!current.empty() && current != "-" && current != "+")
        {
            values.push_back(static_cast<float>(std::strtod(current.c_str(), 0)));
        }
        start = index;
    }
    return values;
}

void emit_globals(AsmEmitter &out, const irtext::ModuleView &view)
{
    for (size_t index = 0; index < view.globals.size(); ++index)
    {
        const irtext::GlobalView &global = view.globals[index];
        const int size_bytes = type_size_bytes(global.type);
        const bool is_scalar_i32 = global.type.array_dims.empty() && global.type.is_i32;
        const bool is_scalar_float = global.type.array_dims.empty() && global.type.is_float;
        if (is_scalar_i32 && global.initializer != "0")
        {
            out.line("    .data");
            out.line("    .globl " + global.name);
            out.line(global.name + ":");
            out.line("    .word " + global.initializer);
            continue;
        }
        if (is_scalar_float && global.initializer != "0.000000")
        {
            out.line("    .data");
            out.line("    .globl " + global.name);
            out.line(global.name + ":");
            std::ostringstream text;
            text << "    .word " << float_bits(static_cast<float>(std::strtod(global.initializer.c_str(), 0)));
            out.line(text.str());
            continue;
        }
        if (is_scalar_i32)
        {
            out.line("    .bss");
            out.line("    .globl " + global.name);
            out.line(global.name + ":");
            {
                std::ostringstream text;
                text << "    .zero " << size_bytes;
                out.line(text.str());
            }
            continue;
        }
        if (is_scalar_float)
        {
            out.line("    .bss");
            out.line("    .globl " + global.name);
            out.line(global.name + ":");
            {
                std::ostringstream text;
                text << "    .zero " << size_bytes;
                out.line(text.str());
            }
            continue;
        }
        if (global.zero_initializer)
        {
            out.line("    .bss");
            out.line("    .globl " + global.name);
            out.line(global.name + ":");
            {
                std::ostringstream text;
                text << "    .zero " << size_bytes;
                out.line(text.str());
            }
            continue;
        }

        out.line("    .data");
        out.line("    .globl " + global.name);
        out.line(global.name + ":");
        if (global.type.is_float)
        {
            const std::vector<float> values = parse_float_initializer_words(global.initializer);
            for (size_t value_index = 0; value_index < values.size(); ++value_index)
            {
                std::ostringstream text;
                text << "    .word " << float_bits(values[value_index]);
                out.line(text.str());
            }
        }
        else
        {
            const std::vector<int> values = parse_initializer_words(global.initializer);
            for (size_t value_index = 0; value_index < values.size(); ++value_index)
            {
                std::ostringstream text;
                text << "    .word " << values[value_index];
                out.line(text.str());
            }
        }
    }
}

void emit_gep(AsmEmitter &out, const irtext::InstructionView &inst, const Frame &frame)
{
    const irtext::Operand &base_operand = inst.operands[0];
    load_address(out, base_operand, frame, "t0");

    irtext::TypeView base_type;
    std::string error;
    parse_type(inst.result_type, &base_type, &error);
    int constant_offset = 0;
    bool skipped_zero_prefix = false;

    for (size_t index = 1; index < inst.operands.size(); ++index)
    {
        const irtext::Operand &operand = inst.operands[index];
        const size_t raw_index = index - 1;
        if (raw_index == 0 && operand.is_constant && operand.value == "0")
        {
            skipped_zero_prefix = true;
            continue;
        }

        int stride = 4;
        if (!skipped_zero_prefix && raw_index == 0 && !base_type.array_dims.empty())
        {
            stride = type_size_bytes(base_type);
        }
        else
        {
            const size_t dim_index = raw_index - 1;
            stride = dim_index < base_type.array_dims.size()
                         ? array_stride_bytes(base_type.array_dims, dim_index)
                         : 4;
        }
        if (operand.is_constant)
        {
            constant_offset += std::atoi(operand.value.c_str()) * stride;
            continue;
        }

        load_i32(out, operand, frame, "t1");
        if (stride == 4)
        {
            out.inst("slli t1, t1, 2");
        }
        else
        {
            std::ostringstream text;
            text << "li t2, " << stride;
            out.inst(text.str());
            out.inst("mul t1, t1, t2");
        }
        out.inst("add t0, t0, t1");
    }

    if (constant_offset != 0)
    {
        emit_add_immediate(out, "t0", "t0", constant_offset, "t6");
    }

    store_doubleword_to_slot(out, frame.value_slot(inst.result), "t0");
}

Frame build_frame(const irtext::FunctionView &fn)
{
    Frame frame;
    for (size_t param_index = 0; param_index < fn.params.size(); ++param_index)
    {
        std::ostringstream name;
        name << "%" << param_index;
        frame.reserve_value(name.str(), 8, false);
    }
    for (size_t block_index = 0; block_index < fn.blocks.size(); ++block_index)
    {
        const irtext::BlockView &block = fn.blocks[block_index];
        for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
        {
            const irtext::InstructionView &inst = block.instructions[inst_index];
            if (inst.kind == irtext::OpKind::Alloca)
            {
                irtext::TypeView type;
                std::string error;
                parse_type(inst.result_type, &type, &error);
                frame.reserve_value(inst.result, type_size_bytes(type), true);
            }
            else if (inst.kind == irtext::OpKind::Gep)
            {
                frame.reserve_value(inst.result, 8, false);
            }
            else if (!inst.result.empty())
            {
                frame.reserve_value(inst.result, 8, false);
            }
        }
    }
    return frame;
}

bool operand_is_pointer_like(const irtext::Operand &operand)
{
    return operand.type.find('*') != std::string::npos;
}

bool type_is_pointer_like(const std::string &type)
{
    return type.find('*') != std::string::npos;
}

bool operand_is_float_like(const irtext::Operand &operand)
{
    return operand.type == "float";
}

bool type_is_float_like(const std::string &type)
{
    return type == "float";
}

void load_operand_value(AsmEmitter &out, const irtext::Operand &operand, const Frame &frame, const char *reg)
{
    if (operand_is_pointer_like(operand))
    {
        load_address(out, operand, frame, reg);
        return;
    }
    load_i32(out, operand, frame, reg);
}

void load_float_value(AsmEmitter &out, const irtext::Operand &operand, const Frame &frame, const char *freg)
{
    if (operand.is_constant)
    {
        std::ostringstream bits_text;
        bits_text << "li t6, " << float_bits(static_cast<float>(std::strtod(operand.value.c_str(), 0)));
        out.inst(bits_text.str());
        out.inst(std::string("fmv.w.x ") + freg + ", t6");
        return;
    }

    if (operand.is_global)
    {
        out.inst(std::string("lla t6, ") + operand.value.substr(1));
        out.inst(std::string("flw ") + freg + ", 0(t6)");
        return;
    }

    if (operand.is_local)
    {
        const StackSlot slot = frame.value_slot(operand.value);
        if (fits_i12(slot.offset))
        {
            std::ostringstream text;
            text << "flw " << freg << ", " << slot.offset << "(s0)";
            out.inst(text.str());
        }
        else
        {
            emit_stack_address(out, slot.offset, "t6");
            out.inst(std::string("flw ") + freg + ", 0(t6)");
        }
        return;
    }
}

void store_float_to_slot(AsmEmitter &out, const StackSlot &slot, const char *freg)
{
    if (fits_i12(slot.offset))
    {
        std::ostringstream text;
        text << "fsw " << freg << ", " << slot.offset << "(s0)";
        out.inst(text.str());
        return;
    }
    emit_stack_address(out, slot.offset, "t6");
    out.inst(std::string("fsw ") + freg + ", 0(t6)");
}

void save_incoming_params(AsmEmitter &out, const irtext::FunctionView &fn, const Frame &frame)
{
    static const char *arg_regs[] = {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
    static const char *farg_regs[] = {"fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"};
    int int_arg_index = 0;
    int float_arg_index = 0;
    int stack_offset = 0;
    for (size_t param_index = 0; param_index < fn.params.size(); ++param_index)
    {
        std::ostringstream name;
        name << "%" << param_index;
        const StackSlot slot = frame.value_slot(name.str());
        const bool is_float = fn.params[param_index].is_float && fn.params[param_index].pointer_depth == 0;
        if (is_float)
        {
            if (float_arg_index < 8)
            {
                store_float_to_slot(out, slot, farg_regs[float_arg_index++]);
            }
            else
            {
                emit_memory_load(out, "flw", "ft0", "s0", stack_offset, "t6");
                store_float_to_slot(out, slot, "ft0");
                stack_offset += 8;
            }
            continue;
        }

        if (fn.params[param_index].pointer_depth > 0)
        {
            if (int_arg_index < 8)
            {
                store_doubleword_to_slot(out, slot, arg_regs[int_arg_index++]);
            }
            else
            {
                emit_memory_load(out, "ld", "t0", "s0", stack_offset, "t6");
                store_doubleword_to_slot(out, slot, "t0");
                stack_offset += 8;
            }
            continue;
        }

        if (int_arg_index < 8)
        {
            store_word_to_slot(out, slot, arg_regs[int_arg_index++]);
        }
        else
        {
            emit_memory_load(out, "lw", "t0", "s0", stack_offset, "t6");
            store_word_to_slot(out, slot, "t0");
            stack_offset += 8;
        }
    }
}

void emit_call(AsmEmitter &out, const irtext::InstructionView &inst, const Frame &frame)
{
    static const char *arg_regs[] = {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
    static const char *farg_regs[] = {"fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"};
    int stack_arg_count = 0;
    int int_arg_count = 0;
    int float_arg_count = 0;
    for (size_t arg_index = 0; arg_index < inst.args.size(); ++arg_index)
    {
        const irtext::Operand &arg = inst.args[arg_index];
        if (operand_is_float_like(arg))
        {
            if (float_arg_count++ >= 8)
            {
                ++stack_arg_count;
            }
        }
        else
        {
            if (int_arg_count++ >= 8)
            {
                ++stack_arg_count;
            }
        }
    }
    const int stack_bytes = align_up(stack_arg_count * 8, 16);
    if (stack_bytes > 0)
    {
        emit_add_immediate(out, "sp", "sp", -stack_bytes, "t6");
    }

    int int_arg_index = 0;
    int float_arg_index = 0;
    int stack_offset = 0;
    for (size_t arg_index = 0; arg_index < inst.args.size(); ++arg_index)
    {
        const irtext::Operand &arg = inst.args[arg_index];
        if (operand_is_float_like(arg))
        {
            if (float_arg_index < 8)
            {
                load_float_value(out, arg, frame, farg_regs[float_arg_index++]);
            }
            else
            {
                load_float_value(out, arg, frame, "ft0");
                emit_memory_store(out, "fsw", "ft0", "sp", stack_offset, "t6");
                stack_offset += 8;
            }
            continue;
        }

        if (int_arg_index < 8)
        {
            if (arg.is_constant)
            {
                out.inst(std::string("li ") + arg_regs[int_arg_index] + ", " + arg.value);
            }
            else
            {
                load_operand_value(out, arg, frame, arg_regs[int_arg_index]);
            }
            ++int_arg_index;
        }
        else
        {
            load_operand_value(out, arg, frame, "t0");
            emit_memory_store(out,
                              operand_is_pointer_like(arg) ? "sd" : "sw",
                              "t0",
                              "sp",
                              stack_offset,
                              "t6");
            stack_offset += 8;
        }
    }

    out.inst("call " + inst.callee);

    if (!inst.result.empty())
    {
        if (type_is_float_like(inst.result_type))
        {
            store_float_to_slot(out, frame.value_slot(inst.result), "fa0");
        }
        else if (type_is_pointer_like(inst.result_type))
        {
            store_doubleword_to_slot(out, frame.value_slot(inst.result), "a0");
        }
        else
        {
            store_word_to_slot(out, frame.value_slot(inst.result), "a0");
        }
    }

    if (stack_bytes > 0)
    {
        emit_add_immediate(out, "sp", "sp", stack_bytes, "t6");
    }
}

void emit_instruction(AsmEmitter &out,
                      const irtext::InstructionView &inst,
                      const Frame &frame,
                      const std::string &function_name)
{
    switch (inst.kind)
    {
    case irtext::OpKind::Alloca:
        return;
    case irtext::OpKind::Store:
        load_address(out, inst.operands[1], frame, "t1");
        if (operand_is_float_like(inst.operands[0]))
        {
            load_float_value(out, inst.operands[0], frame, "ft0");
            out.inst("fsw ft0, 0(t1)");
            return;
        }
        load_i32(out, inst.operands[0], frame, "t0");
        out.inst("sw t0, 0(t1)");
        return;
    case irtext::OpKind::Gep:
        emit_gep(out, inst, frame);
        return;
    case irtext::OpKind::Load:
        if (type_is_float_like(inst.result_type))
        {
            load_address(out, inst.operands[0], frame, "t1");
            out.inst("flw ft0, 0(t1)");
            store_float_to_slot(out, frame.value_slot(inst.result), "ft0");
            return;
        }
        load_address(out, inst.operands[0], frame, "t1");
        out.inst("lw t0, 0(t1)");
        store_i32(out, inst.result, frame, "t0");
        return;
    case irtext::OpKind::Binary:
        if (type_is_float_like(inst.result_type))
        {
            load_float_value(out, inst.operands[0], frame, "ft0");
            load_float_value(out, inst.operands[1], frame, "ft1");
            if (inst.opcode == "fadd")
            {
                out.inst("fadd.s ft2, ft0, ft1");
            }
            else if (inst.opcode == "fsub")
            {
                out.inst("fsub.s ft2, ft0, ft1");
            }
            else if (inst.opcode == "fmul")
            {
                out.inst("fmul.s ft2, ft0, ft1");
            }
            else if (inst.opcode == "fdiv")
            {
                out.inst("fdiv.s ft2, ft0, ft1");
            }
            store_float_to_slot(out, frame.value_slot(inst.result), "ft2");
            return;
        }
        load_i32(out, inst.operands[0], frame, "t0");
        load_i32(out, inst.operands[1], frame, "t1");
        if (inst.opcode == "add")
        {
            out.inst("addw t2, t0, t1");
        }
        else if (inst.opcode == "sub")
        {
            out.inst("subw t2, t0, t1");
        }
        else if (inst.opcode == "mul")
        {
            out.inst("mulw t2, t0, t1");
        }
        else if (inst.opcode == "sdiv")
        {
            out.inst("divw t2, t0, t1");
        }
        else if (inst.opcode == "srem")
        {
            out.inst("remw t2, t0, t1");
        }
        else if (inst.opcode == "and")
        {
            out.inst("and t2, t0, t1");
        }
        else if (inst.opcode == "or")
        {
            out.inst("or t2, t0, t1");
        }
        else if (inst.opcode == "xor")
        {
            out.inst("xor t2, t0, t1");
        }
        store_i32(out, inst.result, frame, "t2");
        return;
    case irtext::OpKind::Icmp:
        load_i32(out, inst.operands[0], frame, "t0");
        load_i32(out, inst.operands[1], frame, "t1");
        if (inst.opcode == "eq")
        {
            out.inst("subw t2, t0, t1");
            out.inst("seqz t2, t2");
        }
        else if (inst.opcode == "ne")
        {
            out.inst("subw t2, t0, t1");
            out.inst("snez t2, t2");
        }
        else if (inst.opcode == "slt")
        {
            out.inst("slt t2, t0, t1");
        }
        else if (inst.opcode == "sgt")
        {
            out.inst("slt t2, t1, t0");
        }
        else if (inst.opcode == "sle")
        {
            out.inst("slt t2, t1, t0");
            out.inst("xori t2, t2, 1");
        }
        else if (inst.opcode == "sge")
        {
            out.inst("slt t2, t0, t1");
            out.inst("xori t2, t2, 1");
        }
        store_i32(out, inst.result, frame, "t2");
        return;
    case irtext::OpKind::Fcmp:
        load_float_value(out, inst.operands[0], frame, "ft0");
        load_float_value(out, inst.operands[1], frame, "ft1");
        if (inst.opcode == "oeq")
        {
            out.inst("feq.s t2, ft0, ft1");
        }
        else if (inst.opcode == "one")
        {
            out.inst("feq.s t2, ft0, ft1");
            out.inst("xori t2, t2, 1");
        }
        else if (inst.opcode == "olt")
        {
            out.inst("flt.s t2, ft0, ft1");
        }
        else if (inst.opcode == "ogt")
        {
            out.inst("flt.s t2, ft1, ft0");
        }
        else if (inst.opcode == "ole")
        {
            out.inst("fle.s t2, ft0, ft1");
        }
        else if (inst.opcode == "oge")
        {
            out.inst("fle.s t2, ft1, ft0");
        }
        store_i32(out, inst.result, frame, "t2");
        return;
    case irtext::OpKind::Convert:
        if (inst.opcode == "sitofp")
        {
            load_i32(out, inst.operands[0], frame, "t0");
            out.inst("fcvt.s.w ft0, t0");
            store_float_to_slot(out, frame.value_slot(inst.result), "ft0");
            return;
        }
        if (inst.opcode == "fptosi")
        {
            load_float_value(out, inst.operands[0], frame, "ft0");
            out.inst("fcvt.w.s t0, ft0, rtz");
            store_i32(out, inst.result, frame, "t0");
            return;
        }
        load_i32(out, inst.operands[0], frame, "t0");
        store_i32(out, inst.result, frame, "t0");
        return;
    case irtext::OpKind::Call:
        emit_call(out, inst, frame);
        return;
    case irtext::OpKind::Br:
        out.inst("j " + block_label(function_name, inst.target_label));
        return;
    case irtext::OpKind::CondBr:
        load_i32(out, inst.operands[0], frame, "t0");
        out.inst("bnez t0, " + block_label(function_name, inst.true_label));
        out.inst("j " + block_label(function_name, inst.false_label));
        return;
    case irtext::OpKind::Ret:
        if (inst.operands.empty())
        {
            out.inst("j .Lreturn_" + function_name);
            return;
        }
        if (operand_is_float_like(inst.operands[0]))
        {
            load_float_value(out, inst.operands[0], frame, "fa0");
        }
        else if (inst.operands[0].is_constant)
        {
            out.inst("li a0, " + inst.operands[0].value);
        }
        else
        {
            load_i32(out, inst.operands[0], frame, "a0");
        }
        out.inst("j .Lreturn_" + function_name);
        return;
    default:
        return;
    }
}
} // namespace

Result emit_module(const ir::Module &module, const Options &options)
{
    (void)options;

    irtext::ModuleView view;
    std::string error;
    if (!irtext::parse_module(module, &view, &error))
    {
        return fail(error);
    }

    AsmEmitter out;
    emit_globals(out, view);
    for (size_t fn_index = 0; fn_index < view.functions.size(); ++fn_index)
    {
        const irtext::FunctionView &fn = view.functions[fn_index];
        const Frame frame = build_frame(fn);
        const int frame_size = frame.frame_size();
        out.line("    .text");
        out.line("    .globl " + fn.name);
        out.line(fn.name + ":");
        emit_add_immediate(out, "sp", "sp", -frame_size, "t6");
        emit_memory_store(out, "sd", "ra", "sp", frame_size - 8, "t6");
        emit_memory_store(out, "sd", "s0", "sp", frame_size - 16, "t6");
        emit_add_immediate(out, "s0", "sp", frame_size, "t6");
        save_incoming_params(out, fn, frame);
        for (size_t block_index = 0; block_index < fn.blocks.size(); ++block_index)
        {
            const irtext::BlockView &block = fn.blocks[block_index];
            out.line(block_label(fn.name, block.label) + ":");
            for (size_t inst_index = 0; inst_index < block.instructions.size(); ++inst_index)
            {
                const irtext::InstructionView &inst = block.instructions[inst_index];
                emit_instruction(out, inst, frame, fn.name);
            }
        }

        out.line(".Lreturn_" + fn.name + ":");
        emit_memory_load(out, "ld", "ra", "sp", frame_size - 8, "t6");
        emit_memory_load(out, "ld", "s0", "sp", frame_size - 16, "t6");
        emit_add_immediate(out, "sp", "sp", frame_size, "t6");
        out.inst("ret");
    }

    Result result;
    result.ok = true;
    result.assembly = out.str();
    return result;
}
} // namespace riscv
