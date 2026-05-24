#pragma once

#include <stdlib.h>
#include "tokens.c"
#include "helpers.c"

static Token* g_token;
typedef struct Option Option;
typedef struct OptionList OptionList;
typedef struct Generator Generator;
typedef struct Rule Rule;

Option* parse_option();

typedef enum {
    G_ID,
    G_RANGE,
    G_STR,
    G_ANY,
    G_GROUP,
} GeneratorType;

typedef enum {
    MOD_NONE,
    MOD_OPTIONAL,
    MOD_ATLEAST_1,
    MOD_ATLEAST_0,
    // MOD_RANGED, // Not implemented for now
} CountModifier;


struct OptionList {
    Option* items;
    size_t count;
    size_t capacity;
};

struct Generator {
    GeneratorType typ;
    CountModifier mod;
    int negative;
    int invisible;

    // For ids and literals
    String_View value;

    // For literals
    char* str_value;

    // For groups
    OptionList options;

    // For ranges
    char min;
    char max;
};



struct Option {
    struct {
        Generator* items;
        size_t count;
        size_t capacity;
    } generators;
};


struct Rule {
    char* name;
    OptionList options;
};


typedef struct {
    struct {
        Rule* items;
        size_t count;
        size_t capacity;
    } rules;
} Syntax;


char* parse_string_literal(String_View view) {

    char* str = view_to_cstr(view);

    // Assume str[0] = str[len - 1] = ' or "
    int i = 0;
    int j = 1;
    while (j < view.len - 1) {
        if (str[j] == '\\') {
            switch (str[++j]) {
                case '\\':
                case '\'':
                case '\"': str[i++] = str[j]; break;
                case 't':  str[i++] = '\t'; break;
                case 'n':  str[i++] = '\n'; break;
                case 'v':  str[i++] = '\v'; break;
                case 'r':  str[i++] = '\r'; break;
                default: error("Invalid string literal");
            }
            j++;
        }
        else str[i++] = str[j++];

    }
    str[i] = '\0';
    return str;
}

Generator* parse_generator() {
    Generator* gen = calloc(1, sizeof(Generator));
    gen->negative = 0;
    gen->invisible = 0;

    for (int i = 0; i < 2; i++) {

        if (g_token->typ == T_BANG) {
            // Parse string literal generator
            gen->negative = 1;
            g_token = g_token->next;
        }
        else if (g_token->typ == T_HASH) {
            // Parse string literal generator
            gen->invisible = 1;
            g_token = g_token->next;
        }
    }

    if (g_token->typ == T_DOT) {
        // Parse string literal generator
        gen->value = g_token->value;
        gen->typ = G_ANY;
    }

    else if (g_token->typ == T_STR) {
        // Parse string literal generator
        gen->value = g_token->value;
        gen->typ = G_STR;
        gen->str_value = parse_string_literal(gen->value);
    }
    
    else if (g_token->typ == T_IDENT) {
        // Parse id generator
        gen->value = g_token->value;
        gen->typ = G_ID;
        gen->str_value = view_to_cstr(gen->value); // should?
    }

    else if (g_token->typ == T_RANGE) {
        // Parse range literal generator
        gen->typ = G_RANGE;

        // Ensure the range is well formed
        if (
            g_token->value.str[0] != '[' ||
            g_token->value.str[2] != '-' ||
            g_token->value.str[4] != ']'
        ) error("Malformed range generator.\n");

        gen->min = g_token->value.str[1];
        gen->max = g_token->value.str[3];
    }

    else if (g_token->typ == T_OPEN) {
        // Parse id generator
        gen->typ = G_GROUP;
        g_token = g_token->next;

        Option* option = parse_option();
        if (!option) {
            error("Expected group definition.\n");
        }
        da_append(gen->options, *option);
        
        while (g_token->typ == T_PIPE) {
            g_token = g_token->next;
            if (!(option = parse_option())) 
                error("Expected group option.\n");
            
            da_append(gen->options, *option);
        }

        if (g_token->typ != T_CLOSE) {
            error("Expected a matching closing parenthesis\n");
        }
    }

    else {

        if (gen->negative) {
            printf("Expected generator definition after '!'");
            error("Could not parse generator");
        }

        free(gen);
        return NULL;
        // printf("Unexpected token for generator: %s", str_tokentype(g_token->typ));
        // error("Could not parse generator");
    }
    g_token = g_token->next;

    // Parse modifier
    gen->mod = MOD_NONE;
    if (g_token->typ == T_STAR) {
        gen->mod = MOD_ATLEAST_0;
        g_token = g_token->next;
    }
    
    else if (g_token->typ == T_PLUS) {
        gen->mod = MOD_ATLEAST_1;
        g_token = g_token->next;
    }
    
    else if (g_token->typ == T_QUEST) {
        gen->mod = MOD_OPTIONAL;
        g_token = g_token->next;
    }

    if (gen->typ == G_ANY && (gen->mod == MOD_ATLEAST_0 || gen->mod == MOD_ATLEAST_1)) {
        printf("Error: '.' generators with variable modifiers ('*' or '+') are disallowed (for now).");
        error("Could not parse generator");
    }

    return gen;
}

Option* parse_option() {

    Option* option = calloc(1, sizeof(Option));

    Generator* gen = parse_generator();
    if (!gen) {
        error("Expected generator definition.\n");
    }

    do {
        da_append(option->generators, *gen);
    } while (gen = parse_generator());

    return option;
}

Rule* parse_rule() {

    if (g_token->typ == T_NEWLINE) {
        g_token = g_token->next;
        return NULL;
    }
    
    Rule* rule = calloc(1, sizeof(Rule));
    if (g_token->typ != T_IDENT) {
        error("Expected rule identifier.\n");
    }

    rule->name = view_to_cstr(g_token->value);
    if ((g_token = g_token->next)->typ != T_EQ) {
        error("Expected token '='\n");
    }
    else {
        g_token = g_token->next;
    }
    
    Option* option = parse_option();
    if (!option) {
        error("Expected rule definition.\n");
    }
    da_append(rule->options, *option);
    
    while (g_token->typ == T_PIPE) {
        g_token = g_token->next;
        if (!(option = parse_option())) 
            error("Expected rule option.\n");
        
        da_append(rule->options, *option);
    }
    
    if (g_token->typ != T_NEWLINE) {
        printf("Unexpected token: %s.\n", str_tokentype(g_token->typ));
        error("Expected '\\n' instead.\n");
    }
    g_token = g_token->next;

    return rule;
}


Syntax* parse_syntax(Token* tokens) {

    g_token = tokens;
    Syntax* syntax = calloc(1, sizeof(Syntax));
    Rule* r;
    while (g_token->typ != T_EOF) {
        if (r = parse_rule()) da_append(syntax->rules, *r);
    }

    return syntax;
}

void print_option(Option* option);
void print_generator(Generator* gen) {
    if (gen->invisible) printf("#");
    if (gen->negative) printf("!");
    if (gen->typ == G_GROUP) {
        printf("(");
        print_option(gen->options.items);
        for (int i = 1; i < gen->options.count; i++) {
            printf(" | ");
            print_option(&gen->options.items[i]);
        }
        printf(")");
    }
    else if (gen->typ == G_RANGE) {
        printf("[%c-%c]", gen->min, gen->max);
    }
    else {
        printf("%.*s", (int) gen->value.len, gen->value.str);
    }
    
    if (gen->mod == MOD_OPTIONAL) printf("?");
    else if (gen->mod == MOD_ATLEAST_0) printf("*");
    else if (gen->mod == MOD_ATLEAST_1) printf("+");
}

void print_option(Option* option) {
    print_generator(option->generators.items);
    for (int i = 1; i < option->generators.count; i++) {
        printf(" ");
        print_generator(&option->generators.items[i]);
    }
}

void print_rule(Rule* rule) {
    printf("  %s = ", rule->name);
    print_option(rule->options.items);
    for (int i = 1; i < rule->options.count; i++) {
        printf(" | ");
        print_option(&rule->options.items[i]);
    }
    printf("\n");
}


void print_syntax(Syntax* syn) {
    printf("Syntax:\n");
    for (int i = 0; i < syn->rules.count; i++) {
        print_rule(&syn->rules.items[i]);
    }
}