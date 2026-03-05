%option noyywrap nounistd never-interactive header-file="lex.yy.h"
%{
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "common.h"
#include "cmm.tab.h"

#define YYSTYPE YYSTYPE
extern YYSTYPE yylval;
%}
%%
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

[0-9]+          {
                    yylval.ival = std::stoi(std::string(yytext));
                    return INTEGER_LITERAL;
                }
[0-9]+\.[0-9]+([eE][+-]?[0-9]+)? {
                    yylval.dval = std::stod(std::string(yytext));
                    return FLOAT_LITERAL;
                }
[a-zA-Z_][a-zA-Z0-9_]* {
                    yylval.sval = _strdup(yytext);
                    return IDENTIFIER;
                }

[ \t\r\n]       { /* skip whitespace */ }

.               {
                    std::cerr << "Warning: unexpected character '" << yytext[0] << "'" << std::endl;
                }
%%
