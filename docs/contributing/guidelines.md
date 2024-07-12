# Coding guidelines

## Naming convention

- Variables and functions are named using the `snake_case` naming convention.
- Namespaces and classes are named using the `PascalCase` naming convention.
- Macros and enum values are named using the `SCREAMING_SNAKE_CASE` naming convention.

## Macros

When avoidable, try not to use `#define`s to define constants as they are not namespaced. Instead, prefer using enums.

Do this:

```cpp

namespace SomeNamespace {
    enum MyEnum {
        SOME_VALUE = 1,
    };
}
```

Not that:

```cpp
namespace SomeNamespace {
    #define SOME_VALUE 1
}
```

## Enums

Prefer using `enum class` rather than a standard `enum` if you're not going to do arithmetic on it.
This allows for better type safety and forces the `EnumName::VALUE` syntax which is more explicit than the `VALUE` syntax.

Do this:

```cpp
namespace SomeNamespace {
    enum class MyEnum {
        SOME_VALUE = 1,
    };
}
```

Not that:

```cpp
namespace SomeNamespace {
    enum MyEnum {
        MY_ENUM_SOME_VALUE = 1,
    };
}
```

## Comments

Documentation using Doxygen comments is encouraged, paraphrasing is discouraged: comments shall describe the why and the code the how.

However, if a piece of code is unclear, comments describing the operation are encouraged.

## Formatting the code

The source code comes with a `clang-format` file can be used along with the tool of the same name to format code properly. It is suggested to make your text editor format the files you modify on save.

## Error handling in functions

Error handling in functions is achieved by using the {class}`Gaia::Result` class.
This class is very similar to Rust's `Result<T, E>` type.

The {class}`Gaia::Result` class is a templated class taking two arguments: `T` and `E`, with the former representing the type of the value that will be returned on success and the latter representing the type of the value that will be returned on failure. One common option for the `E` parameter is the {class}`Gaia::Error` enum. The {class}`Gaia::Ok` class and the {class}`Gaia::Err` class are used to return `T` and `E` respectively.

### Returning void on success

Since passing `void` as the `T` parameter would produce invalid code, the {class}`Gaia::Null` class is available for use; it is simply an empty `struct` made for that purpose.

### The TRY() macro

The `TRY()` macro can be used in a function that returns a result to call another function returning a result. On failure, the error of the other function will be returned in the caller (the macro inserts a `return` statement), this allows for a pretty powerful error passing hierarchy. In fact, this is exactly how {func}`Gaia::main` works; the functions it calls that return a result are wrapped in a `TRY()` statement. If the call does succeed, the value is returned by the macro and execution continues.

### Example usage

```cpp
#include <lib/result.hpp>
#include <lib/error.hpp>

using namespace Gaia;

Result<int, Error> function(int param) {
    if(param == 1) {
        return Err(Error::INVALID_PARAMETERS);
    }

    return Ok(2);
}

Result<int, Error> other_function(int param) {
   // On failure, the error will be returned by the function.
   // On success, the value is returned by the macro and execution is continued
   int ret = TRY(function(param));
   return Ok(ret);
}

```
