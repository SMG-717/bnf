#pragma once

#include <stdarg.h>
#include <stdlib.h>

#define da_append(xs, x) \
do {\
    if ((xs).count >= (xs).capacity) {\
        if ((xs).capacity == 0) (xs).capacity = 256;\
        else (xs).capacity *= 2;\
        (xs).items = realloc((xs).items, (xs).capacity * sizeof(*(xs).items));\
    }\
    (xs).items[(xs).count++] = (x);\
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

void error(char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    printf(fmt, args);
    va_end (args);
    exit(1);
}

// void error_with_info(char* filename, size_t line_num, size_t char_num, char* fmt, int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9) {
//     printf("%s:%ld:%ld: ", filename, line_num, char_num);
//     printf(fmt, x0, x1, x2, x3, x4, x5, x6, x7, x8, x9);

//     exit(1);
// }

void error_with_info(char* filename, size_t line_num, size_t char_num, char* fmt, ...) {
    printf("%s:%ld:%ld: ", filename, line_num, char_num);

    va_list args;
    char buf[1000];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printf("%s", buf);

    exit(1);
}


char* read_file(const char* path) {
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
