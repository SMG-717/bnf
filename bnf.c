#include <stdio.h>
#include <stdlib.h>

#include "tokens.c"
#include "parse.c"


char* read_file(const char * path) {
    FILE *ptr = fopen(path, "r");

    fseek(ptr, 0L, SEEK_END);
    size_t size = ftell(ptr);    
    fseek(ptr, 0L, SEEK_SET);

    char* data = malloc(size + 1);
    fread(data, size, 1, ptr);

    data[size] = '\0';
    fclose(ptr);

    return data;
}

void print_tokens(Token* t) {
    do {
        print_token(*t);
    } while (t = t->next);
}


int main(int argc, char* args[]) {
    char* fd = read_file("./examples/main.bnf");

    Token* token = tokenise(fd);
    Syntax* syntax = parse_syntax(token);
    print_syntax(syntax);

    return 0;
}
