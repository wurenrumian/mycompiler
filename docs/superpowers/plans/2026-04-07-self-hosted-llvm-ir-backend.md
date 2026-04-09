# Self-Hosted LLVM IR Backend Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the runtime `clang`-based code generation path with a self-hosted SysY semantic analysis and LLVM IR text emitter that is stable on the course judge.

**Architecture:** Keep the existing lexer and Bison grammar as the front end, add AST construction to parser reductions, then run a semantic analysis pass and a deterministic LLVM IR emitter over the AST. Prioritize judge compatibility and complete SysY coverage over polish, but keep the new backend split into focused files so later optimization work can build on it.

**Tech Stack:** C++11, existing `Lexer` + Bison parser, hand-written AST/semantic analysis, hand-written LLVM IR text emission, CTest.

---

## File Structure

- Create: `include/Ast.h`
  Responsibility: define AST node types for declarations, functions, statements, expressions, initializers, and source-level type metadata.
- Create: `include/Semantic.h`
  Responsibility: semantic model types, symbol table entries, scope stack, constant-evaluation helpers, semantic driver interface.
- Create: `include/IRBuilder.h`
  Responsibility: LLVM IR text builder utilities, register/label allocation, module/function/global emission helpers.
- Create: `src/Ast.cpp`
  Responsibility: AST helper implementations and node-construction utilities not suitable for headers.
- Create: `src/Semantic.cpp`
  Responsibility: symbol tables, scope management, function/global collection, const folding, semantic validation, lowering-ready annotations.
- Create: `src/IRBuilder.cpp`
  Responsibility: generate stable LLVM IR text for builtins, globals, functions, control flow, expressions, arrays, and initializers.
- Modify: `src/sysy.y`
  Responsibility: add `%union`/typed semantic values, construct AST nodes in grammar actions, keep parse acceptance behavior intact.
- Modify: `include/Codegen.h`
  Responsibility: replace `clang`-wrapper interface with AST-based compile pipeline interface.
- Modify: `src/Codegen.cpp`
  Responsibility: orchestrate parse -> AST -> semantic -> IR emission; remove temporary C/`clang` path entirely.
- Modify: `src/parser_main.cpp`
  Responsibility: wire parser entrypoint to the new codegen pipeline and keep `testfile.txt -> output.ll` behavior unchanged.
- Modify: `CMakeLists.txt`
  Responsibility: compile/link the new AST/semantic/IR files into `parser` and relevant tests.
- Modify: `tests/test_codegen.cpp`
  Responsibility: shift tests from “runtime clang works” to “self-hosted LLVM IR matches SysY semantics on lli”.
- Modify: `tests/test_parser.cpp`
  Responsibility: keep parser coverage green after adding typed semantic values and AST actions.

## Chunk 1: Freeze the failing backend behavior and remove `clang` dependency assumptions

### Task 1: Add regression tests that describe judge-facing requirements

**Files:**
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing tests**

Add cases that assert the new backend no longer depends on host `clang` behavior:

```cpp
cases.push_back(TestCase{"manual_ir_no_clang_dependency",
    "int main(){ return 0; }\n",
    "",
    ""});
```

Add checks that `output.ll` does not contain host-generated `ModuleID`/MSVC mangled constants as the only implementation route and still runs with `lli`.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL because current implementation still shells out to `clang` and emits host-shaped IR.

- [ ] **Step 3: Write minimal implementation scaffolding**

In `include/Codegen.h` and `src/Codegen.cpp`, introduce placeholders for:

```cpp
struct CompileResult {
    bool ok;
    std::string error;
    std::string ir;
};
```

and a pipeline entry that will later consume parser output instead of temporary C.

- [ ] **Step 4: Run test to verify scaffolding compiles**

Run: `cmake --build build --target parser test_codegen`
Expected: build succeeds, runtime tests still fail until later tasks land.

- [ ] **Step 5: Commit**

```bash
git add tests/test_codegen.cpp include/Codegen.h src/Codegen.cpp
git commit -m "test: lock codegen requirements before backend rewrite"
```

## Chunk 2: Build AST support on top of the existing parser

### Task 2: Define AST nodes that cover the full SysY grammar used by the course

**Files:**
- Create: `include/Ast.h`
- Create: `src/Ast.cpp`
- Test: `tests/test_parser.cpp`

- [ ] **Step 1: Write the failing test**

Add a parser-facing smoke test that requires building an AST for a complete program with globals, arrays, function definitions, loops, `printf`, and `getint`:

```cpp
void test_parser_builds_ast_for_full_program();
```

Expected AST roots: compilation unit with global decl list and function list sizes matching the sample input.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_parser`
Expected: FAIL because there is no AST API or parser AST output.

- [ ] **Step 3: Write minimal implementation**

Define focused node families:

```cpp
struct TypeSpec { bool is_const; std::vector<int> dims; };
struct Expr;
struct Stmt;
struct Decl;
struct FuncDef;
struct CompUnit;
```

Use enums for statement/expression kinds and simple owning pointers/vectors compatible with C++11.

- [ ] **Step 4: Run parser tests**

Run: `cmake --build build --target test_parser && ctest --output-on-failure -R test_parser`
Expected: AST-specific test still fails until grammar actions are wired; existing parser acceptance remains green.

- [ ] **Step 5: Commit**

```bash
git add include/Ast.h src/Ast.cpp tests/test_parser.cpp
git commit -m "feat: add SysY AST model"
```

### Task 3: Wire Bison actions to produce AST without changing accepted grammar

**Files:**
- Modify: `src/sysy.y`
- Modify: `tests/test_parser.cpp`
- Test: `tests/test_parser.cpp`

- [ ] **Step 1: Write the failing test**

Add assertions that parsing returns a non-null AST root for representative grammar constructs:

```cpp
assert(parsed_ast.get() != nullptr);
assert(parsed_ast->functions.size() >= 1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_parser`
Expected: FAIL because `yyparse()` does not yet expose AST state.

- [ ] **Step 3: Write minimal implementation**

In `src/sysy.y`:
- add typed semantic values for identifiers, literals, expressions, statements, declarations, initializer lists, and function pieces
- build AST nodes in reductions
- export parser-global accessors such as:

```cpp
std::unique_ptr<CompUnit> take_ast_root();
void reset_ast_root();
```

Do not remove current parse success behavior; preserve precedence and current grammar shape.

- [ ] **Step 4: Run parser tests**

Run: `cmake --build build --target test_parser && ctest --output-on-failure -R test_parser`
Expected: all parser tests pass, including new AST assertions.

- [ ] **Step 5: Commit**

```bash
git add src/sysy.y tests/test_parser.cpp include/Ast.h src/Ast.cpp
git commit -m "feat: build AST from SysY parser"
```

## Chunk 3: Add semantic analysis required by judge-facing SysY behavior

### Task 4: Implement scope and symbol management for globals, locals, params, and functions

**Files:**
- Create: `include/Semantic.h`
- Create: `src/Semantic.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add codegen/runtime tests covering:
- global/local shadowing
- const variables used in expressions
- function parameter passing
- array parameter indexing

```cpp
cases.push_back(TestCase{"shadowing", ...});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL because semantic analysis is not implemented.

- [ ] **Step 3: Write minimal implementation**

Implement:

```cpp
struct Symbol {
    std::string name;
    SymbolKind kind;
    ValueType type;
    bool is_const;
};
```

with scope stack APIs:

```cpp
void enter_scope();
void exit_scope();
bool define_symbol(const Symbol& symbol, std::string* error);
const Symbol* lookup_symbol(const std::string& name) const;
```

- [ ] **Step 4: Run targeted tests**

Run: `cmake --build build --target test_codegen && ctest --output-on-failure -R test_codegen`
Expected: semantic failures move from lookup errors toward missing IR generation.

- [ ] **Step 5: Commit**

```bash
git add include/Semantic.h src/Semantic.cpp tests/test_codegen.cpp
git commit -m "feat: add semantic symbol tables"
```

### Task 5: Implement constant evaluation and initializer flattening

**Files:**
- Modify: `src/Semantic.cpp`
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add runtime tests for:
- const expressions in array dimensions
- nested array initializers
- default zero-fill for missing initializer elements

```cpp
cases.push_back(TestCase{"array_zero_fill", ...});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL on globals/arrays/const dims.

- [ ] **Step 3: Write minimal implementation**

Add helpers:

```cpp
bool eval_const_expr(const Expr& expr, int* value, std::string* error);
std::vector<int> flatten_initializer(...);
```

Ensure SysY zero-initialization rules are honored for globals and locals.

- [ ] **Step 4: Run targeted tests**

Run: `ctest --output-on-failure -R test_codegen`
Expected: array and const-expression tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/Semantic.cpp tests/test_codegen.cpp
git commit -m "feat: evaluate const expressions and initializers"
```

## Chunk 4: Emit judge-stable LLVM IR directly

### Task 6: Build a small LLVM IR text builder for registers, blocks, and module sections

**Files:**
- Create: `include/IRBuilder.h`
- Create: `src/IRBuilder.cpp`
- Modify: `include/Codegen.h`
- Modify: `src/Codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add structural checks over `output.ll`:
- contains builtins as plain declarations/definitions we own
- contains `define i32 @main()`
- does not depend on temporary C source naming

```cpp
assert(ir.find("define i32 @main()") != std::string::npos);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL because current IR still comes from host clang.

- [ ] **Step 3: Write minimal implementation**

Implement builder APIs:

```cpp
std::string next_reg();
std::string next_label(const std::string& hint);
void emit_line(const std::string& text);
void begin_function(...);
void end_function();
```

Emit a stable runtime prelude for `getint`, `getch`, `putint`, `putch`, `putarray`, and `putf` support.

- [ ] **Step 4: Run targeted tests**

Run: `cmake --build build --target parser test_codegen && ctest --output-on-failure -R test_codegen`
Expected: IR structure tests pass, runtime still incomplete until expressions/statements are emitted.

- [ ] **Step 5: Commit**

```bash
git add include/IRBuilder.h src/IRBuilder.cpp include/Codegen.h src/Codegen.cpp tests/test_codegen.cpp
git commit -m "feat: add deterministic LLVM IR builder"
```

### Task 7: Emit expressions, lvalues, assignments, calls, and returns

**Files:**
- Modify: `src/IRBuilder.cpp`
- Modify: `src/Codegen.cpp`
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add runtime cases for:
- arithmetic precedence
- unary ops
- scalar assignment/load/store
- simple function calls and return values

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL on expression-heavy cases.

- [ ] **Step 3: Write minimal implementation**

Implement expression lowering helpers:

```cpp
ValueRef emit_expr(const Expr& expr);
AddressRef emit_lvalue_address(const Expr& lval);
```

Cover globals, locals, params, calls, `printf`, `getint`, `getch`, and `return`.

- [ ] **Step 4: Run targeted tests**

Run: `ctest --output-on-failure -R test_codegen`
Expected: arithmetic/function/call cases pass.

- [ ] **Step 5: Commit**

```bash
git add src/IRBuilder.cpp src/Codegen.cpp tests/test_codegen.cpp
git commit -m "feat: emit SysY scalar expressions and calls"
```

### Task 8: Emit control flow and short-circuit logic correctly

**Files:**
- Modify: `src/IRBuilder.cpp`
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add runtime cases for:
- `if` / `if-else`
- `while`
- `break` / `continue`
- `&&` / `||` short-circuit with side effects

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL on control-flow and short-circuit cases.

- [ ] **Step 3: Write minimal implementation**

Implement block/branch helpers:

```cpp
void emit_stmt(const Stmt& stmt);
CondRef emit_cond_branch(const Expr& cond, const std::string& true_label, const std::string& false_label);
```

Track loop context for `break`/`continue` targets.

- [ ] **Step 4: Run targeted tests**

Run: `ctest --output-on-failure -R test_codegen`
Expected: control-flow tests pass, including the short-circuit sample from `request.md`.

- [ ] **Step 5: Commit**

```bash
git add src/IRBuilder.cpp tests/test_codegen.cpp
git commit -m "feat: emit SysY control flow and short-circuit logic"
```

### Task 9: Emit arrays, pointers-to-first-element conventions, and nested initializers

**Files:**
- Modify: `src/IRBuilder.cpp`
- Modify: `src/Semantic.cpp`
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add runtime cases for:
- local/global arrays
- multidimensional arrays
- array function parameters
- nested initializer lists
- `getarray` / `putarray`

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL on array-heavy cases.

- [ ] **Step 3: Write minimal implementation**

Implement:
- LLVM aggregate types for arrays
- `getelementptr` address computation for any dimension count
- parameter lowering for `int a[]` / `int a[][N]`
- zero-filled aggregate constants and store sequences for locals

- [ ] **Step 4: Run targeted tests**

Run: `ctest --output-on-failure -R test_codegen`
Expected: array and builtin-array tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/IRBuilder.cpp src/Semantic.cpp tests/test_codegen.cpp
git commit -m "feat: emit SysY arrays and aggregate initialization"
```

## Chunk 5: Replace the old backend end-to-end and verify against course-style flow

### Task 10: Remove old `clang` backend path and wire parser main to the new pipeline

**Files:**
- Modify: `include/Codegen.h`
- Modify: `src/Codegen.cpp`
- Modify: `src/parser_main.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add an integration check that code generation works even when `LLVM_CLANG` is unset or invalid:

```cpp
clear_opt_level_env();
_putenv("LLVM_CLANG=missing-clang");
```

Expected behavior: parser still produces valid `output.ll` because it no longer shells out to `clang`.

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL because current pipeline still references `clang`.

- [ ] **Step 3: Write minimal implementation**

Delete the temporary C / `clang` execution path from `src/Codegen.cpp` and replace it with:

```cpp
bool LLVMIRGenerator::generate_from_file(...) {
    // parse
    // semantic analyze
    // emit IR text
    // write output.ll
}
```

- [ ] **Step 4: Run targeted tests**

Run: `cmake --build build --target parser test_codegen && ctest --output-on-failure -R test_codegen`
Expected: all codegen tests pass without host `clang`.

- [ ] **Step 5: Commit**

```bash
git add include/Codegen.h src/Codegen.cpp src/parser_main.cpp CMakeLists.txt tests/test_codegen.cpp
git commit -m "feat: replace clang-dependent backend with self-hosted LLVM IR generation"
```

### Task 11: Expand batch tests to course-style full-grammar coverage

**Files:**
- Modify: `tests/test_codegen.cpp`
- Modify: `tests/test_parser.cpp`
- Test: `tests/test_codegen.cpp`
- Test: `tests/test_parser.cpp`

- [ ] **Step 1: Write the failing tests**

Add a curated batch covering the grammar and judge-sensitive semantics:
- recursive functions
- nested blocks and shadowing
- empty statements
- dangling-`else`
- multiple `printf` args
- multidimensional arrays
- const dimensions
- complex logical expressions
- main return values

- [ ] **Step 2: Run tests to verify they fail where expected**

Run: `ctest --output-on-failure -R "test_parser|test_codegen"`
Expected: FAIL until missing semantics are implemented.

- [ ] **Step 3: Write minimal implementation/test data updates**

Keep tests flat and course-oriented; prefer a dozen meaningful cases over hundreds of tiny cases. Ensure every major `src/sysy.y` construct appears in at least one runtime or parser batch case.

- [ ] **Step 4: Run test suite**

Run: `ctest --output-on-failure`
Expected: all parser, lexer, token, stream, and codegen tests pass.

- [ ] **Step 5: Commit**

```bash
git add tests/test_codegen.cpp tests/test_parser.cpp
git commit -m "test: expand course-style parser and codegen coverage"
```

### Task 12: Verify course simulation flow explicitly

**Files:**
- Modify: `tests/test_codegen.cpp`
- Test: `tests/test_codegen.cpp`

- [ ] **Step 1: Write the failing test**

Add a helper that mirrors the judge contract exactly:

```cpp
write_text("testfile.txt", source);
write_text("input.txt", input);
run_command("parser.exe");
run_command("cmd /C \"lli output.ll < input.txt\"");
```

- [ ] **Step 2: Run test to verify it fails**

Run: `ctest --output-on-failure -R test_codegen`
Expected: FAIL until the codegen harness matches the contract exactly.

- [ ] **Step 3: Write minimal implementation**

Update the test harness to use file redirection for at least one smoke case in addition to pipe-based execution. Keep output normalization strict so trailing output/exit-code confusion is visible.

- [ ] **Step 4: Run final verification**

Run: `ctest --output-on-failure`
Expected: 0 failed tests.

- [ ] **Step 5: Commit**

```bash
git add tests/test_codegen.cpp
git commit -m "test: simulate judge lli redirection flow"
```
