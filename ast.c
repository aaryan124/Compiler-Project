#include "ast.h"

ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    node->type = type;

    node->name = NULL;
    node->data_type = NULL;
    node->value = 0;
    node->op[0] = '\0';

    node->left = NULL;
    node->right = NULL;

    node->children = NULL;
    node->child_count = 0;

    node->array_size = -1;
    node->index = NULL;

    node->params = NULL;
    node->param_count = 0;

    return node;
}

/* FIXED: safe add_child */
void add_child(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return;

    parent->child_count++;

    parent->children = realloc(parent->children,
        parent->child_count * sizeof(ASTNode*));

    if (!parent->children) {
        printf("realloc failed\n");
        exit(1);
    }

    parent->children[parent->child_count - 1] = child;
}

/* FIXED PRINT (NO DUPLICATE TRAVERSAL) */
void print_ast(ASTNode* node, int level) {
    if (!node) return;

    for (int i = 0; i < level; i++) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM: printf("Program\n"); break;
        case NODE_FUNCTION: printf("Function: %s\n", node->name); break;
        case NODE_BLOCK: printf("Block\n"); break;
        case NODE_VARDECL: printf("VarDecl: %s\n", node->name); break;
        case NODE_ASSIGN: printf("Assign\n"); break;
        case NODE_RETURN: printf("Return\n"); break;
        case NODE_IF: printf("If\n"); break;
        case NODE_WHILE: printf("While\n"); break;
        case NODE_BINARY: printf("BinaryOp: %s\n", node->op); break;
        case NODE_UNARY: printf("UnaryOp: %s\n", node->op); break;
        case NODE_IDENTIFIER: printf("Identifier: %s\n", node->name); break;
        case NODE_LITERAL: printf("Literal: %d\n", node->value); break;
        case NODE_CALL: printf("Call: %s\n", node->name); break;
        case NODE_ARRAY_ACCESS: printf("ArrayAccess: %s\n", node->name); break;
        case NODE_PRINT: printf("Printf\n"); break;
        case NODE_SCANF: printf("Scanf: %s\n", node->name); break;
    }

    /* ONLY children traversal (correct design) */
    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }

    /* OPTIONAL single-node links (safe) */
    if (node->left)
        print_ast(node->left, level + 1);

    if (node->right)
        print_ast(node->right, level + 1);

    if (node->index)
        print_ast(node->index, level + 1);
}