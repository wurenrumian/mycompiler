%{
/* C++ 头文件 */
#include <cstdio>
#include <iostream>
#include "common.h"

// 前向声明
void yyerror(const char* msg);
int yylex(void);

// 全局变量（用于错误位置追踪）
extern int yylineno;
%}

%define parse.error verbose
%defines

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

/* 变量声明 */
declaration:
    type_specifier IDENTIFIER SEMICOLON {
        // Phase 2 实现：简单变量声明 int a;
    }
    | type_specifier IDENTIFIER ASSIGN expression SEMICOLON {
        // Phase 2 实现：带初始化的声明 int a = 5;
    }
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
	| declaration {
        // Phase 2 实现：变量声明（declaration 已包含分号）
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

// ==================== 辅助函数 ====================
// 注意：yyerror 和 yywrap 应在主程序（main.cpp）中定义
// 这里留空，Phase 2 再实现