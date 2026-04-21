# Codegen 实现说明

## 职责

代码生成模块负责驱动“源文件到 LLVM IR 文件”的完整流程，当前采用 clang 辅助发射策略。

## 核心文件

- `include/Codegen.h`：`CodegenOptions` 与 `LLVMIRGenerator` 接口
- `src/Codegen.cpp`：配置读取、阶段编排、clang 调用、IR 规范化
- `include/ParserFrontend.h`、`src/ParserFrontend.cpp`：解析入口
- `include/Semantic.h`、`src/Semantic.cpp`：语义检查
- `src/parser_main.cpp`：Homework 2/3 主程序

## 调用链

1. `parser_main.cpp` 读取环境配置并构造 `LLVMIRGenerator`。
2. 调用 `generate_from_file("testfile.txt", "output.ll", options, &error)`。
3. `generate_from_file()` 按阶段执行并在任一阶段失败时返回 false。

## 关键实现

### 1) 配置来源

- `LLVMIRGenerator::from_environment()` 支持：
  - `MYCOMPILER_OPT_LEVEL=0~3`
  - `MYCOMPILER_KEEP_TEMP=1`
- `LLVM_CLANG` 在 clang 查找阶段优先使用。

### 2) clang 查找策略

按优先级查找可执行文件：

1. `LLVM_CLANG` 指定路径
2. `PATH` 中与 `lli` 同目录的 clang
3. 常见命令名：`clang`, `clang-18`, `clang-17` ...

### 3) 预处理与临时文件

- 读取源文件后执行 `strip_builtin_declarations()`，避免内置函数声明重复。
- 生成临时解析输入 `__tmp_parse_input.sy`。
- 生成临时 C 文件 `__tmp_codegen_input.c`，由 prelude + 用户源码 + main wrapper 组成。

### 4) 语法/语义前置验证

- `parse_file_to_ast()` 先确认语法可解析并产出 AST。
- `semantic::analyze_program()` 当前至少验证：程序非空、`main` 函数恰好一个。

### 5) LLVM IR 发射与规范化

- 通过系统命令调用 clang：`-S -emit-llvm -x c -O{level}`。
- 生成 `output.ll` 后，执行 `strip_host_specific_llvm_module_lines()`，移除：
  - `target datalayout = ...`
  - `target triple = ...`

## 错误处理

- 统一通过 `report_stage_error(stage, message)` 输出并回填 `error_message`。
- 主要阶段标签：`lexer`、`parser`、`translator`。
- 每个阶段均带异常捕获，避免中断时丢失诊断。

## 边界与限制

- 当前并非“手写 LLVM IRBuilder 后端”，而是“转 C + clang 发射 IR”。
- 依赖外部 clang 可执行文件与运行环境 PATH。
- 语义检查能力仍偏最小集合，复杂语义错误不在当前覆盖范围内。

## 可扩展点

- 增量替换为 AST->LLVM 原生后端（先表达式，再控制流，再函数）。
- 扩展 `semantic` 规则（符号表、类型一致性、作用域检查）。
- 将临时文件策略改为唯一命名并强化并发安全。

## 验证方式

- 运行：`build/parser`
- 输入：项目根目录 `testfile.txt`
- 输出：项目根目录 `output.ll`
- 可选：`lli output.ll` 验证 IR 可执行性（需 LLVM 工具链）
