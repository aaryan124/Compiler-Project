/*
 * optimizer.c  —  Phase 5: TAC Optimiser
 *
 * Applies four classic local optimisation passes to the Three-Address Code
 * linked list produced by tac_gen.c.  All passes are data-flow-independent
 * so they work per basic block / per instruction without building a full CFG.
 *
 * Pass order matters:
 *   constant_folding  →  constant_propagation  →  copy_propagation  →  DCE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "optimizer.h"
#include "tac.h"

/* ============================================================
 *  INTERNAL LIMITS
 * ============================================================ */

#define MAX_VARS   256   /* max distinct variable names tracked  */
#define MAX_NAME    64   /* max length of a name (matches tac.h) */

/* ============================================================
 *  HELPER: is a string a literal integer?
 * ============================================================ */

static int is_integer(const char* s) {
    if (!s || s[0] == '\0') return 0;
    int i = 0;
    if (s[0] == '-') i = 1;          /* optional leading minus */
    if (s[i] == '\0') return 0;
    for (; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

/* ============================================================
 *  HELPER: evaluate a binary operation on two integer constants
 *  Returns 1 on success and stores result in *out.
 * ============================================================ */

static int eval_binary(const char* op, int a, int b, int* out) {
    if (strcmp(op, "+")  == 0) { *out = a + b;          return 1; }
    if (strcmp(op, "-")  == 0) { *out = a - b;          return 1; }
    if (strcmp(op, "*")  == 0) { *out = a * b;          return 1; }
    if (strcmp(op, "/")  == 0) { if (b == 0) return 0;
                                   *out = a / b;         return 1; }
    if (strcmp(op, "%")  == 0) { if (b == 0) return 0;
                                   *out = a % b;         return 1; }
    if (strcmp(op, "<")  == 0) { *out = (a <  b);       return 1; }
    if (strcmp(op, ">")  == 0) { *out = (a >  b);       return 1; }
    if (strcmp(op, "<=") == 0) { *out = (a <= b);       return 1; }
    if (strcmp(op, ">=") == 0) { *out = (a >= b);       return 1; }
    if (strcmp(op, "==") == 0) { *out = (a == b);       return 1; }
    if (strcmp(op, "!=") == 0) { *out = (a != b);       return 1; }
    if (strcmp(op, "&&") == 0) { *out = (a && b);       return 1; }
    if (strcmp(op, "||") == 0) { *out = (a || b);       return 1; }
    return 0;   /* unknown operator — leave untouched */
}

/* ============================================================
 *  HELPER: evaluate a unary operation on an integer constant
 * ============================================================ */

static int eval_unary(const char* op, int a, int* out) {
    if (strcmp(op, "-")  == 0) { *out = -a;  return 1; }
    if (strcmp(op, "!")  == 0) { *out = !a;  return 1; }
    if (strcmp(op, "~")  == 0) { *out = ~a;  return 1; }
    return 0;
}

/* ============================================================
 *  HELPER: count instructions in list
 * ============================================================ */

static int tac_length(TAC* head) {
    int n = 0;
    for (TAC* t = head; t; t = t->next) n++;
    return n;
}

/* ============================================================
 *  PASS 1: CONSTANT FOLDING
 *
 *  If both operands of a binary op are integer literals, replace
 *  the whole instruction with a copy from the computed constant.
 *
 *    t0 = 3 + 4  →  t0 = 7
 *    t1 = a * 0  →  (only if BOTH are literals)
 *
 *  For unary ops: if the operand is a literal, fold it too.
 *    t2 = -5     →  t2 = -5   (already canonical — op="-", arg1="5", result="t2")
 * ============================================================ */

TAC* pass_constant_folding(TAC* head) {
    int folded = 0;
    for (TAC* t = head; t; t = t->next) {

        /* Binary: result = arg1 op arg2 */
        if (t->arg1[0] && t->arg2[0] && t->result[0] &&
            strcmp(t->op, "=")       != 0 &&
            strcmp(t->op, "label")   != 0 &&
            strcmp(t->op, "goto")    != 0 &&
            strcmp(t->op, "ifFalse") != 0 &&
            strcmp(t->op, "param")   != 0 &&
            strcmp(t->op, "call")    != 0 &&
            strcmp(t->op, "return")  != 0) {

            if (is_integer(t->arg1) && is_integer(t->arg2)) {
                int a = atoi(t->arg1);
                int b = atoi(t->arg2);
                int out;
                if (eval_binary(t->op, a, b, &out)) {
                    /* Rewrite to: result = <constant> */
                    sprintf(t->arg1, "%d", out);
                    t->arg2[0] = '\0';
                    strcpy(t->op, "=");
                    folded++;
                }
            }
        }

        /* Unary: result = op arg1   (arg2 is empty) */
        if (t->arg1[0] && t->arg2[0] == '\0' && t->result[0] &&
            strcmp(t->op, "=")       != 0 &&
            strcmp(t->op, "label")   != 0 &&
            strcmp(t->op, "goto")    != 0 &&
            strcmp(t->op, "ifFalse") != 0 &&
            strcmp(t->op, "param")   != 0 &&
            strcmp(t->op, "call")    != 0 &&
            strcmp(t->op, "return")  != 0) {

            if (is_integer(t->arg1)) {
                int a = atoi(t->arg1);
                int out;
                if (eval_unary(t->op, a, &out)) {
                    sprintf(t->arg1, "%d", out);
                    strcpy(t->op, "=");
                    folded++;
                }
            }
        }
    }

    if (folded > 0)
        printf("[optimizer] constant folding:      %d instruction(s) folded\n", folded);
    else
        printf("[optimizer] constant folding:      nothing to fold\n");

    return head;
}

/* ============================================================
 *  PASS 2: CONSTANT PROPAGATION
 *
 *  Track every  x = <literal>  assignment.  For each later use of x
 *  as arg1 or arg2, substitute the known constant value.
 *
 *  The map is invalidated for a variable if it is re-assigned later.
 *
 *    x = 5
 *    t0 = x + 3   →  t0 = 5 + 3
 *    x = 9        (kills the mapping for x)
 *    t1 = x + 1   (left alone)
 * ============================================================ */

typedef struct {
    char var[MAX_NAME];
    char val[MAX_NAME];    /* string form of the constant */
    int  valid;
} ConstEntry;

TAC* pass_constant_propagation(TAC* head) {
    ConstEntry map[MAX_VARS];
    int sz = 0;
    int replaced = 0;

    /* Helper to look up a variable. Returns index or -1. */
    #define FIND_CONST(name) ({ \
        int _r = -1; \
        for (int _i = 0; _i < sz; _i++) \
            if (map[_i].valid && strcmp(map[_i].var, (name)) == 0) { _r = _i; break; } \
        _r; })

    for (TAC* t = head; t; t = t->next) {

        /* Propagate into arg1 */
        if (t->arg1[0] && !is_integer(t->arg1)) {
            int idx = -1;
            for (int i = 0; i < sz; i++)
                if (map[i].valid && strcmp(map[i].var, t->arg1) == 0)
                    { idx = i; break; }
            if (idx >= 0) {
                strcpy(t->arg1, map[idx].val);
                replaced++;
            }
        }

        /* Propagate into arg2 */
        if (t->arg2[0] && !is_integer(t->arg2)) {
            int idx = -1;
            for (int i = 0; i < sz; i++)
                if (map[i].valid && strcmp(map[i].var, t->arg2) == 0)
                    { idx = i; break; }
            if (idx >= 0) {
                strcpy(t->arg2, map[idx].val);
                replaced++;
            }
        }

        /* At a label (possible loop top or join point) — flush ALL mappings
         * to avoid propagating values that may be stale after back-edges. */
        if (strcmp(t->op, "label") == 0) {
            for (int i = 0; i < sz; i++) map[i].valid = 0;
            continue;
        }

        /* Record / kill mapping */
        if (t->result[0] &&
            strcmp(t->op, "label")   != 0 &&
            strcmp(t->op, "goto")    != 0 &&
            strcmp(t->op, "ifFalse") != 0 &&
            strcmp(t->op, "param")   != 0 &&
            strcmp(t->op, "call")    != 0 &&
            strcmp(t->op, "return")  != 0) {

            /* Is this a plain copy from a literal? */
            if (strcmp(t->op, "=") == 0 && is_integer(t->arg1) &&
                !strchr(t->result, '[')) {   /* result must be a plain variable */
                /* Add / update map entry */
                int found = 0;
                for (int i = 0; i < sz; i++) {
                    if (strcmp(map[i].var, t->result) == 0) {
                        strcpy(map[i].val, t->arg1);
                        map[i].valid = 1;
                        found = 1;
                        break;
                    }
                }
                if (!found && sz < MAX_VARS) {
                    strcpy(map[sz].var, t->result);
                    strcpy(map[sz].val, t->arg1);
                    map[sz].valid = 1;
                    sz++;
                }
            } else {
                /* Non-constant assignment → invalidate */
                for (int i = 0; i < sz; i++)
                    if (strcmp(map[i].var, t->result) == 0)
                        map[i].valid = 0;
            }
        }
    }

    #undef FIND_CONST

    if (replaced > 0)
        printf("[optimizer] constant propagation:  %d use(s) replaced\n", replaced);
    else
        printf("[optimizer] constant propagation:  nothing to propagate\n");

    return head;
}

/* ============================================================
 *  PASS 3: COPY PROPAGATION
 *
 *  Track every  x = y  assignment where y is a variable (not a literal).
 *  For each later use of x as arg1 or arg2, substitute y.
 *  Mapping is killed when either x or y is re-assigned.
 *
 *    x = y
 *    t0 = x + 1   →  t0 = y + 1
 * ============================================================ */

typedef struct {
    char lhs[MAX_NAME];  /* the copy destination  */
    char rhs[MAX_NAME];  /* the copy source       */
    int  valid;
} CopyEntry;

TAC* pass_copy_propagation(TAC* head) {
    CopyEntry map[MAX_VARS];
    int sz = 0;
    int replaced = 0;

    for (TAC* t = head; t; t = t->next) {

        /* Propagate into arg1 */
        if (t->arg1[0] && !is_integer(t->arg1)) {
            for (int i = 0; i < sz; i++) {
                if (map[i].valid && strcmp(map[i].lhs, t->arg1) == 0) {
                    strcpy(t->arg1, map[i].rhs);
                    replaced++;
                    break;
                }
            }
        }

        /* Propagate into arg2 */
        if (t->arg2[0] && !is_integer(t->arg2)) {
            for (int i = 0; i < sz; i++) {
                if (map[i].valid && strcmp(map[i].lhs, t->arg2) == 0) {
                    strcpy(t->arg2, map[i].rhs);
                    replaced++;
                    break;
                }
            }
        }

        /* At a label — flush ALL copy mappings (loop back-edge safety). */
        if (strcmp(t->op, "label") == 0) {
            for (int i = 0; i < sz; i++) map[i].valid = 0;
            continue;
        }

        /* Record / kill copy mappings */
        if (t->result[0] &&
            strcmp(t->op, "label")   != 0 &&
            strcmp(t->op, "goto")    != 0 &&
            strcmp(t->op, "ifFalse") != 0 &&
            strcmp(t->op, "param")   != 0 &&
            strcmp(t->op, "call")    != 0 &&
            strcmp(t->op, "return")  != 0) {

            /* Kill all entries where lhs == result OR rhs == result */
            for (int i = 0; i < sz; i++) {
                if (strcmp(map[i].lhs, t->result) == 0 ||
                    strcmp(map[i].rhs, t->result) == 0)
                    map[i].valid = 0;
            }

            /* Record new copy ONLY if:
             *   result = var  (no array brackets on either side, not a literal, no expr) */
            if (strcmp(t->op, "=") == 0 &&
                t->arg1[0]         &&
                t->arg2[0] == '\0' &&
                !is_integer(t->arg1) &&
                !strchr(t->arg1,   '[') &&   /* arg1 must be plain variable */
                !strchr(t->result, '[')) {   /* result must be plain variable */

                int found = 0;
                for (int i = 0; i < sz; i++) {
                    if (strcmp(map[i].lhs, t->result) == 0) {
                        strcpy(map[i].rhs, t->arg1);
                        map[i].valid = 1;
                        found = 1;
                        break;
                    }
                }
                if (!found && sz < MAX_VARS) {
                    strcpy(map[sz].lhs, t->result);
                    strcpy(map[sz].rhs, t->arg1);
                    map[sz].valid = 1;
                    sz++;
                }
            }
        }
    }

    if (replaced > 0)
        printf("[optimizer] copy propagation:      %d use(s) replaced\n", replaced);
    else
        printf("[optimizer] copy propagation:      nothing to propagate\n");

    return head;
}

/* ============================================================
 *  PASS 4: DEAD CODE ELIMINATION
 *
 *  An assignment  x = ...  is dead if x is never used as arg1 or arg2
 *  in any later instruction AND x starts with 't' (it is a compiler-
 *  generated temporary — we do NOT remove user variable assignments).
 *
 *  The pass makes multiple sweeps until no changes occur (needed for
 *  cascaded dead temps: t0 dead → removes t0 = t1+t2 → t1 and t2 may
 *  become dead in the next sweep).
 * ============================================================ */

/* Return 1 if `name` appears as arg1 or arg2 in any instruction AFTER `start`. */
static int is_used_after(TAC* start, const char* name) {
    for (TAC* t = start; t; t = t->next) {
        if (t->arg1[0] && strcmp(t->arg1, name) == 0) return 1;
        if (t->arg2[0] && strcmp(t->arg2, name) == 0) return 1;
        /* Also check inside ifFalse target (that's arg1 there) */
    }
    return 0;
}

/* Return 1 if name looks like a compiler temp (t0, t1, t42 …). */
static int is_temp(const char* name) {
    if (!name || name[0] != 't') return 0;
    for (int i = 1; name[i]; i++)
        if (!isdigit((unsigned char)name[i])) return 0;
    return name[1] != '\0';   /* "t" alone is not a temp */
}

TAC* pass_dead_code_elimination(TAC* head) {
    int total_removed = 0;
    int removed;

    do {
        removed = 0;
        TAC* prev = NULL;
        TAC* t    = head;

        while (t) {
            int kill = 0;

            /* Candidate: result is a compiler temp AND op is assignment-like */
            if (t->result[0] && is_temp(t->result) &&
                strcmp(t->op, "label")   != 0 &&
                strcmp(t->op, "goto")    != 0 &&
                strcmp(t->op, "ifFalse") != 0 &&
                strcmp(t->op, "param")   != 0 &&
                strcmp(t->op, "return")  != 0) {

                /* Check that the result is never used from t->next onwards */
                if (!is_used_after(t->next, t->result)) {

                    /* Also make sure it is not a function call with side-effects
                     * (we still want to keep "t0 = call foo" even if t0 unused,
                     *  because calling foo may have side-effects) */
                    if (strcmp(t->op, "call") != 0) {
                        kill = 1;
                    }
                }
            }

            if (kill) {
                removed++;
                total_removed++;
                TAC* dead = t;
                if (prev) {
                    prev->next = t->next;
                    t = t->next;
                } else {
                    head = t->next;
                    t    = head;
                }
                free(dead);
            } else {
                prev = t;
                t    = t->next;
            }
        }
    } while (removed > 0);  /* repeat until stable */

    if (total_removed > 0)
        printf("[optimizer] dead code elimination: %d instruction(s) removed\n", total_removed);
    else
        printf("[optimizer] dead code elimination: nothing to remove\n");

    return head;
}

/* ============================================================
 *  MAIN ENTRY: run all passes in sequence
 * ============================================================ */

TAC* optimize(TAC* head) {
    printf("\n===== OPTIMIZER =====\n");

    int before = tac_length(head);

    head = pass_constant_folding(head);
    head = pass_constant_propagation(head);
    head = pass_copy_propagation(head);
    head = pass_dead_code_elimination(head);

    int after = tac_length(head);

    printf("[optimizer] instructions before: %d  |  after: %d  |  saved: %d\n",
           before, after, before - after);
    printf("=====================\n");

    return head;
}

/* ============================================================
 *  PRETTY PRINTER (same format as print_tac, new header)
 * ============================================================ */

void print_optimized_tac(TAC* head) {
    printf("\n===== OPTIMIZED THREE ADDRESS CODE =====\n");

    for (TAC* t = head; t; t = t->next) {

        if (strcmp(t->op, "label") == 0) {
            printf("%s:\n", t->result);
        }
        else if (strcmp(t->op, "goto") == 0) {
            printf("  goto %s\n", t->result);
        }
        else if (strcmp(t->op, "ifFalse") == 0) {
            printf("  ifFalse %s goto %s\n", t->arg1, t->result);
        }
        else if (strcmp(t->op, "param") == 0) {
            printf("  param %s\n", t->arg1);
        }
        else if (strcmp(t->op, "call") == 0) {
            if (t->arg2[0])
                printf("  %s = call %s, %s\n", t->result, t->arg1, t->arg2);
            else
                printf("  %s = call %s\n", t->result, t->arg1);
        }
        else if (strcmp(t->op, "return") == 0) {
            printf("  return %s\n", t->arg1);
        }
        else if (strcmp(t->op, "=") == 0) {
            printf("  %s = %s\n", t->result, t->arg1);
        }
        else if (t->arg2[0] == '\0') {
            printf("  %s = %s %s\n", t->result, t->op, t->arg1);
        }
        else {
            printf("  %s = %s %s %s\n", t->result, t->arg1, t->op, t->arg2);
        }
    }

    printf("=========================================\n");
}
