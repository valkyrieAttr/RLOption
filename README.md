<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# `rl::Option`

`rl::Option<T, Ts...>` is a single-header C++26 value container. It stores either no value or exactly one object from a closed set of alternative types. Its API combines primary accessors, alternative-aware combinators, explicit typed access, and exhaustive pattern matching.

This document describes the API implemented by `include/rl_option.hpp`. It intentionally distinguishes implemented behavior from behavior that may be familiar from `std::optional`, `std::variant`, `std::expected`, or Rust. Those types are useful points of comparison, but `rl::Option` is not a drop-in replacement for any of them.

Besides the public API, this document also describes how the header actually implements that API internally — storage layout, discriminator values, dispatch mechanisms, and exception-safety ordering. Those internal descriptions are read directly off the current revision of `include/rl_option.hpp`. They are not part of the stable public contract: §16 already warns against depending on exact layout, and that warning applies to every internals paragraph added below as much as to the ones that were already there.

> **Important:** the first template argument is the *primary type*. Primary accessors such as `value()` and `unwrap()` are tied to the first type. Untargeted combinators operate across all alternatives when their callback supports the full pack; targeted overloads inspect one named type.

**Header:** `include/rl_option.hpp`
**Namespace:** `rl`
**Language baseline:** C++26
**Dependencies:** standard library only
**License identifier:** `GPL-3.0-or-later`

---

## Contents

1. [What the type represents](#1-what-the-type-represents)
2. [Requirements and installation](#2-requirements-and-installation)
3. [The vocabulary types](#3-the-vocabulary-types)
4. [Constructing and assigning values](#4-constructing-and-assigning-values)
5. [A staged tutorial](#5-a-staged-tutorial)
6. [Pattern matching](#6-pattern-matching)
7. [State queries](#7-state-queries)
8. [Reading values and handling absence](#8-reading-values-and-handling-absence)
9. [Functional operations](#9-functional-operations)
10. [Multiple alternatives and result-style code](#10-multiple-alternatives-and-result-style-code)
11. [Mutation, extraction, and swapping](#11-mutation-extraction-and-swapping)
12. [Pointer views](#12-pointer-views)
13. [Comparisons and hashing](#13-comparisons-and-hashing)
14. [Errors, exceptions, and failure behavior](#14-errors-exceptions-and-failure-behavior)
15. [Constraints and edge cases](#15-constraints-and-edge-cases)
16. [Implementation notes](#16-implementation-notes)
17. [A nine-alternative walkthrough](#17-a-nine-alternative-walkthrough)
18. [Known pitfalls and anti-patterns](#18-known-pitfalls-and-anti-patterns)
19. [Complete API reference](#19-complete-api-reference)
20. [Frequently asked questions](#20-frequently-asked-questions)
21. [Test-suite map](#21-test-suite-map)
22. [License](#22-license)

---

## 1. What the type represents

### 1.1 The state model

`rl::Option<T, Ts...>` is an owning, manually discriminated sum type. It contains either no value (`None`) or exactly one active object whose type is one of the listed alternatives.

| Type | Possible states |
|---|---|
| `rl::Option<T>` | `None` or `T` |
| `rl::Option<T, E>` | `None`, `T`, or `E` |
| `rl::Option<A, B, C>` | `None`, `A`, `B`, or `C` |

The first alternative, `T`, is the primary type. Later alternatives are secondary types. The distinction is intentional: ordinary accessors such as `value()` and `unwrap()` are statically tied to `T`; a secondary value must be named with a typed accessor or handled with `match()`.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

enum class ReadError { not_found, permission_denied };

auto main() -> int {
    rl::Option<int, ReadError, std::string> value = rl::Some<std::string>("configuration");

    std::println("engaged: {}", value.is_some());
    std::println("holds string: {}", value.is_some<std::string>());
    std::println("text: {}", value.unwrap<std::string>());
}
```

### 1.2 The primary type and the alternatives

Only the accessors that return the primary payload are primary-only. Most operations that inspect an engaged value are deliberately defined for every alternative.

| Operation family | Behavior with an active secondary |
|---|---|
| `is_some()`, `is_none()`, `operator bool()` | Reports whether any alternative is active |
| `is_some<Q>()`, `unwrap<Q>()`, `as_*_ptr<Q>()` | Inspects or accesses the specifically named alternative |
| `value()`, `unwrap()`, `expect()`, `operator*`, `operator->` | Requires the primary type `T`; wrong active type is a checked-access failure |
| Untargeted `map`, `and_then`, `map_or*`, predicates | Considers every declared alternative; callback constraints must be met for every alternative |
| Targeted `map<Q>`, `and_then<Q>`, `map_or*<Q>`, targeted predicates | Runs only for the named alternative; other alternatives follow the documented inactive path |
| `or_else` | Runs only for `None`; any active alternative is preserved |
| `match()` | Dispatches on `None` and every alternative |

An untargeted `map` does not silently erase an error or other secondary value. If every alternative is mappable, its result type includes the distinct callback result types. If the callback cannot accept every declared alternative, the call is rejected at compile time; use a targeted overload or an exhaustive `match()`.

### 1.3 Comparison with standard vocabulary types

| Standard facility | Related `rl::Option` shape | Important difference |
|---|---|---|
| `std::optional<T>` | `rl::Option<T>` | Both represent a value or absence. This type has explicit `None` / `Some` vocabulary and no default constructor. |
| `std::variant<Ts...>` | `rl::Option<T, Ts...>` | This type adds a distinct empty state; its first alternative also defines primary accessors. |
| `std::expected<T, E>` | Sometimes `rl::Option<T, E>` | This type has three state categories: `None`, `T`, and `E`. It does not implement `expected`'s error-propagation contract. |

Use `std::expected<T, E>` when success and failure should be distinct and errors should propagate according to that standard type's contract. Use `rl::Option<T, E>` only when the extra empty state and this library's alternative semantics are useful.

---



## 2. Requirements and installation

### 2.1 C++26 requirement

RLOption uses C++26 language and standard-library features throughout. It is not backward-compatible with C++23; use a C++26 compiler and standard library.

### 2.2 Compiler notes

The supplied header does not define a tested compiler-version matrix. Select a compiler and standard library that implement the required C++26 features.

The README examples were syntax-checked with Clang 23.1 and `-std=c++26` against the supplied header. This records one verified configuration, not a guarantee for every compiler and library combination.

### 2.3 Add the header to a project

Place the header at `include/rl_option.hpp`, then compile a translation unit that includes it.

```sh
clang++ -std=c++26 -Wall -Wextra -pedantic -I./include main.cpp -o app
./app
```

A minimal translation unit:

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> count = rl::Some<int>(42);
    std::println("{}", count.unwrap());
}
```

### 2.4 Using the header with Compiler Explorer

In Compiler Explorer, open the Library tab, search for `rl_option`, and select the `trunk` version. Compiler Explorer adds the library's include path. Then include the header and select a compiler and standard library with the required C++26 support:

```cpp
#include "rl_option.hpp"
```

Set the compiler option to `-std=c++26` if the selected compiler does not already use that language mode.

---



## 3. The vocabulary types

The namespace `rl` exposes a small set of tag and wrapper types used for construction and matching.

### 3.1 `rl::Some<T>`: construct an engaged alternative

`Some<T>` wraps one value of exactly type `T`. It is the supported construction route for an engaged `Option`.

Its constructors are explicit:

```cpp
#include "rl_option.hpp"

#include <string>

auto main() -> int {
    auto first = rl::Some<int>(42);
    auto second = rl::Some<int>(42);

    auto third = rl::Some<std::string>("ready");

    rl::Option<int> count = first;
    rl::Option<std::string> label = third;
}
```
`get()` preserves the wrapper's value category. An lvalue wrapper exposes an lvalue reference; an rvalue wrapper exposes an rvalue reference:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <utility>

auto main() -> int {
    auto wrapped = rl::Some<std::string>("ready");
    wrapped.get().append(" now");

    const auto fixed = rl::Some<std::string>("read-only");
    const std::string& view = fixed.get();

    std::string extracted = std::move(wrapped).get();
    std::println("{} / {}", view, extracted);
}
```
The class provides:

| Member | Meaning |
|---|---|
| `Some<T>()` | Default-constructs the wrapped `T`; available only when `T` is default-constructible |
| `Some<T>(const T&)` | Copies a `T`; available only when `T` is copy-constructible |
| `Some<T>(T&&)` | Moves a `T`; available only when `T` is move-constructible |
| `get() const&` | Returns `const T&` |
| `get() &` | Returns `T&` |
| `get() &&` | Returns `T&&` |
| `get() const&&` | Returns `const T&&` |
| `operator==(Some<T>, Some<T>)` | Compares wrapped values when `T` is equality-comparable |

`Some<T>` is also a pattern for matching an engaged value equal to the wrapped value. See §6.5 for the pattern's behavior and a type-based predicate class.

### 3.2 `rl::None`: explicitly empty

`rl::None` represents the empty state. It does not construct, inspect, or require a `T`.

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> value = rl::None;

    std::println("empty: {}", value.is_none());

    value = rl::None;
    std::println("still empty: {}", value == rl::None);
}
```
`None` is available for every valid `Option` specialization, including those whose alternatives are not default-constructible.

The tag type is callable: `rl::None()` produces another `None` tag. Equality is defined for `None` with itself and for `None` with `Some<T>`.

### 3.3 `rl::Default`: default-construct an alternative

`rl::Default` requests an engaged, default-constructed value. It does not make `Option` default-constructible.

The selection rule is:

1. If the primary type `T` is default-constructible, construct `T`.
2. Otherwise, select the first default-constructible type in `Ts...`.
3. If no alternative is default-constructible, construction or assignment from `Default` is unavailable.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

struct NoDefault {
    explicit NoDefault(int value) : value(value) {}
    int value;
};

auto main() -> int {
    rl::Option<int> number = rl::Default;
    rl::Option<std::string> text = rl::Default;
    rl::Option<NoDefault, std::string> fallback = rl::Default;

    std::println("number: {}", number.unwrap());
    std::println("text is empty: {}", text.unwrap().empty());
    std::println("selected string: {}", fallback.is_some<std::string>());
}
```
The `Default` tag is also available as a pattern. As a match pattern, it compares an active alternative against a default-constructed object of that same alternative type, provided the type is default-constructible and equality-comparable. This is not the same selection rule as constructing an `Option` from `Default`.

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> value = rl::Some<int>(0);

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        rl::Default >>= [] {
            std::println("primary value is default-initialized");
        },
        rl::Wild >>= [](int number) {
            std::println("non-default value: {}", number);
        }
    );
}
```
The tag is callable (`rl::Default()`) and has an explicit conversion operator to any default-constructible target type other than the `None` and `Default` tag types. That conversion is why it can be used as a fallback supplier with `unwrap_or_else`.

### 3.4 `rl::Wild`: match any state

`rl::Wild` is a catch-all pattern. It covers both `None` and every engaged alternative.

For an engaged value the handler may accept the matched value or be nullary. For `None`, the handler must be callable without an argument.

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> value = rl::None;

    value.match(
        rl::Wild >>= [](auto&&... arguments) {
            if constexpr (sizeof...(arguments) == 0) {
                std::println("empty");
            } else {
                std::println("engaged");
            }
        }
    );
}
```
A `Wild` arm is enough to cover `None` for compile-time coverage checking. When a separate `None` arm appears earlier, that earlier arm handles the empty state.

### 3.5 `rl::SomeAny`: match any engaged alternative

`rl::SomeAny` matches every engaged alternative but never matches `None`.

A `SomeAny` arm therefore does not replace the required `None` coverage.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <type_traits>

auto main() -> int {
    rl::Option<int, std::string> value = rl::Some<std::string>("payload");

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        rl::SomeAny >>= [](auto&& item) {
            using Item = std::remove_cvref_t<decltype(item)>;

            if constexpr (std::same_as<Item, int>) {
                std::println("integer alternative");
            } else {
                std::println("string alternative: {}", item);
            }
        }
    );
}
```
For multiple alternatives, the handler should generally be a generic callable or an overload set that accepts every alternative.

### 3.6 Predicate lambdas as guards

A predicate lambda can be used directly as a pattern. It is tested against a `const` view of the active alternative; the handler receives the matched alternative with the value category of the `Option` expression.

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> value = rl::Some<int>(18);

    value.match(
        rl::None >>= [] {
            std::println("no value");
        },
        [](const int& number) {
            return number >= 18;
        } >>= [](int number) {
            std::println("at least 18: {}", number);
        },
        rl::Wild >>= [](int number) {
            std::println("below 18: {}", number);
        }
    );
}
```
A predicate's being considered applicable to a type is a compile-time property. The predicate's runtime result is not proven at compile time.

The matcher uses compile-time invocability to determine which alternatives a predicate can test. At runtime it evaluates that predicate against each applicable active value; a false result lets dispatch continue to the next arm. Exhaustiveness still requires a fallback for any value the predicate may reject.

---

## 4. Constructing and assigning values

### 4.1 There is no default constructor

`Option()` is explicitly deleted. Initialize each object with the state you mean:

```cpp
#include "rl_option.hpp"

auto main() -> int {
    rl::Option<int> empty = rl::None;
    rl::Option<int> zero = rl::Default;
    rl::Option<int> answer = rl::Some<int>(42);
}
```

`Default` is a tag-based request to construct a value; it is not a default-empty state.

### 4.2 Construct from `Some<T>`

`Some<T>` names the alternative to construct. It is supported for the primary and all declared secondary alternatives.

```cpp
#include "rl_option.hpp"

#include <string>

auto main() -> int {
    rl::Option<int> count = rl::Some<int>(7);
    rl::Option<std::string> default_message = rl::Some<std::string>({});
    rl::Option<int, std::string> message = rl::Some<std::string>("ready");
    rl::Option<int, std::string> code = rl::Some<int>(200);
}
```

Use `Some<T>` when the alternative choice should be explicit at the call site. The value constructors of `Some<T>` are explicit, while `Option`'s constructor from `Some<T>` is non-explicit. This allows copy initialization such as `rl::Option<T> value = rl::Some<T>(...)` while keeping creation of the `Some<T>` wrapper intentional.

### 4.3 Construct from `None` or `Default`

```cpp
#include "rl_option.hpp"

#include <string>

auto main() -> int {
    rl::Option<int, std::string> absent = rl::None;
    rl::Option<int> initialized = rl::Default;
}
```

`None` works for any valid specialization. `Default` always engages a default-constructible alternative: the primary if possible, otherwise the first default-constructible secondary.

### 4.4 Bare payload construction is not supported

An engaged `Option` must be constructed from `rl::Some<T>`. The header has no converting constructor from an unwrapped payload, so `Option<int, std::string> value = "text";` is ill-formed. Wrap the payload to make the engaged state and intended alternative explicit:

```cpp
#include "rl_option.hpp"

#include <string>

auto main() -> int {
    rl::Option<int> a = rl::Some<int>(42);
    rl::Option<int, std::string> b = rl::Some<std::string>("text");
}
```

Copy initialization is supported because the `Option` constructor from `Some<T>` is non-explicit. The `Some<T>` wrapper's value constructors are explicit, so the payload still has to be wrapped deliberately.

### 4.5 Assign a value to an existing `Option`

Bare-value assignment is also supported. It selects a unique exact alternative if available, otherwise a unique constructible alternative. No viable or multiple viable alternatives make the assignment ill-formed.

```cpp
#include "rl_option.hpp"

#include <string>

auto main() -> int {
    rl::Option<int, std::string> value = rl::None;

    value = rl::Some<int>(9);
    value = rl::Some<std::string>("x");
    value = rl::None;
    value = rl::Default;
}
```

For example, assigning a `short` to `Option<int, long>` is ambiguous because both alternatives can be constructed from it. Spell the intended alternative with `Some<long>(...)` or `Some<int>(...)`.

When the selected target is movable, the implementation first constructs a temporary target from the incoming expression while the old active object is still alive. Only after that succeeds does it replace the stored alternative. This ordering supports self-aliasing inputs and avoids destroying the old value before a throwing conversion has completed; see §16.3.

### 4.6 Copy and move operations

Copy construction is available only when every alternative is copy-constructible. Move construction is available only when every alternative is move-constructible. Assignment has corresponding requirements for each alternative's assignment capabilities. The implementation conditionally preserves trivial special members.

Moving an `Option` does not by itself reset the source. The source remains in its original state with a moved-from payload when the active payload was moved.

---



## 5. A staged tutorial

This tutorial builds a small parsing pipeline using the primary-type API. Every stage is shown in one complete translation unit so that the declarations and definitions required by the examples are visible together.

### 5.1 Stage 1: produce an optional value

A parser that may fail without needing to distinguish failure causes can return `Option<int>`.

```cpp
#include "rl_option.hpp"

#include <charconv>
#include <print>
#include <string_view>
#include <system_error>

auto parse_integer(std::string_view text) -> rl::Option<int> {
    if (text.empty()) {
        return rl::None;
    }

    int value = 0;
    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );

    if (error != std::errc{} || end != text.data() + text.size()) {
        return rl::None;
    }

    return rl::Some<int>(value);
}

auto main() -> int {
    const auto parsed = parse_integer("42");

    if (parsed.is_some()) {
        std::println("parsed: {}", parsed.unwrap());
    } else {
        std::println("not an integer");
    }
}
```
The function returns `None` on failure and `Some<T>(value)` on success. It does not return a bare integer.

### 5.2 Stage 2: validate with `and_then`

`and_then` is useful when the next operation itself returns an `Option`.

```cpp
#include "rl_option.hpp"

#include <charconv>
#include <print>
#include <string_view>
#include <system_error>

auto parse_integer(std::string_view text) -> rl::Option<int> {
    if (text.empty()) {
        return rl::None;
    }

    int value = 0;
    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );

    if (error != std::errc{} || end != text.data() + text.size()) {
        return rl::None;
    }

    return rl::Some<int>(value);
}

auto require_positive(int value) -> rl::Option<int> {
    if (value <= 0) {
        return rl::None;
    }

    return rl::Some<int>(value);
}

auto main() -> int {
    const auto positive = parse_integer("42").and_then(require_positive);

    std::println("positive: {}", positive.unwrap_or(-1));
}
```
`and_then` does not create `Option<Option<T>>`. It returns the `Option` produced by the callback. The callback must return an `rl::Option` specialization.

### 5.3 Stage 3: transform with `map`

`map` transforms the primary value and wraps the transformed value in `Option<U>`.

```cpp
#include "rl_option.hpp"

#include <charconv>
#include <print>
#include <string_view>
#include <system_error>

auto parse_integer(std::string_view text) -> rl::Option<int> {
    if (text.empty()) {
        return rl::None;
    }

    int value = 0;
    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );

    if (error != std::errc{} || end != text.data() + text.size()) {
        return rl::None;
    }

    return rl::Some<int>(value);
}

auto require_positive(int value) -> rl::Option<int> {
    if (value <= 0) {
        return rl::None;
    }

    return rl::Some<int>(value);
}

auto main() -> int {
    const auto doubled = parse_integer("21")
        .and_then(require_positive)
        .map([](int value) {
            return value * 2;
        });

    std::println("result: {}", doubled.unwrap_or(-1));
}
```
This stage uses `Option<int>`, so its only engaged alternative is `int`. For a multi-alternative Option, untargeted `map` handles all alternatives only when the callback supports each one; a targeted overload can select a single alternative.

### 5.4 Stage 4: choose an eager or lazy fallback

Use `unwrap_or` for an already available fallback and `unwrap_or_else` for a fallback that should be computed only when required.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

auto main() -> int {
    rl::Option<std::string> missing = rl::None;

    const std::string eager = missing.unwrap_or(std::string("fallback"));

    const std::string lazy = missing.unwrap_or_else([] {
        std::println("computing fallback");
        return std::string("computed");
    });

    std::println("{} / {}", eager, lazy);
}
```
### 5.5 Stage 5: handle all alternatives explicitly

Use `Option<T, E>` when the program needs to distinguish a primary value from a typed secondary state. `match()` is the symmetric interface for that shape.

```cpp
#include "rl_option.hpp"

#include <charconv>
#include <print>
#include <string_view>
#include <system_error>

enum class ParseError {
    empty_input,
    malformed_input
};

auto parse_with_error(std::string_view text) -> rl::Option<int, ParseError> {
    if (text.empty()) {
        return rl::Some<ParseError>(ParseError::empty_input);
    }

    int value = 0;
    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );

    if (error != std::errc{} || end != text.data() + text.size()) {
        return rl::Some<ParseError>(ParseError::malformed_input);
    }

    return rl::Some<int>(value);
}

auto main() -> int {
    const auto result = parse_with_error("42");

    result.match(
        rl::None >>= [] {
            std::println("no result was produced");
        },
        [](const ParseError&) {
            return true;
        } >>= [](ParseError error) {
            std::println("parse error: {}", static_cast<int>(error));
        },
        [](const int&) {
            return true;
        } >>= [](int value) {
            std::println("parsed value: {}", value);
        }
    );
}
```
This version handles `None`, `ParseError`, and `int` separately. The guard predicates are used to tell the compile-time checker which alternative types their arms are intended to cover.

---

## 6. Pattern matching

### 6.1 Forming an arm with `>>=`

An arm is formed by applying the imported `>>=` operator to a pattern and a handler:

```cpp
pattern >>= handler
```

The arm stores its pattern and handler. For an engaged alternative, arms are considered in source order and the first runtime-matching arm is selected. `None` dispatch has a dedicated path described in §16.4.

### 6.2 Supported pattern forms

| Pattern | Matches |
|---|---|
| `rl::None` | The empty state |
| `rl::Wild` | `None` and every engaged alternative |
| `rl::SomeAny` | Every engaged alternative, but not `None` |
| `rl::Default` | An active alternative equal to its default-constructed value when construction and equality are supported |
| `rl::Some<T>(value)` | An active value of exactly `T` equal to `value` |
| A predicate lambda or callable | An applicable alternative for which the predicate returns true |
| An equality-comparable value | An active alternative equal to that value when the comparison is valid |
| A nullary boolean callable | A guard with the same result for each applicable active alternative |

For `bool` alternatives, use `rl::True` and `rl::False` marker patterns when proving both values at compile time. A raw `true` or `false` pattern can test a runtime value, but does not establish both bool cases for exhaustiveness.

### 6.3 Handler forms

For an engaged state, a handler may accept the active alternative, accept no arguments, or be a non-callable object convertible to the deduced result type. A handler covering several alternatives must be valid for each one.

For `None`, the selected handler is invoked without an argument. A handler for `Wild` must therefore support both a nullary call and any engaged calls it may receive, unless an explicit `None` arm takes precedence.

### 6.4 Exhaustiveness is a compile-time contract

A well-formed `match()` must provide coverage for `None` and every declared alternative. Coverage is type-based: each alternative needs at least one arm whose pattern could apply to it. A generic `SomeAny` handler can cover all engaged alternatives; `Wild` can cover both engaged values and `None`. Exhaustiveness does not mean that a separate arm is required for each type.

For a `bool` alternative, the compile-time check requires both cases (`rl::True` and `rl::False`) or a suitable catch-all. A guard that merely accepts `bool` does not prove both values.

A type-compatible guard can still return false for the current value. If every applicable guard returns false at runtime and no later arm matches, `match()` prints a diagnostic and aborts. Put an unconditional fallback after guards unless their joint value coverage is guaranteed.

### 6.5 Value patterns and class-based type patterns

`Some<T>(value)` is a runtime equality pattern. It works with ordinary handlers; the current arm layout does not impose a special handler-alignment restriction.

For type-directed matching without a capture lambda, use a small variadic predicate class whose template arguments name the desired type or types. It is generic so the compiler can check it for each alternative, but it returns true only for one of the listed types:

```cpp
#include "rl_option.hpp"

#include <concepts>
#include <print>
#include <string>
#include <type_traits>

template<class... Wanted>
struct IsOneOf {
    template<class U>
    constexpr bool operator()(U&&) const noexcept {
        return (std::same_as<std::remove_cvref_t<U>, Wanted> || ...);
    }
};

auto main() -> int {
    rl::Option<int, std::string> value = rl::Some<std::string>("text");

    value.match(
        rl::None >>= [] { std::println("empty"); },
        IsOneOf<std::string>{} >>=
            [](std::string const& text) { std::println("{}", text); },
        rl::SomeAny >>= [](auto const&) { std::println("other alternative"); }
    );
}
```

The same idea can be packaged into a handler adapter. The source demo in `src/Option.cpp` defines `TypedHandler<T, F>` and `on_type<T>(f)`: the adapter invokes its stored callable only when the dispatched type is exactly `T`, and marks the impossible other-type path unreachable. This is useful when building a reusable visitor class rather than spelling a generic lambda at every call site. Keep an unconditional fallback arm if a guard can fail for an otherwise covered type.

### 6.6 Matching a plain expression

The public header exposes `Option::match(...)`; it does not expose a free `rl::Match(expr, arms...)` function. The source demo has a local helper named `match_value` that wraps a value in a temporary single-alternative `Option` and forwards to `.match()`. The same adapter shape is:

```cpp
#include "rl_option.hpp"

#include <type_traits>
#include <utility>

template<class T, class... Arms>
auto match_value(T&& expression, Arms&&... arms) -> decltype(auto) {
    using Value = std::remove_cvref_t<T>;
    rl::Option<Value> subject = rl::Some<Value>(std::forward<T>(expression));
    return subject.match(std::forward<Arms>(arms)...);
}
```

This adapter owns a temporary copy or moved value for the duration of the call. Since the temporary is always engaged, its required `None` arm is unreachable at runtime but is still required by `Option::match()`'s compile-time exhaustiveness contract. This helper is an application-level pattern, not part of the public library API.

### 6.7 Match result types

When every handler returns `void`, `match()` returns `void`. When handlers return values, the library materializes those values and wraps them in an `Option` result. Distinct branch result types are retained as alternatives after duplicate types are removed; compatible types may be consolidated by the implementation's result-type rules. A reference returned by a handler does not escape as a reference to the arm's temporary result: results are materialized by value.

Use `match<void>(...)` when branch return values should be discarded. Explicit result template arguments can select the result alternative type or types when the inferred result shape is not the desired one. Returning an `Option` from a handler follows the library's Option-aware result handling rather than requiring callers to manually wrap every scalar branch.

The positive and negative cases are documented in `tests/match_return_types_positive.cpp`, `tests/match_return_types_negative.cpp`, and `tests/match_materialized_results.cpp`; see §21 for the test map.

---

## 7. State queries

State queries inspect the discriminator and are constant-time with respect to the number of alternatives.

| Member | Meaning |
|---|---|
| `explicit operator bool()`, `is_some()`, `has_any_engaged_value()` | True when any alternative is active |
| `is_none()` | True only for the empty state |
| `is_some<Q>()` | True only when alternative `Q` is active |
| `is_default()` | Tests the selected default-constructible alternative against its default value; false for `None` |

`is_default()` uses the primary alternative when it is default-constructible; otherwise it uses the first default-constructible secondary alternative. It requires equality comparison for the selected type. It does not mean that the `Option` is empty.

### 7.1 `is_some_and(predicate)`

Untargeted `is_some_and` calls its predicate for whichever alternative is active. The predicate must accept every declared alternative, or the call is rejected at compile time. It returns false for `None`.

A targeted overload, `is_some_and<Q>(predicate)`, checks only alternative `Q`; it returns false for `None` and for every other alternative. This is useful when a predicate intentionally supports just one type.

The callback receives the receiver's corresponding value category. On an rvalue receiver, an active value is consumed and the `Option` is reset after the predicate returns. If the predicate throws, that reset is not reached.

### 7.2 `is_none_or(predicate)`

Untargeted `is_none_or` returns true for `None`; for any active alternative it calls a predicate that must accept every alternative. The targeted form `is_none_or<Q>(predicate)` checks only `Q`, returning true for `None` and non-`Q` alternatives.

As with `is_some_and`, an rvalue receiver is reset after an active predicate call completes, but not if the callback throws. These are value-category-sensitive operations; do not infer exception cleanup or reset behavior from the spelling alone.

---

## 8. Reading values and handling absence

### 8.1 Checked primary access: `value()`, `unwrap()`, and `expect()`

These accessors require the primary alternative `T` to be active. An empty state or a secondary alternative causes a diagnostic followed by `std::abort()`.

| Member | Lvalue result | Rvalue result |
|---|---|---|
| `value()` | `T&` or `const T&` | `T&&` |
| `unwrap()` | `T&` or `const T&` | `T` by value |
| `expect(message)` | `T&` or `const T&` | `T` by value |

`value() &&` returns a reference into the `Option` and does not reset it. The rvalue `unwrap()` and `expect()` overloads extract by value and reset after successful extraction. A throwing move may leave the source engaged; see §16.3.

### 8.2 Typed access: `unwrap<Q>()`

`unwrap<Q>()` selects a declared alternative by type, including secondary alternatives. Lvalue overloads return references; the rvalue overload returns `Q` by value and resets after successful extraction. If `Q` is not active, the checked access aborts.

This explicit type selection gives the same rule for primitive, class, and enum alternatives: the library never guesses which active type the caller meant.

### 8.3 `unwrap_none()`

`unwrap_none()` verifies that the state is empty and returns the `None` tag. It aborts if any alternative is active.

### 8.4 `unwrap_unchecked()`

`unwrap_unchecked()` reads primary storage without checking the discriminator. Calling it unless the primary alternative is active is undefined behavior. It does not reset the source and is not a checked accessor with a debug-only branch removed. Avoid it in ordinary application code.

### 8.5 Fallback operations

| Member | Behavior |
|---|---|
| `unwrap_or(fallback)` | Converts any active alternative to primary type `T`; converts the fallback to `T` only for `None` |
| `unwrap_or_else(supplier)` | Converts any active alternative to `T`; calls the supplier and converts its result to `T` only for `None` |
| `unwrap_or_default()` | Returns the active alternative converted to `T`, or constructs `T{}` for `None` |

These operations can handle a secondary only if it is convertible to the primary type. For unrelated alternatives, use `match()` or a targeted combinator so the conversion policy is explicit. `unwrap_or_default()` requires the primary `T` to be default-constructible; it does not search for a default-constructible secondary.

`unwrap_or` evaluates its fallback argument before the call. `unwrap_or_else` invokes its supplier only on `None`; the supplier may be nullary or accept one `None` argument. On rvalue receivers, these operations may move from the active value but do not reset the source. A moved-from object can therefore remain engaged.

---

## 9. Functional operations

The combinators come in untargeted forms, which must be valid for every declared alternative, and targeted forms, which name one alternative to inspect. The difference is enforced with constraints at compile time.

### 9.1 `map(operation)`

For `Option<T>`, `map(f)` invokes `f` on the active value and returns `Option<Res>`; on `None` it returns `None`.

For `Option<T, Ts...>`, the untargeted overload requires `f` to accept every alternative. It returns an `Option` whose alternatives are the distinct result types produced for the input alternatives, in source alternative order. The result is not a bare payload. If the callback returns different types for different alternatives, all those result alternatives remain representable.

```cpp
#include "rl_option.hpp"

#include <concepts>
#include <string>

auto main() -> int {
    rl::Option<int, float> number = rl::Some<int>(4);

    auto all_alternatives = number.map([](auto value) {
        if constexpr (std::same_as<decltype(value), int>) {
            return value + 1;
        } else {
            return value + 0.5f;
        }
    }); // Option<int, float>

    auto only_integer = number.map<int>([](int value) {
        return std::to_string(value);
    }); // Option<std::string>; None if int is not active
}
```

The targeted overload `map<Q>(f)` checks and invokes only `Q`; it does not probe callback support for other alternatives. If `Q` is inactive or the source is `None`, it returns `Option<Res>(None)`. A callback result that is itself an `Option` is a payload to `map`; use `and_then` to chain and flatten an Option-returning operation. The callback must produce a non-void result type.

### 9.2 `and_then(operation)`

`and_then` is for callbacks that return an `Option`. Untargeted `and_then(f)` requires every source alternative to be accepted and every callback result to be an `Option`. It merges and deduplicates the callback Option alternatives into the result `Option`. `None` produces `None`.

The targeted form `and_then<Q>(f)` checks only `Q` and returns the callback's Option specialization directly. If `Q` is inactive or the source is `None`, it returns `None` converted to that result type.

Use `map` when the callback returns a payload. Use `and_then` when it returns an Option and you want the callback's Option layer to be the result container.

### 9.3 `or_else(operation)`

`or_else` invokes its callback only for `None`. Any active alternative, primary or secondary, is preserved. The callback must return the same `Option<T, Ts...>` specialization.

The lvalue forms copy an active receiver; the rvalue form moves it. On the rvalue engaged path, the implementation resets the source after constructing the returned Option. On the `None` path, the callback supplies the result. This operation remains Option-shaped.

### 9.4 `map_or(operation, fallback)`

Untargeted `map_or(f, fallback)` applies `f` to whichever alternative is active, or returns the fallback for `None`. The callback must accept every alternative. The result is a bare value type selected from the fallback/result constraints, not an `Option`.

The targeted `map_or<Q>(f, fallback)` applies `f` only when `Q` is active; all other alternatives and `None` use the fallback. The fallback argument is evaluated before the call.

### 9.5 `map_or_else(operation, supplier)`

Untargeted `map_or_else(f, supplier)` applies `f` to any active alternative and calls the supplier only for `None`. The callback must accept every alternative and the two branches must satisfy the result type constraints. The result is a bare value.

The targeted `map_or_else<Q>(f, supplier)` applies `f` only to `Q`; `None` and other alternatives use the supplier.

### 9.6 `map_or_transform(operation, fallback)`

This operation transforms active alternatives to a bare result and provides a special optional callback path for `None`. In the untargeted form, the callback must be valid for every alternative. On `None`, it is called with `None` when that call is supported; otherwise the fallback is returned. The targeted overload transforms only its named alternative, uses the optional `None` callback when present, and uses the fallback for other alternatives.

### 9.7 Predicate combinators

Untargeted `is_some_and(predicate)` and `is_none_or(predicate)` require the predicate to accept every alternative. Their targeted forms add a type argument and inspect only that type. They return `bool`, not an `Option`.

### 9.8 The three golden rules for alternative types

1. **Single-type direct mapping.** For `Option<T>`, including CTAD such as `rl::Option a = rl::Some<double>(4.99);`, the active type is known at compile time. Untargeted `a.map(f)` and `a.and_then(f)` deduce from `T` and return an `Option` result.

2. **Targeted mapping preserves a predictable container.** For a multi-alternative Option, targeted overloads such as `map<Q>`, `and_then<Q>`, `map_or<Q>`, `map_or_else<Q>`, `map_or_transform<Q>`, `is_some_and<Q>`, and `is_none_or<Q>` inspect only `Q`. Other alternatives are not used to test whether the callback is valid. `map`, `and_then`, and `or_else` remain Option-shaped operations. When a targeted `map<Q>` succeeds, its result is a single-primary `Option<Res>`; when the target is inactive, the result is `None`. Targeted `and_then<Q>` returns the callback's Option type.

3. **Untargeted operations reject partial callback support.** Untargeted `map`, `and_then`, `map_or`, `map_or_else`, `map_or_transform`, `is_some_and`, and `is_none_or` are rejected at compile time unless the callback satisfies the constraints for every declared alternative. The concepts `__all_mappable_alternatives_`, `__all_and_then_alternatives_`, `__all_map_or_valid_`, and `__all_predicate_invocable_` enforce these rules. No alternative is silently dropped. Choose a targeted operation or exhaustive `match()`.

These rules keep primitive, class, and enum alternatives uniform. Callers explicitly choose when a value leaves the Option container with `unwrap<Q>()`, while mapping keeps a stable container shape and matching provides exhaustive typed dispatch. That predictability is valuable in generic code and prevents type-category-dependent unwrapping assumptions.

---

## 10. Multiple alternatives and result-style code

### 10.1 A result-shaped type has three cases

For:

```text
rl::Option<int, ParseError>
```

the possible states are:

- `None`;
- primary `int`;
- secondary `ParseError`.

The type does not encode an invariant that exactly one of `int` or `ParseError` must be active. `None` is also valid.

A result-style producer should return `Some<T>(value)` for success and `Some<E>(error)` for a secondary error alternative:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string_view>

enum class LookupError {
    not_found,
    unavailable
};

auto lookup(std::string_view key) -> rl::Option<int, LookupError> {
    if (key == "answer") {
        return rl::Some<int>(42);
    }

    if (key.empty()) {
        return rl::Some<LookupError>(LookupError::not_found);
    }

    return rl::Some<LookupError>(LookupError::unavailable);
}

auto main() -> int {
    const auto result = lookup("answer");

    result.match(
        rl::None >>= [] {
            std::println("no result");
        },
        [](const LookupError&) {
            return true;
        } >>= [](LookupError error) {
            std::println("lookup error: {}", static_cast<int>(error));
        },
        [](const int&) {
            return true;
        } >>= [](int value) {
            std::println("value: {}", value);
        }
    );
}
```
### 10.2 Prefer `match()` when preserving the alternative matters

Do not use a targeted primary transformation when the secondary value must survive unchanged. Untargeted `map` can accept and transform every alternative, but it does not preserve the original error value automatically. Use `match()` when you need to handle the primary and secondary states separately and carry the original error forward.

Use `match()` to make the handling explicit:

```cpp
#include "rl_option.hpp"

#include <print>

enum class ArithmeticError {
    division_by_zero
};

auto divide(int numerator, int denominator) -> rl::Option<int, ArithmeticError> {
    if (denominator == 0) {
        return rl::Some<ArithmeticError>(ArithmeticError::division_by_zero);
    }

    return rl::Some<int>(numerator / denominator);
}

auto main() -> int {
    const auto result = divide(10, 0);

    result.match(
        rl::None >>= [] {
            std::println("no result");
        },
        [](const ArithmeticError&) {
            return true;
        } >>= [](ArithmeticError error) {
            switch (error) {
            case ArithmeticError::division_by_zero:
                std::println("cannot divide by zero");
                break;
            }
        },
        [](const int&) {
            return true;
        } >>= [](int value) {
            std::println("quotient: {}", value);
        }
    );
}
```
---

## 11. Mutation, extraction, and swapping

### 11.1 `emplace(args...)`

`emplace` destroys the currently active alternative, then constructs the primary type `T` from the provided arguments.

- On an lvalue receiver, it returns `T&`.
- On an rvalue receiver, it constructs and then extracts `T` by value, leaving the `Option` empty.

`emplace` only constructs the primary alternative. It is not a general secondary-alternative emplacement function.

The current implementation resets the old state before constructing the new primary value. If construction throws, the receiver remains empty.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <utility>

auto main() -> int {
    rl::Option<int, std::string> value = rl::Some<std::string>("old alternative");

    int& primary = value.emplace(12);
    primary += 1;

    std::println("primary: {}", value.unwrap());

    rl::Option<std::string> temporary = rl::None;
    std::string extracted = std::move(temporary).emplace("temporary value");

    std::println("extracted: {}", extracted);
    std::println("temporary is empty: {}", temporary.is_none());
}
```
### 11.2 `reset()`

`reset()` destroys the active alternative, if any, and returns the receiver to `None`. It is available on lvalues.

Calling it when already empty is valid.

```cpp
#include "rl_option.hpp"

#include <print>

auto main() -> int {
    rl::Option<int> value = rl::Some<int>(5);

    value.reset();
    std::println("empty after reset: {}", value.is_none());

    value.reset(); // Resetting an already-empty Option is valid.
}
```
### 11.3 `take()`

`take()` moves the entire `Option<T, Ts...>` into a new `Option<T, Ts...>` and then resets the source to `None`.

It is available on lvalues and requires every alternative to be move-constructible. Rvalue-qualified and `const&&` forms are deleted.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

auto main() -> int {
    rl::Option<int, std::string> source = rl::Some<std::string>("payload");

    auto extracted = source.take();

    std::println("source empty: {}", source.is_none());
    std::println("extracted: {}", extracted.unwrap<std::string>());
}
```
### 11.4 `replace(new_value)`

`replace` assigns a newly constructed alternative and returns the previous state as an `Option<T, Ts...>`.

The target alternative is selected using the same exact-type-then-unique-construction rule as bare-value assignment. The method requires all alternatives to be move-constructible and is available on lvalues only.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

auto main() -> int {
    rl::Option<int, std::string> value = rl::Some<std::string>("before");

    auto previous = value.replace(99);

    std::println("previous: {}", previous.unwrap<std::string>());
    std::println("current: {}", value.unwrap());
}
```
### 11.5 `swap(other)` and `swap(lhs, rhs)`

The member `swap` and the corresponding non-member friend `swap` exchange the states of two `Option` objects with the same specialization.

They require every alternative to be move-constructible and swappable. The conditional `noexcept` specification reflects the move-construction and swap properties of the alternatives.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

auto main() -> int {
    rl::Option<int, std::string> first = rl::Some<int>(7);
    rl::Option<int, std::string> second = rl::Some<std::string>("text");

    swap(first, second);

    std::println("first: {}", first.unwrap<std::string>());
    std::println("second: {}", second.unwrap());
}
```
Internally, `swap` has three distinct paths, chosen at runtime by comparing the two discriminators:

1. **Same alternative active on both sides.** The values are swapped directly, via a dispatch-table call into `std::ranges::swap` on that one alternative. Nothing is move-constructed.
2. **Exactly one side is `None`.** The engaged side is move-constructed into the empty side and then reset, without touching a temporary `Option` at all.
3. **Both sides are engaged with *different* alternatives.** This is the only path that needs a temporary: `*this` is move-constructed into a local `OptionStorage_`, `*this` is reset and move-constructed from `other`, `other` is reset and move-constructed from the temporary. This three-step shuffle is exactly why `swap` only requires every alternative to be move-*constructible*, not move-*assignable* — no alternative's `operator=` is ever called by this path.

---

## 12. Pointer views

`as_mut_ptr()` and `as_immut_ptr()` return an `Option` containing a pointer to the active object, or `None` when the requested alternative is not active.

| Member | Receiver | Result |
|---|---|---|
| `as_mut_ptr()` | `&` | `Option<T*>` |
| `as_mut_ptr<Q>()` | `&` | `Option<Q*>` |
| `as_immut_ptr()` | `const&` | `Option<const T*>` |
| `as_immut_ptr<Q>()` | `const&` | `Option<const Q*>` |

The typed forms require `Q` to be one of the declared alternatives. Rvalue-qualified forms are deleted so that the returned pointer cannot silently refer into a temporary `Option`.

The pointer refers into the original `Option`; it does not extend the lifetime of that object. Do not keep the pointer after the `Option` is destroyed, reset, replaced, or otherwise changes its active object.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <utility>

auto main() -> int {
    rl::Option<int, std::string> value = rl::Some<std::string>("hello");

    auto mutable_pointer = value.as_mut_ptr<std::string>();

    if (mutable_pointer) {
        mutable_pointer.unwrap()->append(" world");
    }

    const auto& read_only = std::as_const(value);
    auto immutable_pointer = read_only.as_immut_ptr<std::string>();

    if (immutable_pointer) {
        std::println("{}", *immutable_pointer.unwrap());
    }
}
```
An `Option<T*>` can itself hold a null pointer as an engaged value. `is_some()` reports whether the pointer alternative is engaged; it does not test whether the stored pointer is non-null.

---

## 13. Comparisons and hashing

### 13.1 Equality between two `Option` objects

Two `Option<T, Ts...>` objects compare equal when:

1. both are `None`; or
2. both hold the same active alternative and the contained values compare equal.

Different active alternatives compare unequal.

The `operator==` overload requires every alternative to be equality-comparable.

### 13.2 Equality with `None`

`option == rl::None` and `rl::None == option` test whether the `Option` is empty.

### 13.3 Equality with `Some<T>`

An `Option` can be compared with `Some<Q>` when `Q` is one of its alternatives and `Q` is equality-comparable. The comparison succeeds only when that exact alternative is active and its value equals the wrapped value.

There is no direct equality overload between an `Option` and an arbitrary bare payload. Use `Some<T>(value)`.

### 13.4 Equality with `Default`

`option == rl::Default` checks the default value of the primary type if the primary type is default-constructible and equality-comparable. If the primary type is not default-constructible, the comparison is available when the first default-constructible secondary alternative is equality-comparable, and it checks that selected alternative.

### 13.5 Three-way comparison

`operator<=>` between two `Option` objects is available only when every alternative is three-way comparable.

The active alternative index is compared first. `None` sorts before engaged alternatives; among engaged alternatives, the order follows the order of the template argument list. If both objects hold the same alternative, their contained values are compared.

The `Option`/`None` three-way comparison is also provided. `Option`/`Default` three-way comparison is provided only for the single-type `Option<T>` shape when `T` is default-constructible and three-way comparable.

### 13.6 `std::hash`

A `std::hash<rl::Option<T, Ts...>>` specialization is provided when every alternative is hashable by `std::hash`.

A `None` state uses an internal sentinel hash value. Engaged multi-alternative states incorporate the active alternative index and the contained value hash. As with all hash functions, collisions remain possible.

```cpp
#include "rl_option.hpp"

#include <functional>
#include <print>

auto main() -> int {
    rl::Option<int> empty = rl::None;
    rl::Option<int> zero = rl::Some<int>(0);
    rl::Option<int> another_zero = rl::Some<int>(0);

    const bool same = zero == another_zero;
    const bool is_empty = empty == rl::None;
    const bool is_default = zero == rl::Default;

    const auto hash = std::hash<rl::Option<int>>()(zero);

    std::println("equal: {}", same);
    std::println("empty: {}", is_empty);
    std::println("default: {}", is_default);
    std::println("hash: {}", hash);
}
```
The "internal sentinel" for `None` is a concrete, fixed value in the current revision: `static_cast<std::size_t>(-3333)`. For an engaged `Option` with at least one secondary alternative, the per-alternative hash is mixed with the active index using the familiar `boost::hash_combine`-style formula — `value_hash ^ (0x9e3779b9 + index + (value_hash << 6) + (value_hash >> 2))` — so that, say, `Option<int, long>` holding `int{5}` and `Option<int, long>` holding `long{5}` do not collide just because `std::hash<int>` and `std::hash<long>` happen to agree on `5`. For `Option<T>` with **no** secondary alternatives, there is nothing to disambiguate, so this mixing step is skipped entirely: the hash of an engaged `Option<T>` is *exactly* `std::hash<T>` of the contained value, unmodified. That's a deliberate simplification for the single-type case, not a hidden property to rely on — the combining step can and does change in the multi-alternative case, and nothing about `std::hash<rl::Option<T, Ts...>>`'s exact numeric output is part of the documented contract beyond "equal `Option`s hash equal."

---

## 14. Errors, exceptions, and failure behavior

### 14.1 Checked-access failures abort

The following are checked operations:

- `value()`, `unwrap()`, and `expect()` when the primary type is not active;
- `unwrap<Q>()` when `Q` is not the active alternative;
- `unwrap_none()` when any alternative is active;
- `match()` when no runtime arm matches the active state.

These failure paths print a diagnostic to `stderr` and call `std::abort()`. They do not throw a library-defined exception and do not perform normal stack unwinding. The internal failure helper also makes invalid constant evaluation fail: when reached during constant evaluation with exceptions enabled it throws a string; in a runtime build the checked-failure path aborts.

`unwrap_unchecked()` is different: it performs no validation, and incorrect use is undefined behavior rather than a checked abort.

### 14.2 Payload operations and callbacks may throw

It is incorrect to claim that no operation involving `rl::Option` can throw.

The library does not use exceptions to report a bad checked access. However, when exceptions are enabled, ordinary operations performed by user-defined types or callbacks can still throw, including:

- construction and assignment of an alternative;
- copying or moving an alternative;
- swapping alternatives;
- a predicate or transformation callback;
- a fallback supplier.

The header requires every alternative to be nothrow-destructible. That requirement allows it to destroy an active object safely during state changes.

### 14.3 Exceptions-disabled builds

The header conditionally compiles exception-specific cleanup using `__cpp_exceptions`. A C++26 build with exceptions disabled is possible when the compiler and standard library support the required facilities and the user's alternative types and callbacks are suitable for that mode.

Disabling exceptions does not make a potentially throwing user operation safe; it changes what the compiler accepts and how the program can report failures.

### 14.4 Choose the right failure mechanism

Use `None` or a secondary error alternative to represent ordinary, expected outcomes.

Use checked accessors when a failed access is a programming error and process termination is the intended policy.

Use `match()` when every state must be handled at a decision point.

Avoid `unwrap_unchecked()` unless the primary alternative has been independently proven active and the unchecked access is justified.

---

## 15. Constraints and edge cases

### 15.1 Alternative-type requirements

Every alternative must be a non-array object type with a non-throwing destructor. Reference and volatile alternatives are rejected. Types must be unique after removing cv qualification. Use a pointer or `std::reference_wrapper<T>` when an alternative should refer to an object owned elsewhere; doing so does not extend that object's lifetime.

### 15.2 No implicit default-empty state

`Option()` is deleted. Use `None` for empty, `Some<T>` for an engaged value, and `Default` when a default-constructed engaged alternative is intended. Bare payload construction is not available.

### 15.3 Explicit engaged construction

Construction requires `Some<T>`; a bare payload cannot implicitly select an alternative. The current header separately provides bare-value assignment when a target alternative is uniquely selectable, but `Some<T>` remains the clearest choice when the alternative matters.

### 15.4 No cross-specialization converting constructor

There is no general converting constructor from a differently-shaped `Option<U, Us...>`. Convert explicitly by matching alternatives or mapping a value into the target specialization.

### 15.5 `Default` and `unwrap_or_default()` use different rules

`Default` constructs the primary if possible, or the first default-constructible secondary otherwise. `unwrap_or_default()` returns primary type `T` and requires `T` to be default-constructible. It does not search the alternative pack.

### 15.6 Primary access and alternative-aware operations

`value()`, `unwrap()`, `expect()`, dereference, and arrow access the primary `T`. Typed accessors select a named alternative. Untargeted combinators consider all alternatives and are constrained to reject callbacks that cannot handle the full pack. Targeted overloads inspect one named alternative. Fallback-by-value functions convert an active alternative to `T`, so they require that conversion to be valid.

### 15.7 Static exhaustiveness and runtime guards

Compile-time checking proves that patterns can cover `None` and every declared alternative. It cannot prove a guard returns true for a particular value. A guarded match can still abort at runtime if every applicable guard fails. Use an unconditional fallback where required.

### 15.8 Value patterns

`Some<T>(value)` and ordinary comparison patterns do not impose a handler-alignment rule in the current implementation. The old alignment claim is not part of the current source behavior.

### 15.9 Rvalue operations and source state

An rvalue-qualified call may move from a payload, reset the `Option`, or leave it engaged with a moved-from payload. These behaviors are member-specific:

- Moving the `Option` itself does not reset its source.
- `std::move(opt).value()` returns `T&&` and does not reset.
- `unwrap()`, `expect()`, and `unwrap<Q>()` on an rvalue return by value and reset after successful extraction.
- `take()` returns the previous Option and resets after the move succeeds.
- Rvalue `map`, `and_then`, `map_or*`, predicates, and `or_else` may consume the active value and reset after the invoked callback or predicate completes. A callback exception can interrupt that path before reset. A targeted overload does not consume a non-target alternative.
- Rvalue `unwrap_or*` may move from an active alternative while converting it to `T`, but it does not reset the source.
- `match()` does not reset the receiver by default.
- Rvalue `emplace()` constructs the primary alternative and extracts it by value.

Check the exact overload when source state matters. Do not equate `std::move(opt).member()` with `opt.reset()`.

### 15.10 Object lifetime and borrowed views

References returned by lvalue accessors and pointers returned by `as_mut_ptr()` / `as_immut_ptr()` refer into the `Option`'s active storage. They do not own the payload or keep it alive. Reset, replacement, emplace, assignment, swap, or destruction can end that payload's lifetime and invalidate outstanding references and pointers.

`RL_LIFETIME_BOUND` is a compiler diagnostic annotation, not a borrow checker. The type system does not prevent a caller from retaining a view past the owner's lifetime or across a mutation. Keep views local, and do not mutate the Option while using a view into its active object. The pointer-view rvalue overloads are deleted to prevent the most direct dangling-view mistake.

### 15.11 No built-in formatter

This header does not provide `std::formatter` for `rl::Option`. Match or extract a payload and format that value.

### 15.12 No built-in synchronization

An `Option` is not synchronized. Concurrent access that includes a write to the same object requires external synchronization.

---

## 16. Implementation notes

### 16.1 Storage model

The current implementation uses manually managed union storage plus one discriminator. This section describes the present header, not a stable ABI or serialization format.

- **Discriminator values:** `0` means `None`, `1` means the primary type, and values `2` onward identify secondary alternatives in declaration order. A dedicated `destroying_index_` value is `sizeof...(Ts) + 2`; it marks destruction in progress so reentrant queries see an empty-like state and mutation during destruction can be diagnosed. The discriminator's integer type is selected by `option_index_type_<sizeof...(Ts) + 2>`: `unsigned char`, then `unsigned short`, then `std::size_t`, according to the range required.
- **Primary and secondary storage:** the primary has a dedicated union member. Secondary alternatives are stored in a recursive `Alternative_<...>` union whose first member is the first secondary and whose other member is another nested `Alternative_`. This means the active secondary's declaration position determines how many union wrappers must be activated.
- **Construction and destruction:** selecting a nested secondary placement-constructs the empty union wrappers along its path and then constructs the payload. Reset destroys the active payload and then ends the lifetimes of those wrapper unions in reverse. The primary path does not need recursive wrappers.
- **Per-type operation tables:** compile-time arrays of function pointers dispatch copy/move construction, copy/move assignment, equality, three-way comparison, same-alternative swap, and hashing by active index. Primary destruction is direct; a separate destroy table handles secondary types.
- **Layout:** payload storage is sized for the largest primary or secondary representation and aligned for the strictest alternative and discriminator requirement. The discriminator follows the payload storage, with padding as required. The single-type specialization currently uses the internal `Optional_<T>` storage block and has matching size and alignment. These details are implementation observations; callers must not depend on exact `sizeof`, member offsets, or raw bytes.

The discriminator allows state queries to remain constant-time. The recursive union means construction and destruction of a deep secondary perform work proportional to its nesting depth. Adding alternatives also adds generated per-type operations and template instantiations.

### 16.2 Triviality and special members

Internal storage layers conditionally preserve trivial destruction, copy construction, move construction, copy assignment, and move assignment where the payload types permit them. Public copy/move operations are constrained by the requirements of every alternative, even when one particular active value would be copyable.

### 16.3 Exception behavior during state changes

The implementation has selected safeguards, not a universal strong exception guarantee.

- **Bare-payload assignment stages conversions when possible.** For a movable selected target, it constructs a temporary from the incoming expression before destroying the currently active object. A throwing conversion therefore leaves the old value alive. The implementation then resets and moves the staged target into storage. Non-movable target types need a different direct-construction path.
- **Same-alternative assignment can reuse the payload object.** When source and destination hold the same alternative and that type is assignable, the existing object is assigned in place. When the active alternatives differ, or same-type assignment is unavailable, the implementation may destroy and reconstruct the destination. A throwing operation in those paths can leave state different from the original.
- **Successful extraction resets selectively.** Rvalue `unwrap()`, `expect()`, and `unwrap<Q>()` use a guard that resets only when extraction completes without a new exception. A throwing move leaves the Option engaged, though the payload itself may be moved-from depending on the move constructor.
- **Combinators are not transactions.** Rvalue operations may move from an active value before calling user code. If the callback throws, source reset may not happen and the payload may be moved-from. Other operations reset at different points. Do not assume rollback or a strong exception guarantee.
- **User code can throw.** Payload constructors, assignments, moves, comparisons, hash functions, and callbacks are not universally non-throwing. The type requires non-throwing destruction, but that does not make the full Option API non-throwing.

### 16.4 `match()` dispatch mechanics

- `match()` is implemented with an explicit object parameter and `std::forward_like` so the active payload is forwarded as const lvalue, mutable lvalue, or rvalue according to the receiver.
- The active alternative is found by a compile-time-unrolled sequence of runtime discriminator comparisons. This is a linear scan, not a guaranteed jump table. With `N` alternatives, the worst case tests each alternative index.
- For a known active type, arms are considered in source order. Patterns that cannot apply to that type are removed from the dispatch path at compile time; applicable guards and value patterns are checked at runtime until one matches.
- `None` is handled on a separate path. If any explicit `None` arm is present, it is preferred; otherwise a `Wild` arm handles it. The selected `None` handler is called without an argument.
- Compile-time constraints require `None` coverage and exhaustive coverage for the alternatives. For ordinary alternatives a compatible catch-all such as `SomeAny` or `Wild` can cover multiple types. For bool, both true and false cases or a catch-all are required. Guard type compatibility does not establish that the guard will return true at runtime.
- If all runtime guards for the active engaged value fail, the implementation writes a diagnostic and aborts. Callback exceptions are not converted into this failure path; they propagate as ordinary C++ exceptions.
- The result type is deduced from all arms. Non-void handler results are materialized by value and wrapped in an Option result, with result alternatives deduplicated or consolidated by the result rules. `match<void>` discards branch results. References returned by handlers do not turn the result into a borrowed view.

### 16.5 Diagnostics and type names

Bad-access diagnostics may include a type name. Formatting depends on the compiler and configuration. Treat the fact that a call aborts as behavior; exact message text and type-name formatting are not stable API.

### 16.6 Hashing

`std::hash<rl::Option<T, Ts...>>` is enabled only when all alternatives are hashable. In the current implementation, `None` maps to a fixed sentinel value, while an engaged multi-alternative value combines the active index with the payload hash. Hash values and the mixing formula are not stable identifiers and must not be persisted or treated as unique.

### 16.7 Header side effects

The header includes standard facilities needed by its implementation and exposes the documented names in namespace `rl`. It also imports the overloaded `>>=` operator for pattern-arm syntax. Keep examples explicit about other standard-library headers they use, and avoid depending on incidental transitive includes.

---


## 17. A nine-alternative walkthrough

This example uses nine distinct alternative types: a primary integer literal and eight secondary token kinds. `None` remains separate from an explicit end-of-file token.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

enum class Operator { plus, minus, star, slash };
enum class Punct { lparen, rparen, comma, semicolon };
enum class Keyword { kw_if, kw_else, kw_while, kw_return, kw_fn };

struct Identifier {
    std::string name;
    friend auto operator==(const Identifier&, const Identifier&) -> bool = default;
};

struct EofTag {
    friend auto operator==(const EofTag&, const EofTag&) -> bool = default;
};

using Token = rl::Option<long long, double, bool, std::string, Identifier,
                         Operator, Punct, Keyword, EofTag>;

auto describe(const Token& token) -> void {
    token.match(
        rl::None >>= [] { std::println("<no token scanned yet>"); },
        [](const long long&) { return true; } >>= [](long long v) {
            std::println("integer literal: {}", v);
        },
        [](const double&) { return true; } >>= [](double v) {
            std::println("floating literal: {}", v);
        },
        rl::True >>= [](bool) { std::println("boolean literal: true"); },
        rl::False >>= [](bool) { std::println("boolean literal: false"); },
        [](const std::string&) { return true; } >>= [](const std::string& v) {
            std::println("string literal: {}", v);
        },
        [](const Identifier&) { return true; } >>= [](const Identifier& v) {
            std::println("identifier: {}", v.name);
        },
        [](const Operator&) { return true; } >>= [](Operator v) {
            std::println("operator #{}", static_cast<int>(v));
        },
        [](const Punct&) { return true; } >>= [](Punct v) {
            std::println("punctuation #{}", static_cast<int>(v));
        },
        [](const Keyword&) { return true; } >>= [](Keyword v) {
            std::println("keyword #{}", static_cast<int>(v));
        },
        [](const EofTag&) { return true; } >>= [] {
            std::println("end of file");
        }
    );
}

auto main() -> int {
    Token empty(rl::None);
    Token integer = rl::Some<long long>(42);
    Token name = rl::Some<Identifier>({"accumulator"});
    Token eof = rl::Some<EofTag>({});

    describe(empty);
    describe(integer);
    describe(name);
    describe(eof);

    std::println("name holds Identifier: {}", name.is_some<Identifier>());
    std::println("EOF is an integer: {}", eof.is_some<long long>());
}
```

The example chooses separate arms because each token kind has a different meaning. Exhaustiveness does not require this much repetition: a generic `SomeAny` arm can cover all engaged alternatives, while `Wild` can cover `None` too. A `bool` alternative needs both `rl::True` and `rl::False`,  or a suitable catch-all.

The discriminator stores one value for `None`, the primary and eight secondary alternatives, plus an internal destruction marker. The marker affects the width calculation; a nine-alternative token still fits in the current byte-sized index. Secondary alternatives use recursively nested union storage, so deeper alternatives require more wrapper construction and destruction. Every alternative also contributes operations and template instantiations. The list should reflect types the program actually needs to distinguish.

This shape is plausible for a lexer token because token kinds are mutually exclusive and the separate empty state represents "no token available." For throughput-sensitive token dispatch, compare it with `std::variant` or an enum-plus-payload representation using the real compiler, standard library, and workload. Source structure alone does not prove which implementation is faster.

---

## 18. Known pitfalls and anti-patterns

### 18.1 Expecting `Option<T>()` to mean empty

```cpp
// rl::Option<int> count; // Error: Option() is deleted.
```

Use `rl::Option<int> count = rl::None;`. `Default` creates an engaged default value, not an empty object.

### 18.2 Treating `Option<T, E>` as `std::expected<T, E>`

A callback that accepts only `T` cannot be used with untargeted `map` on `Option<T, E>`; the compiler rejects it. That prevents silently dropping an unsupported error alternative. A targeted `map<T>` intentionally returns `None` when `E` is active, so it also does not preserve an error. Use `match()` when every state must be handled and the error must survive.

### 18.3 Calling `unwrap()` as if it threw a catchable error

Checked access failure prints a diagnostic and calls `std::abort()`. A `try/catch` block cannot recover from `abort()`. Check the state first or use `match()` when absence is expected.

### 18.4 Using `unwrap_unchecked()` as a fast path

`unwrap_unchecked()` performs no active-type check in any build. Calling it when the primary type is not active is undefined behavior. Use checked access unless profiling proves a need and the invariant is airtight.

### 18.5 Assuming static exhaustiveness proves every guard will match

A guard's type compatibility satisfies a type-coverage part of the compile-time check; its runtime result may still be false. End a guarded match with a fallback if there are values not covered by the guards.

### 18.6 Writing one repetitive arm per alternative by habit

Separate arms are useful when each type has distinct behavior, but exhaustive matching can use `SomeAny` for all engaged values or `Wild` for all values including `None`. Bool still needs the true and false cases or a catch-all. Prefer the simplest exhaustive structure that makes the handling clear.

### 18.7 Assuming every rvalue call empties the source

```cpp
auto&& reference = std::move(option).value(); // Source remains engaged.
auto converted = std::move(option).unwrap_or_default(); // May move payload; source remains engaged.
```

Rvalue behavior is member-specific (§15.9). If the source must be empty, call `reset()` explicitly after using the returned value, or use an extraction operation documented to reset on success.

### 18.8 Depending on exact abort diagnostics

Type-name formatting and diagnostic text depend on compiler features. Do not parse the full message or use it as a stable output protocol.

### 18.9 Growing the alternative list for speculative extensibility

Every additional alternative adds type constraints, generated operations, template work, and a case to reason about. Deep secondary alternatives also traverse more recursive union layers. Add an alternative when it represents a real, mutually exclusive state the caller needs to distinguish.

### 18.10 Expecting `Default` to produce an empty Option

`rl::Option<int> value = rl::Default;` is engaged with `int{}`. Use `None` when absence is intended.

### 18.11 Expecting `match()` raw results to stay raw

Non-void `match()` handlers return a materialized `Option` result. For example, integer results produce an `Option<int>`; heterogeneous handler results produce a multi-alternative Option after deduplication or consolidation. Use `match<void>` to discard branch results.

### 18.12 Relying on primitive-versus-class implicit unwrap rules

Do not make generic code depend on whether a compiler or wrapper implicitly exposes a primitive but preserves a class object. This library keeps the payload inside an Option result for both. Use `unwrap<Q>()` when a specific alternative must be extracted, `map<Q>` when it should be transformed while staying optional, and `match()` when alternatives need separate handling.

### 18.13 Keeping references or pointers across mutation

A pointer view or lvalue accessor refers into the Option's active storage. Reset, assignment, emplace, replacement, swap, or destruction can invalidate it. The compiler does not enforce a borrow lifetime. Keep the view inside the period where the same active object is known to remain alive.

### 18.14 Using a fallback conversion for unrelated alternatives

`unwrap_or*` converts an active value to the primary type. If a secondary is not meaningfully convertible to the primary, do not force that conversion just to use a fallback helper. Handle the alternatives with `match()` or use a targeted operation.

---


## 19. Complete API reference

This is a practical inventory of the public names and main overload families in `include/rl_option.hpp`. The constrained overloads in the header determine whether a particular payload and callback combination is valid.

### 19.1 Vocabulary and match patterns

| Name | Kind | Purpose |
|---|---|---|
| `Option<T, Ts...>` | Class template | Empty-or-one-of-a-closed-set value container |
| `Some<T>` | Class template | Explicit engaged-alternative wrapper |
| `None` | `rl` object | Empty-state tag |
| `Default` | `rl` object | Construct an engaged default value; also a value pattern |
| `Wild` | `rl` object | Catch-all that includes `None` |
| `SomeAny` | `rl` object | Catch-all for engaged alternatives |
| `True`, `False` | `rl` objects | Distinct bool-case pattern tags |
| `operator>>=(pattern, handler)` | Imported operator | Forms a match arm |

The header also provides global compatibility objects `True_t` and `False_t`, backed by the same distinct bool marker types. `std::hash<Option<...>>` is available when every alternative is hashable.

### 19.2 `Some<T>`

`Some<T>` has explicit value constructors, `get()` overloads preserving cv/ref category, and equality when `T` is equality-comparable. Spell the payload type explicitly as `Some<T>(value)` when constructing a wrapper; the examples use this form instead of CTAD. The `Option` constructors accepting `Some<T>` construct the selected declared alternative.

### 19.3 Construction and assignment

| Operation | Behavior |
|---|---|
| `Option()` | Deleted |
| `Option(None)` | Empty state |
| `Option(Default)` | Primary if default-constructible, otherwise first default-constructible secondary |
| `Option(Some<Q>)` | Constructs declared alternative `Q` |
| Copy/move construction | Requires every alternative to support the requested operation |
| Copy/move assignment | Requires relevant per-alternative construction and assignment capabilities |
| Assignment from `Some<Q>`, `None`, `Default` | Explicit alternative, empty, or selected default state |
| Assignment from bare `value` | Uses unique alternative selection; ambiguous conversions are rejected |
| Cross-specialization converting constructor | Not provided |

### 19.4 State and access

| Member family | Behavior |
|---|---|
| `operator bool()`, `is_some()`, `has_any_engaged_value()`, `is_none()` | State queries |
| `is_some<Q>()` | Query whether the named alternative is active |
| `is_default()` | Compare the selected default-constructible alternative with its default value |
| `value()`, `unwrap()`, `expect()`, `operator*`, `operator->` | Checked primary access |
| `unwrap<Q>()` | Checked access to a named alternative |
| `unwrap_none()` | Checked empty-state access |
| `unwrap_unchecked()` | Unchecked primary payload read; invalid use is undefined behavior |
| `unwrap_or*` | Convert active alternatives to primary `T`, or use fallback for `None`; `unwrap_or_default` uses `T{}` |

Lvalue checked access generally returns a reference. Rvalue `unwrap`, `expect`, and `unwrap<Q>` extract by value and reset after success. Rvalue `value()` returns a reference and does not reset.

### 19.5 Functional operations

| Member family | Untargeted behavior | Targeted behavior |
|---|---|---|
| `map(f)` | Requires `f` for every alternative; returns Option of distinct result types | `map<Q>(f)` examines only `Q`, returns `Option<Res>` |
| `and_then(f)` | Requires Option-returning `f` for every alternative; merges output alternatives | `and_then<Q>(f)` returns the callback's Option type |
| `or_else(f)` | Runs only for `None`; preserves any active alternative | No targeted form |
| `map_or(f, fallback)`, `map_or_else(f, supplier)` | Transform any active alternative; return a bare result | Targeted forms transform only `Q`; other cases use fallback |
| `map_or_transform(f, fallback)` | Transforms all alternatives; supports an optional `None` callback | Targeted form transforms `Q`, uses fallback for other alternatives |
| `is_some_and(p)`, `is_none_or(p)` | Predicate must support every alternative; return `bool` | Targeted forms inspect only `Q` |

These methods have receiver-category-aware overloads. See §§8–9 and §15.9 for conversion and reset behavior.

### 19.6 Mutation and pointer views

| Member | Behavior |
|---|---|
| `emplace(args...)` | Replaces the current state with a primary `T` |
| `emplace(args...) &&` | Creates the primary value, extracts it by value |
| `reset()` | Destroys the active payload and becomes `None` |
| `take()` | Moves the whole Option out and resets the source after success |
| `replace(value)` | Replaces with the selected alternative and returns the prior Option |
| `swap(other)` | Exchanges same-specialization objects |
| `as_mut_ptr[<Q>]()`, `as_immut_ptr[<Q>]()` | Return non-owning pointer views; rvalue overloads are deleted |

Pointer and reference views do not extend payload lifetime.

### 19.7 Matching, comparisons, and hashing

`match(arms...)` requires exhaustive type coverage, dispatches by active alternative, and returns `void` for all-void handlers or an Option-wrapped materialized result for non-void handlers. It does not reset the receiver automatically.

Comparisons are available when their payload comparison requirements are met. `None` and `Some<Q>` comparisons have dedicated overloads. The `Default` three-way comparison is limited to single-type `Option<T>` in this revision. Hashing requires all alternatives to be hashable; hash values are not stable IDs.

---

## 20. Frequently asked questions

### Is this a C++ standard-library type?

No. `rl::Option` is a third-party library in this repository, not a committee-provided standard-library facility.

### Why return `Option<Res>` from `map` instead of a raw primitive or class?

Keeping the result inside `Option` gives primitive, class, and enum payloads the same contract: a transformation may be absent, so its result remains a container. It avoids type-category-dependent behavior where a primitive is implicitly extracted but a class remains wrapped. That kind of desugaring makes generic code harder to reason about and can create different unwrap rules for otherwise equivalent alternatives.

Use `unwrap<Q>()` when a value should explicitly leave the container. Use `map<Q>` to transform one alternative while remaining optional, `and_then` to chain an Option-returning operation, and `match()` to make alternative handling exhaustive. This is deliberate strictness: the caller, not a primitive-versus-class heuristic, decides when extraction happens.

### Does `Option<T>` accept a bare `T` in its constructor?

No. Engaged construction must use `Some<T>`, for example `rl::Option<T> value = rl::Some<T>(payload)`. The `Option` constructor from `Some<T>` is non-explicit, so this copy-initialization form works. The header has a separate bare-value assignment overload, but using `Some<T>` there also documents the intended alternative.

### Why does construction require `Some<T>`?

The wrapper makes the engaged state and intended payload explicit. It prevents a primitive, string literal, or class value from silently selecting an alternative through conversion ranking. Copy initialization from `Some<T>` remains convenient while creation of the wrapper itself is intentional.

### Why are `True_t` and `False_t` separate from `true` and `false`?

They are distinct marker types for the two values of a bool alternative. A plain bool is a runtime value and does not prove to the type checker that both bool cases are covered. `rl::True` and `rl::False` (also exposed through `True_t` and `False_t`) let the exhaustive checker distinguish the cases.

### Why is exhaustiveness required? Can it be disabled?

The current API requires exhaustive type coverage. There is no switch to disable it. `SomeAny` and `Wild` provide concise catch-alls when every alternative does not need a separate arm. A guard alone cannot prove that it will match at runtime, so a fallback is still needed when guards may all fail.

### Why does failure abort? Is this Rust panic?

The internal failure path is named `panic_`, but it is not Rust-style unwinding. A bad checked access or a runtime match with no matching arm prints a diagnostic and calls `std::abort()`; C++ destructors do not unwind through that failure. This treats violated access assumptions as fatal programming errors.

The library does not promise that it never throws. Payload construction, move/copy, assignment, comparison, hash operations, and user callbacks can throw ordinary C++ exceptions when exceptions are enabled. Those exceptions propagate according to the operation and are distinct from aborting checked-access failures.

### Is it safe?

The checked accessors detect an empty or wrong active type before reading it, and the constrained combinators reject callbacks that cannot handle every declared alternative. The implementation also requires non-throwing destructors. Those properties do not make every operation non-throwing or eliminate dangling pointers, stale references, callback exceptions, or undefined behavior from `unwrap_unchecked()`. Treat it as a strict value type, not as a borrow checker or a universal safety proof.

### Why use this library?

Use it when one value must represent absence plus a closed set of mutually exclusive types, and when typed matching or explicit targeted combinators improve the call site's clarity. For plain optional storage, `std::optional` may be simpler. For standard error propagation, use `std::expected`. For maximum portability, a standard vocabulary type is usually preferable.

### Why is the implementation so complex?

The implementation combines owning union storage, conditional triviality, per-alternative operation dispatch, constrained all-alternative combinators, targeted overloads, exhaustive pattern checking, value-category forwarding, result materialization, and diagnostics. Each feature supports a specific contract, but together they increase source size, compiler work, and maintenance burden. The abstractions are not free; prefer a simpler standard type when these behaviors are unnecessary.

### Why can it take a long time to compile?

Each alternative participates in storage and operation dispatch. Combinator constraints check callback validity across the alternative pack, while `match()` checks pattern applicability and result types across arms and alternatives. Larger packs and many distinct arm/callback types create more template instantiations and diagnostics. Keep alternative packs small, use the targeted overload when only one type matters, and avoid repeating large visitor definitions across many translation units when project organization permits.

### Is template metaprogramming Turing-complete?

C++ template instantiation and constant evaluation are Turing-complete in principle. That is a statement about the language's expressive power, not a practical target for this library. Compiler depth, memory, time, and diagnostic limits are finite; template machinery should encode useful constraints and dispatch, not arbitrary computation.

### Is it slower than `std::visit`?

In the supplied benchmark, yes: `std::visit` was faster than `rl::Option::match()` by 13.8% in median elapsed time.

| Metric | `rl::Option::match()` (`a.out`) | `std::visit` (`Visitor.out`) | Difference |
|---|---:|---:|---:|
| Task-clock | 1.37 ms | **1.14 ms** | 16.8% lower |
| CPU cycles | 808,730 | **673,950** | 16.7% lower; 134,780 cycles saved |
| Instructions | 985,165 | **878,731** | 10.8% lower; 106,434 instructions eliminated |
| Branches | 224,276 | **216,802** | 3.3% lower |
| Branch misses | **4,494** | 4,835 | 7.6% higher |
| Median elapsed time | 1.656 ms | **1.427 ms** | 13.8% faster |

This is evidence that `std::visit` is faster for the measured workload, not a universal percentage for every alternative set, compiler, or CPU. `match()` finds the active alternative with a linear discriminator scan and tests compatible arms in order, which can add dispatch work; the timing alone does not isolate how much of the gap comes from that path. For performance-sensitive code, benchmark the actual workload and consider `std::variant`/`std::visit` when its semantics fit.

### Why are callbacks and results kept wrapped?

Keeping `map` results in an Option gives one predictable type flow across primitive and class payloads. A nested Option returned by `map` remains a payload; use `and_then` when flattening the callback's Option is intended. Non-void `match()` also materializes branch results inside an Option result. Explicit unwrapping keeps extraction visible at the point where the caller has decided absence is handled.

### Can this later target C++20, C++17, C++14, or C++11?

This revision supports C++26 only. A backport would be substantial: it would need replacements for explicit object parameters, pack indexing, `std::forward_like`, `std::unreachable`, `std::views::zip`, and other newer language/library facilities, as well as changes to the matching and constraint implementation. C++20 may be a more plausible redesign target than C++11, but it is not a mechanical switch and is not supported by this header.

### Is there a free `Match(expression, arms...)` function?

No. The public interface is `option.match(...)`. The source demo defines a local `match_value` adapter that wraps an expression in a temporary `Option<T>`; see §6.6. It is an example, not a library API.

### How do I avoid dangling references and pointers?

Keep access references and pointer views within the lifetime of the owning Option and do not mutate its active state while they are in use. `RL_LIFETIME_BOUND` can help diagnostics but cannot enforce a borrow relationship. If a value must outlive the Option, copy or move it out by value.

### Does `match()` return a raw handler result?

No. All-void handlers produce `void`; non-void handlers produce an Option-wrapped materialized result. Use `match<void>` to discard values. See §§6.7 and 16.4.

---

## 21. Test-suite map

These tests are useful executable references for the documented rules. They are listed as navigation aids; this README update does not claim that they were run.

| Test file | What it demonstrates |
|---|---|
| `tests/test_active_mapping.cpp` | All-alternative mapping, targeted mapping, and the three alternative rules |
| `tests/RLOption_tests.cpp` | Positive behavior and compile-time rejection of callbacks that miss alternatives |
| `tests/match_exhaustive_positive.cpp` | Bool markers, catch-alls, value patterns, and exhaustive matching |
| `tests/match_exhaustive_negative.cpp` | Rejection of missing bool cases, missing `None`, and partial coverage |
| `tests/match_return_types_positive.cpp` | Automatic Option wrapping, heterogeneous results, common result types, and `match<void>` |
| `tests/match_return_types_negative.cpp` | Rejection of mixed void and value-returning arms |
| `tests/match_materialized_results.cpp` | By-value materialization of handler results, including references and move-only values |
| `tests/match_constraints.cpp` | Constraint participation and callback/category requirements |
| `tests/match_receiver_categories.cpp` | Forwarding based on const/lvalue/rvalue receiver category |
| `tests/match_exception_propagation.cpp` | Ordinary callback exceptions propagate through matching |
| `tests/dispatch_optimizations.cpp` | Compile-time elimination of inapplicable arms and dispatch order |
| `tests/lifetime_regressions.cpp` | Replacement ordering, conversions, and lifetime regressions |
| `tests/storage_reentrancy.cpp` | Discriminator boundaries and destruction reentrancy behavior |
| `tests/monadic_materialization_constraints.cpp` | Combinator callback and result materialization constraints |
| `tests/test_typed_lambda.cpp` | Predicate patterns, handlers, and `SomeAny` use |

---

## 22. License

This project is licensed under the GNU General Public License, version 3 or later.

SPDX-License-Identifier: `GPL-3.0-or-later`

See the repository's `LICENSE` file for the full license text.
