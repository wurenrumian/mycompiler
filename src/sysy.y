%glr-parser
%{
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Ast.h"
#include "Lexer.h"
#include "Token.h"

Lexer* lexer = nullptr;
std::ofstream output_file;
std::string parser_error_message;

void set_lexer(Lexer* l) {
    lexer = l;
    ast::reset_ast_root();
    parser_error_message.clear();
}

void reset_parser_error() {
    parser_error_message.clear();
}

std::string take_parser_error() {
    return parser_error_message;
}

void yyerror(const char *s);
int yylex();

bool enable_parser_output = true;
#define PRINT_TAG(tag) if (enable_parser_output && output_file.is_open()) output_file << "<" << tag << ">" << std::endl;

%}

%token CONSTTK INTTK VOIDTK IFTK ELSETK WHILETK BREAKTK CONTINUETK RETURNTK MAINTK GETINTTK PRINTFTK
%token GEQ EQL AND OR LEQ LSS GRE PLUS MINU MULT DIV MOD ASSIGN NOT NEQ
%token SEMICN LPARENT RPARENT LBRACK RBRACK LBRACE RBRACE COMMA
%token INTCON IDENFR STRCON

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSETK

%start CompUnit

%%

CompUnit : Unit { PRINT_TAG("CompUnit"); }
         | CompUnit Unit { PRINT_TAG("CompUnit"); }
         ;

Unit : Decl { ast::push_ast_item(ast::ItemKind::Decl, std::string()); }
     | FuncDef { ast::push_ast_item(ast::ItemKind::FuncDef, std::string()); }
     | MainFuncDef { ast::push_ast_item(ast::ItemKind::MainFuncDef, "main"); }
     ;

Decl : ConstDecl
     | VarDecl
     ;

ConstDecl : CONSTTK BType ConstDefList SEMICN { PRINT_TAG("ConstDecl"); }
          ;

ConstDefList : ConstDef
             | ConstDefList COMMA ConstDef
             ;

BType : INTTK
      ;

ConstDef : IDENFR ConstExpDims ASSIGN ConstInitVal { PRINT_TAG("ConstDef"); }
         ;

ConstExpDims : /* empty */
             | ConstExpDims LBRACK ConstExp RBRACK
             ;

ConstInitVal : ConstExp { PRINT_TAG("ConstInitVal"); }
             | LBRACE ConstInitValList RBRACE { PRINT_TAG("ConstInitVal"); }
             | LBRACE RBRACE { PRINT_TAG("ConstInitVal"); }
             ;

ConstInitValList : ConstInitVal
                 | ConstInitValList COMMA ConstInitVal
                 ;

VarDecl : BType VarDefList SEMICN { PRINT_TAG("VarDecl"); }
        ;

VarDefList : VarDef
           | VarDefList COMMA VarDef
           ;

VarDef : IDENFR ConstExpDims { PRINT_TAG("VarDef"); }
       | IDENFR ConstExpDims ASSIGN InitVal { PRINT_TAG("VarDef"); }
       ;

InitVal : Exp { PRINT_TAG("InitVal"); }
        | LBRACE InitValList RBRACE { PRINT_TAG("InitVal"); }
        | LBRACE RBRACE { PRINT_TAG("InitVal"); }
        ;

InitValList : InitVal
            | InitValList COMMA InitVal
            ;

FuncDef : FuncType IDENFR LPARENT FuncFParamsWrapper RPARENT Block { PRINT_TAG("FuncDef"); }
        ;

MainFuncDef : FuncType MAINTK LPARENT RPARENT Block { PRINT_TAG("FuncDef"); }
            ;

FuncType : VOIDTK { PRINT_TAG("FuncType"); }
         | INTTK { PRINT_TAG("FuncType"); }
         ;

FuncFParamsWrapper : /* empty */
                   | FuncFParams
                   ;

FuncFParams : FuncFParamList { PRINT_TAG("FuncFParams"); }
            ;

FuncFParamList : FuncFParam
               | FuncFParamList COMMA FuncFParam
               ;

FuncFParam : BType IDENFR { PRINT_TAG("FuncFParam"); }
           | BType IDENFR LBRACK RBRACK ExpDims { PRINT_TAG("FuncFParam"); }
           ;

Block : LBRACE BlockItemList RBRACE { PRINT_TAG("Block"); }
      ;

BlockItemList : /* empty */
              | BlockItemList BlockItem
              ;

BlockItem : Decl
          | Stmt
          ;

Stmt : LVal ASSIGN Exp SEMICN { PRINT_TAG("Stmt"); }
     | Exp SEMICN { PRINT_TAG("Stmt"); }
     | SEMICN { PRINT_TAG("Stmt"); }
     | Block { PRINT_TAG("Stmt"); }
     | IFTK LPARENT Cond RPARENT Stmt %prec LOWER_THAN_ELSE { PRINT_TAG("Stmt"); }
     | IFTK LPARENT Cond RPARENT Stmt ELSETK Stmt { PRINT_TAG("Stmt"); }
     | WHILETK LPARENT Cond RPARENT Stmt { PRINT_TAG("Stmt"); }
     | BREAKTK SEMICN { PRINT_TAG("Stmt"); }
     | CONTINUETK SEMICN { PRINT_TAG("Stmt"); }
     | RETURNTK SEMICN { PRINT_TAG("Stmt"); }
     | RETURNTK Exp SEMICN { PRINT_TAG("Stmt"); }
     | PRINTFTK LPARENT STRCON ExpListWrapper RPARENT SEMICN { PRINT_TAG("Stmt"); }
     ;

ExpListWrapper : /* empty */
               | ExpList
               ;

ExpList : COMMA Exp
        | ExpList COMMA Exp
        ;

Exp : AddExp { PRINT_TAG("Exp"); }
    ;

Cond : LOrExp { PRINT_TAG("Cond"); }
     ;

LVal : IDENFR ExpDims { PRINT_TAG("LVal"); }
     ;

ExpDims : /* empty */
        | ExpDims LBRACK Exp RBRACK
        ;

PrimaryExp : LPARENT Exp RPARENT { PRINT_TAG("PrimaryExp"); }
           | LVal { PRINT_TAG("PrimaryExp"); }
           | Number { PRINT_TAG("PrimaryExp"); }
           ;

Number : INTCON { PRINT_TAG("Number"); }
       ;

UnaryExp : PrimaryExp { PRINT_TAG("UnaryExp"); }
         | IDENFR LPARENT FuncRParamsWrapper RPARENT { PRINT_TAG("UnaryExp"); }
         | GETINTTK LPARENT RPARENT { PRINT_TAG("UnaryExp"); }
         | UnaryOp UnaryExp { PRINT_TAG("UnaryExp"); }
         ;

UnaryOp : PLUS { PRINT_TAG("UnaryOp"); }
        | MINU { PRINT_TAG("UnaryOp"); }
        | NOT { PRINT_TAG("UnaryOp"); }
        ;

FuncRParamsWrapper : /* empty */
                   | FuncRParams
                   ;

FuncRParams : FuncRParamList { PRINT_TAG("FuncRParams"); }
            ;

FuncRParamList : Exp
               | FuncRParamList COMMA Exp
               ;

MulExp : UnaryExp { PRINT_TAG("MulExp"); }
       | MulExp MULT UnaryExp { PRINT_TAG("MulExp"); }
       | MulExp DIV UnaryExp { PRINT_TAG("MulExp"); }
       | MulExp MOD UnaryExp { PRINT_TAG("MulExp"); }
       ;

AddExp : MulExp { PRINT_TAG("AddExp"); }
       | AddExp PLUS MulExp { PRINT_TAG("AddExp"); }
       | AddExp MINU MulExp { PRINT_TAG("AddExp"); }
       ;

RelExp : AddExp { PRINT_TAG("RelExp"); }
       | RelExp LSS AddExp { PRINT_TAG("RelExp"); }
       | RelExp GRE AddExp { PRINT_TAG("RelExp"); }
       | RelExp LEQ AddExp { PRINT_TAG("RelExp"); }
       | RelExp GEQ AddExp { PRINT_TAG("RelExp"); }
       ;

EqExp : RelExp { PRINT_TAG("EqExp"); }
      | EqExp EQL RelExp { PRINT_TAG("EqExp"); }
      | EqExp NEQ RelExp { PRINT_TAG("EqExp"); }
      ;

LAndExp : EqExp { PRINT_TAG("LAndExp"); }
        | LAndExp AND EqExp { PRINT_TAG("LAndExp"); }
        ;

LOrExp : LAndExp { PRINT_TAG("LOrExp"); }
       | LOrExp OR LAndExp { PRINT_TAG("LOrExp"); }
       ;

ConstExp : AddExp { PRINT_TAG("ConstExp"); }
         ;

%%

void yyerror(const char *s) {
    std::ostringstream os;
    os << "Error: " << s << " at line " << lexer->current_token().location.line
       << ", column " << lexer->current_token().location.column
       << ", token: " << lexer->current_token().lexeme;
    parser_error_message = os.str();
    std::cerr << parser_error_message << std::endl;
}

int yylex() {
    Token t = lexer->next_token();
    switch (t.type) {
        case TokenType::CONSTTK: return CONSTTK;
        case TokenType::INTTK: return INTTK;
        case TokenType::VOIDTK: return VOIDTK;
        case TokenType::IFTK: return IFTK;
        case TokenType::ELSETK: return ELSETK;
        case TokenType::WHILETK: return WHILETK;
        case TokenType::BREAKTK: return BREAKTK;
        case TokenType::CONTINUETK: return CONTINUETK;
        case TokenType::RETURNTK: return RETURNTK;
        case TokenType::MAINTK: return MAINTK;
        case TokenType::GETINTTK: return GETINTTK;
        case TokenType::PRINTFTK: return PRINTFTK;
        case TokenType::GEQ: return GEQ;
        case TokenType::EQL: return EQL;
        case TokenType::AND: return AND;
        case TokenType::OR: return OR;
        case TokenType::LEQ: return LEQ;
        case TokenType::LSS: return LSS;
        case TokenType::GRE: return GRE;
        case TokenType::PLUS: return PLUS;
        case TokenType::MINU: return MINU;
        case TokenType::MULT: return MULT;
        case TokenType::DIV: return DIV;
        case TokenType::MOD: return MOD;
        case TokenType::ASSIGN: return ASSIGN;
        case TokenType::NOT: return NOT;
        case TokenType::NEQ: return NEQ;
        case TokenType::SEMICN: return SEMICN;
        case TokenType::LPARENT: return LPARENT;
        case TokenType::RPARENT: return RPARENT;
        case TokenType::LBRACK: return LBRACK;
        case TokenType::RBRACK: return RBRACK;
        case TokenType::LBRACE: return LBRACE;
        case TokenType::RBRACE: return RBRACE;
        case TokenType::COMMA: return COMMA;
        case TokenType::INTCON: return INTCON;
        case TokenType::IDENFR: return IDENFR;
        case TokenType::STRCON: return STRCON;
        case TokenType::END_OF_FILE: return 0;
        default: return -1;
    }
}
