// #include <stdio.h>
// #include <stdlib.h>
// #include "ast.h"
// #include "symbol_table.h"
// #include "parser.tab.h"

// extern FILE *yyin;
// extern int yylex();
// extern int yylineno;
// extern char* yytext;
// extern const char* get_token_name(int token);

// int yyparse();
// extern ASTNode* root;

// void print_token_stream(const char* filename) {
//     FILE* fp = fopen(filename, "r");
//     if (!fp) {
//         printf("Error: Cannot open file %s\n", filename);
//         return;
//     }

//     yyin = fp;

//     printf("\n--- TOKEN STREAM ---\n");
//     printf("%-20s | %-20s | %-10s\n", "TOKEN", "LEXEME", "LINE");
//     printf("------------------------------------------------\n");

//     int token;
//     while ((token = yylex()) != 0) {
//         printf("%-20s | %-20s | %-10d\n",
//                get_token_name(token), yytext, yylineno);
//     }

//     fclose(fp);
//     yylineno = 1;
// }

// int main(int argc, char **argv) {
//     if (argc < 2) {
//         printf("Usage: %s <file>\n", argv[0]);
//         return 1;
//     }

//     print_token_stream(argv[1]);

//     yyin = fopen(argv[1], "r");
//     if (!yyin) {
//         printf("Cannot open file\n");
//         return 1;
//     }

//     enter_scope(); /* global scope */

//     if (yyparse() == 0) {
//         printf("\nParsing Successful!\n");

//         if (root) {
//             printf("\n--- AST ---\n");
//             print_ast(root, 0);
//         }

//         print_symbol_table();
//     } else {
//         printf("\nParsing Failed!\n");
//     }

//     fclose(yyin);
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "symbol_table.h"
#include "parser.tab.h"

#include "tac_gen.h"
#include "tac.h"
#include "optimizer.h"
#include "codegen.h"

extern FILE *yyin;
extern int yylex();
extern int yylineno;
extern char* yytext;
extern const char* get_token_name(int token);

int yyparse();
extern ASTNode* root;

/* ================= ERROR HANDLING ================= */
const char* source_filename = NULL;

void print_error_line(int line) {
    if (!source_filename) return;
    FILE *f = fopen(source_filename, "r");
    if (!f) return;
    char buffer[1024];
    int current = 1;
    while (fgets(buffer, sizeof(buffer), f)) {
        if (current == line) {
            printf("    | %s", buffer);
            if (buffer[strlen(buffer)-1] != '\n') printf("\n");
            break;
        }
        current++;
    }
    fclose(f);
}

/* ================= TOKEN STREAM ================= */

void print_token_stream(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    yyin = fp;

    printf("\n--- TOKEN STREAM ---\n");
    printf("%-20s | %-20s | %-10s\n", "TOKEN", "LEXEME", "LINE");
    printf("------------------------------------------------\n");

    int token;
    while ((token = yylex()) != 0) {
        printf("%-20s | %-20s | %-10d\n",
               get_token_name(token),
               yytext,
               yylineno);
    }

    fclose(fp);
    yylineno = 1;
}

/* ================= MAIN ================= */

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    source_filename = argv[1];

    /* 1. Token stream */
    print_token_stream(argv[1]);

    /* 2. Parsing setup */
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        printf("Cannot open file\n");
        return 1;
    }

    /* 3. Global scope */
    enter_scope();

    /* 4. Parse */
    if (yyparse() == 0) {
        printf("\nParsing Successful!\n");

        /* 5. AST */
        if (root) {
            printf("\n--- AST ---\n");
            print_ast(root, 0);

            /* 6. SYMBOL TABLE */
            print_symbol_table();

            /* 7. TAC GENERATION */
            TAC* code = generate_tac(root);
            print_tac(code);

            /* 8. OPTIMISATION */
            TAC* opt_code = optimize(code);
            print_optimized_tac(opt_code);

            /* 9. X86 CODE GENERATION ⭐ NEW */
            generate_x86(opt_code, "output.asm");
            printf("[codegen] Assemble & link: nasm -f win32 output.asm -o output.obj && gcc -m32 output.obj -o program.exe\n");
        }
    } else {
        printf("\nParsing Failed!\n");
    }

    fclose(yyin);
    return 0;
}