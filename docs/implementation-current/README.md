# 当前实现总览（混合视角）

本目录描述当前代码库中 `lexer`、`parser`、`codegen`、`tools` 的实现状态，兼顾：

- 课程评审：做了什么、如何满足作业要求、关键设计取舍。
- 开发维护：模块边界、调用链、扩展点、已知限制。

## 阅读顺序

1. `lexer.md`：词法层（字符流到 token 流）。
2. `parser.md`：语法层（token 流到 AST）。
3. `codegen.md`：后端层（AST 到 LLVM IR）。
4. `tools.md`：构建运行、环境变量、测试入口与排障。

## 编译流水线

当前 `parser` 可执行程序的主路径如下：

1. 入口：`src/parser_main.cpp`
2. 配置：`LLVMIRGenerator::from_environment()` 读取环境变量
3. 驱动：`LLVMIRGenerator::generate_from_file()`
4. 阶段：
   - 读取源文件并预处理（去除内置声明）
   - 通过 `parse_file_to_ast()` 执行词法+语法分析
   - 执行 `semantic::analyze_program()`
   - 生成临时 C，并调用 clang 发射 LLVM IR
   - 规范化 IR（去除主机相关 module 行）

## 模块边界

- 词法层：`include/Stream.h`、`include/Token.h`、`include/Lexer.h`、`src/Lexer.cpp`、`src/Token.cpp`
- 语法层：`src/sysy.y`、`include/ParserFrontend.h`、`src/ParserFrontend.cpp`
- 语义层（最小）：`include/Semantic.h`、`src/Semantic.cpp`
- 代码生成层：`include/Codegen.h`、`src/Codegen.cpp`

## 课程要求映射（当前实现）

- Homework 1（词法分析）：实现 token 识别、注释跳过、位置追踪、输出到 `output.txt`。
- Homework 2（语法分析）：基于 Bison 的 SysY 文法，`yylex` 对接自定义 `Lexer`。
- Homework 3（LLVM IR）：通过 clang 辅助链路从源代码生成 `output.ll`，并做基础规范化。

## 已知实现取向

- 当前 codegen 是“clang 辅助发射 IR”，不是手写 AST->LLVM IRBuilder 的后端。
- AST 语义检查目前聚焦于结构合法性（如 `main` 唯一性），规则较精简。
