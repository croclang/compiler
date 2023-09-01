#include "error.h"

#include <assert.h>
#include <stdio.h>
#include <stddef.h>

Error ok = { ERROR_NONE, NULL };

void print_error(Error err) {
    if (err.type == ERROR_NONE) {
        return;
    }

    printf("error: ");
    assert(ERROR_MAX == 6);

    switch (err.type) {
    case ERROR_TODO:
        printf("TODO (not implemented)");

        break;

    case ERROR_SYNTAX:
        printf("invalid syntax");

        break;
    
    case ERROR_TYPE:
        printf("mismatched types");

        break;

    case ERROR_ARGUMENTS:
        printf("invalid arguments");

        break;

    case ERROR_GENERIC:
    case ERROR_NONE:
        break;

    default:
        printf("unknown error type");

        break;
    }
    
    putchar('\n');

    if (err.msg) {
        printf("    : %s\n", err.msg);
    }
}