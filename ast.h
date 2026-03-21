#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_BLOCK,
    NODE_VARDECL,
    NODE_ASSIGN,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_BINARY,
    NODE_UNARY,
    NODE_IDENTIFIER,
    NODE_LITERAL,
    NODE_CALL,
    NODE_ARRAY_ACCESS,
    NODE_PRINT,
    NODE_SCANF
} NodeType;

typedef struct ASTNode {
    NodeType type;

    char *name;
    char *data_type;
    int value;
    char op[4];

    struct ASTNode *left;
    struct ASTNode *right;

    struct ASTNode **children;
    int child_count;

    int array_size;
    struct ASTNode *index;

    struct ASTNode **params;
    int param_count;

} ASTNode;

ASTNode* create_node(NodeType type);
void add_child(ASTNode* parent, ASTNode* child);
void print_ast(ASTNode* node, int level);

#endif