# Tools 与工程使用说明

## 职责

本文件聚焦工程层工具与使用方式，包括构建、运行、测试、环境变量和常见排障。

## 构建入口

项目使用 CMake。

常用流程：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 可执行程序

- `lexer`：Homework 1 词法分析入口（默认读 `testfile.txt`，输出 `output.txt`）
- `parser`：Homework 2/3 入口（触发 parse + semantic + codegen，输出 `output.ll`）

## 环境变量

- `MYCOMPILER_OPT_LEVEL`：0~3，控制 clang 优化等级
- `MYCOMPILER_KEEP_TEMP=1`：保留中间临时文件
- `LLVM_CLANG`：指定 clang 路径，优先级最高

## 测试入口

项目测试集中于 `tests/`：

- `test_lexer.cpp`
- `test_parser.cpp`
- `test_parser_debug.cpp`
- `test_codegen.cpp`

构建后可用：

```bash
ctest --output-on-failure
```

## 与实现模块的关系

- `lexer` 侧重词法正确性与 token 输出格式。
- `parser` 侧重语法可解析性和基本 AST 产出。
- `codegen` 侧重阶段可达性、clang 调用与 IR 产物稳定性。

## 常见排障

### 1) `parser` 运行后未生成 `output.ll`

- 检查 clang 是否可用：
  - 设置 `LLVM_CLANG`
  - 或确认 PATH 中存在可执行 clang
- 检查终端是否出现 `[codegen][translator]` 报错。

### 2) 解析失败

- 查看 `[codegen][parser]` 或 `yyerror` 输出的行列与 token。
- 先单独验证输入文件在 Homework 2 文法范围内。

### 3) 临时文件定位困难

- 设置 `MYCOMPILER_KEEP_TEMP=1`，保留：
  - `__tmp_parse_input.sy`
  - `__tmp_codegen_input.c`

## 维护建议

- 新增环境变量时，同步更新：`Codegen.h`、`Codegen.cpp`、`README` 与本文件。
- 修改运行入口参数时，保持 `lexer_main.cpp` 与 `parser_main.cpp` 的行为文档一致。
