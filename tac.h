#ifndef TAC_H
#define TAC_H

typedef struct TAC {
    char op[32];     // +, -, *, /, <=, ifFalse, goto, call, param, return, label
    char arg1[64];
    char arg2[64];
    char result[64];

    struct TAC* next;
} TAC;

TAC* make_tac(char* op, char* a1, char* a2, char* res);
TAC* append_tac(TAC* a, TAC* b);
void print_tac(TAC* head);

#endif