# C-- 编译器 Phase 1: 环境搭建与基础框架实现方案

## 一、Phase 1 目标与范围

**时间节点**：第 3-4 周前完成
**核心目标**：搭建完整的编译环境，建立项目基础架构，实现最小可运行框架
**验收标准**：
- 项目结构完整，支持 flex/bison 集成
- 基础工具模块（SourceLocation、字符分类）实现并通过编译
- main.cpp 能够调用 yyparse() 执行最小词法-语法分析流程

---

## 二、项目结构设计

### 2.1 目录结构（参考 compiler-example 的工程化思路）

```
cmm_compiler/
├── CMakeLists.txt              # 构建配置（推荐，比 Makefile 更现代）
├── Makefile                   # 备用，简单构建
├── README.md                  # 项目说明
├── .gitignore
├── configure.sh              # 环境检测脚本（检测 flex/bison 版本）
│
├── src/
│   ├── cmm.l                # flex 词法规则
│   ├── cmm.y                # bison 语法定义 + 语义动作骨架
│   ├── main.cpp             # 主程序入口
│   │
│   ├── include/             # 公共头文件
│   │   └── common.h         # SourceLocation, 字符工具, 错误报告
│   │
│   ├── symtab/              # 符号表（Phase 1 只创建空骨架）
│   │   ├── Symbol.h
│   │   ├── SymbolTable.h
│   │   └── SymbolTable.cpp
│   │
│   ├── ir/                  # 中间表示（Phase 1 只创建空骨架）
│   │   ├── Quad.h
│   │   └── IRGenerator.h
│   │
│   └── codegen/             # 代码生成（Phase 1 空骨架）
│       ├── CodeGen.h
│       └── MIPSAssembler.h
│
├── tests/                    # 单元测试（Phase 1 可选，建议后续添加）
│   ├── lexer/
│   ├── parser/
│   └── common/
│
├── examples/                 # 示例 C-- 源文件
│   ├── hello.cmm
│   └── test_expr.cmm
│
└── build/                   # 构建输出（gitignore）
    ├── lexer/
    ├── parser/
    └── objects/
```

**设计原则**（借鉴 compiler-example/roadmap.md）：
- 单向依赖：上层模块（codegen）依赖下层（symtab/ir），反之不成立
- 接口隔离：头文件暴露最小必要接口，实现细节隐藏在 .cpp
- 数据传递：通过 DTO（Symbol、Quad）层间通信

---

## 三、Phase 1 详细任务清单

### 任务 1: 环境检测与构建系统配置

#### 1.1 检测工具链
```bash
# 运行 configure.sh 检测
flex -version        # 应 >= 2.6
bison -version       # 应 >= 3.0
g++ --version        # 支持 C++17
```

**configure.sh 核心逻辑**：
```bash
#!/bin/bash
# 检测 flex/bison/g++ 版本
# 版本过低则提示安装
# 生成 config.h 或设置 CMake 变量
```

#### 1.2 CMakeLists.txt 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(CMMCompiler LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 flex/bison（通过 FindFLEX.cmake / FindBISON.cmake 或 pkg-config）
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 生成词法/语法文件
FLEX_TARGET(CMMLexer src/cmm.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
BISON_TARGET(CMMParser src/cmm.y ${CMAKE_CURRENT_BINARY_DIR}/cmm.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/cmm.tab.h)

# 添加可执行文件
add_executable(cmm_compiler
    src/main.cpp
    ${FLEX_CMMLexer_OUTPUTS}
    ${BISON_CMMParser_OUTPUTS}
    # Phase 1 暂时只加 common 模块
    src/symtab/SymbolTable.cpp
)

# 包含目录
target_include_directories(cmm_compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}  # 包含生成的 cmm.tab.h
)

# 链接 flex/bison 库（某些系统需要）
target_link_libraries(cmm_compiler PRIVATE flex bison)
```

**关键点**：
- `FLEX_TARGET` 和 `BISON_TARGET` 自动处理生成规则
- `DEFINES_FILE` 生成 `cmm.tab.h`，包含 token 定义（供 lexer 使用）
- 输出目录设为 `build/`，避免污染源码

---

### 任务 2: 实现 common.h（核心工具模块）

**文件路径**：`include/common.h`

**设计参考**：compiler-example/src/main/java/common/AlphabetHelper.java + PeekIterator.java

```cpp
#pragma once

#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>

// ============================================
// SourceLocation: 源码位置追踪（用于错误报告）
// ============================================
struct SourceLocation {
    int line = 1;           // 行号（从 1 开始）
    int column = 1;         // 列号（从 1 开始）
    std::string filename;   // 文件名（可选）

    SourceLocation() = default;
    SourceLocation(int line, int column, std::string filename = "")
        : line(line), column(column), filename(std::move(filename)) {}

    // 转换为可读字符串，如 "file.cmm:10:5"
    std::string to_string() const {
        std::ostringstream oss;
        if (!filename.empty()) oss << filename << ":";
        oss << line << ":" << column;
        return oss.str();
    }
};

// ============================================
// SourceSpan: 位置范围（token 起始-结束）
// ============================================
struct SourceSpan {
    SourceLocation start;
    SourceLocation end;

    std::string to_string() const {
        if (start.filename != end.filename) {
            return start.to_string() + "-" + end.to_string();
        }
        if (start.line == end.line) {
            return start.filename + ":" + std::to_string(start.line) +
                   ":" + std::to_string(start.column) + "-" + std::to_string(end.column);
        }
        return start.to_string() + "-" + end.to_string();
    }
};

// ============================================
// char_util: 字符分类工具（constexpr 版本）
// ============================================
namespace char_util {

// 判断是否为数字字符（0-9）
constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

// 判断是否为字母（a-z, A-Z）
constexpr bool is_letter(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// 判断是否为标识符首字符（字母或下划线）
constexpr bool is_identifier_start(char c) noexcept {
    return is_letter(c) || c == '_';
}

// 判断是否为标识符后续字符（字母、数字、下划线）
constexpr bool is_identifier_part(char c) noexcept {
    return is_identifier_start(c) || is_digit(c);
}

// 判断是否为空白字符（空格、制表符、换行、回车）
constexpr bool is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// 判断是否为十六进制数字
constexpr bool is_hex_digit(char c) noexcept {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// 字符转小写（constexpr）
constexpr char to_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// 字符串转小写（返回新字符串）
inline std::string to_lower_string(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) result.push_back(to_lower(c));
    return result;
}

} // namespace char_util

// ============================================
// 错误报告辅助函数
// ============================================
// 格式化错误消息：位置 + 消息
inline std::string format_error_message(const SourceLocation& loc, const std::string& msg) {
    return loc.to_string() + ": error: " + msg;
}

// 格式化警告消息
inline std::string format_warning_message(const SourceLocation& loc, const std::string& msg) {
    return loc.to_string() + ": warning: " + msg;
}
```

**设计说明**：
- 所有工具函数均为 `constexpr` 或 `inline`，零运行时开销
- `SourceLocation` 和 `SourceSpan` 用于 bison 的 `@n` 位置追踪
- 字符工具函数供调试或语义分析使用（词法由 flex 处理）

---

### 任务 3: 实现最小化 main.cpp

**文件路径**：`src/main.cpp`

```cpp
#include <cstdio>
#include <iostream>

// flex 生成的函数
extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE* yyin;

// bison 可能需要的错误报告函数（后续扩展）
extern "C" void yyerror(const char* msg);

int main(int argc, char** argv) {
    // 1. 设置输入文件
    if (argc > 1) {
        yyin = std::fopen(argv[1], "r");
        if (!yyin) {
            std::cerr << "Error: cannot open file '" << argv[1] << "'" << std::endl;
            return 1;
        }
    } else {
        yyin = stdin;  // 从标准输入读取
    }

    // 2. 调用 bison 生成的解析器
    std::cout << "Starting parser..." << std::endl;
    int result = yyparse();

    // 3. 输出结果
    if (result == 0) {
        std::cout << "Parsing completed successfully." << std::endl;
    } else {
        std::cerr << "Parsing failed with error code " << result << std::endl;
    }

    // 4. 清理资源
    if (argc > 1) std::fclose(yyin);

    return result;
}

// 最小错误处理（后续扩展为带位置信息的版本）
extern "C" void yyerror(const char* msg) {
    std::cerr << "Parse error: " << msg << std::endl;
}
```

**关键点**：
- `extern "C"` 确保链接到 flex/bison 生成的 C 函数
- 支持文件输入或标准输入
- 错误处理留空，Phase 2 再完善

---

### 任务 4: 编写 cmm.l（词法规则骨架）

**文件路径**：`src/cmm.l`

**目标**：实现最小词法分析器，识别基本 token 类型

```lex
%{
/* C++ 头文件 */
#include <cstdio>
#include <cstdlib>
#include <string>
#include "common.h"  // 工具函数

// 传递给 bison 的 yylval 类型（使用 %union 定义）
// 这里先声明，具体定义在 cmm.y 中
typedef union YYSTYPE YYSTYPE;
extern YYSTYPE yylval;

// 行号追踪（flex 内置变量 yylineno）
// 如果需要列号，需要手动维护
%}

/* 定义正则表达式宏 */
DIGIT       [0-9]
LETTER      [a-zA-Z]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*
INTEGER     {DIGIT}+
FLOAT       {DIGIT}+"."{DIGIT}+([eE][+-]?{DIGIT}+)?
WHITESPACE  [ \t\r\n]

%%

"/*"            { /* 多行注释开始 - Phase 2 实现 */ }
"//".*          { /* 单行注释 - Phase 2 实现 */ }

"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"return"        { return RETURN; }
"int"           { return INT_TYPE; }
"float"         { return FLOAT_TYPE; }
"void"          { return VOID_TYPE; }

"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MUL; }
"/"             { return DIV; }
"%"             { return MOD; }
"=="            { return EQ; }
"!="            { return NEQ; }
"<"             { return LT; }
"<="            { return LE; }
">"             { return GT; }
">="            { return GE; }
"="             { return ASSIGN; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
";"             { return SEMICOLON; }
","             { return COMMA; }

{INTEGER}       {
                    yylval.ival = std::stoi(std::string(yytext));
                    return INTEGER_LITERAL;
                }
{FLOAT}         {
                    yylval.dval = std::stod(std::string(yytext));
                    return FLOAT_LITERAL;
                }
{IDENTIFIER}    {
                    yylval.sval = strdup(yytext);  // 需要手动释放
                    return IDENTIFIER;
                }

{WHITESPACE}    { /* 跳过空白 */ }

.               { /* 非法字符 - Phase 2 完善错误处理 */ 
                    std::cerr << "Warning: unexpected character '" << yytext[0] << "'" << std::endl;
                }

%%

/* 自定义输入函数（可选，用于从字符串而非文件读取） */
int yywrap() {
    return 1;  // 表示输入结束
}
```

**关键点**：
- 使用 `%{ %}` 包裹 C++ 代码（包含头文件、声明变量）
- `yylval` 用于传递 token 值给 bison（类型由 `%union` 定义）
- 数字字面量使用 `std::stoi`/`std::stod` 转换（注意异常处理 Phase 2 加）
- 字符串使用 `strdup` 分配内存（Phase 2 需要统一管理释放）
- 注释和错误处理暂时留空，Phase 2 完善

---

### 任务 5: 编写 cmm.y（语法定义骨架）

**文件路径**：`src/cmm.y`

**目标**：定义最小语法规则，生成 AST 或四元式骨架

```bison
%{
/* C++ 头文件 */
#include <cstdio>
#include <iostream>
#include "common.h"

// 前向声明（如果需要）
// void yyerror(const char* msg);

// 全局变量（用于错误位置追踪）
extern int yylineno;
%}

/* 定义 %union 类型（传递给 yylval 的值） */
%union {
    int ival;           // 整数字面量
    double dval;        // 浮点字面量
    char* sval;         // 标识符字符串
    // 后续 Phase 添加：ASTNode* node;  Symbol* symbol;
}

/* Token 声明（与 cmm.l 一致） */
%token IF ELSE WHILE FOR RETURN
%token INT_TYPE FLOAT_TYPE VOID_TYPE
%token PLUS MINUS MUL DIV MOD
%token EQ NEQ LT LE GT GE
%token ASSIGN AND OR NOT
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token SEMICOLON COMMA
%token <ival> INTEGER_LITERAL
%token <dval> FLOAT_LITERAL
%token <sval> IDENTIFIER

/* 优先级定义（简化版，Phase 2 完善） */
%left OR
%left AND
%left EQ NEQ
%left LT LE GT GE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT UMINUS  // 单目运算符优先级

/* 非终结符类型（如果使用 %union） */
%type <???> program
%type <???> statement
%type <???> expression

%%

/* ==================== 语法规则 ==================== */

/* 程序入口：多个函数定义或全局变量声明 */
program:
    /* 空 */ {
        // Phase 1 空实现
        std::cout << "Parsed empty program." << std::endl;
    }
    | program external_declaration {
        // Phase 2 实现：构建 AST 或生成四元式
    }
    ;

/* 外部声明：函数定义或全局变量声明 */
external_declaration:
    function_definition {
        // Phase 2 实现
    }
    | declaration {
        // Phase 2 实现
    }
    ;

/* 函数定义：类型 标识符 (参数列表) 函数体 */
function_definition:
    type_specifier IDENTIFIER LPAREN parameter_list_opt RPAREN compound_statement {
        // Phase 2 实现：构建函数 AST 节点
        // 示例：$$ = new FunctionNode($1, $2, $4, $6);
    }
    ;

/* 参数列表 */
parameter_list_opt:
    /* 空 */ { /* 无参数 */ }
    | parameter_list { /* Phase 2 实现 */ }
    ;

parameter_list:
    parameter_declaration {
        // Phase 2 实现
    }
    | parameter_list COMMA parameter_declaration {
        // Phase 2 实现
    }
    ;

parameter_declaration:
    type_specifier IDENTIFIER {
        // Phase 2 实现：创建参数符号
    }
    ;

/* 类型说明符 */
type_specifier:
    INT_TYPE { /* 返回 int 类型 */ }
    | FLOAT_TYPE { /* 返回 float 类型 */ }
    | VOID_TYPE { /* 返回 void 类型 */ }
    ;

/* 复合语句（函数体） */
compound_statement:
    LBRACE statement_list_opt RBRACE {
        // Phase 2 实现：创建块节点
    }
    ;

/* 语句列表（可选） */
statement_list_opt:
    /* 空 */ { /* 空语句列表 */ }
    | statement_list { /* Phase 2 实现 */ }
    ;

statement_list:
    statement {
        // Phase 2 实现
    }
    | statement_list statement {
        // Phase 2 实现
    }
    ;

/* 语句 */
statement:
    expression_statement {
        // Phase 2 实现
    }
    | compound_statement {
        // Phase 2 实现：嵌套块
    }
    | selection_statement {
        // Phase 2 实现：if/else
    }
    | iteration_statement {
        // Phase 2 实现：while/for
    }
    | return_statement {
        // Phase 2 实现
    }
    | declaration SEMICOLON {
        // Phase 2 实现：变量声明
    }
    ;

/* 表达式语句 */
expression_statement:
    expression_opt SEMICOLON {
        // Phase 2 实现
    }
    ;

/* 可选表达式 */
expression_opt:
    /* 空 */ { /* 空表达式 */ }
    | expression { /* Phase 2 实现 */ }
    ;

/* 表达式 */
expression:
    assignment_expression {
        // Phase 2 实现
    }
    ;

/* 赋值表达式 */
assignment_expression:
    logical_or_expression {
        // Phase 2 实现
    }
    | IDENTIFIER ASSIGN assignment_expression {
        // Phase 2 实现：赋值节点
    }
    ;

/* 逻辑或表达式 */
logical_or_expression:
    logical_and_expression {
        // Phase 2 实现
    }
    | logical_or_expression OR logical_and_expression {
        // Phase 2 实现：逻辑或节点
    }
    ;

/* 逻辑与表达式 */
logical_and_expression:
    equality_expression {
        // Phase 2 实现
    }
    | logical_and_expression AND equality_expression {
        // Phase 2 实现：逻辑与节点
    }
    ;

/* 相等表达式 */
equality_expression:
    relational_expression {
        // Phase 2 实现
    }
    | equality_expression EQ relational_expression {
        // Phase 2 实现：相等比较
    }
    | equality_expression NEQ relational_expression {
        // Phase 2 实现：不等比较
    }
    ;

/* 关系表达式 */
relational_expression:
    additive_expression {
        // Phase 2 实现
    }
    | relational_expression LT additive_expression {
        // Phase 2 实现：小于
    }
    | relational_expression LE additive_expression {
        // Phase 2 实现：小于等于
    }
    | relational_expression GT additive_expression {
        // Phase 2 实现：大于
    }
    | relational_expression GE additive_expression {
        // Phase 2 实现：大于等于
    }
    ;

/* 加减表达式 */
additive_expression:
    multiplicative_expression {
        // Phase 2 实现
    }
    | additive_expression PLUS multiplicative_expression {
        // Phase 2 实现：加法
    }
    | additive_expression MINUS multiplicative_expression {
        // Phase 2 实现：减法
    }
    ;

/* 乘除模表达式 */
multiplicative_expression:
    unary_expression {
        // Phase 2 实现
    }
    | multiplicative_expression MUL unary_expression {
        // Phase 2 实现：乘法
    }
    | multiplicative_expression DIV unary_expression {
        // Phase 2 实现：除法
    }
    | multiplicative_expression MOD unary_expression {
        // Phase 2 实现：取模
    }
    ;

/* 单目表达式 */
unary_expression:
    primary_expression {
        // Phase 2 实现
    }
    | PLUS unary_expression %prec UMINUS {
        // Phase 2 实现：正号（通常优化掉）
    }
    | MINUS unary_expression %prec UMINUS {
        // Phase 2 实现：负号
    }
    | NOT unary_expression {
        // Phase 2 实现：逻辑非
    }
    ;

/* 主表达式 */
primary_expression:
    INTEGER_LITERAL {
        // Phase 2 实现：整数字面量节点
    }
    | FLOAT_LITERAL {
        // Phase 2 实现：浮点字面量节点
    }
    | IDENTIFIER {
        // Phase 2 实现：变量引用节点
    }
    | IDENTIFIER LPAREN argument_list_opt RPAREN {
        // Phase 2 实现：函数调用节点
    }
    | LPAREN expression RPAREN {
        // Phase 2 实现：括号表达式
    }
    ;

/* 参数列表（可选） */
argument_list_opt:
    /* 空 */ { /* 无参数 */ }
    | argument_list { /* Phase 2 实现 */ }
    ;

argument_list:
    assignment_expression {
        // Phase 2 实现
    }
    | argument_list COMMA assignment_expression {
        // Phase 2 实现
    }
    ;

/* 选择语句 */
selection_statement:
    IF LPAREN expression RPAREN statement %prec IF {
        // Phase 2 实现：if 节点
    }
    | IF LPAREN expression RPAREN statement ELSE statement {
        // Phase 2 实现：if-else 节点
    }
    ;

/* 循环语句 */
iteration_statement:
    WHILE LPAREN expression RPAREN statement {
        // Phase 2 实现：while 节点
    }
    | FOR LPAREN expression_opt SEMICOLON expression_opt SEMICOLON expression_opt RPAREN statement {
        // Phase 2 实现：for 节点
    }
    ;

/* 返回语句 */
return_statement:
    RETURN expression_opt SEMICOLON {
        // Phase 2 实现：return 节点
    }
    ;

%%

/* ==================== 辅助函数 ==================== */

// 错误处理函数（Phase 2 完善位置信息）
void yyerror(const char* msg) {
    // 当前行号由 flex 维护的 yylineno 提供
    std::cerr << "Line " << yylineno << ": " << msg << std::endl;
}

// 可选：自定义输入函数（用于测试）
int yywrap() {
    return 1;
}
```

**关键点**：
- 使用 `%union` 定义语义值类型（Phase 2 填充具体类型）
- Token 声明与 cmm.l 完全一致
- 优先级定义避免歧义（如 `a + b * c`）
- 所有语义动作暂时留空，Phase 2 实现 AST 或四元式生成
- `yyerror` 使用 `yylineno` 提供行号（Phase 2 扩展列号）

---

### 任务 6: 符号表骨架（Phase 1 仅定义接口）

**文件路径**：`src/symtab/Symbol.h`

```cpp
#pragma once

#include <string>
#include <memory>
#include "common.h"

// 符号类型枚举
enum class SymbolKind {
    Variable,     // 变量
    Function,     // 函数
    Parameter,    // 函数参数
    Constant      // 常量
};

// 数据类型枚举（C-- 子集）
enum class DataType {
    Int,
    Float,
    Void,
    Unknown  // 未确定类型（语义分析阶段填充）
};

// Symbol: 符号表条目
struct Symbol {
    std::string name;           // 符号名
    SymbolKind kind;            // 符号种类
    DataType type;              // 数据类型
    int offset = 0;             // 栈偏移量（相对于帧指针）
    int size = 0;               // 大小（字节）
    bool is_global = false;     // 是否全局变量

    Symbol(std::string name, SymbolKind kind, DataType type)
        : name(std::move(name)), kind(kind), type(type) {}
};

// SymbolTable: 作用域符号表
class SymbolTable {
private:
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols_;
    SymbolTable* parent_ = nullptr;  // 父作用域（支持嵌套）

public:
    SymbolTable() = default;
    explicit SymbolTable(SymbolTable* parent) : parent_(parent) {}

    // 插入符号（返回是否成功，重复插入失败）
    bool insert(const std::shared_ptr<Symbol>& sym);

    // 查找符号（从当前作用域向上查找）
    std::shared_ptr<Symbol> lookup(const std::string& name) const;

    // 创建变量符号（自动分配偏移量）
    std::shared_ptr<Symbol> create_variable(const std::string& name, DataType type, int size);

    // 设置父作用域
    void set_parent(SymbolTable* parent) { parent_ = parent; }
    SymbolTable* parent() const { return parent_; }

    // 获取当前作用域已分配的总大小（用于计算栈偏移）
    int current_offset() const { return /* TODO */ 0; }
};
```

**文件路径**：`src/symtab/SymbolTable.cpp`

```cpp
#include "symtab/Symbol.h"
#include <unordered_map>

bool SymbolTable::insert(const std::shared_ptr<Symbol>& sym) {
    // Phase 2 实现：检查重复，计算偏移量
    return symbols_.insert({sym->name, sym}).second;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string& name) const {
    // Phase 2 实现：在当前表查找，找不到则向上查找 parent_
    auto it = symbols_.find(name);
    if (it != symbols_.end()) return it->second;
    if (parent_) return parent_->lookup(name);
    return nullptr;
}

std::shared_ptr<Symbol> SymbolTable::create_variable(const std::string& name, DataType type, int size) {
    // Phase 2 实现：创建 Symbol，计算 offset（基于 current_offset）
    auto sym = std::make_shared<Symbol>(name, SymbolKind::Variable, type);
    sym->size = size;
    // sym->offset = -current_offset() - size;  // 栈向下增长
    insert(sym);
    return sym;
}
```

**设计说明**：
- Phase 1 仅定义接口和数据结构，不实现具体逻辑
- 使用 `std::shared_ptr` 管理 Symbol 生命周期
- `parent_` 支持嵌套作用域（函数内嵌块）
- `offset` 计算留到 Phase 3（需要知道局部变量总数）

---

### 任务 7: 中间表示（IR）骨架

**文件路径**：`src/ir/Quad.h`

```cpp
#pragma once

#include <string>
#include <vector>
#include <variant>
#include "symtab/Symbol.h"

// 四元式操作符类型（简化版）
enum class QuadOp {
    // 赋值
    Assign,

    // 算术
    Add, Sub, Mul, Div, Mod,

    // 比较
    Eq, Neq, Lt, Le, Gt, Ge,

    // 控制流
    Jump,
    JumpIfTrue,
    JumpIfFalse,

    // 函数
    Call,
    Return,

    // 地址运算
    LoadAddress,
    LoadValue,
    StoreValue
};

// 操作数类型：可以是符号、立即数、临时变量
using Operand = std::variant<
    std::shared_ptr<Symbol>,  // 变量/参数
    int,                      // 整型立即数
    double,                   // 浮点立即数
    std::string               // 临时变量名（如 t1, t2）
>;

// Quad: 四元式结构
struct Quad {
    QuadOp op;
    Operand arg1;
    Operand arg2;
    Operand result;

    Quad(QuadOp op, Operand arg1 = {}, Operand arg2 = {}, Operand result = {})
        : op(op), arg1(std::move(arg1)), arg2(std::move(arg2)), result(std::move(result)) {}
};

// IR 程序：四元式列表
class IRProgram {
private:
    std::vector<Quad> quads_;

public:
    void emit(const Quad& quad) { quads_.push_back(quad); }
    const std::vector<Quad>& quads() const { return quads_; }
    size_t size() const { return quads_.size(); }

    // Phase 2 实现：标签管理（用于跳转）
    int new_label() { /* TODO */ return 0; }
    void mark_label(int label) { /* TODO */ }
    void patch_label(int label, int quad_index) { /* TODO */ }
};
```

**设计说明**：
- 使用 `std::variant` 实现类型安全的操作数（C++17 特性）
- `IRProgram` 管理四元式序列，支持标签（Phase 2 实现）
- Phase 1 仅定义数据结构，不实现生成逻辑

---

### 任务 8: 代码生成骨架

**文件路径**：`src/codegen/CodeGen.h`

```cpp
#pragma once

#include "ir/Quad.h"
#include <string>

class CodeGen {
private:
    // 输出流
    std::ostream& out_;

    // 标签映射（四元式索引 → 汇编标签）
    std::unordered_map<int, std::string> label_map_;

public:
    explicit CodeGen(std::ostream& out) : out_(out) {}

    // 生成汇编代码（主入口）
    void generate(const IRProgram& program);

    // 辅助方法（Phase 2 实现）
    void emit_prologue();
    void emit_epilogue();
    void emit_data_section();
    void emit_text_section();
    std::string quad_to_assembly(const Quad& quad);
};
```

**设计说明**：
- Phase 1 仅声明接口，Phase 4 实现具体汇编生成
- 输出到 `std::ostream` 便于重定向到文件或内存

---

## 四、构建与验证流程

### 4.1 构建步骤

```bash
# 1. 创建 build 目录
mkdir build && cd build

# 2. 配置 CMake（指定 C++ 标准）
cmake .. -DCMAKE_CXX_STANDARD=17

# 3. 编译
make -j4

# 4. 运行（测试示例）
./cmm_compiler ../examples/hello.cmm
```

### 4.2 验证 Phase 1 成功标准

**最小验证**：
1. 编译成功，无链接错误
2. 运行 `./cmm_compiler` 不崩溃（即使输入空文件）
3. 能够输出 "Parsing completed successfully."（空文法）

**扩展验证**（Phase 1 完成后）：
- 输入简单 C-- 代码（如 `int main() { return 0; }`）
- 能够通过词法分析（识别 token）
- 能够通过语法分析（不报错）

---

## 五、风险控制与常见问题

### 5.1 构建问题

| 问题                                    | 原因           | 解决方案                                                                           |
| --------------------------------------- | -------------- | ---------------------------------------------------------------------------------- |
| `flex: command not found`               | 未安装 flex    | 安装 flex（Linux: `apt install flex`; macOS: `brew install flex`; Windows: MSYS2） |
| `bison: command not found`              | 未安装 bison   | 安装 bison（同上）                                                                 |
| CMake 找不到 FLEX/BISON                 | 未设置路径     | `cmake .. -DFLEX_EXECUTABLE=/path/to/flex`                                         |
| 链接错误 `undefined reference to yylex` | 未链接 flex 库 | `target_link_libraries(cmm_compiler PRIVATE flex)`                                 |
| 编译错误：`'std::variant' not found`    | C++ 版本过低   | 升级 g++ 或使用 `-std=c++17`                                                       |

### 5.2 词法/语法问题

| 问题             | 原因                                                | 解决方案                                                       |
| ---------------- | --------------------------------------------------- | -------------------------------------------------------------- |
| 行号不准确       | flex 默认 yylineno 从 1 开始，但未在 `%{ %}` 中定义 | 在 `%{ %}` 中添加 `extern int yylineno;` 或 `%option noyywrap` |
| 注释导致解析错误 | 多行注释未正确处理（状态机缺失）                    | Phase 2 实现注释状态机，确保 `/* ... */` 正确跳过              |
| 标识符截断       | `strdup` 分配内存但未管理                           | Phase 2 统一使用 `std::string` 或内存池                        |
| 优先级冲突       | 运算符优先级未定义或错误                            | 在 cmm.y 中使用 `%left`/`%right` 明确定义                      |

### 5.3 设计风险

- **过度设计**：Phase 1 避免实现完整逻辑，保持骨架简洁
- **接口变更**：Phase 2 可能需要调整 `%union` 和语义动作，预留扩展点
- **内存管理**：Phase 1 不考虑内存释放，Phase 2 统一管理（使用智能指针）

---

## 六、Phase 1 交付物清单

- [x] 项目目录结构（CMakeLists.txt、src/、include/）
- [x] configure.sh（环境检测脚本）
- [x] README.md（项目说明、构建步骤）
- [x] common.h（SourceLocation、char_util、错误格式化）
- [x] main.cpp（最小主程序）
- [x] cmm.l（词法规则骨架，识别基本 token）
- [x] cmm.y（语法规则骨架，空语义动作）
- [x] Symbol.h / SymbolTable.h（符号表接口定义）
- [x] Quad.h（四元式数据结构定义）
- [x] CodeGen.h（代码生成接口定义）
- [x] 示例文件（examples/hello.cmm、test_expr.cmm）
- [x] 构建验证（编译通过，运行空文法成功）

---

## 七、后续 Phase 衔接

**Phase 2 词法+语法**：
- 完善 cmm.l：实现注释、字符串、错误字符处理
- 完善 cmm.y：填充语义动作，构建 AST 或直接生成四元式
- 扩展 common.h：添加 SourceSpan 追踪 token 位置

**Phase 3 符号表+IR**：
- 实现 SymbolTable.cpp：插入、查找、偏移量计算
- 实现 IRGenerator：遍历 AST 生成四元式
- 扩展 cmm.y：在语义动作中调用 IRGenerator

**Phase 4 代码生成**：
- 实现 CodeGen.cpp：四元式 → MIPS 汇编
- 添加优化 pass（常量折叠、死代码消除）

---

## 八、参考资源

- **compiler-example/roadmap.md**：工程管理最佳实践（分层架构、测试策略）
- **flex 手册**：`man flex` 或 https://westes.github.io/flex/manual/
- **bison 手册**：`man bison` 或 https://www.gnu.org/software/bison/manual/
- **CMake 文档**：https://cmake.org/documentation/
- **C-- 语言规范**：课程提供的实验文档

---

## 九、时间规划建议

| 任务                   | 预计耗时    | 截止时间        |
| ---------------------- | ----------- | --------------- |
| 环境检测 + CMake 配置  | 2 小时      | Day 1           |
| common.h + main.cpp    | 2 小时      | Day 1           |
| cmm.l 词法骨架         | 3 小时      | Day 2           |
| cmm.y 语法骨架         | 4 小时      | Day 2-3         |
| 符号表/IR/CodeGen 接口 | 3 小时      | Day 3           |
| 构建调试 + 示例编写    | 2 小时      | Day 4           |
| **总计**               | **16 小时** | **Week 1 结束** |

---

**Phase 1 核心原则**：
- 保持骨架简洁，不实现具体逻辑
- 接口设计预留扩展点（如 `%union`、Symbol 字段）
- 构建系统优先，确保后续 Phase 能快速迭代
- 参考 compiler-example 的工程化思路，但根据 C++ + flex/bison 调整

祝 Phase 1 顺利！🚀