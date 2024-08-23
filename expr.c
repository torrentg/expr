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

#include <math.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "expr.h"

/**
 * @see https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
 * @see https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 */

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER) 
    #define INLINE        __attribute__((always_inline)) inline
    #define likely(x)     __builtin_expect(!!(x), 1)
    #define unlikely(x)   __builtin_expect(!!(x), 0)
#else
    #define INLINE        /**/
    #define likely(x)     x
    #define unlikely(x)   x
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifndef M_E
    #define M_E     2.7182818284590452354
#endif

#ifndef M_PI
    #define M_PI    3.14159265358979323846
#endif

#define FPTR  (void (*)(void))

typedef yy_token_t (*yy_func_0)(void);
typedef yy_token_t (*yy_func_1)(yy_token_t);
typedef yy_token_t (*yy_func_2)(yy_token_t, yy_token_t);
typedef yy_token_t (*yy_func_3)(yy_token_t, yy_token_t, yy_token_t);

typedef enum yy_symbol_e
{
    YY_SYMBOL_NONE,                 //!< Unassigned symbol.
    YY_SYMBOL_TRUE,                 //!< true, True, TRUE
    YY_SYMBOL_FALSE,                //!< false, False, FALSE
    YY_SYMBOL_NUMBER_VAL,           //!< 1234, 3.14159, 0.24e-4
    YY_SYMBOL_DATETIME_VAL,         //!< 2024-07-24T07:57:14.494Z
    YY_SYMBOL_STRING_VAL,           //!< "s3cr3t"
    YY_SYMBOL_VARIABLE,             //!< ${x}
    YY_SYMBOL_CONST_E,              //!< E
    YY_SYMBOL_CONST_PI,             //!< PI
    YY_SYMBOL_PAREN_LEFT,           //!< (
    YY_SYMBOL_PAREN_RIGHT,          //!< )
    YY_SYMBOL_COMMA,                //!< ,
    YY_SYMBOL_AND_OP,               //!< &&
    YY_SYMBOL_OR_OP,                //!< ||
    YY_SYMBOL_EQUALS_OP,            //!< ==
    YY_SYMBOL_DISTINCT_OP,          //!< !=
    YY_SYMBOL_LESS_OP,              //!< <
    YY_SYMBOL_LESS_EQUALS_OP,       //!< <=
    YY_SYMBOL_GREAT_OP,             //!< >
    YY_SYMBOL_GREAT_EQUALS_OP,      //!< >=
    YY_SYMBOL_NOT_OP,               //!< !
    YY_SYMBOL_PLUS_OP,              //!< + (prefix)
    YY_SYMBOL_MINUS_OP,             //!< - (prefix)
    YY_SYMBOL_ADDITION_OP,          //!< + (infix)
    YY_SYMBOL_SUBTRACTION_OP,       //!< - (infix)
    YY_SYMBOL_PRODUCT_OP,           //!< *
    YY_SYMBOL_DIVIDE_OP,            //!< /
    YY_SYMBOL_MODULO_OP,            //!< %
    YY_SYMBOL_POWER_OP,             //!< ^
    YY_SYMBOL_ABS,                  //!< abs
    YY_SYMBOL_MIN,                  //!< min
    YY_SYMBOL_MAX,                  //!< max
    YY_SYMBOL_MODULO,               //!< mod
    YY_SYMBOL_POWER,                //!< pow
    YY_SYMBOL_SQRT,                 //!< sqrt
    YY_SYMBOL_SIN,                  //!< sin
    YY_SYMBOL_COS,                  //!< cos
    YY_SYMBOL_TAN,                  //!< tan
    YY_SYMBOL_EXP,                  //!< exp
    YY_SYMBOL_LOG,                  //!< log
    YY_SYMBOL_TRUNC,                //!< trunc
    YY_SYMBOL_CEIL,                 //!< ceil
    YY_SYMBOL_FLOOR,                //!< floor
    YY_SYMBOL_NOW,                  //!< now
    YY_SYMBOL_DATEPART,             //!< datepart
    YY_SYMBOL_DATEADD,              //!< dateadd
    YY_SYMBOL_LENGTH,               //!< length
    YY_SYMBOL_LOWER,                //!< lower
    YY_SYMBOL_UPPER,                //!< upper
    YY_SYMBOL_TRIM,                 //!< trim
    YY_SYMBOL_CONCAT,               //!< concat
    YY_SYMBOL_SUBSTR,               //!< substr
    YY_SYMBOL_END,                  //!< No more symbols (maintain at the end of list)
} yy_symbol_e;

typedef struct yy_symbol_t
{
    yy_str_t lexeme;                //!< String representing the symbol.
    yy_symbol_e type;               //!< Type of symbol.
    union
    {
        double number_val;          //!< Number value (without sign).
        uint64_t datetime_val;      //!< Timestamp value (millis from epoch-time).
        yy_str_t str_val;           //!< String value.
        yy_str_t variable;          //!< Variable name.
    };
} yy_symbol_t;

typedef struct yy_parser_t
{
    const char *begin;              //!< Expression to parse.
    const char *end;                //!< One char after the end of the expression to parse.
    const char *curr;               //!< Current position being parsed.
    yy_stack_t *stack;              //!< Resulting RPN stack.
    yy_stack_t *operators;          //!< Operators stack.
    yy_symbol_t curr_symbol;        //!< Current symbol.
    yy_symbol_t prev_symbol;        //!< Previous symbol.
    yy_retcode_e error;             //!< Error code (YY_OK means no error), curr points to error location.
} yy_parser_t;

typedef struct yy_identifier_t
{
    const char *str;
    yy_symbol_e type;
} yy_identifier_t;

// Days in month
static const int days_in_month[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Forward declarations
static bool parse_expr_number(yy_parser_t *parser);
static yy_token_t func_now(void);
static yy_token_t func_datepart(yy_token_t date, yy_token_t part);
static yy_token_t func_dateadd(yy_token_t date, yy_token_t part, yy_token_t value);
static yy_token_t func_trim(yy_token_t str);
static yy_token_t func_lower(yy_token_t str);
static yy_token_t func_upper(yy_token_t str);
static yy_token_t func_length(yy_token_t str);
static yy_token_t func_concat(yy_token_t str1, yy_token_t str2);
static yy_token_t func_substr(yy_token_t str, yy_token_t start, yy_token_t len);
static yy_token_t func_abs(yy_token_t x);
static yy_token_t func_ceil(yy_token_t x);
static yy_token_t func_floor(yy_token_t x);
static yy_token_t func_trunc(yy_token_t x);
static yy_token_t func_sin(yy_token_t x);
static yy_token_t func_cos(yy_token_t x);
static yy_token_t func_tan(yy_token_t x);
static yy_token_t func_exp(yy_token_t x);
static yy_token_t func_log(yy_token_t x);
static yy_token_t func_sqrt(yy_token_t x);
static yy_token_t func_pow(yy_token_t x, yy_token_t y);
static yy_token_t func_minus(yy_token_t x);
static yy_token_t func_ident(yy_token_t x);
static yy_token_t func_mult(yy_token_t x, yy_token_t y);
static yy_token_t func_div(yy_token_t x, yy_token_t y);
static yy_token_t func_mod(yy_token_t x, yy_token_t y);
static yy_token_t func_addition(yy_token_t x, yy_token_t y);
static yy_token_t func_subtraction(yy_token_t x, yy_token_t y);
static yy_token_t func_not(yy_token_t x);
static yy_token_t func_lt(yy_token_t x, yy_token_t y);
static yy_token_t func_le(yy_token_t x, yy_token_t y);
static yy_token_t func_gt(yy_token_t x, yy_token_t y);
static yy_token_t func_ge(yy_token_t x, yy_token_t y);
static yy_token_t func_eq(yy_token_t x, yy_token_t y);
static yy_token_t func_ne(yy_token_t x, yy_token_t y);
static yy_token_t func_and(yy_token_t x, yy_token_t y);
static yy_token_t func_or(yy_token_t x, yy_token_t y);
static yy_token_t func_max(yy_token_t x, yy_token_t y);
static yy_token_t func_min(yy_token_t x, yy_token_t y);

// Identifiers list (alphabetical order)
static const yy_identifier_t identifiers[] =
{
    { "E",        YY_SYMBOL_CONST_E  },
    { "FALSE",    YY_SYMBOL_FALSE    },
    { "False",    YY_SYMBOL_FALSE    },
    { "PI",       YY_SYMBOL_CONST_PI },
    { "TRUE",     YY_SYMBOL_TRUE     },
    { "True",     YY_SYMBOL_TRUE     },
    { "abs",      YY_SYMBOL_ABS      },
    { "ceil",     YY_SYMBOL_CEIL     },
    { "concat",   YY_SYMBOL_CONCAT   },
    { "cos",      YY_SYMBOL_COS      },
    { "dateadd",  YY_SYMBOL_DATEADD  },
    { "datepart", YY_SYMBOL_DATEPART },
    { "exp",      YY_SYMBOL_EXP      },
    { "false",    YY_SYMBOL_FALSE    },
    { "floor",    YY_SYMBOL_FLOOR    },
    { "length",   YY_SYMBOL_LENGTH   },
    { "log",      YY_SYMBOL_LOG      },
    { "lower",    YY_SYMBOL_LOWER    },
    { "max",      YY_SYMBOL_MAX      },
    { "min",      YY_SYMBOL_MIN      },
    { "mod",      YY_SYMBOL_MODULO   },
    { "now",      YY_SYMBOL_NOW      },
    { "pow",      YY_SYMBOL_POWER    },
    { "sin",      YY_SYMBOL_SIN      },
    { "sqrt",     YY_SYMBOL_SQRT     },
    { "substr",   YY_SYMBOL_SUBSTR   },
    { "tan",      YY_SYMBOL_TAN      },
    { "trim",     YY_SYMBOL_TRIM     },
    { "true",     YY_SYMBOL_TRUE     },
    { "trunc",    YY_SYMBOL_TRUNC    },
    { "upper",    YY_SYMBOL_UPPER    },
};

#define NUM_IDENTIFIERS (sizeof(identifiers)/sizeof(yy_identifier_t))

static const yy_token_t symbol_to_token[] =
{
    [YY_SYMBOL_NUMBER_VAL]      = { .type = YY_TOKEN_NUMBER   }, 
    [YY_SYMBOL_DATETIME_VAL]    = { .type = YY_TOKEN_DATETIME }, 
    [YY_SYMBOL_STRING_VAL]      = { .type = YY_TOKEN_STRING   }, 
    [YY_SYMBOL_VARIABLE]        = { .type = YY_TOKEN_VARIABLE }, 

    [YY_SYMBOL_TRUE]            = { .type = YY_TOKEN_BOOL    , .bool_val = true    },
    [YY_SYMBOL_FALSE]           = { .type = YY_TOKEN_BOOL    , .bool_val = false   },
    [YY_SYMBOL_CONST_E]         = { .type = YY_TOKEN_NUMBER  , .number_val = M_E   },
    [YY_SYMBOL_CONST_PI]        = { .type = YY_TOKEN_NUMBER  , .number_val = M_PI  },

    // [YY_SYMBOL_PAREN_LEFT]   = { .type = YY_TOKEN_NULL     },
    // [YY_SYMBOL_PAREN_RIGHT]  = { .type = YY_TOKEN_NULL     },
    // [YY_SYMBOL_COMMA]        = { .type = YY_TOKEN_NULL     },

    [YY_SYMBOL_POWER_OP]        = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_pow        , 2, 2 } },
    [YY_SYMBOL_NOT_OP]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_not        , 1, 3, true } },
    [YY_SYMBOL_MINUS_OP]        = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_minus      , 1, 3, true } },
    [YY_SYMBOL_PLUS_OP]         = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_ident      , 1, 3, true } },
    [YY_SYMBOL_PRODUCT_OP]      = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_mult       , 2, 4 } },
    [YY_SYMBOL_DIVIDE_OP]       = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_div        , 2, 4 } },
    [YY_SYMBOL_MODULO_OP]       = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_mod        , 2, 4 } },
    [YY_SYMBOL_ADDITION_OP]     = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_addition   , 2, 5 } },
    [YY_SYMBOL_SUBTRACTION_OP]  = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_subtraction, 2, 5 } },
    [YY_SYMBOL_LESS_OP]         = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_lt         , 2, 6 } },
    [YY_SYMBOL_LESS_EQUALS_OP]  = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_le         , 2, 6 } },
    [YY_SYMBOL_GREAT_OP]        = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_gt         , 2, 6 } },
    [YY_SYMBOL_GREAT_EQUALS_OP] = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_ge         , 2, 6 } },
    [YY_SYMBOL_EQUALS_OP]       = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_eq         , 2, 7 } },
    [YY_SYMBOL_DISTINCT_OP]     = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_ne         , 2, 7 } },
    [YY_SYMBOL_AND_OP]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_and        , 2, 8 } },
    [YY_SYMBOL_OR_OP]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_or         , 2, 9 } },

    [YY_SYMBOL_ABS]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_abs        , 1 } },
    [YY_SYMBOL_MODULO]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_mod        , 2 } },
    [YY_SYMBOL_POWER]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_pow        , 2 } },
    [YY_SYMBOL_SQRT]            = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_sqrt       , 1 } },
    [YY_SYMBOL_SIN]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_sin        , 1 } },
    [YY_SYMBOL_COS]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_cos        , 1 } },
    [YY_SYMBOL_TAN]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_tan        , 1 } },
    [YY_SYMBOL_EXP]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_exp        , 1 } },
    [YY_SYMBOL_LOG]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_log        , 1 } },
    [YY_SYMBOL_TRUNC]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_trunc      , 1 } },
    [YY_SYMBOL_CEIL]            = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_ceil       , 1 } },
    [YY_SYMBOL_FLOOR]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_floor      , 1 } },
    [YY_SYMBOL_NOW]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_now        , 0 } },
    [YY_SYMBOL_DATEPART]        = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_datepart   , 2 } },
    [YY_SYMBOL_DATEADD]         = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_dateadd    , 3 } },
    [YY_SYMBOL_LENGTH]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_length     , 1 } },
    [YY_SYMBOL_LOWER]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_lower      , 1 } },
    [YY_SYMBOL_UPPER]           = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_upper      , 1 } },
    [YY_SYMBOL_TRIM]            = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_trim       , 1 } },
    [YY_SYMBOL_CONCAT]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_concat     , 2 } },
    [YY_SYMBOL_SUBSTR]          = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_substr     , 3 } },
    [YY_SYMBOL_MIN]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_min        , 2 } },
    [YY_SYMBOL_MAX]             = { .type = YY_TOKEN_FUNCTION, .function = { FPTR func_max        , 2 } },
    [YY_SYMBOL_END]             = { .type = YY_TOKEN_NULL }
};

// Check if year is a leap year
static bool is_leap_year(int year) {
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

/**
 * Parse an unsigned number.
 * 
 * Parse until an unrecognized char is found.
 * Input string range can be greater than parsed string.
 * 
 * YY_SYMBOL_NUMBER_VAL
 *    - regex := (0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?(0|[1-9][0-9]*))?
 *    - range := @see https://en.wikipedia.org/wiki/Double-precision_floating-point_format
 *    - format := JSON-format, RFC-7159 (section 6 -numbers-)
 *    - see := https://tools.ietf.org/html/rfc7159 (section 6 -numbers-)
 * 
 * Unsupported strtod() valid cases:
 *    - Non-zero number starting with zero: 00, 03, 003.14
 *    - Number starting with dot: .5, .314e1
 *    - Number ending with dot: 1., 42.
 *    - Dot without fractional part: 1.e1
 *    - Non-zero exponent starting with zero: 1e04
 * 
 * This function was inspired by re2c generated code:
 * 
 *   bool lex2(const char *str) {
 *     const char *YYMARKER = NULL;
 *     /\*!re2c
 *        re2c:yyfill:enable = 0;
 *        re2c:define:YYCTYPE = char;
 *        re2c:define:YYCURSOR = str;
 *        number = '0' | [1-9][0-9]*;
 *        float = number ( "." [0-9]+ )? ([eE][+-]? number)?;
 *        float { return true; }
 *        *     { return false; }
 *     *\/
 *   }
 * 
 * Alternatives to consider:
 *   - https://github.com/lemire/fast_double_parser (need port to C)
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_INVALID_NUMBER: Invalid number.
 *          YY_ERROR_RANGE_NUMBER: Number out of range
 */
static yy_retcode_e parse_number(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    char buf[128];
    size_t len = 0;
    int64_t int_val = 0;
    double number_val;
    const char *ptr = begin;

    if (unlikely(ptr == end))
        return YY_ERROR_INVALID_NUMBER;

    switch (*ptr) {
        case '0': 
            goto NUMBER_INTEGER_PART_0;
        case '1' ... '9': goto NUMBER_INTEGER_PART_NON_0;
        default: return YY_ERROR_INVALID_NUMBER;
    }

NUMBER_INTEGER_PART_0:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_INTEGER;

    switch (*ptr) {
        case '0' ... '9': return YY_ERROR_INVALID_NUMBER;
        case '.': goto NUMBER_FRACTION_START;
        case 'E':
        case 'e': goto NUMBER_EXPONENT_START;
        default: goto NUMBER_READ_INTEGER;
    }

NUMBER_INTEGER_PART_NON_0:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_INTEGER;

    switch (*ptr) {
        case '0' ... '9': goto NUMBER_INTEGER_PART_NON_0;
        case '.': goto NUMBER_FRACTION_START;
        case 'E':
        case 'e': goto NUMBER_EXPONENT_START;
        default: goto NUMBER_READ_INTEGER;
    }

NUMBER_FRACTION_START:

    if (unlikely(++ptr == end))
        return YY_ERROR_INVALID_NUMBER;

    switch (*ptr) {
        case '0' ... '9': goto NUMBER_FRACTION_CONT;
        default: return YY_ERROR_INVALID_NUMBER;
    }

NUMBER_FRACTION_CONT:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_FLOAT;

    switch (*ptr) {
        case '0' ... '9': goto NUMBER_FRACTION_CONT;
        case 'E':
        case 'e': goto NUMBER_EXPONENT_START;
        default: goto NUMBER_READ_FLOAT;
    }

NUMBER_EXPONENT_START:

    if (unlikely(++ptr == end))
        return YY_ERROR_INVALID_NUMBER;

    switch (*ptr) {
        case '+':
        case '-': goto NUMBER_EXPONENT_NUM_START;
        case '0': goto NUMBER_EXPONENT_0;
        case '1' ... '9': goto NUMBER_EXPONENT_NUM_CONT;
        default: return YY_ERROR_INVALID_NUMBER;
    }

NUMBER_EXPONENT_NUM_START:

    if (unlikely(++ptr == end))
        return YY_ERROR_INVALID_NUMBER;

    switch (*ptr) {
        case '0': goto NUMBER_EXPONENT_0;
        case '1' ... '9': goto NUMBER_EXPONENT_NUM_CONT;
        default: return YY_ERROR_INVALID_NUMBER;
    }

NUMBER_EXPONENT_0:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_FLOAT;

    switch (*ptr) {
        case '1' ... '9': return YY_ERROR_INVALID_NUMBER;
        default: goto NUMBER_READ_FLOAT;
    }

NUMBER_EXPONENT_NUM_CONT:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_FLOAT;

    switch (*ptr) {
        case '0' ... '9': goto NUMBER_EXPONENT_NUM_CONT;
        default: goto NUMBER_READ_FLOAT;
    }

NUMBER_READ_INTEGER:

    assert(ptr > begin);
    assert(ptr <= end);

    if ((len = ptr - begin) > 16)  // 16 = length(2^53) in base-10
        return YY_ERROR_RANGE_NUMBER;

    for (const char *aux = begin; aux < ptr; ++aux)
        int_val = int_val * 10 + (*aux - '0');

    if (int_val > (1LL << 53))
        return YY_ERROR_RANGE_NUMBER;

    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) len;
    symbol->type = YY_SYMBOL_NUMBER_VAL;
    symbol->number_val = (double) int_val;

    return YY_OK;

NUMBER_READ_FLOAT:

    assert(ptr > begin);
    assert(ptr <= end);

    if ((len = ptr - begin) >= sizeof(buf))
        return YY_ERROR_RANGE_NUMBER;

    memcpy(buf, begin, len);
    buf[len] = 0;
    errno = 0;

    number_val = strtod(buf, NULL);

    if (errno == ERANGE)
        return YY_ERROR_RANGE_NUMBER;

    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) len;
    symbol->type = YY_SYMBOL_NUMBER_VAL;
    symbol->number_val = number_val;

    return YY_OK;
}

/**
 * Parse a datetime value in format ISO-8601.
 *    - format = YYYY-MM-DD[Tdd:mm:ss[.SSS[Z]]]
 *    - @see https://en.wikipedia.org/wiki/ISO_8601
 * 
 * Parse whole content (^.*$).
 * Anything distinct than datetime is reported as error.
 * 
 * YY_SYMBOL_DATETIME_VAL
 *    - regex := (19[7-9][0-9]|2[0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])(T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(.[0-9]{1,3})?)?
 *    - range := [1970-01-01T00:00:00.000Z, 2999-12-31T23:59:59.999Z]
 *
 * This function was inspired by re2c generated code:
 * 
 *   bool lex2(const char *str) {
 *     const char *YYMARKER = NULL;
 *     /\*!re2c
 *        re2c:yyfill:enable = 0;
 *        re2c:define:YYCTYPE = char;
 *        re2c:define:YYCURSOR = str;
 *        year = '19' [7-9][0-9] | '2' [0-9]{3};
 *        month = '0' [1-9] | '1' [0-2];
 *        day = '0' [1-9] | [12][0-9] | '3' [01];
 *        hour = [01][0-9] | '2' [0-3];
 *        minute = [0-5][0-9];
 *        second = [0-5][0-9];
 *        millis = [0-9]{1,3};
 *        datetime = year '-' month '-' day ( 'T' hour ':' minute ':' second ( '.' millis )? 'Z'? )?;
 *        datetime { return true; }
 *        *        { return false; }
 *     *\/
 *   }
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_INVALID_DATETIME: Invalid datetime.
 *          YY_ERROR_RANGE_DATETIME: Datetime out of range
 */
static yy_retcode_e parse_datetime(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    size_t len = 0;
    char buf[128];
    const char *ptr = begin;
    struct tm stm = {0};
    int millis = 0;

    // epoch time (used to parse only time)
    stm.tm_year = 70;
    stm.tm_mon = 0;
    stm.tm_mday = 1;

//DATETIME_DATE:

    if (end - ptr < 10) // no place for YYYY-MM-DD
        return YY_ERROR_INVALID_DATETIME;

    switch (*ptr) {
        case '1': buf[len++] = *ptr; goto DATETIME_YEAR_1900;
        case '2': buf[len++] = *ptr; goto DATETIME_YEAR_2000;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_YEAR_1900:

    if (*++ptr != '9')
        return YY_ERROR_INVALID_DATETIME;

    buf[len++] = *ptr;

    switch (*++ptr) {
        case '7' ... '9': buf[len++] = *ptr; break;
        default: return YY_ERROR_INVALID_DATETIME;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_YEAR;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_YEAR_2000:

    for (int i = 0; i < 3; i++)
    {
        switch (*++ptr) {
            case '0' ... '9': buf[len++] = *ptr; break;
            default: return YY_ERROR_INVALID_DATETIME;
        }
    }

DATETIME_READ_YEAR:

    assert(len == 4);
    buf[len] = 0;
    stm.tm_year = atoi(buf) - 1900;
    len = 0;

    if (*++ptr != '-')
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_MONTH:

    switch (*++ptr) {
        case '0': buf[len++] = *ptr; goto DATETIME_MONTH_0;
        case '1': buf[len++] = *ptr; goto DATETIME_MONTH_1;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_MONTH_0:

    switch (*++ptr) {
        case '1' ... '9': buf[len++] = *ptr; goto DATETIME_READ_MONTH;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_MONTH_1:

    switch (*++ptr) {
        case '0' ... '2': buf[len++] = *ptr; goto DATETIME_READ_MONTH;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_READ_MONTH:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_mon = atoi(buf) - 1;
    len = 0;

    if (*++ptr != '-')
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_DAY:

    switch (*++ptr) {
        case '0': buf[len++] = *ptr; goto DATETIME_DAY_0;
        case '1' ... '2': buf[len++] = *ptr; goto DATETIME_DAY_1_2;
        case '3': buf[len++] = *ptr; goto DATETIME_DAY_3;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_DAY_0:

    switch (*++ptr) {
        case '1' ... '9': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_DAY_1_2:

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_DAY_3:

    switch (*++ptr) {
        case '0' ... '1': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_READ_DAY:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_mday = atoi(buf);
    len = 0;

    if (++ptr == end)
        goto DATETIME_END;

    if (*ptr != 'T')
        return YY_ERROR_INVALID_DATETIME;

    if (++ptr == end)
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_TIME:

    if (end - ptr < 8) // no place for hh:mm:ss
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_HOUR:

    switch (*ptr) {
        case '0' ... '1': buf[len++] = *ptr; goto DATETIME_HOUR_0_1;
        case '2': buf[len++] = *ptr; goto DATETIME_HOUR_2;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_HOUR_0_1:

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_HOUR;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_HOUR_2:

    switch (*++ptr) {
        case '0' ... '3': buf[len++] = *ptr; goto DATETIME_READ_HOUR;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_READ_HOUR:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_hour = atoi(buf);
    len = 0;

    if (*++ptr != ':')
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_MINUTE:

    switch (*++ptr) {
        case '0' ... '5': buf[len++] = *ptr; break;
        default: return YY_ERROR_INVALID_DATETIME;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_MINUTE;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_READ_MINUTE:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_min = atoi(buf);
    len = 0;

    if (*++ptr != ':')
        return YY_ERROR_INVALID_DATETIME;

//DATETIME_SECOND:

    switch (*++ptr) {
        case '0' ... '5': buf[len++] = *ptr; break;
        default: return YY_ERROR_INVALID_DATETIME;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_SECOND;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_READ_SECOND:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_sec = atoi(buf);
    len = 0;

    if (++ptr == end)
        goto DATETIME_END;

    switch (*ptr) {
        case '.': goto DATETIME_MILLIS;
        case 'Z': goto DATETIME_ENDING_Z;
        default: return YY_ERROR_INVALID_DATETIME;
    }

DATETIME_MILLIS:

    if (++ptr == end)
        return YY_ERROR_INVALID_DATETIME;

    switch (*ptr) {
        case '0' ... '9': buf[len++] = *ptr; break;
        default: return YY_ERROR_INVALID_DATETIME;
    }

    for (int i = 0; i < 2; i++)
    {
        if (++ptr == end)
            goto DATETIME_READ_MILLIS;

        switch (*ptr) {
            case '0' ... '9': buf[len++] = *ptr; break;
            case 'Z': goto DATETIME_READ_MILLIS;
            default: return YY_ERROR_INVALID_DATETIME;
        }
    }

    ++ptr;

DATETIME_READ_MILLIS:

    assert(len > 0 && len < 4);
    buf[len] = 0;
    millis = atoi(buf);
    len = 0;

    if (ptr == end)
        goto DATETIME_END;

    if (*ptr != 'Z')
        return YY_ERROR_INVALID_DATETIME;

DATETIME_ENDING_Z:

    if (++ptr != end)
        return YY_ERROR_INVALID_DATETIME;

DATETIME_END:

    assert(ptr > begin);
    assert(ptr <= end);

    if (stm.tm_mday > days_in_month[stm.tm_mon])
        return YY_ERROR_INVALID_DATETIME;

    if (!is_leap_year(1900 + stm.tm_year) && stm.tm_mon == 1 && stm.tm_mday == 29)
        return YY_ERROR_INVALID_DATETIME;

    errno = 0;

    time_t val_secs =  timegm(&stm);

    if (errno != 0)
        return YY_ERROR_RANGE_DATETIME;

    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) (ptr - begin);
    symbol->type = YY_SYMBOL_DATETIME_VAL;
    symbol->datetime_val = val_secs * 1000 + millis;

    return YY_OK;
}

/**
 * Parse a string surrounded by double-quotes.
 * Accepted escape characters: \n (next-line), \t (tab), \" (double-quote), \\ (slash)
 * 
 * Parse until ending double-quote.
 * Input string range can be greater than parsed string.
 * 
 * Returned type:
 *   - YY_SYMBOL_STRING_VAL
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_INVALID_STRING: Invalid string.
 */
static yy_retcode_e parse_quoted_string(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    const char *ptr = begin;

    if (end - ptr < 2 || *ptr != '"') // no place for ""
        return YY_ERROR_INVALID_STRING;

STRING_NEXT_CHAR:

    if (++ptr == end)
        return YY_ERROR_INVALID_STRING;

    switch (*ptr) {
        case '\0': return YY_ERROR_INVALID_STRING;
        case '\\': goto STRING_ESCAPED_CHAR;
        case '"': goto STRING_END;
        default: goto STRING_NEXT_CHAR;
    }

STRING_ESCAPED_CHAR:

    if (end - ptr < 3)  // no place for \x"
        return YY_ERROR_INVALID_STRING;

    switch (*++ptr) {
        case '"': 
        case '\\': 
        case 'n': 
        case 't': goto STRING_NEXT_CHAR;
        default: return YY_ERROR_INVALID_STRING;
    }

STRING_END:

    ++ptr;
    assert(ptr - begin > 1); // minimum size is ""

    symbol->type = YY_SYMBOL_STRING_VAL;
    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) (ptr - begin);
    symbol->str_val.ptr = begin + 1;
    symbol->str_val.len = symbol->lexeme.len - 2;

    return YY_OK;
}

/**
 * Parse a boolean.
 * Accepted values = true, True, TRUE, false, False, FALSE.
 *
 * Parse whole content (^.*$).
 * Anything distinct than allowed values is reported as error.
 * 
 * Returned types:
 *   - YY_SYMBOL_TRUE
 *   - YY_SYMBOL_FALSE
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_INVALID_BOOLEAN: Invalid boolean.
 */
static yy_retcode_e parse_boolean(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    const char *ptr = begin;

    if (begin == end)
        return YY_ERROR_INVALID_BOOLEAN;

    switch (*ptr) {
        case 'f':
        case 'F': goto BOOLEAN_FALSE;
        case 't':
        case 'T': goto BOOLEAN_TRUE;
        default: return YY_ERROR_INVALID_BOOLEAN;
    }

BOOLEAN_FALSE:

    if (end - ptr != 5)
        return YY_ERROR_INVALID_BOOLEAN;

    switch (*++ptr) {
        case 'A':
            if (ptr[-1] == 'F' && ptr[1] == 'L' && ptr[2] == 'S' && ptr[3] == 'E') {
                symbol->type = YY_SYMBOL_FALSE;
                symbol->lexeme.ptr = begin;
                symbol->lexeme.len = 5;
                return YY_OK;
            }
            else
                return YY_ERROR_INVALID_BOOLEAN;
        case 'a':
            if (ptr[1] == 'l' && ptr[2] == 's' && ptr[3] == 'e') {
                symbol->type = YY_SYMBOL_FALSE;
                symbol->lexeme.ptr = begin;
                symbol->lexeme.len = 5;
                return YY_OK;
            }
            else
                return YY_ERROR_INVALID_BOOLEAN;
        default: return YY_ERROR_INVALID_BOOLEAN;
    }

BOOLEAN_TRUE:

    if (end - ptr != 4)
        return YY_ERROR_INVALID_BOOLEAN;

    switch (*++ptr) {
        case 'R':
            if (ptr[-1] == 'T' && ptr[1] == 'U' && ptr[2] == 'E') {
                symbol->type = YY_SYMBOL_TRUE;
                symbol->lexeme.ptr = begin;
                symbol->lexeme.len = 4;
                return YY_OK;
            }
            else
                return YY_ERROR_INVALID_BOOLEAN;
        case 'r':
            if (ptr[1] == 'u' && ptr[2] == 'e') {
                symbol->type = YY_SYMBOL_TRUE;
                symbol->lexeme.ptr = begin;
                symbol->lexeme.len = 4;
                return YY_OK;
            }
            else
                return YY_ERROR_INVALID_BOOLEAN;
        default: return YY_ERROR_INVALID_BOOLEAN;
    }
}

/**
 * Parse a variable name.
 * 
 * Format  = '${' [a-zA-Z]('.'? [a-zA-Z0-9_]+)* '}'
 * Example = ${train.001.driver}
 * 
 * Parse until ending char is found.
 * Input string range can be greater than parsed string.
 * 
 * Returned type:
 *   - YY_SYMBOL_VARIABLE
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_INVALID_VARIABLE: Invalid string.
 */
static yy_retcode_e parse_variable(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    const char *ptr = begin;

    if (end - ptr < 4 || *ptr != '$' || *++ptr != '{') // no place for ${x}
        return YY_ERROR_INVALID_VARIABLE;

    switch (*++ptr) {
        case 'A' ... 'Z':
        case 'a' ... 'z': goto VARIABLE_NAME;
        default: return YY_ERROR_INVALID_VARIABLE;
    }

VARIABLE_NAME:

    if (++ptr == end)
        return YY_ERROR_INVALID_VARIABLE;

    switch (*ptr) {
        case '.': 
            if (ptr[-1] == '.') // two consecutive dot is not allowed
                return YY_ERROR_INVALID_VARIABLE;
        case '0' ... '9':
        case 'A' ... 'Z':
        case '_':
        case 'a' ... 'z': goto VARIABLE_NAME;
        case '}': goto VARIABLE_END;
        default: return YY_ERROR_INVALID_VARIABLE;
    }

VARIABLE_END:

    if (ptr[-1] == '.') // ending dot not allowed
        return YY_ERROR_INVALID_VARIABLE;

    ++ptr;

    assert(ptr - begin > 3);

    symbol->type = YY_SYMBOL_VARIABLE;
    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = ptr - begin;
    symbol->variable.ptr = begin + 2;
    symbol->variable.len = symbol->lexeme.len - 3;

    return YY_OK;
}

#define return_ok(symbol_, len_) do { \
    symbol->lexeme.ptr = begin; \
    symbol->lexeme.len = len_; \
    symbol->type = symbol_; \
    return YY_OK; \
} while(0)

#define return_error(err_) do { \
    return err_; \
} while(0)

/**
 * Search the identifier matching the given string
 * using the binary search algo on the identifiers list.
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] len Identifier length.
 * 
 * @return The identifier,
 *         NULL if not found.
 */
INLINE
static const yy_identifier_t * get_identifier(const char *str, size_t len)
{
    int imin = 0;
    int imax = NUM_IDENTIFIERS - 1;

    while (imax >= imin)
    {
        int i = imin + (imax-imin) / 2;

        int c = *str - identifiers[i].str[0];

        if (c == 0)
        {
            c = strncmp(str, identifiers[i].str, len);
            c = (c != 0 ? c : '\0' - identifiers[i].str[len]);
        }

        if (c == 0)
            return &identifiers[i];
        else if (c > 0)
            imin = i + 1;
        else
            imax = i - 1;
    }

    return NULL;
}

/**
 * Parse symbol.
 * 
 * Parse until an unrecognized char is found.
 * Input string range can be greater than parsed string.
 * 
 * This function is equivalent to function next() in Theodore Norvell doc.
 * 
 * Ambiguous symbols:
 *    '+' : returned as YY_SYMBOL_ADDITION_OP
 *    '-' : returned as YY_SYMBOL_SUBTRACTION_OP
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: success
 *          other: error.
 */
static yy_retcode_e parse_symbol(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin);
    assert(end);
    assert(begin <= end);
    assert(symbol);

    const char *ptr = begin;
    const yy_identifier_t *identifier;
    size_t len = 0;

    if (unlikely(begin == end))
        return_ok(YY_SYMBOL_END, 0);

    switch (*ptr) {
        case '!': goto NEXT_EXCLAMATION;
        case '"': goto NEXT_DOUBLE_QUOTE;
        case '$': goto NEXT_DOLLAR;
        case '%': return_ok(YY_SYMBOL_MODULO_OP, 1);
        case '&': goto NEXT_AMPERSAND;
        case '(': return_ok(YY_SYMBOL_PAREN_LEFT, 1);
        case ')': return_ok(YY_SYMBOL_PAREN_RIGHT, 1);
        case '*': return_ok(YY_SYMBOL_PRODUCT_OP, 1);
        case '+': return_ok(YY_SYMBOL_ADDITION_OP, 1);
        case ',': return_ok(YY_SYMBOL_COMMA, 1);
        case '-': return_ok(YY_SYMBOL_SUBTRACTION_OP, 1);
        case '/': return_ok(YY_SYMBOL_DIVIDE_OP, 1);
        case '0' ... '9': goto NEXT_DIGIT;
        case '<': goto NEXT_LESS;
        case '=': goto NEXT_EQUALS;
        case '>': goto NEXT_GREAT;
        case 'A' ... 'Z': goto NEXT_IDENTIFIER;
        case '^': return_ok(YY_SYMBOL_POWER_OP, 1);
        case 'a' ... 'z': goto NEXT_IDENTIFIER;
        case '|': goto NEXT_VERTICAL_BAR;
        default: return_error(YY_ERROR_SYNTAX);
    }

NEXT_EXCLAMATION: // !, !=

    if (unlikely(++ptr == end))
        return_ok(YY_SYMBOL_NOT_OP, 1);

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_DISTINCT_OP, 2);
        default: return_ok(YY_SYMBOL_NOT_OP, 1);
    }

NEXT_DOUBLE_QUOTE:

    return parse_quoted_string(ptr, end, symbol);

NEXT_DOLLAR:

    return parse_variable(ptr, end, symbol);

NEXT_AMPERSAND: // &&

    if (unlikely(++ptr == end))
        return_error(YY_ERROR_SYNTAX);

    switch (*ptr) {
        case '&': return_ok(YY_SYMBOL_AND_OP, 2);
        default: return_error(YY_ERROR_SYNTAX);
    }

NEXT_DIGIT:

    return parse_number(ptr, end, symbol);

NEXT_LESS: // <, <=

    if (unlikely(++ptr == end))
        return_ok(YY_SYMBOL_LESS_OP, 1);

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_LESS_EQUALS_OP, 2);
        default: return_ok(YY_SYMBOL_LESS_OP, 1);
    }

NEXT_EQUALS: // ==

    if (unlikely(++ptr == end))
        return_error(YY_ERROR_SYNTAX);

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_EQUALS_OP, 2);
        default: return_error(YY_ERROR_SYNTAX);
    }

NEXT_GREAT: // >, >=

    if (unlikely(++ptr == end))
        return_ok(YY_SYMBOL_GREAT_OP, 1);

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_GREAT_EQUALS_OP, 2);
        default: return_ok(YY_SYMBOL_GREAT_OP, 1);
    }

NEXT_IDENTIFIER:

    ++len;

    if (unlikely(++ptr == end))
        goto NEXT_IDENTIFIER_END;

    switch (*ptr) {
        case '0' ... '9':
        case 'A' ... 'Z':
        case '_':
        case 'a' ... 'z':  goto NEXT_IDENTIFIER;
        default: goto NEXT_IDENTIFIER_END;
    }

NEXT_IDENTIFIER_END:

    if ((identifier = get_identifier(begin, len)) != NULL)
        return_ok(identifier->type, len);

    return_error(YY_ERROR_SYNTAX);

NEXT_VERTICAL_BAR: // ||

    if (unlikely(++ptr == end))
        return_error(YY_ERROR_SYNTAX);

    switch (*ptr) {
        case '|': return_ok(YY_SYMBOL_OR_OP, 2);
        default: return_error(YY_ERROR_SYNTAX);
    }
}

/**
 * Skips initial spaces.
 * 
 * @param[in] begin String to parse.
 * @param[in] end One char after the string end.
 * 
 * @return Pointer to first non-space char or the end of the string.
 */
INLINE
const char * skip_spaces(const char *begin, const char *end)
{
    assert(begin && end && begin <= end);

SKIP_SPACES_START:

    if (unlikely(begin == end))
        return begin;

    switch (*begin)
    {
        case '\t': // tab
        case '\n': // new line
        case '\f': // form feed
        case '\v': // vertical tab
        case '\r': // carriage return
        case ' ':  // space
        case (char) 160:  // non-breaking space
            ++begin;
            goto SKIP_SPACES_START;
        default: break;
    }

    return begin;
}

INLINE
static bool is_numeric_operator(yy_symbol_e type)
{
    switch (type)
    {
        case YY_SYMBOL_PLUS_OP:
        case YY_SYMBOL_MINUS_OP:
        case YY_SYMBOL_ADDITION_OP:
        case YY_SYMBOL_SUBTRACTION_OP:
        case YY_SYMBOL_PRODUCT_OP:
        case YY_SYMBOL_DIVIDE_OP:
        case YY_SYMBOL_MODULO_OP:
        case YY_SYMBOL_POWER_OP:
            return true;
        default:
            return false;
    }
}

INLINE
static bool is_token_value(yy_token_e type)
{
    switch (type)
    {
        case YY_TOKEN_BOOL:
        case YY_TOKEN_NUMBER:
        case YY_TOKEN_DATETIME:
        case YY_TOKEN_STRING:
        case YY_TOKEN_VARIABLE:
            return true;
        default:
            return false;
    }
}

INLINE
static bool is_token_fixed_value(yy_token_e type)
{
    switch (type)
    {
        case YY_TOKEN_BOOL:
        case YY_TOKEN_NUMBER:
        case YY_TOKEN_DATETIME:
        case YY_TOKEN_STRING:
            return true;
        default:
            return false;
    }
}

INLINE
static bool is_token_operator(yy_token_t token)
{
    // an operator is a function with precedence
    return (token.type == YY_TOKEN_FUNCTION && token.function.precedence > 0);
}

INLINE
static bool is_token_regfunc(yy_token_t token)
{
    // a regular function is a function without precedence
    return (token.type == YY_TOKEN_FUNCTION && token.function.precedence == 0);
}

INLINE
static yy_token_t create_token(const yy_symbol_t *symbol)
{
    yy_token_t token = symbol_to_token[symbol->type];

    switch (symbol->type)
    {
        case YY_SYMBOL_NUMBER_VAL:
        case YY_SYMBOL_DATETIME_VAL:
        case YY_SYMBOL_STRING_VAL:
        case YY_SYMBOL_VARIABLE:
            token.str_val = symbol->str_val;
            break;
        default:
            break;
    }

    return token;
}

INLINE
static yy_token_t * top(yy_stack_t *stack)
{
    if (stack->len == 0)
        return NULL;

    return (&stack->data[stack->len - 1]);
}

INLINE
static yy_token_t * get(yy_stack_t *stack, uint32_t idx)
{
    if (stack->len <= idx)
        return NULL;

    return (&stack->data[stack->len - 1 - idx]);
}

INLINE
static void pop(yy_stack_t *stack)
{
    assert(stack->len != 0);
    stack->len--;
}

/**
 * Try to simplify tokens on the top of the RPN stack.
 * 
 * Supported simplifications:
 *   - Remove plus operator (ex: +1 -> 1)
 *   - Eval top function if it has fixed values (ex: 1+1 -> 2)
 * 
 * Unsupported simplifications:
 *   - For commutative operators, move fixed values to top (ex: 1+$a+3 -> $a+1+3 -> $a+4)
 * 
 * @param stack Stack to simplify.
 * 
 * @return true = simplified, false = not simplified.
 */
INLINE
static bool simplify_stack(yy_stack_t *stack)
{
    assert(stack);

    yy_token_t *token0 = top(stack);

    if (!token0 || token0->type != YY_TOKEN_FUNCTION)
        return false;

    if (stack->len < token0->function.num_args)
        return false;

    if (token0->function.num_args == 0)
        return false;

    if ((yy_func_1) token0->function.ptr == func_ident)
        return true;

    yy_token_t *token1 = get(stack, 1);

    if (!token1 || !is_token_fixed_value(token1->type))
        return false;

    if (token0->function.num_args == 1) {
        *token1 = ((yy_func_1) token0->function.ptr)(*token1);
        pop(stack);
        return true;
    }

    yy_token_t *token2 = get(stack, 2);

    if (!token2 || !is_token_fixed_value(token2->type))
        return false;

    if (token0->function.num_args == 2) {
        *token2 = ((yy_func_2) token0->function.ptr)(*token2, *token1);
        pop(stack);
        pop(stack);
        return true;
    }

    yy_token_t *token3 = get(stack, 3);

    if (!token3 || !is_token_fixed_value(token3->type))
        return false;

    if (token0->function.num_args == 3) {
        *token3 = ((yy_func_3) token0->function.ptr)(*token3, *token2, *token1);
        pop(stack);
        pop(stack);
        pop(stack);
        return true;
    }

    assert(false);
    return false;
}

static void push_to_stack(yy_parser_t *parser, const yy_token_t *token)
{
    assert(token->type != YY_TOKEN_NULL);

    yy_stack_t *stack = parser->stack;

    if (unlikely(stack->len >= stack->reserved)) {
        assert(stack->len == stack->reserved);
        parser->error = YY_ERROR_MEM;
        return;
    }

    stack->data[stack->len++] = *token;

    if (token->type == YY_TOKEN_FUNCTION)
        simplify_stack(stack);
}

static void push_to_operators(yy_parser_t *parser, const yy_token_t *token)
{
    yy_stack_t *stack = parser->operators;

    if (unlikely(stack->len >= stack->reserved)) {
        assert(stack->len == stack->reserved);
        parser->error = YY_ERROR_MEM;
        return;
    }

    stack->data[stack->len++] = *token;
}

/**
 * Process current symbol using the Shunting Yard algorithm.
 * 
 * Recursive descent parser grants adherence to grammar, except in 
 * the END case (ex: the invalid entry '(1' flush an END after the 
 * subexpression '1' with an unmatched ')'.
 * 
 * We use YY_TOKEN_NULL in operators stack as grouping sentinel '('.
 * 
 * @see https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 * 
 * @param[in] parser Parser to update.
 */
static void process_current_symbol(yy_parser_t *parser)
{
    if (parser->error != YY_OK)
        return;

    const yy_token_t *op = NULL;
    yy_token_t token = create_token(&parser->curr_symbol);
    yy_symbol_e type = parser->curr_symbol.type;

    if (is_token_value(token.type))
    {
        push_to_stack(parser, &token);
        return;
    }

    if (is_token_operator(token))
    {
        while ((op = top(parser->operators)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;

            if (token.function.precedence < op->function.precedence)
                break;

            if (token.function.precedence == op->function.precedence && token.function.right_to_left)
                break;

            push_to_stack(parser, op);
            pop(parser->operators);
        }

        push_to_operators(parser, &token);
        return;
    }

    if (is_token_regfunc(token))
    {
        push_to_operators(parser, &token);
        return;
    }

    if (type == YY_SYMBOL_PAREN_LEFT)
    {
        push_to_operators(parser, &token);
        return;
    }

    if (type == YY_SYMBOL_PAREN_RIGHT)
    {
        while ((op = top(parser->operators)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;
            
            push_to_stack(parser, op);
            pop(parser->operators);
        }

        assert(op != NULL && op->type == YY_TOKEN_NULL);
        pop(parser->operators);

        op = top(parser->operators);

        if (op && is_token_regfunc(*op)) {
            push_to_stack(parser, op);
            pop(parser->operators);
        }

        return;
    }

    if (type == YY_SYMBOL_COMMA)
    {
        while ((op = top(parser->operators)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;

            push_to_stack(parser, op);
            pop(parser->operators);
        }

        return;
    }

    if (type == YY_SYMBOL_END)
    {
        while ((op = top(parser->operators)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL) {
                // unmatched parentesis
                parser->error = YY_ERROR_SYNTAX;
                return;
            }

            push_to_stack(parser, op);
            pop(parser->operators);
        }

        return;
    }

    assert(false);
}

/**
 * Read next symbol and updates parser state accordingly.
 * 
 * @param[in] parser Parser to update.
 * 
 * @return true = success, false = error.
 */
static bool consume(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return false;

    if (parser->curr_symbol.type != YY_SYMBOL_NONE)
    {
        // current symbol was accepted

        // append current symbol to output (rpn stack)
        process_current_symbol(parser);

        if (parser->error != YY_OK)
            return false;

        // moving current position just after the accepted symbol
        parser->curr += parser->curr_symbol.lexeme.len;

    }

    assert(parser->begin <= parser->curr);
    assert(parser->curr <= parser->end);

    if (parser->curr_symbol.type == YY_SYMBOL_END)
        return true;

    parser->prev_symbol = parser->curr_symbol;
    parser->curr = skip_spaces(parser->curr, parser->end);
    parser->error = parse_symbol(parser->curr, parser->end, &parser->curr_symbol);

    return (parser->error == YY_OK);
}

/**
 * Checks that current symbol is the expected one and moves to next symbol.
 * 
 * @param[in] parser Parser to update.
 * @param[in] type Expected symbol.
 * 
 * @return true = success, 
 *         false = unexpected symbol or 
 *                 error processing current symbol or 
 *                 error reading next symbol.
 */
static bool expect(yy_parser_t *parser, yy_symbol_e type)
{
    assert(parser);

    if (parser->error != YY_OK)
        return false;

    if (parser->curr_symbol.type != type) {
        parser->error = YY_ERROR_SYNTAX;
        return false;
    }

    return consume(parser);
}

static void init_parser(yy_parser_t *parser, const char *begin, const char *end, yy_stack_t *stack)
{
    // TODO: remove this temporary static shit
    static yy_token_t data[64] = {0};
    static yy_stack_t operators = {data, sizeof(data)/sizeof(data[0]), 0};

    assert(parser);

    stack->len = 0;
    operators.len = 0;

    parser->begin = begin;
    parser->end = end;
    parser->curr = begin;
    parser->stack = stack;
    parser->operators = &operators;
    parser->curr_symbol = (yy_symbol_t){0};
    parser->prev_symbol = (yy_symbol_t){0};
    parser->error = (stack && stack->reserved ? YY_OK : YY_ERROR_MEM);

    consume(parser);
}

/**
 * Parse a numeric term.
 * 
 * @see Numeric expression grammar (in comments).
 * 
 * @param[in] parser Parser to update.
 * 
 * @return true = success, false = error.
 */
static bool parse_term_number(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return false;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_CONST_E:
        case YY_SYMBOL_CONST_PI:
        case YY_SYMBOL_NUMBER_VAL:
        case YY_SYMBOL_VARIABLE:
            consume(parser);
            break;
        case YY_SYMBOL_ABS:
        case YY_SYMBOL_SQRT:
        case YY_SYMBOL_SIN:
        case YY_SYMBOL_COS:
        case YY_SYMBOL_TAN:
        case YY_SYMBOL_EXP:
        case YY_SYMBOL_LOG:
        case YY_SYMBOL_CEIL:
        case YY_SYMBOL_FLOOR:
        case YY_SYMBOL_TRUNC:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_MAX:
        case YY_SYMBOL_MIN:
        case YY_SYMBOL_MODULO:
        case YY_SYMBOL_POWER:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_PAREN_LEFT:
            consume(parser);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_ADDITION_OP:
            if (is_numeric_operator(parser->prev_symbol.type)) {
                parser->error = YY_ERROR_SYNTAX;
                break;
            }
            parser->curr_symbol.type = YY_SYMBOL_PLUS_OP;
            consume(parser);
            parse_expr_number(parser);
            break;
        case YY_SYMBOL_SUBTRACTION_OP:
            if (is_numeric_operator(parser->prev_symbol.type)) {
                parser->error = YY_ERROR_SYNTAX;
                break;
            }
            parser->curr_symbol.type = YY_SYMBOL_MINUS_OP;
            consume(parser);
            parse_expr_number(parser);
            break;
        default:
            parser->error = YY_ERROR_SYNTAX;
    }

    return (parser->error == YY_OK);
}

/**
 * Parse a numeric expression until an unrecognized symbol is found.
 * 
 * @param[in] Parser object.
 * 
 * @return true=success, false=error.
 */
static bool parse_expr_number(yy_parser_t *parser)
{ 
    assert(parser);

NUMBER_EXPR_START:

    if (parser->error != YY_OK)
        return false;

    if (!parse_term_number(parser))
        return false;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_ADDITION_OP: 
        case YY_SYMBOL_SUBTRACTION_OP: 
        case YY_SYMBOL_PRODUCT_OP: 
        case YY_SYMBOL_DIVIDE_OP: 
        case YY_SYMBOL_MODULO_OP: 
        case YY_SYMBOL_POWER_OP: 
            consume(parser);
            goto NUMBER_EXPR_START;
        case YY_SYMBOL_END: 
            consume(parser);
            goto NUMBER_EXPR_END;
        default:
            goto NUMBER_EXPR_END;
    }

NUMBER_EXPR_END:

    // TODO: flush auxiliar stack

    return true;
}

/**
 * Implements a recursive descent parser.
 * 
 * Grammar:
 * 
 *    numExpr       = numTerm (numInfixOp numTerm)*
 *    numTerm       = numValue | numFunc | "(" numExpr ")" | numPrefixOp numTerm
 *    numInfixOp    = "+" | "-" | "*" | "/" | "^" | "%"
 *    numPrefixOp   = "+" | "-"
 *    numFunc       = numFunc1 | numFunc2
 *    numFunc1      = ("abs" | "sqrt" | "ceil" | "trunc" | "floor" | "sin" | "cos" | "tan" | "exp" | "log") "(" numExpr ")"
 *    numFunc2      = ("min" | "max" | "pow" | "mod") "(" numExpr "," numExpr ")"
 *    numValue      = numConst | numVal | numVar
 * 
 * Exceptions:
 * 
 *    We reject 2 consecutive operators (ex: 1 + -1).
 */
yy_retcode_e yy_parse_expr_number(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR_ARGS;

    yy_parser_t parser;

    init_parser(&parser, begin, end, stack);
    parse_expr_number(&parser);

    if (parser.error == YY_OK && parser.curr_symbol.type != YY_SYMBOL_END)
        parser.error = YY_ERROR_SYNTAX;

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

// ==================================================

#undef return_error
#define return_error(err_) return (yy_token_t){ .error = err_, .type = YY_TOKEN_ERROR };
#define return_number(val_) return (yy_token_t){ .number_val = val_, .type = YY_TOKEN_NUMBER };
#define return_datetime(val_) return (yy_token_t){ .datetime_val = val_, .type = YY_TOKEN_DATETIME };
#define UNUSED(x) (void)(x)

static yy_token_t func_now(void) {
    return (yy_token_t){0};
}

static yy_token_t func_datepart(yy_token_t date, yy_token_t part) {
    UNUSED(date);
    UNUSED(part);
    return (yy_token_t){0};
}

static yy_token_t func_dateadd(yy_token_t date, yy_token_t part, yy_token_t value) {
    UNUSED(date);
    UNUSED(part);
    UNUSED(value);
    return (yy_token_t){0};
}

static yy_token_t func_trim(yy_token_t str) {
    UNUSED(str);
    return (yy_token_t){0};
}

static yy_token_t func_lower(yy_token_t str) {
    UNUSED(str);
    return (yy_token_t){0};
}

static yy_token_t func_upper(yy_token_t str) {
    UNUSED(str);
    return (yy_token_t){0};
}

static yy_token_t func_length(yy_token_t str) {
    UNUSED(str);
    return (yy_token_t){0};
}

static yy_token_t func_concat(yy_token_t str1, yy_token_t str2) {
    UNUSED(str1);
    UNUSED(str2);
    return (yy_token_t){0};
}

static yy_token_t func_substr(yy_token_t str, yy_token_t start, yy_token_t len) {
    UNUSED(str);
    UNUSED(start);
    UNUSED(len);
    return (yy_token_t){0};
}

static yy_token_t func_abs(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = fabs(x.number_val);

    return x;
}

static yy_token_t func_ceil(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = ceil(x.number_val);

    return x;
}

static yy_token_t func_floor(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = floor(x.number_val);

    return x;
}

static yy_token_t func_trunc(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = trunc(x.number_val);

    return x;
}

static yy_token_t func_sin(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = sin(x.number_val);

    return x;
}

static yy_token_t func_cos(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = cos(x.number_val);

    return x;
}

static yy_token_t func_tan(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = tan(x.number_val);

    return x;
}

static yy_token_t func_exp(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = exp(x.number_val);

    return x;
}

static yy_token_t func_log(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = log(x.number_val);

    return x;
}

static yy_token_t func_max(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
    {
        double val = fmax(x.number_val, y.number_val);
        return_number(val);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        uint64_t val = MAX(x.datetime_val, y.datetime_val);
        return_datetime(val);
    }

    return_error(YY_ERROR_VALUE);
}

static yy_token_t func_min(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
    {
        double val = fmin(x.number_val, y.number_val);
        return_number(val);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        uint64_t val = MIN(x.datetime_val, y.datetime_val);
        return_datetime(val);
    }

    return_error(YY_ERROR_VALUE);
}

static yy_token_t func_sqrt(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val = sqrt(x.number_val);

    return x;
}

static yy_token_t func_pow(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = pow(x.number_val, y.number_val);

    return_number(val);
}

static yy_token_t func_minus(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    x.number_val *= -1;

    return x;
}

static yy_token_t func_ident(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    return x;
}

static yy_token_t func_mult(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = x.number_val * y.number_val;

    return_number(val);
}

static yy_token_t func_div(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = x.number_val / y.number_val;

    return_number(val);
}

static yy_token_t func_mod(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = fmod(x.number_val, y.number_val);

    return_number(val);
}

static yy_token_t func_addition(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = x.number_val + y.number_val;

    return_number(val);
}

static yy_token_t func_subtraction(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_NUMBER)
        return_error(YY_ERROR_VALUE);

    double val = x.number_val - y.number_val;

    return_number(val);
}

static yy_token_t func_not(yy_token_t x) {
    UNUSED(x);
    return (yy_token_t){0};
}

static yy_token_t func_lt(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_le(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_gt(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_ge(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_eq(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_ne(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_and(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}

static yy_token_t func_or(yy_token_t x, yy_token_t y) {
    UNUSED(x);
    UNUSED(y);
    return (yy_token_t){0};
}
