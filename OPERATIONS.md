# 操作文档 - CMM Compiler

本文档详细说明如何构建项目、运行单元测试、进行针对性测试。

## 目录

1. [环境要求](#环境要求)
2. [项目结构](#项目结构)
3. [构建项目](#构建项目)
4. [运行单元测试](#运行单元测试)
5. [针对性测试](#针对性测试)
6. [常见问题](#常见问题)

---

## 环境要求

### 必需软件

- **CMake** 3.15 或更高版本
  - 下载：https://cmake.org/download/
  - 验证：`cmake --version`

- **MinGW-w64** (Windows) 或 **g++** (Linux/macOS)
  - 推荐：gcc/g++ 8.1.0 或更高
  - 验证：`g++ --version`

- **Flex** 2.6+ (仅语法分析器需要)
  - 下载：https://github.com/westes/flex
  - 验证：`flex --version`

- **Bison** 3.0+ (仅语法分析器需要)
  - 下载：https://www.gnu.org/software/bison/
  - 验证：`bison --version`

### Windows 用户特别说明

如果使用 MSYS2/MinGW：
1. 安装 MSYS2：https://www.msys2.org/
2. 在 MSYS2 中安装工具链：
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-flex
   pacman -S mingw-w64-x86_64-bison
   ```

---

## 项目结构

```
.
├── CMakeLists.txt          # CMake 构建配置
├── build_lexer.bat         # Homework 1 快速构建脚本（Windows）
├── build_parser.bat        # Homework 2 快速构建脚本（Windows）
├── testfile.txt            # 测试输入文件
├── README.md               # 项目说明
├── OPERATIONS.md           # 本文档
├── include/                # 头文件
│   ├── common.h           # 通用工具（SourceLocation、char_util）
│   ├── Stream.h           # 通用流模板类
│   ├── Token.h            # Token 定义
│   └── Lexer.h            # 词法分析器接口
├── src/                    # 源文件
│   ├── cmm.l              # Flex 词法分析器（Homework 2）
│   ├── cmm.y              # Bison 语法分析器（Homework 2）
│   ├── lexer_main.cpp     # Homework 1 主程序
│   ├── Lexer.cpp          # 词法分析器实现
│   ├── Token.cpp          # Token 工具实现
│   └── (生成文件)         # lex.yy.cc, cmm.tab.cc 等（构建时生成）
├── tests/                  # 单元测试
│   ├── test_stream.cpp    # Stream 类测试
│   ├── test_token.cpp     # Token 类测试
│   └── test_lexer.cpp     # Lexer 类测试
└── homework/              # 作业要求
    ├── homework1.md
    └── homework2.md
```

---

## 构建项目

### 方法 1：使用快速构建脚本（推荐 Windows 用户）

#### Homework 1（词法分析器）

```bash
# 双击运行或在命令行执行：
build_lexer.bat
```

脚本会自动：
1. 创建 `build/` 目录
2. 运行 CMake 生成 Makefile
3. 编译 `lexer` 目标
4. 用 `testfile.txt` 测试
5. 显示 `output.txt` 内容

#### Homework 2（语法分析器）

```bash
build_parser.bat
```

同样会自动完成构建、测试、显示输出。

---

### 方法 2：手动构建（跨平台）

#### 步骤 1：创建构建目录

```bash
mkdir build
cd build
```

#### 步骤 2：生成构建文件

**Windows (MinGW Makefiles):**
```bash
cmake -G "MinGW Makefiles" ..
```

**Linux/macOS:**
```bash
cmake ..
```

**Visual Studio (Windows):**
```bash
cmake -G "Visual Studio 16 2019" ..
# 或
cmake -G "Visual Studio 17 2022" ..
```

#### 步骤 3：编译

**使用 make（MinGW/MSYS2/Linux/macOS）：**
```bash
make -j4  # 使用 4 个并行任务
```

**使用 Visual Studio：**
```bash
cmake --build . --config Release
```

#### 步骤 4：可执行文件位置

- `lexer` - Homework 1 词法分析器（位于 `build/` 目录）
- `parser` - Homework 2 语法分析器（位于 `build/` 目录）
- `test_stream`, `test_token`, `test_lexer` - 单元测试

---

## 运行程序

### Homework 1：词法分析器

```bash
cd build
./lexer ../testfile.txt
```

输出写入 `output.txt`，格式：
```
CONSTTK  const
INTTK  int
IDENFR  array
...
```

### Homework 2：语法分析器

```bash
cd build
./parser ../testfile.txt
```

输出写入 `output.txt`，格式：
```
<CompUnit>
<FuncDef>
<Block>
<Stmt>
...
```

---

## 运行单元测试

### 运行所有测试

```bash
cd build
ctest --output-on-failure
```

或使用 CMake 自定义目标：
```bash
make run_all_tests
```

### 单独运行某个测试

```bash
# 直接运行可执行文件
./test_stream
./test_token
./test_lexer

# 或使用 ctest
ctest -R test_stream
ctest -R test_token
ctest -R test_lexer
```

---

## 针对性测试

### 1. 测试 Stream 类

**测试内容：**
- 从文件读取
- 从字符串读取
- `next()`、`peek()`、`unget()` 操作
- 行号/列号追踪

**运行：**
```bash
cd build
./test_stream
```

**预期输出：**
```
Running Stream tests...
[PASS] test_stream_from_file
[PASS] test_stream_from_string
[PASS] test_stream_peek_unget
All Stream tests passed!
```

**调试：**
- 查看源码：`tests/test_stream.cpp`
- 测试失败时会显示断言位置

---

### 2. 测试 Token 类

**测试内容：**
- Token 构造和属性
- `TokenUtils::is_keyword()` 保留字识别
- `TokenUtils::to_string()` 类型转换

**运行：**
```bash
cd build
./test_token
```

**预期输出：**
```
Running Token tests...
[PASS] test_token_creation
[PASS] test_tokenutils_keywords
[PASS] test_tokenutils_to_string
All Token tests passed!
```

**调试：**
- 查看 `tests/test_token.cpp`
- 修改 `TokenUtils` 后重新测试

---

### 3. 测试 Lexer 类

**测试内容：**
- 基本词法分析（`int x = 5;`）
- 多字符运算符（`>=`、`&&`、`==`）
- 字符串常量（`"Hello %d\n"`）
- 空白字符处理

**运行：**
```bash
cd build
./test_lexer
```

**预期输出：**
```
Running Lexer tests...
[PASS] test_lexer_basic
[PASS] test_lexer_operators
[PASS] test_lexer_strings
[PASS] test_lexer_whitespace
All Lexer tests passed!
```

**调试：**
- 测试文件在 `tests/test_lexer.cpp` 中创建临时文件
- 失败时会显示断言行号

---

### 4. 测试特定功能

#### 示例：测试新的运算符

修改 `tests/test_lexer.cpp`：

```cpp
void test_lexer_new_operator()
{
    create_test_file("test_new_op.txt", "a <= b >= c");
    
    Lexer lexer("test_new_op.txt");
    Token token;
    
    // a
    token = lexer.next_token();
    assert(token.type == TokenType::IDENFR);
    
    // <=
    token = lexer.next_token();
    assert(token.type == TokenType::LSS);  // 注意：<= 当前未定义
    
    // ... 继续测试
    
    delete_test_file("test_new_op.txt");
    std::cout << "[PASS] test_lexer_new_operator" << std::endl;
}
```

然后在 `main()` 中调用：
```cpp
int main()
{
    test_lexer_basic();
    test_lexer_new_operator();  // 添加这行
    // ...
}
```

重新编译并运行：
```bash
cd build
make test_lexer
./test_lexer
```

---

## 清理和重新构建

### 清理构建目录

```bash
cd build
make clean          # 删除编译结果
rm -rf *            # 完全清理（谨慎使用）
```

### 重新构建

```bash
cd build
make -j4
```

### 从零开始

```bash
rm -rf build
mkdir build
cd build
cmake ..
make -j4
```

---

## 常见问题

### 1. CMake 找不到 Flex/Bison

**错误：** `Could NOT find FLEX` 或 `Could NOT find BISON`

**解决：**
- 确保 Flex/Bison 已安装并在 PATH 中
- Windows 用户：安装 MSYS2 后，在 MSYS2 终端中运行 cmake
- 手动指定路径：
  ```bash
  cmake -DFLEX_EXECUTABLE="D:/msys64/mingw64/bin/flex.exe" \
        -DBISON_EXECUTABLE="D:/msys64/mingw64/bin/bison.exe" ..
  ```

### 2. 编译错误：找不到头文件

**错误：** `fatal error: Stream.h: No such file or directory`

**解决：**
- 确保在 `build/` 目录中运行 cmake
- 检查 `CMakeLists.txt` 中的 `target_include_directories` 设置

### 3. 测试失败

**查看详细错误：**
```bash
ctest -VV  # 显示完整输出
```

**调试方法：**
1. 查看测试源码（`tests/test_*.cpp`）
2. 在测试中添加打印语句
3. 重新编译并运行

### 4. 修改代码后重新测试

```bash
cd build
make test_lexer  # 只重新编译测试
./test_lexer
```

或重新生成所有：
```bash
cd build
make -j4
ctest --output-on-failure
```

### 5. 添加新测试

1. 在 `tests/` 目录创建 `test_mytest.cpp`
2. 包含必要头文件
3. 编写测试函数
4. 在 `CMakeLists.txt` 中添加：
   ```cmake
   add_executable(test_mytest tests/test_mytest.cpp)
   target_include_directories(test_mytest PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}/include
   )
   add_test(NAME test_mytest COMMAND test_mytest)
   ```
5. 重新运行 cmake 或直接 `make test_mytest`

---

## 性能测试

### 测试大文件

创建大测试文件：
```bash
# Linux/macOS
seq 1 10000 > big_test.txt

# Windows (PowerShell)
1..10000 | ForEach-Object { "int x$_;" } > big_test.txt
```

运行词法分析器：
```bash
time ./lexer big_test.txt
```

### 内存检查（Valgrind）

Linux/macOS 用户：
```bash
valgrind --leak-check=full ./test_lexer
```

---

## 输出控制

### Homework 1

修改 `src/lexer_main.cpp` 中的 `ENABLE_OUTPUT`：
```cpp
#ifndef ENABLE_OUTPUT
#define ENABLE_OUTPUT 0  // 0=输出到 stdout，1=输出到文件
#endif
```

### Homework 2

修改 `src/cmm.y` 中的 `ENABLE_PARSER_OUTPUT`：
```cpp
#ifndef ENABLE_PARSER_OUTPUT
#define ENABLE_PARSER_OUTPUT 0  // 0=关闭输出，1=输出到文件
#endif
```

---

## 联系与反馈

如有问题，请检查：
1. 环境变量是否正确（`PATH` 包含 g++、cmake、flex、bison）
2. 构建目录是否为 `build/`
3. 是否运行了 `cmake ..` 生成构建文件

---

**最后更新：** 2026-03-05
**版本：** 1.0
