#include "parser.h"
#include "error.h"
#include "file_io.h"
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
        printf("%.*s", (int)(t.end - t.beginning), t.beginning);
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

    assert(NODE_TYPE_MAX == 10 && "node_compare: node_compare() does not handle all node types");

    if (a->type != b->type) {
        return 0;
    }

    switch (a->type) {
    case NODE_TYPE_NONE:
        if (nonep(*b)) {
            return 1;
        }

        break;

    case NODE_TYPE_INTEGER:
        if (a->value.integer == b->value.integer) {
            return 1;
        }

        break;

    case NODE_TYPE_SYMBOL:
        if (a->value.symbol && b->value.symbol) {
            if (strcmp(a->value.symbol, b->value.symbol) == 0) {
                return 1;
            }

            break;
        } else if (!a->value.symbol && !b->value.symbol) {
            return 1;
        }

        break;
    
    case NODE_TYPE_BINARY_OPERATOR:
        printf("TODO: node_compare() binary operator\n");

        break;

    case NODE_TYPE_FUNCTION:
        printf("TODO: node_compare() function\n");

        break;

    case NODE_TYPE_FUNCTION_CALL:
        printf("TODO: node_compare() function call\n");
        
        break;

    case NODE_TYPE_VARIABLE_REASSIGNMENT:
        printf("TODO: node_compare() variable reassignment\n");

        break;

    case NODE_TYPE_VARIABLE_DECLARATION:
        printf("TODO: node_compare() variable declaration\n");

        break;

    case NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED:
        printf("TODO: node_compare() variable declaration initialized\n");

        break;

    case NODE_TYPE_PROGRAM:
        printf("TODO: compare two programs\n");

        break;
    }

    return 0;
}

Node* node_none() {
    Node* none = node_allocate();
    none->type = NODE_TYPE_NONE;

    return none;
}

Node* node_integer(long long value) {
    Node* integer = node_allocate();
    integer->type = NODE_TYPE_INTEGER;
    integer->value.integer = value;

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

// Take owner of type symbol
Error define_type(Environment* types, int type, Node* type_symbol, long long byte_size) {
    assert(types && "node_add_type: cannot add type to NULL types environment");
    assert(type_symbol && "node_add_type: cannot add NULL type symbol to types environment");
    assert(byte_size >= 0 && "node_add_type: cannot define new type with zero or negative byte size");

    Node* size_node = node_allocate();
    size_node->type = NODE_TYPE_INTEGER;
    size_node->value.integer = byte_size;

    Node* type_node = node_allocate();
    type_node->type = type;
    type_node->children = size_node;

    if (environment_set(types, type_symbol, type_node) == 1) {
        return ok;
    }

    printf("type that was redefined: \"%s\"\n", type_symbol->value.symbol);
    ERROR_CREATE(err, ERROR_TYPE, "redefinition of type");

    return err;
}

void print_node(Node* node, size_t indent_level) {
    if (!node) {
        return;
    }

    for (size_t i = 0; i < indent_level; ++i) {
        putchar(' ');
    }

    assert(NODE_TYPE_MAX == 10 && "print_node: print_node() does not handle all node types");

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

    case NODE_TYPE_VARIABLE_REASSIGNMENT:
        printf("VARIABLE REASSIGNMENT");

        break;
    
    case NODE_TYPE_VARIABLE_DECLARATION:
        printf("VARIABLE DECLARATION");

        break;

    case NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED:
        printf("VARIABLE DECLARATION INITIALIZED");
    
    case NODE_TYPE_PROGRAM:
        printf("PROGRAM");

        break;

    case NODE_TYPE_FUNCTION:
        printf("FUNCTION");

        break;

    case NODE_TYPE_FUNCTION_CALL:
        printf("FUNCTION CALL");

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

ParsingContext* parse_context_create(ParsingContext* parent) {
    ParsingContext* ctx = calloc(1, sizeof(ParsingContext));
    assert(ctx && "parse_context_create: could not allocate memory for parsing context");
    
    ctx->parent = parent;
    ctx->operator = NULL;
    ctx->result = NULL;
    ctx->types = environment_create(NULL);
    ctx->variables = environment_create(NULL);
    ctx->functions = environment_create(NULL);

    return ctx;
}

ParsingContext* parse_context_default_create() {
    ParsingContext* ctx = parse_context_create(NULL);
    Error err = define_type(ctx->types, NODE_TYPE_INTEGER, node_symbol("integer"), sizeof(long long));

    if (err.type != ERROR_NONE) {
        printf("ERROR: failed to set builtin integer type in types environment\n");
    }

    return ctx;
}

Error lex_advance(Token* token, size_t* token_length, char** end) {
    if (!token || !token_length || !end) {
        ERROR_CREATE(err, ERROR_ARGUMENTS, "lex_advance: pointer argumnets must not be NULL");

        return err;
    }

    Error err = lex(token->end, token);
    *end = token->end;

    if (err.type != ERROR_NONE) {
        return err;
    }

    *token_length = token->end - token->beginning;

    return err;
}

typedef struct ExpectReturnValue {
    Error err;
    char found;
    char done;
} ExpectReturnValue;

ExpectReturnValue lex_expect(char* expected, Token* current, size_t* current_length, char** end) {
    ExpectReturnValue out;
    out.done = 0;
    out.found = 0;
    out.err = ok;

    if (!expected || !current || !current_length || !end) {
        ERROR_PREP(out.err, ERROR_ARGUMENTS, "lex_expect: lex_expect() must not be passed NULL pointers");

        return out;
    }

    Token current_copy = *current;
    size_t current_length_copy = *current_length;
    char* end_value = *end;

    out.err = lex_advance(&current_copy, &current_length_copy, &end_value);
    if (out.err.type != ERROR_NONE) {
        return out;
    }

    if (current_length_copy == 0) {
        out.done = 1;

        return out;
    }

    if (token_string_equalp(expected, &current_copy)) {
        out.found = 1;
        *end = end_value;
        *current = current_copy;
        *current_length = current_length_copy;
    }

    return out;
}

Error parse_get_type(ParsingContext* context, Node* id, Node* result) {
    Error err = ok;

    while (context) {
        int status = environment_get(*context->types, id, result);
        if (status) { return ok; }

        context = context->parent;
    }

    result->type = NODE_TYPE_NONE;
    ERROR_PREP(err, ERROR_GENERIC, "type was not found in environment");

    return err;
}

#define EXPECT(expected, expected_string, current_token, current_length, end) \
    expected = lex_expect(expected_string, &current_token, &current_length, end); \
    if (expected.err.type) { return expected.err; } \
    if (expected.done) { return ok; }

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
    ExpectReturnValue expected;
    size_t token_length = 0;
    Token current_token;
    current_token.beginning = source;
    current_token.end = source;

    Error err = ok;
    Node* working_result = result;

    while ((err = lex_advance(&current_token, &token_length, end)).type == ERROR_NONE) {
        // printf("lexed: ");
        // print_token(current_token);
        // putchar('\n');

        if (token_length == 0) {
            return ok;
        }

        if (parse_integer(&current_token, working_result)) {
            
        } else {
            Node* symbol = node_symbol_from_buffer(current_token.beginning, token_length);
            
            if (strcmp("defun", symbol->value.symbol) == 0) {
                working_result->type = NODE_TYPE_FUNCTION;

                lex_advance(&current_token, &token_length, end);
                Node* function_name = node_symbol_from_buffer(current_token.beginning, token_length);
                
                EXPECT(expected, "(", current_token, token_length, end);
                if (!expected.found) {
                    printf("function name: \"%s\"\n", function_name->value.symbol);
                    ERROR_PREP(err, ERROR_SYNTAX, "expected opening parenthesis for parameter list after function name");

                    return err;
                }

                Node* parameter_list = node_allocate();
                node_add_child(working_result, parameter_list);

                for (;;) {
                    EXPECT(expected, ")", current_token, token_length, end);
                    if (expected.found) { break; }
                    if (expected.done) {
                        ERROR_PREP(err, ERROR_SYNTAX, "expected closing parenthesis for parameter list");
                        
                        return err;
                    }

                    err = lex_advance(&current_token, &token_length, end);
                    if (err.type) { return err; }

                    Node* parameter_name = node_symbol_from_buffer(current_token.beginning, token_length);

                    EXPECT(expected, ":", current_token, token_length, end);
                    if (expected.done || !expected.found) {
                        ERROR_PREP(err, ERROR_SYNTAX, "parameter declaration requires a type annotation");
                        
                        return err;
                    }

                    lex_advance(&current_token, &token_length, end);
                    
                    Node* parameter_type = node_symbol_from_buffer(current_token.beginning, token_length);
                    Node* parameter = node_allocate();

                    node_add_child(parameter, parameter_name);
                    node_add_child(parameter, parameter_type);
                    node_add_child(parameter_list, parameter);

                    EXPECT(expected, ",", current_token, token_length, end);
                    if (expected.found) { continue; }

                    EXPECT(expected, ")", current_token, token_length, end);
                    if (!expected.found) {
                        ERROR_PREP(err, ERROR_SYNTAX, "expected closing parenthesis following parameter list");

                        return err;
                    }

                    break;
                }

                EXPECT(expected, ":", current_token, token_length, end);
                if (expected.done || !expected.found) {
                    ERROR_PREP(err, ERROR_SYNTAX, "function definition requires a return type annotation following parameter list");

                    return err;
                }

                lex_advance(&current_token, &token_length, end);

                Node* function_return_type = node_symbol_from_buffer(current_token.beginning, token_length);
                node_add_child(working_result, function_return_type);
                environment_set(context->functions, function_name, working_result);

                EXPECT(expected, "{", current_token, token_length, end);
                if (expected.done || !expected.found) {
                    ERROR_PREP(err, ERROR_SYNTAX, "function definition requires body following return type \"{ body! }\"");
                    
                    return err;
                }

                context = parse_context_create(context);
                context->operator = node_symbol("defun");

                Node* param_it = working_result->children->children;
                while (param_it) {
                    environment_set(context->variables, param_it->children, param_it->children->next_child);

                    param_it = param_it->next_child;
                }

                Node* function_body = node_allocate();
                Node* function_first_expression = node_allocate();
                node_add_child(function_body, function_first_expression);
                node_add_child(working_result, function_body);

                working_result = function_first_expression;
                context->result = working_result;

                continue;
            } else {
                EXPECT(expected, ":", current_token, token_length, end);
                if (expected.found) {
                    EXPECT(expected, "=", current_token, token_length, end);
                    if (expected.found) {
                        Node* variable_binding = node_allocate();

                        if (!environment_get(*context->variables, symbol, variable_binding)) {
                            printf("id of undeclared variable: \"%s\"\n", symbol->value.symbol);
                            ERROR_PREP(err, ERROR_GENERIC, "reassignment of variable that has not been declared");

                            return err;
                        }
                        
                        free(variable_binding);

                        working_result->type = NODE_TYPE_VARIABLE_REASSIGNMENT;
                        node_add_child(working_result, symbol);

                        Node* reassign_expr = node_allocate();
                        node_add_child(working_result, reassign_expr);

                        working_result = reassign_expr;

                        continue;
                    }

                    err = lex_advance(&current_token, &token_length, end);

                    if (err.type != ERROR_NONE) { return err; }
                    if (token_length == 0) { break; }

                    Node* type_symbol = node_symbol_from_buffer(current_token.beginning, token_length);
                    Node* type_value = node_allocate();
                    if (environment_get(*context->types, type_symbol, type_value) == 0) {
                        ERROR_PREP(err, ERROR_TYPE, "invalid type within variable declaration");
                        printf("\ninvalid type: \"%s\"\n", type_symbol->value.symbol);

                        return err;
                    }

                    free(type_value);

                    Node* variable_binding = node_allocate();
                    if (environment_get(*context->variables, symbol, variable_binding)) {
                        ERROR_PREP(err, ERROR_GENERIC, "redefinition of variable");
                        printf("id of redefined variable: \"%s\"\n", symbol->value.symbol);

                        return err;
                    }

                    free(variable_binding);

                    working_result->type = NODE_TYPE_VARIABLE_DECLARATION;
                    Node* value_expression = node_none();

                    node_add_child(working_result, symbol);
                    node_add_child(working_result, value_expression);

                    Node* symbol_for_env = node_allocate();
                    node_copy(symbol, symbol_for_env);

                    int status = environment_set(context->variables, symbol_for_env, type_symbol);
                    if (status != 1) {
                        printf("variable: \"%s\", status: %d\n", symbol_for_env->value.symbol, status);
                        ERROR_PREP(err, ERROR_GENERIC, "failed to define variable");

                        return err;
                    }

                    EXPECT(expected, "=", current_token, token_length, end);
                    if (expected.found) {
                        working_result = value_expression;

                        continue;
                    }

                    return ok;
                } else {
                    EXPECT(expected, "(", current_token, token_length, end);
                    if (expected.found) {
                        working_result->type = NODE_TYPE_FUNCTION_CALL;

                        node_add_child(working_result, symbol);

                        Node* argument_list = node_allocate();
                        Node* first_argument = node_allocate();

                        node_add_child(argument_list, first_argument);
                        node_add_child(working_result, argument_list);

                        working_result = first_argument;

                        context = parse_context_create(context);
                        context->operator = node_symbol("funcall");
                        context->result = working_result;

                        continue;
                    } else {
                        // TOOD: check for variable access
                    }
                }

                printf("unrecognized token: ");
                print_token(current_token);
                putchar('\n');

                ERROR_PREP(err, ERROR_SYNTAX, "unrecognized token reached during parsing");

                return err;
            }
        }

        if (!context->parent) {
            break;
        }

        Node* operator = context->operator;
        if (operator->type != NODE_TYPE_SYMBOL) {
            ERROR_PREP(err, ERROR_TYPE, "[ likely interal error :( ] parsing context operator must be symbol.");

            return err;
        }

        if (strcmp(operator->value.symbol, "defun") == 0) {
            EXPECT(expected, "}", current_token, token_length, end);
            if (expected.done || expected.found) { break; }

            context->result->next_child = node_allocate();
            working_result = context->result->next_child;
            context->result = working_result;

            continue;
        }

        if (strcmp(operator->value.symbol, "funcall") == 0) {
            EXPECT(expected, ")", current_token, token_length, end);
            if (expected.done || expected.found) { break; }

            EXPECT(expected, ",", current_token, token_length, end);
            if (expected.done || !expected.found) {
                print_token(current_token);
                ERROR_PREP(err, ERROR_SYNTAX, "parameter list expected closing parenthesis or comma for another parameter");

                return err;
            }

            context->result->next_child = node_allocate();
            working_result = context->result->next_child;
            context->result = working_result;

            continue;
        }
    }

    return err;
}

Error parse_program(char* filepath, ParsingContext* context, Node* result) {
    Error err = ok;
    char* contents = file_contents(filepath);

    if (!contents) {
        printf("filepath: \"%s\"\n", filepath);
        ERROR_PREP(err, ERROR_GENERIC, "parse_program: failed to get file contents");

        return err;
    }

    result->type = NODE_TYPE_PROGRAM;
    char* contents_it = contents;

    for (;;) {
        Node* expression = node_allocate();
        node_add_child(result, expression);

        err = parse_expr(context, contents_it, &contents_it, expression);
        if (err.type != ERROR_NONE) {
            free(contents);

            return err;
        }

        if (!(*contents_it)) { break; }
    }

    free(contents);

    return ok;
}