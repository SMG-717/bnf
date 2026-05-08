#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static char* g_text;


typedef enum TokenType TokenType;
enum TokenType {
    // Rule identifiers, like 'expr'
    T_IDENT,
    
    // Rule decorators
    T_EQ,    // =
    T_STAR,  // *
    T_PLUS,  // +
    T_PIPE,  // |
    T_QUEST, // ?
    T_OPEN,  // (
    T_CLOSE, // )
    
    // Ranged options, like [A-Z], [xyz], [0-9a-f]
    T_RANGE,

    // Symbol literals, like 'define', '(', ')'
    T_STR,
    
    // End of File
    T_EOF
};

typedef struct Token Token;
struct Token {
    TokenType typ;
    char* loc;
    size_t len;
    Token* next;
};

Token* new_token(TokenType typ, char* q, char* p) {
    Token* token = calloc(1, sizeof(Token));
    token->typ = typ;
    token->loc = q;
    token->len = p - q;
}

void error(char* message) {
    printf("%s", message);
    exit(1);
}

void print_token(Token t) {
    char* token_names[] = {
        "T_IDENT",
        "T_EQ\t =",
        "T_STAR\t *",
        "T_PLUS\t +",
        "T_PIPE\t |",
        "T_QUEST\t ?",
        "T_OPEN\t (",
        "T_CLOSE\t )",
        "T_RANGE",
        "T_STR",
        "T_EOF"
    };

    printf("%s\t %.*s\n", token_names[t.typ], (int) t.len, (char *) t.loc);
}

Token* tokenise(char* text) {
    g_text = text;
    Token head = { };

    Token* cur = &head;
    char* p = g_text;
    while (*p) {

        // Ignore comments
        if (*p == '/' && *(p + 1) == '/') {
            p += 2;
            while (*p != '\n' && *p != '\0') p += 1;
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
        else if (*p == '=') cur = cur->next = new_token(T_EQ, p, ++p);
        else if (*p == '|') cur = cur->next = new_token(T_PIPE, p, ++p);
        else if (*p == '+') cur = cur->next = new_token(T_PLUS, p, ++p);
        else if (*p == '?') cur = cur->next = new_token(T_QUEST, p, ++p);
        else if (*p == '*') cur = cur->next = new_token(T_STAR, p, ++p);
        else if (*p == '(') cur = cur->next = new_token(T_OPEN, p, ++p);
        else if (*p == ')') cur = cur->next = new_token(T_CLOSE, p, ++p);
        
        // Process string literals
        else if (*p == '\'') {
            char* q = p;
            while (*++p != '\'') {
                if (*p == '\0' || *p == '\n') error("Unclosed string literal.\n");
            }
            cur = cur->next = new_token(T_STR, q, ++p);
        }
        
        // Process string literals
        else if (*p == '"') {
            char* q = p;
            while (*++p != '"') {
                if (*p == '\0' || *p == '\n') error("Unclosed string literal.\n");
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
            p += 1;
        }
    }

    cur = cur->next = new_token(T_EOF, p, p);
    return head.next;
}