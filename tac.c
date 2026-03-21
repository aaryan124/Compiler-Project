#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"

TAC* make_tac(char* op, char* a1, char* a2, char* res) {
    TAC* node = (TAC*)malloc(sizeof(TAC));

    strcpy(node->op, op ? op : "");
    strcpy(node->arg1, a1 ? a1 : "");
    strcpy(node->arg2, a2 ? a2 : "");
    strcpy(node->result, res ? res : "");

    node->next = NULL;
    return node;
}

TAC* append_tac(TAC* a, TAC* b) {
    if (!a) return b;
    if (!b) return a;

    TAC* temp = a;
    while (temp->next) temp = temp->next;
    temp->next = b;

    return a;
}

void print_tac(TAC* head) {
    printf("\n===== THREE ADDRESS CODE =====\n");

    while (head) {
        if (strcmp(head->op, "label") == 0) {
            printf("%s:\n", head->result);
        }
        else if (strcmp(head->op, "goto") == 0) {
            printf("goto %s\n", head->result);
        }
        else if (strcmp(head->op, "ifFalse") == 0) {
            printf("ifFalse %s goto %s\n", head->arg1, head->result);
        }
        else if (strcmp(head->op, "param") == 0) {
            printf("param %s\n", head->arg1);
        }
        else if (strcmp(head->op, "call") == 0) {
            if (strlen(head->arg2) > 0) {
                printf("%s = call %s, %s\n", head->result, head->arg1, head->arg2);
            } else {
                printf("%s = call %s\n", head->result, head->arg1);
            }
        }
        else if (strcmp(head->op, "return") == 0) {
            printf("return %s\n", head->arg1);
        }
        else if (strcmp(head->op, "print") == 0) {
            printf("print %s\n", head->arg1);
        }
        else if (strcmp(head->op, "scan") == 0) {
            printf("scan %s\n", head->arg1);
        }
        else {
            if (strcmp(head->op, "=") == 0) {
                printf("%s = %s\n", head->result, head->arg1);
            } else if (strlen(head->arg2) == 0) {
                printf("%s = %s %s\n", head->result, head->op, head->arg1);
            } else {
                printf("%s = %s %s %s\n",
                    head->result,
                    head->arg1,
                    head->op,
                    head->arg2);
            }
        }

        head = head->next;
    }

    printf("==============================\n");
}