# SysY LLVM IR and RISC-V Compiler Design

## Context

The current project already has a hand-written lexer, a Bison parser, a small AST facade, and a `compiler -S -o <out.s> <in.sy>` entry point. The current code generation path only uses the parser as a syntax gate: it strips selected runtime declarations, prepends C declarations, writes a temporary C file, and invokes clang to emit RISC-V assembly. That approach can pass some coursework tests but does not satisfy the requirement to implement the compiler pipeline from source language through semantic analysis, LLVM IR generation, CFG/data-flow analysis, optimization, and final target code generation.

The replacement keeps the existing lexer and Bison grammar skeleton because they already match the main SysY2022 grammar shape, including declarations, functions, statements, expression precedence, arrays, and `int`/`float`. The parser actions must be upgraded from tag printing and top-level item recording to building a complete AST.

## Goals

- Remove clang/LLVM tool invocation from the compiler's own translation path.
- Preserve the existing front-end entry points and public test command shape.
- Generate valid LLVM IR for the intermediate-code coursework path: read `testfile.txt`, write `output.ll`, and support `lli` execution after runtime linking.
- Generate RISC-V64 assembly for the final-code coursework path: `compiler -S -o testcase.s testcase.sy`.
- Prioritize public tests before hidden-test completeness or performance optimization.
- Keep the design explainable as a real compiler pipeline: lexical analysis, parsing, semantic analysis, syntax-directed LLVM IR generation, CFG/data-flow analysis, optimization, and RISC-V code generation.

## Non-Goals For The First Public-Test Pass

- Do not implement a production-quality register allocator before public tests pass.
- Do not depend on external `clang`, `llc`, `opt`, or LLVM libraries during translation.
- Do not optimize aggressively before semantic correctness is stable.
- Do not rewrite the lexer/parser from scratch unless a public-test grammar issue proves it necessary.

## Architecture

The compiler pipeline is:

1. `Lexer` tokenizes SysY2022 source.
2. `sysy.y` parses tokens and constructs a full AST.
3. `Semantic` builds symbol tables, checks types/scopes, evaluates constant expressions, and annotates AST nodes with semantic types.
4. `LLVM IR` generation lowers AST into an in-memory LLVM IR model: module, types, globals, functions, basic blocks, values, and instructions.
5. `Analysis` builds CFG edges and computes simple data-flow facts such as liveness.
6. `Optimization` applies conservative LLVM IR-level passes: constant folding, unreachable block deletion, and dead temporary deletion.
7. `LLVM IR printer` emits `output.ll` for the intermediate-code assignment.
8. `RISC-V backend` lowers the same in-memory LLVM IR model to RISC-V64 assembly for the final-code assignment.

## Front End

The existing `Lexer`, `ParserFrontend`, and Bison-generated parser remain the front-end foundation. Parser actions will be changed so each grammar reduction returns typed AST nodes instead of only printing grammar tags. The AST must represent:

- Compilation units with global declarations and function definitions.
- Types: `int`, `float`, `void`, scalar values, fixed-size arrays, and array parameters.
- Declarations: const/var, scalar/array, global/local, and initializer trees.
- Statements: block, assignment, expression statement, empty statement, if/else, while, break, continue, and return.
- Expressions: integer and float literals, lvalues, function calls, unary operators, binary operators, comparisons, and short-circuit logical operators.
- Runtime output syntax: legacy `printf`/`putf` format-string statement support and normal runtime calls.

Known grammar deviations will be corrected while preserving compatibility. `main` is semantically required to be `int main()` with no parameters. Runtime names such as `getarray`, `putarray`, `getfloat`, and `putfloat` should be accepted as normal identifiers or predeclared runtime functions instead of requiring hard-coded special tokens.

## Semantic Analysis

Semantic analysis will be a separate pass over the complete AST. It will maintain nested scopes, a global function table, and predeclared SysY runtime functions:

- `getint`, `getch`, `getfloat`
- `getarray`, `getfarray`
- `putint`, `putch`, `putfloat`
- `putarray`, `putfarray`, `putf`
- `_sysy_starttime`, `_sysy_stoptime`, plus `starttime`/`stoptime` lowering support

The pass checks duplicate definitions, undefined names, const assignment, function argument count and type compatibility, array indexing, array argument decay, `break`/`continue` placement, and return type compatibility. It evaluates `ConstExp` for array dimensions and global/const initializers. It records implicit `int`/`float` conversions so IR generation and the backend can emit the right conversion instructions.

## LLVM IR Model

LLVM IR is the official intermediate representation. The project will implement its own lightweight LLVM IR object model rather than calling LLVM libraries:

- `Module`: target metadata, global variables, string constants, function declarations, function definitions.
- `Type`: void, i1, i32, float, pointer, array, and function types.
- `Value`: constants, globals, parameters, temporaries, basic block labels.
- `Function`: return type, parameters, local slot numbering, basic blocks.
- `BasicBlock`: label, ordered instructions, predecessor/successor lists.
- `Instruction`: alloca, load, store, getelementptr, binary arithmetic, integer/floating comparisons, type conversion, call, branch, conditional branch, and return.

The IR printer must emit textual LLVM IR accepted by `lli`/`llvm-link` in the coursework environment. Runtime functions are declared, not defined, so generated `output.ll` can link with the provided runtime. In intermediate-code mode, this task variant expects `main`'s return value to be visible in stdout, so IR generation will run with an `emit_main_return_value` option. With that option enabled, every `return Exp;` inside `main` lowers to: evaluate `Exp` once, call `putint` with that value, then `ret i32` with the same value. The output call should not add an implicit newline. Final-code mode disables this option, preserves normal program stdout, and uses `main`'s return value only as the process exit code.

## LLVM IR Generation

IR generation is syntax-directed from the semantically checked AST:

- Global scalar and array initializers are folded into LLVM constants or `zeroinitializer`.
- Local scalar variables use `alloca`, `store`, and `load`.
- Local arrays use `alloca` of array type and `getelementptr` for element addresses.
- Function parameters are represented as LLVM parameters, then stored into local slots for uniform access.
- Scalar expressions generate typed SSA temporaries.
- `&&` and `||` generate short-circuit control-flow blocks, not eager binary operations.
- `if`, `while`, `break`, and `continue` generate explicit basic blocks and branches.
- Format-string output is lowered either to `putf` with a string constant and arguments, or split into runtime output calls if needed for compatibility.
- Intermediate-code mode applies the `main` return-value output specialization during `return` lowering; final-code mode uses ordinary `ret` lowering.
- Type conversions use `sitofp`, `fptosi`, and appropriate compare/result conversions.

## CFG, Data Flow, And Optimization

After IR generation, each function records block successors from branch terminators and derives predecessor lists. The first required data-flow analysis is liveness over LLVM IR values; it is mainly used for documentation, diagnostics, and future backend improvements.

The first optimization set is deliberately conservative:

- Constant folding for integer and float arithmetic/comparisons when both operands are constants.
- Simplification of branches with constant conditions.
- Deletion of unreachable basic blocks.
- Deletion of unused pure temporary instructions.

These passes must not change observable runtime calls, volatile-like I/O behavior, or control-flow semantics.

## RISC-V Backend

The backend lowers the same in-memory LLVM IR model to RISC-V64 assembly. It must not call clang, llc, or another compiler. Initial code generation will use a conservative stack-slot strategy:

- Each LLVM local, temporary, and spilled parameter gets a stack slot.
- Integer operations use caller-saved integer registers such as `t0-t6` for short-lived computation.
- Float operations use `ft0-ft7` and single-precision instructions.
- Function calls follow the RISC-V lp64d ABI with integer/pointer arguments in `a0-a7` and float arguments in `fa0-fa7`, spilling extra arguments to the call stack.
- Return values use `a0` for integer/pointer and `fa0` for float.
- Global data is emitted to `.data`, `.bss`, or `.rodata`.
- Address materialization uses PC-relative forms suitable for `-mcmodel=medany`.
- `float` to `int` conversion must use round-towards-zero semantics.

The backend should produce assembly linkable with the SysY runtime library on the target platform.

## Command Modes

Two coursework modes are required:

- Intermediate code mode: `parser` with no arguments reads `testfile.txt` and writes `output.ll`. `compiler` without `-S` may share this behavior for compatibility, but the required local test path is the existing `parser` executable.
- Final code mode: `compiler -S -o <output.s> <input.sy>` writes RISC-V64 assembly.

Both modes must share the same front end, semantic analysis, LLVM IR generation, analysis, and optimization. Only the final output stage differs.

## Testing Strategy

Testing should be built around public tests and small focused unit tests:

- Parser/AST tests for representative declarations, statements, expressions, arrays, and functions.
- Semantic tests for symbols, constants, types, control-flow checks, and runtime function declarations.
- LLVM IR text tests for simple expressions, short-circuit logic, arrays, function calls, and control flow.
- `lli` integration tests for intermediate-code public cases where runtime tools are available.
- Assembly smoke tests that ensure `compiler -S -o` writes non-empty RISC-V assembly for public final-code cases.
- Targeted backend tests for ABI calls, array addressing, return values, and float/int conversion rounding.

Public-test progress should drive implementation order: parse and lower easy integer cases first, then arrays/functions/control flow, then runtime calls, then floats and performance cases.

## Risks

- The existing parser grammar is close but not exactly the SysY2022 reference. Runtime function tokenization and `main` handling need careful correction.
- Full SysY array initializer semantics are easy to get wrong. Flattening must follow row-major order and brace grouping rules.
- RISC-V ABI details can cause silent WA even when generated assembly links.
- Float support is required by SysY2022 and hidden tests, but public tests may not expose all corner cases. The backend must still avoid known incorrect rounding behavior.
