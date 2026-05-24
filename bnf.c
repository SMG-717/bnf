#include <stdio.h>

#include "tokens.c"
#include "parse.c"
#include "apply.c"

void print_tokens(Token* t) {
    do print_token(*t);
    while (t = t->next);
}


int main(int argc, char* args[]) {

    if (argc < 3) {
        error("Expected at least two filename arguments.\n");
    }

    char* grammar_file = args[1];

    Token* token = tokenise_file(grammar_file);
    Syntax* syntax = parse_syntax(token);
    print_syntax(syntax);

    char* script_file = args[2];
    char* script_text = read_file(script_file);

    Node *ast = apply_syntax(*syntax, script_text);
    printf("\n%s\n", str_node(ast));

    return 0;
}
