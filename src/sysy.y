%glr-parser
%{
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Ast.h"
#include "Lexer.h"
#include "Token.h"

Lexer* lexer = nullptr;
std::ofstream output_file;
std::string parser_error_message;

char *duplicate_text(const std::string &value) {
    char *copy = static_cast<char *>(std::malloc(value.size() + 1));
    if (copy == nullptr) {
        return nullptr;
    }
    std::memcpy(copy, value.c_str(), value.size() + 1);
    return copy;
}

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

%union {
    char *text;
    ast::Type *type;
    ast::Decl *decl;
    ast::VarDef *var_def;
    ast::FunctionDef *function;
    ast::Block *block;
    ast::BlockItem *block_item;
    ast::Stmt *stmt;
    ast::Expr *expr;
    ast::LValueExpr *lvalue;
    ast::InitVal *init;
    ast::Param *param;
    std::vector<ast::VarDef *> *var_defs;
    std::vector<ast::Expr *> *expr_list;
    std::vector<ast::InitVal *> *init_list;
    std::vector<ast::BlockItem *> *block_items;
    std::vector<ast::Param *> *params;
}

%token CONSTTK INTTK FLOATTK VOIDTK IFTK ELSETK WHILETK BREAKTK CONTINUETK RETURNTK MAINTK GETINTTK PRINTFTK
%token GEQ EQL AND OR LEQ LSS GRE PLUS MINU MULT DIV MOD ASSIGN NOT NEQ
%token SEMICN LPARENT RPARENT LBRACK RBRACK LBRACE RBRACE COMMA
%token <text> INTCON FLOATCON IDENFR STRCON

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSETK

%type <text> Ident
%type <type> BType FuncType
%type <decl> Decl ConstDecl VarDecl
%type <var_def> ConstDef VarDef
%type <function> FuncDef MainFuncDef
%type <block> Block
%type <block_item> BlockItem
%type <stmt> Stmt
%type <expr> Exp Cond PrimaryExp Number UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <lvalue> LVal
%type <init> ConstInitVal InitVal
%type <param> FuncFParam
%type <var_defs> ConstDefList VarDefList
%type <expr_list> ConstExpDims ExpDims ExpListWrapper ExpList FuncRParamsWrapper FuncRParams FuncRParamList
%type <init_list> ConstInitValList InitValList
%type <block_items> BlockItemList
%type <params> FuncFParamsWrapper FuncFParams FuncFParamList

%start CompUnit

%%

CompUnit : Unit { PRINT_TAG("CompUnit"); }
         | CompUnit Unit { PRINT_TAG("CompUnit"); }
         ;

Unit : Decl { ast::push_ast_node($1); }
     | FuncDef { ast::push_ast_node($1); }
     | MainFuncDef { ast::push_ast_node($1); }
     ;

Ident : IDENFR { $$ = $1; }
      | MAINTK { $$ = duplicate_text("main"); }
      ;

Decl : ConstDecl { $$ = $1; }
     | VarDecl { $$ = $1; }
     ;

ConstDecl : CONSTTK BType ConstDefList SEMICN {
                $$ = ast::make_decl(true, $2, $3);
                PRINT_TAG("ConstDecl");
            }
          ;

ConstDefList : ConstDef {
                    $$ = new std::vector<ast::VarDef *>();
                    $$->push_back($1);
               }
             | ConstDefList COMMA ConstDef {
                    $$ = $1;
                    $$->push_back($3);
               }
             ;

BType : INTTK {
            $$ = new ast::Type(ast::Type::int_type());
        }
      | FLOATTK {
            $$ = new ast::Type(ast::Type::float_type());
        }
      ;

ConstDef : Ident ConstExpDims ASSIGN ConstInitVal {
                $$ = ast::make_var_def($1, $2, $4);
                std::free($1);
                PRINT_TAG("ConstDef");
           }
         ;

ConstExpDims : /* empty */ {
                    $$ = new std::vector<ast::Expr *>();
               }
             | ConstExpDims LBRACK ConstExp RBRACK {
                    $$ = $1;
                    $$->push_back($3);
               }
             ;

ConstInitVal : ConstExp {
                    $$ = ast::make_init_expr($1);
                    PRINT_TAG("ConstInitVal");
               }
             | LBRACE ConstInitValList RBRACE {
                    $$ = ast::make_init_list($2);
                    PRINT_TAG("ConstInitVal");
               }
             | LBRACE RBRACE {
                    $$ = ast::make_init_list(new std::vector<ast::InitVal *>());
                    PRINT_TAG("ConstInitVal");
               }
             ;

ConstInitValList : ConstInitVal {
                        $$ = new std::vector<ast::InitVal *>();
                        $$->push_back($1);
                   }
                 | ConstInitValList COMMA ConstInitVal {
                        $$ = $1;
                        $$->push_back($3);
                   }
                 ;

VarDecl : BType VarDefList SEMICN {
              $$ = ast::make_decl(false, $1, $2);
              PRINT_TAG("VarDecl");
         }
        ;

VarDefList : VarDef {
                  $$ = new std::vector<ast::VarDef *>();
                  $$->push_back($1);
             }
           | VarDefList COMMA VarDef {
                  $$ = $1;
                  $$->push_back($3);
             }
           ;

VarDef : Ident ConstExpDims {
              $$ = ast::make_var_def($1, $2, 0);
              std::free($1);
              PRINT_TAG("VarDef");
         }
       | Ident ConstExpDims ASSIGN InitVal {
              $$ = ast::make_var_def($1, $2, $4);
              std::free($1);
              PRINT_TAG("VarDef");
         }
       ;

InitVal : Exp {
              $$ = ast::make_init_expr($1);
              PRINT_TAG("InitVal");
         }
        | LBRACE InitValList RBRACE {
              $$ = ast::make_init_list($2);
              PRINT_TAG("InitVal");
         }
        | LBRACE RBRACE {
              $$ = ast::make_init_list(new std::vector<ast::InitVal *>());
              PRINT_TAG("InitVal");
         }
        ;

InitValList : InitVal {
                  $$ = new std::vector<ast::InitVal *>();
                  $$->push_back($1);
             }
            | InitValList COMMA InitVal {
                  $$ = $1;
                  $$->push_back($3);
             }
            ;

FuncDef : FuncType IDENFR LPARENT FuncFParamsWrapper RPARENT Block {
              $$ = ast::make_function_node($1, $2, $4, $6);
              std::free($2);
              PRINT_TAG("FuncDef");
         }
        ;

MainFuncDef : FuncType MAINTK LPARENT RPARENT Block {
                  $$ = ast::make_function_node($1, "main", new std::vector<ast::Param *>(), $5);
                  PRINT_TAG("FuncDef");
             }
            ;

FuncType : VOIDTK {
               $$ = new ast::Type(ast::Type::void_type());
               PRINT_TAG("FuncType");
          }
         | INTTK {
               $$ = new ast::Type(ast::Type::int_type());
               PRINT_TAG("FuncType");
          }
         | FLOATTK {
               $$ = new ast::Type(ast::Type::float_type());
               PRINT_TAG("FuncType");
          }
         ;

FuncFParamsWrapper : /* empty */ {
                         $$ = new std::vector<ast::Param *>();
                    }
                   | FuncFParams { $$ = $1; }
                   ;

FuncFParams : FuncFParamList {
                  $$ = $1;
                  PRINT_TAG("FuncFParams");
             }
            ;

FuncFParamList : FuncFParam {
                     $$ = new std::vector<ast::Param *>();
                     $$->push_back($1);
                }
               | FuncFParamList COMMA FuncFParam {
                     $$ = $1;
                     $$->push_back($3);
                }
               ;

FuncFParam : BType Ident {
                 $$ = ast::make_param($1, $2, false, new std::vector<ast::Expr *>());
                 std::free($2);
                 PRINT_TAG("FuncFParam");
            }
           | BType Ident LBRACK RBRACK ExpDims {
                 $$ = ast::make_param($1, $2, true, $5);
                 std::free($2);
                 PRINT_TAG("FuncFParam");
            }
           ;

Block : LBRACE BlockItemList RBRACE {
            $$ = ast::make_block_node($2);
            PRINT_TAG("Block");
       }
      ;

BlockItemList : /* empty */ {
                    $$ = new std::vector<ast::BlockItem *>();
               }
              | BlockItemList BlockItem {
                    $$ = $1;
                    $$->push_back($2);
               }
              ;

BlockItem : Decl { $$ = $1; }
          | Stmt { $$ = $1; }
          ;

Stmt : LVal ASSIGN Exp SEMICN {
           $$ = ast::make_assign_stmt($1, $3);
           PRINT_TAG("Stmt");
      }
     | Exp SEMICN {
           $$ = ast::make_expr_stmt($1);
           PRINT_TAG("Stmt");
      }
     | SEMICN {
           $$ = ast::make_empty_stmt();
           PRINT_TAG("Stmt");
      }
     | Block {
           $$ = $1;
           PRINT_TAG("Stmt");
      }
     | IFTK LPARENT Cond RPARENT Stmt %prec LOWER_THAN_ELSE {
           $$ = ast::make_if_stmt($3, $5, 0);
           PRINT_TAG("Stmt");
      }
     | IFTK LPARENT Cond RPARENT Stmt ELSETK Stmt {
           $$ = ast::make_if_stmt($3, $5, $7);
           PRINT_TAG("Stmt");
      }
     | WHILETK LPARENT Cond RPARENT Stmt {
           $$ = ast::make_while_stmt($3, $5);
           PRINT_TAG("Stmt");
      }
     | BREAKTK SEMICN {
           $$ = ast::make_break_stmt();
           PRINT_TAG("Stmt");
      }
     | CONTINUETK SEMICN {
           $$ = ast::make_continue_stmt();
           PRINT_TAG("Stmt");
      }
     | RETURNTK SEMICN {
           $$ = ast::make_return_stmt(0);
           PRINT_TAG("Stmt");
      }
     | RETURNTK Exp SEMICN {
           $$ = ast::make_return_stmt($2);
           PRINT_TAG("Stmt");
      }
     | PRINTFTK LPARENT STRCON ExpListWrapper RPARENT SEMICN {
           $$ = ast::make_format_output_stmt($3, $4);
           std::free($3);
           PRINT_TAG("Stmt");
      }
     ;

ExpListWrapper : /* empty */ {
                     $$ = new std::vector<ast::Expr *>();
                }
               | ExpList { $$ = $1; }
               ;

ExpList : COMMA Exp {
              $$ = new std::vector<ast::Expr *>();
              $$->push_back($2);
         }
        | ExpList COMMA Exp {
              $$ = $1;
              $$->push_back($3);
         }
        ;

Exp : AddExp {
          $$ = $1;
          PRINT_TAG("Exp");
     }
    ;

Cond : LOrExp {
           $$ = $1;
           PRINT_TAG("Cond");
      }
     ;

LVal : Ident ExpDims {
           $$ = ast::make_lvalue($1, $2);
           std::free($1);
           PRINT_TAG("LVal");
      }
     ;

ExpDims : /* empty */ {
              $$ = new std::vector<ast::Expr *>();
         }
        | ExpDims LBRACK Exp RBRACK {
              $$ = $1;
              $$->push_back($3);
         }
        ;

PrimaryExp : LPARENT Exp RPARENT {
                 $$ = $2;
                 PRINT_TAG("PrimaryExp");
            }
           | LVal {
                 $$ = $1;
                 PRINT_TAG("PrimaryExp");
            }
           | Number {
                 $$ = $1;
                 PRINT_TAG("PrimaryExp");
            }
           ;

Number : INTCON {
             $$ = ast::make_int_literal($1);
             std::free($1);
             PRINT_TAG("Number");
        }
       | FLOATCON {
             $$ = ast::make_float_literal($1);
             std::free($1);
             PRINT_TAG("Number");
        }
       ;

UnaryExp : PrimaryExp {
               $$ = $1;
               PRINT_TAG("UnaryExp");
          }
         | Ident LPARENT FuncRParamsWrapper RPARENT {
               $$ = ast::make_call($1, $3);
               std::free($1);
               PRINT_TAG("UnaryExp");
          }
         | GETINTTK LPARENT RPARENT {
               $$ = ast::make_call("getint", new std::vector<ast::Expr *>());
               PRINT_TAG("UnaryExp");
          }
         | PLUS UnaryExp {
               PRINT_TAG("UnaryOp");
               $$ = ast::make_unary(ast::UnaryOp::Plus, $2);
               PRINT_TAG("UnaryExp");
          }
         | MINU UnaryExp {
               PRINT_TAG("UnaryOp");
               $$ = ast::make_unary(ast::UnaryOp::Minus, $2);
               PRINT_TAG("UnaryExp");
          }
         | NOT UnaryExp {
               PRINT_TAG("UnaryOp");
               $$ = ast::make_unary(ast::UnaryOp::Not, $2);
               PRINT_TAG("UnaryExp");
          }
         ;

FuncRParamsWrapper : /* empty */ {
                         $$ = new std::vector<ast::Expr *>();
                    }
                   | FuncRParams { $$ = $1; }
                   ;

FuncRParams : FuncRParamList {
                  $$ = $1;
                  PRINT_TAG("FuncRParams");
             }
            ;

FuncRParamList : Exp {
                     $$ = new std::vector<ast::Expr *>();
                     $$->push_back($1);
                }
               | FuncRParamList COMMA Exp {
                     $$ = $1;
                     $$->push_back($3);
                }
               ;

MulExp : UnaryExp {
             $$ = $1;
             PRINT_TAG("MulExp");
        }
       | MulExp MULT UnaryExp {
             $$ = ast::make_binary(ast::BinaryOp::Mul, $1, $3);
             PRINT_TAG("MulExp");
        }
       | MulExp DIV UnaryExp {
             $$ = ast::make_binary(ast::BinaryOp::Div, $1, $3);
             PRINT_TAG("MulExp");
        }
       | MulExp MOD UnaryExp {
             $$ = ast::make_binary(ast::BinaryOp::Mod, $1, $3);
             PRINT_TAG("MulExp");
        }
       ;

AddExp : MulExp {
             $$ = $1;
             PRINT_TAG("AddExp");
        }
       | AddExp PLUS MulExp {
             $$ = ast::make_binary(ast::BinaryOp::Add, $1, $3);
             PRINT_TAG("AddExp");
        }
       | AddExp MINU MulExp {
             $$ = ast::make_binary(ast::BinaryOp::Sub, $1, $3);
             PRINT_TAG("AddExp");
        }
       ;

RelExp : AddExp {
             $$ = $1;
             PRINT_TAG("RelExp");
        }
       | RelExp LSS AddExp {
             $$ = ast::make_binary(ast::BinaryOp::Lt, $1, $3);
             PRINT_TAG("RelExp");
        }
       | RelExp GRE AddExp {
             $$ = ast::make_binary(ast::BinaryOp::Gt, $1, $3);
             PRINT_TAG("RelExp");
        }
       | RelExp LEQ AddExp {
             $$ = ast::make_binary(ast::BinaryOp::Le, $1, $3);
             PRINT_TAG("RelExp");
        }
       | RelExp GEQ AddExp {
             $$ = ast::make_binary(ast::BinaryOp::Ge, $1, $3);
             PRINT_TAG("RelExp");
        }
       ;

EqExp : RelExp {
            $$ = $1;
            PRINT_TAG("EqExp");
       }
      | EqExp EQL RelExp {
            $$ = ast::make_binary(ast::BinaryOp::Eq, $1, $3);
            PRINT_TAG("EqExp");
       }
      | EqExp NEQ RelExp {
            $$ = ast::make_binary(ast::BinaryOp::Ne, $1, $3);
            PRINT_TAG("EqExp");
       }
      ;

LAndExp : EqExp {
              $$ = $1;
              PRINT_TAG("LAndExp");
         }
        | LAndExp AND EqExp {
              $$ = ast::make_binary(ast::BinaryOp::And, $1, $3);
              PRINT_TAG("LAndExp");
         }
        ;

LOrExp : LAndExp {
             $$ = $1;
             PRINT_TAG("LOrExp");
        }
       | LOrExp OR LAndExp {
             $$ = ast::make_binary(ast::BinaryOp::Or, $1, $3);
             PRINT_TAG("LOrExp");
        }
       ;

ConstExp : AddExp {
               $$ = $1;
               PRINT_TAG("ConstExp");
          }
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
        case TokenType::FLOATTK: return FLOATTK;
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
        case TokenType::INTCON:
            yylval.text = duplicate_text(t.lexeme);
            return INTCON;
        case TokenType::FLOATCON:
            yylval.text = duplicate_text(t.lexeme);
            return FLOATCON;
        case TokenType::IDENFR:
            yylval.text = duplicate_text(t.lexeme);
            return IDENFR;
        case TokenType::STRCON:
            yylval.text = duplicate_text(t.lexeme);
            return STRCON;
        case TokenType::END_OF_FILE: return 0;
        default: return -1;
    }
}
