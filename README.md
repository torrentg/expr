# expr

A simple expressions parser.

Features:

* Supports multiple types (number, bool, time, string and error)
* Memory managed by user (no allocs)
* Iterator based interface
* No dependencies

## Examples

```txt
(1 + ifelse($x == "John", 3, 25)) * datepart(now(), "day")
```

## Grammar for types

```txt
number      = (0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?(0|[1-9][0-9]*))?
datetime    = '"' (19[7-9][0-9]|2[0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])(T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(.[0-9]{1,3}Z?)?)? '"'
boolean     = 'true' | 'True' | 'TRUE' | 'false' | 'False| 'FALSE'
string      = '"' ([^\\] | '\n' | '\t' | '\"' | '\\')* '"'
variable    = '$' ( [a-zA-Z][a-zA-Z0-9_]* | '{' [^{}]+ '}' )
timePart    = '"' ('year' | 'month' | 'day' | 'hour' | 'minute' | 'second' | 'millis') '"'
```

Number: [double-precision floating point](https://en.wikipedia.org/wiki/Double-precision_floating-point_format) in JSON-format ([RFC-7159](https://tools.ietf.org/html/rfc7159) , section 6 -numbers-)

* Examples: `42`, `-0.5`, `314e-2`, `0.314e1`
* Not supported: ~~`03`, `.5`, `42.`, `1.e1`, `1e04`~~

Datetime: UTC millis from epoch-time in [ISO-8601](https://en.wikipedia.org/wiki/ISO_8601)

* Examples: `2024-08-24T08:19:25.402Z`, `2024-08-24T08:19:25.402`, `2024-08-24T08:19:25`, `2024-08-24`
* Not supported: ~~`24/08/2024`, `08/24/2024`, `24 August 2024`, etc.~~

String: double-quoted string supporting escape characters (`\n`, `\t`, `\"`, `\\`)

* Examples: `"I am a string"`, `"val=\"xxx\""`
* Not supported: A string surrounded by quotes containing a NUL.

Boolean: `true`, `True`, `TRUE`, `false`, `False`, `FALSE`

* Examples: `true`, `false`
* Not supported: ~~`1`, `0`, `TrUe`~~

Variable: Variable name prefixed by `'$'`

* Examples: `$x`, `$sum_values`, `${x}`, `${free var name, yep}`
* Not supported: ~~`$a-b`, `$_a`, `${x}d}`, `${}`~~

## Grammar for numeric expressions

```txt
numExpr     = numTerm (numInfixOp numTerm)*
numTerm     = number | 
              numConst | 
              varExpr | 
              numFunc | 
              '(' numExpr ')' | 
              numPrefixOp numTerm
numInfixOp  = '+' | '-' | '*' | '/' | '^' | '%'
numPrefixOp = '+' | '-'
numFunc     = <see list below>
numConst    = <see list below>
```

> Consecutive operators are not supported (example: `1+-1` is not valid)

| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| number   | `-`          | (numExpr)                     | Minus (infix operator)                   |
| number   | `abs`        | (numExpr)                     | Absolute value                           |
| number   | `sin`        | (numExpr)                     | Sine                                     |
| number   | `cos`        | (numExpr)                     | Cosine                                   |
| number   | `tan`        | (numExpr)                     | Tangent                                  |
| number   | `sqrt`       | (numExpr)                     | Square root                              |
| number   | `exp`        | (numExpr)                     | Base-e exponential                       |
| number   | `log`        | (numExpr)                     | Natural logarithm                        |
| number   | `ceil`       | (numExpr)                     | Smallest int not less than num           |
| number   | `floor`      | (numExpr)                     | Largest int not less than num            |
| number   | `trunc`      | (numExpr)                     | Round to integer, toward zero            |
| number   | `+`          | (numExpr, numExpr)            | Addition                                 |
| number   | `-`          | (numExpr, numExpr)            | Subtraction                              |
| number   | `*`          | (numExpr, numExpr)            | Multiplication                           |
| number   | `/`          | (numExpr, numExpr)            | Division                                 |
| number   | `%`, `mod`   | (numExpr, numExpr)            | Modulus                                  |
| number   | `^`, `pow`   | (numExpr, numExpr)            | Power                                    |
| number   | `min`        | (numExpr, numExpr)            | Returns the smaller of two given values  |
| number   | `max`        | (numExpr, numExpr)            | Returns the larger of two given values   |
| number   | `random`     | (numExpr, numExpr)            | Returns a random value in range [min,max)|
| number   | `clamp`      | (numExpr, numExpr, numExpr)   | Coerce a value to be in a fixed range    |
| number   | `length`     | (strExpr)                     | Length of a string                       |
| number   | `find`       | (strExpr, strExpr, numExpr)   | Locate a text string into another text   |
| number   | `datepart`   | (timeExpr, timePart)          | Returns a part from a date               |
| number   | `ifelse`     | (boolExpr, numExpr, numExpr)  | Conditional value                        |

| Type   | Constant | Value               |
| ------ | -------- | --------------------|
| number | `E`      | 2.71828...          |
| number | `PI`     | 3.14159...          |
| number | `Inf`    | Infinite            |
| number | `NaN`    | Not-a-number        |

## Grammar for datetime expressions

```txt
timeExpr    = timeTerm
timeTerm    = datetime | 
              varExpr | 
              timeFunc
timeFunc    = <see list below>
```

> Parenthesis are not supported because there are not operators nor precedence to consider.

| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| datetime | `now`        | (\<none\>)                    | Current UTC time                         |
| datetime | `dateadd`    | (timeExpr, numExpr, timePart) | Increments/decrements a date part        |
| datetime | `dateset`    | (timeExpr, numExpr, timePart) | Modifies a date part                     |
| datetime | `datetrunc`  | (timeExpr, timePart)          | Returns a date truncated to datepart     |
| datetime | `clamp`      | (timeExpr, timeExpr, timeExpr)| Coerce a datetime to be in a fixed range |
| datetime | `min`        | (timeExpr, timeExpr)          | Returns the smaller of two given values  |
| datetime | `max`        | (timeExpr, timeExpr)          | Returns the larger of two given values   |
| datetime | `ifelse`     | (boolExpr, timeExpr, timeExpr)| Conditional value                        |

## Grammar for string expressions

```txt
strExpr     = strTerm ('+' strTerm)*
strTerm     = string | 
              varExpr | 
              strFunc | 
              '(' strExpr ')'
strFunc     = <see list below>
```

| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| string   | `trim`       | (strExpr)                     | Removes leading and trailing whitespaces |
| string   | `lower`      | (strExpr)                     | Converts all characters to lowercase     |
| string   | `upper`      | (strExpr)                     | Converts all characters to uppercase     |
| string   | `unescape`   | (strExpr)                     | Replaces '\\\\', '\\n', '\\t' and '\\"'  |
| string   | `+`          | (strExpr, strExpr)            | Concatenate two strings                  |
| string   | `min`        | (strExpr, strExpr)            | Returns the smaller of two given values  |
| string   | `max`        | (strExpr, strExpr)            | Returns the larger of two given values   |
| string   | `substr`     | (strExpr, numExpr, numExpr)   | Extracts a substring from a given string |
| string   | `replace`    | (strExpr, strExpr, strExpr)   | Replaces all ocurrences of x in a string |
| string   | `ifelse`     | (boolExpr, strExpr, strExpr)  | Conditional value                        |
| string   | `str`        | (numExpr)<br/>(timeExpr)<br/>(boolExpr)<br/>(strExpr) | Converts value to string                 |

## Grammar for boolean expressions

```txt
boolExpr     = boolTerm (boolInfixOp boolTerm)*
boolTerm     = (numExpr | timeExpr | strExpr) <  (numExpr | timeExpr | strExpr) |
               (numExpr | timeExpr | strExpr) <= (numExpr | timeExpr | strExpr) |
               (numExpr | timeExpr | strExpr) >  (numExpr | timeExpr | strExpr) |
               (numExpr | timeExpr | strExpr) >= (numExpr | timeExpr | strExpr) |
               (numExpr | timeExpr | strExpr) == (numExpr | timeExpr | strExpr) |
               (numExpr | timeExpr | strExpr) != (numExpr | timeExpr | strExpr) |
               boolean | 
               varExpr |
               boolFunc |
               '(' boolExpr ')'
boolInfixOp  = '&&' | '||' | '==' | '!='
boolFunc     = <see list below>
```

| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| boolean  | `&&`         | (boolExpr, boolExpr)          | And                                      |
| boolean  | `\|\|`       | (boolExpr, boolExpr)          | Or                                       |
| boolean  | `<`          | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr)   | Less-than             |
| boolean  | `<=`         | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr)   | Less-than-or-equal    |
| boolean  | `>`          | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr)   | Greater-than          |
| boolean  | `>=`         | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr)   | Greater-than-or-equal |
| boolean  | `==`         | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr) <br/> (boolExpr, boolExpr)  | Equal-to           |
| boolean  | `!=`         | (numExpr, numExpr) <br/> (timeExpr, timeExpr) <br/> (strExpr, strExpr) <br/> (boolExpr, boolExpr)  | Not-equal-to       |
| boolean  | `not`        | (boolExpr)                    | Negate                                   |
| boolean  | `isinf`      | (numExpr)                     | Checks if a number is &plusmn; infinite  |
| boolean  | `isnan`      | (numExpr)                     | Checks if a number is a NaN              |
| boolean  | `iserror`    | (numExpr) <br/> (timeExpr) <br/> (boolExpr) <br/> (strExpr) | Checks if there is an error              |
| boolean  | `ifelse`     | (boolExpr, boolExpr, boolExpr)| Conditional value                        |

## Grammar for variable expressions

```txt
varExpr      = varTerm
varTerm      = variable | 
               varFunc
varFunc      = <see list below>
```

| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| variable | `variable`   | (strExpr)                     | Creates a reference to the variable name |

> Utility function to create references to variables on-the-fly.<br/>
> `variable("x[" + str($index) + "]")` &rarr; `${x[1]}`

## Operators precedence

Expr implements the [standard](https://en.cppreference.com/w/c/language/operator_precedence) operators precedence

| Precedence  | Type             | Symbols      | Associativity  |
|:----------: | ---------------- | -------------| :------------: |
|     1       | Grouping         | (, )         | left-to-right  |
|     2       | Power            | ^            | left-to-right  |
|     3       | Not, plus, minus | !, +, -      | right-to-left  |
|     4       | Prod, div, mod   | *, /, %      | left-to-right  |
|     5       | Add, subtract    | +, -         | left-to-right  |
|     6       | Comparison       | <, <=, >, >= | left-to-right  |
|     7       | Equal            | ==, !=       | left-to-right  |
|     8       | And              | &&           | left-to-right  |
|     9       | Or               | \|\|         | left-to-right  |
