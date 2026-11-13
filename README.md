<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# `rl::Option`

`rl::Option<T, Ts...>` is a single-header C++23 value container. It stores either no value or exactly one object from a closed set of alternative types. Its API combines a primary-value interface, alternative-aware access, functional operations, and a pattern-matching dispatcher.

This document describes the API implemented by `include/rl_option.hpp`. It intentionally distinguishes implemented behavior from behavior that may be familiar from `std::optional`, `std::variant`, `std::expected`, or Rust. Those types are useful points of comparison, but `rl::Option` is not a drop-in replacement for any of them.

Besides the public API, this document also describes how the header actually implements that API internally — storage layout, discriminator values, dispatch mechanisms, and exception-safety ordering. Those internal descriptions are read directly off the current revision of `include/rl_option.hpp`. They are not part of the stable public contract: §16 already warns against depending on exact layout, and that warning applies to every internals paragraph added below as much as to the ones that were already there.

> **Important:** the first template argument is the *primary type*. Most optional-style accessors and combinators operate only on that type. The remaining template arguments are secondary alternatives and must be handled explicitly or through `match()`.

**Header:** `include/rl_option.hpp`
**Namespace:** `rl`
**Language baseline:** C++23
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
21. [License](#21-license)

---

## 1. What the type represents

### 1.1 The state model

`rl::Option<T, Ts...>` has one empty state and one engaged state for each listed alternative.

| Type | States it can hold |
|---|---|
| `rl::Option<T>` | `None` or a `T` |
| `rl::Option<T, E>` | `None`, a `T`, or an `E` |
| `rl::Option<A, B, C>` | `None`, an `A`, a `B`, or a `C` |
| `rl::Option<A, B, C, Ts...>` | `None`, an `A`, a `B`, `C` or multiple alternatives types |

An instance never intentionally holds two alternatives at once. The first template parameter is the primary type. Every later parameter is a secondary alternative.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

enum class ReadError {
    not_found,
    permission_denied
};

int main() {
    rl::Option<int, ReadError, std::string> value(
        rl::Some(std::string("configuration"))
    );

    std::println("engaged: {}", value.is_some());
    std::println("holds int: {}", value.is_some<int>());
    std::println("holds string: {}", value.is_some<std::string>());
}
```
The example is engaged, but it does not hold the primary `int`. That distinction is central to the API.

### 1.2 Primary and secondary alternatives

The primary type is not merely the first item in a type list. It determines the behavior of the optional-style operations.

| Operation family | What it observes |
|---|---|
| `is_some()`, `is_none()`, `operator bool()` | Whether any alternative is active |
| `is_some<Q>()`, `unwrap<Q>()`, `as_*_ptr<Q>()` | The specifically named alternative `Q` |
| `value()`, `unwrap()`, `expect()`, `operator*`, `operator->` | The primary type `T` only |
| `map`, `and_then`, `map_or`, `map_or_else`, `map_or_transform` | The primary type `T`; `map_or_transform` also has a special `None` callback path |
| `is_some_and`, `is_none_or`, `unwrap_or*` | The primary type `T` only |
| `or_else` | Any engaged alternative; it runs the callback only for `None` |
| `match()` | Every alternative symmetrically |

For example, when the secondary `std::string` alternative is active, `is_some()` is true but `value()` and `unwrap()` still try to access the primary `int`. Those primary accessors fail rather than returning the string.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("secondary"))
    );

    std::println("engaged: {}", value.is_some());
    std::println("holds int: {}", value.is_some<int>());
    std::println("holds string: {}", value.is_some<std::string>());

    // This is a checked primary-type access and would abort:
    // value.value();

    std::println("secondary: {}", value.unwrap<std::string>());
}
```
Use the explicitly typed overloads or `match()` whenever a secondary alternative may be active.

### 1.3 Comparison with standard vocabulary types

The library offers related capabilities, but its semantics are distinct.

| Standard facility | Related `rl::Option` shape | Important difference |
|---|---|---|
| `std::optional<T>` | `rl::Option<T>` | `rl::Option<T>` has no default constructor; it is initialized with `Some`, `None`, or `Default`. |
| `std::variant<Ts...>` | `rl::Option<T, Ts...>` | `rl::Option` adds an explicit empty state and treats the first alternative as primary for many operations. |
| `std::expected<T, E>` | `rl::Option<T, E>` | This is not `std::expected` semantics. There is an additional `None` state, and primary-only combinators do not preserve a secondary error alternative. |

For ordinary error propagation, `std::expected<T, E>` is generally the more direct standard abstraction. `rl::Option<T, E>` can represent a value, an error alternative, or no result, but callers must account for those three states explicitly.

---

## 2. Requirements and installation

### 2.1 Language and library requirements

The header rejects translation units where `__cplusplus` is less than `202302L`.

The implementation uses C++23 language and library facilities, including:

- explicit object parameters (`this Self&&`) for the ref-qualified `match()` implementation;
- `std::forward_like`;
- `std::unreachable`;
- `std::views::zip`;
- `std::println` and `<print>`;
- concepts and constrained member functions;
- `if consteval` / `if !consteval` syntax where exception-aware cleanup is selected.

A compiler accepting `-std=c++23` is not, by itself, sufficient. Its standard library must also provide the C++23 library facilities used by the header, especially `<print>`, `std::views::zip`, and `std::forward_like`.

The header is header-only. It does not require a separate `rl_option` object file or a project-specific shared library.

### 2.2 C++26 support

C++26 is not the language baseline for the public API. The header's minimum-standard check is C++23.

There is an optional reflection path for diagnostic type names. When the required reflection feature-test macros are visible, the header includes `<meta>` and uses `std::meta::display_string_of`. Otherwise, it falls back to parsing `__PRETTY_FUNCTION__`.

This optional path changes diagnostic type-name formatting. It does not add a separate public API or change the value semantics.

The header's include-guard name, `RL_OPTION_CXX26_HPP`, is an implementation detail; it does not mean that C++26 is required.

### 2.3 Compiler notes

The supplied header does not define a tested compiler-version matrix. Select a compiler and standard library that implement the required C++23 features.

The fenced C++ translation units in this README were individually syntax-checked with GCC 14.2 using `-std=c++23` against the supplied header. This records one verified compiler/standard-library configuration; it is not a guarantee that every compiler or standard-library combination is supported.

The non-reflection diagnostic fallback uses `__PRETTY_FUNCTION__`, which is a compiler extension commonly provided by GCC and Clang. The header does not provide a corresponding `__FUNCSIG__` fallback in the supplied revision. Do not assume that a non-reflection MSVC build is supported without compiling it against the actual toolchain.

### 2.4 Add the header to a project

Place the header at `include/rl_option.hpp`, then compile a translation unit that includes it.

```bash
g++ -std=c++23 -Wall -Wextra -pedantic -I./include main.cpp -o app
./app
```

A minimal translation unit:

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> count(rl::Some(42));
    std::println("{}", count.unwrap());
}
```
### 2.5 Using the header with Compiler Explorer

For a Compiler Explorer instance that has the header installed on its include path, use:

```text
-std=c++23 -Wall -Wextra -pedantic
```

If the instance is self-hosted, add the directory containing `rl_option.hpp` to the compiler's include path, for example with `-I/path/to/include`.

A public Compiler Explorer session cannot resolve an arbitrary path on your own computer. In that environment, make the header available through the session's supported library mechanism or include its contents in the source being compiled. Compiler and standard-library availability varies by selected toolchain; do not assume that every listed compiler supports every C++23 library facility used here.

### 2.6 Practical considerations for publishing this on Compiler Explorer

Sharing a Compiler Explorer link puts the header in front of readers who did not choose, and may not know, which compiler and standard-library combination the link happens to use. A few consequences follow directly from §2.1–§2.3:

- "Accepts `-std=c++23`" is necessary but not sufficient. `<print>`, `std::views::zip`, and `std::forward_like` landed in different standard-library releases on different vendors' timelines, so two compilers that both claim C++23 support can still disagree on whether this header compiles at all. Pin an exact compiler version in the shared link's description rather than a bare "any C++23 compiler" claim.
- The reflection-vs-`__PRETTY_FUNCTION__` fork in §2.2 and §16.5 is picked per translation unit based on feature-test macros, not by anything this library controls. A reader who forks the link onto a different compiler on the same site can get differently formatted (but behaviorally identical) abort diagnostics without changing a single line of their own code. Don't build an expected-output comparison in a Compiler Explorer "diff" view around the exact text of an abort message (see §18.8).
- Compiler Explorer's compiler list changes over time, independent of this header's own revisions. A link that worked when it was published can stop compiling months later purely because the pinned compiler version was retired from the site, with nothing in this header having changed. If the library is meant to be a long-lived reference example, periodically re-open and re-save the link rather than treating "it compiled once" as a permanent property.
- Every example in this README was written to compile standalone with a single `#include "rl_option.hpp"`. When adapting one for a Compiler Explorer session that only has the header, not the file tree, around it, keep that property: don't let an example quietly depend on a previous example's `using` declarations or helper functions that live in an earlier, unshared snippet.

---

## 3. The vocabulary types

The namespace `rl` exposes a small set of tag and wrapper types used for construction and matching.

### 3.1 `rl::Some<T>`: construct an engaged alternative

`Some<T>` wraps one value of exactly type `T`. It is the supported construction route for an engaged `Option`.

Its constructors are explicit:

```cpp
#include "rl_option.hpp"

#include <string>

int main() {
    rl::Some<int> first(42);
    auto second = rl::Some(42);

    rl::Some<std::string> third(std::string("ready"));

    rl::Option<int> count(first);
    rl::Option<std::string> label(third);
}
```
`get()` preserves the wrapper's value category. An lvalue wrapper exposes an lvalue reference; an rvalue wrapper exposes an rvalue reference:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <utility>

int main() {
    rl::Some<std::string> wrapped(std::string("ready"));
    wrapped.get().append(" now");

    const rl::Some<std::string> fixed(std::string("read-only"));
    const std::string& view = fixed.get();

    std::string extracted = std::move(wrapped).get();
    std::println("{} / {}", view, extracted);
}
```
The class provides:

| Member | Meaning |
|---|---|
| `Some()` | Default-constructs the wrapped `T`; available only when `T` is default-constructible |
| `Some(const T&)` | Copies a `T`; available only when `T` is copy-constructible |
| `Some(T&&)` | Moves a `T`; available only when `T` is move-constructible |
| `get() const&` | Returns `const T&` |
| `get() &` | Returns `T&` |
| `get() &&` | Returns `T&&` |
| `get() const&&` | Returns `const T&&` |
| `operator==(Some<T>, Some<T>)` | Compares wrapped values when `T` is equality-comparable |

`Some<T>` is also a pattern for matching an engaged value equal to the wrapped value. See the alignment constraint in [§6.5](#65-value-patterns-somevalue-and-ordinary-comparison-patterns).

### 3.2 `rl::None`: explicitly empty

`rl::None` represents the empty state. It does not construct, inspect, or require a `T`.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> value(rl::None);

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

int main() {
    rl::Option<int> number(rl::Default);
    rl::Option<std::string> text(rl::Default);
    rl::Option<NoDefault, std::string> fallback(rl::Default);

    std::println("number: {}", number.unwrap());
    std::println("text is empty: {}", text.unwrap().empty());
    std::println("selected string: {}", fallback.is_some<std::string>());
}
```
The `Default` tag is also available as a pattern. As a match pattern, it compares an active alternative against a default-constructed object of that same alternative type, provided the type is default-constructible and equality-comparable. This is not the same selection rule as constructing an `Option` from `Default`.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> value(rl::Some(0));

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

int main() {
    rl::Option<int> value(rl::None);

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

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("payload"))
    );

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

### 3.6 `rl::Where(predicate)`: name a guard

`Where` stores a predicate and marks the arm as a guard. A bare predicate callable can also be used directly as a pattern; `Where` is a named wrapper for readability.

The predicate is tested against a `const` view of the active alternative. The handler receives the matched alternative with the value category of the `Option` expression.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> value(rl::Some(18));

    value.match(
        rl::None >>= [] {
            std::println("no value");
        },
        rl::Where([](const int& number) {
            return number >= 18;
        }) >>= [](int number) {
            std::println("at least 18: {}", number);
        },
        rl::Wild >>= [](int number) {
            std::println("below 18: {}", number);
        }
    );
}
```
A predicate's being considered applicable to a type is a compile-time property. The predicate's runtime result is not proven at compile time.

Internally, `Where(pred)` is nothing more than `Where_<std::decay_t<Pred>>{pred}` — a one-member struct holding the predicate plus a nested `where_tag_` type alias that the pattern-matching machinery uses to recognize it as a named guard rather than an ordinary callable pattern. A bare predicate lambda used directly as a pattern is matched the same way at runtime; the only thing `Where` adds is that `where_tag_` marker, which exists purely so the intent "this is deliberately a guard" is legible both to a reader and to the small number of `if constexpr` branches in the matcher that special-case guards.

---

## 4. Constructing and assigning values

### 4.1 There is no default constructor

`Option()` is explicitly deleted. A declaration that attempts to default-construct an object is ill-formed:

```cpp
// rl::Option<int> value;  // Error: Option() is deleted.
```
There is no default-empty state hidden behind either an omitted initializer or
an empty initializer list. Call an actual constructor with the state tag you
intend.

Choose the desired initial state explicitly:

```cpp
#include "rl_option.hpp"

int main() {
    rl::Option<int> empty(rl::None);
    rl::Option<int> zero(rl::Default);
    rl::Option<int> answer(rl::Some(42));
}
```
`Default` is a tag-based construction request. It is not a default constructor.

### 4.2 Construct from `Some<T>`

Construction from `Some<T>` is available for the primary type and for each declared secondary type.

```cpp
#include "rl_option.hpp"

#include <string>

int main() {
    rl::Option<int> count(rl::Some(7));

    rl::Option<int, std::string> message(
        rl::Some(std::string("ready"))
    );

    rl::Option<int, std::string> code(
        rl::Some<int>(200)
    );
}
```
Use the explicit `Some<T>` spelling when the intended alternative should be unmistakable.

### 4.3 Construct from `None` or `Default`

```cpp
#include "rl_option.hpp"

#include <string>

int main() {
    rl::Option<int, std::string> absent(rl::None);
    rl::Option<int> initialized(rl::Default);
}
```
`None` always works for a valid `Option`. `Default` works only if the primary type or at least one secondary alternative is default-constructible.

### 4.4 A bare value is not an `Option` constructor argument

The supplied header does **not** declare a constructor from an arbitrary payload value.

These forms do not compile:

```text
// rl::Option<int> a(42);
// rl::Option<int> b = 42;
// rl::Option<int, std::string> c(std::string("text"));
```

Construct with `Some` instead:

```cpp
#include "rl_option.hpp"

#include <string>

int main() {
    rl::Option<int> a(rl::Some(42));
    rl::Option<int, std::string> b(
        rl::Some(std::string("text"))
    );
}
```
This distinction matters in return statements too. A function returning `Option<int>` must return `Some(value)` or an already-constructed `Option<int>`; returning a bare `int` is not supported by an `Option` converting constructor.

### 4.5 Assign a bare value to an existing `Option`

The assignment operator does accept a bare payload, but this is an assignment overload, not a constructor. The object must already exist. Do not read the assignments below as examples of constructing an `Option` from a bare value: that constructor does not exist. It selects the target alternative using the following rule:

1. If exactly one alternative has the same type as the assigned expression after removing cv- and reference-qualifiers, use that exact alternative.
2. Otherwise, if exactly one alternative is constructible from the assigned expression, use that alternative.
3. If there is no viable alternative or more than one constructible alternative, the assignment is rejected.

```cpp
#include "rl_option.hpp"

#include <string>

int main() {
    rl::Option<int, std::string> value(rl::None);

    value = rl::Some<int>(9);           // Explicit alternative selection.
    value = rl::Some<std::string>("x"); // Explicit alternative selection.
    value = rl::None;
    value = rl::Default;
}
```
An ambiguous assignment is rejected. For example, if both `int` and `long` can be constructed from a `short`, the target cannot be chosen uniquely:

```cpp
#include "rl_option.hpp"

int main() {
    rl::Option<int, long> value(rl::None);

    // value = short(5);            // Error: int and long are both viable.
    value = rl::Some<long>(5);      // Explicitly select long.
}
```
Internally this bare-value assignment path does something a naive "reset, then construct" implementation would not: when the deduced target alternative is movable, it first constructs a temporary of that target type *from the assigned expression*, while whatever alternative was previously active is still alive and untouched, and only afterwards resets the old alternative and move-constructs the real storage from that temporary. See [§16.3](#163-exception-safety-during-state-changes) for why this ordering matters and what it protects against.

### 4.6 Copy and move operations

Copy construction is available only when every alternative is copy-constructible. Move construction is available only when every alternative is move-constructible.

Copy assignment additionally requires every alternative to be copy-assignable. Move assignment additionally requires every alternative to be move-assignable.

The implementation conditionally preserves trivial copy/move operations where the alternatives permit it.

```cpp
#include "rl_option.hpp"

#include <memory>
#include <type_traits>
#include <utility>

struct MoveOnly {
    explicit MoveOnly(int value)
        : value(std::make_unique<int>(value)) {}

    std::unique_ptr<int> value;
};

int main() {
    using Value = rl::Option<MoveOnly>;

    static_assert(!std::is_copy_constructible_v<Value>);
    static_assert(std::is_move_constructible_v<Value>);

    Value source(rl::Some(MoveOnly(7)));
    Value destination(std::move(source));

    // Moving an Option does not itself reset the source.
    // The source remains engaged with a moved-from MoveOnly.
    (void)destination;
}
```
When both sides of a copy or move *assignment* already have the same alternative active, the implementation assigns directly onto the existing object (falling back to destroy-and-reconstruct only if that alternative type isn't copy-/move-assignable) rather than always tearing down and rebuilding; when the active alternatives differ, or either side is `None`, it resets the destination and constructs the new alternative from scratch. See [§16.1](#161-storage-model) for the dispatch mechanism behind this.

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

rl::Option<int> parse_integer(std::string_view text) {
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

    return rl::Some(value);
}

int main() {
    const auto parsed = parse_integer("42");

    if (parsed.is_some()) {
        std::println("parsed: {}", parsed.unwrap());
    } else {
        std::println("not an integer");
    }
}
```
The function returns `None` on failure and `Some(value)` on success. It does not return a bare integer.

### 5.2 Stage 2: validate with `and_then`

`and_then` is useful when the next operation itself returns an `Option`.

```cpp
#include "rl_option.hpp"

#include <charconv>
#include <print>
#include <string_view>
#include <system_error>

rl::Option<int> parse_integer(std::string_view text) {
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

    return rl::Some(value);
}

rl::Option<int> require_positive(int value) {
    if (value <= 0) {
        return rl::None;
    }

    return rl::Some(value);
}

int main() {
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

rl::Option<int> parse_integer(std::string_view text) {
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

    return rl::Some(value);
}

rl::Option<int> require_positive(int value) {
    if (value <= 0) {
        return rl::None;
    }

    return rl::Some(value);
}

int main() {
    const auto doubled = parse_integer("21")
        .and_then(require_positive)
        .map([](int value) {
            return value * 2;
        });

    std::println("result: {}", doubled.unwrap_or(-1));
}
```
If the receiver is empty, or a secondary alternative is active, `map` returns an empty `Option<U>`. It does not carry secondary alternatives into the new result.

### 5.4 Stage 4: choose an eager or lazy fallback

Use `unwrap_or` for an already available fallback and `unwrap_or_else` for a fallback that should be computed only when required.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<std::string> missing(rl::None);

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

rl::Option<int, ParseError> parse_with_error(std::string_view text) {
    if (text.empty()) {
        return rl::Some(ParseError::empty_input);
    }

    int value = 0;
    const auto [end, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value
    );

    if (error != std::errc{} || end != text.data() + text.size()) {
        return rl::Some(ParseError::malformed_input);
    }

    return rl::Some(value);
}

int main() {
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

An arm is formed by applying the overloaded `>>=` operator to a pattern and a handler:

```text
pattern >>= handler
```

The header imports this operator into the global namespace so the infix form can be used without qualifying the operator name. The pattern and handler are stored together in an internal arm object.

Arms are evaluated in source order. The first arm that matches is selected.

### 6.2 Supported pattern forms

| Pattern | Matches |
|---|---|
| `rl::None` | The empty state only |
| `rl::Wild` | The empty state and all engaged alternatives |
| `rl::SomeAny` | Every engaged alternative, but not `None` |
| `rl::Default` | An active alternative equal to its default-constructed value, when that alternative is default-constructible and equality-comparable |
| `rl::Some<T>(value)` | An active alternative of exactly type `T` whose value compares equal to `value` |
| `rl::Where(predicate)` | An alternative for which the predicate is applicable and returns true |
| A predicate callable | Same guard behavior as `Where(predicate)` |
| An equality-comparable value | An active alternative for which `active_value == pattern` is valid and true |
| A nullary boolean callable | A guard that returns the same boolean result for each applicable active alternative |

A plain `bool` pattern is treated as a boolean value pattern for a `bool` alternative and as a constant guard for other alternative types.

### 6.3 Handler forms

For an engaged state, the handler may:

- accept the matched alternative;
- accept no arguments; or
- be a non-callable object convertible to the deduced dispatch result type.

For `None`, the handler is invoked without an argument. A `Wild` handler must therefore support the empty-state call as well if no earlier `None` arm handles it.

For a multi-type `Option`, a catch-all handler must be valid for every alternative it covers. A generic lambda is usually the most direct handler:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <type_traits>

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("text"))
    );

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        rl::SomeAny >>= [](auto&& active) {
            using Active = std::remove_cvref_t<decltype(active)>;

            if constexpr (std::same_as<Active, int>) {
                std::println("integer: {}", active);
            } else {
                std::println("string: {}", active);
            }
        }
    );
}
```
### 6.4 Type coverage is checked; runtime guard success is not

`match()` performs compile-time checks that:

1. at least one arm covers `None`;
2. the primary type has a covering arm; and
3. every secondary alternative has a covering arm.

A typed guard is considered a covering arm for an alternative when the guard is invocable with that alternative's `const` view. The compiler cannot prove that the predicate will return true for a particular runtime value.

If all applicable runtime guards return false and no later arm matches, `match()` prints a diagnostic and calls `std::abort()`.

Therefore, a complete match should normally end with a suitable catch-all arm after more specific guards.

### 6.5 Value patterns: `Some(value)` and ordinary comparison patterns

There is an implementation-specific alignment constraint in the supplied header. Each arm statically requires:

```text
alignof(handler_type) >= alignof(pattern_type)
```

`Some<T>` is declared with `alignas(T)`. A raw value pattern such as an `int` also has the alignment of its type. Typical lambda closure types often have alignment 1. Consequently, a value pattern for an ordinarily aligned type can fail the arm's `static_assert` when paired with a typical lambda handler.

The following example provides a handler type with sufficient alignment:

```cpp
#include "rl_option.hpp"

#include <print>

struct alignas(int) ExactZero {
    void operator()() const {
        std::println("the value is zero");
    }
};

int main() {
    rl::Option<int> value(rl::Some(0));

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        rl::Some(0) >>= ExactZero(),
        rl::Wild >>= [](int number) {
            std::println("other value: {}", number);
        }
    );
}
```
For ordinary matching, `Where` is generally easier to use because a typical predicate lambda and a typical handler lambda have compatible alignment:

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> value(rl::Some(42));

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        rl::Where([](const int& number) {
            return number == 42;
        }) >>= [] {
            std::println("matched 42");
        },
        rl::Wild >>= [](int number) {
            std::println("other: {}", number);
        }
    );
}
```

Alternatively, it is possible to use basic lambdas in place of Where. It is mostly user-preference to keep it that way. Mind your preference.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> value(rl::Some(42));

    value.match(
        rl::None >>= [] {
            std::println("empty");
        },
        [](const int& number) {
            return number == 42;
        } >>= [] {
            std::println("matched 42");
        },
        rl::Wild >>= [](int number) {
            std::println("other: {}", number);
        }
    );
}
```
This alignment rule is not a lint or a style guideline — it is a hard `static_assert` inside `Arm_<Pattern, Handler>` itself, and it exists because of how `Arm_` is laid out. `Arm_` stores the pattern, just enough mid-padding to bring the handler up to its own alignment boundary, and then the handler, in that order, relying on the handler to supply the struct's overall trailing alignment. A pattern with *stricter* alignment than its handler would need padding bytes *after* the handler as well, and because the pattern is stored with `[[no_unique_address]]`, a `Pad_<>` placed after the handler would be the very last member of the class — with nothing following it to overlap into, `[[no_unique_address]]` cannot make that trailing padding free. Rather than silently pay for a byte the layout doesn't otherwise need, the header refuses the combination at compile time. See [§16.4](#164-match-dispatch-mechanics) for the rest of how arms are laid out and dispatched.

---

## 7. State queries

All state queries are `constexpr` and inspect the active-state discriminator. The relevant functions are:

| Member | Meaning |
|---|---|
| `explicit operator bool() const` | True when any alternative is engaged |
| `is_some() const` | True when any alternative is engaged |
| `has_any_engaged_value() const` | Synonym for `is_some()` |
| `is_none() const` | True only for the empty state |
| `is_some<Q>() const` | True only when alternative `Q` is active |
| `is_default() const` | True when the primary value is equal to `T{}` |

`is_default()` is available only when the primary type is both default-constructible and equality-comparable. It returns false for `None` and for every secondary alternative.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int, std::string> value(rl::Some(std::string("secondary")));

    std::println("bool: {}", static_cast<bool>(value));
    std::println("is_some: {}", value.is_some());
    std::println("has value: {}", value.has_any_engaged_value());
    std::println("is_none: {}", value.is_none());
    std::println("is int: {}", value.is_some<int>());
    std::println("is string: {}", value.is_some<std::string>());

    rl::Option<int> zero(rl::Some(0));
    rl::Option<int> nonzero(rl::Some(7));
    rl::Option<int> empty(rl::None);

    std::println("zero is default: {}", zero.is_default());
    std::println("seven is default: {}", nonzero.is_default());
    std::println("empty is default: {}", empty.is_default());
}
```
### 7.1 `is_some_and(predicate)`

`is_some_and` returns true only when the primary alternative is active and the predicate returns true.

It accepts a predicate invocable with the receiver's corresponding primary-value category:

- `const T&` on `const&`;
- `T&` on `&`;
- `T&&` on `&&`.

The `&&` overload consumes and resets an active primary value when the predicate call exits, including during exception unwinding. This happens whether the predicate returns `true` or `false`. If the state is `None` or a secondary alternative, the predicate is not called and the object is not consumed.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> age(rl::Some(21));

    const bool is_adult = age.is_some_and([](int value) {
        return value >= 18;
    });

    std::println("adult: {}", is_adult);
}
```
### 7.2 `is_none_or(predicate)`

`is_none_or` returns true when the state is `None`, or when the primary alternative is active and the predicate returns true.

A secondary alternative is **not** treated as `None`; for a secondary-active `Option`, `is_none_or` returns false. On an rvalue receiver, an active primary value is consumed and the `Option` is reset after the predicate call regardless of its boolean result. `None` is already empty, and a secondary alternative is left untouched.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int, std::string> absent(rl::None);
    rl::Option<int, std::string> primary(rl::Some(12));
    rl::Option<int, std::string> secondary(
        rl::Some(std::string("not primary"))
    );

    const auto below_limit = [](int value) {
        return value <= 20;
    };

    std::println("none: {}", absent.is_none_or(below_limit));
    std::println("primary: {}", primary.is_none_or(below_limit));
    std::println("secondary: {}", secondary.is_none_or(below_limit));
}
```
---

## 8. Reading values and handling absence

### 8.1 Checked primary access: `value()`, `unwrap()`, and `expect()`

These functions access only the primary type `T`.

| Member | Lvalue result | Rvalue result |
|---|---|---|
| `value()` | `T&` or `const T&` | `T&&` |
| `unwrap()` | `T&` or `const T&` | `T` by value |
| `expect(message)` | `T&` or `const T&` | `T` by value |

If the primary alternative is not active, these functions print a diagnostic to `stderr` and call `std::abort()`.

`value() &&` returns an rvalue reference and does not itself reset the `Option`.

By contrast, `unwrap() &&` and `expect() &&` extract by value and reset the source after a successful extraction — "successful" is doing real work here, not just describing the common case; see [§16.3](#163-exception-safety-during-state-changes) for what happens if the extraction itself throws.

The pointer and dereference operators provide the familiar optional-style interface:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<std::string> name(
        rl::Some(std::string("Ada Lovelace"))
    );

    std::string& mutable_name = name.value();
    mutable_name.append(" (mathematician)");

    std::println("length through operator*: {}", (*name).size());
    std::println("length through operator->: {}", name->size());

    const std::string& checked = name.expect("name must be present");
    std::println("{}", checked);
}
```
```cpp
#include "rl_option.hpp"

#include <print>
#include <string>
#include <utility>

int main() {
    rl::Option<std::string> value(
        rl::Some(std::string("payload"))
    );

    std::string extracted = std::move(value).unwrap();

    std::println("extracted: {}", extracted);
    std::println("source is empty: {}", value.is_none());
}
```
### 8.2 Typed access: `unwrap<Q>()`

`unwrap<Q>()` accesses a specifically named alternative, including a secondary alternative.

- On `const&`, it returns `const Q&`.
- On `&`, it returns `Q&`.
- On `&&`, it returns `Q` by value and resets the source after successful extraction.

The requested `Q` must be one of the declared alternatives. Accessing the wrong active alternative aborts.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("message"))
    );

    std::println("{}", value.unwrap<std::string>());

    // value.unwrap<int>(); // Would abort: int is not active.
}
```
### 8.3 `unwrap_none()`

`unwrap_none()` checks that no alternative is active and returns the `None` tag. It aborts if the `Option` holds any value, including a secondary alternative.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> absent(rl::None);
    const auto empty_tag = absent.unwrap_none();

    (void)empty_tag;
    std::println("the Option is confirmed empty: {}", absent.is_none());
}
```
### 8.4 `unwrap_unchecked()`

`unwrap_unchecked()` reads the primary storage without checking that the primary alternative is active.

It always returns `T` by value:

- `const&` and `&` overloads require `T` to be copy-constructible;
- `&&` requires `T` to be move-constructible.

Calling it when the primary alternative is not active is undefined behavior. It does not reset the source.

This operation is not a faster checked accessor. It is an unchecked storage access and should be avoided in ordinary application code. The following example uses it only after checking the primary alternative:

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int, double> value(rl::Some(17));

    if (value.is_some<int>()) {
        const int copy = value.unwrap_unchecked();
        std::println("checked primary value: {}", copy);
    }
}
```
The check and the unchecked read must remain correctly related: do not mutate the `Option` between them, and do not use this function when another thread can mutate the same object without synchronization.

Mechanically, the `const&`/`&` overloads of `unwrap_unchecked()` are nothing more than `*static_cast<const T*>(address of the primary storage slot)` (or the non-`const` equivalent) — there is no branch on the discriminator at all, not even in a debug build. The `&&` overload is the same read wrapped in `std::move`. If the primary alternative is not actually active, this reads whatever object (or partially-formed/uninitialized bytes) currently occupies that union slot as if it were a `T`, which is undefined behavior in the strict sense — not merely "wrong value," but a violation the compiler is entitled to use for optimization in ways that make the resulting behavior unpredictable beyond just returning a nonsense `T`.

### 8.5 Fallback operations

| Member | Behavior |
|---|---|
| `unwrap_or(fallback)` | Returns the primary value, or converts the supplied fallback to `T` |
| `unwrap_or_default()` | Returns the primary value, or constructs `T{}` |
| `unwrap_or_else(supplier)` | Returns the primary value, or calls a supplier and converts its result to `T` |

All three inspect only the primary type. `None` and a secondary alternative both select the fallback path.

`unwrap_or` evaluates its argument before the call. `unwrap_or_else` invokes its supplier only when required. The supplier can be nullary or accept one `None` argument. `rl::Default` itself can be used as a supplier for a default-constructible primary type.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<std::string> present(
        rl::Some(std::string("cached"))
    );
    rl::Option<std::string> missing(rl::None);

    std::println("present: {}", present.unwrap_or("fallback"));
    std::println("missing: {}", missing.unwrap_or_default());

    const auto generated = missing.unwrap_or_else([] {
        std::println("supplier invoked");
        return std::string("computed");
    });

    std::println("generated: {}", generated);
}
```
---

## 9. Functional operations

The methods in this section operate on the primary type unless explicitly stated otherwise. Each has `const&`, `&`, and `&&` overloads, with the callback receiving the corresponding primary-value category where applicable.

### 9.1 `map(operation)`

Transforms the primary value and returns `Option<U>`, where `U` is the non-`void` result type of the operation.

If the primary type is not active, it returns `Option<U>(None)`. A secondary alternative is not propagated.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int> count(rl::Some(21));
    rl::Option<int> missing(rl::None);

    auto text = count.map([](int value) {
        return std::to_string(value);
    });

    auto absent_text = missing.map([](int value) {
        return std::to_string(value);
    });

    std::println("{}", text.unwrap());
    std::println("is absent: {}", absent_text.is_none());
}
```
The operation must not return `void`.

### 9.2 `and_then(operation)`

`and_then` invokes an operation on the primary value. The operation must itself return an `rl::Option` specialization. The returned `Option` type is the operation's return type.

When the primary alternative is not active, `and_then` returns `None` converted to that return type.

```cpp
#include "rl_option.hpp"

#include <print>

rl::Option<int> reciprocal(int value) {
    if (value == 0) {
        return rl::None;
    }

    return rl::Some(1 / value);
}

int main() {
    rl::Option<int> input(rl::Some(2));

    auto result = input.and_then(reciprocal);

    std::println("reciprocal: {}", result.unwrap_or(-1));
}
```
### 9.3 `or_else(operation)`

`or_else` runs its callback only when the receiver is `None`. Any engaged alternative, including a secondary alternative, is preserved.

The callback must return a value convertible to the same `Option<T, Ts...>` specialization. It may be nullary or accept one `None` argument.

The `const&` and `&` overloads copy the engaged receiver and therefore require all alternatives to be copy-constructible. The `&&` overload moves the receiver and requires every alternative to be move-constructible.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int, std::string> error(
        rl::Some(std::string("already engaged"))
    );

    rl::Option<int, std::string> missing(rl::None);

    auto preserved = error.or_else([] {
        return rl::Option<int, std::string>(rl::Some(0));
    });

    auto supplied = missing.or_else([] {
        return rl::Option<int, std::string>(rl::Some(0));
    });

    std::println("preserved string: {}", preserved.unwrap<std::string>());
    std::println("fallback primary: {}", supplied.unwrap());
}
```
### 9.4 `map_or(operation, fallback)`

Applies an operation to the primary value or returns the fallback. The fallback is an ordinary value and is evaluated before the call.

The result type is the decayed type of the fallback. Both the operation's result and the fallback must be usable to construct that type.

```cpp
#include "rl_option.hpp"

#include <print>

int main() {
    rl::Option<int> present(rl::Some(21));
    rl::Option<int> absent(rl::None);

    const auto double_value = [](int value) {
        return value * 2;
    };

    const int first = present.map_or(double_value, -1);
    const int second = absent.map_or(double_value, -1);

    std::println("{} / {}", first, second);
}
```
### 9.5 `map_or_else(operation, supplier)`

Applies an operation to the primary value or invokes a nullary fallback supplier. Both branches must have the same result type after removing cv- and reference-qualifiers.

A secondary alternative also selects the fallback supplier.

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

int main() {
    rl::Option<int> present(rl::Some(42));
    rl::Option<int> absent(rl::None);

    const auto to_text = [](int value) {
        return std::to_string(value);
    };

    const auto no_value = [] {
        return std::string("not available");
    };

    const auto first = present.map_or_else(to_text, no_value);
    const auto second = absent.map_or_else(to_text, no_value);

    std::println("{} / {}", first, second);
}
```
### 9.6 `map_or_transform(operation, fallback)`

This method combines a primary transformation with a value fallback and has an additional `None` callback path.

Its behavior is:

1. If the state is `None` and `operation` accepts a `None` argument, invoke `operation(None)`.
2. If the state is `None` and `operation` does not accept `None`, return the fallback.
3. If the primary alternative is active and `operation` accepts the primary value, invoke it.
4. If a secondary alternative is active, return the fallback.
5. If the primary alternative is active but the operation is not invocable with it, return the fallback.

The result type is the decayed type of the fallback, and the invoked operation result must be constructible as that type.

```cpp
#include "rl_option.hpp"

#include <concepts>
#include <print>
#include <string>
#include <type_traits>

int main() {
    rl::Option<int, std::string> value(rl::None);

    const auto describe = [](auto&& item) -> std::string {
        using Item = std::remove_cvref_t<decltype(item)>;

        if constexpr (std::same_as<Item, decltype(rl::None)>) {
            return "empty";
        } else if constexpr (std::same_as<Item, int>) {
            return std::to_string(item);
        } else {
            return "secondary alternative";
        }
    };

    const auto empty_text = value.map_or_transform(
        describe,
        std::string("not the primary alternative")
    );

    value = std::string("secondary");

    const auto secondary_text = value.map_or_transform(
        describe,
        std::string("not the primary alternative")
    );

    std::println("{} / {}", empty_text, secondary_text);
}
```
### 9.7 Primary-only combinators on a multi-type `Option`

The distinction between primary and secondary alternatives is especially important for `Option<T, E>`.

- `map` on an active `E` returns `None` as `Option<U>`. The error value is not preserved.
- `and_then` on an active `E` returns `None` as the callback's `Option` return type. The error value is not preserved.
- `map_or`, `map_or_else`, and `unwrap_or*` use their fallback path for `E`.
- `or_else` treats `E` as engaged and preserves it without calling the fallback.
- `match()` can inspect and handle `T` and `E` separately.

This is why `Option<T, E>` must not be described as having `std::expected` error-propagation semantics.

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

A result-style producer should return `Some(value)` for success and `Some(error)` for a secondary error alternative:

```cpp
#include "rl_option.hpp"

#include <print>
#include <string_view>

enum class LookupError {
    not_found,
    unavailable
};

rl::Option<int, LookupError> lookup(std::string_view key) {
    if (key == "answer") {
        return rl::Some(42);
    }

    if (key.empty()) {
        return rl::Some(LookupError::not_found);
    }

    return rl::Some(LookupError::unavailable);
}

int main() {
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

Do not transform a result-shaped `Option<T, E>` with a primary-only combinator when the secondary value must survive.

Use `match()` to make the handling explicit:

```cpp
#include "rl_option.hpp"

#include <print>

enum class ArithmeticError {
    division_by_zero
};

rl::Option<int, ArithmeticError> divide(int numerator, int denominator) {
    if (denominator == 0) {
        return rl::Some(ArithmeticError::division_by_zero);
    }

    return rl::Some(numerator / denominator);
}

int main() {
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

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("old alternative"))
    );

    int& primary = value.emplace(12);
    primary += 1;

    std::println("primary: {}", value.unwrap());

    rl::Option<std::string> temporary(rl::None);
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

int main() {
    rl::Option<int> value(rl::Some(5));

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

int main() {
    rl::Option<int, std::string> source(
        rl::Some(std::string("payload"))
    );

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

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("before"))
    );

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

int main() {
    rl::Option<int, std::string> first(rl::Some(7));
    rl::Option<int, std::string> second(
        rl::Some(std::string("text"))
    );

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

int main() {
    rl::Option<int, std::string> value(
        rl::Some(std::string("hello"))
    );

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

There is no direct equality overload between an `Option` and an arbitrary bare payload. Use `Some(value)`.

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

int main() {
    rl::Option<int> empty(rl::None);
    rl::Option<int> zero(rl::Some(0));
    rl::Option<int> another_zero(rl::Some(0));

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

These failure paths print a diagnostic to `stderr` and call `std::abort()`. They do not throw a library-defined exception and do not perform normal stack unwinding.

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

The header conditionally compiles exception-specific cleanup using `__cpp_exceptions`. A C++23 build with exceptions disabled is possible when the compiler and standard library support the required facilities and the user's alternative types and callbacks are suitable for that mode.

Disabling exceptions does not make a potentially throwing user operation safe; it changes what the compiler accepts and how the program can report failures.

### 14.4 Choose the right failure mechanism

Use `None` or a secondary error alternative to represent ordinary, expected outcomes.

Use checked accessors when a failed access is a programming error and process termination is the intended policy.

Use `match()` when every state must be handled at a decision point.

Avoid `unwrap_unchecked()` unless the primary alternative has been independently proven active and the unchecked access is justified.

---

## 15. Constraints and edge cases

The constraints below are enforced by the supplied header or follow directly from its public operation contracts.

### 15.1 Alternative-type requirements

Every alternative must be:

- an object type;
- not an array type;
- not `volatile`-qualified;
- nothrow-destructible.

Reference types are not object types and are not supported. Use a pointer or `std::reference_wrapper<T>` when a non-owning reference-like alternative is required.

All alternatives must be distinct after removing cv-qualification. A specialization such as `Option<int, const int>` is rejected as a duplicate-alternative list.

### 15.2 No implicit default-empty state

`Option()` is deleted for every specialization. Initialize all objects explicitly with `Some`, `None`, or `Default`.

### 15.3 Bare-value construction and bare-value assignment differ

A bare payload is not a constructor argument. It is accepted by the assignment operator only after an `Option` object exists.

Use:

```cpp
#include "rl_option.hpp"

int main() {
    rl::Option<int> value(rl::None);
    value = rl::Some(42);
}
```
Do not use `rl::Option<int> value(42);` or `rl::Option<int> value = 42;`.

### 15.4 No cross-specialization converting constructor

`Option<T, Ts...>` has no general converting constructor from a differently-shaped `Option<U, Us...>`. Convert explicitly, for example by mapping a primary value into a new type or by matching each alternative and constructing the target specialization.

### 15.5 `Default` and `unwrap_or_default()` use different rules

`Default` can select the first default-constructible secondary alternative when the primary type is not default-constructible.

`unwrap_or_default()` always returns the primary type `T` and therefore requires `T` itself to be default-constructible. It does not search secondary alternatives.

### 15.6 Secondary alternatives are not primary values

An active secondary makes `is_some()` true, but it does not make `value()`, `unwrap()`, `map()`, or `and_then()` operate on that secondary.

Use `unwrap<Q>()`, `is_some<Q>()`, the typed pointer views, or `match()`.

### 15.7 Runtime guards can fail after static coverage succeeds

Compile-time coverage checks the types a pattern can accept. It cannot prove that a predicate returns true for every runtime object. End a guarded match with a suitable catch-all or otherwise prove that the guarded cases cover all possible values.

### 15.8 `match()` value-pattern alignment

A value pattern's type alignment must not exceed the handler type alignment. This implementation detail can reject common `Some<int>(...)` or raw integer patterns paired with ordinary lambdas. `Where` guards avoid this issue for the usual stateless or ordinarily aligned lambdas. See [§6.5](#65-value-patterns-somevalue-and-ordinary-comparison-patterns).

### 15.9 Rvalue operations may consume the primary value

The `&&` overloads of `map`, `and_then`, `is_some_and`, `is_none_or`, and the primary extraction/fallback functions can move from the primary value. Some successful rvalue paths reset the source; others return an rvalue reference or move the `Option` without resetting it.

Do not infer that every `std::move(option).function()` call leaves `option` empty. In particular:

- `std::move(option).value()` returns `T&&` and does not reset the source;
- `std::move(option).unwrap()` extracts by value and resets on success;
- `std::move(option).unwrap_unchecked()` returns by value without resetting;
- `std::move(option).or_else(...)` moves the engaged receiver into its return value; it does not explicitly reset the moved-from source;
- `std::move(option).is_some_and(predicate)` and `std::move(option).is_none_or(predicate)` consume an active primary value and reset the source after the predicate call, whether it returns `true` or `false`. Neither operation consumes a secondary alternative;
- the rvalue `map`, `and_then`, `map_or`, and `map_or_else` overloads consume an active primary value. Their non-primary paths do not consume a secondary alternative;
- `take()` is the explicit lvalue operation that returns the old `Option` and resets the source.

### 15.10 Pointer-view lifetime

The pointers returned by `as_mut_ptr` and `as_immut_ptr` point into the original `Option`. They do not own the object or extend its lifetime. The rvalue overloads are deleted for this reason.

### 15.11 No built-in formatter

The header does not provide `std::formatter` for `rl::Option`. Formatting an `Option` directly with `std::print` or `std::println` is not supported by this header. Extract or match the contained value and format that instead.

### 15.12 No built-in synchronization

`rl::Option` contains no mutex or atomic synchronization. Concurrent access that includes a write to the same instance requires external synchronization, just as it would for an ordinary object.

---

## 16. Implementation notes

### 16.1 Storage model

The implementation uses manually managed storage for the primary and secondary alternatives, plus an active-state discriminator. It maintains alignment requirements for the stored alternatives and the discriminator.

Concretely, for `Option<T, Args...>`:

- **The discriminator is a single integer, not a per-type flag.** Its type is chosen by a small trait (`option_index_type_<N>`, with `N = sizeof...(Args) + 1`): `unsigned char` if `N` fits, else `unsigned short`, else `std::size_t`. The value `0` means `None`. The value `1` means the primary type `T` is active. Each secondary alternative occupies `2, 3, 4, ...` in declaration order — the second template argument is index `2`, the third is `3`, and so on.
- **The primary type gets dedicated storage; the secondaries share one union.** The top-level payload is itself a union of two things: a `Tp_` member for the primary type, and an `Alternative_<Args...>` member for the secondaries. `Alternative_<Args...>` is built recursively: `Alternative_<First, Rest...>` is a union of a `first_` member (type `First`) and a `rest_` member (type `Alternative_<Rest...>`). So for `Option<T, A, B, C>`, the secondary union is effectively "`A`, or (`B`, or `C`)" nested three unions deep, not a single flat union of `A`, `B`, and `C` side by side.
- **Activating a deep secondary costs one step per level of nesting.** To construct the alternative at nesting depth `k` (0-indexed among the secondaries), the implementation placement-constructs `k` empty wrapper unions on the way down (`std::construct_at` on each intervening `Alternative_<...>`, which is itself a legitimate "live" union object with no initialized member yet) until it reaches the union that actually has the target type as a direct member, and only then constructs the real value there from the supplied arguments. Destruction reverses this: destroy the leaf value, then destroy each wrapper it passed through, in the opposite order, because each wrapper's own lifetime must end explicitly once its member's lifetime has ended. This is why the *n*-th secondary alternative is not "free" relative to the first one — it carries `n` extra union-activation and union-deactivation steps on every construction and destruction, which is exactly the kind of compounding cost that shows up when a project pushes many distinct secondary alternatives onto one `Option` (see [§17](#17-a-nine-alternative-walkthrough) and [§18.9](#189-dont-grow-the-alternative-list-for-just-in-case-extensibility)).
- **Per-alternative operations are collected into dispatch tables, not `if`/`switch` chains.** Copy-construct, move-construct, copy-assign, move-assign, equality, three-way comparison, same-alternative `swap`, and hashing are each implemented once per alternative type and collected into a `static constexpr std::array` of function pointers, indexed by `active_index - 1` (the table has one entry for the primary plus one per secondary). The one exception is destruction: since the primary's destructor is called directly and inline wherever `reset()` happens, the destroy-dispatch table only needs entries for the secondaries, and is indexed by `active_index - 2`.
- **Whole-object alignment and size follow directly from the above.** The object's alignment is the strictest alignment among `T`, every `Args`, and the discriminator type. Its payload size is the larger of `sizeof(T)` and `sizeof(Alternative_<Args...>)`, rounded up to that alignment; the discriminator sits immediately after the payload, and any trailing padding needed to make the whole object a multiple of its alignment comes last. When there are **zero** secondary alternatives, this collapses to exactly the same size and alignment as the header's internal single-type `Optional_<T>` building block — the `None`/multi-alternative machinery costs nothing extra for the common `Option<T>` case beyond what a bespoke hand-rolled optional would already need.

None of the above is part of the stable public contract. It is accurate for the current revision of the header and is written down here because understanding *why* certain operations have the costs and guarantees they do (§16.3, §16.4, §17) requires knowing roughly how the object is laid out. Do not serialize the raw bytes of an `Option`, depend on its exact `sizeof`, or assume this layout survives a header upgrade, a different compiler, or a different standard library.

### 16.2 Triviality and special members

The internal storage layers conditionally preserve trivial destruction, copy construction, move construction, copy assignment, and move assignment when the relevant alternative types permit them.

At the public `Option` level, copy/move operations are constrained by the capabilities of every declared alternative.

### 16.3 Exception safety during state changes

Three places in the implementation go out of their way to order operations so that a throwing user type cannot leave an `Option` in a worse state than it started in, beyond the ordinary "this object's lifetime has ended" guarantee that applies to any C++ type:

- **Bare-value assignment stages a temporary before touching the old value.** When `option = some_value;` resolves to a movable target alternative, the implementation first constructs a temporary of that target type from `some_value` — while whatever alternative was previously active, if any, is still fully alive — and only afterwards resets the old alternative and move-constructs the real storage from the temporary. This matters whenever the assigned expression can read from the `Option`'s *current* value (directly or through aliasing), and it means a throwing converting constructor leaves the original value completely untouched rather than having already destroyed it. (When the target type isn't movable, this staging isn't possible, and the implementation falls back to constructing directly in place — which is also why `replace()` and `operator=` for a non-movable alternative type behave slightly differently from the movable case.)
- **Copy/move assignment reuses live storage instead of always rebuilding it.** If both sides of an assignment already have the *same* alternative active, the value is assigned onto the existing object in place (falling back to destroy-then-reconstruct only when that type isn't copy-/move-assignable). Only when the active alternatives differ, or either side is `None`, does assignment reset the destination and construct the new alternative from scratch. Both the copy and move assignment operators check for self-assignment (`this == std::addressof(other)`) up front and no-op in that case, before any of the above.
- **"Extract by value and empty the source" is conditioned on actually succeeding.** `unwrap() &&`, `expect() &&`, the shared `take_unchecked()` helper they (and `emplace() &&`, `unwrap_or() &&`, `unwrap_or_default() &&`, `unwrap_or_else() &&`) are built on, and the consuming-lambda wrapper behind the rvalue `map`, `and_then`, `map_or`, `map_or_else`, `map_or_transform`, `is_some_and`, and `is_none_or` overloads, all share one small RAII guard. The guard records `std::uncaught_exceptions()` at construction and, in its destructor, resets the `Option` only if that count has **not** increased since — i.e., only if the enclosing expression ran to completion without a new exception propagating through it. If, for example, `Tp_`'s move constructor throws while `unwrap() &&` is building its by-value return, the guard's destructor still runs (during unwinding), observes the higher exception count, and deliberately skips the reset — the `Option` is left exactly as engaged as it was before the call. So "extraction empties the source" is really "extraction empties the source *if it didn't throw while doing so*"; a type whose move constructor can throw should be understood with that caveat.

### 16.4 `match()` dispatch mechanics

- `match()` is one function template written with an explicit object parameter — `template <typename... Rp_, typename Self_, typename... Arms_> constexpr auto match(this Self_&& self, Arms_&&... arms)` — rather than three separately ref-qualified overloads. `std::forward_like<Self_>` is used to recover the right value category (`const&`, `&`, or `&&`) for whichever alternative turns out to be active, so one function body serves all three call forms.
- Finding *which* alternative is active is a compile-time-unrolled, runtime-tested linear scan, not a jump table: a local generic lambda recurses over a compile-time index from `0` up to `sizeof...(Args)`, and at each step compares the live discriminator against `Index + 1` at runtime, stopping at the first match (`std::unreachable()` terminates the recursion if somehow none match, which cannot happen for a well-formed discriminator value). One more alternative in the list is, in the worst case, one more runtime comparison before the active type is identified.
- Once the active type is known, the implementation walks the arm pack in source order. For each arm, whether its pattern *could* possibly apply to the active type is decided entirely at compile time via `if constexpr (pattern_matches_type_<Pattern, CurrentType>())` — an arm that statically cannot apply to the active alternative costs nothing at that call site, not even a skipped runtime branch. Only arms that are statically compatible with the active type get a runtime check (`matches_value_`, which evaluates the guard predicate, the `Some(value)` equality check, or the plain equality comparison, as appropriate). The first such arm whose runtime check returns `true` is invoked, with the forwarded value and the correctly deduced return type.
- If execution runs off the end of the arm list for the engaged path without any runtime match — which can only happen when every statically-applicable arm was a guard and every one of them returned `false` at runtime — the implementation prints `Option<{type}>::match() failed - no matching pattern arm` to `stderr` and calls `std::abort()`. `None` is dispatched completely separately and more simply: the first arm in source order whose pattern is `None` or `Wild` is invoked with no arguments; `None` never goes through `matches_value_`'s runtime guard evaluation at all.
- `Arm_<Pattern, Handler>` is laid out as `[[no_unique_address]] pattern`, then computed mid-padding so the handler starts on its own alignment boundary, then `handler` as the last member — relying on the handler to supply the whole struct's trailing alignment. This is exactly why `static_assert(alignof(handler_type) >= alignof(pattern_type))` exists (see [§6.5](#65-value-patterns-somevalue-and-ordinary-comparison-patterns)): a stricter-aligned pattern would need padding bytes *after* the handler, and a `[[no_unique_address]]` member with nothing following it to overlap into cannot make that padding free, so the combination is refused outright rather than silently costing an extra byte.
- The three `static_assert`s that gate `match()` at compile time (an arm covers `None`; an arm covers the primary type; every alternative, primary and secondary, has at least one potentially-applicable arm) all go through the same `pattern_matches_type_` compile-time predicate used for dispatch. They check whether an arm's *pattern type* is the kind of thing that could ever apply to a given alternative type — never whether a guard's *runtime return value* will actually be `true` for the object in front of it at any particular moment. That gap between "statically could apply" and "actually did apply this time" is precisely what `std::abort()` exists to catch, and precisely why [§15.7](#157-runtime-guards-can-fail-after-static-coverage-succeeds)'s advice to end a guarded match with a true catch-all is not optional once more than one guard is in play for the same alternative.

### 16.5 Diagnostics and type names

Bad-access diagnostics include a type name. The implementation uses C++26 reflection when its feature-test macros are available; otherwise, it parses `__PRETTY_FUNCTION__`.

Concretely, when reflection is unavailable, the type name is recovered from the compiler's own function signature for a small helper template (`get_type_name<Qp_>()`), by finding the substring between the outermost `[` and `]` that GCC/Clang embed in `__PRETTY_FUNCTION__`, locating the literal text `"Qp_ = "` inside it, and then scanning forward character by character — tracking nested `<...>`, `(...)`, and `[...]` depth — until a `,` or `;` appears at depth zero, or an unmatched `]` is found. That scan is necessary, not cosmetic: a naive "stop at the first comma" approach would cut off a type name like `std::pair<int, int>` or `std::array<int, 4>` partway through, because those names contain commas of their own at a nested depth. The same scan is reused unchanged whether the subview helper is `std::string_view::subview` (when available) or `substr`.

This machinery exists purely to make `abort()` diagnostics readable — messages such as `Option<int>::unwrap() failed: Option does not wield any value.`, `Option<int>::unwrap<ReadError>() failed: active alternative does not match.`, or `Option<int>::match() failed - no matching pattern arm`. The exact spelling of a type name in these messages is compiler- and configuration-dependent (reflection vs. `__PRETTY_FUNCTION__`, and even which `__PRETTY_FUNCTION__` format a given compiler emits), so, as [§16.4 of the original notes already warned](#164-match-dispatch-mechanics) in spirit, nothing about the literal text of these messages should be treated as a stable interface. See [§18.8](#188-dont-build-tooling-around-the-exact-wording-of-abort-diagnostics) for the practical version of that warning.

### 16.6 Hashing

`std::hash<rl::Option<T, Ts...>>` is defined only when every alternative is itself hashable via `std::hash`. Its behavior:

- The empty state always hashes to the fixed sentinel `static_cast<std::size_t>(-3333)`, regardless of `T` or `Ts...`.
- When there is at least one secondary alternative, an engaged state's hash is `std::hash<ActiveType>{}(value)` combined with the active index using `value_hash ^ (0x9e3779b9 + index + (value_hash << 6) + (value_hash >> 2))` — the same combine formula popularized by `boost::hash_combine`. This is what keeps, say, `Option<int, long>` holding `5` as an `int` from hashing identically to the same `Option` holding `5` as a `long`, even though `std::hash<int>(5) == std::hash<long>(5)` on common implementations.
- When there are **zero** secondary alternatives, there is nothing to disambiguate against, so the combine step is skipped entirely: an engaged `Option<T>`'s hash is exactly `std::hash<T>` of the contained value, byte-for-byte identical to hashing a bare `T`. This is a simplification specific to the single-alternative case, not a documented cross-version guarantee.

As with any hash function, collisions are possible and expected; `std::hash<rl::Option<...>>`'s only documented obligation is that equal `Option`s hash equal, which the sentinel and the per-alternative mixing above are both designed to uphold.

### 16.7 Header side effects

The header pushes and restores compiler diagnostic state for the compiler families it recognizes (Clang via `#pragma clang diagnostic push`/`pop`, GCC via the `#pragma GCC diagnostic` equivalents), with the final `pop` sitting at the very end of the file, after the closing `#endif` guards. This means the suppressed warnings (`-Wc++98-compat`, `-Wreserved-identifier`, and similar pedantic-compatibility warnings, among others) apply only while the header's own declarations are being parsed — they do not leak into code written after the `#include` line in the including translation unit.

Three function-like macros used internally — `RL_LIFETIME_BOUND`, `RL_SUPPORTS_REFLECTION`, and `RL_IS_SUBVIEW_OR_SUBSTR` — are all explicitly `#undef`'d before the header's closing `#endif`. They do not leak into the including translation unit's macro namespace either, so there is no risk of an including file's own identically-named macro silently colliding with (or being silently overwritten by) this header's internal ones.

The header also makes a global using-declaration for the `operator>>=` used by the pattern-matching DSL. This is intentional for the infix arm syntax, but it is still a global namespace exposure that users should be aware of.

---

## 17. A nine-alternative walkthrough

Every multi-alternative example earlier in this document uses two or three alternatives. This section works through a `rl::Option` with **nine** distinct alternative types — one primary plus eight secondaries — to make the scaling behavior described in [§16.1](#161-storage-model) concrete, and to show that `match()`'s compile-time coverage checking still works exactly the same way once the alternative list gets large.

The motivating shape is a lexer token payload: a single scanned token can carry an integer literal, a floating-point literal, a boolean literal, a string literal, an identifier, an operator, a piece of punctuation, a keyword, or an explicit end-of-file marker — and, separately from all of that, a `Option` built around this type can itself be `None` when nothing has been scanned yet. That last distinction is exactly the kind of thing a `std::variant<...>` alone cannot express, but `rl::Option`'s separate empty state can: "no token yet" (`None`) is different from "a token that happens to be the explicit EOF marker" (an engaged `EofTag` alternative).

```cpp
#include "rl_option.hpp"

#include <print>
#include <string>

enum class Operator { plus, minus, star, slash };
enum class Punct { lparen, rparen, comma, semicolon };
enum class Keyword { kw_if, kw_else, kw_while, kw_return, kw_fn };

struct Identifier {
    std::string name;
    friend auto operator==(const Identifier&, const Identifier&)
        -> bool = default;
};

struct EofTag {
    friend auto operator==(const EofTag&, const EofTag&) -> bool = default;
};

// Primary (1): long long          -- integer literal
// Secondaries (8): double, bool, std::string, Identifier,
//                  Operator, Punct, Keyword, EofTag
using Token = rl::Option<long long, double, bool, std::string, Identifier,
                         Operator, Punct, Keyword, EofTag>;

void describe(const Token& token) {
    token.match(
        rl::None >>= [] {
            std::println("<no token scanned yet>");
        },
        [](const long long&) { return true; } >>= [](long long value) {
            std::println("int literal: {}", value);
        },
        [](const double&) { return true; } >>= [](double value) {
            std::println("float literal: {}", value);
        },
        [](const bool&) { return true; } >>= [](bool value) {
            std::println("bool literal: {}", value);
        },
        [](const std::string&) { return true; } >>= [](const std::string& value) {
            std::println("string literal: \"{}\"", value);
        },
        [](const Identifier&) { return true; } >>= [](const Identifier& id) {
            std::println("identifier: {}", id.name);
        },
        [](const Operator&) { return true; } >>= [](Operator op) {
            std::println("operator #{}", static_cast<int>(op));
        },
        [](const Punct&) { return true; } >>= [](Punct p) {
            std::println("punctuation #{}", static_cast<int>(p));
        },
        [](const Keyword&) { return true; } >>= [](Keyword kw) {
            std::println("keyword #{}", static_cast<int>(kw));
        },
        [](const EofTag&) { return true; } >>= [] {
            std::println("end of file");
        }
    );
}

int main() {
    Token empty(rl::None);
    Token integer(rl::Some<long long>(42));
    Token name(rl::Some(Identifier{"accumulator"}));
    Token eof(rl::Some(EofTag{}));

    describe(empty);
    describe(integer);
    describe(name);
    describe(eof);

    std::println("integer holds long long: {}", integer.is_some<long long>());
    std::println("name holds Identifier: {}", name.is_some<Identifier>());
    std::println("eof holds int: {}", eof.is_some<long long>());
}
```

A few things worth calling out about this specific shape:

- **Ten arms for ten states.** `match()` needs one arm for `None` plus one for each of the nine alternative types; omitting any one of them — including `EofTag`, which is easy to treat as an afterthought — fails the `static_assert` in [§6.4](#64-type-coverage-is-checked-runtime-guard-success-is-not) at compile time, not silently at runtime.
- **The discriminator is still a single byte.** With `N = 9` (one primary plus eight secondaries), the discriminator needs to represent `10` possible values (`0` through `9`), which is still comfortably inside `unsigned char`'s range per [§16.1](#161-storage-model). Growing the alternative list further doesn't widen the discriminator until it would need to represent more than 256 states — an unusual thing to want from a single tagged value in the first place.
- **The recursion depth for the deepest secondary is eight.** `EofTag`, the last secondary in the list, sits at the bottom of eight nested `Alternative_<...>` unions. Constructing an `EofTag` token — which happens every single time the lexer reaches end-of-file — placement-constructs seven empty wrapper unions before reaching the one that actually holds an `EofTag`, then constructs the (stateless) `EofTag` itself; destroying it undoes those same eight steps in reverse. None of that work is visible in the example above, but it is real object code generated once per secondary position, and it compounds with every alternative added after it — see [§18.9](#189-dont-grow-the-alternative-list-for-just-in-case-extensibility) for when that cost is and isn't worth it.
- **`is_some<long long>()` on the `eof` token is simply `false`.** It does not throw, abort, or need a prior check; typed `is_some<Q>()` is always a safe query regardless of which alternative (if any) is actually active, which is exactly why the last line of `main()` above is safe to call even though it is checking for a type that obviously isn't the active one.

This is also a reasonable point at which to reconsider whether `rl::Option` with nine alternatives is the right tool at all, rather than, say, a conventional `enum class TokenKind` plus a `std::variant` or a hand-rolled tagged union designed around the lexer's actual hot path. `rl::Option` adds the separate `None` state and the pattern-matching convenience shown above; it does not remove the structural cost described in §16.1 of having many secondary alternatives. Whether that trade is worthwhile is a question about the specific program, not something this document can answer in the abstract — but §16.1's description of what "one more alternative" actually costs is the information needed to make that call deliberately instead of by accident.

---

## 18. Known pitfalls and anti-patterns

This section collects behaviors that compile, run, and look reasonable, but that rest on an assumption the implementation does not actually honor. Each entry names the assumption, says what actually happens, and points back to the section that establishes it.

### 18.1 Expecting `Option<T>()` to mean "empty"

```cpp
// rl::Option<int> count;  // Does not compile: Option() is deleted.
```
There is no implicit empty state reachable through default construction, an empty brace-initializer, or a default member initializer that omits an argument. Every `Option` must be constructed with `Some`, `None`, or `Default` (§4.1, §15.2). Code that is ported from `std::optional<T> count;` needs to become `rl::Option<int> count(rl::None);`, not a bare declaration.

### 18.2 Treating `Option<T, E>` as `std::expected<T, E>`

```cpp
rl::Option<int, ParseError> parse(std::string_view text);

// Looks like error-propagating chaining. It is not:
auto doubled = parse(text).map([](int v) { return v * 2; });
```
If `parse(text)` actually produced the `ParseError` alternative, `map` silently discards it and returns an empty `Option<int>` — the specific error is gone, not just unobserved (§9.7). The same applies to `and_then`. If the error value needs to survive a transformation, use `match()` (§10.2) or write the transformation to check `is_some<ParseError>()` explicitly before falling back to the primary-only combinators.

### 18.3 Calling `unwrap()`/`value()`/`expect()` as if they were exceptions

```cpp
// This does not "throw a catchable error on empty":
int n = maybe_count.unwrap();
```
A failed checked access prints to `stderr` and calls `std::abort()` (§14.1). There is no library-defined exception type involved, and wrapping the call in `try { ... } catch (...) { ... }` does not protect the program — `std::abort()` is not recoverable by a `catch` block. If a failure needs to be recoverable, check `is_some()` (or the typed `is_some<Q>()`) first, or route the whole decision through `match()`.

### 18.4 Using `unwrap_unchecked()` as a "fast path"

```cpp
// Wrong: no verification that the primary alternative is active.
const int fast = maybe_value.unwrap_unchecked();
```
This is not a cheaper version of `unwrap()` that happens to skip a branch in release builds while still being "basically safe." It is a direct read of the union storage with no discriminator check anywhere, in any build mode (§8.4, §16.1). If the primary alternative is not actually active, this is undefined behavior in the full sense — not "returns a wrong but harmless integer," but a condition the optimizer is entitled to assume cannot happen, with results that can go well beyond the value of that one expression. The only legitimate use is immediately after an `is_some<T>()` (or equivalent) check that the same thread has just performed and nothing has mutated the `Option` since.

### 18.5 Trusting compile-time coverage to mean runtime coverage

```cpp
value.match(
    rl::None >>= [] { /* ... */ },
    rl::Where([](int n) { return n > 0; }) >>= [](int n) { /* ... */ },
    rl::Where([](int n) { return n < 0; }) >>= [](int n) { /* ... */ }
    // No arm for n == 0.
);
```
This compiles: both guards are invocable with `int`, so the primary type is statically "covered" for the purposes of §6.4's three `static_assert`s. It still aborts at runtime the moment `value` holds `0`, because neither guard's *runtime* result is `true` for that input (§6.4, §15.7, §16.4). The fix is not a different pattern syntax — it's making sure the guards are jointly exhaustive, normally by ending with `rl::Wild` or an unconditional handler.

### 18.6 Pairing a value pattern with an ordinarily aligned lambda

```cpp
// Fails a static_assert, not a runtime check:
value.match(
    rl::None >>= [] {},
    rl::Some(0) >>= [] { /* handle zero */ },   // error: alignment mismatch
    rl::Wild >>= [](int n) { /* ... */ }
);
```
`rl::Some(0)` has `int`'s alignment; an ordinary capture-less lambda typically has alignment `1`. `Arm_`'s layout requires the handler to supply the arm's trailing alignment (§6.5, §16.4), so this specific combination is rejected at compile time rather than silently costing an extra padding byte. Reach for `rl::Where([](int n) { return n == 0; })` instead of a raw value pattern whenever the handler is an ordinary lambda — the predicate and the handler then have compatible, usually-1, alignment.

### 18.7 Assuming every `std::move(option).method()` empties `option`

```cpp
std::string s = std::move(opt).value();   // opt is still engaged afterward.
auto fallback = std::move(opt).or_else(supplier); // opt's own state is untouched by this call.
```
Only the extraction-flavored rvalue operations reset the source on success: `unwrap()`, `expect()`, `unwrap<Q>()`, `take()`, `emplace() &&`, the fallback-by-value functions, and the consuming rvalue overloads of `map`/`and_then`/`map_or`/`map_or_else`/`map_or_transform`/`is_some_and`/`is_none_or`. `value() &&` returns `T&&` without resetting anything, and `or_else() &&` moves the receiver *into its return value* without separately clearing the moved-from object (§15.9). Don't assume "I called it on an rvalue" is equivalent to "the source is now empty" — check the specific member in §19 (or re-read §15.9) rather than extrapolating from one operation to another.

### 18.8 Don't build tooling around the exact wording of abort diagnostics

```text
Option<int>::unwrap() failed: Option does not wield any value.
```
The type name embedded in this message comes from either C++26 reflection or a hand-rolled `__PRETTY_FUNCTION__` parser, selected per translation unit based on feature-test macros (§16.5). Two otherwise-identical programs built with different compilers — or the same program built before and after a toolchain upgrade — can legitimately print differently formatted (but behaviorally identical) diagnostics for the exact same failure. Log-scraping or test assertions that pattern-match the literal text of these messages are coupling to an explicitly non-stable implementation detail, not to the library's documented behavior (which is simply: "this call aborts the process").

### 18.9 Don't grow the alternative list for "just in case" extensibility

```cpp
// Every one of these "just in case" alternatives has a real, compounding cost.
using Config = rl::Option<std::string, int, double, bool, std::vector<int>,
                          std::map<std::string, std::string>, /* ...and more */>;
```
Each secondary alternative adds one more level of nested union activation/deactivation on construction and destruction (§16.1), one more entry in every per-alternative dispatch table (copy, move, copy-assign, move-assign, equality, three-way comparison, same-alternative swap, hashing), and one more required arm in every `match()` that needs to stay exhaustive (§6.4). None of this shows up as a correctness problem — the type system and the `static_assert`s in §6.4 will still hold everyone honest — but it is compile-time cost, generated-code size, and `match()` verbosity that accrues whether or not the "just in case" alternative is ever actually constructed at runtime. Prefer the smallest alternative list that reflects types the calling code genuinely needs to distinguish at the type level, and reach for a narrower `Option<T, E>` (or a separate `Option` per concern) over a single `Option` with many secondary alternatives when the alternatives aren't actually mutually exclusive facets of one conceptual value.

### 18.10 Expecting `Default` to produce an empty `Option`

```cpp
rl::Option<int> value(rl::Default); // value.is_some() is true, not false.
```
`rl::Default` always engages an alternative — the primary type if it's default-constructible, otherwise the first default-constructible secondary (§3.3). It never produces `None`. Code migrating from `std::optional<T> value{};` (which is empty) to `rl::Option<T>` should use `rl::Option<T>(rl::None)`, not `rl::Option<T>(rl::Default)`, to preserve "starts empty" semantics.

### 18.11 Expecting `match()`'s non-`void` dispatch to auto-wrap a raw value in `Some`

```cpp
// Does not compile in the supplied header's non-void dispatch path:
// auto result = value.match(
//     rl::None >>= [] { return 0; },
//     rl::Wild >>= [](int n) { return n; }
// );
```
The non-`void` dispatcher deduces an `Option`-shaped final result type and requires every handler to return something convertible to *that* type directly (§6.7). It does not take a raw `int` from one arm and silently wrap it in `Some<int>` for you. Write every non-`void` arm so it returns the already-wrapped final result type explicitly (an `rl::Option<...>` constructed with `Some(...)` or `None`), matching the pattern already shown in §6.7's working example.

---

## 19. Complete API reference

This section lists the public vocabulary and member operations implemented by the supplied header. Constraints are summarized; the compiler's constrained overloads remain the final authority for a particular type.

### 19.1 Names in `namespace rl`

| Name | Kind | Purpose |
|---|---|---|
| `Option<T, Ts...>` | Class template | Main value container |
| `Some<T>` | Class template | Explicit wrapper for an engaged alternative |
| `None` | `inline constexpr` object | Empty-state tag |
| `Default` | `inline constexpr` object | Default-value construction and pattern tag |
| `Wild` | `inline constexpr` object | Catch-all pattern including `None` |
| `SomeAny` | `inline constexpr` object | Catch-all pattern for engaged values |
| `Where(predicate)` | Function template | Named guard wrapper |
| `operator>>=(pattern, handler)` | Imported function template | Builds a pattern-matching arm |

The header also specializes `std::hash` for `Option<T, Ts...>` when every alternative is hashable.

### 19.2 `Some<T>`

| Member | Qualifier | Requirements / result |
|---|---|---|
| `Some()` | — | Explicit; requires `T` default-constructible |
| `Some(const T&)` | — | Explicit; requires `T` copy-constructible |
| `Some(T&&)` | — | Explicit; requires `T` move-constructible |
| `get()` | `const&` | `const T&` |
| `get()` | `&` | `T&` |
| `get()` | `&&` | `T&&` |
| `get()` | `const&&` | `const T&&` |
| `operator==(Some<T>, Some<T>)` | friend | Requires `T` equality-comparable |

The deduction guide lets `Some(value)` deduce `Some<T>` from the argument.

### 19.3 `Option<T, Ts...>`: construction and assignment

| Operation | Requirements / behavior |
|---|---|
| `Option()` | Deleted |
| `Option(Some<T>)` | Constructs the primary alternative |
| `Option(Some<Q>)` | Constructs a declared secondary alternative `Q` |
| `Option(None)` | Constructs the empty state |
| `Option(Default)` | Constructs the primary type if default-constructible; otherwise the first default-constructible secondary |
| `Option(const Option&)` | Requires every alternative copy-constructible |
| `Option(Option&&)` | Requires every alternative move-constructible |
| `operator=(const Option&)` | Requires every alternative copy-constructible and copy-assignable |
| `operator=(Option&&)` | Requires every alternative move-constructible and move-assignable |
| `operator=(Some<T>)` | Assigns/constructs the primary alternative |
| `operator=(Some<Q>)` | Assigns/constructs a declared secondary alternative |
| `operator=(None)` | Resets to empty |
| `operator=(Default)` | Assigns/constructs the selected default alternative |
| `operator=(value)` | Selects one alternative by exact-type match or unique constructibility |

There is no converting constructor from a bare payload value and no general converting constructor between differently-shaped `Option` specializations.

### 19.4 `Option<T, Ts...>`: state and access

| Member | Qualifiers | Scope |
|---|---|---|
| `explicit operator bool()` | `const` | Any engaged alternative |
| `is_some()` | `const` | Any engaged alternative |
| `has_any_engaged_value()` | `const` | Any engaged alternative |
| `is_none()` | `const` | Empty state |
| `is_some<Q>()` | `const` | Named alternative `Q` |
| `is_default()` | `const` | Primary type only; requires default construction and equality |
| `value()` | `const&`, `&`, `&&` | Checked primary access |
| `operator*()` | `const&`, `&`, `&&` | Checked primary access |
| `operator->()` | `const&`, `&` | Checked primary pointer access |
| `unwrap()` | `const&`, `&`, `&&` | Checked primary access; rvalue overload extracts by value and resets |
| `unwrap<Q>()` | `const&`, `&`, `&&` | Checked access to named alternative; rvalue overload extracts by value and resets |
| `expect(message)` | `const&`, `&`, `&&` | Checked primary access with custom diagnostic |
| `unwrap_none()` | `const&`, `&`, `&&` | Checks that the entire `Option` is empty |
| `unwrap_unchecked()` | `const&`, `&`, `&&` | Unchecked primary access by value; invalid state is undefined behavior |

### 19.5 `Option<T, Ts...>`: fallback and functional operations

| Member | Qualifiers | Behavior |
|---|---|---|
| `unwrap_or(value)` | `const&`, `&`, `&&` | Primary value or eager fallback |
| `unwrap_or_default()` | `const&`, `&`, `&&` | Primary value or `T{}` |
| `unwrap_or_else(supplier)` | `const&`, `&`, `&&` | Primary value or lazy supplier result |
| `map(operation)` | `const&`, `&`, `&&` | Transforms primary value to `Option<U>` |
| `and_then(operation)` | `const&`, `&`, `&&` | Calls primary-value operation returning an `Option` |
| `or_else(operation)` | `const&`, `&`, `&&` | Calls fallback only for `None`; preserves all engaged alternatives |
| `map_or(operation, fallback)` | `const&`, `&`, `&&` | Primary transform or eager fallback |
| `map_or_else(operation, supplier)` | `const&`, `&`, `&&` | Primary transform or lazy nullary supplier |
| `map_or_transform(operation, fallback)` | `const&`, `&`, `&&` | Primary transform, optional `None` callback, or fallback |
| `is_some_and(predicate)` | `const&`, `&`, `&&` | Tests the primary alternative; an rvalue receiver with an active primary is consumed and reset after the predicate call, irrespective of the returned boolean |
| `is_none_or(predicate)` | `const&`, `&`, `&&` | True for `None` or a primary value satisfying the predicate; secondary state is false; an rvalue receiver with an active primary is consumed and reset after the predicate call |

### 19.6 `Option<T, Ts...>`: mutation and pointer views

| Member | Qualifiers | Behavior |
|---|---|---|
| `emplace(args...)` | `&` | Destroys current state, constructs primary `T`, returns `T&` |
| `emplace(args...)` | `&&` | Constructs primary `T`, extracts by value, leaves source empty |
| `reset()` | `&` | Destroys active value and sets `None` |
| `take()` | `&` | Moves the whole `Option` out and resets source |
| `replace(value)` | `&` | Stores a deduced alternative and returns the previous `Option` |
| `swap(other)` | member | Exchanges two same-specialization objects |
| `swap(lhs, rhs)` | friend | ADL-enabled swap |
| `as_mut_ptr()` | `&` | `Option<T*>`; rvalue forms deleted |
| `as_mut_ptr<Q>()` | `&` | `Option<Q*>`; requires declared alternative `Q`; rvalue forms deleted |
| `as_immut_ptr()` | `const&` | `Option<const T*>`; rvalue forms deleted |
| `as_immut_ptr<Q>()` | `const&` | `Option<const Q*>`; requires declared alternative `Q`; rvalue forms deleted |

### 19.7 `Option<T, Ts...>`: comparisons

| Expression | Availability |
|---|---|
| `option_a == option_b` | Requires every alternative equality-comparable |
| `option <=> other_option` | Requires every alternative three-way comparable |
| `option == None`, `None == option` | Always available |
| `option <=> None`, `None <=> option` | Always available |
| `option == Some<Q>`, `Some<Q> == option` | Requires `Q` to be a declared, equality-comparable alternative |
| `option == Default`, `Default == option` | Requires the selected default alternative to be default-constructible and equality-comparable |
| `option <=> Default`, `Default <=> option` | Single-type `Option<T>` only; requires default construction and three-way comparison |

---

## 20. Frequently asked questions

### Does `Option<T>` start empty?

No. `Option()` is deleted. Use `Option<T>(None)` for an empty value.

### Does `Option<T>` accept a bare `T` in its constructor?

No. Use `Option<T>(Some(value))`. Bare payload assignment is available after an `Option` has been constructed.

### Is `Option<T, E>` equivalent to `std::expected<T, E>`?

No. It has three possible states: `None`, `T`, and `E`. Its primary-only combinators do not automatically preserve `E`. Use `match()` to inspect the alternatives explicitly.

### Does `map()` preserve secondary alternatives?

No. `map()` operates on the primary type. If a secondary alternative is active, it returns an empty `Option<U>`.

### Does `or_else()` run for a secondary alternative?

No. `or_else()` runs only for `None`. Any active alternative is preserved.

### Does `unwrap<Q>()` support a secondary type?

Yes, when `Q` is one of the declared alternatives. The rvalue overload extracts by value and resets the source after success.

### Does `value() &&` reset the source?

No. It returns `T&&`. Use `std::move(option).unwrap()` or `take()` when extraction should leave the source empty.

### Can an alternative be a reference type?

No. Alternatives must be object types. Use a pointer or `std::reference_wrapper<T>`.

### Can the same type appear more than once?

No. The alternative list must contain distinct types after cv-qualification is removed.

### Does `match()` prove that a runtime predicate always matches?

No. Compile-time checking verifies type applicability. It cannot prove the result of a runtime predicate. If no arm accepts the active value at runtime, the dispatcher aborts.

### Does `match()` wrap raw non-void handler results automatically?

Not in the supplied header. The non-void dispatch path deduces an `Option`-shaped result but expects handlers to return a value convertible to that final result type. Return an explicit `Option<...>` from every non-void arm.

### How many alternatives can an `Option` have?

There's no fixed cap in the API itself — the discriminator trait (§16.1) scales from `unsigned char` up to `std::size_t` as the alternative count grows, and `Alternative_`'s recursive union can nest arbitrarily deep. In practice, the per-alternative cost described in §16.1 and demonstrated in §17 (one more nested union activation, one more dispatch-table entry, one more required `match()` arm per secondary) grows with the list, so the practical limit is whatever amount of generated code and `match()` verbosity a given project is willing to accept, not a hard limit in the header. See §18.9 for why "add it just in case" is a bad default.

### Can I print an `Option` directly?

No `std::formatter` specialization is supplied. Extract or match the value and print that. The reason to not implement `std::formatter` is because it breaks predictability of the returning values. And thus, any handler returned are always wrapped into Option<T> instead of `T`. They shall be unwrapped always. If primitive were unwrapped implicitly but the classes weren't, it would've destroyed the reasoning and immediately create ambiguous returns which must be handled explicitly.

Mind your `unwrap()`. It is there for these reasons.

### Is the type thread-safe?

There is no internal synchronization. Concurrent mutation of the same instance requires external synchronization.

### Can it be used in constant expressions?

The public operations are broadly declared `constexpr`, and valid paths can be evaluated at compile time when the involved alternative and callback operations are also constant-evaluable. A path that reaches diagnostic I/O or `std::abort()` is not a valid constant expression.

---

## 21. License

This project is licensed under the **GNU General Public License, version 3**.

SPDX-License-Identifier: `GPL-3.0`

See the repository's `LICENSE` file for the full license text.
