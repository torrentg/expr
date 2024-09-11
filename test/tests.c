#include "acutest.h"
#include "expr.c"

#define EPSILON 1e-14

yy_token_t make_datetime(const char *str) {
    return yy_parse_datetime(str, str + strlen(str));
}

yy_symbol_e get_function_symbol(void (*func)(void))
{
    for (size_t i = 0; i < (size_t) YY_SYMBOL_END; i++)
    {
        if (symbol_to_token[i].type != YY_TOKEN_FUNCTION)
            continue;
        
        if (symbol_to_token[i].function.ptr == func)
            return (yy_symbol_e) i;
    }

    return YY_SYMBOL_NONE;
}

const char * symbol_to_str(yy_symbol_e type)
{
    switch (type)
    {
        case YY_SYMBOL_NONE: return "NONE";
        case YY_SYMBOL_TRUE: return "TRUE";
        case YY_SYMBOL_FALSE: return "FALSE";
        case YY_SYMBOL_NUMBER_VAL: return "NUMBER_VAL";
        case YY_SYMBOL_DATETIME_VAL: return "DATETIME_VAL";
        case YY_SYMBOL_STRING_VAL: return "STRING_VAL";
        case YY_SYMBOL_VARIABLE: return "VARIABLE";
        case YY_SYMBOL_CONST_E: return "CONST_E";
        case YY_SYMBOL_CONST_PI: return "CONST_PI";
        case YY_SYMBOL_PAREN_LEFT: return "PAREN_LEFT";
        case YY_SYMBOL_PAREN_RIGHT: return "PAREN_RIGHT";
        case YY_SYMBOL_COMMA: return "COMMA";
        case YY_SYMBOL_AND_OP: return "AND_OP";
        case YY_SYMBOL_OR_OP: return "OR_OP";
        case YY_SYMBOL_EQUALS_OP: return "EQUALS_OP";
        case YY_SYMBOL_DISTINCT_OP: return "DISTINCT_OP";
        case YY_SYMBOL_LESS_OP: return "LESS_OP";
        case YY_SYMBOL_LESS_EQUALS_OP: return "LESS_EQUALS_OP";
        case YY_SYMBOL_GREAT_OP: return "GREAT_OP";
        case YY_SYMBOL_GREAT_EQUALS_OP: return "GREAT_EQUALS_OP";
        case YY_SYMBOL_PLUS_OP: return "PLUS_OP";
        case YY_SYMBOL_MINUS_OP: return "MINUS_OP";
        case YY_SYMBOL_ADDITION_OP: return "ADDITION_OP";
        case YY_SYMBOL_SUBTRACTION_OP: return "SUBTRACTION_OP";
        case YY_SYMBOL_PRODUCT_OP: return "PRODUCT_OP";
        case YY_SYMBOL_DIVIDE_OP: return "DIVIDE_OP";
        case YY_SYMBOL_MODULO_OP: return "MODULO_OP";
        case YY_SYMBOL_POWER_OP: return "POWER_OP";
        case YY_SYMBOL_ABS: return "ABS";
        case YY_SYMBOL_MIN: return "MIN";
        case YY_SYMBOL_MAX: return "MAX";
        case YY_SYMBOL_MODULO: return "MODULO";
        case YY_SYMBOL_POWER: return "POWER";
        case YY_SYMBOL_SQRT: return "SQRT";
        case YY_SYMBOL_SIN: return "SIN";
        case YY_SYMBOL_COS: return "COS";
        case YY_SYMBOL_TAN: return "TAN";
        case YY_SYMBOL_EXP: return "EXP";
        case YY_SYMBOL_LOG: return "LOG";
        case YY_SYMBOL_TRUNC: return "TRUNC";
        case YY_SYMBOL_CEIL: return "CEIL";
        case YY_SYMBOL_FLOOR: return "FLOOR";
        case YY_SYMBOL_RANDOM: return "RANDOM";
        case YY_SYMBOL_NOW: return "NOW";
        case YY_SYMBOL_DATEPART: return "DATEPART";
        case YY_SYMBOL_DATEADD: return "DATEADD";
        case YY_SYMBOL_DATETRUNC: return "DATETRUNC";
        case YY_SYMBOL_LENGTH: return "LENGTH";
        case YY_SYMBOL_FIND: return "FIND";
        case YY_SYMBOL_LOWER: return "LOWER";
        case YY_SYMBOL_UPPER: return "UPPER";
        case YY_SYMBOL_TRIM: return "TRIM";
        case YY_SYMBOL_CONCAT_OP: return "CONCAT";
        case YY_SYMBOL_SUBSTR: return "SUBSTR";
        case YY_SYMBOL_UNESCAPE: return "UNESCAPE";
        case YY_SYMBOL_REPLACE: return "REPLACE";
        case YY_SYMBOL_CLAMP: return "CLAMP";
        case YY_SYMBOL_NOT: return "NOT";
        case YY_SYMBOL_ISNAN: return "ISNAN";
        case YY_SYMBOL_ISINF: return "ISINF";
        case YY_SYMBOL_ISERROR: return "ISERROR";
        case YY_SYMBOL_IFELSE: return "IFELSE";
        case YY_SYMBOL_STR: return "STR";
        case YY_SYMBOL_VARIABLE_FUNC: return "VARIABLE_FUNC";
        case YY_SYMBOL_END: return "END";
        default: return "UNKNOW";
    }
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

const char * get_function_name(void (*func)(void))
{
    return symbol_to_str(get_function_symbol(func));
}

void print_token(yy_token_t token)
{
    char buf[128] = {0};

    switch(token.type)
    {
        case YY_TOKEN_NULL:
            printf("NULL");
            break;
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
            printf("\"%.*s\"", token.str_val.len, token.str_val.ptr);
            break;
        case YY_TOKEN_VARIABLE:
            printf("%.*s", token.variable.len, token.variable.ptr);
            break;
        case YY_TOKEN_FUNCTION:
            printf("%s", get_function_name(token.function.ptr));
            break;
        case YY_TOKEN_ERROR:
            printf("%s", error_to_str(token.error));
            break;
        default:
            printf("UNKNOW");
    }
}

void print_stack(const yy_stack_t *stack)
{
    for (size_t i = 0; i < stack->len; i++)
    {
        print_token(stack->data[i]);
        printf(" ");
    }

    yy_token_t data[128] = {0};
    yy_stack_t aux = {.data = data, .len = 0, .reserved = sizeof(data)/sizeof(data[0])};
    yy_token_t result = yy_eval_stack(stack, &aux, NULL, NULL);

    printf( " = ");
    print_token(result);
}

yy_token_t resolve(yy_str_t var, void *data)
{
    UNUSED(data);

    if (!var.ptr)
        return token_error(YY_ERROR);

    if (var.len != 1)
        return token_error(YY_ERROR_REF);

    switch (var.ptr[0])
    {
        case 'a': return token_number(0);
        case 'b': return token_number(1);
        case 'c': return token_number(2);

        case 'd': return token_datetime(1725776766211);

        case 'm': return token_bool(true);
        case 'n': return token_bool(false);

        case 'p': return token_string("Bob", 3);
        case 'q': return token_string("John", 4);
        case 's': return token_string("lorem ipsum", 11);

        case 'u': return token_error(YY_ERROR_SYNTAX);
        case 'v': return token_error(YY_ERROR_VALUE);
        case 'w': return token_error(YY_ERROR_CREF);

        case 'x': return token_number(0.5);
        case 'y': return token_number(M_PI);
        case 'z': return token_number(1.0/3.0);

        default: return token_error(YY_ERROR_REF);
    }
}

void check_parse_number_ok(const char *str, double expected_val)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_number(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_MSG("Case='%s', error=failed", str);
    TEST_CHECK(fabs(result.number_val - expected_val) < EPSILON);
    TEST_MSG("Case='%s', expected=%lf, result=%lf", str, expected_val, result.number_val);
}

void check_parse_number_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_number(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void check_parse_datetime_ok(const char *str, uint64_t expected_val)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_datetime(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_MSG("Case='%s', error=failed", str);
    TEST_CHECK(result.datetime_val == expected_val);
    TEST_MSG("Case='%s', expected=%lu, result=%lu", str, expected_val, result.datetime_val);
}

void check_parse_datetime_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_datetime(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void check_parse_boolean_ok(const char *str, bool expected_val)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_bool(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_MSG("Case='%s', error=not-a-boolean", str);
    TEST_CHECK(expected_val == result.bool_val);
    TEST_MSG("Case='%s', error=unexpected-value", str);
}

void check_parse_boolean_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse_bool(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void check_parse_string_ok(const char *str, const char *expected_val)
{
    size_t len = strlen(str);
    uint32_t expected_len = strlen(expected_val);

    yy_token_t result = yy_parse_string(str, str + len);

    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_MSG("Case='%s', error=not-a-string", str);
    TEST_CHECK(expected_len == result.str_val.len);
    TEST_MSG("Case='%s', error=unexpected-length", str);
    TEST_CHECK(strncmp(expected_val, result.str_val.ptr, expected_len) == 0);
    TEST_MSG("Case='%s', error=unexpected-value", str);
}

void check_parse_string_ko(const char *begin, const char *end)
{
    yy_token_t result = yy_parse_string(begin, end);

    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='not-printed', error=not-failed");
}

void check_parse_ok(const char *str, yy_token_e type)
{
    size_t len = strlen(str);

    yy_token_t result = yy_parse(str, str + len);

    TEST_CHECK(result.type == type);
    TEST_MSG("Case='%s', error=failed, result=%d, expected=%d", str, result.type, type);
}

void check_read_symbol_string_ok(const char *str, const char *expected_val)
{
    yy_symbol_t symbol = {0};
    size_t len = strlen(str);
    size_t expected_len = strlen(expected_val);

    yy_error_e rc = read_symbol_string(str, str + len, &symbol);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%.*s', error=failed", (int) len, str);
    TEST_CHECK(symbol.type == YY_SYMBOL_STRING_VAL || symbol.type == YY_SYMBOL_ESCAPED_STRING_VAL);
    TEST_MSG("Case='%.*s', error=not-a-string, type=%d", (int) len, str, symbol.type);
    TEST_CHECK(symbol.lexeme.ptr == str);
    TEST_MSG("Case='%.*s', error=invalid-lexeme-ptr", (int) len, str);
    TEST_CHECK(symbol.lexeme.len == expected_len + 2);
    TEST_MSG("Case='%.*s', error=invalid-lexeme-len", (int) len, str);
    TEST_CHECK(symbol.str_val.len == expected_len);
    TEST_MSG("Case='%.*s', error=invalid-string-len", (int) len, str);
    TEST_CHECK(strncmp(symbol.str_val.ptr, expected_val, expected_len) == 0);
    TEST_MSG("Case='%.*s', error=invalid-string-content", (int) len, str);
}

void check_read_symbol_string_ko(const char *str)
{
    yy_symbol_t symbol = {0};
    size_t len = (str == NULL ? 0 : strlen(str));
    const char *end = (str == NULL ? NULL : str + len);

    yy_error_e rc = read_symbol_string(str, end, &symbol);

    TEST_CHECK(rc != YY_OK);
    TEST_MSG("Case='%s', error=non-failed", str);
}

void check_read_symbol_variable_ok(const char *str, const char *expected_val)
{
    yy_symbol_t symbol = {0};
    size_t len = strlen(str);
    size_t expected_len = strlen(expected_val);

    yy_error_e rc = read_symbol_variable(str, str + len, &symbol);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%.*s', error=failed", (int) len, str);
    TEST_CHECK(symbol.type == YY_SYMBOL_VARIABLE);
    TEST_MSG("Case='%.*s', error=not-a-variable", (int) len, str);
    TEST_CHECK(symbol.variable.len == expected_len);
    TEST_MSG("Case='%.*s', error=invalid-length", (int) len, str);
    TEST_CHECK(strncmp(symbol.variable.ptr, expected_val, expected_len) == 0);
    TEST_MSG("Case='%.*s', expected=%s, result=%.*s", (int) len, str, expected_val, (int) symbol.variable.len, symbol.variable.ptr);
    TEST_CHECK(symbol.lexeme.len == expected_len + (str[1] == '{' ? 3 : 1));
    TEST_MSG("Case='%.*s', error=invalid-lexeme-len", (int) len, str);
}

void check_read_symbol_variable_ko(const char *str)
{
    yy_symbol_t symbol = {0};
    const char *end = str + (str ? strlen(str) : 0);

    yy_error_e rc = read_symbol_variable(str, end, &symbol);

    TEST_CHECK(rc != YY_OK);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void check_next_ok(const char *str, yy_symbol_e type, yy_symbol_t *symbol)
{
    const char *begin = str;
    const char *end = str + strlen(str);

    yy_error_e rc = read_symbol(begin, end, symbol);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%s', error=failed", str);
    TEST_CHECK(symbol->type == type);
    TEST_MSG("Case='%s', expected=%d, result=%d", str, type, symbol->type);
}

void check_next_ko(const char *str)
{
    const char *begin = str;
    const char *end = str + strlen(str);
    yy_symbol_t symbol = {0};

    yy_error_e rc = read_symbol(begin, end, &symbol);

    TEST_CHECK(rc != YY_OK);
    TEST_MSG("Case='%s', error=failed", str);
}

void check_skip_spaces(const char *str, int end_len, size_t expected_len)
{
    const char *end = str + (end_len < 0 ? (ptrdiff_t) strlen(str) : (ptrdiff_t) end_len);
    const char *ptr = skip_spaces(str, end);

    TEST_CHECK(ptr && ptr <= end);
    TEST_MSG("Case='%s', failed", str);
    TEST_CHECK((size_t)(ptr - str) == expected_len);
    TEST_MSG("Case='%s', expected=%d, result=%d", str, (int) expected_len, (int) (ptr - str));
}

void check_eval_number_ok(const char *str, double expected)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_token_t result = yy_eval_number(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_MSG("Case='%s', error=unexpected-type, type=%d", str, result.type);

    TEST_CHECK(fabs(expected - result.number_val) < EPSILON);
    TEST_MSG("Case='%s', error=unexpected-value, expected=%lf, result=%lf", str, expected, result.number_val);
}

void check_eval_number_ko(const char *str, yy_error_e expected_err)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_token_t result = yy_eval_number(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=unexpected-type, type=%d", str, result.type);

    TEST_CHECK(result.error == expected_err);
    TEST_MSG("Case='%s', error=unexpected-error, expected=%d, result=%d", str, expected_err, result.error);
}

void check_eval_datetime_ok(const char *str, const char *expected_str)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t expected = yy_parse_datetime(expected_str, expected_str + strlen(expected_str));

    yy_token_t result = yy_eval_datetime(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_MSG("Case='%s', error=unexpected-type, type=%d", str, result.type);

    char buf[1024] = {0};
    TEST_CHECK(result.datetime_val == expected.datetime_val);
    TEST_MSG("Case='%s', error=unexpected-value, expected=%s, result=%s", 
        str, expected_str, datetime_to_str(result.datetime_val, buf));
}

void check_eval_string_ok(const char *str, const char *expected)
{
    yy_token_t data[256] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_token_t result = yy_eval_string(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_MSG("Case='%s', error=unexpected-type, type=%d", str, result.type);

    TEST_CHECK(str_cmp(result.str_val, (yy_str_t){expected, strlen(expected)}) == 0);
    TEST_MSG("Case='%s', error=unexpected-value, expected=%s, result=%.*s", 
        str, expected, result.str_val.len, result.str_val.ptr);
}

void check_eval_bool_ok(const char *str, bool expected)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_token_t result = yy_eval_bool(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_MSG("Case='%s', error=unexpected-type, type=%d", str, result.type);

    TEST_CHECK(result.bool_val == expected);
    TEST_MSG("Case='%s', error=unexpected-value, expected=%d, result=%d", str, expected, result.bool_val);
}

void check_eval_ok(const char *str, yy_token_e type)
{
    yy_token_t data[128] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_token_t result = yy_eval(str, str + strlen(str), &stack, resolve, NULL);

    TEST_CHECK(result.type == type);
    TEST_MSG("Case='%s', error=unexpected-type, result=%d expected=%d", str, result.type, type);
}

// ==============

void check_dateadd(const char *str_date, int val, const char *str_part, const char *str_expected)
{
    yy_token_t date = yy_parse_datetime(str_date, str_date + strlen(str_date));
    TEST_CHECK(date.type == YY_TOKEN_DATETIME);

    yy_token_t value = token_number(val);
    yy_token_t part = token_number(get_datepart(&((yy_str_t){str_part, strlen(str_part)})));

    yy_token_t result = func_dateadd(date, value, part);

    if (str_expected)
    {
        yy_token_t expected = yy_parse_datetime(str_expected, str_expected + strlen(str_expected));
        TEST_CHECK(date.type == YY_TOKEN_DATETIME);

        TEST_CHECK(result.type == YY_TOKEN_DATETIME);
        TEST_CHECK(result.datetime_val == expected.datetime_val);
    }
    else
    {
        TEST_CHECK(result.type == YY_TOKEN_ERROR);
    }
}

void check_dateset(const char *str_date, int val, const char *str_part, const char *str_expected)
{
    yy_token_t date = yy_parse_datetime(str_date, str_date + strlen(str_date));
    TEST_CHECK(date.type == YY_TOKEN_DATETIME);

    yy_token_t value = token_number(val);
    yy_token_t part = token_number(get_datepart(&((yy_str_t){str_part, strlen(str_part)})));

    yy_token_t result = func_dateset(date, value, part);

    if (str_expected)
    {
        yy_token_t expected = yy_parse_datetime(str_expected, str_expected + strlen(str_expected));
        TEST_CHECK(date.type == YY_TOKEN_DATETIME);

        TEST_CHECK(result.type == YY_TOKEN_DATETIME);
        TEST_CHECK(result.datetime_val == expected.datetime_val);
        TEST_MSG("Case='%s', failed", str_expected);
    }
    else
    {
        TEST_CHECK(result.type == YY_TOKEN_ERROR);
    }
}

void check_datetrunc(const char *str_date, const char *str_part, const char *str_expected)
{
    yy_token_t date = yy_parse_datetime(str_date, str_date + strlen(str_date));
    TEST_CHECK(date.type == YY_TOKEN_DATETIME);

    yy_token_t part = token_number(get_datepart(&((yy_str_t){str_part, strlen(str_part)})));

    yy_token_t result = func_datetrunc(date, part);

    if (str_expected)
    {
        yy_token_t expected = yy_parse_datetime(str_expected, str_expected + strlen(str_expected));
        TEST_CHECK(date.type == YY_TOKEN_DATETIME);

        TEST_CHECK(result.type == YY_TOKEN_DATETIME);
        TEST_CHECK(result.datetime_val == expected.datetime_val);
        TEST_MSG("Case='%s', failed", str_expected);
    }
    else
    {
        TEST_CHECK(result.type == YY_TOKEN_ERROR);
    }
}

// ==============

void test_parse_number_ok(void)
{
    check_parse_number_ok("0", 0);
    check_parse_number_ok("1", 1);
    check_parse_number_ok("1234", 1234);
    check_parse_number_ok("100000000", 100000000);
    check_parse_number_ok("9007199254740992" , 9007199254740992LL); // 2^53

    check_parse_number_ok("0e0"     , 0.0);
    check_parse_number_ok("0e+0"    , 0.0);
    check_parse_number_ok("0e-0"    , 0.0);
    check_parse_number_ok("0.0"     , 0.0);
    check_parse_number_ok("0.0e0"   , 0.0);
    check_parse_number_ok("0.0e+0"  , 0.0);
    check_parse_number_ok("0.0e-0"  , 0.0);

    check_parse_number_ok("0e1"     , 0.0);
    check_parse_number_ok("0e+1"    , 0.0);
    check_parse_number_ok("0e-1"    , 0.0);
    check_parse_number_ok("0.0"     , 0.0);
    check_parse_number_ok("0.0e1"   , 0.0);
    check_parse_number_ok("0.0e+1"  , 0.0);
    check_parse_number_ok("0.0e-1"  , 0.0);

    check_parse_number_ok("3.14"    , 3.14);
    check_parse_number_ok("3.14e0"  , 3.14);
    check_parse_number_ok("314e-2"  , 3.14);
    check_parse_number_ok("314e+2"  , 31400.0);
    check_parse_number_ok("314.0e-2", 3.14);
    check_parse_number_ok("0.314e1" , 3.14);
    check_parse_number_ok("0.314e+1", 3.14);

    check_parse_number_ok("+0", 0.0);
    check_parse_number_ok("-0", 0.0);
    check_parse_number_ok("+1", 1.0);
    check_parse_number_ok("-1", -1.0);
    check_parse_number_ok("+1.0", 1.0);
    check_parse_number_ok("-1.0", -1.0);
    check_parse_number_ok("+1e3", 1000.0);
    check_parse_number_ok("-1e3", -1000.0);
    check_parse_number_ok("+1.5e3", 1500.0);
    check_parse_number_ok("-1.5e3", -1500.0);
    check_parse_number_ok("+1.5e+3", 1500.0);
    check_parse_number_ok("-1.5e-3", -0.0015);
}

void test_parse_number_ko(void)
{
    check_parse_number_ko("");
    check_parse_number_ko(" ");
    check_parse_number_ko("  ");
    check_parse_number_ko(" 1"); // initial space
    check_parse_number_ko("1 "); // ending space
    check_parse_number_ko("+ 1"); // intermediate space
    check_parse_number_ko("+");
    check_parse_number_ko("-");
    check_parse_number_ko("a");
    check_parse_number_ko("a1");
    check_parse_number_ko("00");
    check_parse_number_ko("01");
    check_parse_number_ko("9007199254740993"); // 2^53 + 1
    check_parse_number_ko("10000000000000000");

    check_parse_number_ko(".0");
    check_parse_number_ko(".5");
    check_parse_number_ko("1.");
    check_parse_number_ko("1.a");
    check_parse_number_ko("1.e1");
    check_parse_number_ko("1.2e");
    check_parse_number_ko("1.2ea");
    check_parse_number_ko("1.2e+");
    check_parse_number_ko("1.2e+a");
    check_parse_number_ko("1e01");
    check_parse_number_ko("1e+01");
    check_parse_number_ko("1e+1000000");
    check_parse_number_ko("++0");
    check_parse_number_ko("+-0");
}

void test_parse_datetime_ok(void)
{
    check_parse_datetime_ok("1970-01-01T00:00:00.000Z", 0);
    check_parse_datetime_ok("1970-11-01T00:00:00.000Z", 26265600000);
    check_parse_datetime_ok("2024-07-28T09:27:43.678Z", 1722158863678);
    check_parse_datetime_ok("2024-07-28T09:27:43.678", 1722158863678);
    check_parse_datetime_ok("2024-07-28T09:27:43.1", 1722158863001);
    check_parse_datetime_ok("2024-07-28T09:27:43.12", 1722158863012);
    check_parse_datetime_ok("2024-07-28T09:27:43", 1722158863000);
    check_parse_datetime_ok("2024-07-28T09:27:43Z", 1722158863000);
    check_parse_datetime_ok("2024-07-28T23:27:43Z", 1722209263000);
    check_parse_datetime_ok("2024-07-28T09:27:43.1Z", 1722158863001);
    check_parse_datetime_ok("2024-07-28T09:27:43.12Z", 1722158863012);
    check_parse_datetime_ok("2024-07-28", 1722124800000);
    check_parse_datetime_ok("2024-02-29", 1709164800000); // 29-feb of leap-year
}

void test_parse_datetime_ko(void)
{
    check_parse_datetime_ko("");
    check_parse_datetime_ko(" ");
    check_parse_datetime_ko("a");
    check_parse_datetime_ko("T");

    // range error
    check_parse_datetime_ko(" 1970-01-01T00:00:00.000Z");
    check_parse_datetime_ko("1970-01-01T00:00:00.000Z ");
    check_parse_datetime_ko(" 1970-01-01");
    check_parse_datetime_ko("1970-01-01 ");

    // additional symbol after end
    check_parse_datetime_ko("2024-07-28T09:27:43.678Z+");
    check_parse_datetime_ko("2024-07-28T09:27:43.678+");
    check_parse_datetime_ko("2024-07-28T09:27:43+");
    check_parse_datetime_ko("2024-07-28+");
    check_parse_datetime_ko("2024-07-28T");
    check_parse_datetime_ko("2024-07-28T13:54");

    // invalid year
    check_parse_datetime_ko("197a-01-01T00:00:00.000Z");
    check_parse_datetime_ko("1824-07-28T09:27:43.678Z");
    check_parse_datetime_ko("1924-07-28T09:27:43.678Z");
    check_parse_datetime_ko("924-07-28T09:27:43.678Z");
    check_parse_datetime_ko("24-07-28T09:27:43.678Z");
    check_parse_datetime_ko("4-07-28T09:27:43.678Z");
    check_parse_datetime_ko("-07-28T09:27:43.678Z");

    // invalid month
    check_parse_datetime_ko("2024--28T09:27:43.678Z");
    check_parse_datetime_ko("2024-a-28T09:27:43.678Z");
    check_parse_datetime_ko("2024-1a-28T09:27:43.678Z");
    check_parse_datetime_ko("2024-1-28T09:27:43.678Z");
    check_parse_datetime_ko("2024-0-28T09:27:43.678Z");
    check_parse_datetime_ko("202428T09:27:43.678Z");
    check_parse_datetime_ko("2024-00-28T09:27:43.678Z");
    check_parse_datetime_ko("2024-13-28T09:27:43.678Z");
    check_parse_datetime_ko("2024-20-28T09:27:43.678Z");

    // invalid day
    check_parse_datetime_ko("2024-07T09:27:43.678Z");
    check_parse_datetime_ko("2024-07-T09:27:43.678Z");
    check_parse_datetime_ko("2024-07-1T09:27:43.678Z");
    check_parse_datetime_ko("2024-07-aT09:27:43.678Z");
    check_parse_datetime_ko("2024-07-1aT09:27:43.678Z");
    check_parse_datetime_ko("2024-07-0T09:27:43.678Z");
    check_parse_datetime_ko("2024-07-00T09:27:43.678Z");
    check_parse_datetime_ko("2024-07-32T09:27:43.678Z");

    // T separator
    check_parse_datetime_ko("2024-07-2809:27:43.678Z");
    check_parse_datetime_ko("2024-07-28t09:27:43.678Z");
    check_parse_datetime_ko("2024-07-28x09:27:43.678Z");
    check_parse_datetime_ko("2024-07-28 09:27:43.678Z");

    // invalid hour
    check_parse_datetime_ko("2024-07-28T27:43.678Z");
    check_parse_datetime_ko("2024-07-28T:27:43.678Z");
    check_parse_datetime_ko("2024-07-28Ta:27:43.678Z");
    check_parse_datetime_ko("2024-07-28T1a:27:43.678Z");
    check_parse_datetime_ko("2024-07-28T0:27:43.678Z");
    check_parse_datetime_ko("2024-07-28T24:27:43.678Z");
    check_parse_datetime_ko("2024-07-28T002:27:43.678Z");

    // invalid minute
    check_parse_datetime_ko("2024-07-28T09:a:43.678Z");
    check_parse_datetime_ko("2024-07-28T09::43.678Z");
    check_parse_datetime_ko("2024-07-28T09: :43.678Z");
    check_parse_datetime_ko("2024-07-28T09:a:43.678Z");
    check_parse_datetime_ko("2024-07-28T09:0:43.678Z");
    check_parse_datetime_ko("2024-07-28T09:1:43.678Z");
    check_parse_datetime_ko("2024-07-28T09:60:43.678Z");
    check_parse_datetime_ko("2024-07-28T09:004:43.678Z");

    // invalid second
    check_parse_datetime_ko("2024-07-28T09:27:678Z");
    check_parse_datetime_ko("2024-07-28T09:27: .678Z");
    check_parse_datetime_ko("2024-07-28T09:27:a.678Z");
    check_parse_datetime_ko("2024-07-28T09:27:0.678Z");
    check_parse_datetime_ko("2024-07-28T09:27:1.678Z");
    check_parse_datetime_ko("2024-07-28T09:27:60.678Z");
    check_parse_datetime_ko("2024-07-28T09:27:004.678Z");

    // invalid millis
    check_parse_datetime_ko("2024-07-28T09:27:43.");
    check_parse_datetime_ko("2024-07-28T09:27:43.Z");
    check_parse_datetime_ko("2024-07-28T09:27:43. ");
    check_parse_datetime_ko("2024-07-28T09:27:43.a");
    check_parse_datetime_ko("2024-07-28T09:27:43.1a");
    check_parse_datetime_ko("2024-07-28T09:27:43.12a");
    check_parse_datetime_ko("2024-07-28T09:27:43.123a");
    check_parse_datetime_ko("2024-07-28T09:27:43.1234");
    check_parse_datetime_ko("2024-07-28T09:27:43.+123");

    // zulu time
    check_parse_datetime_ko("2024-07-28T09:27:43.678z");
    check_parse_datetime_ko("2024-07-28T09:27:43.678ZZ");

    // invalid date
    check_parse_datetime_ko("2024-02-31"); // 31-feb!
    check_parse_datetime_ko("2023-02-29"); // non-leap-year
}

void test_read_symbol_string_ok(void)
{
    check_read_symbol_string_ok("\"\"", "");

    check_read_symbol_string_ok("\"abc\"", "abc");

    check_read_symbol_string_ok("\"\\n\"", "\\n");
    check_read_symbol_string_ok("\"\\nabc\"", "\\nabc");
    check_read_symbol_string_ok("\"abc\\n\"", "abc\\n");
    check_read_symbol_string_ok("\"abc\\ndef\"", "abc\\ndef");

    check_read_symbol_string_ok("\"\\t\"", "\\t");
    check_read_symbol_string_ok("\"\\tabc\"", "\\tabc");
    check_read_symbol_string_ok("\"abc\\t\"", "abc\\t");
    check_read_symbol_string_ok("\"abc\\tdef\"", "abc\\tdef");

    check_read_symbol_string_ok("\"\\\\\"", "\\\\");
    check_read_symbol_string_ok("\"\\\\abc\"", "\\\\abc");
    check_read_symbol_string_ok("\"abc\\\\\"", "abc\\\\");
    check_read_symbol_string_ok("\"abc\\\\def\"", "abc\\\\def");

    check_read_symbol_string_ok("\"\\\"\"", "\\\"");
    check_read_symbol_string_ok("\"\\\"abc\"", "\\\"abc");
    check_read_symbol_string_ok("\"abc\\\"\"", "abc\\\"");
    check_read_symbol_string_ok("\"abc\\\"def\"", "abc\\\"def");

    check_read_symbol_string_ok("\"\\n\\\\\\t\\\"\"", "\\n\\\\\\t\\\"");

    check_read_symbol_string_ok("\"abc\\xdef\"", "abc\\xdef");  // not escaped string
}

void test_read_symbol_string_ko(void)
{
    check_read_symbol_string_ko("");
    check_read_symbol_string_ko(" ");
    check_read_symbol_string_ko("a");
    check_read_symbol_string_ko(" \"abc\"");
    check_read_symbol_string_ko("\"");
    check_read_symbol_string_ko("\"non terminated str");

    check_read_symbol_string_ko("\"\\\"");
    check_read_symbol_string_ko("\"\\t");
    check_read_symbol_string_ko("\"\\n");
    check_read_symbol_string_ko("\"\\\\");

    check_read_symbol_string_ko("\"abc\\\"");
    check_read_symbol_string_ko("\"abc\\t");
    check_read_symbol_string_ko("\"abc\\n");
    check_read_symbol_string_ko("\"abc\\\\");

    check_read_symbol_string_ko("\"\\\"abc");
    check_read_symbol_string_ko("\"\\tabc");
    check_read_symbol_string_ko("\"\\nabc");
    check_read_symbol_string_ko("\"\\\\abc");

    // 0 in-the-middle
    yy_symbol_t symbol = {0};
    char str[] = { '"', 'a', 'b', 'c', 0, 'd', 'e', 'f', '"', 0};
    TEST_CHECK(read_symbol_string(str, str + sizeof(str) - 1, &symbol) == YY_ERROR_SYNTAX);
}

void test_parse_boolean_ok(void)
{
    check_parse_boolean_ok("true", true);
    check_parse_boolean_ok("True", true);
    check_parse_boolean_ok("TRUE", true);

    check_parse_boolean_ok("false", false);
    check_parse_boolean_ok("False", false);
    check_parse_boolean_ok("FALSE", false);
}

void test_parse_boolean_ko(void)
{
    check_parse_boolean_ko("");
    check_parse_boolean_ko(" ");
    check_parse_boolean_ko(" true");
    check_parse_boolean_ko(" false");

    check_parse_boolean_ko("aaa");

    check_parse_boolean_ko("txue");
    check_parse_boolean_ko("tRue");
    check_parse_boolean_ko("trUe");
    check_parse_boolean_ko("TrUE");
    check_parse_boolean_ko("tRUE");

    check_parse_boolean_ko("fxlse");
    check_parse_boolean_ko("fAlse");
    check_parse_boolean_ko("falsE");
    check_parse_boolean_ko("falsE");
    check_parse_boolean_ko("FaLSE");
    check_parse_boolean_ko("fALSE");

    check_parse_boolean_ko("trueX");
    check_parse_boolean_ko("TrueX");
    check_parse_boolean_ko("TRUEX");
    check_parse_boolean_ko("falseX");
    check_parse_boolean_ko("FalseX");
    check_parse_boolean_ko("FALSEX");
}

void test_parse_string_ok(void)
{
    check_parse_string_ok("", "");
    check_parse_string_ok("abc", "abc");
    check_parse_string_ok("escaped\\tstring\\n", "escaped\\tstring\\n");
}

void test_parse_string_ko(void)
{
    const char *str1 = "abc";
    const char str2[] = {'a', 'b', 'c', '\0', 'x', 'y', 'z'};

    check_parse_string_ko(NULL, NULL);
    check_parse_string_ko("a", NULL);
    check_parse_string_ko(NULL, "z");
    check_parse_string_ko(str1 + 2, str1);
    check_parse_string_ko(str1, str1 + UINT32_MAX + 10);
    check_parse_string_ko(str2, str2 + sizeof(str2));
}

void test_parse_ok(void)
{
    check_parse_ok("1", YY_TOKEN_NUMBER);
    check_parse_ok("3.14", YY_TOKEN_NUMBER);
    check_parse_ok("true", YY_TOKEN_BOOL);
    check_parse_ok("2024-09-10", YY_TOKEN_DATETIME);
    check_parse_ok("abc...xyz", YY_TOKEN_STRING);
}

void test_parse_ko(void)
{
    yy_token_t result = {0};
    const char str[] = {'a', '\0', 'b'};
    
    result = yy_parse(NULL, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    result = yy_parse(str, str + 3);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);
}

void test_read_symbol_variable_ok(void)
{
    check_read_symbol_variable_ok("$x", "x");
    check_read_symbol_variable_ok("$x_", "x_");
    check_read_symbol_variable_ok("$x__", "x__");
    check_read_symbol_variable_ok("$abc", "abc");
    check_read_symbol_variable_ok("$abc_xyz", "abc_xyz");
    check_read_symbol_variable_ok("$abc_xyz_", "abc_xyz_");
    check_read_symbol_variable_ok("${x}", "x");
    check_read_symbol_variable_ok("${a.b.c}", "a.b.c");
    check_read_symbol_variable_ok("${abc_def}", "abc_def");
    check_read_symbol_variable_ok("${ABC_DEF.GHI}", "ABC_DEF.GHI");
    check_read_symbol_variable_ok("${x01.A23._56}", "x01.A23._56");
    check_read_symbol_variable_ok("${x__}", "x__");
    check_read_symbol_variable_ok("${x._._._}", "x._._._");
    check_read_symbol_variable_ok("${x[1]}", "x[1]");
    check_read_symbol_variable_ok("${a b c}", "a b c");
    check_read_symbol_variable_ok("${ }", " ");
}

void test_read_symbol_variable_ko(void)
{
    check_read_symbol_variable_ko("");
    check_read_symbol_variable_ko(" ");
    check_read_symbol_variable_ko(" $x"); // not trimmed
    check_read_symbol_variable_ko(" ${x}"); // not trimmed
    check_read_symbol_variable_ko("$ x");
    check_read_symbol_variable_ko("$_x");
    check_read_symbol_variable_ko("$+");
    check_read_symbol_variable_ko("$ñ");
    check_read_symbol_variable_ko("$[1]");
    check_read_symbol_variable_ko("$_");
    check_read_symbol_variable_ko("${");
    check_read_symbol_variable_ko("${}");
    check_read_symbol_variable_ko("${{a}");
    check_read_symbol_variable_ko("${ab{cd}");
}

void test_read_symbol_ok(void)
{
    yy_symbol_t symbol = {0};

    check_next_ok("", YY_SYMBOL_END, &symbol);
    check_next_ok("42 + ", YY_SYMBOL_NUMBER_VAL, &symbol);
    assert(symbol.number_val == 42);
    check_next_ok("3.14- ", YY_SYMBOL_NUMBER_VAL, &symbol);
    assert(symbol.number_val == 3.14);
    check_next_ok("true", YY_SYMBOL_TRUE, &symbol);
    check_next_ok("True", YY_SYMBOL_TRUE, &symbol);
    check_next_ok("TRUE", YY_SYMBOL_TRUE, &symbol);
    check_next_ok("false", YY_SYMBOL_FALSE, &symbol);
    check_next_ok("False", YY_SYMBOL_FALSE, &symbol);
    check_next_ok("FALSE", YY_SYMBOL_FALSE, &symbol);
    check_next_ok("E", YY_SYMBOL_CONST_E, &symbol);
    check_next_ok("PI ", YY_SYMBOL_CONST_PI, &symbol);
    check_next_ok("\"abcdef\"", YY_SYMBOL_STRING_VAL, &symbol);
    assert(symbol.str_val.len == 6);
    assert(strncmp(symbol.str_val.ptr, "abcdef", 6) == 0);
    check_next_ok("${x}", YY_SYMBOL_VARIABLE, &symbol);
    assert(symbol.variable.len == 1);
    assert(strncmp(symbol.variable.ptr, "x", 1) == 0);
    check_next_ok("( 1 + 4)", YY_SYMBOL_PAREN_LEFT, &symbol);
    check_next_ok(") * 3", YY_SYMBOL_PAREN_RIGHT, &symbol);
    check_next_ok(", 25)", YY_SYMBOL_COMMA, &symbol);
    check_next_ok("&& ($x > 4)", YY_SYMBOL_AND_OP, &symbol);
    check_next_ok("|| x", YY_SYMBOL_OR_OP, &symbol);
    check_next_ok("== 25", YY_SYMBOL_EQUALS_OP, &symbol);
    check_next_ok("!= 42", YY_SYMBOL_DISTINCT_OP, &symbol);
    check_next_ok("<", YY_SYMBOL_LESS_OP, &symbol);
    check_next_ok("< 42", YY_SYMBOL_LESS_OP, &symbol);
    check_next_ok("<= 42", YY_SYMBOL_LESS_EQUALS_OP, &symbol);
    check_next_ok(">", YY_SYMBOL_GREAT_OP, &symbol);
    check_next_ok("> 42", YY_SYMBOL_GREAT_OP, &symbol);
    check_next_ok(">= 42", YY_SYMBOL_GREAT_EQUALS_OP, &symbol);
    check_next_ok("not(false)", YY_SYMBOL_NOT, &symbol);
    check_next_ok("+ 42", YY_SYMBOL_ADDITION_OP, &symbol);
    check_next_ok("- 42", YY_SYMBOL_SUBTRACTION_OP, &symbol);
    check_next_ok("* 3", YY_SYMBOL_PRODUCT_OP, &symbol);
    check_next_ok("/ 10", YY_SYMBOL_DIVIDE_OP, &symbol);
    check_next_ok("% 2", YY_SYMBOL_MODULO_OP, &symbol);
    check_next_ok("^ 3", YY_SYMBOL_POWER_OP, &symbol);
    check_next_ok("abs(${x})", YY_SYMBOL_ABS, &symbol);
    check_next_ok("min(3, 6)", YY_SYMBOL_MIN, &symbol);
    check_next_ok("max(3,6)", YY_SYMBOL_MAX, &symbol);
    check_next_ok("mod(10, 2)", YY_SYMBOL_MODULO, &symbol);
    check_next_ok("datepart(${d}, \"day\")", YY_SYMBOL_DATEPART, &symbol);
    check_next_ok("length(${str})", YY_SYMBOL_LENGTH, &symbol);
    check_next_ok("datetrunc(\"2024-08-24T08:55:06.123Z\", \"day\")", YY_SYMBOL_DATETRUNC, &symbol);
    check_next_ok("sqrt(2)", YY_SYMBOL_SQRT, &symbol);
    check_next_ok("sin(PI)", YY_SYMBOL_SIN, &symbol);
    check_next_ok("cos(PI)", YY_SYMBOL_COS, &symbol);
    check_next_ok("tan(PI)", YY_SYMBOL_TAN, &symbol);
    check_next_ok("exp(1)", YY_SYMBOL_EXP, &symbol);
    check_next_ok("log(1)", YY_SYMBOL_LOG, &symbol);
    check_next_ok("trunc(PI)", YY_SYMBOL_TRUNC, &symbol);
    check_next_ok("ceil(PI)", YY_SYMBOL_CEIL, &symbol);
    check_next_ok("floor(PI)", YY_SYMBOL_FLOOR, &symbol);
    check_next_ok("now()", YY_SYMBOL_NOW, &symbol);
    check_next_ok("dateadd(now(), \"day\", 25)", YY_SYMBOL_DATEADD, &symbol);
    check_next_ok("lower(\"AbCdEf\")", YY_SYMBOL_LOWER, &symbol);
    check_next_ok("upper(\"AbCdEf\")", YY_SYMBOL_UPPER, &symbol);
    check_next_ok("trim(\"  abc  \")", YY_SYMBOL_TRIM, &symbol);
    check_next_ok("substr(\"abcdef\", 1, 3)", YY_SYMBOL_SUBSTR, &symbol);
    check_next_ok("replace(\"abcdef\", \"a\", \"b\")", YY_SYMBOL_REPLACE, &symbol);
    check_next_ok("find(\"abcdef\", \"a\", 3)", YY_SYMBOL_FIND, &symbol);
    check_next_ok("clamp(1, 7, 15)", YY_SYMBOL_CLAMP, &symbol);
    check_next_ok("iserror(${x})", YY_SYMBOL_ISERROR, &symbol);
    check_next_ok("ifelse(1==1, 1, 2)", YY_SYMBOL_IFELSE, &symbol);
    check_next_ok("pow(2, 3+1)", YY_SYMBOL_POWER, &symbol);
    check_next_ok("not(${b})", YY_SYMBOL_NOT, &symbol);
    check_next_ok("str(now())", YY_SYMBOL_STR, &symbol);
    check_next_ok("variable(str(now()))", YY_SYMBOL_VARIABLE_FUNC, &symbol);
    check_next_ok("random(1,5)", YY_SYMBOL_RANDOM, &symbol);
    check_next_ok("< 5", YY_SYMBOL_LESS_OP, &symbol);
    check_next_ok("> 42", YY_SYMBOL_GREAT_OP, &symbol);
}

void test_read_symbol_ko(void)
{
    check_next_ko("unknow_keyword");
    check_next_ko("mmm");
    check_next_ko("@ unrecognized first letter");
    check_next_ko("!");
    check_next_ko("!a");
    check_next_ko("=");
    check_next_ko("=a");
    check_next_ko("=+");
    check_next_ko("&");
    check_next_ko("&a");
    check_next_ko("& a");
    check_next_ko("|");
    check_next_ko("|a");
    check_next_ko("| a");
    check_next_ko("\"string lacks ending double-quote");
    check_next_ko("${}");
    check_next_ko(".25");
    check_next_ko("25..0");
    check_next_ko("2e06");
}

void test_skip_spaces(void)
{
    check_skip_spaces("", -1, 0); // emptry string
    check_skip_spaces("   ", 0, 0); // empty string

    check_skip_spaces("aaa", -1, 0); // no spaces to skip

    size_t len = strlen("  \n \r \t \f \v  ");
    check_skip_spaces("  \n \r \t \f \v  ", -1, len);
    check_skip_spaces("  \n \r \t \f \v  a", -1, len);
    check_skip_spaces("    ", 2, 2);
}

void test_datepart(void)
{
    TEST_CHECK(get_datepart(&((yy_str_t){"year", 4})) == 0);
    TEST_CHECK(get_datepart(&((yy_str_t){"month", 5})) == 1);
    TEST_CHECK(get_datepart(&((yy_str_t){"day", 3})) == 2);
    TEST_CHECK(get_datepart(&((yy_str_t){"hour", 4})) == 3);
    TEST_CHECK(get_datepart(&((yy_str_t){"minute", 6})) == 4);
    TEST_CHECK(get_datepart(&((yy_str_t){"second", 6})) == 5);
    TEST_CHECK(get_datepart(&((yy_str_t){"millis", 6})) == 6);

    TEST_CHECK(get_datepart(&((yy_str_t){"", 0})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"xxx", 3})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"years", 5})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"months", 6})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"days", 4})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"hours", 5})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"minutes", 7})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"seconds", 7})) == -1);
    TEST_CHECK(get_datepart(&((yy_str_t){"ms", 2})) == -1);
}

void test_eval_number_ok(void)
{
    check_eval_number_ok("1+2", 3);
    check_eval_number_ok("1+2-3", 0);
    check_eval_number_ok("1*2/3", 2.0/3.0);
    check_eval_number_ok("1+2*3", 7);
    check_eval_number_ok("1*2+3", 5);
    check_eval_number_ok("-3+1", -2);
    check_eval_number_ok("+3", 3);
    check_eval_number_ok("1+(2*3)-3", 4);
    check_eval_number_ok("1+(2*3)/4-3", -0.5);
    check_eval_number_ok("-(1+(2*3)/4)-3", -5.5);
    check_eval_number_ok("(min(1,2)-max(3,4))*3", -9);
    check_eval_number_ok("-4%3 + 2^5 - (pow(2,3))", 23);
    check_eval_number_ok("${a} + 1", 1);
    check_eval_number_ok("((((-1))))", -1);
    check_eval_number_ok("abs(-PI)", M_PI);
    check_eval_number_ok("2 * (-1)", -2);
    check_eval_number_ok("min(2+3*4, 1+3*5)", 14);
    check_eval_number_ok("sqrt(exp(((0 * (-4332.4091)) / (10865972.2922 - 275715300.8411))))", 1);
    check_eval_number_ok("log((-2729166) / (-0.0205) * exp(0))", log((-2729166) / (-0.0205) * exp(0)));
    check_eval_number_ok("1 + length(\"abc\")", 4);
    check_eval_number_ok("find(\"cd\", \"abcdefg\", 0)", 2);
    check_eval_number_ok("clamp(1, 5, 7)", 5);
    check_eval_number_ok("ifelse(1 < 4 && false, 5, 7)", 7);
    check_eval_number_ok("1 + cos(variable(\"a\"))", 2);
    check_eval_number_ok("datepart(\"2024-09-10\", \"day\")", 10);
    check_eval_number_ok("${c}^3", 8);
}

void test_eval_number_ko(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t result = {0};
    char buf[256] = {0};

    check_eval_number_ko(" ", YY_ERROR_SYNTAX);
    check_eval_number_ko("not_a_var", YY_ERROR_SYNTAX);
    check_eval_number_ko("+", YY_ERROR_SYNTAX);
    check_eval_number_ko("()", YY_ERROR_SYNTAX);
    check_eval_number_ko("(((((())))))", YY_ERROR_SYNTAX);
    check_eval_number_ko("((1)", YY_ERROR_SYNTAX);
    check_eval_number_ko("1+(", YY_ERROR_SYNTAX);
    check_eval_number_ko("1+()", YY_ERROR_SYNTAX);
    check_eval_number_ko("1*/3", YY_ERROR_SYNTAX);
    check_eval_number_ko("2^^3", YY_ERROR_SYNTAX);
    check_eval_number_ko("coa(pi)", YY_ERROR_SYNTAX);
    check_eval_number_ko("cosh(pi)", YY_ERROR_SYNTAX);
    check_eval_number_ko("min", YY_ERROR_SYNTAX);
    check_eval_number_ko("min(", YY_ERROR_SYNTAX);
    check_eval_number_ko("min(,", YY_ERROR_SYNTAX);
    check_eval_number_ko("min(1,", YY_ERROR_SYNTAX);
    check_eval_number_ko("min(1,)", YY_ERROR_SYNTAX);
    check_eval_number_ko("min(1,2", YY_ERROR_SYNTAX);
    check_eval_number_ko("+-1", YY_ERROR_SYNTAX);
    check_eval_number_ko("++1", YY_ERROR_SYNTAX);
    check_eval_number_ko("1++1", YY_ERROR_SYNTAX);
    check_eval_number_ko("1+-1", YY_ERROR_SYNTAX);
    check_eval_number_ko("2 * -1", YY_ERROR_SYNTAX);

    // invalid string to parse
    result = yy_eval_number(NULL, NULL, &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid stack
    result = yy_eval_number(buf, buf + strlen(buf), NULL, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // not enough memory in the stack
    stack.reserved = 0;
    sprintf(buf, "PI*(1-7)/5");
    result = yy_eval_number(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_MEM);
}

void test_eval_datetime_ok(void)
{
    check_eval_datetime_ok("\"2024-08-30T06:16:34.123Z\"", "2024-08-30T06:16:34.123Z");
    check_eval_datetime_ok("datetrunc(\"2024-08-30T06:16:34.123Z\", \"day\")", "2024-08-30T00:00:00.000Z");
    check_eval_datetime_ok("dateadd(\"2024-08-30T06:16:34.123Z\", 3, \"month\")", "2024-11-30T06:16:34.123Z");
    check_eval_datetime_ok("dateset(\"2024-08-30T06:16:34.123Z\", 14, \"hour\")", "2024-08-30T14:16:34.123Z");
    check_eval_datetime_ok("min(\"2023-08-30T06:16:34.123Z\", now())", "2023-08-30T06:16:34.123Z");
    check_eval_datetime_ok("max(\"2053-08-30T06:16:34.123Z\", now())", "2053-08-30T06:16:34.123Z");
    check_eval_datetime_ok("clamp(now(), \"2023-08-30\", \"2023-11-06\")", "2023-11-06T00:00:00.000Z");
    check_eval_datetime_ok("datetrunc($d, \"day\")", "2024-09-08T00:00:00.000Z");
    check_eval_datetime_ok("datetrunc(variable(\"d\"), \"day\")", "2024-09-08T00:00:00.000Z");
    check_eval_datetime_ok("ifelse(true, \"2023-01-01\", \"2024-01-01\")", "2023-01-01T00:00:00.000Z");
}

void test_eval_datetime_ko(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t result = {0};
    char buf[256] = {0};

    // invalid string to parse
    result = yy_eval_datetime(NULL, NULL, &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid stack
    result = yy_eval_datetime(buf, buf + strlen(buf), NULL, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // not surrounded by double-quotes (valid number expr, but not a valid datetime expr)
    sprintf(buf, "2024-11-03");
    result = yy_eval_datetime(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // invalid datepart
    sprintf(buf, "datetrunc(now(), \"TODAY\")");
    result = yy_eval_datetime(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // invalid function call (lacks ending parenthesis)
    sprintf(buf, "now(");
    result = yy_eval_datetime(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // not enough memory in the stack
    stack.reserved = 0;
    sprintf(buf, "now()");
    result = yy_eval_datetime(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_MEM);
}

void test_eval_string_ok(void)
{
    check_eval_string_ok("\"Hi bob!\"", "Hi bob!");
    check_eval_string_ok("$s + \"!\"", "lorem ipsum!");
    check_eval_string_ok("\"first part \" + \"plus second part\"", "first part plus second part");
    check_eval_string_ok("upper(\"Hi bob!\")", "HI BOB!");
    check_eval_string_ok("lower(\"Hi bob!\")", "hi bob!");
    check_eval_string_ok("\"hi \" + upper(\"bob\")", "hi BOB");
    check_eval_string_ok("lower(\"Hi \") + upper(\"bob\")", "hi BOB");
    check_eval_string_ok("( lower(\"Hi \") + upper(\"bob\") ) + \"!\"", "hi BOB!");
    check_eval_string_ok("trim(\"  <- leading spaces and trailing spaces->  \")", "<- leading spaces and trailing spaces->");
    check_eval_string_ok("substr(\"0123456789\", 3, 4)", "3456");
    check_eval_string_ok("substr(\"0123456789\", 3, 10)", "3456789");
    check_eval_string_ok("substr(\"0123456789\", -10, 30)", "0123456789");
    check_eval_string_ok("min(\"abc\", \"xyz\")", "abc");
    check_eval_string_ok("max(\"abc\", \"xyz\")", "xyz");
    check_eval_string_ok("min(\"abc\", \"xyz\") + \"...\" + max(\"abc\", \"xyz\")", "abc...xyz");
    check_eval_string_ok("trim(upper(\"  abc   \"))", "ABC");
    check_eval_string_ok("trim(substr(\"  abc   \", 3, 5))", "bc");
    check_eval_string_ok("\"\\\\escaped string\\\\\"", "\\escaped string\\");
    check_eval_string_ok("replace(\"Hi Bob!\", \"Bob\", \"John\")", "Hi John!");
    check_eval_string_ok("trim(replace(\" Hi BOB \", upper(\"Bob\"), lower(\"John\"))) + \"!\"", "Hi john!");
    check_eval_string_ok("str(PI + 10)", "13.1416");
    check_eval_string_ok("str(datetrunc(\"2024-09-08T09:24:51.742Z\", \"second\"))", "2024-09-08T09:24:51.000Z");
    check_eval_string_ok("str(\"Hi Bob\" + \"!\")", "Hi Bob!");
    check_eval_string_ok("str(1 < 3)", "true");
    check_eval_string_ok("variable(\"s\")", "lorem ipsum");
    check_eval_string_ok("ifelse(1 == 2, \"true\", \"false\")", "false");
}

void test_eval_string_ko(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t result = {0};
    char buf[256] = {0};

    // invalid string to parse
    result = yy_eval_string(NULL, NULL, &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid stack
    result = yy_eval_string(buf, buf + strlen(buf), NULL, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid string (lacks ending parenthesis)
    sprintf(buf, "\"Hi Bob!");
    result = yy_eval_string(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // Unrecognized operator
    sprintf(buf, "\"Hi \" - \"Bob!\"");
    result = yy_eval_string(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // Non-string function calls
    sprintf(buf, "trunc(\"Hi Bob!\", 3)");
    result = yy_eval_string(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // not enough memory in the stack
    stack.reserved = 2;
    sprintf(buf, "upper(\"Hi Bob!\")");
    result = yy_eval_string(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_MEM);
}

void test_eval_bool_ok(void)
{
    check_eval_bool_ok("true", true);
    check_eval_bool_ok("true || false", true);
    check_eval_bool_ok("true && false", false);
    check_eval_bool_ok("not(true)", false);
    check_eval_bool_ok("not(1 > 2)", true);
    check_eval_bool_ok("1 < 2 || not(1 < 2)", true);
    check_eval_bool_ok("1 < 2 || not(1 < 2) && 1 != 1", true);
    check_eval_bool_ok("1 < 2 && 1 > 2", false);
    check_eval_bool_ok("length(\"xxx\") < 5 || isinf(cos(PI))", true);
    check_eval_bool_ok("length(\"xxx\") > 5 == false", true);
    check_eval_bool_ok("exp(1) != E && length(\"xxx\") > 0", false);
    check_eval_bool_ok("variable(\"m\")", true);
    check_eval_bool_ok("iserror($a) || isnan(3)", false);
    check_eval_bool_ok("(iserror($a) && not($m)) || false", false);
}

void test_eval_bool_ko(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t result = {0};
    char buf[256] = {0};

    // invalid string to parse
    result = yy_eval_bool(NULL, NULL, &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid stack
    result = yy_eval_bool(buf, buf + strlen(buf), NULL, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // unrecognized boolean value
    sprintf(buf, "TrUe");
    result = yy_eval_bool(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // Unrecognized operator
    sprintf(buf, "true & false");
    result = yy_eval_bool(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // not enough memory in the stack
    stack.reserved = 2;
    sprintf(buf, "true && false");
    result = yy_eval_bool(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_MEM);
}

void test_eval_ok(void)
{
    check_eval_ok("(true || false)", YY_TOKEN_BOOL);
    check_eval_ok("(1 + PI)", YY_TOKEN_NUMBER);
    check_eval_ok("now()", YY_TOKEN_DATETIME);
    check_eval_ok("substr(\"abcdef\", 2, 3)", YY_TOKEN_STRING);
    check_eval_ok("ifelse(\"abc\" == $s, 1, 2)", YY_TOKEN_NUMBER);
    check_eval_ok("ifelse(\"abc\" == $s, 1, 2) + 5 == 3", YY_TOKEN_BOOL);
    check_eval_ok("${c}^3", YY_TOKEN_NUMBER);
    check_eval_ok("ifelse(1 < 4, \"sensei\", \"opa\")", YY_TOKEN_STRING);
    check_eval_ok("ifelse(${c}^2 == 4, true, false)", YY_TOKEN_BOOL);
    check_eval_ok("$c^2 == 4", YY_TOKEN_BOOL);
}

void test_eval_ko(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};
    yy_token_t result = {0};
    char buf[256] = {0};

    // invalid string to parse
    result = yy_eval(NULL, NULL, &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // invalid stack
    result = yy_eval(buf, buf + strlen(buf), NULL, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR);

    // syntax error
    sprintf(buf, "1 + \"str\"");
    result = yy_eval(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_SYNTAX);

    // not enough memory in the stack
    stack.reserved = 1;
    sprintf(buf, "true && false");
    result = yy_eval(buf, buf + strlen(buf), &stack, resolve, NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
    TEST_CHECK(result.error == YY_ERROR_MEM);
}

void test_sizeof(void)
{
    TEST_CHECK(sizeof(uint64_t) == 8);
    TEST_CHECK(sizeof(yy_str_t) == 12);
    TEST_CHECK(sizeof(yy_func_t) == 11);
    TEST_CHECK(sizeof(yy_token_e) <= 4);
    TEST_CHECK(sizeof(yy_token_t) == 16);
    TEST_CHECK(sizeof(yy_symbol_t) == 32);
}

void test_func_length(void)
{
    yy_token_t result = {0};

    result = func_length(token_string("xxx", 3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_length(token_string("", 0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 0);

    result = func_length(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_find(void)
{
    yy_token_t result = {0};

    result = func_find(token_string("xxx", 3), token_string("abc_xxx_yz", 10), token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 4);

    result = func_find(token_string("xxx", 3), token_string("", 0), token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_find(token_string("", 0), token_string("abc", 3), token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 0);

    result = func_find(token_bool(true), token_string("abc_xxx_yz", 10), token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_find(token_string("xxx", 3), token_bool(true), token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_find(token_string("xxx", 3), token_string("abc_xxx_yz", 10), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_datepart(void)
{
    const char *date_str = "2024-08-26T14:16:53.493Z";
    yy_token_t date = {0};
    yy_token_t result = {0};

    date = yy_parse_datetime(date_str, date_str + strlen(date_str));
    TEST_CHECK(date.type == YY_TOKEN_DATETIME);

    result = func_datepart(date, token_number(0)); // year
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2024);

    result = func_datepart(date, token_number(1)); // month
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 8);

    result = func_datepart(date, token_number(2)); // day
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 26);

    result = func_datepart(date, token_number(3)); // hour
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 14);

    result = func_datepart(date, token_number(4)); // minute
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 16);

    result = func_datepart(date, token_number(5)); // second
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 53);

    result = func_datepart(date, token_number(6)); // millis
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 493);

    result = func_datepart(date, token_number(99)); // unknow part
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_datepart(date, token_bool(false)); // unexpected type
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_datepart(token_number(1), token_number(1)); // invalid date
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_abs(void)
{
    yy_token_t result = {0};

    result = func_abs(token_number(10));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 10);

    result = func_abs(token_number(-10));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 10);

    result = func_abs(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_ceil(void)
{
    yy_token_t result = {0};

    result = func_ceil(token_number(2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_ceil(token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_ceil(token_number(-2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == -2);

    result = func_ceil(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_floor(void)
{
    yy_token_t result = {0};

    result = func_floor(token_number(2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2);

    result = func_floor(token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_floor(token_number(-2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == -3);

    result = func_floor(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_trunc(void)
{
    yy_token_t result = {0};

    result = func_trunc(token_number(2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2);

    result = func_trunc(token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_trunc(token_number(-2.3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == -2);

    result = func_trunc(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_sin(void)
{
    yy_token_t result = {0};

    result = func_sin(token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(result.number_val) < EPSILON);

    result = func_sin(token_number(M_PI_2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_sin(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_cos(void)
{
    yy_token_t result = {0};

    result = func_cos(token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_cos(token_number(M_PI_2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(result.number_val) < EPSILON);

    result = func_cos(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_tan(void)
{
    yy_token_t result = {0};

    result = func_tan(token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(result.number_val) < EPSILON);

    result = func_tan(token_number(M_PI_4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_tan(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_exp(void)
{
    yy_token_t result = {0};

    result = func_exp(token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(M_E - result.number_val) < EPSILON);

    result = func_exp(token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_exp(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_log(void)
{
    yy_token_t result = {0};

    result = func_log(token_number(M_E));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_log(token_number(M_E*M_E));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(2.0 - result.number_val) < EPSILON);

    result = func_log(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_sqrt(void)
{
    yy_token_t result = {0};

    result = func_sqrt(token_number(0));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(result.number_val) < EPSILON);

    result = func_sqrt(token_number(100));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(10.0 - result.number_val) < EPSILON);

    result = func_sqrt(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_pow(void)
{
    yy_token_t result = {0};

    result = func_pow(token_number(2), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(8.0 - result.number_val) < EPSILON);

    result = func_pow(token_number(1), token_number(6));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_pow(token_number(1), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_pow(token_bool(true), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_minus(void)
{
    yy_token_t result = {0};

    result = func_minus(token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(-1.0 - result.number_val) < EPSILON);

    result = func_minus(token_number(-1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - result.number_val) < EPSILON);

    result = func_minus(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_ident(void)
{
    yy_token_t result = {0};

    result = func_ident(token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 1.0);

    result = func_ident(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_addition(void)
{
    yy_token_t result = {0};

    result = func_addition(token_number(2), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 5.0);

    result = func_addition(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_addition(token_number(2), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_subtraction(void)
{
    yy_token_t result = {0};

    result = func_subtraction(token_number(2), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == -1.0);

    result = func_subtraction(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_subtraction(token_number(2), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_mult(void)
{
    yy_token_t result = {0};

    result = func_mult(token_number(2), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 6.0);

    result = func_mult(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_mult(token_number(2), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_div(void)
{
    yy_token_t result = {0};

    result = func_div(token_number(2), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(2.0/3.0 - result.number_val) < EPSILON);

    result = func_div(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_div(token_number(2), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_mod(void)
{
    yy_token_t result = {0};

    result = func_mod(token_number(7), token_number(5));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2.0);

    result = func_mod(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_mod(token_number(2), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_min(void)
{
    yy_token_t result = {0};

    result = func_min(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 1.0);

    result = func_min(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 1.0);

    result = func_min(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 1.0);

    yy_token_t aux = make_datetime("2024-09-09");

    result = func_min(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_min(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_min(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_min(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_min(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abc", 3)) == 0);

    result = func_min(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abc", 3)) == 0);

    result = func_min(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abc", 3)) == 0);

    result = func_min(token_number(3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_min(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_max(void)
{
    yy_token_t result = {0};

    result = func_max(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2.0);

    result = func_max(token_number(2), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2.0);

    result = func_max(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2.0);

    yy_token_t aux = make_datetime("2024-09-10");

    result = func_max(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_max(make_datetime("2024-09-10"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_max(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_max(token_bool(true), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_max(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("xyz", 3)) == 0);

    result = func_max(token_string("xyz", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("xyz", 3)) == 0);

    result = func_max(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("xyz", 3)) == 0);

    result = func_max(token_number(3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_max(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_now(void)
{
    char buf[128] = {0};
    yy_token_t token1 = {0};
    yy_token_t token2 = {0};

    token1 = func_now(NULL);
    TEST_CHECK(token1.type == YY_TOKEN_DATETIME);
    datetime_to_str(token1.datetime_val, buf);

    token2 = yy_parse_datetime(buf, buf + strlen(buf));
    TEST_CHECK(token2.type == YY_TOKEN_DATETIME);
    TEST_CHECK(token1.datetime_val == token2.datetime_val);
}

void test_func_dateadd(void)
{
    yy_token_t result = {0};

    check_dateadd("2024-08-26T14:16:53.493Z",   +10, "year"  , "2034-08-26T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   -10, "year"  , "2014-08-26T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   +10, "month" , "2025-06-26T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   -10, "month" , "2023-10-26T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",  +150, "day"   , "2025-01-23T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",  -150, "day"   , "2024-03-29T14:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   +40, "hour"  , "2024-08-28T06:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   -40, "hour"  , "2024-08-24T22:16:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   +40, "minute", "2024-08-26T14:56:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   -40, "minute", "2024-08-26T13:36:53.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   +70, "second", "2024-08-26T14:18:03.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z",   -70, "second", "2024-08-26T14:15:43.493Z");
    check_dateadd("2024-08-26T14:16:53.493Z", +5500, "millis", "2024-08-26T14:16:58.993Z");
    check_dateadd("2024-08-26T14:16:53.493Z", -5500, "millis", "2024-08-26T14:16:47.993Z");
    check_dateadd("2024-08-26T14:16:53.493Z", -5500, "millis", "2024-08-26T14:16:47.993Z");
    check_dateadd("2024-08-26T14:16:53.493Z",    10, "xxx"   , NULL);

    result = func_dateadd(make_datetime("2024-09-09"), token_number(1), token_number(99));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateadd(token_error(YY_ERROR_VALUE), token_number(1), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateadd(make_datetime("2024-09-09"), token_error(YY_ERROR_VALUE), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateadd(make_datetime("2024-09-09"), token_number(1), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_dateset(void)
{
    yy_token_t result = {0};

    check_dateset("2024-08-26T14:16:53.493Z",  2027, "year"  , "2027-08-26T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",     3, "month" , "2024-03-26T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    23, "day"   , "2024-08-23T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",     5, "hour"  , "2024-08-26T05:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    40, "minute", "2024-08-26T14:40:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    12, "second", "2024-08-26T14:16:12.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",   123, "millis", "2024-08-26T14:16:53.123Z");
    check_dateset("2024-08-26T14:16:53.493Z",  1123, "millis", "2024-08-26T14:16:54.123Z");
    check_dateset("2024-08-26T14:16:53.493Z",    10, "xxx"   , NULL);

    // negative value
    result = func_dateset(make_datetime("2024-09-09"), token_number(-1), token_number(99));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    // invalid datepart
    result = func_dateset(make_datetime("2024-09-09"), token_number(1), token_number(99));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateset(token_error(YY_ERROR_VALUE), token_number(1), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateset(make_datetime("2024-09-09"), token_error(YY_ERROR_VALUE), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_dateset(make_datetime("2024-09-09"), token_number(1), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_datetrunc(void)
{
    check_datetrunc("2024-08-26T14:16:53.493Z", "year"  , "2024-01-01T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "month" , "2024-08-01T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "day"   , "2024-08-26T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "hour"  , "2024-08-26T14:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "minute", "2024-08-26T14:16:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "second", "2024-08-26T14:16:53.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "millis", "2024-08-26T14:16:53.493Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "xxx"   , NULL);
}

void test_func_isinf(void)
{
    yy_token_t result = {0};

    result = func_isinf(token_number(3.14));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_isinf(token_number(0.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_isinf(token_number(+1.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_isinf(token_number(-1.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_isinf(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_isnan(void)
{
    yy_token_t result = {0};

    result = func_isnan(token_number(+0.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_isnan(token_number(-0.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_isnan(token_number(1.0/0.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_isnan(token_number(1.0));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_isnan(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_str(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};
    yy_token_t aux = {0};
    const char date_str[] = "2024-09-08T00:00:00.000Z";

    result = func_str(token_number(1), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("1", 1)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_number(M_PI), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("3.14159", 7)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_number(NAN), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("NaN", 3)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_number(-INFINITY), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("-Inf", 4)) == 0);

    ctx.tmp_str = end_stack;
    aux = yy_parse_datetime(date_str, date_str + 24);
    TEST_CHECK(aux.type == YY_TOKEN_DATETIME);
    result = func_str(aux, &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(date_str, 24)) == 0);

    ctx.tmp_str = end_stack;
    aux = yy_parse_string(date_str, date_str + 24);
    TEST_CHECK(aux.type == YY_TOKEN_STRING);
    result = func_str(aux, &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(date_str, 24)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_bool(true), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("true", 4)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_bool(false), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("false", 5)) == 0);

    ctx.tmp_str = end_stack;
    result = func_str(token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_unescape(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};
    const char str_escaped[] = "a\\tb\\nc\\xd\\\\e\\\"f";
    const char str_unescaped[] = "a\tb\nc\\xd\\e\"f";

    ctx.tmp_str = end_stack;
    result = func_unescape(token_string(str_escaped, strlen(str_escaped)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(str_unescaped, strlen(str_unescaped))) == 0);

    // leading and trailing escaped chars
    ctx.tmp_str = end_stack;
    result = func_unescape(token_string(str_escaped + 1, strlen(str_escaped) - 2), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(str_unescaped + 1, strlen(str_unescaped) - 2)) == 0);

    // empty string
    ctx.tmp_str = end_stack;
    result = func_unescape(token_string("", 0), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("", 0)) == 0);

    ctx.tmp_str = end_stack;
    result = func_unescape(token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_upper(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};
    const char str[] = ",aBc...\\t...xYz_";
    const char expected[] = ",ABC...\\T...XYZ_";

    ctx.tmp_str = end_stack;
    result = func_upper(token_string(str, strlen(str)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_upper(token_string(expected, strlen(expected)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_upper(token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_lower(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};
    const char str[] = ",AbC...\\T...XyZ_";
    const char expected[] = ",abc...\\t...xyz_";

    ctx.tmp_str = end_stack;
    result = func_lower(token_string(str, strlen(str)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_lower(token_string(expected, strlen(expected)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_lower(token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_trim(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};
    const char str[] = " \n\r\t\f\v  abc  \n\r\t\f\v ";
    const char expected[] = "abc";

    ctx.tmp_str = end_stack;
    result = func_trim(token_string(str, strlen(str)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_trim(token_string(expected, strlen(expected)), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string(expected, strlen(expected))) == 0);

    ctx.tmp_str = end_stack;
    result = func_trim(token_string("", 0), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("", 0)) == 0);

    ctx.tmp_str = end_stack;
    result = func_trim(token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_concat(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};

    ctx.tmp_str = end_stack;
    result = func_concat(token_string("abc", 3), token_string("def", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abcdef", 6)) == 0);

    ctx.tmp_str = end_stack;
    result = func_concat(token_string("abc", 3), token_string("", 0), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abc", 3)) == 0);

    ctx.tmp_str = end_stack;
    result = func_concat(token_string("", 0), token_string("def", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("def", 3)) == 0);

    ctx.tmp_str = end_stack;
    result = func_concat(token_error(YY_ERROR_VALUE), token_string("def", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    ctx.tmp_str = end_stack;
    result = func_concat(token_string("abc", 3), token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_substr(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("abcdef", 6), token_number(3), token_number(2), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("de", 2)) == 0);

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("abcdef", 6), token_number(-1), token_number(20), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abcdef", 6)) == 0);

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("abcdef", 6), token_number(10), token_number(3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("", 0)) == 0);

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("", 0), token_number(1), token_number(3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("", 0)) == 0);

    ctx.tmp_str = end_stack;
    result = func_substr(token_error(YY_ERROR_VALUE), token_number(1), token_number(3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("abcdef", 6), token_error(YY_ERROR_VALUE), token_number(3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    ctx.tmp_str = end_stack;
    result = func_substr(token_string("abcdef", 6), token_number(1), token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_replace(void)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = sizeof(data)/sizeof(data[0]), .len = 0};
    char *end_stack = (char *) &stack.data[stack.reserved];
    yy_eval_ctx_t ctx = {.stack = &stack, .tmp_str = end_stack};
    yy_token_t result = {0};

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("cd", 2), token_string("XXXXXX", 5), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abXXXXXef", 9)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("ab", 2), token_string("X", 1), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("Xcdef", 5)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("ef", 2), token_string("X", 1), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abcdX", 5)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("xxx", 3), token_string("yyy", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abcdef", 6)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("", 0), token_string("yyy", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abcdef", 6)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("cd", 2), token_string("", 0), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abef", 4)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("", 0), token_string("cd", 2), token_string("XXXXXX", 5), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("", 0)) == 0);

    ctx.tmp_str = end_stack;
    result = func_replace(token_error(YY_ERROR_VALUE), token_string("xxx", 3), token_string("yyy", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_error(YY_ERROR_VALUE), token_string("yyy", 3), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    ctx.tmp_str = end_stack;
    result = func_replace(token_string("abcdef", 6), token_string("xxx", 3), token_error(YY_ERROR_VALUE), &ctx);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_and(void)
{
    yy_token_t result = {0};

    result = func_and(token_bool(true), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_and(token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_and(token_bool(false), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_and(token_bool(false), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_and(token_error(YY_ERROR_VALUE), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_and(token_bool(false), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_or(void)
{
    yy_token_t result = {0};

    result = func_or(token_bool(true), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_or(token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_or(token_bool(false), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_or(token_bool(false), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_or(token_error(YY_ERROR_VALUE), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_or(token_bool(false), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_not(void)
{
    yy_token_t result = {0};

    result = func_not(token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_not(token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_not(token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_iserror(void)
{
    const yy_token_e types[] = {YY_TOKEN_NULL, YY_TOKEN_BOOL, YY_TOKEN_NUMBER, YY_TOKEN_DATETIME, 
        YY_TOKEN_STRING, YY_TOKEN_VARIABLE, YY_TOKEN_FUNCTION};
    const yy_error_e errors[] = {YY_OK, YY_ERROR, YY_ERROR_REF, YY_ERROR_CREF, YY_ERROR_MEM, 
        YY_ERROR_EVAL, YY_ERROR_VALUE, YY_ERROR_SYNTAX};
    yy_token_t token = {0};
    yy_token_t result = {0};

    for (size_t i = 0; i < sizeof(errors)/sizeof(errors[0]); i++)
    {
        result = func_iserror(token_error(errors[i]));
        TEST_CHECK(result.type == YY_TOKEN_BOOL);
        TEST_CHECK(result.bool_val == true);
    }

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++)
    {
        token.type = types[i];
        result = func_iserror(token);
        TEST_CHECK(result.type == YY_TOKEN_BOOL);
        TEST_CHECK(result.bool_val == false);
    }
}

void test_func_variable(void)
{
    yy_token_t result = {0};

    result = func_variable(token_string("x", 1));
    TEST_CHECK(result.type == YY_TOKEN_VARIABLE);
    TEST_CHECK(str_cmp(result.variable, make_string("x", 1)) == 0);

    result = func_variable(token_variable("x", 1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_lt(void)
{
    yy_token_t result = {0};

    result = func_lt(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_lt(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_lt(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_lt(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_lt(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_lt(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_le(void)
{
    yy_token_t result = {0};

    result = func_le(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_le(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_le(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_le(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_le(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_le(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_gt(void)
{
    yy_token_t result = {0};

    result = func_gt(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_gt(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_gt(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_gt(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_gt(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_gt(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_ge(void)
{
    yy_token_t result = {0};

    result = func_ge(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ge(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(token_number(2), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ge(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ge(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(token_string("xyz", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ge(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_ge(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_eq(void)
{
    yy_token_t result = {0};

    result = func_eq(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_eq(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_eq(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_eq(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_eq(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_eq(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_eq(token_bool(true), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_eq(token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_eq(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_eq(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_ne(void)
{
    yy_token_t result = {0};

    result = func_ne(token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ne(token_number(1), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ne(make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ne(make_datetime("2024-09-09"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ne(token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ne(token_string("abc", 3), token_string("abc", 3));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ne(token_bool(true), token_bool(true));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    result = func_ne(token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ne(token_error(YY_ERROR_VALUE), token_number(1));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_ne(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_clamp(void)
{
    yy_token_t result = {0};

    result = func_clamp(token_number(1), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2);

    result = func_clamp(token_number(2), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2);

    result = func_clamp(token_number(3), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 3);

    result = func_clamp(token_number(4), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 4);

    result = func_clamp(token_number(5), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 4);

    result = func_clamp(make_datetime("2024-09-09"), make_datetime("2024-09-10"), make_datetime("2024-09-12"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == make_datetime("2024-09-10").datetime_val);

    result = func_clamp(make_datetime("2024-09-10"), make_datetime("2024-09-10"), make_datetime("2024-09-12"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == make_datetime("2024-09-10").datetime_val);

    result = func_clamp(make_datetime("2024-09-11"), make_datetime("2024-09-10"), make_datetime("2024-09-12"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == make_datetime("2024-09-11").datetime_val);

    result = func_clamp(make_datetime("2024-09-12"), make_datetime("2024-09-10"), make_datetime("2024-09-12"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == make_datetime("2024-09-12").datetime_val);

    result = func_clamp(make_datetime("2024-09-13"), make_datetime("2024-09-10"), make_datetime("2024-09-12"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == make_datetime("2024-09-12").datetime_val);

    result = func_clamp(token_error(YY_ERROR_VALUE), token_number(2), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_clamp(token_number(1), token_error(YY_ERROR_VALUE), token_number(4));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_clamp(token_number(1), token_number(2), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_clamp(token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE), token_error(YY_ERROR_VALUE));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_ifelse(void)
{
    yy_token_t result = {0};

    // number case
    result = func_ifelse(token_bool(true), token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 1);

    result = func_ifelse(token_bool(false), token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_NUMBER);
    TEST_CHECK(result.number_val == 2);

    // datetime case
    yy_token_t aux = make_datetime("2024-09-09");

    result = func_ifelse(token_bool(true), make_datetime("2024-09-09"), make_datetime("2024-09-10"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    result = func_ifelse(token_bool(false), make_datetime("2024-09-10"), make_datetime("2024-09-09"));
    TEST_CHECK(result.type == YY_TOKEN_DATETIME);
    TEST_CHECK(result.datetime_val == aux.datetime_val);

    // string case
    result = func_ifelse(token_bool(true), token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("abc", 3)) == 0);

    result = func_ifelse(token_bool(false), token_string("abc", 3), token_string("xyz", 3));
    TEST_CHECK(result.type == YY_TOKEN_STRING);
    TEST_CHECK(str_cmp(result.str_val, make_string("xyz", 3)) == 0);

    // bool case
    result = func_ifelse(token_bool(true), token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == true);

    result = func_ifelse(token_bool(false), token_bool(true), token_bool(false));
    TEST_CHECK(result.type == YY_TOKEN_BOOL);
    TEST_CHECK(result.bool_val == false);

    // error case (condition non boolean)
    result = func_ifelse(token_number(0), token_number(1), token_number(2));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    // error case (distinct return types)
    result = func_ifelse(token_bool(true), token_string("abc", 3), token_number(3));
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_func_random(void)
{
    yy_token_t result = {0};

    srand(1234567);

    for (int i = 0; i < 100; i++)
    {
        result = func_random(token_number(10), token_number(20), NULL);
        TEST_CHECK(result.type == YY_TOKEN_NUMBER);
        TEST_CHECK(10 <= result.number_val && result.number_val < 20);
    }

    result = func_random(token_number(2), token_number(1), NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_random(token_bool(true), token_number(2), NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);

    result = func_random(token_number(1), token_bool(true), NULL);
    TEST_CHECK(result.type == YY_TOKEN_ERROR);
}

void test_funcs(void)
{
    test_func_now();
    test_func_datepart();
    test_func_datetrunc();
    test_func_dateset();
    test_func_dateadd();
    test_func_datepart();
    test_func_isnan();
    test_func_isinf();
    test_func_div();
    test_func_mod();
    test_func_mult();
    test_func_subtraction();
    test_func_addition();
    test_func_ident();
    test_func_minus();
    test_func_pow();
    test_func_sqrt();
    test_func_log();
    test_func_exp();
    test_func_sin();
    test_func_cos();
    test_func_tan();
    test_func_trunc();
    test_func_floor();
    test_func_ceil();
    test_func_random();
    test_func_abs();
    test_func_str();
    test_func_length();
    test_func_find();
    test_func_unescape();
    test_func_upper();
    test_func_lower();
    test_func_trim();
    test_func_concat();
    test_func_substr();
    test_func_replace();
    test_func_min();
    test_func_max();
    test_func_clamp();
    test_func_ifelse();
    test_func_lt();
    test_func_le();
    test_func_gt();
    test_func_ge();
    test_func_eq();
    test_func_ne();
    test_func_iserror();
    test_func_not();
    test_func_and();
    test_func_or();
    test_func_variable();
}

TEST_LIST = {
    { "sizeof",                       test_sizeof },
    { "yy_parse_number_ok",           test_parse_number_ok },
    { "yy_parse_number_ko",           test_parse_number_ko },
    { "yy_parse_datetime_ok",         test_parse_datetime_ok },
    { "yy_parse_datetime_ko",         test_parse_datetime_ko },
    { "yy_parse_boolean_ok",          test_parse_boolean_ok },
    { "yy_parse_boolean_ko",          test_parse_boolean_ko },
    { "yy_parse_string_ok",           test_parse_string_ok },
    { "yy_parse_string_ko",           test_parse_string_ko },
    { "yy_parse_ok",                  test_parse_ok },
    { "yy_parse_ko",                  test_parse_ko },
    { "read_symbol_string_ok",        test_read_symbol_string_ok },
    { "read_symbol_string_ko",        test_read_symbol_string_ko },
    { "read_symbol_variable_ok",      test_read_symbol_variable_ok },
    { "read_symbol_variable_ko",      test_read_symbol_variable_ko },
    { "read_symbol_ok",               test_read_symbol_ok },
    { "read_symbol_ko",               test_read_symbol_ko },
    { "skip_spaces",                  test_skip_spaces },
    { "datepart",                     test_datepart },
    { "yy_eval_number_ok",            test_eval_number_ok },
    { "yy_eval_number_ko",            test_eval_number_ko },
    { "yy_eval_datetime_ok",          test_eval_datetime_ok },
    { "yy_eval_datetime_ko",          test_eval_datetime_ko },
    { "yy_eval_string_ok",            test_eval_string_ok },
    { "yy_eval_string_ko",            test_eval_string_ko },
    { "yy_eval_bool_ok",              test_eval_bool_ok },
    { "yy_eval_bool_ko",              test_eval_bool_ko },
    { "yy_eval_ok",                   test_eval_ok },
    { "yy_eval_ko",                   test_eval_ko },
    { "yy_funcs",                     test_funcs },
    { NULL, NULL }
};
