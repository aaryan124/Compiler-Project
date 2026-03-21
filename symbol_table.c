#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

extern int yylineno;
extern void print_error_line(int line);

void report_semantic_error(const char* msg, const char* name) {
    if (name) {
        printf("Semantic Error at line %d: %s '%s'\n", yylineno, msg, name);
    } else {
        printf("Semantic Error at line %d: %s\n", yylineno, msg);
    }
    print_error_line(yylineno);
    exit(1);
}

/* ---------------- GLOBAL STORAGE ---------------- */

Scope scopes[MAX_SCOPES];
int total_scopes = 0;

int scope_stack[MAX_SCOPES];
int scope_top = -1;

char current_function[50] = "";

/* ---------------- ENTER SCOPE ---------------- */

void enter_scope() {
    if (scope_top + 1 >= MAX_SCOPES) {
        printf("Error: too many scopes\n");
        exit(1);
    }

    scope_top++;
    scope_stack[scope_top] = total_scopes;

    scopes[total_scopes].count = 0;
    scopes[total_scopes].is_used = 0;

    total_scopes++;
}

/* ---------------- EXIT SCOPE ---------------- */

void exit_scope() {
    if (scope_top < 0) {
        printf("Error: scope underflow\n");
        exit(1);
    }

    int idx = scope_stack[scope_top];

    /* If scope never used → rollback it */
    if (scopes[idx].count == 0) {
        total_scopes--;
    } else {
        scopes[idx].is_used = 1;
    }

    scope_top--;
}

/* ---------------- INSERT SYMBOL ---------------- */

void insert_symbol(char *name, char *kind, char *type, int size) {
    if (scope_top < 0) {
        printf("Error: no active scope\n");
        exit(1);
    }

    int idx = scope_stack[scope_top];
    Scope *s = &scopes[idx];

    s->is_used = 1;

    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->symbols[i].name, name) == 0) {
            report_semantic_error("redeclaration of", name);
        }
    }

    Symbol sym;
    strcpy(sym.name, name);
    strcpy(sym.kind, kind);
    strcpy(sym.type, type);
    sym.size = size;
    sym.param_count = 0;
    sym.is_used = 1;

    for (int i = 0; i < MAX_PARAMS; i++) {
        sym.param_types[i][0] = '\0';
    }

    s->symbols[s->count++] = sym;
}

/* ---------------- INSERT FUNCTION ---------------- */

void insert_function(char *name, char *return_type) {
    if (scope_top < 0) {
        printf("Error: no active scope\n");
        exit(1);
    }

    int idx = scope_stack[scope_top];
    Scope *s = &scopes[idx];

    s->is_used = 1;

    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->symbols[i].name, name) == 0) {
            report_semantic_error("redeclaration of function", name);
        }
    }

    Symbol sym;
    strcpy(sym.name, name);
    strcpy(sym.kind, "function");
    strcpy(sym.type, return_type);
    sym.size = 0;
    sym.param_count = 0;
    sym.is_used = 1;

    for (int i = 0; i < MAX_PARAMS; i++) {
        sym.param_types[i][0] = '\0';
    }

    s->symbols[s->count++] = sym;
}

/* ---------------- ADD PARAM ---------------- */

void add_param(char *func_name, char *param_type) {
    for (int i = scope_top; i >= 0; i--) {
        int idx = scope_stack[i];
        Scope *s = &scopes[idx];

        for (int j = 0; j < s->count; j++) {
            if (strcmp(s->symbols[j].name, func_name) == 0 &&
                strcmp(s->symbols[j].kind, "function") == 0) {

                Symbol *target = &s->symbols[j];

                if (target->param_count >= MAX_PARAMS) {
                    report_semantic_error("too many parameters", NULL);
                }

                strcpy(target->param_types[target->param_count], param_type);
                target->param_count++;
                return;
            }
        }
    }

    report_semantic_error("function not found", func_name);
}

/* ---------------- LOOKUP ---------------- */

Symbol* lookup(char *name) {
    for (int i = scope_top; i >= 0; i--) {
        int idx = scope_stack[i];
        Scope *s = &scopes[idx];

        for (int j = 0; j < s->count; j++) {
            if (strcmp(s->symbols[j].name, name) == 0) {
                return &s->symbols[j];
            }
        }
    }

    report_semantic_error("undeclared identifier", name);
    return NULL;
}

/* ---------------- PRINT ---------------- */

void print_symbol_table() {
    printf("\n===== SYMBOL TABLE =====\n");

    for (int i = 0; i < total_scopes; i++) {
        if (scopes[i].count == 0) continue;  // remove empty scopes

        printf("Scope %d:\n", i);

        for (int j = 0; j < scopes[i].count; j++) {
            Symbol *s = &scopes[i].symbols[j];

            printf("  %s | %s | %s",
                   s->name,
                   s->kind,
                   s->type);

            if (strcmp(s->kind, "array") == 0) {
                printf(" | size=%d", s->size);
            }

            if (strcmp(s->kind, "function") == 0) {
                printf(" | params=%d", s->param_count);
            }

            printf("\n");
        }
    }

    printf("========================\n");
}