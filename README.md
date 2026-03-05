# C-- 编译器

一个用 C++ 实现的 C-- 语言编译器，使用 flex/bison 进行词法和语法分析，支持中间表示（IR）和 MIPS 汇编代码生成。

## 项目结构

```
cmm_compiler/
├── CMakeLists.txt          # CMake 构建配置
├── Makefile                # 简易 Makefile（备用）
├── configure.sh            # 环境检测脚本
├── README.md               # 项目说明
├── .gitignore              # Git 忽略文件
│
├── include/
│   └── common.h            # 公共头文件（SourceLocation、字符工具）
│
├── src/
│   ├── cmm.l               # flex 词法规则
│   ├── cmm.y               # bison 语法定义
│   ├── main.cpp            # 主程序入口
│   │
│   ├── symtab/             # 符号表模块
│   │   ├── Symbol.h
│   │   ├── SymbolTable.h
│   │   └── SymbolTable.cpp
│   │
│   ├── ir/                 # 中间表示模块
│   │   ├── Quad.h
│   │   └── IRGenerator.h
│   │
│   └── codegen/            # 代码生成模块
│       ├── CodeGen.h
│       └── MIPSAssembler.h
│
├── examples/               # 示例 C-- 源文件
│   ├── hello.cmm
│   └── test_expr.cmm
│
└── build/                  # 构建输出目录（gitignore）
```

## 环境要求

- **g++**: 支持 C++17（g++ 7+）
- **flex**: >= 2.6
- **bison**: >= 3.0
- **CMake**: >= 3.10（可选，推荐）

### 安装依赖

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential flex bison cmake
```

**macOS:**
```bash
brew install gcc flex bison cmake
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gcc flex bison cmake make
```

## 构建步骤

### 方式一：使用 CMake（推荐）

**MSYS2 MinGW 终端:**
```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目（使用 MinGW Makefiles 生成器）
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_STANDARD=17

# 3. 编译
mingw32-make -j4

# 4. 运行
./cmm_compiler.exe ../examples/hello.cmm
```

**Windows 命令行（使用构建脚本）:**
```batch
build_msys2.bat
```

### 方式二：使用 Makefile

**MSYS2 MinGW 终端:**
```bash
make -j4
```

**Windows 命令行（使用构建脚本）:**
```batch
build_simple.bat
```

### 方式三：简易手动构建

**MSYS2 MinGW 终端:**
```bash
# 生成词法/语法分析器
flex -o src/lex.yy.c src/cmm.l
bison -d -o src/cmm.tab.c src/cmm.y

# 编译
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/main.cpp -o src/main.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/symtab/SymbolTable.cpp -o src/symtab/SymbolTable.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/lex.yy.c -o src/lex.yy.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/cmm.tab.c -o src/cmm.tab.o

# 链接
g++ -o cmm_compiler.exe src/main.o src/lex.yy.o src/cmm.tab.o src/symtab/SymbolTable.o
```

## 运行测试

```bash
# 从标准输入读取
echo "int main() { return 0; }" | ./cmm_compiler

# 从文件读取
./cmm_compiler examples/hello.cmm
```

## 开发阶段

- **Phase 1**: 环境搭建与基础框架（当前）
- **Phase 2**: 词法+语法分析（语义动作）
- **Phase 3**: 符号表+中间表示（IR 生成）
- **Phase 4**: 代码生成（MIPS 汇编）

## 示例文件

### hello.cmm
```c
int main() {
    return 0;
}
```

### test_expr.cmm
```c
int main() {
    int a = 1 + 2 * 3;
    float b = 3.14;
    return a;
}
```

## 故障排除

| 问题                                    | 原因           | 解决方案                                                      |
| --------------------------------------- | -------------- | ------------------------------------------------------------- |
| `flex: command not found`               | 未安装 flex    | 安装 flex                                                     |
| `bison: command not found`              | 未安装 bison   | 安装 bison                                                    |
| CMake 找不到 FLEX/BISON                 | 未设置路径     | `cmake .. -DFLEX_EXECUTABLE=/path/to/flex`                    |
| 链接错误 `undefined reference to yylex` | 未链接 flex 库 | 确保 `target_link_libraries(cmm_compiler PRIVATE flex bison)` |
| 编译错误：`'std::variant' not found`    | C++ 版本过低   | 升级 g++ 或使用 `-std=c++17`                                  |

详细问题请参考 [phase1.md](phase1.md) 的"风险控制与常见问题"章节。

## 许可证

MIT License