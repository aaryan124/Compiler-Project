#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdbool.h>

#define TAB_WIDTH 8
#define INIT_TOKEN_CAPACITY 64
#define INIT_INDENT_CAPACITY 8

typedef enum {

    // Identifiers & literals
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,

    // Keywords
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DEF,
    TOKEN_RETURN,
    TOKEN_PRINT,
    TOKEN_IN,
    TOKEN_IMPORT,
    TOKEN_FROM,
    TOKEN_AS,
    TOKEN_CLASS,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NONE,

    // Arithmetic operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,

    // Assignment / comparison
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,

    // Logical operators
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    // Bitwise operators
    TOKEN_BIT_AND,      // &
    TOKEN_BIT_OR,       // |
    TOKEN_BIT_XOR,      // ^
    TOKEN_BIT_NOT,      // ~
    TOKEN_SHIFT_LEFT,   // <<
    TOKEN_SHIFT_RIGHT,  // >>

    // Delimiters
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,

    // Special tokens
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_EOF,
    TOKEN_ERROR

} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int column;
} Token;

typedef struct {
    char* source;
    int start;
    int current;
    int line;
    int column;

    Token* tokens;
    int token_count;
    int token_capacity;

    int* indent_stack;
    int indent_top;
    int indent_capacity;

    bool bol;   // beginning of line

} Lexer;

void init_lexer(Lexer* lexer, char* source);
void free_lexer(Lexer* lexer);
void scan_tokens(Lexer* lexer);
void print_token(const Token* token);

#endif