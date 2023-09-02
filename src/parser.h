#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "error.h"
#include <stddef.h>

typedef struct Environment Environment;

typedef struct Token {
    char* beginning;
    char* end;
} Token;

void print_token(Token t);
Error lex(char* source, Token* token);

typedef enum NodeType {
    NODE_TYPE_NONE = 0,
    NODE_TYPE_INTEGER,
    NODE_TYPE_SYMBOL,
    NODE_TYPE_VARIABLE_DECLARATION,
    NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED,
    NODE_TYPE_VARIABLE_REASSIGNMENT,
    NODE_TYPE_BINARY_OPERATOR,
    NODE_TYPE_PROGRAM,
    NODE_TYPE_MAX,
} NodeType;

typedef struct Node {
    int type;

    union NodeValue {
        long long integer;
        char* symbol;
    } value;

    struct Node* children;
    struct Node* next_child;
} Node;

Node* node_allocate();

#define nonep(node)     ((node).type == NODE_TYPE_NONE)
#define integerp(node)  ((node).type == NODE_TYPE_INTEGER)
#define symbolp(node)   ((node).type == NODE_TYPE_SYMBOL)

void node_add_child(Node* parent, Node* new_child);
int node_compare(Node* a, Node* b);
Node* node_integer(long long value);
Node* node_symbol(char* symbol_string);
Node* node_symbol_from_buffer(char* buffer, size_t length);

void print_node(Node* node, size_t indent_level);
void node_free(Node* root);
void node_copy(Node* a, Node* b);

int token_string_equalp(char* string, Token* token);
int parse_integer(Token* token, Node* node);

typedef struct ParsingContext {
    Environment* types;
    Environment* variables;
} ParsingContext;

ParsingContext* parse_context_create();
Error parse_expr(ParsingContext* context, char* source, char** end, Node* result);

#endif