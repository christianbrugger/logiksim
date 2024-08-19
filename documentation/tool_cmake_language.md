

# CMake Language

Here the features of the CMake language are documented used by the project.







References:

* [cmake-language](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html)

* [Learn CMake's Scripting Language in 15 Minutes](https://preshing.com/20170522/learn-cmakes-scripting-language-in-15-minutes/)





### Statements / Commands

Every line in CMake file is a comment or statement.

Takes a list of **string arguments** and has **no return value**.

- No operators are supported in CMake.
  
  - For calculation use [math](https://cmake.org/cmake/help/latest/command/math.html)
  
  - For string manipulation use [string](https://cmake.org/cmake/help/latest/command/string.html)
  
  - For file manipulation [file](https://cmake.org/cmake/help/latest/command/file.html)
* Quote variable references are never split

* Single unquotes variable references are parsed as list
  `func($test)   =>    func(abc;c e)   =>   func(abc "c e")`

* `if` and `while` use value of variable for unquoted names

### Comment

### Bracket Coment

```cmake
#[[This is a bracket comment.
It runs until the close bracket.]]
message("First Argument\n" #[[Bracket Comment]] "Second Argument")##### Line Comment
```

#### Line Comment

```cmake
message("First Argument\n" # This is a line comment :)
        "Second Argument") # This is a line comment.
```

### Identation

Identation is totally optional and completely ignored.

### Naming Conventions

**????**

* Functions: best all lowercase names with underscores

* Variables: 

* Modules: CamelCase

* Module Functions: `MyModule_Some_Function(...)`

### Variables

Name can contain any text. Recommended:

```
variable ::= [a-zA-Z0-9_-]+
```

* Value is **always** string.

* Case sensitive.

* To set variables with space they need to be quoted

```cmake
set(<variable> value)
unset(<variable>)


Best:
set(NAME "value")  # NOT: set("NAME" value)
```

Scopes:

* block scope `block() ... endblock()`

* function scope: Only within the same or nested function. Not outside.

* Directory scope: Parent directory variables are copied to child. Not otherway.

* Persistent cache: Requires `set(... CACHE ...)`

Default scope outside of functions is *Directory scope*.

Cache variables:

* Stored in `CMakeCache.txt`

* Global scope **?????**

Can be set via the command line:

```shell
cmake -DNAME=value ...
```

More reading:

[CMake syntax to set and use variables? - Stack Overflow](https://stackoverflow.com/a/31044116/476266)

### Properties

Global, non-cached, variables can be set with:

```cmake
set_property(GLOBAL PROPERTY NAME VALUE)
```

List of all properties:

[cmake-properties](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html)



### Lists

Certain string values are evaluated as lists:

Set

```cmake
set(NAME abc cde def) ==> NAME=abc;cde;def
```

* Nested lists not supported

* There is no generally accepted escape sequence for semicolon. Function define them on a case by case basis.

* Do not use unquoted arguments for functions, as it will be split! **??????**
  Avoid: `call(${})` Better: `call("${var}")`

* Macos okay **????**

List manipulation via [list](https://cmake.org/cmake/help/latest/command/list.html)

```cmake
list(LENGTH <list> <out-var>)

list(APPEND <list> [<element>...])
```

### Buildin Variables

```cmake
CMAKE_*           # any case
_CMAKE_*          # any case
_<cmake-command>  # e.g. if, include, add_test, 
```

All variables:

https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html

### Environment Variables

* Not scoped, all global.

* Initialized by calling process.

* Can be changed with set(), unset(). Do seen by parent or child processes.

* Not cached or preserved for future runs.

Inspect

```shell
cmake -E environment
```

Set manually

https://cmake.org/cmake/help/latest/manual/cmake.1.html#cmdoption-cmake-E-arg-env

```cmake
set(ENV{<name>} VALUE)
```

[cmake-env-variables](https://cmake.org/cmake/help/latest/manual/cmake-env-variables.7.html)

### Variable References

#### Normal Variable Reference

Inside:

* quoted argument: `"test ${<variable>} test"`

* unquoted argument `${<variable>}`

* Also `abc${inner}` possible **????**

```cmake
${<variable>}
```

Nested evaluation is supported => Emulate structs

```cmake
${outer_${inner}_suffix}
```

What if it does not exist **?????**

Evaluation order:

* Searches the function call stack.

* Searches current directory.

* Searches Cache.

* Returns empty string.

#### Environment Variable Reference

```cmake
$ENV{<variable>}
```

What if it does not exist **?????**

#### Cache Variable Reference

Skips checking of variables.

```cmake
$CACHE{<variable>}
```

Empty string if it does not exist.

### Control Structures

### Condition Expression

The content of `if`, `elseif`,  `while` are specially evaluated:

* True: `1`, `ON`, `YES`, `TRUE`, `Y` or number > 0 (including float)

* Flase: `0`, `OFF`, `NO`, `FALSE`, `N`, `IGNORE`, `NOTFOUND`, `""`, suffix `-NOTFOUND`

* Named booleans above: case-insensitive

[Condition-Syntax](https://cmake.org/cmake/help/latest/command/if.html#condition-syntax)



#### Conditionals

```cmake
if()
elseif()
else()
endif()
```

Unquoted names read the variable content:

```cmake
if(A LESS "100")

<equivalent to>


if("${A}" LESS "1000")
```

Anything quoted is never evaluated, sincd 3.1.

#### Loops

```cmake
foreach()
    break()
    continue()
endforeach()
```

Takes 1+ arguments. Loops over arguments and assigns it to the first:

```cmake
foreach(ARG abc cde)
    message("${ARG}")   # printstwo lines: abc \n cde
endforeach()
```

Lists are split

```cmake
set(MY_LIST abc cde)

foreach(ARG ${MY_LIST})
    message("${ARG}")   # printstwo lines: abc \n cde
endforeach()
```

```cmake
while()
    break()
    continue()
endwhile()
```

### Defining Commands

* Function and macro names are **case-insensitive**

#### Macro

```cmake
macro()

endmacro()
```

* Dont define their own scope. Variable `set` and references use *directory scope*.

* Otherwise same as functions.

* Macro **arguments are not variables**
  
  * They cannot be used in `if` **?????**
  
  * How to use them **????**

#### Function

```cmake
function(name [<argument>...])

endfunction()
```

Can be nested:

```cmake
function()

    function()

        ...
    endfunction()    

endfunction()
```

Return value convention:

```cmake
function(init_list RESULT)
    set("${RESULT}" "abc" "cde" PARENT_SCOPE)  # sets it in caller scope
endfunction()


init_list("MY_LIST")
message("${MY_LIST}")    # prints: abc;cde
```

* If no arguments are defined accepts any amount.

* If defined, calling function with incorrect number throws error.

Arbitrary number of arguments. Function needs to access `ARGN`

```cmake
function(test)
    message("${ARGN}")
endfunction()

test("abc" "cde")    # prints abc;cde
```

Recommended: **Better use ARGC and ARG#**, see [function](https://cmake.org/cmake/help/latest/command/function.html#arguments)

### Generator Expressions

* Only evaluated during initial cmake run



### Buildin Functions

```cmake
message("Hello World!")
```

### Standard Library

Build in modules:

[cmake-modules](https://cmake.org/cmake/help/latest/manual/cmake-modules.7.html)

Useful:

* [CMakePrintHelpers](https://cmake.org/cmake/help/latest/module/CMakePrintHelpers.html)

### Debuggin

print style

```cmake
message("message")
```

View variable cache

```shell
cat CMakeCache.txt
```

Inspect scope

```cmake
message(${VARIABLES})
```

Watch

```cmake
variable_watch(NAME)
```

Trace

```shell
cmake --trace ...
```
