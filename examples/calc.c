/*
MIT License

expr -- A simple expressions parser.
<https://github.com/torrentg/expr>

Copyright (c) 2024 Gerard Torrent <gerard@generacio.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"
#include "expr.h"

/**
 * This is a simple REPL calculator used to demonstrate the 
 * capabilities of the expr library.
 * 
 * Current limitations:
 *   - Maximum number or variables limited to 1024
 *   - Variable names restricted to ${num}
 *   - Variables in a list instead of a map
 *   - Results in a list instead of a map
 */

#define PROMT "calc"
#define MAX_VARIABLES 1024
#define UNUSED(x) (void)(x)

typedef struct variable_t
{
    char *name;             // variable name
    char *formula;          // formula
    yy_stack_t stack;       // rpn stack
} variable_t;

typedef struct result_t
{
    yy_str_t name;          // variable name
    yy_token_t value;       // evaluation result (YY_TOKEN_NULL -> ongoing evaluation)
} result_t;

typedef struct identifier_t
{
    const char *str;
    int type;
} identifier_t;

extern const identifier_t yy_identifiers[];
variable_t variables[MAX_VARIABLES] = {0};
int num_variables = 0;

void print_header(void)
{
    printf("Calc is a tool for evaluating formulas.\n");
    printf("Type 'info' for additional information.\n");
    printf("Type 'exit' to quit.\n");
    printf("\n");
}

void print_info(void)
{
    printf("  exit        : Quit program\n");
    printf("  identifiers : List identifiers\n");
    printf("  info        : Display this information\n");
    printf("  ${<num>}    : Variable corresponding to line <num>\n");
}

void print_identifiers(void)
{
    const identifier_t *ptr = yy_identifiers;

    while (ptr && ptr->str)
    {
        printf("%s, ", ptr->str);
        ptr++;
    }

    printf("\n");
}

char * datetime_to_str(uint64_t millis_utc, char *ret)
{
    time_t time = (time_t)(millis_utc / 1000UL);
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    sprintf(ret, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
        stm.tm_year + 1900, stm.tm_mon + 1,
        stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec,
        (int)(millis_utc % 1000));

    return ret;
}

const char * error_to_str(yy_error_e err)
{
    switch (err)
    {
        case YY_OK: return "#OK";
        case YY_ERROR: return "#ERROR";
        case YY_ERROR_REF: return "#REF";
        case YY_ERROR_MEM: return "#MEM";
        case YY_ERROR_EVAL: return "#EVAL";
        case YY_ERROR_CREF: return "#CREF";
        case YY_ERROR_VALUE: return "#VALUE";
        case YY_ERROR_SYNTAX: return "#SYNTAX";
        default: return "#???";
    }
}

void print_token(yy_token_t token)
{
    char buf[128] = {0};

    switch(token.type)
    {
        case YY_TOKEN_BOOL:
            printf("%s", (token.bool_val ? "true" : "false"));
            break;
        case YY_TOKEN_NUMBER:
            printf("%g", token.number_val);
            break;
        case YY_TOKEN_DATETIME:
            printf("%s", datetime_to_str(token.datetime_val, buf));
            break;
        case YY_TOKEN_STRING:
            printf("%.*s", token.str_val.len, token.str_val.ptr);
            break;
        case YY_TOKEN_ERROR:
            printf("%s", error_to_str(token.error));
            break;
        default:
            printf("Unexpected token (%d)", token.type);
    }

    printf("\n");
}

// duplicates a stack
yy_stack_t stackdup(const yy_stack_t *stack)
{
    yy_token_t *data = NULL;

    if (!stack || !stack->len || !stack->reserved || stack->reserved < stack->len)
        return (yy_stack_t){0};

    size_t len = stack->len * sizeof(yy_token_t);
    data = (yy_token_t *) malloc(len);
    memcpy(data, stack->data, len);

    return (yy_stack_t){.data = data, .reserved = stack->len, .len = stack->len};
}

// Resolve a variable
yy_token_t resolve(yy_str_t var, void *data)
{
    result_t *results = (result_t *) data;

    // search in previous results
    while (results->name.ptr != NULL && results->name.len)
    {
        if (results->name.len == var.len && strncmp(var.ptr, results->name.ptr, var.len) == 0)
        {
            if (results->value.type == YY_TOKEN_NULL)
                return (yy_token_t){.type = YY_TOKEN_ERROR, .error = YY_ERROR_CREF};
            
            else
                return results->value;
        }

        results++;
    }

    // variable name not found in previous results
    int num = atoi(var.ptr);

    // unrecognized variable name
    if (num == 0 || num >= num_variables)
        return (yy_token_t){.type = YY_TOKEN_ERROR, .error = YY_ERROR_REF};

    // append variable to list
    results->name = var;

    // evaluate variable
    yy_token_t tokens[256] = {0};
    yy_stack_t stack = {.data = tokens, .reserved = sizeof(tokens)/sizeof(tokens[0]), .len = 0};
    yy_token_t ret = yy_eval_stack(&variables[num-1].stack, &stack, resolve, data);

    results->value = ret;

    return ret;
}

void process_line(char *line)
{
    const char *err = NULL;
    yy_error_e rc = YY_OK;
    yy_token_t data[1024] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    result_t results[32] = {0};
    yy_token_t result = {0};
    char *name = NULL;
    char buf[64] = {0};
    char *ptr = line;

    while(isspace(*ptr)) ++ptr;

    if (ptr[0] == '\0' || ptr[0] == '#' || num_variables + 1 >= MAX_VARIABLES) {
        free(line);
        return;
    }

    snprintf(buf, sizeof(buf), "%d", num_variables + 1);
    name = strdup(buf);

    variables[num_variables].name = name;
    variables[num_variables].formula = line;
    num_variables++;

    rc = yy_compile(line, line + strlen(line), &stack, &err);

    switch (rc)
    {
        case YY_OK:
            variables[num_variables-1].stack = stackdup(&stack);
            stack.len = 0;
            results[0].name = (yy_str_t){.ptr = name, .len = strlen(name)};
            result = yy_eval_stack(&variables[num_variables-1].stack, &stack, resolve, &results);
            print_token(result);
            break;
        case YY_ERROR_SYNTAX:
            printf("Syntax error at position %ld\n", (err - line) + 1);
            break;
        case YY_ERROR_MEM:
            printf("Not enough memory\n");
            break;
        default:
            printf("Unexpected error (rc=%d)\n", rc);
            break;
    }
}

void finish()
{
    for (int i = 0; i < num_variables; i++)
    {
        free(variables[i].name);
        free(variables[i].formula);
        free(variables[i].stack.data);
    }
}

void interactive_mode(void)
{
    char prompt[32] = {0};

    print_header();

    while (true)
    {
        snprintf(prompt, sizeof(prompt), "%s[%d]> ", PROMT, num_variables + 1);

        char *line = linenoise(prompt);

        if (!line)
            break;

        linenoiseHistoryAdd(line);

        if (strcmp(line, "exit") == 0) {
            free(line);
            return;
        }

        if (strcmp(line, "info") == 0) {
            print_info();
            free(line);
            continue;
        }

        if (strcmp(line, "identifiers") == 0) {
            print_identifiers();
            free(line);
            continue;
        }

        process_line(line);
    }
}

void stream_mode(void)
{
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = 0;

        printf("%s[%d]> %s\n", PROMT, num_variables + 1, buffer);
        process_line(strdup(buffer));
    }
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    srand(time(NULL));

    if (isatty(STDIN_FILENO))
        interactive_mode();
    else
        stream_mode();

    finish();
    return 0;
}
