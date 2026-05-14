# CMM Compiler

SysY2022 到 RISC-V64 的编译器实现。

当前项目覆盖：

- 词法分析
- 语法分析
- 语义检查
- 中间 LLVM IR 生成
- 自写 RISC-V64 汇编生成

## 项目结构

```text
.
├── CMakeLists.txt
├── README.md
├── include/
│   ├── Ast.h
│   ├── Codegen.h
│   ├── Ir.h
│   ├── IrBuilder.h
│   ├── IrPrinter.h
│   ├── IrText.h
│   ├── Lexer.h
│   ├── ParserFrontend.h
│   ├── RiscVBackend.h
│   ├── Semantic.h
│   ├── Stream.h
│   └── Token.h
├── src/
│   ├── lexer_main.cpp
│   ├── parser_main.cpp
│   ├── compiler_main.cpp
│   ├── Ast.cpp
│   ├── Codegen.cpp
│   ├── Ir.cpp
│   ├── IrBuilder.cpp
│   ├── IrPrinter.cpp
│   ├── IrText.cpp
│   ├── Lexer.cpp
│   ├── ParserFrontend.cpp
│   ├── RiscVBackend.cpp
│   ├── Semantic.cpp
│   ├── Token.cpp
│   └── sysy.y
├── tests/
│   ├── test_stream.cpp
│   ├── test_token.cpp
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   ├── test_ast_semantic.cpp
│   ├── test_ir_generation.cpp
│   ├── test_ir_text.cpp
│   └── test_riscv_backend.cpp
├── docs/
├── public/
├── runtime/
└── build/
```

## 构建

前置要求：

- C++17 编译器
- CMake 3.15+
- Bison 3.0+

构建命令：

```bash
mkdir build
cd build
cmake ..
cmake --build . -j4
```

Windows 下也可以直接用：

```bash
cmake --build build -j4
```

## 运行

词法分析器：

```bash
cd build
./lexer ../testfile.txt
```

解析并生成中间代码：

```bash
cd build
./parser
```

`parser` 在无参数时会读取 `build/testfile.txt`，并输出 `output.ll`。

生成 RISC-V 汇编：

```bash
cd build
./compiler -S -o testcase.s ../public/functional_easy/00_main.sy
```

## 测试

运行单元测试：

```bash
cd build
ctest --output-on-failure
```

公开集自检脚本：

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_public_ir.ps1 -StartCase 1 -EndCase 40
powershell -ExecutionPolicy Bypass -File tests/test_public_riscv.ps1
powershell -ExecutionPolicy Bypass -File tests/test_public_riscv_cases.ps1 -Cases D:\Project\mycompile\public\functional_hard\51_short_circuit3.sy
```

## 打包脚本

- `pack_ir_submission.ps1`：打包中间代码提交文件
- `pack_riscv_submission.ps1`：打包最终 RISC-V 提交文件

## 说明

- `compiler` 走项目内完整流水线，不依赖 `clang`、`llc` 或 `opt` 完成最终代码生成
- `parser` 主要用于解析与 IR 导出
- `testfile.txt` 是默认输入样例
