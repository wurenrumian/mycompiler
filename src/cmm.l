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
extern int yylineno;
%}

%x STRING

%%

"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"return"        { return RETURN; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }
"int"           { return INT_TYPE; }
"float"         { return FLOAT_TYPE; }
"void"          { return VOID_TYPE; }

"+"             { return PLUS; }
"-"             { return MINU; }
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

"("             { return LPARENT; }
")"             { return RPARENT; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACK; }
"]"             { return RBRACK; }
";"             { return SEMICOLON; }
","             { return COMMA; }

\"              { BEGIN(STRING); yylval.sval = (char*)malloc(1024); *yylval.sval = '\0'; }
<STRING>\"      { BEGIN(INITIAL); return STRCON; }
<STRING>\\n     { strcat(yylval.sval, "\n"); }
<STRING>\\t     { strcat(yylval.sval, "\t"); }
<STRING>\\\"    { strcat(yylval.sval, "\""); }
<STRING>\\      { strcat(yylval.sval, "\\"); }
<STRING>\n      { std::cerr << "Error: newline in string literal" << std::endl; BEGIN(INITIAL); }
<STRING>.       { char temp[2] = { yytext[0], '\0' }; strcat(yylval.sval, temp); }

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
