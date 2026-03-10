#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper: Add token to the lexer array
void add_token(Lexer* lexer, TokenType type) {
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity *= 2;
        lexer->tokens = realloc(lexer->tokens, lexer->token_capacity * sizeof(Token));
    }

    Token* t = &lexer->tokens[lexer->token_count++];
    t->type = type;
    t->line = lexer->line;
    t->column = lexer->column;

    int length = lexer->current - lexer->start;
    t->lexeme = malloc(length + 1);
    memcpy(t->lexeme, lexer->source + lexer->start, length);
    t->lexeme[length] = '\0';
}

// Error reporting
void report_error(Lexer* lexer, const char* message) {
    fprintf(stderr, "Lexical error at line %d, column %d: %s\n",
            lexer->line, lexer->column, message);
}

// Character traversal
char advance(Lexer* lexer) {
    char c = lexer->source[lexer->current++];

    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    return c;
}

char peek(Lexer* lexer) {
    return lexer->source[lexer->current];
}

char peek_next(Lexer* lexer) {
    if (lexer->source[lexer->current] == '\0')
        return '\0';

    return lexer->source[lexer->current + 1];
}

bool match(Lexer* lexer, char expected) {
    if (peek(lexer) != expected)
        return false;

    advance(lexer);
    return true;
}

bool is_at_end(Lexer* lexer) {
    return lexer->source[lexer->current] == '\0';
}

// Keyword recognition
TokenType identifier_type(const char* lexeme) {

    static struct {
        const char* word;
        TokenType type;
    } keywords[] = {

        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"elif", TOKEN_ELIF},
        {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR},
        {"def", TOKEN_DEF},
        {"return", TOKEN_RETURN},
        {"print", TOKEN_PRINT},
        {"in", TOKEN_IN},
        {"import", TOKEN_IMPORT},
        {"from", TOKEN_FROM},
        {"as", TOKEN_AS},
        {"class", TOKEN_CLASS},
        {"True", TOKEN_TRUE},
        {"False", TOKEN_FALSE},
        {"None", TOKEN_NONE},
        {"and", TOKEN_AND},
        {"or", TOKEN_OR},
        {"not", TOKEN_NOT},

        {NULL, TOKEN_IDENTIFIER}
    };

    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(lexeme, keywords[i].word) == 0)
            return keywords[i].type;
    }

    return TOKEN_IDENTIFIER;
}

// Handle Python-style indentation
void handle_indentation(Lexer* lexer) {

    while (1) {

        int indent = 0;

        while (!is_at_end(lexer)) {

            char c = peek(lexer);

            if (c == ' ') {
                indent++;
                advance(lexer);
            }

            else if (c == '\t') {
                indent += TAB_WIDTH;
                advance(lexer);
            }

            else break;
        }

        if (is_at_end(lexer))
            break;

        char c = peek(lexer);

        if (c == '#') {

            while (!is_at_end(lexer) && peek(lexer) != '\n')
                advance(lexer);

            if (!is_at_end(lexer) && peek(lexer) == '\n') {
                advance(lexer);
                continue;
            }
        }

        else if (c == '\n') {
            advance(lexer);
            continue;
        }

        else {

            int top = lexer->indent_stack[lexer->indent_top - 1];

            if (indent > top) {

                if (lexer->indent_top >= lexer->indent_capacity) {

                    lexer->indent_capacity *= 2;

                    lexer->indent_stack =
                        realloc(lexer->indent_stack,
                                lexer->indent_capacity * sizeof(int));
                }

                lexer->indent_stack[lexer->indent_top++] = indent;
                add_token(lexer, TOKEN_INDENT);
            }

            else if (indent < top) {

                while (lexer->indent_top > 0 &&
                       lexer->indent_stack[lexer->indent_top - 1] > indent) {

                    lexer->indent_top--;
                    add_token(lexer, TOKEN_DEDENT);
                }

                if (lexer->indent_stack[lexer->indent_top - 1] != indent) {

                    report_error(lexer, "Inconsistent indentation");
                    add_token(lexer, TOKEN_ERROR);
                }
            }

            break;
        }
    }

    lexer->bol = false;
}

// Token scanner
void scan_token(Lexer* lexer) {

    if (lexer->bol) {
        handle_indentation(lexer);
        return;
    }

    else {

        // Skip whitespace and inline comments
        while (!is_at_end(lexer)) {

            char c = peek(lexer);

            if (c == ' ' || c == '\t' || c == '\r') {
                advance(lexer);
            }

            else if (c == '#') {
                while (!is_at_end(lexer) && peek(lexer) != '\n')
                    advance(lexer);
            }

            else
                break;
        }
    }

    if (is_at_end(lexer))
        return;

    lexer->start = lexer->current;
    char c = advance(lexer);

    switch (c) {

        case '#':
            while (!is_at_end(lexer) && peek(lexer) != '\n')
                advance(lexer);
            break;

        case '(':
            add_token(lexer, TOKEN_LPAREN);
            break;

        case ')':
            add_token(lexer, TOKEN_RPAREN);
            break;

        case '[':
            add_token(lexer, TOKEN_LBRACKET);
            break;

        case ']':
            add_token(lexer, TOKEN_RBRACKET);
            break;

        case '{':
            add_token(lexer, TOKEN_LBRACE);
            break;

        case '}':
            add_token(lexer, TOKEN_RBRACE);
            break;

        case ',':
            add_token(lexer, TOKEN_COMMA);
            break;

        case '.':
            add_token(lexer, TOKEN_DOT);
            break;

        case ':':
            add_token(lexer, TOKEN_COLON);
            break;

        case '+':
            add_token(lexer, TOKEN_PLUS);
            break;

        case '-':
            add_token(lexer, TOKEN_MINUS);
            break;

        case '*':
            add_token(lexer, TOKEN_STAR);
            break;

        case '/':
            add_token(lexer, TOKEN_SLASH);
            break;

        case '%':
            add_token(lexer, TOKEN_PERCENT);
            break;

        case '&':
            add_token(lexer, TOKEN_BIT_AND);
            break;

        case '|':
            add_token(lexer, TOKEN_BIT_OR);
            break;

        case '^':
            add_token(lexer, TOKEN_BIT_XOR);
            break;

        case '~':
            add_token(lexer, TOKEN_BIT_NOT);
            break;

        case '=':
            add_token(lexer, match(lexer, '=') ? TOKEN_EQUAL : TOKEN_ASSIGN);
            break;

        case '!':
            if (match(lexer, '='))
                add_token(lexer, TOKEN_NOT_EQUAL);
            else
                add_token(lexer, TOKEN_ERROR);
            break;

        case '<':
            if (match(lexer, '<'))
                add_token(lexer, TOKEN_SHIFT_LEFT);
            else if (match(lexer, '='))
                add_token(lexer, TOKEN_LESS_EQUAL);
            else
                add_token(lexer, TOKEN_LESS);
            break;

        case '>':
            if (match(lexer, '>'))
                add_token(lexer, TOKEN_SHIFT_RIGHT);
            else if (match(lexer, '='))
                add_token(lexer, TOKEN_GREATER_EQUAL);
            else
                add_token(lexer, TOKEN_GREATER);
            break;

        case '\n':
            add_token(lexer, TOKEN_NEWLINE);
            lexer->bol = true;
            break;

        case '"':
        case '\'':
        {
            char quote = c;

            while (!is_at_end(lexer) && peek(lexer) != quote) {

                if (peek(lexer) == '\\')
                    advance(lexer);

                advance(lexer);
            }

            if (is_at_end(lexer)) {

                report_error(lexer, "Unterminated string");
                add_token(lexer, TOKEN_ERROR);
            }

            else {

                advance(lexer);
                add_token(lexer, TOKEN_STRING);
            }

            break;
        }

        default:

            if (isdigit(c)) {

                while (isdigit(peek(lexer)))
                    advance(lexer);

                if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {

                    advance(lexer);

                    while (isdigit(peek(lexer)))
                        advance(lexer);
                }

                add_token(lexer, TOKEN_NUMBER);
            }

            else if (isalpha(c) || c == '_') {

                while (isalnum(peek(lexer)) || peek(lexer) == '_')
                    advance(lexer);

                int len = lexer->current - lexer->start;

                char* text = malloc(len + 1);
                memcpy(text, lexer->source + lexer->start, len);
                text[len] = '\0';

                add_token(lexer, identifier_type(text));

                free(text);
            }

            else {

                report_error(lexer, "Unexpected character");
                add_token(lexer, TOKEN_ERROR);
            }

            break;
    }
}

// Lexer initialization
void init_lexer(Lexer* lexer, char* source) {

    lexer->source = source;
    lexer->start = 0;
    lexer->current = 0;

    lexer->line = 1;
    lexer->column = 1;

    lexer->bol = true;

    lexer->token_count = 0;
    lexer->token_capacity = INIT_TOKEN_CAPACITY;

    lexer->tokens = malloc(lexer->token_capacity * sizeof(Token));

    lexer->indent_capacity = INIT_INDENT_CAPACITY;
    lexer->indent_stack = malloc(lexer->indent_capacity * sizeof(int));

    lexer->indent_top = 0;
    lexer->indent_stack[lexer->indent_top++] = 0;
}

// Free memory
void free_lexer(Lexer* lexer) {

    for (int i = 0; i < lexer->token_count; i++)
        free(lexer->tokens[i].lexeme);

    free(lexer->tokens);
    free(lexer->indent_stack);
}

// Scan entire source
void scan_tokens(Lexer* lexer) {

    while (!is_at_end(lexer))
        scan_token(lexer);

    while (lexer->indent_top > 1) {

        lexer->indent_top--;
        add_token(lexer, TOKEN_DEDENT);
    }

    lexer->start = lexer->current;
    add_token(lexer, TOKEN_EOF);
}

// Token printing
void print_token(const Token* token) {

    const char* names[] = {

        "IDENTIFIER","NUMBER","STRING",
        "IF","ELSE","ELIF","WHILE","FOR","DEF","RETURN","PRINT","IN","IMPORT",
        "FROM","AS","CLASS","TRUE","FALSE","NONE",
        "PLUS","MINUS","STAR","SLASH","PERCENT",
        "ASSIGN","EQUAL","NOT_EQUAL","LESS","GREATER","LESS_EQUAL","GREATER_EQUAL",
        "AND","OR","NOT",
        "BIT_AND","BIT_OR","BIT_XOR","BIT_NOT","SHIFT_LEFT","SHIFT_RIGHT",
        "LPAREN","RPAREN","LBRACKET","RBRACKET","LBRACE","RBRACE",
        "COMMA","DOT","COLON",
        "NEWLINE","INDENT","DEDENT","EOF","ERROR"
    };

    printf("[%d:%d] %s (%s)\n",
           token->line,
           token->column,
           names[token->type],
           token->lexeme);
}