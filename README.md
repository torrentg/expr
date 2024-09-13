# expr

A simple expression parser supporting multiple types.

_Features_:

* Multiple types (number, bool, datetime, string and error)
* Memory managed by user (no allocs)
* Iterator based interface
* Variables are supported
* Expressions can be compiled once ([RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation) stack) and subsequently evaluated multiple times
* Syntax checked at compile time (ex. `1 + "str"` fails)
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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
