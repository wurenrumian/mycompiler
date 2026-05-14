# RISC-V Backend Design

## Context

The project now has a hand-written lexer, a Bison parser that builds AST nodes, semantic analysis, a lightweight in-memory LLVM IR model, IR printing, and AST-to-LLVM lowering. The intermediate-code path is complete for the public `public/public` cases: `parser` reads `testfile.txt`, writes `output.ll`, and the generated IR can be linked with a runtime stub and executed by `lli`.

The remaining non-compliant part is final code generation. The current `compiler -S -o <out.s> <in.sy>` path still writes temporary C and invokes clang to emit RISC-V assembly. That must be replaced with a self-written backend that lowers the same in-memory `ir::Module` used by the intermediate-code path into RISC-V64 assembly.

## Goals

- Replace clang-based final-code generation with a self-written RISC-V64 backend.
- Keep the existing front end, semantic analyzer, and LLVM IR builder as the single source of truth.
- Make `compiler -S -o testcase.s testcase.sy` generate assembly directly from `ir::Module`.
- Support the SysY2022 final-code requirements: linkable 64-bit RISC-V assembly, Linux runtime execution, normal stdout behavior, and correct process return value.
- Design the backend comprehensively from the start, including integer, float, arrays, globals, calls, runtime calls, stack frames, ABI handling, and validation.
- Keep implementation conservative and explainable. Correctness and course compliance matter more than optimized code quality.

## Non-Goals

- Do not implement a production-quality register allocator before correctness is stable.
- Do not call `clang`, `llc`, `opt`, LLVM libraries, or another compiler from the translation path.
- Do not rewrite the parser, semantic analyzer, or IR builder unless backend work exposes a concrete missing IR feature.
- Do not optimize aggressively in the backend. Use simple IR-level passes and straightforward instruction selection.

## End-To-End Pipeline

Final-code mode uses this pipeline:

```text
SysY source
-> Lexer
-> Bison parser / AST
-> Semantic analysis
-> LLVM IR model with emit_main_return_value = false
-> Conservative IR analysis and cleanup
-> RISC-V64 backend
-> testcase.s
```

The intermediate-code path and final-code path share all stages through `ir::Module`. The only required behavior split is `main` return-value output:

- Intermediate-code mode enables `emit_main_return_value`, so `main` returns also emit visible `putint` output for the coursework IR judge.
- Final-code mode disables that option. It must not synthesize output for `main` returns. It only places the return value in `a0`.

## Backend Modules

### Public API

Create `include/RiscVBackend.h` and `src/RiscVBackend.cpp`.

```cpp
namespace riscv
{
struct Options
{
    bool emit_comments;
    bool optimize_for_size;

    Options();
};

struct Result
{
    bool ok;
    std::string message;
    std::string assembly;
};

Result emit_module(const ir::Module &module, const Options &options);
}
```

`emit_module` is the only public backend entry point. It returns an error instead of throwing for unsupported IR shapes so `compiler` can report failures cleanly.

### Assembly Emitter

`RiscVAsmEmitter` owns textual assembly formatting:

- section switches: `.text`, `.data`, `.bss`, `.rodata`
- global directives: `.globl`
- alignment directives
- labels
- instruction indentation
- optional comments

All backend lowering code writes through this emitter instead of concatenating strings directly.

### Data Lowering

`RiscVDataLowering` emits global objects before functions:

- initialized scalar `i32` globals with `.word`
- initialized scalar `float` globals with `.word <ieee754-bits>`
- zero scalar globals with `.zero 4`
- initialized arrays in row-major order
- zero arrays with `.zero <byte-count>`
- string constants in `.rodata` if the IR model grows string global support

Global symbols are emitted with stable names matching LLVM global names without the leading `@`.

### Function Lowering

`RiscVFunctionLowering` lowers one `ir::Function`:

- creates a function-specific frame layout
- emits `.text`, `.globl <name>`, and function label
- emits prologue
- emits each basic block label
- lowers each instruction
- emits one shared return label and epilogue

Every LLVM `ret` stores the return value to `a0` or `fa0`, then jumps to the shared return label.

### Frame Layout

`RiscVFrame` computes stack layout for each function before emitting instructions.

The frame contains:

- saved `ra`
- saved `s0`
- incoming parameter save slots
- stack slots for LLVM scalar temporaries
- stack slots for all `alloca` objects
- local array storage
- spill slots for intermediate address calculations when needed
- outgoing call argument area for arguments beyond ABI registers

The final frame size is rounded up to 16 bytes.

The frame pointer convention is:

```asm
addi sp, sp, -FRAME_SIZE
sd ra, FRAME_SIZE-8(sp)
sd s0, FRAME_SIZE-16(sp)
addi s0, sp, FRAME_SIZE
```

Stack slots are addressed as negative offsets from `s0`. The epilogue restores `ra`, restores `s0`, deallocates the frame, and returns.

### Value Mapping

`RiscVValueMap` maps LLVM value names to backend locations:

- immediate integer constants
- immediate float bit patterns
- stack slots
- alloca object base slots
- global symbols
- function parameters
- basic block labels

All SSA temporaries that produce values receive stack slots. This avoids a global register allocator and makes instruction emission local and predictable.

### ABI Helper

`RiscVAbi` owns call and return conventions for RV64 lp64d:

- integer and pointer parameters use `a0` through `a7`
- float parameters use `fa0` through `fa7`
- integer and pointer return values use `a0`
- float return values use `fa0`
- extra parameters are written to the outgoing call area on the stack
- the stack is 16-byte aligned at calls

The backend saves incoming parameters to their frame slots at function entry so later lowering can treat parameters like normal stack-backed values.

### Address Lowering

`RiscVAddressLowering` materializes addresses:

- local scalar stack slot address
- local array base address
- global scalar or array address
- array parameter base address
- `getelementptr` for one-dimensional and multidimensional arrays

Global addresses use:

```asm
lla t0, symbol
```

This is suitable for medany-style position-independent address materialization through assembler expansion.

Element addressing computes byte offsets from IR type information. For `i32` and `float`, element size is 4 bytes. For nested arrays, the offset multiplies each index by the row-major stride.

## Supported IR Subset

The backend is designed for the IR emitted by this project, not arbitrary LLVM IR.

Required instruction support:

- `ret void`
- `ret i32`
- `ret float`
- `br label`
- `br i1`
- `alloca`
- `load`
- `store`
- `getelementptr`
- integer binary operations: `add`, `sub`, `mul`, `sdiv`, `srem`
- float binary operations: `fadd`, `fsub`, `fmul`, `fdiv`
- integer comparisons: `eq`, `ne`, `slt`, `sgt`, `sle`, `sge`
- float comparisons: `oeq`, `one`, `olt`, `ogt`, `ole`, `oge`
- conversions: `zext i1 to i32`, `sitofp i32 to float`, `fptosi float to i32`
- direct calls to user functions
- direct calls to SysY runtime functions

Unsupported IR should produce a clear backend error. It should not silently emit invalid assembly.

## Instruction Selection

### Integers

Integer values are 32-bit SysY values held in the low 32 bits of RV64 registers.

Required lowering:

- `add i32`: `addw`
- `sub i32`: `subw`
- `mul i32`: `mulw`
- `sdiv i32`: `divw`
- `srem i32`: `remw`
- `icmp eq`: `subw` plus `seqz`
- `icmp ne`: `subw` plus `snez`
- `icmp slt`: `slt`
- `icmp sgt`: reverse operands with `slt`
- `icmp sle`: `sgt` then invert
- `icmp sge`: `slt` then invert

Loads use `lw`, stores use `sw`. Constants use `li`.

### Booleans

LLVM `i1` values are represented as integer `0` or `1` in stack slots. Conditional branches load the value into a scratch register and branch with `bnez` or `beqz`.

### Floating Point

Float values are single-precision.

Required lowering:

- `load float`: `flw`
- `store float`: `fsw`
- `fadd`: `fadd.s`
- `fsub`: `fsub.s`
- `fmul`: `fmul.s`
- `fdiv`: `fdiv.s`
- `sitofp`: `fcvt.s.w`
- `fptosi`: `fcvt.w.s <rd>, <fs>, rtz`
- `fcmp olt`: `flt.s`
- `fcmp ole`: `fle.s`
- `fcmp oeq`: `feq.s`
- `fcmp ogt`: reverse operands with `flt.s`
- `fcmp oge`: reverse operands with `fle.s`
- `fcmp one`: `feq.s` then invert

The backend must not use double-precision instructions for SysY `float`.

### Branches

Basic block labels become local assembly labels:

```asm
.Lfunc_block:
```

Unconditional branch:

```asm
j .Ltarget
```

Conditional branch:

```asm
bnez t0, .Ltrue
j .Lfalse
```

### Calls

The backend evaluates call arguments left to right into temporary stack or scratch locations, then places them into ABI registers or outgoing stack slots.

Void calls emit only `call <symbol>`. Non-void calls store `a0` or `fa0` to the destination value slot after the call.

Caller-saved scratch registers are treated as volatile across calls. Because all live LLVM values are stack-backed, no special save/restore is required for `t*`, `a*`, `ft*`, or `fa*`.

## Runtime Functions

The backend emits calls to runtime symbols exactly by name:

- `getint`
- `getch`
- `getfloat`
- `getarray`
- `getfarray`
- `putint`
- `putch`
- `putfloat`
- `putarray`
- `putfarray`
- `putf`
- `_sysy_starttime`
- `_sysy_stoptime`

`starttime()` and `stoptime()` should already be lowered by the front end or IR builder to `_sysy_starttime(line)` and `_sysy_stoptime(line)`. If the IR still contains direct `starttime` or `stoptime` calls, the backend reports an unsupported runtime symbol so the issue is fixed in the front end.

## Integration With Codegen

`include/Codegen.h` should be updated to describe a compiler pipeline rather than an LLVM/clang generator. The final-code path should:

1. parse the input file
2. run semantic analysis
3. build `ir::Module` with `emit_main_return_value = false`
4. run conservative IR passes
5. call `riscv::emit_module`
6. write assembly to the requested output path

Temporary C files, `find_clang_executable`, and `std::system` command execution must be removed from final-code generation.

The existing `parser` no-argument intermediate-code path remains intact and continues to write `output.ll`.

## IR Passes

The backend design assumes a small pass layer exists or is added before final emission:

- rebuild CFG successors and predecessors
- remove unreachable blocks
- fold constant integer and float operations
- simplify constant conditional branches
- remove unused pure temporaries when this does not affect calls, stores, branches, or returns

These passes are not responsible for target code quality. Their purpose is to keep backend input simpler and more robust.

## Testing

Testing should cover backend units and end-to-end behavior.

### Backend Unit Tests

Create `tests/test_riscv_backend.cpp` with direct IR construction tests for:

- `ret i32 0`
- integer arithmetic and comparisons
- branches and labels
- local `alloca`, `load`, and `store`
- direct function call with integer arguments
- global scalar output
- local and global array addressing through `getelementptr`
- float arithmetic and `fptosi rtz`

These tests assert key assembly substrings and structural properties, not exact full assembly text.

### End-To-End Compile Tests

Extend codegen tests to compile small SysY programs through `compiler -S -o` and assert:

- the assembly file exists
- the assembly file is non-empty
- it contains `.text` and `.globl main`
- it does not contain clang-only metadata or comments that imply external generation

### Runtime Public Tests

Use existing PowerShell scripts with WSL tools:

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_public_riscv.ps1
```

The script links generated assembly with `sylib.c` using `riscv64-linux-gnu-gcc` and runs it with `qemu-riscv64`.

### Float Probe

Keep or extend the float probe to assert:

- no `fadd.d`
- no `fsub.d`
- no `fmul.d`
- no `fdiv.d`
- no `fcvt.d.s`
- `fcvt.w.s` uses `rtz`

## Error Handling

Backend errors include:

- unsupported IR instruction
- unsupported type
- unknown value reference
- malformed call instruction
- missing basic block target
- unsupported runtime symbol
- stack frame too large for immediate offsets without materialization support

Large stack offsets should be handled by materializing the offset in a scratch register when they do not fit a 12-bit signed immediate. The backend should not assume all frames fit immediate addressing.

## Documentation Updates

Update `README.md` after implementation:

- remove `LLVM_CLANG` from final-code generation instructions
- remove wording that says optimization level is passed to clang
- document that final code generation uses the self-written RISC-V backend
- keep local validation commands for IR and RISC-V public tests

## Risks

- Current IR instructions store most semantics in raw text. The backend may need a parser for this limited IR text or an IR model extension with structured operands.
- ABI mistakes can produce assembly that links but gives wrong answers. Calls, float arguments, array pointer arguments, and return values require focused tests.
- Multidimensional `getelementptr` lowering depends on correct type and dimension information. If the IR text omits enough structure, the IR builder should be extended rather than guessing in the backend.
- Large stack frames can exceed 12-bit load/store offsets. The design must include offset materialization from the first implementation.
- Float hidden tests may expose conversion and comparison corner cases not present in public tests. The backend must use single-precision instructions and explicit round-towards-zero conversion.
