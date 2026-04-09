# Git Commit 变更报告

## 提交信息

- **HEAD 提交**: `c88c423` - fix: codex turns LLVM tool mismatch into a one-shot AC
- **上一次提交**: `8892233` - docs: initialize minimal superpowers structure
- **分支**: master
- **状态**: 有未提交的更改

---

## 本次提交 (c88c423) 与上一次提交 (8892233) 的区别

### 提交统计

| 指标 | 数值 |
|------|------|
| 新增文件 | 9 个 |
| 修改文件 | 7 个 |
| 新增代码行 | +1360 行 |
| 删除代码行 | -40 行 |

---

## 核心变更概要

本次提交 (`c88c423`) 是一个 **重大架构升级**，将编译器从纯语法分析输出 (`output.txt`) 转变为完整的 **LLVM IR 代码生成器** (`output.ll`)。

### 关键架构变化

1. **新增 AST 构建能力** - 在 Bison 语法分析器中集成 AST 节点构建
2. **新增语义分析模块** - 符号表管理和程序信息分析
3. **新增 LLVM IR 生成器** - 手写的 IR 文本发射器
4. **移除对 host clang 的依赖** - 改为自托管 IR 生成
5. **输出格式变更** - `output.txt` → `output.ll`

---

## 新增文件

| 文件路径 | 说明 |
|----------|------|
| `include/Ast.h` | AST 节点类型定义 (CompUnit, Item, ItemKind) |
| `include/Semantic.h` | 语义分析接口 (ProgramInfo, analyze_program) |
| `include/IRBuilder.h` | LLVM IR 构建器接口 |
| `include/ParserFrontend.h` | 语法分析前端封装 |
| `src/Ast.cpp` | AST 节点实现 |
| `src/Semantic.cpp` | 语义分析实现 |
| `src/IRBuilder.cpp` | IR 构建器实现 |
| `src/ParserFrontend.cpp` | 语法分析前端实现 |
| `pack_src.ps1` | PowerShell 打包脚本 |

---

## 修改文件

### 1. CMakeLists.txt

```diff
- add_executable(parser src/parser_main.cpp ...)
+ add_executable(parser
+     src/parser_main.cpp
+     src/Ast.cpp
+     src/ParserFrontend.cpp
+     src/Semantic.cpp
+     src/IRBuilder.cpp
+     src/Codegen.cpp
+     ...)

+ add_executable(test_codegen tests/test_codegen.cpp)
+ add_test(NAME test_codegen COMMAND test_codegen)
```

**变更内容**:
- parser 可执行文件新增 5 个源文件依赖
- 新增 `test_codegen` 测试目标
- 测试套件新增 codegen 测试

### 2. src/parser_main.cpp

```diff
- #include "Lexer.h"
- extern int yyparse();
- extern void set_lexer(Lexer *lexer);
- extern std::ofstream output_file;
- 
- int main() {
-     Lexer current_lexer(input_filename);
-     output_file.open(output_filename);
-     yyparse();
-     output_file.close();
- }

+ #include "Codegen.h"
+ int main() {
+     CodegenOptions options = LLVMIRGenerator::from_environment();
+     LLVMIRGenerator generator;
+     generator.generate_from_file(input, output, options, &error);
+ }
```

**变更内容**:
- 简化为纯代码生成调用
- 输出文件名从 `output.txt` 改为 `output.ll`
- 移除了直接的 lexer/parser 调用

### 3. src/sysy.y

```diff
+ #include "Ast.h"
+ std::string parser_error_message;
+ void set_lexer(Lexer* l) {
+     lexer = l;
+     ast::reset_ast_root();
+     parser_error_message.clear();
+ }

- Unit : Decl
-      | FuncDef
-      | MainFuncDef
+ Unit : Decl { ast::push_ast_item(ast::ItemKind::Decl, ...); }
+      | FuncDef { ast::push_ast_item(ast::ItemKind::FuncDef, ...); }
+      | MainFuncDef { ast::push_ast_item(ast::ItemKind::MainFuncDef, "main"); }
```

**变更内容**:
- 添加 AST 头文件
- 语法归约动作中构建 AST 节点
- 错误处理改为存储到 `parser_error_message`

### 4. src/Codegen.cpp (416 行新增)

核心代码生成器实现，包含:
- `LLVMIRGenerator::from_environment()` - 从环境变量读取配置
- `LLVMIRGenerator::generate_from_file()` - 文件到文件的代码生成
- 完整的 parse → AST → semantic → IR 流程

### 5. tests/test_codegen.cpp (483 行新增)

代码生成集成测试，包含多个测试用例:
- 基本函数调用
- 算术运算
- 条件分支
- 循环结构
- 数组操作

---

## 核心新增模块详解

### AST 模块 (Ast.h/Ast.cpp)

```cpp
namespace ast {
    enum class ItemKind { Decl, FuncDef, MainFuncDef };
    struct Item { ItemKind kind; std::string name; };
    struct CompUnit { std::vector<Item> items; };
    
    void reset_ast_root();
    void push_ast_item(ItemKind kind, const std::string &name);
    std::unique_ptr<CompUnit> take_ast_root();
}
```

### 语义分析模块 (Semantic.h/Semantic.cpp)

```cpp
namespace semantic {
    struct ProgramInfo {
        size_t global_item_count;
        size_t function_count;
        size_t main_function_count;
    };
    
    bool analyze_program(const ast::CompUnit &root,
                        ProgramInfo *program_info,
                        std::string *error_message);
}
```

### IR 构建模块 (IRBuilder.h/IRBuilder.cpp)

```cpp
namespace ir {
    std::string build_runtime_prelude();
    std::string build_placeholder_module(const semantic::ProgramInfo &program_info);
}
```

---

## 未提交更改 (当前工作区)

| 文件 | 状态 |
|------|------|
| `README.md` | 已修改 - 更新项目描述和运行说明 |
| `request.md` | 已修改 - 需求从语法分析改为 LLVM IR |
| `problem.md` | 已删除 |
| `docs/superpowers/plans/*.md` | 新增 - 实现计划文档 |
| `more.md` | 新增 - LLVM IR 教程 |

---

## 总结

**c88c423** 是一个里程碑式的提交，它:

1. 将编译器从**语法分析器**升级为**代码生成器**
2. 实现了**自托管的 LLVM IR 生成**，不再依赖 host clang
3. 添加了完整的 **AST 构建 → 语义分析 → IR 发射** 流程
4. 新增 **test_codegen** 集成测试框架
5. 保留了原有的 lexer 和 Bison parser 作为前端

这个提交代表了从 Homework 2 (语法分析) 到 Homework 3 (LLVM IR 生成) 的完整过渡。
