# CMM Compiler - 词法分析与语法分析

本项目实现 CMM 语言的词法分析器（Homework 1）和语法分析器（Homework 2），基于 SysY 文法标准。

## 项目结构

```
.
├── CMakeLists.txt          # 构建配置
├── testfile.txt            # 测试输入文件
├── build_lexer.bat         # Homework 1 构建脚本
├── build_parser.bat        # Homework 2 构建脚本
├── include/
│   ├── common.h           # 通用工具（SourceLocation、char_util）
│   ├── Stream.h           # 通用流模板类（Homework 1）
│   ├── Token.h            # Token 定义（Homework 1）
│   └── Lexer.h            # 词法分析器类（Homework 1）
├── src/
│   ├── cmm.l              # Flex 词法分析器（Homework 2）
│   ├── cmm.y              # Bison 语法分析器（Homework 2）
│   ├── lexer_main.cpp     # Homework 1 主程序
│   ├── Lexer.cpp          # Homework 1 词法分析器实现
│   ├── StreamBuffer.cpp   # Homework 1 流缓冲区实现
│   └── Token.cpp          # Homework 1 Token 工具实现
└── homework/
    ├── homework1.md       # Homework 1 要求
    └── homework2.md       # Homework 2 要求
```

## 快速开始

### Homework 1: 词法分析器

**构建：**
```bash
build_lexer.bat
```

**运行：**
```bash
lexer testfile.txt
```

**输出：** `output.txt`，格式为 `类别码  lexeme`

### Homework 2: 语法分析器

**构建：**
```bash
build_parser.bat
```

**运行：**
```bash
parser testfile.txt
```

**输出：** `output.txt`，包含语法成分标签（如 `<Stmt>`、`<Exp>` 等）

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
- CMake 3.15+
- Flex 2.6+
- Bison 3.0+

## 测试

使用提供的 `testfile.txt` 进行测试，或创建自己的测试文件。

```bash
# Homework 1
lexer testfile.txt

# Homework 2
parser testfile.txt
```

查看 `output.txt` 检查结果。

## 注意事项

1. **提交内容：**
   - Homework 1：词法分析器源文件（`.cpp`/`.h`）
   - Homework 2：语法分析器生成的源文件（`.cpp`/`.h`），不含 `.l`/`.y`

2. **输出控制：**
   - Homework 1：通过 `ENABLE_OUTPUT` 宏控制
   - Homework 2：通过 `ENABLE_PARSER_OUTPUT` 宏控制

3. **兼容性：** 评测环境为 gcc/g++ 8.1.0，请确保代码兼容。

## 开发日志

- 2026-03-05: 完成 Homework 1 和 Homework 2 实现
  - Homework 1: 通用流容器 + 词法分析器
  - Homework 2: Flex/Bison 语法分析器，符合 SysY 文法
