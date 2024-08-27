#include "acutest.h"
#include "expr.c"

#define EPSILON 1e-14

char * datetime_to_str(uint64_t millis_utc, char *ret)
{
    time_t time = (time_t)(millis_utc / 1000UL);
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    sprintf(ret, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
        stm.tm_year + 1900, stm.tm_mon + 1,
        stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec,
        (long)(millis_utc % 1000UL));

    return ret;
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
        case YY_SYMBOL_NOT_OP: return "NOT_OP";
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
        case YY_SYMBOL_NOW: return "NOW";
        case YY_SYMBOL_DATEPART: return "DATEPART";
        case YY_SYMBOL_DATEADD: return "DATEADD";
        case YY_SYMBOL_DATETRUNC: return "DATETRUNC";
        case YY_SYMBOL_LENGTH: return "LENGTH";
        case YY_SYMBOL_LOWER: return "LOWER";
        case YY_SYMBOL_UPPER: return "UPPER";
        case YY_SYMBOL_TRIM: return "TRIM";
        case YY_SYMBOL_CONCAT: return "CONCAT";
        case YY_SYMBOL_SUBSTR: return "SUBSTR";
        case YY_SYMBOL_END: return "END";
        default: return "UNKNOW";
    }
}

const char * get_function_name(void (*func)(void))
{
    return symbol_to_str(get_function_symbol(func));
}

void print_stack(const yy_stack_t *stack)
{
    for (size_t i = 0; i < stack->len; i++)
    {
        const yy_token_t token = stack->data[i];

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
                printf("%lu", token.datetime_val);
                break;
            case YY_TOKEN_STRING:
                printf("%.*s", token.str_val.len, token.str_val.ptr);
                break;
            case YY_TOKEN_VARIABLE:
                printf("%.*s", token.variable.len, token.variable.ptr);
                break;
            case YY_TOKEN_FUNCTION:
                printf("%s", get_function_name(token.function.ptr));
                break;
            case YY_TOKEN_ERROR:
                printf("ERROR");
                break;
            default:
                assert(false);
        }

        printf(" ");
    }
}

void check_parse_number_ok(const char *str, double expected_val)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_number(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_MSG("Case='%s', error=failed", str);
    TEST_CHECK(fabs(token.number_val - expected_val) < EPSILON);
    TEST_MSG("Case='%s', expected=%lf, result=%lf", str, expected_val, token.number_val);
}

void check_parse_number_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_number(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void check_parse_datetime_ok(const char *str, uint64_t expected_val)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_datetime(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_DATETIME);
    TEST_MSG("Case='%s', error=failed", str);
    TEST_CHECK(token.datetime_val == expected_val);
    TEST_MSG("Case='%s', expected=%lu, result=%lu", str, expected_val, token.datetime_val);
}

void check_parse_datetime_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_datetime(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void _check_string_ok(const char *str, size_t len, const char *expected_val)
{
    yy_symbol_t symbol = {0};
    size_t expected_len = strlen(expected_val);

    yy_retcode_e rc = read_symbol_string(str, str + len, &symbol);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%.*s', error=failed", (int) len, str);
    TEST_CHECK(symbol.type == YY_SYMBOL_STRING_VAL);
    TEST_MSG("Case='%.*s', error=not-a-string", (int) len, str);
    TEST_CHECK(symbol.lexeme.ptr == str);
    TEST_MSG("Case='%.*s', error=invalid-lexeme-ptr", (int) len, str);
    TEST_CHECK(symbol.lexeme.len == expected_len + 2);
    TEST_MSG("Case='%.*s', error=invalid-lexeme-len", (int) len, str);
    TEST_CHECK(symbol.str_val.len == expected_len);
    TEST_MSG("Case='%.*s', error=invalid-string-len", (int) len, str);
    TEST_CHECK(strncmp(symbol.str_val.ptr, expected_val, expected_len) == 0);
    TEST_MSG("Case='%.*s', error=invalid-string-content", (int) len, str);
}

void check_string_ko(const char *str, yy_retcode_e expected_result)
{
    yy_symbol_t symbol = {0};
    size_t len = (str == NULL ? 0 : strlen(str));
    const char *end = (str == NULL ? NULL : str + len);

    yy_retcode_e rc = read_symbol_string(str, end, &symbol);

    TEST_CHECK(rc == expected_result);
    TEST_MSG("Case='%s', expected=%d, result=%d", str, expected_result, rc);
}

void check_parse_boolean_ok(const char *str, bool expected_val)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_bool(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_BOOL);
    TEST_MSG("Case='%s', error=not-a-boolean", str);
    TEST_CHECK(expected_val == token.bool_val);
    TEST_MSG("Case='%s', error=unexpected-value", str);
}

void check_parse_boolean_ko(const char *str)
{
    size_t len = strlen(str);

    yy_token_t token = yy_parse_bool(str, str + len);

    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    TEST_MSG("Case='%s', error=not-failed", str);
}

void _check_read_symbol_variable_ok(const char *str, size_t len, const char *expected_val)
{
    yy_symbol_t symbol = {0};
    size_t expected_len = strlen(expected_val);

    yy_retcode_e rc = read_symbol_variable(str, str + len, &symbol);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%.*s', error=failed", (int) len, str);
    TEST_CHECK(symbol.type == YY_SYMBOL_VARIABLE);
    TEST_MSG("Case='%.*s', error=not-a-variable", (int) len, str);
    TEST_CHECK(symbol.variable.len == expected_len);
    TEST_MSG("Case='%.*s', error=invalid-length", (int) len, str);
    TEST_CHECK(strncmp(symbol.variable.ptr, expected_val, expected_len) == 0);
    TEST_MSG("Case='%.*s', expected=%s, result=%.*s", (int) len, str, expected_val, (int) symbol.variable.len, symbol.variable.ptr);
    TEST_CHECK(symbol.lexeme.len == expected_len + 3);
    TEST_MSG("Case='%.*s', error=invalid-lexeme-len", (int) len, str);
}

void check_read_symbol_variable_ko(const char *str, yy_retcode_e expected_result)
{
    yy_symbol_t symbol = {0};
    const char *end = str + (str ? strlen(str) : 0);

    yy_retcode_e rc = read_symbol_variable(str, end, &symbol);

    TEST_CHECK(rc == expected_result);
    TEST_MSG("Case='%s', expected=%d, result=%d", str, expected_result, rc);
}

void check_next_ok(const char *str, yy_symbol_e type, yy_symbol_t *symbol)
{
    const char *begin = str;
    const char *end = str + strlen(str);

    yy_retcode_e rc = read_symbol(begin, end, symbol);

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

    yy_retcode_e rc = read_symbol(begin, end, &symbol);

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

void check_compile_number_ok(const char *str)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_retcode_e rc = yy_compile_number(str, str + strlen(str), &stack, NULL);

    TEST_CHECK(rc == YY_OK);
    TEST_MSG("Case='%s', error=unexpected-rc, rc=%d", str, rc);

    printf("%s --> ", str);
    print_stack(&stack);
    printf("\n");
}

void check_compile_number_ko(const char *str)
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {data, sizeof(data)/sizeof(data[0]), 0};

    yy_retcode_e rc = yy_compile_number(str, str + strlen(str), &stack, NULL);

    TEST_CHECK(rc != YY_OK);
    TEST_MSG("Case='%s', error=failed", str);
}

// ==============

void check_string_ok(const char *str, const char *expected_val)
{
    char buf[1024] = {0};
    size_t len = strlen(str);

    strcpy(buf, str);

    // isolate
    _check_string_ok(buf, len, expected_val);

    // same range, ending by '9'
    buf[len] = '9';
    _check_string_ok(buf, len, expected_val);

    // range + 1, ending by '+'
    buf[len] = '+';
    _check_string_ok(buf, len + 1, expected_val);
}

void check_read_symbol_variable_ok(const char *str, const char *expected_val)
{
    char buf[1024] = {0};
    size_t len = strlen(str);

    strcpy(buf, str);

    // isolate
    _check_read_symbol_variable_ok(buf, len, expected_val);

    // same range, ending by '9'
    buf[len] = '9';
    _check_read_symbol_variable_ok(buf, len, expected_val);

    // range + 1, ending by '+'
    buf[len] = '+';
    _check_read_symbol_variable_ok(buf, len + 1, expected_val);
}

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

void test_string_ok(void)
{
    check_string_ok("\"\"", "");

    check_string_ok("\"abc\"", "abc");

    check_string_ok("\"\\n\"", "\\n");
    check_string_ok("\"\\nabc\"", "\\nabc");
    check_string_ok("\"abc\\n\"", "abc\\n");
    check_string_ok("\"abc\\ndef\"", "abc\\ndef");

    check_string_ok("\"\\t\"", "\\t");
    check_string_ok("\"\\tabc\"", "\\tabc");
    check_string_ok("\"abc\\t\"", "abc\\t");
    check_string_ok("\"abc\\tdef\"", "abc\\tdef");

    check_string_ok("\"\\\\\"", "\\\\");
    check_string_ok("\"\\\\abc\"", "\\\\abc");
    check_string_ok("\"abc\\\\\"", "abc\\\\");
    check_string_ok("\"abc\\\\def\"", "abc\\\\def");

    check_string_ok("\"\\\"\"", "\\\"");
    check_string_ok("\"\\\"abc\"", "\\\"abc");
    check_string_ok("\"abc\\\"\"", "abc\\\"");
    check_string_ok("\"abc\\\"def\"", "abc\\\"def");

    check_string_ok("\"\\n\\\\\\t\\\"\"", "\\n\\\\\\t\\\"");
}

void test_string_ko(void)
{
    check_string_ko("", YY_ERROR_INVALID_STRING);
    check_string_ko(" ", YY_ERROR_INVALID_STRING);
    check_string_ko("a", YY_ERROR_INVALID_STRING);
    check_string_ko(" \"abc\"", YY_ERROR_INVALID_STRING);
    check_string_ko("\"", YY_ERROR_INVALID_STRING);
    check_string_ko("\"non terminated str", YY_ERROR_INVALID_STRING);

    check_string_ko("\"\\\"", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\t", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\n", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\\\", YY_ERROR_INVALID_STRING);

    check_string_ko("\"abc\\\"", YY_ERROR_INVALID_STRING);
    check_string_ko("\"abc\\t", YY_ERROR_INVALID_STRING);
    check_string_ko("\"abc\\n", YY_ERROR_INVALID_STRING);
    check_string_ko("\"abc\\\\", YY_ERROR_INVALID_STRING);

    check_string_ko("\"\\\"abc", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\tabc", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\nabc", YY_ERROR_INVALID_STRING);
    check_string_ko("\"\\\\abc", YY_ERROR_INVALID_STRING);

    check_string_ko("\"abc\\xdef\"", YY_ERROR_INVALID_STRING);  // unrecognized escaped char (\x)

    // 0 in-the-middle
    yy_symbol_t symbol = {0};
    char str[] = { '"', 'a', 'b', 'c', 0, 'd', 'e', 'f', '"', 0};
    TEST_CHECK(read_symbol_string(str, str + sizeof(str) - 1, &symbol) == YY_ERROR_INVALID_STRING);
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

void test_read_symbol_variable_ok(void)
{
    check_read_symbol_variable_ok("${x}", "x");
    check_read_symbol_variable_ok("${a.b.c}", "a.b.c");
    check_read_symbol_variable_ok("${abc_def}", "abc_def");
    check_read_symbol_variable_ok("${ABC_DEF.GHI}", "ABC_DEF.GHI");
    check_read_symbol_variable_ok("${x01.A23._56}", "x01.A23._56");
    check_read_symbol_variable_ok("${x__}", "x__");
    check_read_symbol_variable_ok("${x._._._}", "x._._._");
}

void test_read_symbol_variable_ko(void)
{
    check_read_symbol_variable_ko("", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko(" ", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko(" ${x}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("x", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("$x", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("$x}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${xxxx", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${0abc}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${.abc}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${_abc}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${abc..def}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${abc.def.}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${abc+def}", YY_ERROR_INVALID_VARIABLE);
    check_read_symbol_variable_ko("${abc-def}", YY_ERROR_INVALID_VARIABLE);
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
    check_next_ok("&& (${x} > 4)", YY_SYMBOL_AND_OP, &symbol);
    check_next_ok("|| x", YY_SYMBOL_OR_OP, &symbol);
    check_next_ok("== 25", YY_SYMBOL_EQUALS_OP, &symbol);
    check_next_ok("!= 42", YY_SYMBOL_DISTINCT_OP, &symbol);
    check_next_ok("< 42", YY_SYMBOL_LESS_OP, &symbol);
    check_next_ok("<= 42", YY_SYMBOL_LESS_EQUALS_OP, &symbol);
    check_next_ok("> 42", YY_SYMBOL_GREAT_OP, &symbol);
    check_next_ok(">= 42", YY_SYMBOL_GREAT_EQUALS_OP, &symbol);
    check_next_ok("!!false", YY_SYMBOL_NOT_OP, &symbol);
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
    check_next_ok("concat(\"abc\", \"def\")", YY_SYMBOL_CONCAT, &symbol);
    check_next_ok("substr(\"abcdef\", 1, 3)", YY_SYMBOL_SUBSTR, &symbol);
    check_next_ok("pow(2, 3+1)", YY_SYMBOL_POWER, &symbol);
    check_next_ok("!${b}", YY_SYMBOL_NOT_OP, &symbol);
    check_next_ok("< 5", YY_SYMBOL_LESS_OP, &symbol);
    check_next_ok("> 42", YY_SYMBOL_GREAT_OP, &symbol);
}

void test_read_symbol_ko(void)
{
    check_next_ko("unknow_keyword");
    check_next_ko("mmm");
    check_next_ko("@ unrecognized first letter");
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
    check_next_ko("$a");
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

void test_compile_number(void)
{
    check_compile_number_ok("1+2");
    check_compile_number_ok("1+2-3");
    check_compile_number_ok("1*2/3");
    check_compile_number_ok("1+2*3");
    check_compile_number_ok("1*2+3");
    check_compile_number_ok("-3+1");
    check_compile_number_ok("+3");
    check_compile_number_ok("1+(2*3)-3");
    check_compile_number_ok("1+(2*3)/4-3");
    check_compile_number_ok("-(1+(2*3)/4)-3");
    check_compile_number_ok("(min(1,2)-max(3,4))*3");
    check_compile_number_ok("-4%3 + 2^5 - (pow(2,3))");
    check_compile_number_ok("${a} + 1");
    check_compile_number_ok("((((-1))))");
    check_compile_number_ok("abs(-PI)");
    check_compile_number_ok("2 * (-1)");
    check_compile_number_ok("min(2+3*4, 1+3*5)");
    check_compile_number_ok("sqrt(exp(((0 * (-4332.4091)) / (10865972.2922 - 275715300.8411))))");
    check_compile_number_ok("log((-2729166) / (-0.0205) * exp(0))");

    check_compile_number_ko(" ");
    check_compile_number_ko("not_a_var");
    check_compile_number_ko("+");
    check_compile_number_ko("()");
    check_compile_number_ko("(((((())))))");
    check_compile_number_ko("((1)");
    check_compile_number_ko("1+(");
    check_compile_number_ko("1+()");
    check_compile_number_ko("1*/3");
    check_compile_number_ko("2^^3");
    check_compile_number_ko("coa(pi)");
    check_compile_number_ko("cosh(pi)");
    check_compile_number_ko("min");
    check_compile_number_ko("min(");
    check_compile_number_ko("min(,");
    check_compile_number_ko("min(1,");
    check_compile_number_ko("min(1,)");
    check_compile_number_ko("min(1,2");
    check_compile_number_ko("+-1"); // two consecutive operators
    check_compile_number_ko("++1"); // two consecutive operators
    check_compile_number_ko("1++1"); // two consecutive operators
    check_compile_number_ko("1+-1"); // two consecutive operators
    check_compile_number_ko("2 * -1"); // two consecutive operators
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

void test_funcs_number(void)
{
    const char *date_str = "2024-08-26T14:16:53.493Z";
    yy_token_t token = {0};
    yy_token_t date = {0};

    // length()
    token = func_length(token_string("xxx", 3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 3);
    token = func_length(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // datepart()
    date = yy_parse_datetime(date_str, date_str + strlen(date_str));
    TEST_CHECK(date.type == YY_TOKEN_DATETIME);
    token = func_datepart(date, token_number(0)); // year
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 2024);
    token = func_datepart(date, token_number(1)); // month
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 8);
    token = func_datepart(date, token_number(2)); // day
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 26);
    token = func_datepart(date, token_number(3)); // hour
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 14);
    token = func_datepart(date, token_number(4)); // minute
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 16);
    token = func_datepart(date, token_number(5)); // second
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 53);
    token = func_datepart(date, token_number(6)); // millis
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 493);
    token = func_datepart(date, token_number(99)); // unknow part
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_datepart(date, token_bool(false)); // unexpected type
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_datepart(token_number(1), token_number(1)); // invalid date
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // abs()
    token = func_abs(token_number(10));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 10);
    token = func_abs(token_number(-10));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 10);
    token = func_abs(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // ceil()
    token = func_ceil(token_number(2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 3);
    token = func_ceil(token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 3);
    token = func_ceil(token_number(-2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == -2);
    token = func_ceil(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // floor()
    token = func_floor(token_number(2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 2);
    token = func_floor(token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 3);
    token = func_floor(token_number(-2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == -3);
    token = func_floor(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // trunc()
    token = func_trunc(token_number(2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 2);
    token = func_trunc(token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 3);
    token = func_trunc(token_number(-2.3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == -2);
    token = func_trunc(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // sin()
    token = func_sin(token_number(0));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(token.number_val) < EPSILON);
    token = func_sin(token_number(M_PI_2));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_sin(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // cos()
    token = func_cos(token_number(0));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_cos(token_number(M_PI_2));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(token.number_val) < EPSILON);
    token = func_cos(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // tan()
    token = func_tan(token_number(0));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(token.number_val) < EPSILON);
    token = func_tan(token_number(M_PI_4));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_tan(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // exp()
    token = func_exp(token_number(1));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(M_E - token.number_val) < EPSILON);
    token = func_exp(token_number(0));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_exp(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // log()
    token = func_log(token_number(M_E));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_log(token_number(M_E*M_E));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(2.0 - token.number_val) < EPSILON);
    token = func_log(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // sqrt()
    token = func_sqrt(token_number(0));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(token.number_val) < EPSILON);
    token = func_sqrt(token_number(100));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(10.0 - token.number_val) < EPSILON);
    token = func_sqrt(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // pow()
    token = func_pow(token_number(2), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(8.0 - token.number_val) < EPSILON);
    token = func_pow(token_number(1), token_number(6));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_pow(token_number(1), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_pow(token_bool(true), token_number(1));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // minus()
    token = func_minus(token_number(1));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(-1.0 - token.number_val) < EPSILON);
    token = func_minus(token_number(-1));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(1.0 - token.number_val) < EPSILON);
    token = func_minus(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // ident()
    token = func_ident(token_number(1));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 1.0);
    token = func_ident(token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // addition()
    token = func_addition(token_number(2), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 5.0);
    token = func_addition(token_bool(true), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_addition(token_number(2), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // subtraction()
    token = func_subtraction(token_number(2), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == -1.0);
    token = func_subtraction(token_bool(true), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_subtraction(token_number(2), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // mult()
    token = func_mult(token_number(2), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 6.0);
    token = func_mult(token_bool(true), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_mult(token_number(2), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // div()
    token = func_div(token_number(2), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(fabs(2.0/3.0 - token.number_val) < EPSILON);
    token = func_div(token_bool(true), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_div(token_number(2), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);

    // mod()
    token = func_mod(token_number(7), token_number(5));
    TEST_CHECK(token.type == YY_TOKEN_NUMBER);
    TEST_CHECK(token.number_val == 2.0);
    token = func_mod(token_bool(true), token_number(3));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
    token = func_mod(token_number(2), token_bool(true));
    TEST_CHECK(token.type == YY_TOKEN_ERROR);
}

void test_funcs_datetime(void)
{
    char buf[128] = {0};
    yy_token_t token1 = {0};
    yy_token_t token2 = {0};

    // get_datepart
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

    // now()
    token1 = func_now();
    TEST_CHECK(token1.type == YY_TOKEN_DATETIME);
    datetime_to_str(token1.datetime_val, buf);
    token2 = yy_parse_datetime(buf, buf + strlen(buf));
    TEST_CHECK(token2.type == YY_TOKEN_DATETIME);
    TEST_CHECK(token1.datetime_val == token2.datetime_val);

    // dateadd()
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

    // dateset()
    check_dateset("2024-08-26T14:16:53.493Z",  2027, "year"  , "2027-08-26T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",     3, "month" , "2024-03-26T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    23, "day"   , "2024-08-23T14:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",     5, "hour"  , "2024-08-26T05:16:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    40, "minute", "2024-08-26T14:40:53.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",    12, "second", "2024-08-26T14:16:12.493Z");
    check_dateset("2024-08-26T14:16:53.493Z",   123, "millis", "2024-08-26T14:16:53.123Z");
    check_dateset("2024-08-26T14:16:53.493Z",    10, "xxx"   , NULL);

    // datetrunc()
    check_datetrunc("2024-08-26T14:16:53.493Z", "year"  , "1970-01-01T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "month" , "2024-01-01T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "day"   , "2024-08-01T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "hour"  , "2024-08-26T00:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "minute", "2024-08-26T14:00:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "second", "2024-08-26T14:16:00.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "millis", "2024-08-26T14:16:53.000Z");
    check_datetrunc("2024-08-26T14:16:53.493Z", "xxx"   , NULL);
}

TEST_LIST = {
    { "sizeof",                       test_sizeof },
    { "yy_parse_number_ok",           test_parse_number_ok },
    { "yy_parse_number_ko",           test_parse_number_ko },
    { "yy_parse_datetime_ok",         test_parse_datetime_ok },
    { "yy_parse_datetime_ko",         test_parse_datetime_ko },
    { "yy_parse_boolean_ok",          test_parse_boolean_ok },
    { "yy_parse_boolean_ko",          test_parse_boolean_ko },
    { "string_ok",                    test_string_ok },
    { "string_ko",                    test_string_ko },
    { "read_symbol_variable_ok",      test_read_symbol_variable_ok },
    { "read_symbol_variable_ko",      test_read_symbol_variable_ko },
    { "read_symbol_ok",               test_read_symbol_ok },
    { "read_symbol_ko",               test_read_symbol_ko },
    { "skip_spaces",                  test_skip_spaces },
    { "yy_compile_number",            test_compile_number },
    { "funcs_number",                 test_funcs_number },
    { "funcs_datetime",               test_funcs_datetime },
    { NULL, NULL }
};
