#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tac_gen.h"
#include "temp.h"

/* ================= HELPERS ================= */

static ExprResult empty() {
    ExprResult r;
    r.place[0] = '\0';
    r.code = NULL;
    return r;
}

/* forward declaration */
ExprResult gen(ASTNode* node);

/* ================= ENTRY ================= */

TAC* generate_tac(ASTNode* root) {
    ExprResult r = gen(root);
    return r.code;
}

/* ================= MAIN DISPATCH ================= */

ExprResult gen(ASTNode* node) {
    if (!node) return empty();

    ExprResult res = empty();

    switch (node->type) {

    /* ================= PROGRAM ================= */
    case NODE_PROGRAM: {
        TAC* code = NULL;

        for (int i = 0; i < node->child_count; i++) {
            ExprResult r = gen(node->children[i]);
            code = append_tac(code, r.code);
        }

        res.code = code;
        return res;
    }

    /* ================= FUNCTION (FIXED) ================= */
    case NODE_FUNCTION: {
        TAC* code = NULL;

        /* function label */
        TAC* label = make_tac("label", "", "", node->name);
        code = append_tac(code, label);

        /* IMPORTANT: AST = Function -> Block */
        if (node->left) {
            ExprResult r = gen(node->left);   // BLOCK ONLY
            code = append_tac(code, r.code);
        }

        res.code = code;
        return res;
    }

    /* ================= BLOCK ================= */
    case NODE_BLOCK: {
        TAC* code = NULL;

        for (int i = 0; i < node->child_count; i++) {
            ExprResult r = gen(node->children[i]);
            code = append_tac(code, r.code);
        }

        res.code = code;
        return res;
    }

    /* ================= VAR DECL (🔥 FIX - IMPORTANT) ================= */
    case NODE_VARDECL: {
        if (node->left) {
            ExprResult r = gen(node->left);

            TAC* instr = make_tac("=", r.place, "", node->name);
            res.code = append_tac(r.code, instr);
        } else {
            res.code = NULL;
        }
        return res;
    }

    /* ================= LITERAL ================= */
    case NODE_LITERAL: {
        sprintf(res.place, "%d", node->value);
        res.code = NULL;
        return res;
    }

    /* ================= IDENTIFIER ================= */
    case NODE_IDENTIFIER: {
        strcpy(res.place, node->name);
        return res;
    }

    /* ================= BINARY ================= */
    case NODE_BINARY: {
        ExprResult L = gen(node->left);
        ExprResult R = gen(node->right);

        char* t = new_temp();

        TAC* code = append_tac(L.code, R.code);
        TAC* instr = make_tac(node->op, L.place, R.place, t);

        res.code = append_tac(code, instr);
        strcpy(res.place, t);

        return res;
    }

    /* ================= ASSIGN ================= */
    case NODE_ASSIGN: {
        ExprResult R = gen(node->right);

        if (node->left->type == NODE_ARRAY_ACCESS) {
            ExprResult indexR = gen(node->left->index);
            TAC* code = append_tac(R.code, indexR.code);
            char array_ref[64];
            sprintf(array_ref, "%s[%s]", node->left->name, indexR.place);
            TAC* instr = make_tac("=", R.place, "", array_ref);
            res.code = append_tac(code, instr);
            strcpy(res.place, array_ref);
        } else {
            char* id = node->left->name;
            TAC* code = R.code;
            TAC* instr = make_tac("=", R.place, "", id);
            res.code = append_tac(code, instr);
            strcpy(res.place, id);
        }

        return res;
    }

    /* ================= RETURN ================= */
    case NODE_RETURN: {
        ExprResult E = gen(node->left);

        TAC* instr = make_tac("return", E.place, "", "");
        res.code = append_tac(E.code, instr);

        return res;
    }

    /* ================= IF (SAFE VERSION) ================= */
    case NODE_IF: {
        ExprResult cond = gen(node->left);

        char* Lfalse = new_label();
        char* Lend = new_label();

        TAC* code = cond.code;

        TAC* ifz = make_tac("ifFalse", cond.place, "", Lfalse);
        code = append_tac(code, ifz);

        /* THEN */
        ExprResult thenR = gen(node->children[0]);
        code = append_tac(code, thenR.code);

        TAC* jmp = make_tac("goto", "", "", Lend);
        code = append_tac(code, jmp);

        /* ELSE label */
        TAC* l1 = make_tac("label", "", "", Lfalse);
        code = append_tac(code, l1);

        if (node->child_count > 1) {
            ExprResult elseR = gen(node->children[1]);
            code = append_tac(code, elseR.code);
        }

        TAC* l2 = make_tac("label", "", "", Lend);
        code = append_tac(code, l2);

        res.code = code;
        return res;
    }

    /* ================= WHILE ================= */
    case NODE_WHILE: {
        char* Lstart = new_label();
        char* Lend = new_label();

        TAC* start = make_tac("label", "", "", Lstart);

        ExprResult cond = gen(node->left);
        ExprResult body = gen(node->right);   // FIXED SAFE

        TAC* ifz = make_tac("ifFalse", cond.place, "", Lend);
        TAC* jmp = make_tac("goto", "", "", Lstart);
        TAC* lend = make_tac("label", "", "", Lend);

        TAC* code = start;
        code = append_tac(code, cond.code);
        code = append_tac(code, ifz);
        code = append_tac(code, body.code);
        code = append_tac(code, jmp);
        code = append_tac(code, lend);

        res.code = code;
        return res;
    }

    /* ================= FUNCTION CALL (FIXED) ================= */
    case NODE_CALL: {
        TAC* code = NULL;

        /* FIX: handle ALL arguments */
        for (int i = 0; i < node->child_count; i++) {
            ExprResult arg = gen(node->children[i]);

            code = append_tac(code, arg.code);
            code = append_tac(code, make_tac("param", arg.place, "", ""));
        }

        char* t = new_temp();

        TAC* call = make_tac("call", node->name, "", t);
        code = append_tac(code, call);

        strcpy(res.place, t);
        res.code = code;

        return res;
    }

    /* ================= ARRAY ACCESS ================= */
    case NODE_ARRAY_ACCESS: {
        ExprResult index_expr = gen(node->index);
        char* t = new_temp();
        char array_ref[64];
        sprintf(array_ref, "%s[%s]", node->name, index_expr.place);
        TAC* instr = make_tac("=", array_ref, "", t);
        res.code = append_tac(index_expr.code, instr);
        strcpy(res.place, t);
        return res;
    }

    /* ================= UNARY ================= */
    case NODE_UNARY: {
        ExprResult E = gen(node->left);
        char* t = new_temp();
        TAC* instr = make_tac(node->op, E.place, "", t);
        res.code = append_tac(E.code, instr);
        strcpy(res.place, t);
        return res;
    }

    /* ================= PRINTF (⭐ NEW) ================= */
    case NODE_PRINT: {
        ExprResult E = gen(node->left);
        res.code = append_tac(E.code, make_tac("print", E.place, "", ""));
        return res;
    }

    /* ================= SCANF (⭐ NEW) ================= */
    case NODE_SCANF: {
        res.code = make_tac("scan", node->name, "", "");
        return res;
    }

    /* ================= DEBUG ================= */
    default:
        printf("⚠️ Unhandled AST node type: %d\n", node->type);
        return empty();
    }
}