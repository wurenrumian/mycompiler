# CMM Compiler - 词法分析与语法分析

本项目实现 CMM 语言的词法分析器（Homework 1）和语法分析器（Homework 2），基于 SysY 文法标准。

## 项目结构

```
.
├── CMakeLists.txt          # CMake 构建配置
├── Makefile                # 简易 Makefile（备用）
├── testfile.txt            # 测试输入文件（示例）
├── README.md               # 项目说明
├── OPERATIONS.md           # 详细操作文档
├── include/                # 头文件目录
│   ├── common.h           # 通用工具（SourceLocation、char_util）
│   ├── Stream.h           # 通用流模板类（Homework 1）
│   ├── Token.h            # Token 定义
│   └── Lexer.h            # 词法分析器接口
├── src/                    # 源文件目录
│   ├── cmm.l              # Flex 词法分析器（Homework 2）
│   ├── cmm.y              # Bison 语法分析器（Homework 2）
│   ├── lexer_main.cpp     # Homework 1 主程序
│   ├── Lexer.cpp          # 词法分析器实现
│   ├── parser_main.cpp    # Homework 2 主程序
│   ├── Token.cpp          # Token 工具实现
│   └── yylex.cpp          # Flex/Bison 桥接文件
├── tests/                  # 单元测试
│   ├── test_stream.cpp    # Stream 类测试
│   ├── test_token.cpp     # Token 类测试
│   ├── test_lexer.cpp     # Lexer 类测试
│   └── test_parser.cpp    # Parser 类测试
└── build/                 # 构建目录（自动生成）
    ├── lexer              # Homework 1 可执行文件
    ├── parser             # Homework 2 可执行文件
    ├── output.txt         # 程序输出
    └── (其他构建文件)
```

## 快速开始

### 方法一：使用 CMake（推荐）

**构建：**
```bash
mkdir build && cd build
cmake ..                # Windows: cmake -G "MinGW Makefiles" ..
make -j4                # 或: cmake --build .
```

**运行 Homework 1（词法分析器）：**
```bash
cd build
./lexer ../testfile.txt
```

**运行 Homework 2（语法分析器）：**
```bash
cd build
./parser ../testfile.txt
```

输出文件：`build/output.txt`

### 方法二：使用 Makefile（备用）

**构建：**
```bash
make                    # 同时构建 lexer 和 parser
make lexer              # 仅构建词法分析器
make parser             # 仅构建语法分析器
```

**运行：**（同方法一）

## 实现说明

### Homework 1 - 词法分析器

- **核心类：** [`Stream<char>`](include/Stream.h) - 通用流模板类，支持 `next()`、`peek()`、`unget()`
- **Token 定义：** [`Token`](include/Token.h) - 包含类型、原始字符串、位置信息
- **词法分析：** [`Lexer`](include/Lexer.h) - 流式处理，识别所有类别码

**特性：**
- 支持所有保留字、运算符、界符
- 整数常量、标识符、格式字符串
- 位置追踪（行号、列号）
- 可配置输出开关

### Homework 2 - 语法分析器

- **词法分析：** 基于 Flex 的 [`cmm.l`](src/cmm.l)
- **语法分析：** 基于 Bison 的 [`cmm.y`](src/cmm.y)，实现完整 SysY 文法
- **输出：** 仅输出非终结符标签（除 BlockItem、Decl、BType 外）

**文法覆盖：**
- CompUnit, Decl, BType, DeclDef
- InitVal, InitValList
- FuncDef, FuncFParams, FuncFParam
- Block, BlockItemList, BlockItem
- Stmt, Exp, AddExp, MulExp, UnaryExp, PrimaryExp
- LVal, FuncRParams, ConstExp

**关键特性：**
- 左递归实现（AddExp、MulExp 等）
- 重复结构父节点只输出一次
- 通过 `ENABLE_PARSER_OUTPUT` 控制输出

## 编译要求

- C++17 编译器（g++ 8.1.0+）
- CMake 3.15+ 或 GNU Make
- Flex 2.6+
- Bison 3.0+

## 测试

使用提供的 `testfile.txt` 进行测试，或创建自己的测试文件。

```bash
# 方法一：CMake
cd build
./lexer ../testfile.txt
./parser ../testfile.txt

# 方法二：Make
make test_lexer
make test_parser
```

查看 `output.txt` 检查结果。

## 运行单元测试

### 使用 CMake

```bash
cd build
ctest --output-on-failure    # 运行所有测试
make run_all_tests           # 或使用自定义目标
```

### 使用 Makefile

当前 Makefile 仅支持构建，单元测试请使用 CMake 方法。

```bash
make                # 构建 lexer 和 parser
make clean          # 清理构建文件
```

## 注意事项

1. **提交内容：**
   - Homework 1：词法分析器源文件（`.cpp`/`.h`）
   - Homework 2：语法分析器生成的源文件（`.cpp`/`.h`），不含 `.l`/`.y`

2. **输出控制：**
   - Homework 1：通过 `ENABLE_OUTPUT` 宏控制
   - Homework 2：通过 `ENABLE_PARSER_OUTPUT` 宏控制

3. **兼容性：** 评测环境为 gcc/g++ 8.1.0，请确保代码兼容。

4. **Windows 用户：**
   - 推荐使用 MSYS2/MinGW 环境
   - 或使用 Visual Studio 生成项目文件

## 开发日志

- 2026-03-05: 完成 Homework 1 和 Homework 2 实现
  - Homework 1: 通用流容器 + 词法分析器
  - Homework 2: Flex/Bison 语法分析器，符合 SysY 文法
