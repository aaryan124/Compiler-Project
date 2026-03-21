#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define MAX_SCOPES 100
#define MAX_SYMBOLS 100
#define MAX_PARAMS 10

typedef struct {
    char name[50];
    char kind[20];   // variable, function, parameter, array
    char type[20];   // int, void, etc.
    int size;

    int param_count;
    char param_types[MAX_PARAMS][20];

    /* NEW: mark if scope has real symbols */
    int is_used;
} Symbol;

typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int count;

    /* NEW: mark if scope is meaningful */
    int is_used;
} Scope;

/* Globals */
extern Scope scopes[MAX_SCOPES];
extern int total_scopes;

/* Scope stack */
extern int scope_stack[MAX_SCOPES];
extern int scope_top;

/* Current function */
extern char current_function[50];

/* API */
void enter_scope();
void exit_scope();

void insert_symbol(char *name, char *kind, char *type, int size);
void insert_function(char *name, char *return_type);

void add_param(char *func_name, char *param_type);

Symbol* lookup(char *name);

void print_symbol_table();

#endif