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
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "expr.h"

/**
 * @see https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
 * @see https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 * @see https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B
 * 
 * Guidelines:
 *   - yy_xxx() functions are public (declared in header), rest are private (static)
 *   - read_symbol_xxx() functions converts text to symbol
 *   - parse_xxx() functions are the recursive descent parser functions
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

#if defined __has_attribute
    #if __has_attribute(__fallthrough__)
        # define fallthrough   __attribute__((__fallthrough__))
    #endif
#endif
#ifndef fallthrough
    # define fallthrough   do {} while (0)  /* fallthrough */
#endif

#ifndef NAN
    #define NAN (0.0/0.0)
#endif

#ifndef INFINITY
    #define INFINITY (1.0 /0.0)
#endif

#define MIN(a, b)       (((a)<(b))?(a):(b))
#define MAX(a, b)       (((a)>(b))?(a):(b))
#define CLAMP(x, a, b)  (((x)<(a))?(a):(((b)<(x))?(b):(x)))

#ifndef M_E
    #define M_E     2.7182818284590452354
#endif

#ifndef M_PI
    #define M_PI    3.14159265358979323846
#endif

#define make_string(ptr_, len_)    (yy_str_t){.ptr = (ptr_), .len = (uint32_t)(len_)}
#define token_error(err_)          (yy_token_t){ .error = (err_)                    , .type = YY_TOKEN_ERROR    }
#define token_bool(val_)           (yy_token_t){ .bool_val = (val_)                 , .type = YY_TOKEN_BOOL     }
#define token_number(val_)         (yy_token_t){ .number_val = (val_)               , .type = YY_TOKEN_NUMBER   }
#define token_datetime(val_)       (yy_token_t){ .datetime_val = (val_)             , .type = YY_TOKEN_DATETIME }
#define token_string(ptr_, len_)   (yy_token_t){ .str_val = make_string(ptr_, len_) , .type = YY_TOKEN_STRING   }
#define token_variable(ptr_, len_) (yy_token_t){ .str_val = make_string(ptr_, len_) , .type = YY_TOKEN_VARIABLE }

typedef enum yy_symbol_e
{
    YY_SYMBOL_NONE,                 //!< Unassigned symbol.
    YY_SYMBOL_TRUE,                 //!< true, True, TRUE
    YY_SYMBOL_FALSE,                //!< false, False, FALSE
    YY_SYMBOL_NUMBER_VAL,           //!< 1234, 3.14159, 0.24e-4
    YY_SYMBOL_DATETIME_VAL,         //!< 2024-07-24T07:57:14.494Z
    YY_SYMBOL_STRING_VAL,           //!< "s3cr3t"
    YY_SYMBOL_ESCAPED_STRING_VAL,   //!< "ends with new-line: \n"
    YY_SYMBOL_VARIABLE,             //!< ${x}
    YY_SYMBOL_CONST_E,              //!< E
    YY_SYMBOL_CONST_PI,             //!< PI
    YY_SYMBOL_CONST_INF,            //!< Inf
    YY_SYMBOL_CONST_NAN,            //!< NaN
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
    YY_SYMBOL_CLAMP,                //!< clamp
    YY_SYMBOL_NOW,                  //!< now
    YY_SYMBOL_NOT,                  //!< not
    YY_SYMBOL_ISINF,                //!< isinf
    YY_SYMBOL_ISNAN,                //!< isnan
    YY_SYMBOL_ISERROR,              //!< iserror
    YY_SYMBOL_DATEPART,             //!< datepart
    YY_SYMBOL_DATEADD,              //!< dateadd
    YY_SYMBOL_DATESET,              //!< dateset
    YY_SYMBOL_DATETRUNC,            //!< datetrunc
    YY_SYMBOL_LENGTH,               //!< length
    YY_SYMBOL_FIND,                 //!< find
    YY_SYMBOL_LOWER,                //!< lower
    YY_SYMBOL_UPPER,                //!< upper
    YY_SYMBOL_TRIM,                 //!< trim
    YY_SYMBOL_CONCAT_OP,            //!< concat
    YY_SYMBOL_SUBSTR,               //!< substr
    YY_SYMBOL_REPLACE,              //!< replace
    YY_SYMBOL_UNESCAPE,             //!< unescape
    YY_SYMBOL_IFELSE,               //!< ifelse
    YY_SYMBOL_STR,                  //!< str
    YY_SYMBOL_VARIABLE_FUNC,        //!< variable
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
    uint32_t operators_len;         //!< Length of the operators stack.
    yy_symbol_t curr_symbol;        //!< Current symbol.
    yy_symbol_t prev_symbol;        //!< Previous symbol.
    yy_error_e error;               //!< Error code (YY_OK means no error), curr points to error location.
} yy_parser_t;

typedef struct yy_identifier_t
{
    const char *str;
    yy_symbol_e type;
} yy_identifier_t;

typedef struct yy_eval_ctx_t
{
    yy_stack_t *stack;              //!< Stack values.
    char *tmp_str;                  //!< Temporary memory used for intermediate strings.
} yy_eval_ctx_t;

// types for pure functions
typedef yy_token_t (*yy_func_0)(void);
typedef yy_token_t (*yy_func_1)(yy_token_t);
typedef yy_token_t (*yy_func_2)(yy_token_t, yy_token_t);
typedef yy_token_t (*yy_func_3)(yy_token_t, yy_token_t, yy_token_t);

// types for impure functions
typedef yy_token_t (*yy_func_0_x)(yy_eval_ctx_t *);
typedef yy_token_t (*yy_func_1_x)(yy_token_t, yy_eval_ctx_t *);
typedef yy_token_t (*yy_func_2_x)(yy_token_t, yy_token_t, yy_eval_ctx_t *);
typedef yy_token_t (*yy_func_3_x)(yy_token_t, yy_token_t, yy_token_t, yy_eval_ctx_t *);

// Days in month
static const int days_in_month[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Forward declarations
static bool is_temp_ptr(yy_eval_ctx_t *ctx, const char *ptr);
static void free_str(yy_eval_ctx_t *ctx, yy_str_t *str);
static yy_token_e parse_expr_generic(yy_parser_t *parser, bool check_bool);
static void parse_expr_datetime(yy_parser_t *parser);
static void parse_expr_number(yy_parser_t *parser);
static void parse_expr_string(yy_parser_t *parser);
static void parse_expr_bool(yy_parser_t *parser);

// Functions in expressions
static yy_token_t func_now(yy_eval_ctx_t *ctx);
static yy_token_t func_datepart(yy_token_t date, yy_token_t part);
static yy_token_t func_dateadd(yy_token_t date, yy_token_t value, yy_token_t part);
static yy_token_t func_dateset(yy_token_t date, yy_token_t value, yy_token_t part);
static yy_token_t func_datetrunc(yy_token_t date, yy_token_t part);
static yy_token_t func_str(yy_token_t str, yy_eval_ctx_t *ctx);
static yy_token_t func_unescape(yy_token_t str, yy_eval_ctx_t *ctx);
static yy_token_t func_trim(yy_token_t str, yy_eval_ctx_t *ctx);
static yy_token_t func_lower(yy_token_t str, yy_eval_ctx_t *ctx);
static yy_token_t func_upper(yy_token_t str, yy_eval_ctx_t *ctx);
static yy_token_t func_concat(yy_token_t str1, yy_token_t str2, yy_eval_ctx_t *ctx);
static yy_token_t func_substr(yy_token_t str, yy_token_t start, yy_token_t len, yy_eval_ctx_t *ctx);
static yy_token_t func_replace(yy_token_t str, yy_token_t old_str, yy_token_t new_str, yy_eval_ctx_t *ctx);
static yy_token_t func_length(yy_token_t str);
static yy_token_t func_find(yy_token_t needle, yy_token_t haystack, yy_token_t pos);
static yy_token_t func_clamp(yy_token_t x, yy_token_t vmin, yy_token_t vmax);
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
static yy_token_t func_addition(yy_token_t x, yy_token_t y);
static yy_token_t func_subtraction(yy_token_t x, yy_token_t y);
static yy_token_t func_mult(yy_token_t x, yy_token_t y);
static yy_token_t func_div(yy_token_t x, yy_token_t y);
static yy_token_t func_mod(yy_token_t x, yy_token_t y);
static yy_token_t func_iserror(yy_token_t x);
static yy_token_t func_isinf(yy_token_t x);
static yy_token_t func_isnan(yy_token_t x);
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
static yy_token_t func_ifelse(yy_token_t cond, yy_token_t x, yy_token_t y);
static yy_token_t func_variable(yy_token_t str);

// Identifiers list (alphabetical order)
static const yy_identifier_t identifiers[] =
{
    { "E",         YY_SYMBOL_CONST_E       },
    { "FALSE",     YY_SYMBOL_FALSE         },
    { "False",     YY_SYMBOL_FALSE         },
    { "Inf",       YY_SYMBOL_CONST_INF     },
    { "NaN",       YY_SYMBOL_CONST_NAN     },
    { "PI",        YY_SYMBOL_CONST_PI      },
    { "TRUE",      YY_SYMBOL_TRUE          },
    { "True",      YY_SYMBOL_TRUE          },
    { "abs",       YY_SYMBOL_ABS           },
    { "ceil",      YY_SYMBOL_CEIL          },
    { "clamp",     YY_SYMBOL_CLAMP         },
    { "cos",       YY_SYMBOL_COS           },
    { "dateadd",   YY_SYMBOL_DATEADD       },
    { "datepart",  YY_SYMBOL_DATEPART      },
    { "dateset",   YY_SYMBOL_DATESET       },
    { "datetrunc", YY_SYMBOL_DATETRUNC     },
    { "exp",       YY_SYMBOL_EXP           },
    { "false",     YY_SYMBOL_FALSE         },
    { "find",      YY_SYMBOL_FIND          },
    { "floor",     YY_SYMBOL_FLOOR         },
    { "ifelse",    YY_SYMBOL_IFELSE        },
    { "iserror",   YY_SYMBOL_ISERROR       },
    { "isinf",     YY_SYMBOL_ISINF         },
    { "isnan",     YY_SYMBOL_ISNAN         },
    { "length",    YY_SYMBOL_LENGTH        },
    { "log",       YY_SYMBOL_LOG           },
    { "lower",     YY_SYMBOL_LOWER         },
    { "max",       YY_SYMBOL_MAX           },
    { "min",       YY_SYMBOL_MIN           },
    { "mod",       YY_SYMBOL_MODULO        },
    { "not",       YY_SYMBOL_NOT           },
    { "now",       YY_SYMBOL_NOW           },
    { "pow",       YY_SYMBOL_POWER         },
    { "replace",   YY_SYMBOL_REPLACE       },
    { "sin",       YY_SYMBOL_SIN           },
    { "sqrt",      YY_SYMBOL_SQRT          },
    { "str",       YY_SYMBOL_STR           },
    { "substr",    YY_SYMBOL_SUBSTR        },
    { "tan",       YY_SYMBOL_TAN           },
    { "trim",      YY_SYMBOL_TRIM          },
    { "true",      YY_SYMBOL_TRUE          },
    { "trunc",     YY_SYMBOL_TRUNC         },
    { "unescape",  YY_SYMBOL_UNESCAPE      },
    { "upper",     YY_SYMBOL_UPPER         },
    { "variable",  YY_SYMBOL_VARIABLE_FUNC },
};

#define NUM_IDENTIFIERS (sizeof(identifiers)/sizeof(identifiers[0]))

#define make_func(func_, args_, ...) (yy_func_t){ .ptr = (void (*)(void)) func_, .num_args = args_, __VA_ARGS__ }

static const yy_token_t symbol_to_token[] =
{
    [YY_SYMBOL_NUMBER_VAL]         = { .type = YY_TOKEN_NUMBER   }, 
    [YY_SYMBOL_DATETIME_VAL]       = { .type = YY_TOKEN_DATETIME }, 
    [YY_SYMBOL_STRING_VAL]         = { .type = YY_TOKEN_STRING   }, 
    [YY_SYMBOL_ESCAPED_STRING_VAL] = { .type = YY_TOKEN_STRING   }, 
    [YY_SYMBOL_VARIABLE]           = { .type = YY_TOKEN_VARIABLE }, 

    [YY_SYMBOL_TRUE]            = { .type = YY_TOKEN_BOOL    , .bool_val = true       },
    [YY_SYMBOL_FALSE]           = { .type = YY_TOKEN_BOOL    , .bool_val = false      },
    [YY_SYMBOL_CONST_E]         = { .type = YY_TOKEN_NUMBER  , .number_val = M_E      },
    [YY_SYMBOL_CONST_PI]        = { .type = YY_TOKEN_NUMBER  , .number_val = M_PI     },
    [YY_SYMBOL_CONST_INF]       = { .type = YY_TOKEN_NUMBER  , .number_val = INFINITY },
    [YY_SYMBOL_CONST_NAN]       = { .type = YY_TOKEN_NUMBER  , .number_val = NAN      },

    // [YY_SYMBOL_PAREN_LEFT]   = { .type = YY_TOKEN_NULL     },
    // [YY_SYMBOL_PAREN_RIGHT]  = { .type = YY_TOKEN_NULL     },
    // [YY_SYMBOL_COMMA]        = { .type = YY_TOKEN_NULL     },

    [YY_SYMBOL_POWER_OP]        = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_pow        , 2, .precedence = 2) },
    [YY_SYMBOL_MINUS_OP]        = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_minus      , 1, .precedence = 3, .right_to_left = true) },
    [YY_SYMBOL_PLUS_OP]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_ident      , 1, .precedence = 3, .right_to_left = true) },
    [YY_SYMBOL_PRODUCT_OP]      = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_mult       , 2, .precedence = 4) },
    [YY_SYMBOL_DIVIDE_OP]       = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_div        , 2, .precedence = 4) },
    [YY_SYMBOL_MODULO_OP]       = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_mod        , 2, .precedence = 4) },
    [YY_SYMBOL_ADDITION_OP]     = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_addition   , 2, .precedence = 5) },
    [YY_SYMBOL_SUBTRACTION_OP]  = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_subtraction, 2, .precedence = 5) },
    [YY_SYMBOL_LESS_OP]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_lt         , 2, .precedence = 6) },
    [YY_SYMBOL_LESS_EQUALS_OP]  = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_le         , 2, .precedence = 6) },
    [YY_SYMBOL_GREAT_OP]        = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_gt         , 2, .precedence = 6) },
    [YY_SYMBOL_GREAT_EQUALS_OP] = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_ge         , 2, .precedence = 6) },
    [YY_SYMBOL_EQUALS_OP]       = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_eq         , 2, .precedence = 7) },
    [YY_SYMBOL_DISTINCT_OP]     = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_ne         , 2, .precedence = 7) },
    [YY_SYMBOL_AND_OP]          = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_and        , 2, .precedence = 8) },
    [YY_SYMBOL_OR_OP]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_or         , 2, .precedence = 9) },
    [YY_SYMBOL_NOT]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_not        , 1) },
    [YY_SYMBOL_ISINF]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_isinf      , 1) },
    [YY_SYMBOL_ISNAN]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_isnan      , 1) },
    [YY_SYMBOL_ISERROR]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_iserror    , 1) },
    [YY_SYMBOL_ABS]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_abs        , 1) },
    [YY_SYMBOL_MODULO]          = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_mod        , 2) },
    [YY_SYMBOL_POWER]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_pow        , 2) },
    [YY_SYMBOL_SQRT]            = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_sqrt       , 1) },
    [YY_SYMBOL_SIN]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_sin        , 1) },
    [YY_SYMBOL_COS]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_cos        , 1) },
    [YY_SYMBOL_TAN]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_tan        , 1) },
    [YY_SYMBOL_EXP]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_exp        , 1) },
    [YY_SYMBOL_LOG]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_log        , 1) },
    [YY_SYMBOL_TRUNC]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_trunc      , 1) },
    [YY_SYMBOL_CEIL]            = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_ceil       , 1) },
    [YY_SYMBOL_FLOOR]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_floor      , 1) },
    [YY_SYMBOL_CLAMP]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_clamp      , 3) },
    [YY_SYMBOL_NOW]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_now        , 0, .is_not_pure = true) },
    [YY_SYMBOL_DATEPART]        = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_datepart   , 2) },
    [YY_SYMBOL_DATEADD]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_dateadd    , 3) },
    [YY_SYMBOL_DATESET]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_dateset    , 3) },
    [YY_SYMBOL_DATETRUNC]       = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_datetrunc  , 2) },
    [YY_SYMBOL_LENGTH]          = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_length     , 1) },
    [YY_SYMBOL_FIND]            = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_find       , 3) },
    [YY_SYMBOL_STR]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_str        , 1, .is_not_pure = true) },
    [YY_SYMBOL_LOWER]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_lower      , 1, .is_not_pure = true) },
    [YY_SYMBOL_UPPER]           = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_upper      , 1, .is_not_pure = true) },
    [YY_SYMBOL_TRIM]            = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_trim       , 1, .is_not_pure = true) },
    [YY_SYMBOL_CONCAT_OP]       = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_concat     , 2, .precedence = 5, .is_not_pure = true) },
    [YY_SYMBOL_SUBSTR]          = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_substr     , 3, .is_not_pure = true) },
    [YY_SYMBOL_REPLACE]         = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_replace    , 3, .is_not_pure = true) },
    [YY_SYMBOL_UNESCAPE]        = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_unescape   , 1, .is_not_pure = true) },
    [YY_SYMBOL_MIN]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_min        , 2) },
    [YY_SYMBOL_MAX]             = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_max        , 2) },
    [YY_SYMBOL_IFELSE]          = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_ifelse     , 3) },
    [YY_SYMBOL_VARIABLE_FUNC]   = { .type = YY_TOKEN_FUNCTION, .function = make_func(func_variable   , 1) },
    [YY_SYMBOL_END]             = { .type = YY_TOKEN_NULL }
};

static const char *date_parts[] = {
    "year",     // 0
    "month",    // 1
    "day",      // 2
    "hour",     // 3
    "minute",   // 4
    "second",   // 5
    "millis"    // 6
};

#define NUM_DATEPARTS (sizeof(date_parts)/sizeof(date_parts[0]))

// Check if year is a leap year
static bool is_leap_year(int year) {
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

INLINE
bool is_blocking_error(yy_error_e err)
{
    switch (err) {
        case YY_ERROR:
        case YY_ERROR_CREF:
        case YY_ERROR_MEM:
        case YY_ERROR_EVAL:
        case YY_ERROR_SYNTAX:
            return true;
        default:
            return false;
    }
}

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
 * Search the datepart identifier corresponding  to the given string.
 * 
 * @param[in] str String to match.
 * 
 * @return The identifier (0 = year, 1 = month, ..., 6 = millis),
 *         -1 if not found.
 */
INLINE
static int get_datepart(const yy_str_t *str)
{
    for (size_t i = 0; i < NUM_DATEPARTS; i++)
        if (strncmp(str->ptr, date_parts[i], str->len) == 0 && date_parts[i][str->len] == '\0')
            return i;

    return -1;
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
static const char * skip_spaces(const char *begin, const char *end)
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
 *          YY_ERROR_SYNTAX: Invalid number.
 *          YY_ERROR_VALUE: Number out of range.
 */
static yy_error_e read_symbol_number(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    char buf[128];
    size_t len = 0;
    int64_t int_val = 0;
    double number_val;
    const char *ptr = begin;

    if (unlikely(ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '0': 
            goto NUMBER_INTEGER_PART_0;
        case '1' ... '9': goto NUMBER_INTEGER_PART_NON_0;
        default: return YY_ERROR_SYNTAX;
    }

NUMBER_INTEGER_PART_0:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_INTEGER;

    switch (*ptr) {
        case '0' ... '9': return YY_ERROR_SYNTAX;
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
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '0' ... '9': goto NUMBER_FRACTION_CONT;
        default: return YY_ERROR_SYNTAX;
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
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '+':
        case '-': goto NUMBER_EXPONENT_NUM_START;
        case '0': goto NUMBER_EXPONENT_0;
        case '1' ... '9': goto NUMBER_EXPONENT_NUM_CONT;
        default: return YY_ERROR_SYNTAX;
    }

NUMBER_EXPONENT_NUM_START:

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '0': goto NUMBER_EXPONENT_0;
        case '1' ... '9': goto NUMBER_EXPONENT_NUM_CONT;
        default: return YY_ERROR_SYNTAX;
    }

NUMBER_EXPONENT_0:

    if (unlikely(++ptr == end))
        goto NUMBER_READ_FLOAT;

    switch (*ptr) {
        case '1' ... '9': return YY_ERROR_SYNTAX;
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
        return YY_ERROR_VALUE;

    for (const char *aux = begin; aux < ptr; ++aux)
        int_val = int_val * 10 + (*aux - '0');

    if (int_val > (1LL << 53))
        return YY_ERROR_VALUE;

    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) len;
    symbol->type = YY_SYMBOL_NUMBER_VAL;
    symbol->number_val = (double) int_val;

    return YY_OK;

NUMBER_READ_FLOAT:

    assert(ptr > begin);
    assert(ptr <= end);

    if ((len = ptr - begin) >= sizeof(buf))
        return YY_ERROR_VALUE;

    memcpy(buf, begin, len);
    buf[len] = 0;
    errno = 0;

    number_val = strtod(buf, NULL);

    if (errno == ERANGE)
        return YY_ERROR_VALUE;

    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) len;
    symbol->type = YY_SYMBOL_NUMBER_VAL;
    symbol->number_val = number_val;

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
 *   - YY_SYMBOL_ESCAPED_STRING_VAL
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * @param[out] symbol Parsed symbol.
 * 
 * @return  YY_OK: Success.
 *          YY_ERROR_SYNTAX: Invalid string.
 */
static yy_error_e read_symbol_string(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    bool is_escaped = false;
    const char *ptr = begin;

    if (end - ptr < 2 || *ptr != '"') // no place for ""
        return YY_ERROR_SYNTAX;

STRING_NEXT_CHAR:

    if (++ptr == end)
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '\0': return YY_ERROR_SYNTAX;
        case '\\': goto STRING_ESCAPED_CHAR;
        case '"': goto STRING_END;
        default: goto STRING_NEXT_CHAR;
    }

STRING_ESCAPED_CHAR:

    if (end - ptr < 3)  // no place for \x"
        return YY_ERROR_SYNTAX;

    switch (*++ptr) {
        case '"': 
        case '\\': 
        case 'n': 
        case 't':
            is_escaped = true;
            goto STRING_NEXT_CHAR;
        default:
            goto STRING_NEXT_CHAR;
            //return YY_ERROR_SYNTAX;
    }

STRING_END:

    ++ptr;
    assert(ptr - begin > 1); // minimum size is ""

    symbol->type = (is_escaped ? YY_SYMBOL_ESCAPED_STRING_VAL : YY_SYMBOL_STRING_VAL);
    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = (uint32_t) (ptr - begin);
    symbol->str_val.ptr = begin + 1;
    symbol->str_val.len = symbol->lexeme.len - 2;

    return YY_OK;
}

/**
 * Parse a variable name.
 * 
 * Format  = '$' ( [a-zA-Z][a-zA-Z0-9_]* | '{' [^{}]+ '}')
 * Examples = $a, $a_x, ${train.001.driver}, ${train[1].driver}, ${with espaces}
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
 *          YY_ERROR_SYNTAX: Invalid variable.
 */
static yy_error_e read_symbol_variable(const char *begin, const char *end, yy_symbol_t *symbol)
{
    assert(begin && end && begin <= end && symbol);

    bool has_braces = false;
    const char *ptr = begin;

    if (end - ptr < 2 || *ptr != '$')
        return YY_ERROR_SYNTAX;

    switch (*++ptr) {
        case 'A' ... 'Z':
        case 'a' ... 'z': 
            goto VARIABLE_WITHOUT_BRACES;
        case '{': 
            goto VARIABLE_WITH_BRACES;
        default:
            return YY_ERROR_SYNTAX;
    }

VARIABLE_WITHOUT_BRACES:

    if (unlikely(++ptr == end))
        goto VARIABLE_END;

    switch (*ptr) {
        case '0' ... '9':
        case 'A' ... 'Z':
        case '_':
        case 'a' ... 'z':
            goto VARIABLE_WITHOUT_BRACES;
        default:
            goto VARIABLE_END;
    }

VARIABLE_WITH_BRACES:

    has_braces = true;

    if (unlikely(++ptr == end) || *ptr == '{' || *ptr == '}')
        return YY_ERROR_SYNTAX;

VARIABLE_WITH_BRACES_CONT:

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '{': return YY_ERROR_SYNTAX;
        case '}': break;
        default: goto VARIABLE_WITH_BRACES_CONT;
    }

    ++ptr;

VARIABLE_END:

    assert(ptr - begin > 1);

    symbol->type = YY_SYMBOL_VARIABLE;
    symbol->lexeme.ptr = begin;
    symbol->lexeme.len = ptr - begin;
    symbol->variable.ptr = begin + (has_braces ? 2 : 1);
    symbol->variable.len = symbol->lexeme.len - (has_braces ? 3 : 1);

    return YY_OK;
}

#define return_ok(symbol_, len_) do { \
    symbol->lexeme.ptr = begin; \
    symbol->lexeme.len = len_; \
    symbol->type = symbol_; \
    return YY_OK; \
} while(0)

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
static yy_error_e read_symbol(const char *begin, const char *end, yy_symbol_t *symbol)
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
        case '"': return read_symbol_string(ptr, end, symbol);
        case '$': return read_symbol_variable(ptr, end, symbol);
        case '%': return_ok(YY_SYMBOL_MODULO_OP, 1);
        case '&': goto NEXT_AMPERSAND;
        case '(': return_ok(YY_SYMBOL_PAREN_LEFT, 1);
        case ')': return_ok(YY_SYMBOL_PAREN_RIGHT, 1);
        case '*': return_ok(YY_SYMBOL_PRODUCT_OP, 1);
        case '+': return_ok(YY_SYMBOL_ADDITION_OP, 1);
        case ',': return_ok(YY_SYMBOL_COMMA, 1);
        case '-': return_ok(YY_SYMBOL_SUBTRACTION_OP, 1);
        case '/': return_ok(YY_SYMBOL_DIVIDE_OP, 1);
        case '0' ... '9': return read_symbol_number(ptr, end, symbol);
        case '<': goto NEXT_LESS;
        case '=': goto NEXT_EQUALS;
        case '>': goto NEXT_GREAT;
        case 'A' ... 'Z': goto NEXT_IDENTIFIER;
        case '^': return_ok(YY_SYMBOL_POWER_OP, 1);
        case 'a' ... 'z': goto NEXT_IDENTIFIER;
        case '|': goto NEXT_VERTICAL_BAR;
        default: return YY_ERROR_SYNTAX;
    }

NEXT_EXCLAMATION: // !=

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_DISTINCT_OP, 2);
        default: return YY_ERROR_SYNTAX;
    }

NEXT_AMPERSAND: // &&

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '&': return_ok(YY_SYMBOL_AND_OP, 2);
        default: return YY_ERROR_SYNTAX;
    }

NEXT_LESS: // <, <=

    if (unlikely(++ptr == end))
        return_ok(YY_SYMBOL_LESS_OP, 1);

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_LESS_EQUALS_OP, 2);
        default: return_ok(YY_SYMBOL_LESS_OP, 1);
    }

NEXT_EQUALS: // ==

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '=': return_ok(YY_SYMBOL_EQUALS_OP, 2);
        default: return YY_ERROR_SYNTAX;
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

    return YY_ERROR_SYNTAX;

NEXT_VERTICAL_BAR: // ||

    if (unlikely(++ptr == end))
        return YY_ERROR_SYNTAX;

    switch (*ptr) {
        case '|': return_ok(YY_SYMBOL_OR_OP, 2);
        default: return YY_ERROR_SYNTAX;
    }
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
        case YY_SYMBOL_ESCAPED_STRING_VAL:
        case YY_SYMBOL_VARIABLE:
            token.str_val = symbol->str_val;
            break;
        default:
            break;
    }

    return token;
}

INLINE
static yy_token_t * top_stack(yy_parser_t *parser)
{
    yy_stack_t *stack = parser->stack;

    if (stack->len == 0)
        return NULL;

    return (&stack->data[stack->len - 1]);
}

INLINE
static yy_token_t * top_operators(yy_parser_t *parser)
{
    if (parser->operators_len == 0)
        return NULL;

    yy_stack_t *stack = parser->stack;
    return (&stack->data[stack->reserved - parser->operators_len]);
}

INLINE
static void pop_stack(yy_parser_t *parser)
{
    yy_stack_t *stack = parser->stack;
    assert(stack->len != 0);
    stack->len--;
}

INLINE
static void pop_operators(yy_parser_t *parser)
{
    assert(parser->operators_len != 0);
    parser->operators_len--;
}

INLINE
static yy_token_t * get(yy_stack_t *stack, uint32_t idx)
{
    if (stack->len <= idx)
        return NULL;

    return (&stack->data[stack->len - 1 - idx]);
}

/**
 * Try to simplify tokens on the top of the RPN stack.
 * 
 * Supported simplifications:
 *   - Remove plus operator (ex: +1 -> 1)
 *   - Eval top function if its arguments are fixed values (ex: 1+1 -> 2)
 * 
 * Unsupported simplifications:
 *   - For commutative operators, move fixed values to top (ex: 1+$a+3 -> $a+1+3 -> $a+4)
 *   - Non-pure functions
 * 
 * @param stack Stack to simplify.
 * 
 * @return true = simplified, false = not simplified.
 */
INLINE
static bool simplify_stack(yy_parser_t *parser)
{
    yy_stack_t *stack = parser->stack;
    yy_token_t *token0 = top_stack(parser);

    if (!token0 || token0->type != YY_TOKEN_FUNCTION)
        return false;

    assert((int)stack->len >= token0->function.num_args + 1);

    if (token0->function.is_not_pure)
        return false;

    if (token0->function.num_args == 0) {
        *token0 = ((yy_func_0) token0->function.ptr)();
        return true;
    }

    if ((yy_func_1) token0->function.ptr == func_ident) {
        pop_stack(parser);
        return true;
    }

    yy_token_t *token1 = get(stack, 1);

    if (!token1 || !is_token_fixed_value(token1->type))
        return false;

    if (token0->function.num_args == 1) {
        *token1 = ((yy_func_1) token0->function.ptr)(*token1);
        pop_stack(parser);
        return true;
    }

    yy_token_t *token2 = get(stack, 2);

    if (!token2 || !is_token_fixed_value(token2->type))
        return false;

    if (token0->function.num_args == 2) {
        *token2 = ((yy_func_2) token0->function.ptr)(*token2, *token1);
        pop_stack(parser);
        pop_stack(parser);
        return true;
    }

    yy_token_t *token3 = get(stack, 3);

    if (!token3 || !is_token_fixed_value(token3->type))
        return false;

    if (token0->function.num_args == 3) {
        *token3 = ((yy_func_3) token0->function.ptr)(*token3, *token2, *token1);
        pop_stack(parser);
        pop_stack(parser);
        pop_stack(parser);
        return true;
    }

    assert(false);
    return false;
}

static void push_to_stack(yy_parser_t *parser, const yy_token_t *token)
{
    assert(token->type != YY_TOKEN_NULL);

    yy_stack_t *stack = parser->stack;

    if (unlikely(stack->len + parser->operators_len >= stack->reserved)) {
        assert(stack->len + parser->operators_len == stack->reserved);
        parser->error = YY_ERROR_MEM;
        return;
    }

    stack->data[stack->len++] = *token;

    if (token->type == YY_TOKEN_FUNCTION)
        simplify_stack(parser);
}

static void push_to_operators(yy_parser_t *parser, const yy_token_t *token)
{
    yy_stack_t *stack = parser->stack;

    if (unlikely(stack->len + parser->operators_len >= stack->reserved)) {
        assert(stack->len + parser->operators_len == stack->reserved);
        parser->error = YY_ERROR_MEM;
        return;
    }

    stack->data[stack->reserved - (++parser->operators_len)] = *token;
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

        if (type == YY_SYMBOL_ESCAPED_STRING_VAL)
            push_to_stack(parser, &symbol_to_token[YY_SYMBOL_UNESCAPE]);

        return;
    }

    if (is_token_operator(token))
    {
        while ((op = top_operators(parser)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;

            if (token.function.precedence < op->function.precedence)
                break;

            if (token.function.precedence == op->function.precedence && token.function.right_to_left)
                break;

            push_to_stack(parser, op);
            pop_operators(parser);
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
        while ((op = top_operators(parser)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;

            push_to_stack(parser, op);
            pop_operators(parser);
        }

        assert(op != NULL && op->type == YY_TOKEN_NULL);
        pop_operators(parser);

        op = top_operators(parser);

        if (op && is_token_regfunc(*op)) {
            push_to_stack(parser, op);
            pop_operators(parser);
        }

        return;
    }

    if (type == YY_SYMBOL_COMMA)
    {
        while ((op = top_operators(parser)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL)
                break;

            push_to_stack(parser, op);
            pop_operators(parser);
        }

        return;
    }

    if (type == YY_SYMBOL_END)
    {
        while ((op = top_operators(parser)) != NULL)
        {
            if (op->type == YY_TOKEN_NULL) {
                // unmatched parentesis
                parser->error = YY_ERROR;
                return;
            }

            push_to_stack(parser, op);
            pop_operators(parser);
        }

        return;
    }

    assert(false);
}

/**
 * Read next symbol and updates parser state accordingly.
 * Errors are notified via parser.error.
 * 
 * @param[in] parser Parser to update.
 */
static void consume(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    if (parser->curr_symbol.type != YY_SYMBOL_NONE)
    {
        // current symbol was accepted

        // append current symbol to output (rpn stack)
        process_current_symbol(parser);

        if (parser->error != YY_OK)
            return;

        // moving current position just after the accepted symbol
        parser->curr += parser->curr_symbol.lexeme.len;

    }

    assert(parser->begin <= parser->curr);
    assert(parser->curr <= parser->end);

    if (parser->curr_symbol.type == YY_SYMBOL_END)
        return;

    parser->prev_symbol = parser->curr_symbol;
    parser->curr = skip_spaces(parser->curr, parser->end);
    parser->error = read_symbol(parser->curr, parser->end, &parser->curr_symbol);
}

/**
 * Checks that current symbol is the expected one and moves to next symbol.
 * Errors are notified via parser.error.
 * 
 * @param[in] parser Parser to update.
 * @param[in] type Expected symbol.
 */
static void expect(yy_parser_t *parser, yy_symbol_e type)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    if (parser->curr_symbol.type != type) {
        parser->error = YY_ERROR_SYNTAX;
        return;
    }

    consume(parser);
}

/**
 * Finalize parse.
 * 
 * @param[in] parser Parser to update.
 */
static void finalize(yy_parser_t *parser)
{
    if (parser->error != YY_OK)
        return;

    if (parser->curr_symbol.type == YY_SYMBOL_END)
        consume(parser);
    else
        parser->error = YY_ERROR_SYNTAX;
}

static void init_parser(yy_parser_t *parser, const char *begin, const char *end, yy_stack_t *stack)
{
    assert(parser);

    stack->len = 0;

    parser->begin = begin;
    parser->end = end;
    parser->curr = begin;
    parser->stack = stack;
    parser->operators_len = 0;
    parser->curr_symbol = (yy_symbol_t){0};
    parser->prev_symbol = (yy_symbol_t){0};
    parser->error = (stack && stack->reserved ? YY_OK : YY_ERROR_MEM);

    consume(parser);
}

/**
 * Parse a numeric term.
 * 
 * @see Grammar described in docs.
 * 
 * @param[in] parser Parser to update.
 */
static void parse_term_number(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_CONST_E:
        case YY_SYMBOL_CONST_PI:
        case YY_SYMBOL_CONST_INF:
        case YY_SYMBOL_CONST_NAN:
        case YY_SYMBOL_NUMBER_VAL:
        case YY_SYMBOL_VARIABLE:
            consume(parser);
            break;
        case YY_SYMBOL_VARIABLE_FUNC:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
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
        case YY_SYMBOL_CLAMP:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_COMMA);
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
        case YY_SYMBOL_LENGTH:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_FIND:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_IFELSE:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_bool(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        default:
            parser->error = YY_ERROR_SYNTAX;
    }
}

/**
 * Parse a numeric expression until an unrecognized symbol is found.
 * 
 * @param[in] Parser object.
 */
static void parse_expr_number(yy_parser_t *parser)
{ 
    assert(parser);

EXPR_NUMBER_START:

    if (parser->error != YY_OK)
        return;

    parse_term_number(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_ADDITION_OP: 
        case YY_SYMBOL_SUBTRACTION_OP: 
        case YY_SYMBOL_PRODUCT_OP: 
        case YY_SYMBOL_DIVIDE_OP: 
        case YY_SYMBOL_MODULO_OP: 
        case YY_SYMBOL_POWER_OP: 
            consume(parser);
            goto EXPR_NUMBER_START;
        default:
            break;
    }
}

/**
 * Parse a string term.
 * 
 * @see Grammar described in docs.
 * 
 * @param[in] parser Parser to update.
 */
static void parse_term_string(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_STRING_VAL:
        case YY_SYMBOL_ESCAPED_STRING_VAL:
        case YY_SYMBOL_VARIABLE:
            consume(parser);
            break;
        case YY_SYMBOL_VARIABLE_FUNC:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_TRIM:
        case YY_SYMBOL_LOWER:
        case YY_SYMBOL_UPPER:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_STR:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_generic(parser, true);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_MIN:
        case YY_SYMBOL_MAX:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_SUBSTR:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_REPLACE:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_IFELSE:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_bool(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_PAREN_LEFT:
            consume(parser);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        default:
            parser->error = YY_ERROR_SYNTAX;
    }
}

/**
 * Parse a string expression until an unrecognized symbol is found.
 * 
 * @param[in] Parser object.
 */
static void parse_expr_string(yy_parser_t *parser)
{ 
    assert(parser);

EXPR_STRING_START:

    if (parser->error != YY_OK)
        return;

    parse_term_string(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_ADDITION_OP: 
            parser->curr_symbol.type = YY_SYMBOL_CONCAT_OP;
            consume(parser);
            goto EXPR_STRING_START;
        default:
            break;
    }
}

static void parse_expr_by_type(yy_parser_t *parser, yy_token_e type)
{
    if (parser->error != YY_OK)
        return;

    switch (type)
    {
        case YY_TOKEN_BOOL: parse_expr_bool(parser); break;
        case YY_TOKEN_NUMBER: parse_expr_number(parser); break;
        case YY_TOKEN_DATETIME: parse_expr_datetime(parser); break;
        case YY_TOKEN_STRING: parse_expr_string(parser); break;
        default: parser->error = YY_ERROR_SYNTAX; break;
    }
}

/**
 * Parse an expression until an unrecognized symbol is found.
 * 
 * @param[in] parser Parser to update.
 * @param[in] check_bool Checks bool type (to avoid a non-ending loop parsing a bool).
 * 
 * @return YY_TOKEN_BOOL - Parsed bool expression,
 *         YY_TOKEN_NUMBER - Parsed number expression,
 *         YY_TOKEN_DATETIME - Parsed datetime expression,
 *         YY_TOKEN_STRING - Parsed string expression,
 *         YY_TOKEN_ERROR - Error parsing expression (see parser.error).
 */
static yy_token_e parse_expr_generic(yy_parser_t *parser, bool check_bool)
{
    static const yy_token_e types[] = {YY_TOKEN_BOOL, YY_TOKEN_NUMBER, YY_TOKEN_DATETIME, YY_TOKEN_STRING};

    if (parser->error != YY_OK)
        return YY_TOKEN_ERROR;

    yy_parser_t orig_parser = *parser;
    yy_stack_t orig_stack = *parser->stack;
    yy_error_e error = YY_OK;

    for (size_t i = (check_bool ? 0 : 1); i < (sizeof(types)/sizeof(types[0])); i++)
    {
        parse_expr_by_type(parser, types[i]);

        if (parser->error == YY_OK)
            return types[i];

        error = MAX(error, parser->error);

        // restore state
        *parser = orig_parser;
        *parser->stack = orig_stack;
    }

    parser->error = error;
    return YY_TOKEN_ERROR;
}

/**
 * Parse a bool term.
 * 
 * @see Grammar described in docs.
 * 
 * @param[in] parser Parser to update.
 */
static void parse_term_bool(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_TRUE:
        case YY_SYMBOL_FALSE:
        case YY_SYMBOL_VARIABLE:
            consume(parser);
            return;
        case YY_SYMBOL_VARIABLE_FUNC:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            return;
        case YY_SYMBOL_NOT:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_bool(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            return;
        case YY_SYMBOL_ISINF:
        case YY_SYMBOL_ISNAN:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            return;
        case YY_SYMBOL_ISERROR:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_generic(parser, true);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            return;
        case YY_SYMBOL_PAREN_LEFT:
            consume(parser);
            parse_expr_bool(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            return;
        default:
            break;
    }

    yy_token_e type = parse_expr_generic(parser, false);

    if (type == YY_TOKEN_ERROR || parser->error)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_LESS_OP:
        case YY_SYMBOL_LESS_EQUALS_OP:
        case YY_SYMBOL_GREAT_OP:
        case YY_SYMBOL_GREAT_EQUALS_OP:
        case YY_SYMBOL_EQUALS_OP:
        case YY_SYMBOL_DISTINCT_OP:
            consume(parser);
            parse_expr_by_type(parser, type);
            break;
        default:
            parser->error = YY_ERROR_SYNTAX;
    }
}

/**
 * Parse a boolean expression until an unrecognized symbol is found.
 * 
 * @param[in] Parser object.
 */
static void parse_expr_bool(yy_parser_t *parser)
{ 
    assert(parser);

EXPR_BOOL_START:

    if (parser->error != YY_OK)
        return;

    parse_term_bool(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_AND_OP: 
        case YY_SYMBOL_OR_OP: 
        case YY_SYMBOL_EQUALS_OP: 
        case YY_SYMBOL_DISTINCT_OP: 
            consume(parser);
            goto EXPR_BOOL_START;
        default:
            break;
    }
}

/**
 * Parse a datepart identifier.
 * 
 * @param[in] Parser object.
 */
static void parse_datepart(yy_parser_t *parser)
{
    assert(parser);

    int part = -1;

    if (parser->error != YY_OK)
        return;

    if (parser->curr_symbol.type != YY_SYMBOL_STRING_VAL) {
        parser->error = YY_ERROR_SYNTAX;
        return;
    }

    if ((part = get_datepart(&parser->curr_symbol.str_val)) < 0) {
        parser->error = YY_ERROR_SYNTAX;
        return;
    }

    parser->curr_symbol.type = YY_SYMBOL_NUMBER_VAL;
    parser->curr_symbol.number_val = (double) part;

    consume(parser);
}

/**
 * Parse a datetime value.
 * 
 * @param[in] Parser object.
 */
static void parse_datetime_val(yy_parser_t *parser)
{
    assert(parser);

    if (parser->error != YY_OK)
        return;

    if (parser->curr_symbol.type != YY_SYMBOL_STRING_VAL) {
        parser->error = YY_ERROR_SYNTAX;
        return;
    }

    const char *begin = parser->curr_symbol.str_val.ptr;
    const char *end = begin + parser->curr_symbol.str_val.len;

    yy_token_t token = yy_parse_datetime(begin, end);

    if (token.type != YY_TOKEN_DATETIME) {
        parser->error = YY_ERROR_SYNTAX;
        return;
    }

    parser->curr_symbol.type = YY_SYMBOL_DATETIME_VAL;
    parser->curr_symbol.datetime_val = token.datetime_val;

    consume(parser);
}

/**
 * Parse a datetime term.
 * 
 * @param[in] Parser object.
 */
static void parse_term_datetime(yy_parser_t *parser)
{ 
    assert(parser);

    if (parser->error != YY_OK)
        return;

    switch (parser->curr_symbol.type)
    {
        case YY_SYMBOL_STRING_VAL:
            parse_datetime_val(parser);
            break;
        case YY_SYMBOL_VARIABLE:
            consume(parser);
            break;
        case YY_SYMBOL_VARIABLE_FUNC:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_string(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_NOW: 
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_DATEADD: 
        case YY_SYMBOL_DATESET: 
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_number(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_datepart(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_DATETRUNC: 
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_datepart(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_MIN: 
        case YY_SYMBOL_MAX: 
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_CLAMP:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        case YY_SYMBOL_IFELSE:
            consume(parser);
            expect(parser, YY_SYMBOL_PAREN_LEFT);
            parse_expr_bool(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_COMMA);
            parse_expr_datetime(parser);
            expect(parser, YY_SYMBOL_PAREN_RIGHT);
            break;
        default:
            parser->error = YY_ERROR_SYNTAX;
    }
}

/**
 * Parse a datetime expression until an unrecognized symbol is found.
 * 
 * @param[in] Parser object.
 */
INLINE
static void parse_expr_datetime(yy_parser_t *parser)
{
    parse_term_datetime(parser);
}

yy_error_e yy_compile_number(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR;

    yy_parser_t parser;

    init_parser(&parser, begin, end, stack);
    parse_expr_number(&parser);
    finalize(&parser);

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

yy_error_e yy_compile_datetime(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR;

    yy_parser_t parser;

    init_parser(&parser, begin, end, stack);
    parse_expr_datetime(&parser);
    finalize(&parser);

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

yy_error_e yy_compile_string(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR;

    yy_parser_t parser;

    init_parser(&parser, begin, end, stack);
    parse_expr_string(&parser);
    finalize(&parser);

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

yy_error_e yy_compile_bool(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR;

    yy_parser_t parser;

    init_parser(&parser, begin, end, stack);
    parse_expr_bool(&parser);
    finalize(&parser);

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

yy_error_e yy_compile(const char *begin, const char *end, yy_stack_t *stack, const char **err)
{
    if (!begin || !end || begin > end || !stack || !stack->data)
        return YY_ERROR;

    yy_parser_t parser;
    yy_token_e rc = YY_TOKEN_NULL;

    init_parser(&parser, begin, end, stack);
    rc = parse_expr_generic(&parser, true);

    switch (rc)
    {
        case YY_TOKEN_BOOL:
        case YY_TOKEN_NUMBER:
        case YY_TOKEN_DATETIME:
        case YY_TOKEN_STRING:
            finalize(&parser);
            break;
        default:
            break;
    }

    if (err && parser.error != YY_OK)
        *err = parser.curr;

    return parser.error;
}

INLINE
static yy_token_t eval_func(yy_func_t func, yy_eval_ctx_t *ctx)
{
    yy_stack_t *stack = ctx->stack;

    if (!func.ptr) 
        return token_error(YY_ERROR_EVAL);

    switch (func.num_args)
    {
        case 0:
        {
            if (func.is_not_pure)
                return ((yy_func_0_x) func.ptr)(ctx);
            else
                return ((yy_func_0) func.ptr)();
        }

        case 1:
        {
            yy_token_t *token1 = get(stack, 0);

            if (func.is_not_pure)
                return ((yy_func_1_x) func.ptr)(*token1, ctx);
            else
                return ((yy_func_1) func.ptr)(*token1);
        }

        case 2:
        {
            yy_token_t *token1 = get(stack, 0);
            yy_token_t *token2 = get(stack, 1);

            if (func.is_not_pure)
                return ((yy_func_2_x) func.ptr)(*token2, *token1, ctx);
            else
                return ((yy_func_2) func.ptr)(*token2, *token1);
        }

        case 3:
        {
            yy_token_t *token1 = get(stack, 0);
            yy_token_t *token2 = get(stack, 1);
            yy_token_t *token3 = get(stack, 2);

            if (func.is_not_pure)
                return ((yy_func_3_x) func.ptr)(*token3, *token2, *token1, ctx);
            else
                return ((yy_func_3) func.ptr)(*token3, *token2, *token1);
        }

        default:
            return token_error(YY_ERROR_EVAL);
    }
}

yy_token_t yy_eval_stack(const yy_stack_t *stack, yy_stack_t *aux, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    if (!stack || !aux || !stack->data || !stack->len || !aux->data)
        return token_error(YY_ERROR);

    yy_eval_ctx_t ctx = {.stack = aux, .tmp_str = (char *) &aux->data[aux->reserved]};
    yy_token_t tmp = {0};

    aux->len = 0;

    for (size_t i = 0; i < stack->len; i++)
    {
        switch (stack->data[i].type)
        {
            case YY_TOKEN_BOOL:
            case YY_TOKEN_NUMBER:
            case YY_TOKEN_DATETIME:
            case YY_TOKEN_STRING:
            {
                if (aux->reserved <= aux->len)
                    return token_error(YY_ERROR_MEM);

                aux->data[aux->len++] = stack->data[i];
                break;
            }
            case YY_TOKEN_ERROR:
            {
                if (is_blocking_error(stack->data[i].error))
                    return token_error(YY_ERROR_EVAL);

                aux->data[aux->len++] = stack->data[i];
                break;
            }
            case YY_TOKEN_VARIABLE:
            {
                if (aux->reserved <= aux->len)
                    return token_error(YY_ERROR_MEM);

                if (!resolve)
                    return token_error(YY_ERROR_REF);

                tmp = resolve(&stack->data[i].variable, data);
                if (tmp.type == YY_TOKEN_ERROR && is_blocking_error(tmp.error))
                    return tmp;

                aux->data[aux->len++] = tmp;
                break;
            }
            case YY_TOKEN_FUNCTION:
            {
                tmp = eval_func(stack->data[i].function, &ctx);
                if (tmp.type == YY_TOKEN_ERROR && is_blocking_error(tmp.error))
                    return tmp;

                // dealloc temp memory used by arguments
                for (uint32_t j = 0; j < stack->data[i].function.num_args; j++)
                {
                    yy_token_t *arg = get(aux, j);

                    if (!arg || arg->type != YY_TOKEN_STRING || !is_temp_ptr(&ctx, arg->str_val.ptr))
                        continue;

                    if (tmp.type == YY_TOKEN_STRING && is_temp_ptr(&ctx, tmp.str_val.ptr))
                    {
                        if (tmp.str_val.ptr == arg->str_val.ptr)
                            continue;

                        if (tmp.str_val.ptr < arg->str_val.ptr)
                            tmp.str_val.ptr += arg->str_val.len;
                    }

                    free_str(&ctx, &arg->str_val);
                }

                if (stack->data[i].function.num_args)
                    aux->len -= stack->data[i].function.num_args;
                else if (aux->reserved <= aux->len)
                    return token_error(YY_ERROR_MEM);

                aux->data[aux->len++] = tmp;
                break;
            }
            default:
                return token_error(YY_ERROR_EVAL);
        }
    }

    if (aux->len != 1)
        return token_error(YY_ERROR_EVAL);

    assert (aux->data[0].type != YY_TOKEN_STRING || 
            !is_temp_ptr(&ctx, aux->data[0].str_val.ptr) || 
            (aux->data[0].str_val.ptr == ctx.tmp_str && ctx.tmp_str + aux->data[0].str_val.len == (char *) &aux->data[aux->reserved]));

    return aux->data[0];
}

yy_token_t yy_eval_number(const char *begin, const char *end, yy_stack_t *stack, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    yy_error_e rc = yy_compile_number(begin, end, stack, NULL);

    if (rc != YY_OK)
        return token_error(rc);

    uint32_t reserved = stack->reserved - stack->len;
    yy_stack_t aux = {.data = stack->data + stack->len, .reserved = reserved, .len = 0};

    return yy_eval_stack(stack, &aux, resolve, data);
}

yy_token_t yy_eval_datetime(const char *begin, const char *end, yy_stack_t *stack, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    yy_error_e rc = yy_compile_datetime(begin, end, stack, NULL);

    if (rc != YY_OK)
        return token_error(rc);

    uint32_t reserved = stack->reserved - stack->len;
    yy_stack_t aux = {.data = stack->data + stack->len, .reserved = reserved, .len = 0};

    return yy_eval_stack(stack, &aux, resolve, data);
}

yy_token_t yy_eval_string(const char *begin, const char *end, yy_stack_t *stack, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    yy_error_e rc = yy_compile_string(begin, end, stack, NULL);

    if (rc != YY_OK)
        return token_error(rc);

    uint32_t reserved = stack->reserved - stack->len;
    yy_stack_t aux = {.data = stack->data + stack->len, .reserved = reserved, .len = 0};

    return yy_eval_stack(stack, &aux, resolve, data);
}

yy_token_t yy_eval_bool(const char *begin, const char *end, yy_stack_t *stack, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    yy_error_e rc = yy_compile_bool(begin, end, stack, NULL);

    if (rc != YY_OK)
        return token_error(rc);

    uint32_t reserved = stack->reserved - stack->len;
    yy_stack_t aux = {.data = stack->data + stack->len, .reserved = reserved, .len = 0};

    return yy_eval_stack(stack, &aux, resolve, data);
}

yy_token_t yy_eval(const char *begin, const char *end, yy_stack_t *stack, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data)
{
    yy_error_e rc = yy_compile(begin, end, stack, NULL);

    if (rc != YY_OK)
        return token_error(rc);

    uint32_t reserved = stack->reserved - stack->len;
    yy_stack_t aux = {.data = stack->data + stack->len, .reserved = reserved, .len = 0};

    return yy_eval_stack(stack, &aux, resolve, data);
}

yy_token_t yy_parse_number(const char *begin, const char *end)
{
    if (!begin || !end || begin >= end)
        return token_error(YY_ERROR);

    bool negated = false;
    yy_symbol_t symbol = {0};

    switch (*begin) {
        case '-':
            negated = true;
            fallthrough;
        case '+':
            begin++;
            break;
        default:
            break;
    }

    if (read_symbol_number(begin, end, &symbol) != YY_OK)
        return token_error(YY_ERROR_VALUE);

    if (symbol.lexeme.ptr + symbol.lexeme.len != end)
        return token_error(YY_ERROR_VALUE);

    if (negated)
        return token_number(-symbol.number_val);
    else
        return token_number(+symbol.number_val);
}

yy_token_t yy_parse_string(const char *begin, const char *end)
{
    if (!begin || !end || begin > end)
        return token_error(YY_ERROR);

    size_t len = end - begin;

    // string too long
    if (len > UINT32_MAX)
        return token_error(YY_ERROR_VALUE);

    // NUL in the midle
    for (const char *ptr = begin; ptr < end; ++ptr)
        if (*ptr == 0)
            return token_error(YY_ERROR_VALUE);

    return token_string(begin, len);
}

yy_token_t yy_parse_bool(const char *begin, const char *end)
{
    if (!begin || !end || begin >= end)
        return token_error(YY_ERROR);

    const char *ptr = begin;

    switch (*ptr) {
        case 'f':
        case 'F': goto BOOL_FALSE;
        case 't':
        case 'T': goto BOOL_TRUE;
        default: goto BOOL_ERROR;
    }

BOOL_FALSE:

    if (end - ptr != 5)
        goto BOOL_ERROR;

    switch (*++ptr) {
        case 'A':
            if (ptr[-1] == 'F' && ptr[1] == 'L' && ptr[2] == 'S' && ptr[3] == 'E')
                return token_bool(false);
            else
                goto BOOL_ERROR;
        case 'a':
            if (ptr[1] == 'l' && ptr[2] == 's' && ptr[3] == 'e')
                return token_bool(false);
            else
                goto BOOL_ERROR;
        default:
            goto BOOL_ERROR;
    }

BOOL_TRUE:

    if (end - ptr != 4)
        goto BOOL_ERROR;

    switch (*++ptr) {
        case 'R':
            if (ptr[-1] == 'T' && ptr[1] == 'U' && ptr[2] == 'E')
                return token_bool(true);
            else
                goto BOOL_ERROR;
        case 'r':
            if (ptr[1] == 'u' && ptr[2] == 'e')
                return token_bool(true);
            else
                goto BOOL_ERROR;
        default: 
            goto BOOL_ERROR;
    }

BOOL_ERROR:

    return token_error(YY_ERROR_VALUE);
}

/**
 * Parse a datetime value in format ISO-8601.
 *    - format = YYYY-MM-DD[Tdd:mm:ss[.SSS[Z]]]
 *    - example = 2024-08-24T08:19:25.402Z
 *    - regex := (19[7-9][0-9]|2[0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])(T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(.[0-9]{1,3}Z?)?)?
 *    - range := [1970-01-01T00:00:00.000Z, 2999-12-31T23:59:59.999Z]
 *    - @see https://en.wikipedia.org/wiki/ISO_8601
 * 
 * Parse whole content (^.*$).
 * Anything distinct than datetime is reported as error.
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
 * 
 * @return  Token with the result. 
 *          On error, token.type == YY_TOKEN_ERROR.
 */
yy_token_t yy_parse_datetime(const char *begin, const char *end)
{
    if (!begin || !end || begin > end)
        return token_error(YY_ERROR);

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
        goto DATETIME_ERROR;

    switch (*ptr) {
        case '1': buf[len++] = *ptr; goto DATETIME_YEAR_1900;
        case '2': buf[len++] = *ptr; goto DATETIME_YEAR_2000;
        default: goto DATETIME_ERROR;
    }

DATETIME_YEAR_1900:

    if (*++ptr != '9')
        goto DATETIME_ERROR;

    buf[len++] = *ptr;

    switch (*++ptr) {
        case '7' ... '9': buf[len++] = *ptr; break;
        default: goto DATETIME_ERROR;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_YEAR;
        default: goto DATETIME_ERROR;
    }

DATETIME_YEAR_2000:

    for (int i = 0; i < 3; i++)
    {
        switch (*++ptr) {
            case '0' ... '9': buf[len++] = *ptr; break;
            default: goto DATETIME_ERROR;
        }
    }

DATETIME_READ_YEAR:

    assert(len == 4);
    buf[len] = 0;
    stm.tm_year = atoi(buf) - 1900;
    len = 0;

    if (*++ptr != '-')
        goto DATETIME_ERROR;

//DATETIME_MONTH:

    switch (*++ptr) {
        case '0': buf[len++] = *ptr; goto DATETIME_MONTH_0;
        case '1': buf[len++] = *ptr; goto DATETIME_MONTH_1;
        default: goto DATETIME_ERROR;
    }

DATETIME_MONTH_0:

    switch (*++ptr) {
        case '1' ... '9': buf[len++] = *ptr; goto DATETIME_READ_MONTH;
        default: goto DATETIME_ERROR;
    }

DATETIME_MONTH_1:

    switch (*++ptr) {
        case '0' ... '2': buf[len++] = *ptr; goto DATETIME_READ_MONTH;
        default: goto DATETIME_ERROR;
    }

DATETIME_READ_MONTH:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_mon = atoi(buf) - 1;
    len = 0;

    if (*++ptr != '-')
        goto DATETIME_ERROR;

//DATETIME_DAY:

    switch (*++ptr) {
        case '0': buf[len++] = *ptr; goto DATETIME_DAY_0;
        case '1' ... '2': buf[len++] = *ptr; goto DATETIME_DAY_1_2;
        case '3': buf[len++] = *ptr; goto DATETIME_DAY_3;
        default: goto DATETIME_ERROR;
    }

DATETIME_DAY_0:

    switch (*++ptr) {
        case '1' ... '9': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: goto DATETIME_ERROR;
    }

DATETIME_DAY_1_2:

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: goto DATETIME_ERROR;
    }

DATETIME_DAY_3:

    switch (*++ptr) {
        case '0' ... '1': buf[len++] = *ptr; goto DATETIME_READ_DAY;
        default: goto DATETIME_ERROR;
    }

DATETIME_READ_DAY:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_mday = atoi(buf);
    len = 0;

    if (++ptr == end)
        goto DATETIME_END;

    if (*ptr != 'T')
        goto DATETIME_ERROR;

    if (++ptr == end)
        goto DATETIME_ERROR;

//DATETIME_TIME:

    if (end - ptr < 8) // no place for hh:mm:ss
        goto DATETIME_ERROR;

//DATETIME_HOUR:

    switch (*ptr) {
        case '0' ... '1': buf[len++] = *ptr; goto DATETIME_HOUR_0_1;
        case '2': buf[len++] = *ptr; goto DATETIME_HOUR_2;
        default: goto DATETIME_ERROR;
    }

DATETIME_HOUR_0_1:

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_HOUR;
        default: goto DATETIME_ERROR;
    }

DATETIME_HOUR_2:

    switch (*++ptr) {
        case '0' ... '3': buf[len++] = *ptr; goto DATETIME_READ_HOUR;
        default: goto DATETIME_ERROR;
    }

DATETIME_READ_HOUR:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_hour = atoi(buf);
    len = 0;

    if (*++ptr != ':')
        goto DATETIME_ERROR;

//DATETIME_MINUTE:

    switch (*++ptr) {
        case '0' ... '5': buf[len++] = *ptr; break;
        default: goto DATETIME_ERROR;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_MINUTE;
        default: goto DATETIME_ERROR;
    }

DATETIME_READ_MINUTE:

    assert(len == 2);
    buf[len] = 0;
    stm.tm_min = atoi(buf);
    len = 0;

    if (*++ptr != ':')
        goto DATETIME_ERROR;

//DATETIME_SECOND:

    switch (*++ptr) {
        case '0' ... '5': buf[len++] = *ptr; break;
        default: goto DATETIME_ERROR;
    }

    switch (*++ptr) {
        case '0' ... '9': buf[len++] = *ptr; goto DATETIME_READ_SECOND;
        default: goto DATETIME_ERROR;
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
        default: goto DATETIME_ERROR;
    }

DATETIME_MILLIS:

    if (++ptr == end)
        goto DATETIME_ERROR;

    switch (*ptr) {
        case '0' ... '9': buf[len++] = *ptr; break;
        default: goto DATETIME_ERROR;
    }

    for (int i = 0; i < 2; i++)
    {
        if (++ptr == end)
            goto DATETIME_READ_MILLIS;

        switch (*ptr) {
            case '0' ... '9': buf[len++] = *ptr; break;
            case 'Z': goto DATETIME_READ_MILLIS;
            default: goto DATETIME_ERROR;
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
        goto DATETIME_ERROR;

DATETIME_ENDING_Z:

    if (++ptr != end)
        goto DATETIME_ERROR;

DATETIME_END:

    assert(ptr > begin);
    assert(ptr <= end);

    if (stm.tm_mday > days_in_month[stm.tm_mon])
        goto DATETIME_ERROR;

    if (!is_leap_year(1900 + stm.tm_year) && stm.tm_mon == 1 && stm.tm_mday == 29)
        goto DATETIME_ERROR;

    errno = 0;

    time_t val_secs =  timegm(&stm);

    if (errno != 0)
        goto DATETIME_ERROR;

    return (yy_token_t){.type = YY_TOKEN_DATETIME, .datetime_val = (uint64_t) val_secs * 1000 + millis};

DATETIME_ERROR:

    return token_error(YY_ERROR_VALUE);
}

yy_token_t yy_parse(const char *begin, const char *end)
{
    if (!begin || !end || begin >= end)
        return token_error(YY_ERROR);

    yy_token_t ret = {0};

    if ((ret = yy_parse_number(begin, end)).type == YY_TOKEN_NUMBER)
        return ret;

    if ((ret = yy_parse_bool(begin, end)).type == YY_TOKEN_BOOL)
        return ret;

    if ((ret = yy_parse_datetime(begin, end)).type == YY_TOKEN_DATETIME)
        return ret;

    if ((ret = yy_parse_string(begin, end)).type == YY_TOKEN_STRING)
        return ret;

    return token_error(YY_ERROR_SYNTAX);
}

// ==================================================
// Every func_xxx() receiving an YY_TYPE_STRING has to
// manage the aux memory (yes, it's weird).
// ==================================================

#define UNUSED(x) (void)(x)

// --- Functions returning a datetime

static yy_token_t func_now(yy_eval_ctx_t *ctx)
{
    UNUSED(ctx);

    struct timeval stv = {0};

    gettimeofday(&stv, NULL);

    uint64_t val = (uint64_t)(stv.tv_sec) * 1000 + (uint64_t)(stv.tv_usec) / 1000;

    return token_datetime(val);
}

static yy_token_t func_dateadd(yy_token_t date, yy_token_t value, yy_token_t part)
{
    if (date.type != YY_TOKEN_DATETIME)
        return token_error(YY_ERROR_VALUE);

    if (value.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    if (part.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    time_t time = (time_t)(date.datetime_val / 1000UL);
    long ms = (long)(date.datetime_val % 1000);
    int val = (int) value.number_val;
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    switch((int) part.number_val)
    {
        case 0: stm.tm_year += val; break;
        case 1: stm.tm_mon  += val; break;
        case 2: stm.tm_mday += val; break;
        case 3: stm.tm_hour += val; break;
        case 4: stm.tm_min  += val; break;
        case 5: stm.tm_sec  += val; break;
        case 6: 
            stm.tm_sec += (ms + val) / 1000;
            ms = (ms + val) % 1000;
            if (ms < 0) {
                stm.tm_sec--;
                ms += 1000;
            }
            break;
        default: 
            return token_error(YY_ERROR_VALUE);
    }

    errno = 0;
    time_t val_secs =  timegm(&stm);

    if (errno != 0)
        return token_error(YY_ERROR_VALUE);

    return token_datetime((uint64_t) val_secs * 1000 + ms);
}

static yy_token_t func_dateset(yy_token_t date, yy_token_t value, yy_token_t part)
{
    if (date.type != YY_TOKEN_DATETIME)
        return token_error(YY_ERROR_VALUE);

    if (value.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    if (part.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    if (value.number_val < 0.0)
        return token_error(YY_ERROR_VALUE);

    time_t time = (time_t)(date.datetime_val / 1000UL);
    long ms = (long)(date.datetime_val % 1000);
    int val = (int) value.number_val;
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    switch((int) part.number_val)
    {
        case 0: stm.tm_year = val - 1900; break;
        case 1: stm.tm_mon  = val - 1; break;
        case 2: stm.tm_mday = val; break;
        case 3: stm.tm_hour = val; break;
        case 4: stm.tm_min  = val; break;
        case 5: stm.tm_sec  = val; break;
        case 6: 
            stm.tm_sec += val / 1000;
            ms = val % 1000;
            break;
        default: 
            return token_error(YY_ERROR_VALUE);
    }

    errno = 0;
    time_t val_secs =  timegm(&stm);

    if (errno != 0)
        return token_error(YY_ERROR_VALUE);

    return token_datetime((uint64_t) val_secs * 1000 + ms);
}

static yy_token_t func_datetrunc(yy_token_t date, yy_token_t part)
{
    if (date.type != YY_TOKEN_DATETIME)
        return token_error(YY_ERROR_VALUE);

    if (part.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    time_t time = (time_t)(date.datetime_val / 1000UL);
    int millis = (int)(date.datetime_val % 1000);
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    switch((int) part.number_val)
    {
        case 0: stm.tm_mon  =  0; fallthrough;
        case 1: stm.tm_mday =  1; fallthrough;
        case 2: stm.tm_hour =  0; fallthrough;
        case 3: stm.tm_min  =  0; fallthrough;
        case 4: stm.tm_sec  =  0; fallthrough;
        case 5: millis =  0; fallthrough;
        case 6: break;
        default: 
            return token_error(YY_ERROR_VALUE);
    }

    errno = 0;
    time_t val_secs =  timegm(&stm);

    if (errno != 0)
        return token_error(YY_ERROR_VALUE);

    return token_datetime((uint64_t) val_secs * 1000 + millis);
}

// --- Functions returning a string

static char * datetime_to_str(uint64_t millis_utc, char *ret)
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

INLINE
static bool is_temp_ptr(yy_eval_ctx_t *ctx, const char *ptr) {
    return (ptr && (char *) &ctx->stack->data[ctx->stack->len] <= ptr && ptr < (char *) &ctx->stack->data[ctx->stack->reserved]);
}

INLINE
static uint32_t temp_total_bytes(yy_eval_ctx_t *ctx) {
    return (uint32_t) sizeof(yy_token_t) * (ctx->stack->reserved - ctx->stack->len);
}

INLINE
static uint32_t temp_used_bytes(yy_eval_ctx_t *ctx) {
    return (uint32_t)((char *) &ctx->stack->data[ctx->stack->reserved] - ctx->tmp_str);
}

INLINE
static uint32_t temp_avail_bytes(yy_eval_ctx_t *ctx) {
    return (uint32_t)(ctx->tmp_str - (char *) &ctx->stack->data[ctx->stack->len]);
}

/**
 * Deallocates a string located in the temporary memory.
 * 
 * Example (str2 removal):
 * 
 *   before:
 *   STACK = [tok1, tok2, tok3, tok4, ... <empty> ..., str3, str2, str1]
 * 
 *   after:
 *   STACK = [tok1, tok2, tok3, tok4, ... <empty> ...,  0000, str3, str1]
 * 
 * Caution: References to str3 are broken!
 * 
 * @param[in] ctx Eval context to use.
 * @param[in,out] str String to deallocate.
 */
static void free_str(yy_eval_ctx_t *ctx, yy_str_t *str)
{
    assert(ctx && ctx->stack && ctx->tmp_str);
    assert((char *) &ctx->stack->data[ctx->stack->len] <= ctx->tmp_str);
    assert(ctx->tmp_str <= (char *) &ctx->stack->data[ctx->stack->reserved]);

    if (!is_temp_ptr(ctx, str->ptr) || str->len == 0)
        return;

    if (str->ptr > ctx->tmp_str)
        memmove(ctx->tmp_str + str->len, ctx->tmp_str, str->ptr - ctx->tmp_str);

    memset(ctx->tmp_str, 0x00, str->len);

    ctx->tmp_str += str->len;
    *str = make_string("", 0);
}

/**
 * Allocates a new string in the temporary memory.
 * 
 * @param[in] ctx Eval context to use.
 * @param[in] size Size of the new string.
 * @param[out] ret Allocated string.
 * 
 * @return true = success, false = not enough memory.
 */
static bool alloc_str(yy_eval_ctx_t *ctx, uint32_t size, yy_str_t *ret)
{
    assert(ctx && ctx->stack && ctx->tmp_str);
    assert((char *) &ctx->stack->data[ctx->stack->len] <= ctx->tmp_str);
    assert(ctx->tmp_str <= (char *) &ctx->stack->data[ctx->stack->reserved]);

    if (size > temp_avail_bytes(ctx))
        return false;

    ctx->tmp_str -= size;

    *ret = make_string(ctx->tmp_str, size);

    return true;
}

/**
 * Create a copy of the string in the temporary memory.
 * 
 * @param[in] ctx Eval context to use.
 * @param[in] str Size of the new string.
 * @param[out] ret Allocated string.
 * 
 * @return true = success, false = not enough memory.
 */
static bool duplicate_str(yy_eval_ctx_t *ctx, yy_str_t str, yy_str_t *ret)
{
    assert(ctx && ctx->stack && ctx->tmp_str);
    assert((char *) &ctx->stack->data[ctx->stack->len] <= ctx->tmp_str);
    assert(ctx->tmp_str <= (char *) &ctx->stack->data[ctx->stack->reserved]);

    if (!alloc_str(ctx, str.len, ret))
        return false;

    memcpy((char *) ret->ptr, str.ptr, str.len);

    return true;
}

static yy_token_t func_str(yy_token_t x, yy_eval_ctx_t *ctx)
{
    if (x.type == YY_TOKEN_STRING)
        return x;

    if (x.type == YY_TOKEN_BOOL)
    {
        if (x.bool_val)
            return token_string("true", 4);
        else
            return token_string("false", 5);
    }

    if (x.type == YY_TOKEN_NUMBER)
    {
        yy_str_t ret = {0};
        char buf[128] = {0};

        if (isinf(x.number_val))
            snprintf(buf, sizeof(buf), "%sInf", (isinf(x.number_val) < 0 ? "-" : ""));
        else if (isnan(x.number_val))
            snprintf(buf, sizeof(buf), "NaN");
        else
            snprintf(buf, sizeof(buf), "%g", x.number_val);

        if (!duplicate_str(ctx, make_string(buf, strlen(buf)), &ret))
            return token_error(YY_ERROR_MEM);

        return token_string(ret.ptr, ret.len);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        yy_str_t ret = {0};
        char buf[64] = {0};

        datetime_to_str(x.datetime_val, buf);

        if (!duplicate_str(ctx, make_string(buf, strlen(buf)), &ret))
            return token_error(YY_ERROR_MEM);

        return token_string(ret.ptr, ret.len);
    }

    return token_error(YY_ERROR);
}

static yy_token_t func_trim(yy_token_t str, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    const char *ptr = str.str_val.ptr;
    const char *end = str.str_val.ptr + str.str_val.len;

    while (ptr < end && isspace(*ptr))
        ++ptr;

    while (ptr < end && isspace(*(end-1)))
        --end;

    uint32_t new_len = end - ptr;

    if (new_len == str.str_val.len)
        return str;

    if (!is_temp_ptr(ctx, ptr))
        return token_string(ptr, new_len);

    yy_str_t ret = {0};

    if (!duplicate_str(ctx, make_string(ptr, new_len), &ret))
        return token_error(YY_ERROR_MEM);

    return token_string(ret.ptr, ret.len);
}

static yy_token_t func_lower(yy_token_t str, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    bool has_upper = false;

    for (uint32_t i = 0; i < str.str_val.len; i++) {
        int c = str.str_val.ptr[i];
        if (isalpha(c) && !islower(c)) {
            has_upper = true;
            break;
        }
    }

    if (!has_upper)
        return str;

    yy_str_t ret = {0};

    if (!alloc_str(ctx, str.str_val.len, &ret))
        return token_error(YY_ERROR_MEM);

    for (uint32_t i = 0; i < str.str_val.len; i++) {
        char *ptr = (char *)(ret.ptr + i);
        *ptr = tolower(str.str_val.ptr[i]);
    }

    return token_string(ret.ptr, ret.len);
}

static yy_token_t func_upper(yy_token_t str, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    bool has_lower = false;

    for (uint32_t i = 0; i < str.str_val.len; i++) {
        int c = str.str_val.ptr[i];
        if (isalpha(c) && !isupper(c)) {
            has_lower = true;
            break;
        }
    }

    if (!has_lower)
        return str;

    yy_str_t ret = {0};

    if (!alloc_str(ctx, str.str_val.len, &ret))
        return token_error(YY_ERROR_MEM);

    for (uint32_t i = 0; i < str.str_val.len; i++) {
        char *ptr = (char *)(ret.ptr + i);
        *ptr = toupper(str.str_val.ptr[i]);
    }

    return token_string(ret.ptr, ret.len);
}

static yy_token_t func_concat(yy_token_t str1, yy_token_t str2, yy_eval_ctx_t *ctx)
{
    if (str1.type != YY_TOKEN_STRING || !str1.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (str2.type != YY_TOKEN_STRING || !str2.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (!str1.str_val.len)
        return str2;

    if (!str2.str_val.len)
        return str1;

    yy_str_t ret = {0};
    uint32_t new_len = str1.str_val.len + str2.str_val.len;

    if (!alloc_str(ctx, new_len, &ret))
        return token_error(YY_ERROR_MEM);

    memcpy((char *) ret.ptr, str1.str_val.ptr, str1.str_val.len);
    memcpy((char *) ret.ptr + str1.str_val.len, str2.str_val.ptr, str2.str_val.len);

    return token_string(ret.ptr, ret.len);
}

static yy_token_t func_substr(yy_token_t str, yy_token_t start, yy_token_t len, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (start.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    if (len.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    uint32_t pos = CLAMP((int) start.number_val, 0, (int) str.str_val.len);
    const char *new_ptr = str.str_val.ptr + pos;
    uint32_t new_len = CLAMP((int) len.number_val, 0, (int)(str.str_val.len - pos));

    if (new_ptr == str.str_val.ptr && new_len == str.str_val.len)
        return str;

    if (!is_temp_ptr(ctx, new_ptr))
        return token_string(new_ptr, new_len);

    yy_str_t ret = {0};

    if (!duplicate_str(ctx, make_string(new_ptr, new_len), &ret))
        return token_error(YY_ERROR_MEM);

    return token_string(ret.ptr, ret.len);
}

static yy_token_t func_unescape(yy_token_t str, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (!str.str_val.len)
        return str;

    uint32_t len = str.str_val.len;
    uint32_t new_len = len;
    const char *ptr = str.str_val.ptr;
    char *new_ptr = NULL;

    // count escaped chars
    for (uint32_t i = 0; i < len - 1; i++) {
        if (ptr[i] == '\\' && (ptr[i+1] == '\\' || ptr[i+1] == '"' || ptr[i+1] == 't' || ptr[i+1] == 'n')) {
            new_len--;
            i++;
        }
    }

    if (len == new_len)
        return str;

    yy_str_t ret = {0};

    if (!alloc_str(ctx, new_len, &ret))
        return token_error(YY_ERROR_MEM);

    ptr = str.str_val.ptr;
    new_ptr = (char *) ret.ptr;

    // unescape string
    for (uint32_t i = 0, j = 0; i < len - 1; i++, j++)
    {
        if (ptr[i] == '\\')
        {
            switch (ptr[i+1])
            {
                case '"':  new_ptr[j] = '"';  i++; break;
                case '\\': new_ptr[j] = '\\'; i++; break;
                case 't':  new_ptr[j] = '\t'; i++; break;
                case 'n':  new_ptr[j] = '\n'; i++; break;
                default:   new_ptr[j] = ptr[i];
            }
        }
        else
            new_ptr[j] = ptr[i];
    }

    new_ptr[new_len - 1] = ptr[len - 1];

    return token_string(ret.ptr, ret.len);
}

static uint32_t str_count_ocurrences(const yy_str_t *str, const yy_str_t *substr)
{
    uint32_t ocurrences = 0;
    uint32_t len = str->len;
    const char *ptr = str->ptr;
    const char *aux = NULL;

    if (!str->len || !substr->len)
        return 0;

    while ((aux = (char *) memmem(ptr, len, substr->ptr, substr->len)) != NULL) {
        len -= (aux - ptr) + substr->len;
        ptr = aux + substr->len;
        ocurrences++;
    }

    return ocurrences;
}

static void str_replace(const yy_str_t str, const yy_str_t old_substr, const yy_str_t new_substr, yy_str_t *ret)
{
    uint32_t len = str.len;
    const char *src = str.ptr;
    const char *aux = NULL;
    char *dest = (char *) ret->ptr;

    assert(old_substr.len);

    while ((aux = (char *) memmem(src, len, old_substr.ptr, old_substr.len)) != NULL)
    {
        size_t num_bytes = aux - src;
        memcpy(dest, src, num_bytes);
        dest += num_bytes;
        memcpy(dest, new_substr.ptr, new_substr.len);
        dest += new_substr.len;
        len -= num_bytes + old_substr.len;
        src = aux + old_substr.len;
    }

    memcpy(dest, src, len);
}

static yy_token_t func_replace(yy_token_t str, yy_token_t old_substr, yy_token_t new_substr, yy_eval_ctx_t *ctx)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (old_substr.type != YY_TOKEN_STRING || !old_substr.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (new_substr.type != YY_TOKEN_STRING || !new_substr.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    int ocurrences = str_count_ocurrences(&str.str_val, &old_substr.str_val);

    if (!ocurrences)
        return str;

    size_t new_len = str.str_val.len + ocurrences * (new_substr.str_val.len - old_substr.str_val.len);

    if (new_len > UINT32_MAX)
        return token_error(YY_ERROR_VALUE);

    yy_str_t ret = {0};

    if (!alloc_str(ctx, new_len, &ret))
        return token_error(YY_ERROR_MEM);

    str_replace(str.str_val, old_substr.str_val, new_substr.str_val, &ret);

    return token_string(ret.ptr, ret.len);
}

// --- Functions returning a number

static yy_token_t func_length(yy_token_t str)
{
    if (str.type != YY_TOKEN_STRING || !str.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    uint32_t len = str.str_val.len;

    return token_number(len);
}

static yy_token_t func_find(yy_token_t needle, yy_token_t haystack, yy_token_t start)
{
    if (needle.type != YY_TOKEN_STRING || !needle.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (haystack.type != YY_TOKEN_STRING || !haystack.str_val.ptr)
        return token_error(YY_ERROR_VALUE);

    if (start.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    const char *ptr = (char *) memmem(haystack.str_val.ptr, haystack.str_val.len, needle.str_val.ptr, needle.str_val.len);

    yy_token_t ret = (!ptr ? token_error(YY_ERROR_VALUE) : token_number(ptr - haystack.str_val.ptr));

    return ret;
}

static yy_token_t func_datepart(yy_token_t date, yy_token_t part)
{
    if (date.type != YY_TOKEN_DATETIME)
        return token_error(YY_ERROR_VALUE);

    if (part.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    time_t time = (time_t)(date.datetime_val / 1000UL);
    struct tm stm = {0};

    gmtime_r(&time, &stm);

    switch((int) part.number_val)
    {
        case 0: return token_number(1900 + stm.tm_year);
        case 1: return token_number(stm.tm_mon + 1);
        case 2: return token_number(stm.tm_mday);
        case 3: return token_number(stm.tm_hour);
        case 4: return token_number(stm.tm_min);
        case 5: return token_number(stm.tm_sec);
        case 6: return token_number(date.datetime_val % 1000UL);
        default: return token_error(YY_ERROR_VALUE);
    }
}

static yy_token_t func_abs(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(fabs(x.number_val));
}

static yy_token_t func_ceil(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(ceil(x.number_val));
}

static yy_token_t func_floor(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(floor(x.number_val));
}

static yy_token_t func_trunc(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(trunc(x.number_val));
}

static yy_token_t func_sin(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(sin(x.number_val));
}

static yy_token_t func_cos(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(cos(x.number_val));
}

static yy_token_t func_tan(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(tan(x.number_val));
}

static yy_token_t func_exp(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(exp(x.number_val));
}

static yy_token_t func_log(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(log(x.number_val));
}

static yy_token_t func_sqrt(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(sqrt(x.number_val));
}

static yy_token_t func_minus(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(-x.number_val);
}

static yy_token_t func_ident(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return x;
}

static yy_token_t func_addition(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(x.number_val + y.number_val);
}

static yy_token_t func_subtraction(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(x.number_val - y.number_val);
}

static yy_token_t func_mult(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(x.number_val * y.number_val);
}

static yy_token_t func_div(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_number(x.number_val / y.number_val);
}

static yy_token_t func_mod(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    double val = fmod(x.number_val, y.number_val);

    return token_number(val);
}

static yy_token_t func_pow(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_NUMBER || y.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    double val = pow(x.number_val, y.number_val);

    return token_number(val);
}

// --- Functions returning a boolean

static yy_token_t func_not(yy_token_t x)
{
    if (x.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    return token_bool(!x.bool_val);
}

static yy_token_t func_iserror(yy_token_t x)
{
    return token_bool(x.type == YY_TOKEN_ERROR);
}

static yy_token_t func_isinf(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_bool(isinf(x.number_val));
}

static yy_token_t func_isnan(yy_token_t x)
{
    if (x.type != YY_TOKEN_NUMBER)
        return token_error(YY_ERROR_VALUE);

    return token_bool(isnan(x.number_val));
}

static int str_cmp(const yy_str_t str1, const yy_str_t str2)
{
    int ret = strncmp(str1.ptr, str2.ptr, MIN(str1.len, str2.len));

    if (ret != 0)
        return ret;

    return (str1.len - str2.len);
}

static yy_token_t func_lt(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val < y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val < y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) < 0);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_le(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val <= y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val <= y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) <= 0);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_gt(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val > y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val > y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) > 0);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_ge(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val >= y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val >= y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) >= 0);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_eq(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val == y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val == y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) == 0);

    if (x.type == YY_TOKEN_BOOL)
        return token_bool(x.bool_val == y.bool_val);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_ne(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
        return token_bool(x.number_val != y.number_val);

    if (x.type == YY_TOKEN_DATETIME)
        return token_bool(x.datetime_val != y.datetime_val);

    if (x.type == YY_TOKEN_STRING && x.str_val.ptr && y.str_val.ptr)
        return token_bool(str_cmp(x.str_val, y.str_val) != 0);

    if (x.type == YY_TOKEN_BOOL)
        return token_bool(x.bool_val != y.bool_val);

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_and(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    return token_bool(x.bool_val && y.bool_val);
}

static yy_token_t func_or(yy_token_t x, yy_token_t y)
{
    if (x.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    if (y.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    return token_bool(x.bool_val || y.bool_val);
}

// --- Functions returning a variable

static yy_token_t func_variable(yy_token_t str)
{
    if (str.type != YY_TOKEN_STRING)
        return token_error(YY_ERROR_VALUE);

    str.type = YY_TOKEN_VARIABLE;

    return str;
}

// --- Functions whose return type depends on parameters

static yy_token_t func_min(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
    {
        double val = fmin(x.number_val, y.number_val);
        return token_number(val);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        uint64_t val = MIN(x.datetime_val, y.datetime_val);
        return token_datetime(val);
    }

    if (x.type == YY_TOKEN_STRING)
    {
        return (str_cmp(x.str_val, y.str_val) < 0 ? x : y);
    }

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_max(yy_token_t x, yy_token_t y)
{
    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
    {
        double val = fmax(x.number_val, y.number_val);
        return token_number(val);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        uint64_t val = MAX(x.datetime_val, y.datetime_val);
        return token_datetime(val);
    }

    if (x.type == YY_TOKEN_STRING)
    {
        return (str_cmp(x.str_val, y.str_val) < 0 ? y : x);
    }

    return token_error(YY_ERROR_VALUE);
}

static yy_token_t func_ifelse(yy_token_t cond, yy_token_t x, yy_token_t y)
{
    if (cond.type != YY_TOKEN_BOOL)
        return token_error(YY_ERROR_VALUE);

    if (x.type != y.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type != YY_TOKEN_NUMBER && x.type != YY_TOKEN_DATETIME && x.type != YY_TOKEN_STRING)
        return token_error(YY_ERROR_VALUE);

    return (cond.bool_val ? x : y);
}

static yy_token_t func_clamp(yy_token_t x, yy_token_t vmin, yy_token_t vmax)
{
    if (x.type != vmin.type || x.type != vmax.type)
        return token_error(YY_ERROR_VALUE);

    if (x.type == YY_TOKEN_NUMBER)
    {
        double val = CLAMP(x.number_val, vmin.number_val, vmax.number_val);
        return token_number(val);
    }

    if (x.type == YY_TOKEN_DATETIME)
    {
        uint64_t val = CLAMP(x.datetime_val, vmin.datetime_val, vmax.datetime_val);
        return token_datetime(val);
    }

    return token_error(YY_ERROR_VALUE);
}
