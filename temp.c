#include <stdio.h>
#include <stdlib.h>

int temp_count = 0;
int label_count = 0;

char* new_temp() {
    char* t = malloc(16);
    sprintf(t, "t%d", temp_count++);
    return t;
}

char* new_label() {
    char* l = malloc(16);
    sprintf(l, "L%d", label_count++);
    return l;
}