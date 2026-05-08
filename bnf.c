#include <stdio.h>
#include <stdlib.h>

#include "tokens.c"

#define da_append(xs, x) \
do {\
    if (xs.count >= xs.capacity) {\
        if (xs.capacity == 0) xs.capacity = 256;\
        else xs.capacity += 2;\
        xs.items = realloc(xs.items, xs.capacity * sizeof(*xs.items));\
    }\
    xs.items[xs.count++] = x;\
} while(0)

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





int main(int argc, char* args[]) {
    char* fd = read_file("./examples/main.bnf");

    Token* token = tokenise(fd);

    do {
        print_token(*token);
    } while (token = token->next);


    return 0;
}
