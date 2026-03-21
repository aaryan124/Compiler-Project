/*
 * codegen.c  —  Phase 6: x86-32 Code Generator
 *
 * Strategy:
 *   • NASM / Intel syntax, 32-bit cdecl calling convention.
 *   • Every variable / temp gets a 4-byte stack slot (ebp-relative).
 *   • Arrays get contiguous stack slots (size from symbol table if available).
 *   • Two passes per function:
 *       Pass 1 – collect all names → build stack frame layout.
 *       Pass 2 – emit one x86 sequence per TAC instruction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "codegen.h"
#include "tac.h"
#include "symbol_table.h"

/* ── constants ──────────────────────────────────────────── */
#define MAX_VARS   512
#define MAX_NAME    64
#define WORD         4        /* bytes per int              */
#define ARR_DEFAULT 10        /* default array elements     */
#define MAX_FUNCS   64

/* ── per-function frame state ───────────────────────────── */
typedef struct { char name[MAX_NAME]; int offset; int is_array; int elems; } VarEntry;

static VarEntry vmap[MAX_VARS];
static int      vcnt  = 0;
static int      vbytes = 0;   /* running stack byte count   */
static int      param_cnt = 0;
static int      skip_goto = 0;  /* suppress unreachable jmp that follows a 'return' */

static void reset_frame(void) { vcnt = 0; vbytes = 0; param_cnt = 0; skip_goto = 0; }

/* ── helpers ────────────────────────────────────────────── */

static int is_int(const char *s) {
    if (!s || !s[0]) return 0;
    int i = (s[0] == '-') ? 1 : 0;
    if (!s[i]) return 0;
    for (; s[i]; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int is_arr_ref(const char *s) { return s && strchr(s, '['); }

/* Control labels are L followed only by digits (L0, L1, …).
   Everything else is a function-entry label. */
static int is_func_label(const char *lbl) {
    if (!lbl || !lbl[0]) return 0;
    if (lbl[0] != 'L') return 1;
    for (int i = 1; lbl[i]; i++) if (!isdigit((unsigned char)lbl[i])) return 1;
    return 0;
}

/* Parse "arr[idx]" → base="arr", idx_s="i" or "0" */
static void parse_arr(const char *s, char *base, char *idx_s) {
    const char *lb = strchr(s, '[');
    const char *rb = strchr(s, ']');
    int blen = (int)(lb - s);
    strncpy(base,  s,      blen); base[blen] = '\0';
    int ilen = (int)(rb - lb - 1);
    strncpy(idx_s, lb + 1, ilen); idx_s[ilen] = '\0';
}

/* Allocate a scalar slot; returns ebp-relative offset. */
static int alloc_scalar(const char *name) {
    for (int i = 0; i < vcnt; i++)
        if (strcmp(vmap[i].name, name) == 0) return vmap[i].offset;
    vbytes += WORD;
    int off = -vbytes;
    strcpy(vmap[vcnt].name, name);
    vmap[vcnt].offset = off; vmap[vcnt].is_array = 0; vmap[vcnt].elems = 1;
    vcnt++;
    return off;
}

/* Allocate an array; returns ebp-relative offset of element 0. */
static int alloc_array(const char *name, int elems) {
    for (int i = 0; i < vcnt; i++)
        if (strcmp(vmap[i].name, name) == 0) return vmap[i].offset;
    vbytes += elems * WORD;
    int off = -vbytes;
    strcpy(vmap[vcnt].name, name);
    vmap[vcnt].offset = off; vmap[vcnt].is_array = 1; vmap[vcnt].elems = elems;
    vcnt++;
    return off;
}

/* Look up or create a scalar slot (used during emit). */
static int get_off(const char *name) {
    for (int i = 0; i < vcnt; i++)
        if (strcmp(vmap[i].name, name) == 0) return vmap[i].offset;
    return alloc_scalar(name);
}

/* ── emit helpers ───────────────────────────────────────── */

static void load_eax(FILE *f, const char *op) {
    if (is_int(op))  fprintf(f, "    mov eax, %s\n", op);
    else             fprintf(f, "    mov eax, dword [ebp%+d]\n", get_off(op));
}
static void load_ecx(FILE *f, const char *op) {
    if (is_int(op))  fprintf(f, "    mov ecx, %s\n", op);
    else             fprintf(f, "    mov ecx, dword [ebp%+d]\n", get_off(op));
}
static void store_eax(FILE *f, const char *var) {
    fprintf(f, "    mov dword [ebp%+d], eax\n", get_off(var));
}

/* arr[idx] → eax */
static void arr_load(FILE *f, const char *base, const char *idx) {
    int off = get_off(base);
    if (is_int(idx)) {
        fprintf(f, "    mov eax, dword [ebp%+d]\n", off + atoi(idx) * WORD);
    } else {
        fprintf(f, "    mov ecx, dword [ebp%+d]\n", get_off(idx));
        fprintf(f, "    lea edx, [ebp%+d]\n", off);
        fprintf(f, "    mov eax, dword [edx + ecx*4]\n");
    }
}

/* eax → arr[idx] */
static void arr_store(FILE *f, const char *base, const char *idx) {
    int off = get_off(base);
    if (is_int(idx)) {
        fprintf(f, "    mov dword [ebp%+d], eax\n", off + atoi(idx) * WORD);
    } else {
        fprintf(f, "    mov ecx, dword [ebp%+d]\n", get_off(idx));
        fprintf(f, "    lea edx, [ebp%+d]\n", off);
        fprintf(f, "    mov dword [edx + ecx*4], eax\n");
    }
}

/* ── pass 1: collect variable names for a function ─────── */

static void scan_vars(TAC *start, TAC *end) {
    for (TAC *t = start; t && t != end; t = t->next) {
        /* result */
        if (t->result[0] &&
            strcmp(t->op,"label")   != 0 &&
            strcmp(t->op,"goto")    != 0 &&
            strcmp(t->op,"ifFalse") != 0 &&
            strcmp(t->op,"param")   != 0 &&
            strcmp(t->op,"return")  != 0) {
            if (is_arr_ref(t->result)) {
                char b[MAX_NAME], ix[MAX_NAME];
                parse_arr(t->result, b, ix);
                /* look up array size from symbol table */
                int elems = ARR_DEFAULT;
                for (int sc = 0; sc < total_scopes; sc++)
                    for (int si = 0; si < scopes[sc].count; si++)
                        if (strcmp(scopes[sc].symbols[si].name, b) == 0 &&
                            strcmp(scopes[sc].symbols[si].kind, "array") == 0)
                            elems = scopes[sc].symbols[si].size;
                alloc_array(b, elems);
                if (!is_int(ix)) alloc_scalar(ix);
            } else if (!is_int(t->result)) {
                alloc_scalar(t->result);
            }
        }
        /* arg1 */
        if (t->arg1[0] && !is_int(t->arg1)) {
            if (is_arr_ref(t->arg1)) {
                char b[MAX_NAME], ix[MAX_NAME];
                parse_arr(t->arg1, b, ix);
                int elems = ARR_DEFAULT;
                for (int sc = 0; sc < total_scopes; sc++)
                    for (int si = 0; si < scopes[sc].count; si++)
                        if (strcmp(scopes[sc].symbols[si].name, b) == 0 &&
                            strcmp(scopes[sc].symbols[si].kind, "array") == 0)
                            elems = scopes[sc].symbols[si].size;
                alloc_array(b, elems);
                if (!is_int(ix)) alloc_scalar(ix);
            } else if (strcmp(t->op,"label") != 0 &&
                       strcmp(t->op,"goto")  != 0 &&
                       strcmp(t->op,"call")  != 0) { /* don't alloc function names */
                alloc_scalar(t->arg1);
            }
        }
        /* arg2 */
        if (t->arg2[0] && !is_int(t->arg2) && !is_arr_ref(t->arg2))
            alloc_scalar(t->arg2);
    }
}

/* ── pass 2: emit x86 for one TAC instruction ───────────── */

static const char *setcc(const char *op) {
    if (!strcmp(op,"<"))  return "setl";
    if (!strcmp(op,">"))  return "setg";
    if (!strcmp(op,"<=")) return "setle";
    if (!strcmp(op,">=")) return "setge";
    if (!strcmp(op,"==")) return "sete";
    if (!strcmp(op,"!=")) return "setne";
    return NULL;
}

static void emit_instr(FILE *f, TAC *t) {

    /* label */
    if (!strcmp(t->op, "label")) {
        if (!is_func_label(t->result)) fprintf(f, "%s:\n", t->result);
        return;
    }
    /* goto */
    if (!strcmp(t->op, "goto")) {
        if (skip_goto) { skip_goto = 0; return; }   /* skip dead jmp after return */
        fprintf(f, "    jmp %s\n", t->result); return;
    }
    /* ifFalse cond goto L */
    if (!strcmp(t->op, "ifFalse")) {
        load_eax(f, t->arg1);
        fprintf(f, "    test eax, eax\n");
        fprintf(f, "    jz %s\n", t->result); return;
    }
    /* param */
    if (!strcmp(t->op, "param")) {
        if (is_int(t->arg1)) fprintf(f, "    push dword %s\n", t->arg1);
        else fprintf(f, "    push dword [ebp%+d]\n", get_off(t->arg1));
        param_cnt++;  return;
    }
    /* call */
    if (!strcmp(t->op, "call")) {
        fprintf(f, "    call _%s\n", t->arg1);
        if (param_cnt > 0) { fprintf(f, "    add esp, %d\n", param_cnt * WORD); param_cnt = 0; }
        if (t->result[0]) store_eax(f, t->result);
        return;
    }
    /* print (new) */
    if (!strcmp(t->op, "print")) {
        load_eax(f, t->arg1);
        fprintf(f, "    push eax\n");
        fprintf(f, "    push dword fmt_int\n");
        fprintf(f, "    call _printf\n");
        fprintf(f, "    add esp, 8\n");
        return;
    }
    /* scan (new) */
    if (!strcmp(t->op, "scan")) {
        int off = get_off(t->arg1);
        fprintf(f, "    lea eax, [ebp%+d]\n", off);
        fprintf(f, "    push eax\n");
        fprintf(f, "    push dword fmt_scan\n");
        fprintf(f, "    call _scanf\n");
        fprintf(f, "    add esp, 8\n");
        return;
    }
    /* return */
    if (!strcmp(t->op, "return")) {
        if (t->arg1[0]) load_eax(f, t->arg1);
        else            fprintf(f, "    xor eax, eax\n");
        fprintf(f, "    leave\n    ret\n");
        skip_goto = 1;   /* next TAC 'goto' (jump-past-else) is now dead code */
        return;
    }
    /* copy / array ops */
    if (!strcmp(t->op, "=")) {
        if (is_arr_ref(t->result)) {
            char b[MAX_NAME], ix[MAX_NAME];
            parse_arr(t->result, b, ix);
            load_eax(f, t->arg1);
            arr_store(f, b, ix);
        } else if (is_arr_ref(t->arg1)) {
            char b[MAX_NAME], ix[MAX_NAME];
            parse_arr(t->arg1, b, ix);
            arr_load(f, b, ix);
            store_eax(f, t->result);
        } else {
            load_eax(f, t->arg1);
            store_eax(f, t->result);
        }
        return;
    }
    /* unary: result = op arg1  (arg2 empty) */
    if (t->arg1[0] && !t->arg2[0] && t->result[0]) {
        load_eax(f, t->arg1);
        if      (!strcmp(t->op, "-")) fprintf(f, "    neg eax\n");
        else if (!strcmp(t->op, "!")) {
            fprintf(f, "    test eax, eax\n");
            fprintf(f, "    sete al\n");
            fprintf(f, "    movzx eax, al\n");
        }
        else if (!strcmp(t->op, "~")) fprintf(f, "    not eax\n");
        else fprintf(f, "    ; unhandled unary: %s\n", t->op);
        store_eax(f, t->result); return;
    }
    /* binary: result = arg1 op arg2 */
    if (t->arg1[0] && t->arg2[0] && t->result[0]) {
        load_eax(f, t->arg1);
        load_ecx(f, t->arg2);
        const char *sc = setcc(t->op);
        if      (!strcmp(t->op, "+"))  fprintf(f, "    add eax, ecx\n");
        else if (!strcmp(t->op, "-"))  fprintf(f, "    sub eax, ecx\n");
        else if (!strcmp(t->op, "*"))  fprintf(f, "    imul eax, ecx\n");
        else if (!strcmp(t->op, "/"))  { fprintf(f, "    cdq\n    idiv ecx\n"); }
        else if (!strcmp(t->op, "%"))  { fprintf(f, "    cdq\n    idiv ecx\n    mov eax, edx\n"); }
        else if (sc) {
            fprintf(f, "    cmp eax, ecx\n");
            fprintf(f, "    %s al\n", sc);
            fprintf(f, "    movzx eax, al\n");
        }
        else if (!strcmp(t->op, "&&")) {
            fprintf(f, "    test eax, eax\n    setnz al\n");
            fprintf(f, "    test ecx, ecx\n    setnz cl\n");
            fprintf(f, "    and al, cl\n    movzx eax, al\n");
        }
        else if (!strcmp(t->op, "||")) {
            fprintf(f, "    test eax, eax\n    setnz al\n");
            fprintf(f, "    test ecx, ecx\n    setnz cl\n");
            fprintf(f, "    or al, cl\n    movzx eax, al\n");
        }
        else fprintf(f, "    ; unhandled binary: %s\n", t->op);
        store_eax(f, t->result); return;
    }

    fprintf(f, "    ; unhandled tac: op=%s arg1=%s arg2=%s res=%s\n",
            t->op, t->arg1, t->arg2, t->result);
}

/* ── main entry ─────────────────────────────────────────── */

void generate_x86(TAC *head, const char *outfile) {
    FILE *f = fopen(outfile, "w");
    if (!f) { fprintf(stderr, "codegen: cannot open '%s'\n", outfile); return; }

    /* header */
    fprintf(f, "; ================================================\n\n");
    fprintf(f, "bits 32\n\n");
    fprintf(f, "extern _printf\n");
    fprintf(f, "extern _scanf\n\n");
    fprintf(f, "section .data\n");
    fprintf(f, "    fmt_int db \"%%d\", 10, 0\n");
    fprintf(f, "    fmt_scan db \"%%d\", 0\n\n");
    fprintf(f, "section .text\n");

    /* collect function starts */
    TAC *func_start[MAX_FUNCS];
    char func_name[MAX_FUNCS][MAX_NAME];
    int  nfuncs = 0;

    for (TAC *t = head; t; t = t->next)
        if (!strcmp(t->op, "label") && is_func_label(t->result) && nfuncs < MAX_FUNCS) {
            fprintf(f, "    global _%s\n", t->result);
            func_start[nfuncs] = t;
            strcpy(func_name[nfuncs], t->result);
            nfuncs++;
        }
    fprintf(f, "\n");

    /* emit each function */
    for (int fi = 0; fi < nfuncs; fi++) {
        TAC *start = func_start[fi];
        TAC *end   = (fi + 1 < nfuncs) ? func_start[fi + 1] : NULL;

        reset_frame();

        /* pass 1 – build frame layout */
        scan_vars(start->next, end);

        /* round to 16-byte alignment */
        int frame = (vbytes + 15) & ~15;

        /* ── detect parameters for this function from the symbol table ──
         * Parameters are in a scope where EVERY entry has kind="parameter".
         * We match by checking if any of the names appear in our var_map. */
        char pname[16][MAX_NAME];
        int  plocal[16];  /* ebp-relative offsets of param locals */
        int  np = 0;
        for (int sc = 0; sc < total_scopes && np == 0; sc++) {
            if (scopes[sc].count == 0) continue;
            int allp = 1;
            for (int si = 0; si < scopes[sc].count; si++)
                if (strcmp(scopes[sc].symbols[si].kind, "parameter") != 0)
                    { allp = 0; break; }
            if (!allp) continue;
            /* check at least one name is in this function's var_map */
            int hit = 0;
            for (int si = 0; si < scopes[sc].count && !hit; si++)
                for (int vi = 0; vi < vcnt && !hit; vi++)
                    if (strcmp(scopes[sc].symbols[si].name, vmap[vi].name) == 0) hit = 1;
            if (!hit) continue;
            /* these are the parameters — record name + local offset */
            np = scopes[sc].count;
            for (int si = 0; si < np; si++) {
                strcpy(pname[si], scopes[sc].symbols[si].name);
                plocal[si] = get_off(pname[si]);  /* already in var_map */
            }
        }

        /* emit prologue */
        fprintf(f, "; ---- %s ----\n", func_name[fi]);
        fprintf(f, "_%s:\n", func_name[fi]);
        fprintf(f, "    push ebp\n    mov ebp, esp\n");
        if (frame > 0) fprintf(f, "    sub esp, %d\n", frame);

        /* print stack layout as comments */
        fprintf(f, "    ; Stack layout for '%s':\n", func_name[fi]);
        for (int i = 0; i < vcnt; i++) {
            if (vmap[i].is_array)
                fprintf(f, "    ;   [ebp%+d .. ebp%+d]  %s[%d]\n",
                        vmap[i].offset, vmap[i].offset + vmap[i].elems * WORD - WORD,
                        vmap[i].name, vmap[i].elems);
            else
                fprintf(f, "    ;   [ebp%+d]  %s\n", vmap[i].offset, vmap[i].name);
        }
        fprintf(f, "\n");

        /* copy parameters from caller stack: [ebp+8] → first param, [ebp+12] → second, … */
        if (np > 0) {
            fprintf(f, "    ; load parameters from caller\n");
            for (int pi = 0; pi < np; pi++) {
                fprintf(f, "    mov eax, dword [ebp+%d]   ; %s\n", 8 + pi * WORD, pname[pi]);
                fprintf(f, "    mov dword [ebp%+d], eax\n", plocal[pi]);
            }
            fprintf(f, "\n");
        }

        /* pass 2 – emit body */
        for (TAC *t = start->next; t && t != end; t = t->next)
            emit_instr(f, t);

        fprintf(f, "\n");
    }

    fclose(f);
    printf("[codegen] x86 assembly written to: %s\n", outfile);
}
