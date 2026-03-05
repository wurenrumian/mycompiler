%{
#include <cstdio>
#include <iostream>
#include <fstream>
#include "common.h"

void yyerror(const char* msg);
int yylex(void);

int yylineno = 1;
FILE* yyin = nullptr;

#ifndef ENABLE_PARSER_OUTPUT
#define ENABLE_PARSER_OUTPUT 1
#endif

static std::ofstream out_file;

inline void print_nt(const char* name) {
#if ENABLE_PARSER_OUTPUT
    if (out_file.is_open()) {
        out_file << "<" << name << ">" << std::endl;
    } else {
        std::cout << "<" << name << ">" << std::endl;
    }
#endif
}
%}

%define parse.error verbose
%defines

%union {
    int ival;
    double dval;
    char* sval;
}

%token IF ELSE WHILE FOR RETURN BREAK CONTINUE CONST
%token INT_TYPE FLOAT_TYPE VOID_TYPE
%token PLUS MINUS MUL DIV MOD
%token EQ NEQ LT LE GT GE
%token ASSIGN AND OR NOT
%token LPARENT RPARENT LBRACE RBRACE LBRACK RBRACK SEMICOLON COMMA
%token <ival> INTEGER_LITERAL
%token <dval> FLOAT_LITERAL
%token <sval> IDENTIFIER
%token STRCON

%left OR
%left AND
%left EQ NEQ
%left LT LE GT GE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT UMINUS

%%

/* SysY 文法规则（符合作业要求） */

/* CompUnit: 程序单元 */
CompUnit
    : CompUnit Decl { print_nt("CompUnit"); }
    | CompUnit FuncDef { print_nt("CompUnit"); }
    | Decl { print_nt("CompUnit"); }
    | FuncDef { print_nt("CompUnit"); }
    ;

/* Decl: 声明（不输出） */
Decl
    : BType DeclDef SEMICOLON { /* 不输出 */ }
    ;

/* BType: 基本类型（不输出） */
BType
    : INT_TYPE { /* 不输出 */ }
    | FLOAT_LITERAL { /* 不输出 */ }
    | VOID_TYPE { /* 不输出 */ }
    ;

/* DeclDef: 声明定义 */
DeclDef
    : IDENTIFIER { print_nt("DeclDef"); }
    | IDENTIFIER ASSIGN Exp { print_nt("DeclDef"); }
    | IDENTIFIER LBRACK ConstExp RBRACK { print_nt("DeclDef"); }
    | IDENTIFIER LBRACK ConstExp RBRACK ASSIGN LBRACE InitValList RBRACE { print_nt("DeclDef"); }
    ;

/* InitValList: 初始化值列表（辅助，不输出） */
InitValList
    : InitVal { }
    | InitValList COMMA InitVal { }
    ;

/* InitVal: 初始化值 */
InitVal
    : Exp { print_nt("InitVal"); }
    | LBRACE InitValList RBRACE { print_nt("InitVal"); }
    ;

/* FuncDef: 函数定义 */
FuncDef
    : BType IDENTIFIER LPARENT FuncFParams RPARENT Block { print_nt("FuncDef"); }
    | BType IDENTIFIER LPARENT RPARENT Block { print_nt("FuncDef"); }
    ;

/* FuncFParams: 函数形参列表 */
FuncFParams
    : FuncFParam { print_nt("FuncFParams"); }
    | FuncFParams COMMA FuncFParam { print_nt("FuncFParams"); }
    ;

/* FuncFParam: 单个形参 */
FuncFParam
    : BType IDENTIFIER { print_nt("FuncFParam"); }
    | BType IDENTIFIER LBRACK RBRACK { print_nt("FuncFParam"); }
    ;

/* Block: 块 */
Block
    : LBRACE BlockItemList RBRACE { print_nt("Block"); }
    ;

/* BlockItemList: 块项列表（辅助，不输出） */
BlockItemList
    : BlockItem { }
    | BlockItemList BlockItem { }
    ;

/* BlockItem: 块项（不输出） */
BlockItem
    : Decl { }
    | Stmt { }
    ;

/* Stmt: 语句 */
Stmt
    : LVal ASSIGN Exp SEMICOLON { print_nt("Stmt"); }
    | Exp SEMICOLON { print_nt("Stmt"); }
    | Block { print_nt("Stmt"); }
    | IF LPARENT Exp RPARENT Stmt { print_nt("Stmt"); }
    | IF LPARENT Exp RPARENT Stmt ELSE Stmt { print_nt("Stmt"); }
    | WHILE LPARENT Exp RPARENT Stmt { print_nt("Stmt"); }
    | BREAK SEMICOLON { print_nt("Stmt"); }
    | CONTINUE SEMICOLON { print_nt("Stmt"); }
    | RETURN SEMICOLON { print_nt("Stmt"); }
    | RETURN Exp SEMICOLON { print_nt("Stmt"); }
    ;

/* Exp: 表达式 */
Exp
    : AddExp { print_nt("Exp"); }
    ;

/* AddExp: 加法表达式（左递归） */
AddExp
    : MulExp { print_nt("AddExp"); }
    | AddExp PLUS MulExp { print_nt("AddExp"); }
    | AddExp MINUS MulExp { print_nt("AddExp"); }
    ;

/* MulExp: 乘除表达式（左递归） */
MulExp
    : UnaryExp { print_nt("MulExp"); }
    | MulExp MUL UnaryExp { print_nt("MulExp"); }
    | MulExp DIV UnaryExp { print_nt("MulExp"); }
    | MulExp MOD UnaryExp { print_nt("MulExp"); }
    ;

/* UnaryExp: 单目表达式 */
UnaryExp
    : PrimaryExp { print_nt("UnaryExp"); }
    | IDENTIFIER LPARENT FuncRParams RPARENT { print_nt("UnaryExp"); }
    | IDENTIFIER LPARENT RPARENT { print_nt("UnaryExp"); }
    | PLUS UnaryExp %prec UMINUS { print_nt("UnaryExp"); }
    | MINUS UnaryExp %prec UMINUS { print_nt("UnaryExp"); }
    | NOT UnaryExp { print_nt("UnaryExp"); }
    ;

/* PrimaryExp: 基本表达式 */
PrimaryExp
    : LPARENT Exp RPARENT { print_nt("PrimaryExp"); }
    | LVal { print_nt("PrimaryExp"); }
    | INTEGER_LITERAL { print_nt("PrimaryExp"); }
    | FLOAT_LITERAL { print_nt("PrimaryExp"); }
    | STRCON { print_nt("PrimaryExp"); }
    ;

/* LVal: 左值 */
LVal
    : IDENTIFIER { print_nt("LVal"); }
    | IDENTIFIER LBRACK Exp RBRACK { print_nt("LVal"); }
    ;

/* FuncRParams: 函数实参列表 */
FuncRParams
    : Exp { print_nt("FuncRParams"); }
    | FuncRParams COMMA Exp { print_nt("FuncRParams"); }
    ;

/* ConstExp: 常量表达式 */
ConstExp
    : AddExp { print_nt("ConstExp"); }
    ;

%%

void yyerror(const char* msg) {
    std::cerr << "Parse error at line " << yylineno << ": " << msg << std::endl;
}

