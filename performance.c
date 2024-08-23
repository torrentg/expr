#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "expr.h"

// #include "tinyexpr.h"

/**
 * Parse and evaluate all expressions in a csv file (with header)
 * with two columns:
 *   - column1 = formula
 *   - column2 = expected result (INVALID if not compilable)
 * 
 * File example:
 * 
 * expression,result
 * sin(tan((215.5585 + 13606810.0658)) / ((0 * 0) - ((-0.1873) / (-913144182.495)))),0.7243120037969705
 * (61795 * 0 - (-590) * 0 - cos((-4.3254)) - (-0.0002) - 2365.5281) - ((0 / 54210) / tan((-1.4142))) + ((-724) * 3041) - ((-79407867) + (-0.0007)),77203817.85020177
 * ((exp((-71413)) - sin((-6932)) + log(cos(0))) * log(sqrt(127355013)) + tan(151676670.8121) - (0 + (-114.8994))),123.58208697938792
 * ((20.2006 / 46956) - ((-1) - (-52458))) - 0 * (-19383.7812) - (342380 + 0.0011) / tan(tan(0.2902) - ((-1720465.9207) * 2.1162)),106353.8580644143
 * sqrt(sqrt((((-0.0027) * (-22)) + exp(0)))),1.0145302297588203
 * (0.3835 / 0.0153 - 0 * 14128) - sqrt(1 - 0) - ((sqrt((-1.7933)) - (130650579 / (-123852587.9881))) / (exp(56935.048) * 0 / (-0.0066))) * sin((cos(cos(log(738566371.7624))) + (((0.0129 * (-218)) / ((-47771907) - 0)) - 0 - 6 + cos(0)))),INVALID
 */

// gcc -O2 -g -DTE_NAT_LOG -DNDEBUG -o performance performance.c expr.c tinyexpr.c -lm

typedef struct input_line_t
{
    char *formula;
    double result;
    struct input_line_t *next;

    bool ok_expr;
    int rc_expr;
    double val_expr;
    bool ok_tinyexpr;
    int rc_tinyexpr;
    double val_tinyexpr;
} input_line_t;

static uint64_t get_millis(void)
{
    struct timespec  ts = {0};

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return 0;

    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

input_line_t * read_file(const char *filename)
{
    char *ptr = NULL;
    char *endptr = NULL;
    char buffer[16000] = {0};
    input_line_t *curr_line = NULL;
    input_line_t *prev_line = NULL;
    input_line_t *ret = NULL;
    FILE *file = NULL;
    
    file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "error: cannot open file '%s'\n", filename);
        return NULL;
    }

    // skips first line
    fgets(buffer, sizeof(buffer), file);

    while (fgets(buffer, sizeof(buffer), file))
    {
        ptr = buffer + strlen(buffer);

        while (ptr > buffer && *ptr != ',')
            --ptr;

        *ptr++ = 0;

        curr_line = calloc(1, sizeof(input_line_t));

        errno = 0;
        curr_line->result = strtod(ptr, &endptr);

        if (errno != 0 || ptr == endptr)
            curr_line->result = NAN;

        curr_line->formula = strdup(buffer);

        if (prev_line != NULL)
            prev_line->next = curr_line;
        else
            ret = curr_line;

        prev_line = curr_line;
        curr_line = NULL;
    }

    fclose(file);

    return ret;
}

void eval_expr(input_line_t *lines)
{
    yy_token_t data[128] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    uint64_t t0 = get_millis();
    int num_ok = 0;
    int num_ko = 0;

    while(lines)
    {
        const char *str = lines->formula;
        yy_retcode_e rc = yy_parse_expr_number(str, str + strlen(str), &stack, NULL);

        double val = stack.data[0].number_val;

        bool success = (rc == YY_OK && fabs(val - lines->result) < 1e-13) || 
                       (rc != YY_OK && isnan(lines->result)) ||
                       (isnan(val) && isnan(lines->result));

        lines->rc_expr = rc;
        lines->val_expr = val;
        lines->ok_expr = success;

        if (success)
            num_ok++;
        else {
            num_ko++;
            //printf("%s\n", str);
            //printf("expected = %lf, result = %lf, rc=%d\n\n", lines->result, stack.data[0].number_val, rc);
        }

        lines = lines->next;
    }

    uint64_t t1 = get_millis();
    printf("eval_expr\n");
    printf("    time = %lu ms\n", t1 - t0);
    printf("    ok = %d\n", num_ok);
    printf("    ko = %d\n", num_ko);
}

#if 0
void eval_tinyexpr(input_line_t *lines)
{
    int error = 0;
    uint64_t t0 = get_millis();
    int num_ok = 0;
    int num_ko = 0;

    while(lines)
    {
        const char *str = lines->formula;
        double val = te_interp(str, &error);

        bool success = (fabs(val - lines->result) < 1e-13) || 
                       (error && isnan(val - lines->result)) ||
                       (isnan(val) && isnan(lines->result));

        lines->rc_tinyexpr = error;
        lines->val_tinyexpr = val;
        lines->ok_tinyexpr = success;

        if (success)
            num_ok++;
        else {
            num_ko++;
            // printf("%s\n", str);
            // printf("expected = %lf, result = %lf, error=%d\n\n", lines->result, val, error);
        }
        
        lines = lines->next;
    }

    uint64_t t1 = get_millis();
    printf("eval_tinyexpr\n");
    printf("    time = %lu ms\n", t1 - t0);
    printf("    ok = %d\n", num_ok);
    printf("    ko = %d\n", num_ko);
}
#endif

void free_lines(input_line_t *lines)
{
    input_line_t *next = NULL;

    while (lines != NULL)
    {
        next = lines->next;
        free(lines->formula);
        free(lines);
        lines = next;
    }
}

int main(int argc, char *argv[])
{
    input_line_t *lines = NULL;
    
    if (argc < 2) {
        fprintf(stderr, "error: no file argument\n");
        exit(EXIT_FAILURE);
    }

    lines = read_file(argv[1]);

    if (!lines)
        return EXIT_FAILURE;

    eval_expr(lines);

    //eval_tinyexpr(lines);

    // input_line_t *line = lines;

    // while (line)
    // {
    //     if (!line->ok_expr || !line->ok_tinyexpr)
    //         if (line->val_expr != line->val_tinyexpr)
    //     {
    //         printf("%s\n", line->formula);
    //         printf("expected: %lf\n", line->result);
    //         printf("expr: rc=%d, result=%lf\n", line->rc_expr, line->val_expr);
    //         printf("tiny: rc=%d, result=%lf\n", line->rc_tinyexpr, line->val_tinyexpr);
    //         printf("\n");
    //     }

    //     line = line->next;
    // }

    free_lines(lines);

    return EXIT_SUCCESS;
}
