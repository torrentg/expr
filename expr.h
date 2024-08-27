/*
MIT License

expr -- A simple expression parser.
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

#ifndef EXPR_H
#define EXPR_H

/**
 * Operators:
 * 
 * @see https://en.wikipedia.org/wiki/Operators_in_C_and_C%2B%2B
 * 
 *    Precedence Type                 Symbols            Associativity
 *    ----------------------------------------------------------------
 *         1     Grouping             ()                 left-to-right 
 *         2     Power                ^                  left-to-right (like octave)
 *         3     Not, plus minus      !, +, -            right-to-left 
 *         4     Prod, div, mod       *, /, %            left-to-right 
 *         5     Add, subtract        +, -               left-to-right 
 *         6     comparison           <, <=, >, >=       left-to-right
 *         7     equal                ==, !=             left-to-right
 *         8     and                  &&                 left-to-right
 *         9     or                   ||                 left-to-right
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER) 
    #define PACKED     __attribute__((__packed__))
#else
    #define PACKED     /**/
#endif

typedef enum yy_retcode_e {
    YY_OK,                          //!< No errors.
    YY_ERROR,                       //!< Generic error.
    YY_ERROR_ARGS,                  //!< Invalid arguments.
    YY_ERROR_EMPTY,                 //!< Empty expression.
    YY_ERROR_SYNTAX,                //!< Syntax error.
    YY_ERROR_EVAL,                  //!< Evaluation error.
    YY_ERROR_MEM,                   //!< Not enough memory.
    YY_ERROR_INVALID_BOOLEAN,       //!< Invalid boolean.
    YY_ERROR_INVALID_NUMBER,        //!< Invalid number.
    YY_ERROR_INVALID_DATETIME,      //!< Invalid timestamp.
    YY_ERROR_INVALID_STRING,        //!< Invalid string.
    YY_ERROR_INVALID_VARIABLE,      //!< Invalid variable.
    YY_ERROR_RANGE_NUMBER,          //!< Number out-of-range.
    YY_ERROR_RANGE_DATETIME,        //!< Timestamp out-of-range.
} yy_retcode_e;

typedef struct PACKED yy_str_t {
    const char *ptr;                //!< Pointer to data (not NUL-ended).
    uint32_t len;                   //!< String length.
} yy_str_t;

typedef struct PACKED yy_func_t {
    void (*ptr)(void);              //!< Pointer to function.
    uint8_t num_args;               //!< Number of arguments.
    uint8_t precedence;             //!< Operator precedence.
    bool right_to_left;             //!< Operator associativity.
} yy_func_t;

typedef enum yy_token_e {
    YY_TOKEN_NULL,                  //!< Unassigned token.
    YY_TOKEN_BOOL,                  //!< Bool value.
    YY_TOKEN_NUMBER,                //!< Number value.
    YY_TOKEN_DATETIME,              //!< Timestamp value.
    YY_TOKEN_STRING,                //!< String value.
    YY_TOKEN_VARIABLE,              //!< Variable.
    YY_TOKEN_FUNCTION,              //!< Function.
    YY_TOKEN_ERROR,                 //!< Evaluation error.
} yy_token_e;

typedef enum yy_error_e {
    YY_ERROR_REF,                   //!< Variable not found.
    YY_ERROR_NAN,                   //!< Not a number (ex: 0/0).
    YY_ERROR_HUGE,                  //!< Numeric overflow or +inf or -inf.
    YY_ERROR_DIV_0,                 //!< Number divided by 0.
    YY_ERROR_VALUE,                 //!< Invalid value (ex: variable contains unexpected type).
} yy_error_e;

typedef struct yy_token_t {
    union PACKED
    {
        bool bool_val;              //!< Boolean value.
        double number_val;          //!< Number value.
        uint64_t datetime_val;      //!< Timestamp value (millis from epoch-time).
        yy_str_t str_val;           //!< String value.
        yy_str_t variable;          //!< Variable name.
        yy_func_t function;         //!< Function data.
        yy_error_e error;           //!< Error type.
    };
    yy_token_e type;                //!< Token type (bool, number, etc.).
} yy_token_t;

typedef struct yy_stack_t {
    yy_token_t *data;               //!< Tokens list.
    uint32_t reserved;              //!< Numbers of allocated tokens.
    uint32_t len;                   //!< Number of tokens in the stack.
} yy_stack_t;

/**
 * Parse a single value.
 * 
 * Use these functions to parse variable content, not expressions.
 * They are used to validate that string data adheres to the expected syntax. 
 * If you just want to create a token, don't use these functions, instead 
 * assign the type and value directly.
 *
 * There is not a parser for variables because variables appears always into
 * expressions.
 * 
 * Caution, strings and datetimes surrounded by double-quote are reported as error.
 * 
 * Features:
 *   - number: parse JSON-format numbers (RFC-7159).
 *   - datetime: parse ISO-8601 datetimes (ex. 2024-08-24T09:05:58.123Z, 2024-08-24, etc.).
 *   - string: parse non-quoted escaped strings (\n \t \" \\).
 *   - bool: accepted values: true, True, TRUE, false, False, FALSE
 * 
 * @param[in] begin String to parse (without initial spaces).
 * @param[in] end One char after the string end.
 * 
 * @return Parsed value,
 *         On error, token.type == YY_TOKEN_ERROR.
 */
yy_token_t yy_parse_number(const char *begin, const char *end);
yy_token_t yy_parse_datetime(const char *begin, const char *end);
yy_token_t yy_parse_string(const char *begin, const char *end);
yy_token_t yy_parse_bool(const char *begin, const char *end);
yy_token_t yy_parse(const char *begin, const char *end);

/**
 * Compile an expression.
 * 
 * Parse full content (^.*$).
 * 
 * In the generic case, yy_compile(), we try to parse it in the following order:
 *   number, datetime, string, bool
 * 
 * Caution, variables and strings on the stack point to str input.
 * Recode these values before deallocating str.
 * 
 * @param[in] begin String to parse (initial spaces are allowed).
 * @param[in] end One char after the string end.
 * @param[out] stack Reverse polish notation (rpn) stack.
 * @param[out] err Error location (can be NULL).
 * 
 * @return YY_OK on success, 
 *         otherwise error (in this case, if err is not NULL, err points to the error location).
 */
yy_retcode_e yy_compile_number(const char *begin, const char *end, yy_stack_t *stack, const char **err);
yy_retcode_e yy_compile_datetime(const char *begin, const char *end, yy_stack_t *stack, const char **err);
yy_retcode_e yy_compile_string(const char *begin, const char *end, yy_stack_t *stack, const char **err);
yy_retcode_e yy_compile_bool(const char *begin, const char *end, yy_stack_t *stack, const char **err);
yy_retcode_e yy_compile(const char *begin, const char *end, yy_stack_t *stack, const char **err);

/**
 * Evaluate a rpn stack.
 * 
 * @param[in] stack Reverse polish notation (rpn) stack.
 * @param[in] aux Auxiliary stack (used to evaluate the rpn stack and store intermediate string values).
 * @param[in] resolve Function used to resolve variables (can be NULL if there are no variables).
 * @param[in] data Data passed to the 'resolve' function (can be NULL).
 * 
 * @return Result as token, can be an error (see yy_token_t.type).
 */
yy_token_t yy_eval(const yy_stack_t *stack, yy_stack_t *aux, yy_token_t (*resolve)(yy_str_t *var, void *data), void *data);

/**
 * Returns the textual description of the error code.
 * 
 * @param[in] errnum Code error.
 * @return Textual description.
 */
const char * yy_strerror(yy_retcode_e errnum);

#endif
