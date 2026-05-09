#pragma once
#define da_append(xs, x) \
do {\
    if (xs.count >= xs.capacity) {\
        if (xs.capacity == 0) xs.capacity = 256;\
        else xs.capacity += 2;\
        xs.items = realloc(xs.items, xs.capacity * sizeof(*xs.items));\
    }\
    xs.items[xs.count++] = x;\
} while(0)


typedef struct {
    char* str;
    size_t len;
} String_View;


char* view_to_cstr(String_View view) {
    char* str = malloc(sizeof(char) * view.len + 1);
    for (int i = 0; i < view.len; i++) {
        str[i] = view.str[i];
    }

    str[view.len] = '\0';
    return str;
}