%{
#include "ast.h" 
#include "parser.tab.h"
#include <stdlib.h>
#include <string.h>

/* Function to map token constants to strings for the table */
const char* get_token_name(int token) {
    switch (token) {
        case T_INT: return "T_INT";
        case T_VOID: return "T_VOID";
        case T_IF: return "T_IF";
        case T_ELSE: return "T_ELSE";
        case T_WHILE: return "T_WHILE";
        case T_RETURN: return "T_RETURN";
        case T_PRINTF: return "T_PRINTF";
        case T_SCANF:  return "T_SCANF";
        case T_EQ: return "T_EQ";
        case T_NEQ: return "T_NEQ";
        case T_LTE: return "T_LTE";
        case T_GTE: return "T_GTE";
        case T_AND: return "T_AND";
        case T_OR: return "T_OR";
        case T_PLUS: return "T_PLUS";
        case T_MINUS: return "T_MINUS";
        case T_STAR: return "T_STAR";
        case T_SLASH: return "T_SLASH";
        case T_ASSIGN: return "T_ASSIGN";
        case T_LT: return "T_LT";
        case T_GT: return "T_GT";
        case T_NOT: return "T_NOT";
        case T_LPAREN: return "T_LPAREN";
        case T_RPAREN: return "T_RPAREN";
        case T_LBRACE: return "T_LBRACE";
        case T_RBRACE: return "T_RBRACE";
        case T_LBRACKET: return "T_LBRACKET";
        case T_RBRACKET: return "T_RBRACKET";
        case T_SEMICOLON: return "T_SEMICOLON";
        case T_COMMA: return "T_COMMA";
        case T_INT_LITERAL: return "T_INT_LITERAL";
        case T_IDENTIFIER: return "T_IDENTIFIER";
        default: return "UNKNOWN";
    }
}
%}

%option noyywrap
%option yylineno

%%

"int"      { return T_INT; }
"void"     { return T_VOID; }
"if"       { return T_IF; }
"else"     { return T_ELSE; }
"while"    { return T_WHILE; }
"return"   { return T_RETURN; }
"printf"   { return T_PRINTF; }
"scanf"    { return T_SCANF; }

"=="       { return T_EQ; }
"!="       { return T_NEQ; }
"<="       { return T_LTE; }
">="       { return T_GTE; }
"&&"       { return T_AND; }
"||"       { return T_OR; }

"+"        { return T_PLUS; }
"-"        { return T_MINUS; }
"*"        { return T_STAR; }
"/"        { return T_SLASH; }
"="        { return T_ASSIGN; }
"<"        { return T_LT; }
">"        { return T_GT; }
"!"        { return T_NOT; }

"("        { return T_LPAREN; }
")"        { return T_RPAREN; }
"{"        { return T_LBRACE; }
"}"        { return T_RBRACE; }
"["        { return T_LBRACKET; }
"]"        { return T_RBRACKET; }
";"        { return T_SEMICOLON; }
","        { return T_COMMA; }

[0-9]+ {
    yylval.intval = atoi(yytext);
    return T_INT_LITERAL;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return T_IDENTIFIER;
}

[ \t\r\n]+   ; /* Skip whitespace */

"//".* ; /* Inline comments */

"/*"([^*]|\*+[^*/])*\*+"/"   ; /* Block comments */

. {
    extern void print_error_line(int line);
    printf("Lexical Error at line %d: Invalid character '%s'\n", yylineno, yytext);
    print_error_line(yylineno);
    exit(1);
}

%%