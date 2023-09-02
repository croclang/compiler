#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "error.h"
#include "file_io.h"
#include "environment.h"
#include "parser.h"

void print_usage(char** argv) {
    printf("Usage: %s <file.croc>\n", argv[0]);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv);
        exit(0);
    }

    char* contents = file_contents(argv[1]);

    if (contents) {
        Error err = ok;
        ParsingContext* context = parse_context_create();
        Node* program = node_allocate();
        program->type = NODE_TYPE_PROGRAM;

        char* contents_it = contents;
        
        for (;;) {
            Node* expression = node_allocate();
            node_add_child(program, expression);

            err = parse_expr(context, contents_it, &contents_it, expression);

            if (err.type != ERROR_NONE) {
                print_error(err);

                break;
            }

            if (!(*contents_it)) { break; }
        }

        print_node(program, 0);
        putchar('\n');

        if (err.type == ERROR_NONE) {
            printf("generating code\n");
            err = codegen_program(OUTPUT_FMT_DEFAULT, context, program);
            print_error(err);
            printf("code generated\n");
        }

        node_free(program);
        free(contents);
    }

    return 0;
}