#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source.py>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) { perror("Failed to open file"); return 1; }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(size + 1);
    fread(source, 1, size, file);
    source[size] = '\0';
    fclose(file);

    Lexer lexer;
    init_lexer(&lexer, source);
    scan_tokens(&lexer);

    for (int i = 0; i < lexer.token_count; i++) {
        print_token(&lexer.tokens[i]);
    }

    free_lexer(&lexer);
    free(source);
    return 0;
}