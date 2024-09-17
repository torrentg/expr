#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "expr.h"

// Use this program to tune the MAX_RECURSION_GENERIC value
// @author: @skeeto

int main(void)
{
    enum { N = 20 };
    yy_token_t result = {0};
    yy_stack_t stack = {0};
    stack.data = (yy_token_t[64]){0};
    stack.reserved = 64;
    char src[7*N] = {0};

    for (int i = 4; i <= N; i++)
    {
        for (int j = 0; j < i; j++)
            memcpy(src+j*7, "ifelse(", 7);

        clock_t start = clock();
        result = yy_eval_number(src, src+i*7, &stack, 0, 0);

        printf("%d time = %ld ms, result = %s\n", i, 
            (long)(clock()-start)/(CLOCKS_PER_SEC/1000),
            (result.error == YY_ERROR_SYNTAX ? "OK" : "ERR"));

        fflush(stdout);
    }
}
