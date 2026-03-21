#ifndef TAC_GEN_H
#define TAC_GEN_H

#include "ast.h"
#include "tac.h"

typedef struct ExprResult {
    char place[64];   // variable/temp holding result
    TAC* code;
} ExprResult;

/* entry */
TAC* generate_tac(ASTNode* root);

/* dispatcher */
ExprResult gen(ASTNode* node);

#endif