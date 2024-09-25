# expr

An expression parser supporting multiple types.

_Key Features_:

* Multiple types (number, bool, datetime, string and error)
* Memory managed by user (no allocs)
* Iterator based interface
* Supporting variables
* Stateless
* Expressions can be compiled ([RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation) stack)
* Fully compile-time checked syntax
* Documented [grammar](grammar.md)
* Standard C11 code
* No dependencies

## Examples

```txt
# Numerical calculations
sin((-1 + 2) * PI)

# Dates
datetrunc(now(), "day")

# Strings
"hi " + upper("bob")  + trim("  !  ")

# Conditionals
ifelse(1 < 5 && length($alphabet) > 25, "case1", "case2")

# Find the missing letter
replace($alphabet, substr($alphabet, 25 - random(0, length($alphabet)), 1), "")
```

## Usage

```C
#include <stdio.h>
#include <string.h>
#include "expr.h"

int main()
{
    yy_token_t data[64] = {0};
    yy_stack_t stack = {.data = data, .reserved = 64};

    const char *txt = "1 + 1";
    yy_token_t result = yy_eval_number(txt, txt + strlen(txt), &stack, NULL, NULL);

    printf("%s = %g\n", txt, result.number_val);
}
```

Compile previous code using:

```bash
gcc -o example example.c expr.c -lm
```

## Build

Follow these steps to compile and run the tests and examples.

```bash
make
cd build
tests
calc
```

## Contributors

| Name | Contribution |
|:-----|:-------------|
| [Gerard Torrent](https://github.com/torrentg/) | Initial work<br/>Code maintainer|
| [skeeto](https://www.reddit.com/user/skeeto/) | [Fuzzer analysis (reddit post)](https://www.reddit.com/r/C_Programming/comments/1fhigj8/a_simple_expression_parser_supporting_multiple/) |

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
