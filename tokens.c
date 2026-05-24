#pragma once

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "helpers.c"

static char* g_text;
static char* g_filename;
static size_t g_line_num;
static size_t g_char_num;

#define TOKEN_ERROR(...) error_with_info(g_filename, g_line_num, g_char_num, __VA_ARGS__)

typedef enum {
    // Rule identifiers, like 'expr'
    T_IDENT,
    
    // Rule decorators
    T_EQ,    // =
    T_STAR,  // *
    T_DOT,   // .
    T_PLUS,  // +
    T_PIPE,  // |
    T_QUEST, // ?
    T_BANG,  // !
    T_HASH,  // #
    T_OPEN,  // (
    T_CLOSE, // )
    T_NEWLINE, // \n
    
    // Ranged options, like [A-Z], [xyz], [0-9a-f]
    T_RANGE,

    // Symbol literals, like 'define', '(', ')'
    T_STR,

    // End of File
    T_EOF
} TokenType;

typedef struct Token Token;
struct Token {
    TokenType typ;
    String_View value;
    Token* next;
};

Token* new_token(TokenType typ, char* q, char* p) {
    Token* token = calloc(1, sizeof(Token));

    token->typ = typ;
    token->value = (String_View) { .str = q, .len = p - q };
    token->next = NULL;
}


char* str_tokentype(TokenType t) {
    char* token_names[] = {
        "T_IDENT",
        "T_EQ",
        "T_STAR",
        "T_DOT",
        "T_PLUS",
        "T_PIPE",
        "T_QUEST",
        "T_BANG",
        "T_HASH",
        "T_OPEN",
        "T_CLOSE",
        "T_NEWLINE",
        "T_RANGE",
        "T_STR",
        "T_EOF"
    };

    return token_names[t];
}

void print_token(Token t) {
    printf("%s\t", str_tokentype(t.typ));
    printf("%.*s\n", (int) t.value.len, t.value.str);
}


char* next(char* p) {
    if (*p == '\n') {
        g_line_num++;
        g_char_num = 1;
    }
    else g_char_num++;

    return p + 1;
}


Token* tokenise(char* text);
Token* tokenise_file(char* filename) {

    // TOKEN_ERROR("This is a test error %s %s\n", "beep", "boop");

    g_filename = filename;
    char* text = read_file(filename);

    return tokenise(text);
}

Token* tokenise(char* text) {

    char* p = g_text = text;

    Token head = { .next = NULL };
    Token* cur = &head;

    g_line_num = g_char_num = 1;
    while (*p) {

        // Parse comments as newlines
        if (*p == '/' && *(p + 1) == '/') {
            p = next(next(p));
            while (*p != '\n' && *p != '\0') p = next(p);
            cur = cur->next = new_token(T_NEWLINE, p, p);
        }

        // Parse newlines
        else if (*p == '\n') {
            cur = cur->next = new_token(T_NEWLINE, p, p);
            p = next(p);
        }

        // Ignore whitespaces
        else if (isspace((int) *p)) {
            p = next(p);
        }

        // Process rule names
        else if (isalpha(*p)) {
            char* q = p;
            do p = next(p);
            while (isalnum(*p) || *p == '-' || *p == '_');
            cur = cur->next = new_token(T_IDENT, q, p);
        }

        // Process punctuation
        else if (*p == '=') { cur = cur->next = new_token(T_EQ, p, p + 1); p = next(p); }
        else if (*p == '|') { cur = cur->next = new_token(T_PIPE, p, p + 1); p = next(p); }
        else if (*p == '+') { cur = cur->next = new_token(T_PLUS, p, p + 1); p = next(p); }
        else if (*p == '?') { cur = cur->next = new_token(T_QUEST, p, p + 1); p = next(p); }
        else if (*p == '!') { cur = cur->next = new_token(T_BANG, p, p + 1); p = next(p); }
        else if (*p == '#') { cur = cur->next = new_token(T_HASH, p, p + 1); p = next(p); }
        else if (*p == '*') { cur = cur->next = new_token(T_STAR, p, p + 1); p = next(p); }
        else if (*p == '(') { cur = cur->next = new_token(T_OPEN, p, p + 1); p = next(p); }
        else if (*p == ')') { cur = cur->next = new_token(T_CLOSE, p, p + 1); p = next(p); }
        else if (*p == '.') { cur = cur->next = new_token(T_DOT, p, p + 1); p = next(p); }

        // Process string literals
        else if (*p == '\'' || *p == '\"') {
            char* q = p;
            char quote = *p;
            while (*(p = next(p)) != quote) {
                if (*p == '\\') p = next(p);
                else if (*p == '\0' || *p == '\n') TOKEN_ERROR("Unclosed string literal. Expected matching (%c).\n", quote);
            }
            cur = cur->next = new_token(T_STR, q, p = next(p));
        }

        // Process range literals
        else if (*p == '[') {
            char* q = p;
            while (*(p = next(p)) != ']') {
                if (*p == '\0' || *p == '\n') TOKEN_ERROR("Unclosed range literal.\n");
            }
            cur = cur->next = new_token(T_RANGE, q, p = next(p));
        }

        else {
            // throw error?
            p = next(p);
        }
    }

    if (cur->typ != T_NEWLINE) 
        // Inject new line at the end if there isn't any
        cur = cur->next = new_token(T_NEWLINE, p, p);
    
    cur = cur->next = new_token(T_EOF, p, p);
    return head.next;
}