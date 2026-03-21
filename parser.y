%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symbol_table.h"

ASTNode* root;

/* function tracker */
char current_function[50];

int yylex();
void yyerror(const char *s);
%}

/* ================= UNION ================= */
%union {
    int intval;
    char* str;
    ASTNode* node;
}

/* ================= TOKENS ================= */
%token T_INT T_VOID T_IF T_ELSE T_WHILE T_RETURN T_PRINTF T_SCANF
%token T_PLUS T_MINUS T_STAR T_SLASH
%token T_ASSIGN
%token T_EQ T_NEQ T_LT T_GT T_LTE T_GTE
%token T_AND T_OR T_NOT
%token T_LPAREN T_RPAREN T_LBRACE T_RBRACE
%token T_LBRACKET T_RBRACKET
%token T_SEMICOLON T_COMMA

%token <str> T_IDENTIFIER
%token <intval> T_INT_LITERAL

/* ================= TYPES ================= */
%type <node> program function_list function block stmt_list statement expression
%type <node> var_decl param_list arg_list

/* ================= PRECEDENCE ================= */
%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

%left T_OR
%left T_AND
%left T_EQ T_NEQ
%left T_LT T_GT T_LTE T_GTE
%left T_PLUS T_MINUS
%left T_STAR T_SLASH
%right T_NOT

%%

/* ================= PROGRAM ================= */

program:
    function_list
    {
        root = $1;
    }
;

function_list:
      function
      {
          ASTNode* prog = create_node(NODE_PROGRAM);
          add_child(prog, $1);
          $$ = prog;
      }
    | function_list function
      {
          add_child($1, $2);
          $$ = $1;
      }
;

/* ================= FUNCTION ================= */

function:
    T_INT T_IDENTIFIER T_LPAREN
    {
        insert_function($2, "int");
        strcpy(current_function, $2);

        enter_scope();   /* function scope */
    }
    param_list T_RPAREN block
    {
        ASTNode* func = create_node(NODE_FUNCTION);
        func->name = $2;
        func->data_type = "int";

        func->params = $5->children;
        func->param_count = $5->child_count;
        func->left = $7;

        exit_scope();
        $$ = func;
    }
;

/* ================= PARAMETERS ================= */

param_list:
      /* empty */
      {
          $$ = create_node(NODE_BLOCK);
      }
    | T_INT T_IDENTIFIER
      {
          insert_symbol($2, "parameter", "int", 0);
          add_param(current_function, "int");

          $$ = create_node(NODE_BLOCK);

          ASTNode* p = create_node(NODE_VARDECL);
          p->name = $2;
          p->data_type = "int";

          add_child($$, p);
      }
    | param_list T_COMMA T_INT T_IDENTIFIER
      {
          insert_symbol($4, "parameter", "int", 0);
          add_param(current_function, "int");

          ASTNode* p = create_node(NODE_VARDECL);
          p->name = $4;
          p->data_type = "int";

          add_child($1, p);
          $$ = $1;
      }
;

/* ================= BLOCK (FIXED) ================= */

block:
    T_LBRACE
    {
        enter_scope();
    }
    stmt_list
    T_RBRACE
    {
        exit_scope();

        ASTNode* b = create_node(NODE_BLOCK);

        /* FIX: safe copy instead of pointer reuse */
        for (int i = 0; i < $3->child_count; i++) {
            add_child(b, $3->children[i]);
        }

        $$ = b;
    }
    |
    T_LBRACE T_RBRACE
    {
        $$ = create_node(NODE_BLOCK);
    }
;

/* ================= STATEMENTS ================= */

stmt_list:
      statement
      {
          $$ = create_node(NODE_BLOCK);
          add_child($$, $1);
      }
    | stmt_list statement
      {
          add_child($1, $2);
          $$ = $1;
      }
;

/* ================= STATEMENTS ================= */

statement:
      var_decl
    | T_IDENTIFIER T_ASSIGN expression T_SEMICOLON
      {
          lookup($1);

          ASTNode* n = create_node(NODE_ASSIGN);
          n->left = create_node(NODE_IDENTIFIER);
          n->left->name = $1;
          n->right = $3;

          $$ = n;
      }
    | T_IDENTIFIER T_LBRACKET expression T_RBRACKET T_ASSIGN expression T_SEMICOLON
      {
          lookup($1);

          ASTNode* n = create_node(NODE_ASSIGN);
          n->left = create_node(NODE_ARRAY_ACCESS);
          n->left->name = $1;
          n->left->index = $3;
          n->right = $6;

          $$ = n;
      }
    | T_RETURN expression T_SEMICOLON
      {
          ASTNode* n = create_node(NODE_RETURN);
          n->left = $2;
          $$ = n;
      }
    | T_PRINTF T_LPAREN expression T_RPAREN T_SEMICOLON
      {
          ASTNode* n = create_node(NODE_PRINT);
          n->left = $3;
          $$ = n;
      }
    | T_SCANF T_LPAREN T_IDENTIFIER T_RPAREN T_SEMICOLON
      {
          lookup($3);
          ASTNode* n = create_node(NODE_SCANF);
          n->name = $3;
          $$ = n;
      }
    | T_IF T_LPAREN expression T_RPAREN statement %prec LOWER_THAN_ELSE
    {
        ASTNode* n = create_node(NODE_IF);
        n->left = $3;
        add_child(n, $5);
        $$ = n;
    }
    | T_IF T_LPAREN expression T_RPAREN statement T_ELSE statement
    {
        ASTNode* n = create_node(NODE_IF);
        n->left = $3;
        add_child(n, $5);
        add_child(n, $7);
        $$ = n;
    }
    | T_WHILE T_LPAREN expression T_RPAREN statement
      {
          ASTNode* n = create_node(NODE_WHILE);
          n->left = $3;
          n->right = $5;
          $$ = n;
      }
    | block
;

/* ================= VARIABLE DECL ================= */

var_decl:
      T_INT T_IDENTIFIER T_SEMICOLON
      {
          insert_symbol($2, "variable", "int", 0);

          ASTNode* n = create_node(NODE_VARDECL);
          n->name = $2;
          n->data_type = "int";
          $$ = n;
      }
    | T_INT T_IDENTIFIER T_ASSIGN expression T_SEMICOLON
      {
          insert_symbol($2, "variable", "int", 0);

          ASTNode* n = create_node(NODE_VARDECL);
          n->name = $2;
          n->data_type = "int";
          n->left = $4;

          $$ = n;
      }
    | T_INT T_IDENTIFIER T_LBRACKET T_INT_LITERAL T_RBRACKET T_SEMICOLON
      {
          insert_symbol($2, "array", "int", $4);

          ASTNode* n = create_node(NODE_VARDECL);
          n->name = $2;
          n->array_size = $4;

          $$ = n;
      }
;

/* ================= EXPRESSIONS ================= */

expression:
      expression T_PLUS expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "+");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_MINUS expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "-");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_STAR expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "*");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_SLASH expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "/");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_EQ expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "==");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_NEQ expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "!=");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_LT expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "<");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_GT expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, ">");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_LTE expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "<=");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_GTE expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, ">=");
          $$->left = $1;
          $$->right = $3;
      }
    | expression T_AND expression
      {
          $$ = create_node(NODE_BINARY);
          strcpy($$->op, "&&");
          $$->left = $1;
          $$->right = $3;
      }
    | T_NOT expression
      {
          $$ = create_node(NODE_UNARY);
          strcpy($$->op, "!");
          $$->left = $2;
      }
    | T_IDENTIFIER
      {
          lookup($1);
          $$ = create_node(NODE_IDENTIFIER);
          $$->name = $1;
      }
    | T_INT_LITERAL
      {
          $$ = create_node(NODE_LITERAL);
          $$->value = $1;
      }
    | T_IDENTIFIER T_LPAREN arg_list T_RPAREN
      {
          lookup($1);

          ASTNode* n = create_node(NODE_CALL);
          n->name = $1;

          for (int i = 0; i < $3->child_count; i++) {
             add_child(n, $3->children[i]);
          }

          $$ = n;
      }
    | T_IDENTIFIER T_LBRACKET expression T_RBRACKET
      {
          lookup($1);

          ASTNode* n = create_node(NODE_ARRAY_ACCESS);
          n->name = $1;
          n->index = $3;

          $$ = n;
      }
    | T_LPAREN expression T_RPAREN
      {
          $$ = $2;
      }
;

/* ================= ARG LIST ================= */

arg_list:
      /* empty */
      {
          $$ = create_node(NODE_BLOCK);
      }
    | expression
      {
          $$ = create_node(NODE_BLOCK);
          add_child($$, $1);
      }
    | arg_list T_COMMA expression
      {
          add_child($1, $3);
          $$ = $1;
      }
;

%%

void yyerror(const char *s)
{
    extern int yylineno;
    extern void print_error_line(int line);
    printf("Parse Error at line %d: %s\n", yylineno, s);
    print_error_line(yylineno);
}