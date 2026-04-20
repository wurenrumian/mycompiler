# CMM Compiler - 词法分析、语法分析与 LLVM IR 生成

本项目实现 CMM 语言的词法分析器（Homework 1）、语法分析器（Homework 2）和 LLVM IR 生成器（Homework 3），基于 SysY 文法标准。

## 项目结构

```
.
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明
├── docs/                   # 项目文档
│   ├── README.md          # 文档索引
│   ├── requirements/      # 作业要求与文法规范
│   ├── reference/         # LLVM IR 参考资料
│   ├── implementation/    # 各组件实现文档
│   └── superpowers/       # 设计草稿与计划
├── include/                # 头文件目录
│   ├── common.h           # 通用工具（SourceLocation、char_util）
│   ├── Stream.h           # 通用流模板类
│   ├── Token.h            # Token 定义
│   └── Lexer.h            # 词法分析器接口
├── src/                    # 源文件目录
│   ├── sysy.y             # Bison 语法分析器（Homework 2）
│   ├── lexer_main.cpp     # Homework 1 主程序
│   ├── parser_main.cpp    # Homework 2 主程序
│   ├── Lexer.cpp          # 词法分析器实现
│   └── Token.cpp          # Token 工具实现
├── tests/                  # 单元测试
│   ├── test_stream.cpp    # Stream 类测试
│   ├── test_token.cpp     # Token 类测试
│   ├── test_lexer.cpp     # Lexer 类测试
│   └── test_parser.cpp    # Parser 类测试
└── build/                  # 构建目录（自动生成）
```

## 文档索引

- `docs/README.md`：文档导航入口
- `docs/requirements/sysy-grammar-2026.md`：2026 SysY 文法与补充约束
- `docs/reference/llvm-ir-primer.md`：LLVM IR 学习资料
- `docs/implementation/README.md`：组件实现说明

## 快速开始

### 构建项目

```bash
mkdir build && cd build
cmake ..                # Windows: cmake -G "MinGW Makefiles" ..
make -j4                # 或: cmake --build .
```

### 运行 Homework 1（词法分析器）

```bash
cd build
./lexer ../testfile.txt
```

### 运行 Homework 2/3（语法分析 + LLVM IR 生成）

在项目根目录准备 `testfile.txt`，然后执行：

```bash
cd build
./parser
```

输出文件：项目根目录 `output.ll`

可选优化开关（为后续优化作业预留）：

- `MYCOMPILER_OPT_LEVEL=0~3`：控制生成 IR 时传给 clang 的优化等级（默认 0）
- `MYCOMPILER_KEEP_TEMP=1`：保留中间 C 临时文件，便于调试
- `LLVM_CLANG=<clang 可执行文件>`：指定 clang 路径

## 实现说明

### Homework 1 - 词法分析器

- **核心类：** `Stream<char>` - 通用流模板类，支持 `next()`、`peek()`、`unget()`
- **Token 定义：** `Token` - 包含类型、原始字符串、位置信息
- **词法分析：** `Lexer` - 流式处理，识别所有类别码

**特性：**
- 支持所有保留字、运算符、界符
- 整数常量（十进制、十六进制）、标识符、格式字符串
- 位置追踪（行号、列号）
- 单行/多行注释跳过

### Homework 2 - 语法分析器

- **语法分析：** 基于 Bison 的 `sysy.y`，实现完整 SysY 文法
- **词法桥接：** `yylex()` 函数连接 Lexer 与 Bison
- **输出：** 仅输出非终结符标签（除 BlockItem、Decl、BType 外）

**文法覆盖：**
- CompUnit, Unit
- Decl (ConstDecl, VarDecl)
- ConstDef, VarDef, InitVal
- FuncDef, MainFuncDef, FuncType, FuncFParams, FuncFParam
- Block, BlockItemList, BlockItem
- Stmt, Exp, Cond, LVal
- AddExp, MulExp, UnaryExp, PrimaryExp, Number
- RelExp, EqExp, LAndExp, LOrExp
- ConstExp

**关键特性：**
- GLR 解析器支持（`%glr-parser`）
- 左递归实现（AddExp、MulExp 等）
- 辅助列表包装器模式（FuncFParamsList 等）
- 可配置输出开关

## 编译要求

- C++17 编译器（g++ 8.1.0+）
- CMake 3.15+
- Flex 2.6+（Homework 2）
- Bison 3.0+（Homework 2）

## 运行单元测试

```bash
cd build
ctest --output-on-failure    # 运行所有测试
```

## 注意事项

1. **输出控制：**
   - Homework 1：通过 `ENABLE_OUTPUT` 宏控制
   - Homework 2：通过 `enable_parser_output` 变量控制

2. **Windows 用户：**
   - 推荐使用 MSYS2/MinGW 环境

3. **评测环境：**
   - gcc/g++ 8.1.0，C++11 标准

## 开发日志

- 2026-03-05: 完成 Homework 1 和 Homework 2 实现
  - Homework 1: 通用流容器 + 词法分析器
  - Homework 2: Bison 语法分析器，符合 SysY 文法
