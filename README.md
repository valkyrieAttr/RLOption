# rl::Option

A header-only, C++23/26 reimplementation of `std::optional<T>` with a much
richer, Rust-flavored surface: explicit `Some` / `None` / `Default`
construction, monadic transforms, exhaustive `match()` pattern matching,
and the ability for a single `Option` to hold one of *several* alternative
types (a lightweight tagged union), not just `T` or nothing.

```cpp
#include "rl_option.hpp"

rl::Option<int> count{rl::Some(3)};
rl::Option<int> empty{rl::None};

int doubled = count.map([](int v) { return v * 2; }).unwrap_or(0); // 6
```

> **Status:** actively hardened. The core storage and assignment paths pass
> `-fsanitize=address,undefined` and compile clean under `-Weverything
> -Werror` on Clang. See [Compiler support](#compiler-support) and
> [Known trade-offs](#known-trade-offs) before adopting this in a hot path
> or a compile-time-sensitive codebase (e.g. a compiler frontend).

## Table of contents

- [Requirements](#requirements)
- [Compiler support](#compiler-support)
- [Installation](#installation)
- [Quick start](#quick-start)
- [Core concepts](#core-concepts)
- [`rl::Some<T>`](#rlsomet)
- [`rl::None`](#rlnone)
- [`rl::Default`](#rldefault)
- [`rl::Option<T, Args...>`](#rloptiont-args)
  - [Construction](#construction)
  - [Checking state](#checking-state)
  - [Reading the value](#reading-the-value-panics-not-exceptions)
  - [Pointer views](#pointer-views)
  - [Transforming](#transforming)
  - [Mutating in place](#mutating-in-place)
  - [Comparison & hashing](#comparison--hashing)
- [Pattern matching with `match()`](#pattern-matching-with-match)
- [Multiple alternatives in depth](#multiple-alternatives-in-depth)
- [Worked example: a toy lexer](#worked-example-a-toy-lexer)
- [Known trade-offs](#known-trade-offs)
- [License](#license)

## Requirements

- C++23 (the header enforces `__cplusplus >= 202302L`), leaning on C++26
  for a few opt-in features.
- `<print>` (`std::print`/`std::println`) is required unconditionally
  today — it's used for panic-style diagnostics in `value()`, `unwrap()`,
  `expect()`, and non-exhaustive `match()` calls.
- Reflection-gated code paths compile only when `__cpp_lib_reflection`
  and `__cpp_impl_reflection` are both defined — currently only available
  via experimental forks (EDG, Bloomberg's `clang-p2996`), e.g. on
  Compiler Explorer.

## Compiler support

| Compiler | Status |
|---|---|
| Clang (recent, with `<print>`) | Primary target; cleanest support. |
| Clang experimental P2996 forks | Needed to exercise reflection-gated code. |
| GCC | Needs GCC 14+ for `<print>`. Also confirm your GCC's `__cplusplus` value for `-std=c++23` actually reports `202302L` — some releases report a provisional value and will trip the version guard even though most language features are present. |
| MSVC | Untested as of this writing. |

## Installation

Single header, no build step:

```cpp
#include "rl_option.hpp"
```

Everything lives under `rl::`: `rl::Option`, `rl::Some`, `rl::None`,
`rl::Default`, `rl::Wild`, `rl::SomeAny`, `rl::Where`.

## Quick start

```cpp
#include "rl_option.hpp"
#include <string>

rl::Option<std::string> find_user(int id) {
  if (id == 1) return rl::Some(std::string{"ada"});
  return rl::None;
}

int main() {
  auto user = find_user(1);

  if (user.is_some()) {
    std::println("found: {}", user.value());
  }

  std::string name = find_user(2).unwrap_or("guest");

  int len = find_user(1)
                .map([](const std::string &s) { return (int)s.size(); })
                .unwrap_or(0);
}
```

## Core concepts

- **No implicit "empty" state.** `rl::Option<T>` has **no default
  constructor** — you must build it from `rl::None`, `rl::Some(value)`,
  `rl::Default`, or a value implicitly convertible into `T`. This differs
  from `std::optional`, which default-constructs to disengaged.
- **Failures panic, they don't throw.** Every "give me the value or else"
  accessor (`value()`, `unwrap()`, `expect()`, an unmatched `match()`)
  prints a diagnostic with `std::println` and calls `std::abort()`. There
  is no `std::bad_optional_access`-style exception to catch — plan your
  error handling around `is_some()`/`unwrap_or*()`/`match()`, not `try`/`catch`.
- **Primary type vs. alternatives.** `Option<T, Args...>` always has one
  *primary* type `T`. The monadic operations (`map`, `and_then`, `map_or`,
  `or_else`) only ever operate on the primary type. `Args...` are
  additional alternatives the `Option` can hold for tagged-union /
  pattern-matching purposes — see [Multiple alternatives](#multiple-alternatives-in-depth).
- **`match()` is the flagship API.** Everything else (`is_some<T>()`,
  `unwrap<T>()`, the pointer views) gives you one fact or one value at a
  time. `match()` is the only tool that dispatches on *which* alternative
  is active and hands you a correctly-typed value for each — see
  [Pattern matching](#pattern-matching-with-match) below.

## `rl::Some<T>`

`Some<T>` is a small wrapper around a `T` value, used both to construct an
`Option` and as an exact-value pattern inside `match()`.

```cpp
rl::Some(5);            // deduces Some<int> (deduction guide)
rl::Some<std::string>("x"); // explicit T, useful when deduction would pick
                              // the wrong type (e.g. const char* vs std::string)
```

- Constructible from a default value (`Some<T>{}`, if `T` is default
  constructible), a `const T&` (copy), or a `T&&` (move) — each
  constructor is `explicit`.
- `.get()` returns the held value, with the usual four ref-qualified
  overloads (`const&`, `&`, `&&`, `const&&`), so it behaves correctly
  whether you're reading from a `Some<T>` lvalue or moving out of a
  temporary.
- `Some<T> == Some<T>` compares the wrapped values directly when `T` is
  `equality_comparable`.
- You rarely construct a *standalone* `Some<T>` for its own sake — its two
  jobs are being handed to an `Option<T, ...>` constructor/assignment, and
  being used as the left-hand side of `>>=` to build a `match()` arm.

## `rl::None`

`None` is an empty tag type with a single global instance, `rl::None`.

- `Option<T>{rl::None}` constructs a disengaged `Option`.
- `rl::None == rl::None` is always `true`.
- `rl::None == Some<T>{...}` and the reverse are always `false` — so you
  can compare it against anything without a type mismatch.
- As a `match()` pattern, `None` matches only a disengaged `Option`, and
  its handler is called with **no arguments**.

## `rl::Default`

`Default` is a pattern/tag meaning "construct (or match) the type's
default value" — it does **not** mean "no value," which is `None`'s job.

- `Option<T>{rl::Default}` value-initializes `T` (`T{}`) when `T` is
  default-constructible.
- If `T` is **not** default-constructible but the `Option` declares a
  default-constructible alternative type among `Args...`, `Default`
  constructs that alternative instead. This is the "fallback alternative"
  mechanism — handy for types like a non-default-constructible handle
  that still wants a sane placeholder state.
- `Default` has an explicit conversion operator to any other
  default-constructible type (excluding `None`/`Default` themselves), so
  it can be used in contexts expecting a concrete default value.
- Equality: `opt == rl::Default` is `true` when the held primary value
  equals `T{}` (requires `T` default-constructible and
  equality-comparable); there's a parallel overload for the
  fallback-alternative case.
- As a `match()` pattern, `Default` matches when the active value compares
  equal to a default-constructed instance of its type.

## `rl::Option<T, Args...>`

### Construction

```cpp
rl::Option<int> a{rl::Some(5)};
rl::Option<int> b{rl::None};
rl::Option<int> c{rl::Default};   // value-initializes int -> 0
rl::Option<int> d{5};             // implicit conversion into Some(5)
// rl::Option<int> e;             // ERROR: no default constructor
```

Copy/move construction and assignment are only available when every one
of `T, Args...` supports the corresponding operation — otherwise they're
explicitly deleted (not just "ill-formed if used"), so the error shows up
at the `Option` declaration/use site rather than deep in a template.

### Checking state

| Call | Meaning |
|---|---|
| `opt.is_some()` | `true` if *any* alternative is active. |
| `opt.is_some<Qp_>()` | `true` only if the *specific* type `Qp_` is the active alternative — the main tool for asking "which one of my alternatives is this?" without extracting it. |
| `opt.is_none()` | `true` if nothing is active. |
| `static_cast<bool>(opt)` | Same as `is_some()`; the conversion is `explicit`, so no accidental `if (opt)` vs `if (opt == 1)` ambiguity. |
| `opt.has_any_engaged_value()` | Alias for `is_some()`. |
| `opt.is_default()` | `true` if the primary value is engaged *and* equals `T{}`. |
| `opt.is_some_and(pred)` | `is_some() && pred(value)`. |
| `opt.is_none_or(pred)` | `is_none() || pred(value)`. |

### Reading the value (panics, not exceptions)

```cpp
opt.value();               // primary type, aborts on empty
opt.unwrap();               // same as value(), Rust-style name
opt.unwrap<AltType>();      // explicit type: aborts if the active
                             //  alternative isn't exactly AltType
opt.unwrap_or(fallback);
opt.unwrap_or_default();
opt.unwrap_or_else(fn);     // fn() or fn(rl::None) supplies the fallback
opt.unwrap_unchecked();     // no check at all — caller guarantees is_some()
opt.expect("message");      // like unwrap(), with your own panic message
opt.unwrap_none();          // asserts the Option *is* None, returns rl::None
*opt;                       // operator*, like std::optional
opt->member;                 // operator->
```

The one-argument-template `unwrap<Qp_>()` is the one to reach for whenever
you have a multi-alternative `Option<T, Args...>` and want *that specific*
alternative rather than just the primary `T` — plain `unwrap()` only ever
looks at the primary type.

Every one of these that can fail (`value`, `unwrap`, `unwrap<Qp_>`,
`expect`) does so by printing a diagnostic and calling `std::abort()` —
there is nothing to `catch`. If you want a non-fatal check first, pair
them with `is_some()`/`is_some<Qp_>()`, or use the `unwrap_or*` family,
or reach for [`match()`](#pattern-matching-with-match) instead.

### Pointer views

```cpp
opt.as_mut_ptr();          // Option<T*>       (None if not holding T)
opt.as_immut_ptr();        // Option<const T*> (None if not holding T)
opt.as_mut_ptr<Alt>();     // Option<Alt*>, typed for a specific alternative
opt.as_immut_ptr<Alt>();   // Option<const Alt*>
```

These are the "soft", non-aborting counterpart to `unwrap<Qp_>()`: instead
of panicking on a type mismatch, you get back `None`. Only available on
lvalues — calling them on a temporary `Option` is a compile error, since
the pointer would dangle immediately.

### Transforming

```cpp
opt.map(fn);               // Option<T> -> Option<U>,       fn(T) -> U
opt.map_or(fn, fallback);   // fn(T) -> U if Some, else fallback
opt.map_or_else(fn, fallback_fn);
opt.map_or_transform(...);
opt.and_then(fn);           // fn(T) -> Option<U>, flattens
opt.or_else(fn);             // fn() -> Option<T>, used when None
```

All of these operate on the **primary** type only. If you need to
transform based on which alternative is active, use `match()`.

### Mutating in place

```cpp
opt.emplace(args...);    // destroy current value, construct T(args...) in place
opt.reset();              // become None
opt.take();                // move the whole Option out, leave None behind
opt.replace(new_value);    // install new_value, return the *previous* Option
swap(a, b);                 // or a.swap(b)
```

`take()`/`replace()` return a whole `Option<T, Args...>` (not just `T`),
since the Option might currently be holding any of its alternatives — and
both are only available on lvalues (calling them on a temporary is a
compile error by design, not an oversight).

### Comparison & hashing

- `==` / `<=>` are defined against `rl::None`, `rl::Default`,
  `rl::Some<Q>`, and other `Option`s of the same type set, dispatching to
  whichever alternative is currently active.
- `std::hash<rl::Option<T>>` is specialized whenever `T` is hashable and
  there are no extra alternatives (`Option<T, Args...>` with
  `sizeof...(Args) == 0`).

## Pattern matching with `match()`

`match()` is where this library earns its keep over `std::optional` — it's
not a convenience wrapper around `if (is_some())`, it's a real pattern
match with first-match-wins dispatch, flexible handlers, and several
distinct kinds of pattern. This section documents the *whole* thing: every
pattern kind, exactly how a match is resolved at runtime, and the one
genuinely sharp edge in the current design.

### How a match is actually resolved

```cpp
int result = opt.match(
    rl::Some(0)  >>= [] { return -1; },              // exact value
    rl::SomeAny  >>= [](auto &v) { return v * 2; },   // any engaged value, gets it
    rl::None     >>= [] { return 0; }                  // disengaged
);
```

Each `pattern >>= handler` expression (`operator>>=`) produces an *arm*.
`match()` builds the arm list once, then does exactly this, in order,
every time it's called:

1. Walk the arms **in the order you wrote them**. For each one, ask "does
   this arm's pattern accept the `Option`'s *current* active type and
   value?" The first arm that says yes wins — its handler is called with
   the matched value (or no value — see arity below) and its result is
   returned immediately. Nothing after it is even inspected.
2. If **no** arm accepted, *then* it decides how to fail. It checks two
   independent facts about your arm list: does *any* arm declare itself as
   covering the `None` case (only `None` and `Wild` are recognized for
   this), and does *any* arm declare itself as covering the `Some` case
   (only `Wild`, `SomeAny`, `Some(...)`, and `Default` are recognized for
   this — see the sharp edge below). If either is missing, it prints
   exactly which one ("`None`", "`Some(_)`", or both) and calls
   `std::abort()`. If both are nominally present but nothing still
   matched (e.g. you have `Some(5)` and `Some(10)` arms and the value is
   `7`), it prints a generic "no matching pattern arm" instead.

The important consequence: **step 1 is what actually determines success
or failure.** Step 2 only runs when step 1 already failed, and only
decides *which error message* to print. This matters for the sharp edge
below.

### The full pattern vocabulary

| Pattern | Matches when... | Handler receives | Counts as "covers None"? | Counts as "covers Some"? |
|---|---|---|---|---|
| `rl::None` | `Option` is disengaged. | nothing — must be callable with no arguments. | ✅ | ❌ |
| `rl::Wild` | **anything** — `None` or any `Some`. Universal catch-all. | nothing — must be callable with no arguments (it has to work for the `None` case too). | ✅ | ✅ |
| `rl::SomeAny` | any engaged value, of any alternative type. | the matched value, forwarded as-is (`auto&`/`auto&&` lambda). | ❌ | ✅ |
| `rl::Some(value)` | the primary type is active **and** equals `value` (`operator==`). | nothing, or the matched value — both are accepted. | ❌ | ✅ |
| `rl::Some<AltType>(value)` | the alternative `AltType` is active **and** equals `value`. | nothing, or the matched value. | ❌ | ✅ |
| `rl::Default` | the active value equals a default-constructed instance of its type. | nothing, or the matched value. | ❌ | ✅ |
| `rl::Where(pred)` | the active alternative's type matches `pred`'s parameter type **and** `pred(value)` returns `true`. | the matched value. | ❌ | ❌ (see below) |
| a raw predicate, e.g. `[](const Alt& v){ return cond; }` | identical behavior to `Where(...)` — `Where` is just the named, self-documenting spelling of the same mechanism. | the matched value. | ❌ | ❌ (see below) |

`Where(pred)` is a thin wrapper (`rl::details::Where_<Pred>`) that marks a
callable as a deliberate guard pattern rather than relying on it being
*implicitly* treated as one. It behaves exactly like passing the bare
lambda — use whichever reads better at the call site; `Where(...)` is
kept as backward compatibility when you want the arm list to visually signal "this is a
guard" at a glance, especially next to `Some(...)`/`SomeAny` arms.

### Handler arity is flexible

For any value-bearing pattern (`Some(...)`, `SomeAny`, `Where(...)`/raw
predicates), the handler can:

```cpp
rl::Some(0) >>= [](int v)      { return v; };     // takes the value
rl::Some(0) >>= []             { return -1; };     // takes nothing
rl::Some(0) >>= -1;                                  // not even a lambda —
                                                      // a literal return value
```

`None` and `Wild` are the exception: their handlers **must** be nullary,
since `Wild` has to work uniformly for the `None` case too.

### The sharp edge: `Where`/predicate arms don't move the "covers Some" needle

This is the one piece of behavior that's easy to get wrong, so it's worth
stating plainly: **the "covers None" / "covers Some" bookkeeping only
recognizes a pattern's *kind*, not what it actually matches.** A `Where`
arm (or an equivalent raw-lambda arm) is never counted as covering `Some`,
*even if its predicate is unconditionally `true` and genuinely covers
every value of its type*.

The good news, from how resolution actually works (see above): this
**does not** make a logically-complete set of `Where(...)` arms unsafe. If
your predicates jointly cover every alternative your `Option` can hold,
step 1 will always find a match, and step 2 (where the bookkeeping lives)
never runs at all. What it *does* affect is diagnostic quality: if your
coverage has a real gap and that gap is ever hit, and the only arms you
had were `Where`/predicate arms, `match()` reports the generic "no
matching pattern arm" (or, if you also skipped a `None` arm, "missing arm
for: `None`") rather than anything more specific — because as far as the
bookkeeping is concerned, nothing ever claimed to cover `Some` at all.

Practical takeaway: it's fine, and shown below, to build your "one arm
per alternative type" dispatch entirely out of `Where(...)` arms — just
make sure they're jointly exhaustive over your actual alternative set
(one guard per type in `Option<T, Args...>`), and keep a `None` arm (or
`Wild`) in the list so the `None` side of the bookkeeping is satisfied
too.

### Ordering matters

Arms are tried top to bottom, and the **first** accepting arm wins — put
more specific patterns before broader ones (`Some(specific_value)` before
`SomeAny`, a `Where(...)` guard for one type before a trailing `SomeAny`
catch-all for "everything else").

## Multiple alternatives in depth

`Option<T, Args...>` can hold `T` or any of `Args...`, with exactly one
active at a time — closer to a tagged union than a plain optional. The
pieces above compose as the way to work with it:

```cpp
rl::Option<int, std::string> value{rl::Some<std::string>("fallback")};

value.is_some<std::string>();   // true
value.is_some<int>();           // false

value.match(
    rl::Some(0)   >>= [] { return "zero"; },
    rl::Where([](const std::string &) { return true; }) >>=
        [](const std::string &s) { return s.c_str(); },
    rl::SomeAny    >>= [](auto &) { return "other"; },
    rl::None        >>= [] { return "none"; }
);
```

If `T` isn't default-constructible but one of `Args...` is, `rl::Default`
transparently falls back to constructing that alternative — see
[`rl::Default`](#rldefault).

For `N` alternatives, the natural style is **one `Where(...)` arm per
alternative type**, each written exactly once regardless of how many
alternatives `N` is, plus a trailing `None` arm. The worked example below
builds a small lexer around exactly this shape with `N = 3`.

## Worked example: a toy lexer

This is the shape this library was built for: a result type that can be
"one of several token kinds, or nothing left to read," matched exhaustively
at every call site. Three alternative types plus `None` for end-of-input —
`N = 3`, same pattern scales to however many token kinds a real lexer needs.

```cpp
#include "rl_option.hpp"
#include <cctype>
#include <print>
#include <string>
#include <string_view>

struct Identifier { std::string name; };
struct NumberLit   { double value; };
struct Symbol       { char ch; };

using Token = rl::Option<Identifier, NumberLit, Symbol>;

Token next_token(std::string_view &src) {
  while (!src.empty() &&
         std::isspace(static_cast<unsigned char>(src.front()))) {
    src.remove_prefix(1);
  }

  if (src.empty()) {
    return rl::None; // end of input
  }

  const char c = src.front();

  if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
    std::size_t len = 0;
    while (len < src.size() && (std::isalnum(static_cast<unsigned char>(
                                     src[len])) ||
                                 src[len] == '_')) {
      ++len;
    }
    std::string name{src.substr(0, len)};
    src.remove_prefix(len);
    return rl::Some(Identifier{std::move(name)});
  }

  if (std::isdigit(static_cast<unsigned char>(c))) {
    std::size_t len = 0;
    while (len < src.size() &&
           std::isdigit(static_cast<unsigned char>(src[len]))) {
      ++len;
    }
    double value = std::stod(std::string{src.substr(0, len)});
    src.remove_prefix(len);
    return rl::Some(NumberLit{value});
  }

  src.remove_prefix(1);
  return rl::Some(Symbol{c});
}

int main() {
  std::string_view source = "x1 + 42 y";

  for (;;) {
    Token tok = next_token(source);

    const bool keep_going = tok.match(
        // exact-value match on one specific alternative
        rl::Some<Symbol>(Symbol{'+'}) >>= [] {
          std::println("plus");
          return true;
        },

        // one Where(...) guard per remaining alternative type — this is
        // the "N alternatives" idiom: it reads the same whether N is 3
        // or 30, and each guard only ever fires for its own type.
        [](const Identifier &) { return true; } >>=
            [](const Identifier &id) {
              std::println("identifier: {}", id.name);
              return true;
            },

        [](const NumberLit &) { return true; } >>=
            [](const NumberLit &num) {
              std::println("number: {}", num.value);
              return true;
            },

        // catches any Symbol other than '+' that slipped past the
        // exact-value arm above — placed after it on purpose, since
        // SomeAny would otherwise swallow the '+' case first.
        rl::SomeAny >>= [](auto &&) {
          std::println("other symbol");
          return true;
        },

        rl::None >>= [] {
          std::println("<eof>");
          return false;
        });

    if (!keep_going) {
      break;
    }
  }
}
```

Running this over `"x1 + 42 y"` prints, in order: `identifier: x1`,
`plus`, `number: 42`, `identifier: y`, `<eof>`. Every arm but the last
reports `true` to keep the loop going; `None` reports `false` to stop it —
a simple example of letting `match()`'s return value drive control flow
instead of branching on `is_some()` yourself.

This example hasn't been run through a conformant C++26 toolchain as part
of writing this README (see [Compiler support](#compiler-support)) — treat
it as a design reference and verify it against your own build before
relying on it.

## Known trade-offs

- **Compile time and binary size scale with the number of distinct
  `Option<T, Args...>` instantiations.** Each one independently
  re-instantiates the storage/union machinery and re-evaluates every
  `requires(...)` clause at each call site. Heavy use across many types
  (e.g. as the result type threaded through a compiler's lexer, parser,
  and semantic analysis) measurably slows builds.
- **Some runtime overhead versus `std::variant` + `std::visit`**, due to
  the extra bookkeeping (a separate tag member, additional indirection in
  the type-check/dispatch helpers) needed to support multiple
  alternatives, pattern matching, and the default-alternative fallback.
- This library is aimed at ergonomics and expressive pattern matching, not
  at being a minimal-overhead drop-in for compile-time- or
  performance-critical code. If that's your use case, prefer
  `std::optional`/`std::variant` directly.

## License
rl-option is licensed under the GNU General Public License v3.0
See LICENSE for further details.
