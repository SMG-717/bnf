#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helpers.c"
#include "parse.c"


// DEFs
typedef struct Array Array;
typedef struct Node Node;
typedef struct NodeArray NodeArray;

char* str_node(Node* node);
void __str_node(Node node, Array* builder, int indent);

struct Array {
    char* items; // int* is a stand-in for any type.
    size_t count;
    size_t capacity;
};

struct NodeArray {
    Node* items; // int* is a stand-in for any type.
    size_t count;
    size_t capacity;
};

struct Node {
    char* name;
    char* value;
    size_t len;
    NodeArray children;
};

static Node EMPTY_NODE = (Node) { .name = "EMPTY", .len = 0 };

Rule* find_rule_by_name(Syntax syn, const char* name);
Node* apply_rule(Syntax syn, Rule rule, String_View view);
Node* apply_gen(Syntax syn, Generator gen, String_View view);


Node* make_node() {
    Node* n = calloc(1, sizeof(Node));
    n->children = (NodeArray) { 0 };
    n->len = 0;
    n->value = NULL;
    n->name = NULL;
    return n;
}

void free_node(Node* node) {
    // for (int i = 0; i < node->children.count; i++) {
    //     free_node(&node->children.items[i]);
    // }

    free(node->children.items);
    free(node);
}


Rule* find_rule_by_name(Syntax syn, const char* name) {

    for (int i = 0; i < syn.rules.count; i++) {
        Rule* r = &syn.rules.items[i];
        if (strcmp(r->name, name) == 0)
            return r;
    }

    return NULL;

}


Node* apply_rule(Syntax syn, Rule rule, String_View view) {

    Node *rule_node, *result;
    String_View current;
    Generator gen;
    Option option;

    rule_node = make_node();
    rule_node->name = rule.name;

    for (int i = 0; i < rule.options.count; i++) {
        option = rule.options.items[i];

        current = view;
        int successful = 1;
        for (int j = 0; j < option.generators.count; j++) {
            gen = option.generators.items[j];
            result = apply_gen(syn, gen, current);

            if (result == NULL && gen.negative == 0 || result != NULL && gen.negative) {
                if (gen.mod != MOD_NONE) continue;

                successful = 0;
                break;
            }

            if (result == NULL) {
                result = make_node();
                result->len = 1;
                result->value = view_to_cstr((String_View) { .len = 1, .str = view.str });
            }

            if (gen.invisible == 0) {
                if (
                    rule_node->children.count == 1
                    && rule_node->children.items[0].name == NULL
                    && rule_node->children.items[0].children.count == 0
                    && result->name == NULL
                    && result->children.count == 0
                ) {
                    char buf[256] = {0};
                    size_t i = 0;
                    char* s;

                    s = rule_node->children.items[0].value;
                    if (s) while (*s) buf[i++] = *s++;

                    s = result->value;
                    if (s) while (*s) buf[i++] = *s++;

                    rule_node->children.items[0].value = view_to_cstr((String_View) { .str = buf, .len = i });
                    rule_node->children.items[0].len += result->len;
                }
                else {
                    da_append(rule_node->children, *result);
                }
            }

            current.len -= result->len;
            current.str += result->len;

            rule_node->len += result->len;


            if (gen.mod == MOD_ATLEAST_0 || gen.mod == MOD_ATLEAST_1) {
                j--;
            }
        }


        if (successful == 0) {
            rule_node->children.count = 0;
            rule_node->len = 0;
        }

        else if (rule_node->children.count) {
            if (
                rule_node->children.count == 1
                && rule_node->name == NULL
            ) {
                rule_node->children.items[0].len = rule_node->len;
                return &rule_node->children.items[0];
            }
            return rule_node;
        }

        else {
            return &EMPTY_NODE;
        }
    }

    return NULL;

}


Node* apply_gen(Syntax syn, Generator gen, String_View view) {

    if (gen.typ == G_STR) {

        int i = 0;
        while (i < view.len && gen.str_value[i]) {
            if (view.str[i] != gen.str_value[i])
                return NULL;

            i++;
        }

        Node* result = make_node();
        result->value = view_to_cstr((String_View) { .str = view.str, .len = i });
        result->len = i;
        return result;

    }

    else if (gen.typ == G_RANGE) {
        
        if (*view.str <= gen.max && *view.str >= gen.min) {
            Node* result = make_node();
            result->value = view_to_cstr((String_View) { .str = view.str, .len = 1 });
            result->len = 1;
            return result;
        }

    }

    else if (gen.typ == G_ID) {
        Rule* rule = find_rule_by_name(syn, gen.str_value);
        if (rule == NULL) return NULL;

        return apply_rule(syn, *rule, view);
    }

    else if (gen.typ == G_GROUP) {

        // A group generator is essentially an anonymous rule
        return apply_rule(syn, (Rule) { .name = NULL, .options = gen.options }, view);

    }

    else if (gen.typ == G_ANY) {

        // Match anything
        Node* result = make_node();
        result->value = view_to_cstr((String_View) { .str = view.str, .len = 1 });
        result->len = 1;
        return result;

    }

    return NULL;

}


Node* apply_syntax(Syntax syn, char* const text) {

    String_View view = (String_View) { .str = text, .len = strlen(text) };
    Node* s = make_node();

    while (view.len) {

        Node* best = &(Node) { .len = 0 };
        for (int i = 0; i < syn.rules.count; i++) {
            Rule rule = syn.rules.items[i];

            Node* result = apply_rule(syn, rule, view);
            if (result == NULL) continue;

            if (result->len > best->len) {
                best = result;
            }
        }


        if (best->len) {
            da_append(s->children, *best);

            view.str += best->len;
            view.len -= best->len;
        }
        
        else {
            view.str += 1;
            view.len -= 1;
        }
    }

    if (s->children.count) return s;
    return NULL;
}




#define da_append_string(xs, str) \
    do { char* __str_append_copy = (str); \
    while(*__str_append_copy) da_append(xs, *__str_append_copy++); } while (0)

#define da_append_string_n(xs, str, n) \
    for (int __i = 0; __i < (n); __i++) da_append(*xs, (str)[__i])


void str_indent(Array* builder, int indent) {
    while (indent--) da_append(*builder, ' ');
}

void __str_node(Node node, Array* builder, int indent) {

    if (node.name != NULL) {
        str_indent(builder, indent);
        da_append_string(*builder, node.name);
        if (node.children.count) {
            da_append(*builder, '(');
            if (node.children.count == 1 && node.children.items[0].children.count == 0) {
                __str_node(node.children.items[0], builder, 0);
            }
            else {
                da_append(*builder, '\n');
                __str_node(node.children.items[0], builder, indent + 2);
                for (int i = 1; i < node.children.count; i++) {
                    da_append(*builder, ',');
                    da_append(*builder, '\n');
                    __str_node(node.children.items[i], builder, indent + 2);
                }
                da_append(*builder, '\n');
                str_indent(builder, indent);
            }
            da_append(*builder, ')');
        }
    }
    else if (node.children.count) {
        str_indent(builder, indent);
        da_append(*builder, '[');
        da_append(*builder, '\n');
        __str_node(node.children.items[0], builder, indent + 2);
        for (int i = 1; i < node.children.count; i++) {
            da_append(*builder, ',');
            da_append(*builder, '\n');
            __str_node(node.children.items[i], builder, indent + 2);
        }
        da_append(*builder, '\n');
        str_indent(builder, indent);
        da_append(*builder, ']');
    }
    else {
        str_indent(builder, indent);
        da_append(*builder, '\'');
        char *node_value = node.value;
        if (node_value) while (*node_value) {
            if      (*node_value == '\n') da_append_string(*builder, "\\n");
            else if (*node_value == '\t') da_append_string(*builder, "\\t");
            else if (*node_value == '\r') da_append_string(*builder, "\\r");
            else if (*node_value == '\v') da_append_string(*builder, "\\v");
            else if (*node_value == '\'') da_append_string(*builder, "\\\'");
            else if (*node_value == '\"') da_append_string(*builder, "\\\"");
            else if (*node_value == '\\') da_append_string(*builder, "\\\\");
            else da_append(*builder, *node_value);

            node_value++;
        }
        da_append(*builder, '\'');
    }

}

char* str_node(Node* node) {
    if (node == NULL) return "FAILED";

    Array builder = (Array) { };
    __str_node(*node, &builder, 0);
    builder.items[builder.count] = '\0';

    return builder.items;
}

