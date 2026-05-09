#pragma once

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "helpers.c"

static char* g_text;


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

void error(char* message) {
    printf("%s", message);
    exit(1);
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

Token* tokenise(char* text) {
    Token head = { .next = NULL };
    Token* cur = &head;
    char* p = g_text = text;
    while (*p) {

        // Parse comments as newlines
        if (*p == '/' && *(p + 1) == '/') {
            p += 2;
            while (*p != '\n' && *p != '\0') p += 1;
            cur = cur->next = new_token(T_NEWLINE, p, p);
        }

        // Parse newlines
        else if (*p == '\n') {
            cur = cur->next = new_token(T_NEWLINE, p, p);
            p += 1;
        }

        // Ignore whitespaces
        else if (isspace((int) *p)) {
            p += 1;
        }

        // Process rule names
        else if (isalpha(*p)) {
            char* q = p++;
            while (isalnum(*p) || *p == '-' || *p == '_') p++;
            cur = cur->next = new_token(T_IDENT, q, p);
        }

        // Process punctuation
        else if (*p == '=') { cur = cur->next = new_token(T_EQ, p, p + 1); p++; }
        else if (*p == '|') { cur = cur->next = new_token(T_PIPE, p, p + 1); p++; }
        else if (*p == '+') { cur = cur->next = new_token(T_PLUS, p, p + 1); p++; }
        else if (*p == '?') { cur = cur->next = new_token(T_QUEST, p, p + 1); p++; }
        else if (*p == '*') { cur = cur->next = new_token(T_STAR, p, p + 1); p++; }
        else if (*p == '(') { cur = cur->next = new_token(T_OPEN, p, p + 1); p++; }
        else if (*p == ')') { cur = cur->next = new_token(T_CLOSE, p, p + 1); p++; }
        else if (*p == '.') { cur = cur->next = new_token(T_DOT, p, p + 1); p++; }

        // Process string literals
        else if (*p == '\'' || *p == '\"') {
            char* q = p;
            char quote = *p;
            while (*++p != quote) {
                if (*p == '\\') p++;
                else if (*p == '\0' || *p == '\n') error("Unclosed string literal.\n");
            }
            cur = cur->next = new_token(T_STR, q, ++p);
        }

        // Process range literals
        else if (*p == '[') {
            char* q = p;
            while (*++p != ']') {
                if (*p == '\0' || *p == '\n') error("Unclosed range literal.\n");
            }
            cur = cur->next = new_token(T_RANGE, q, ++p);
        }

        else {
            // throw error?
            p += 1;
        }
    }

    if (cur->typ != T_NEWLINE) 
        // Inject new line at the end if there isn't any
        cur = cur->next = new_token(T_NEWLINE, p, p);
    
    cur = cur->next = new_token(T_EOF, p, p);
    return head.next;
}