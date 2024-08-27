<style>
table { margin: 1em };
</style>

# expr
A simple expressions parser.

Features:

* Iterator based interface.
* Supports multiple types (number, bool, time, string and error)
* Memory managed by user (no allocs)

__Grammar for types__

```
number      = (0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?(0|[1-9][0-9]*))?
datetime    = (19[7-9][0-9]|2[0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])(T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(.[0-9]{1,3})?)?
boolean     = 'true' | 'True' | 'TRUE' | 'false' | 'False| 'FALSE'
string      = '"' ([^\\] | '\n' | '\t' | '\"' | '\\')* '"'
variable    = '${' [a-zA-Z]('.'? [a-zA-Z0-9_]+)* '}'
timePart    = '"' ('year' | 'month' | 'day' | 'hour' | 'minute' | 'second' | 'millis') '"'
```

__Grammar for numeric expressions__

```
numExpr     = numTerm (numInfixOp numTerm)*
numTerm     = number | variable | numConst | numFunc | '(' numExpr ')' | numPrefixOp numTerm
numInfixOp  = '+' | '-' | '*' | '/' | '^' | '%'
numPrefixOp = '+' | '-'
numFunc     = <see list below>

We reject 2 consecutive operators (ex: 1 + -1).
```

__Grammar for time expressions__

```
timeTerm    = timeVal | variable | timeFunc
timeFunc    = <see list below>
timeVal     = '"' datetime '"'

Parenthesis are not supported because there are not operators nor precedence to consider.
```

__Operators precedence__

| Precedence  | Type             | Symbols      | Associativity  |
|:----------: | ---------------- | -------------| :------------- |
|     1       | Grouping         | ()           | left-to-right  |
|     2       | Power            | ^            | left-to-right  |
|     3       | Not, plus, minus | !, +, -      | right-to-left  |
|     4       | Prod, div, mod   | *, /, %      | left-to-right  |
|     5       | Add, subtract    | +, -         | left-to-right  |
|     6       | comparison       | <, <=, >, >= | left-to-right  |
|     7       | equal            | ==, !=       | left-to-right  |
|     8       | and              | &&           | left-to-right  |
|     9       | or               | \|\|         | left-to-right  |

<br/>

__List of supported constants__

| Type   | Name     | Value               |
| ------ | -------- | --------------------|
| number | `E`      | 2.71828...          |
| number | `PI`     | 3.14159...          |

__List of supported functions__

   
| Return   | Function     | Params                        | Description                              |
| -------- | ------------ | ----------------------------- | -------------------------------------    |
| datetime | `now`        | (\<none\>)                    | Current UTC time                         |
| datetime | `dateadd`    | (timeTerm, numExpr, timePart) | Increments/decrements a date part        |
| datetime | `dateset`    | (timeTerm, numExpr, timePart) | Modifies a date part                     |
| datetime | `datetrunc`  | (timeTerm, timePart)          | Returns a date truncated to datepart     |
| number   | `datepart`   | (timeTerm, timePart)          | Returns a part from a date               |
| number   | `+`          | (numExpr, numExpr)            | Addition                                 |
| number   | `-`          | (numExpr, numExpr)            | Subtraction                              |
| number   | `*`          | (numExpr, numExpr)            | Multiplication                           |
| number   | `/`          | (numExpr, numExpr)            | Division                                 |
| number   | `%`, `mod`   | (numExpr, numExpr)            | Modulus                                  |
| number   | `^`, `pow`   | (numExpr, numExpr)            | Power                                    |
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
| number   | `length`     | (strTerm)                     | Length of a string                       |
| string   | `trim`       | (strTerm)                     | Removes leading and trailing whitespaces |
| string   | `lower`      | (strTerm)                     | Converts all characters to lowercase     |
| string   | `upper`      | (strTerm)                     | Converts all characters to uppercase     |
| string   | `concat`     | (strTerm, strTerm)            | Concatenate two strings                  |
| string   | `substr`     | (strTerm, numExpr, numExpr)   | Extracts a substring from a given string |
| boolean  | `!`          | (boolExpr)                    | Negate                                   |
| boolean  | `&&`         | (boolExpr, boolExpr)          | And                                      |
| boolean  | `\|\|`       | (boolExpr, boolExpr)          | Or                                       |
| boolean  | `<`          | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm)   | Less-than |
| boolean  | `<=`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm)   | Less-than-or-equal |
| boolean  | `>`          | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm)   | Greater-than       |
| boolean  | `>=`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm)   | Greater-than-or-equ|
| boolean  | `==`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm) <br/> (boolExpr, boolExpr)  | Equal-to           |
| boolean  | `!=`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm) <br/> (boolExpr, boolExpr)  | Not-equal-to       |
| number <br/> datetime <br/> string  | `min`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm) | Returns the smaller of two given values |
| number <br/> datetime <br/> string  | `max`         | (numExpr, numExpr) <br/> (timeTerm, timeTerm) <br/> (strTerm, strTerm) | Returns the larger of two given values |
