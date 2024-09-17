#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "expr.h"

// @author: https://www.reddit.com/user/skeeto/
// @see https://www.reddit.com/r/C_Programming/comments/1fhigj8/a_simple_expression_parser_supporting_multiple/
// @see https://github.com/AFLplusplus/AFLplusplus

/*
  $ afl-gcc-fast -g3 -fsanitize=address,undefined fuzz.c
  $ mkdir i
  $ echo 'sin((-1 + 2) * PI)' >i/numerical
  ... copying more examples from README.md like this ...
  $ afl-fuzz -ii -oo ./a.out
*/

__AFL_FUZZ_INIT();

int main(void)
{
    __AFL_INIT();
    char *src = 0;
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        src = realloc(src, len);
        memcpy(src, buf, len);
        yy_stack_t stack = {0};
        stack.data = (yy_token_t[64]){0};
        stack.reserved = 64;
        yy_eval_number(src, src+len, &stack, 0, 0);
    }
}
