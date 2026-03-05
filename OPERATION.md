# CMMCompiler 操作手册

## 一、环境要求

### 必需工具
- **g++**: 支持 C++17 (g++ 7+)
- **flex**: >= 2.6
- **bison**: >= 3.0
- **CMake**: >= 3.10 (推荐) 或 **make**

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

---

## 二、构建步骤

### 方式一：使用 CMake（推荐）

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目
cmake .. -DCMAKE_CXX_STANDARD=17

# 3. 编译
make -j4

# 4. 运行
./cmm_compiler ../examples/hello.cmm
```

**MSYS2 MinGW 特殊配置:**
```bash
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_STANDARD=17
mingw32-make -j4
```

### 方式二：使用 Makefile（备用）

```bash
make -j4
./cmm_compiler examples/hello.cmm
```

### 方式三：手动构建

```bash
# 1. 生成词法/语法分析器
flex -o src/lex.yy.c src/cmm.l
bison -d -o src/cmm.tab.c src/cmm.y

# 2. 编译各个源文件
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/main.cpp -o src/main.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/symtab/SymbolTable.cpp -o src/symtab/SymbolTable.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/lex.yy.c -o src/lex.yy.o
g++ -std=c++17 -Wall -Wextra -Iinclude -I. -c src/cmm.tab.c -o src/cmm.tab.o

# 3. 链接
g++ -o cmm_compiler src/main.o src/lex.yy.o src/cmm.tab.o src/symtab/SymbolTable.o
```

---

## 三、运行测试

### 从文件读取
```bash
./cmm_compiler examples/hello.cmm
./cmm_compiler examples/test_expr.cmm
```

### 从标准输入读取
```bash
echo "int main() { return 0; }" | ./cmm_compiler
```

### 预期输出
```
Starting parser...
Parsing completed successfully.
```

---

## 四、目录结构说明

```
cmm_compiler/
├── CMakeLists.txt          # CMake 构建配置
├── Makefile                # 简易 Makefile（备用）
├── README.md               # 项目说明
├── configure.sh            # 环境检测脚本
├── AGENTS.md               # Agent 协作规范
├── phase1.md               # Phase 1 详细方案
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

---

## 五、故障排除

| 问题                                    | 原因           | 解决方案                                                      |
| --------------------------------------- | -------------- | ------------------------------------------------------------- |
| `flex: command not found`               | 未安装 flex    | 安装 flex                                                     |
| `bison: command not found`              | 未安装 bison   | 安装 bison                                                    |
| CMake 找不到 FLEX/BISON                 | 未设置路径     | `cmake .. -DFLEX_EXECUTABLE=/path/to/flex`                    |
| 链接错误 `undefined reference to yylex` | 未链接 flex 库 | 确保 `target_link_libraries(cmm_compiler PRIVATE flex bison)` |
| 编译错误：`'std::variant' not found`    | C++ 版本过低   | 升级 g++ 或使用 `-std=c++17`                                  |

### 详细问题排查

1. **flex/bison 版本检查:**
   ```bash
   flex --version
   bison --version
   g++ --version
   ```

2. **清理重建:**
   ```bash
   rm -rf build
   mkdir build && cd build
   cmake .. -DCMAKE_CXX_STANDARD=17
   make -j4
   ```

3. **调试构建:**
   ```bash
   cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_VERBOSE_MAKEFILE=ON
   make VERBOSE=1
   ```

---

## 六、开发阶段说明

- **Phase 1**: 环境搭建与基础框架（当前）
- **Phase 2**: 词法+语法分析（语义动作）
- **Phase 3**: 符号表+中间表示（IR 生成）
- **Phase 4**: 代码生成（MIPS 汇编）

当前项目处于 **Phase 1**，实现的是最小可行框架，能够通过编译并执行基本的词法-语法分析流程。

---

## 七、示例文件内容

### examples/hello.cmm
```c
int main() {
    return 0;
}
```

### examples/test_expr.cmm
```c
int main() {
    int a = 1 + 2 * 3;
    float b = 3.14;
    return a;
}
```

---

## 八、环境检测脚本

使用提供的 `configure.sh` 检测环境：
```bash
chmod +x configure.sh
./configure.sh
```

脚本会检查 flex/bison/g++ 版本并给出安装建议。

---

**最后更新**: 2026-03-05
**项目状态**: Phase 1 - 基础框架就绪