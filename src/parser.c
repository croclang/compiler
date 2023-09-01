#include "parser.h"
#include "error.h"
#include "environment.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* comment_delimiters = ";#";
const char* whitespace = " \r\n";
const char* delimiters = " \r\n,():";

int comment_at_beginning(Token token) {
    const char* comment_it = comment_delimiters;

    while (*comment_it) {
        if (*(token.beginning) == *comment_it) {
            return 1;
        }

        comment_it++;
    }

    return 0;
}

Error lex(char* source, Token* token) {
    Error err = ok;

    if (!source || !token) {
        ERROR_PREP(err, ERROR_ARGUMENTS, "cannot lex empty source");

        return err;
    }

    token->beginning = source;
    token->beginning += strspn(token->beginning, whitespace);
    token->end = token->beginning;

    if (*(token->end) == '\0') {
        return err;
    }

    while (comment_at_beginning(*token)) {
        token->beginning = strpbrk(token->beginning, "\n");

        if (!token->beginning) {
            token->end = token->beginning;

            return err;
        }

        token->beginning += strspn(token->beginning, whitespace);
        token->end = token->beginning;
    }

    if (*(token->end) == '\0') {
        return err;
    }

    token->end += strcspn(token->beginning, delimiters);

    if (token->end == token->beginning) {
        token->end += 1;
    }

    return err;
}

int token_string_equalp(char* string, Token* token) {
    if (!string || !token) {
        return 0;
    }

    char* beg = token->beginning;

    while (*string && token->beginning < token->end) {
        if (*string != *beg) {
            return 0;
        }

        string++;
        beg++;
    }

    return 1;
}

void print_token(Token t) {
    if (t.end - t.beginning < 1) {
        printf("print_token: invalid token pointers");
    } else {
        printf("%.*s", t.end - t.beginning, t.beginning);
    }
}

Node* node_allocate() {
    Node* node = calloc(1, sizeof(Node));
    assert(node && "node_allocate: could not allocate memory for AST node");

    return node;
}

void node_add_child(Node* parent, Node* new_child) {
    if (!parent || !new_child) {
        return;
    }

    if (parent->children) {
        Node* child = parent->children;

        while (child->next_child) {
            child = child->next_child;
        }

        child->next_child = new_child;
    } else {
        parent->children = new_child;
    }
}

int node_compare(Node* a, Node* b) {
    if (!a || !b) {
        if (!a && !b) {
            return 1;
        }

        return 0;
    }

    assert(NODE_TYPE_MAX == 7 && "node_compare: node_compare() does not handle all node types");

    if (a->type != b->type) {
        return 0;
    }

    switch (a->type) {
    case NODE_TYPE_NONE:
        if (nonep(*b)) {
            return 1;
        }

        return 0;

    case NODE_TYPE_INTEGER:
        if (a->value.integer == b->value.integer) {
            return 1;
        }

        return 0;

    case NODE_TYPE_SYMBOL:
        if (a->value.symbol && b->value.symbol) {
            if (strcmp(a->value.symbol, b->value.symbol) == 0) {
                return 1;
            }

            return 0;
        } else if (!a->value.symbol && !b->value.symbol) {
            return 1;
        }

        return 0;
    
    case NODE_TYPE_BINARY_OPERATOR:
        printf("TODO: node_compare() binary operator\n");

        break;

    case NODE_TYPE_VARIABLE_DECLARATION:
        printf("TODO: node_compare() variable declaration\n");

    case NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED:
        printf("TODO: node_compare() variable declaration initialized\n");

    case NODE_TYPE_PROGRAM:
        printf("TODO: compare two programs\n");

        break;
    }

    return 0;
}

Node* node_integer(long long value) {
    Node* integer = node_allocate();
    integer->type = NODE_TYPE_INTEGER;
    integer->value.integer = value;
    integer->children = NULL;
    integer->next_child = NULL;

    return integer;
}

Node* node_symbol(char* symbol_string) {
    Node* symbol = node_allocate();
    symbol->type = NODE_TYPE_SYMBOL;
    symbol->value.symbol = strdup(symbol_string);

    return symbol;
}

Node* node_symbol_from_buffer(char* buffer, size_t length) {
    assert(buffer && "node_symbol_from_buffer: cannot create AST Symbol Node from NULL buffer");
    char* symbol_string = malloc(length + 1);
    assert(symbol_string && "could not allocate memory for symbol string");

    memcpy(symbol_string, buffer, length);
    symbol_string[length] = '\0';

    Node* symbol = node_allocate();
    symbol->type = NODE_TYPE_SYMBOL;
    symbol->value.symbol = symbol_string;

    return symbol;
}

void print_node(Node* node, size_t indent_level) {
    if (!node) {
        return;
    }

    for (size_t i = 0; i < indent_level; ++i) {
        putchar(' ');
    }

    assert(NODE_TYPE_MAX == 7 && "print_node: print_node() does not handle all node types");

    switch (node->type) {
    default:
        printf("UNKNOWN");

        break;

    case NODE_TYPE_NONE:
        printf("NONE");

        break;
    
    case NODE_TYPE_INTEGER:
        printf("INT:%lld", node->value.integer);

        break;

    case NODE_TYPE_SYMBOL:
        printf("SYM");

        if (node->value.symbol) {
            printf(":%s", node->value.symbol);
        }

        break;

    case NODE_TYPE_BINARY_OPERATOR:
        printf("BINARY OPERATOR");

        break;
    
    case NODE_TYPE_VARIABLE_DECLARATION:
        printf("VARIABLE DECLARATION");

        break;

    case NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED:
        printf("VARIABLE DECLARATION INITIALIZED");
    
    case NODE_TYPE_PROGRAM:
        printf("PROGRAM");

        break;
    }

    putchar('\n');

    Node* child = node->children;

    while (child) {
        print_node(child, indent_level + 4);
        
        child = child->next_child;
    }
}

// Copy `a` into `b`
void node_copy(Node* a, Node* b) {
    if (!a || !b) {
        return;
    }

    b->type = a->type;

    switch (a->type) {
    default:
        b->value = a->value;

        break;

    case NODE_TYPE_SYMBOL:
        b->value.symbol = strdup(a->value.symbol);

        assert(b->value.symbol && "node_copy: could not allocate memory for new symbol");

        break;
    }

    Node* child = a->children;
    Node* child_it = NULL;

    while (child) {
        Node* new_child = node_allocate();

        if (child_it) {
            child_it->next_child = new_child;
            child_it = child_it->next_child;
        } else {
            b->children = new_child;
            child_it = new_child;
        }

        node_copy(child, child_it);

        child = child->next_child;
    }
}

ParsingContext* parse_context_create() {
    ParsingContext* ctx = calloc(1, sizeof(ParsingContext));
    assert(ctx && "parse_context_create: could not allocate memory for parsing context");
    
    ctx->types = environment_create(NULL);

    if (environment_set(ctx->types, node_symbol("integer"), node_integer(0)) == 0) {
        printf("ERROR: failed to set builtin type in types environment\n");
    }

    ctx->variables = environment_create(NULL);

    return ctx;
}

int parse_integer(Token* token, Node* node) {
    if (!token || !node) {
        return 0;
    }

    char* end = NULL;

    if (token->end - token->beginning == 1 && *(token->beginning) == '0') {
        node->type = NODE_TYPE_INTEGER;
        node->value.integer = 0;
    } else if ((node->value.integer = strtoll(token->beginning, &end, 10)) != 0) {
        if (end != token->end) {
            return 0;
        }

        node->type = NODE_TYPE_INTEGER;
    } else {
        return 0;
    }

    return 1;
}

// FIXME: make more efficient by keeping track of all
//        pointers and freeing them in one go
void node_free(Node* root) {
    if (!root) {
        return;
    }

    Node* child = root->children;
    Node* next_child = NULL;

    while (child) {
        next_child = child->next_child;

        node_free(child);

        child = next_child;
    }

    if (symbolp(*root) && root->value.symbol) {
        free(root->value.symbol);
    }

    free(root);
}

Error parse_expr(ParsingContext* context, char* source, char** end, Node* result) {
    size_t token_count = 0;
    size_t token_length = 0;
    Token current_token;
    current_token.beginning = source;
    current_token.end = source;

    Error err = ok;

    while ((err = lex(current_token.end, &current_token)).type == ERROR_NONE) {
        *end = current_token.end;
        token_length = current_token.end - current_token.beginning;

        if (token_length == 0) {
            break;
        }

        if (parse_integer(&current_token, result)) {
            return ok;
        }

        Node* symbol = node_symbol_from_buffer(current_token.beginning, token_length);
        err = lex(current_token.end, &current_token);
        *end = current_token.end;

        if (err.type != ERROR_NONE) {
            return err;
        }

        token_length = current_token.end - current_token.beginning;

        if (token_length == 0) {
            break;
        }

        if (token_string_equalp(":", &current_token)) {
            err = lex(current_token.end, &current_token);
            *end = current_token.end;

            if (err.type != ERROR_NONE) {
                return err;
            }

            token_length = current_token.end - current_token.beginning;

            if (token_length == 0) {
                break;
            }

            Node* variable_binding = node_allocate();

            printf("looking for \"%s\" in variables environment\n", symbol->value.symbol);
            if (environment_get(*context->variables, symbol, variable_binding)) {
                printf("found existing symbol in environment: %s\n", symbol->value.symbol);

                if (token_string_equalp("=", &current_token)) {
                    Node* reassign_expr = node_allocate();
                    err = parse_expr(context, current_token.end, end, reassign_expr);

                    if (err.type != ERROR_NONE) {
                        return err;
                    }

                    printf("reassign expr: ");
                    print_node(reassign_expr, 0);
                    putchar('\n');

                    exit(0);

                    if (reassign_expr->type != variable_binding->children->type) {
                        ERROR_PREP(err, ERROR_TYPE, "variable assignment expression has mismatched type");

                        return err;
                    }

                    variable_binding->children->value = reassign_expr->value;

                    free(reassign_expr);

                    return ok;
                }

                printf("id of redefined variable: \"%s\"", symbol->value.symbol);
                ERROR_PREP(err, ERROR_GENERIC, "redefinition of variable");

                return err;
            }

            free(variable_binding);

            Node* expected_type_symbol = node_symbol_from_buffer(current_token.beginning, token_length);
            if (environment_get(*context->types, expected_type_symbol, result) == 0) {
                ERROR_PREP(err, ERROR_TYPE, "invalid type within variable declaration");
                printf("\ninvalid type: \"%s\"\n", expected_type_symbol->value.symbol);

                return err;
            }

            printf("valid type symbol: \"%s\"\n", expected_type_symbol->value.symbol);

            Node* var_decl = node_allocate();
            var_decl->type = NODE_TYPE_VARIABLE_DECLARATION;

            Node* type_node = node_allocate();
            type_node->type = result->type;

            node_add_child(var_decl, symbol);

            err = lex(current_token.end, &current_token);
            *end = current_token.end;

            if (err.type != ERROR_NONE) {
                return err;
            }

            token_length = current_token.end - current_token.beginning;

            if (token_length == 0) {
                break;
            }

            if (token_string_equalp("=", &current_token)) {
                Node* assigned_expr = node_allocate();
                err = parse_expr(context, current_token.end, end, assigned_expr);

                if (err.type != ERROR_NONE) {
                    return err;
                }

                printf("assigned expr: ");
                print_node(assigned_expr, 0);
                putchar('\n');

                if (assigned_expr->type != type_node->type) {
                    node_free(assigned_expr);
                    ERROR_PREP(err, ERROR_TYPE, "variable assignment expression has mismatched type");

                    return err;
                }

                type_node->value = assigned_expr->value;

                free(assigned_expr);
            }

            *result = *var_decl;
            free(var_decl);

            Node* symbol_for_env = node_allocate();
            node_copy(symbol, symbol_for_env);

            int status = environment_set(context->variables, symbol_for_env, type_node);

            return ok;
        }

        printf("unrecognized token: ");
        print_token(current_token);
        putchar('\n');

        ERROR_PREP(err, ERROR_SYNTAX, "unrecognized token reached during parsing");

        return err;
    }

    return err;
}