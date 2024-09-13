#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "expr.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define UNUSED(x) (void)(x)

yy_token_t resolve(yy_str_t var, void *data)
{
    UNUSED(data);

    if (var.len == strlen("myvar") && strncmp(var.ptr, "myvar", var.len) == 0)
        return (yy_token_t){.type = YY_TOKEN_NUMBER, .number_val = 42};
    
    return (yy_token_t){.type = YY_TOKEN_ERROR, .error = YY_ERROR_REF};
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = ARRAY_LEN(data)};
    yy_token_t aux_data[64] = {0};
    yy_stack_t aux_stack = {.data = aux_data, .reserved = ARRAY_LEN(aux_data)};
    yy_token_t result = {0};
    const char *err = NULL;

    // required by rand()
    srand(time(NULL));

    // simple numeric expression
    const char *expr1 = "trunc(random(3, 150))^2";
    result = yy_eval_number(expr1, expr1 + strlen(expr1), &stack, NULL, NULL);

    if (result.type == YY_TOKEN_ERROR)
        printf("%s = #ERR\n", expr1);
    else
        printf("%s = %g\n", expr1, result.number_val);

    // numeric expression with variables
    const char *expr2 = "1 + $myvar";
    result = yy_eval_number(expr2, expr2 + strlen(expr2), &stack, resolve, NULL);

    if (result.type == YY_TOKEN_ERROR)
        printf("%s = #ERR\n", expr2);
    else
        printf("%s = %g\n", expr2, result.number_val);

    // mixed types
    const char *expr3 = "\"Hi \" + upper(ifelse(random(0,10) < 5, \"bob\", \"john\")) + \"!\"";
    result = yy_eval(expr3, expr3 + strlen(expr3), &stack, NULL, NULL);

    if (result.type == YY_TOKEN_ERROR)
        printf("%s = #ERR\n", expr3);
    else
        printf("%s = %.*s\n", expr3, result.str_val.len, result.str_val.ptr);

    // compile + eval
    const char *expr4 = "datepart(now(), \"day\")^2";
    yy_error_e rc = yy_compile_number(expr4, expr4 + strlen(expr4), &stack, &err);

    if (rc != YY_OK)
        printf("%s = #ERR\n", expr4);

    result = yy_eval_stack(&stack, &aux_stack, resolve, NULL);

    if (result.type == YY_TOKEN_ERROR)
        printf("%s = #ERR\n", expr4);
    else
        printf("%s = %g\n", expr4, result.number_val);
}
