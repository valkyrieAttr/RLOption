// stress_constexpr_battery.cpp — adversarial constexpr battery for RLOption.hpp
// Every test is a static_assert. Runtime section validates non-constexpr paths.
#include "rl_option.hpp"
#include <cassert>
#include <type_traits>
#include <utility>
#include <cstdio>

// ============================================================
// HELPERS
// ============================================================
struct MoveOnly {
    int v;
    constexpr explicit MoveOnly(int x) : v(x) {}
    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    constexpr MoveOnly(MoveOnly&&) noexcept            = default;
    constexpr MoveOnly& operator=(MoveOnly&&) noexcept = default;
    constexpr bool operator==(const MoveOnly& o) const { return v == o.v; }
};

struct Empty {};
constexpr bool operator==(Empty, Empty) { return true; }

struct Tracker {
    int copies = 0, moves = 0, val = 0;
    constexpr explicit Tracker(int v) : val(v) {}
    constexpr Tracker(const Tracker& o) noexcept
        : copies(o.copies + 1), moves(o.moves), val(o.val) {}
    constexpr Tracker(Tracker&& o) noexcept
        : copies(o.copies), moves(o.moves + 1), val(o.val) {}
    constexpr Tracker& operator=(const Tracker& o) noexcept {
        copies = o.copies + 1; moves = o.moves; val = o.val; return *this;
    }
    constexpr Tracker& operator=(Tracker&& o) noexcept {
        copies = o.copies; moves = o.moves + 1; val = o.val; return *this;
    }
};

// ============================================================
// WAVE 1 — CONSTRUCTION & IS_SOME / IS_NONE
// ============================================================
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(42);
    rl::Option<int> b = rl::None;
    return a.is_some() && !a.is_none() && b.is_none() && !b.is_some();
}(), "W1.1: basic is_some/is_none");

static_assert([]() consteval {
    rl::Option<int, double> a = rl::Some<int>(1);
    rl::Option<int, double> b = rl::Some<double>(2.0);
    rl::Option<int, double> c = rl::None;
    return a.is_some() && b.is_some() && c.is_none();
}(), "W1.2: multi-alt is_some/is_none");

static_assert([]() consteval {
    rl::Option<int, double> a = rl::Some<int>(1);
    rl::Option<int, double> b = rl::Some<double>(2.0);
    return a.is_some<int>() && !a.is_some<double>()
        && b.is_some<double>() && !b.is_some<int>();
}(), "W1.3: typed is_some<T>");

static_assert([]() consteval {
    rl::Option<Empty> a = rl::Some<Empty>(Empty{});
    rl::Option<Empty> b = rl::None;
    return a.is_some() && b.is_none();
}(), "W1.4: Empty type construction");

static_assert([]() consteval {
    // copy construction
    rl::Option<int> a = rl::Some<int>(7);
    rl::Option<int> b = a;          // copy
    return a.is_some() && b.is_some() && b.unwrap() == 7;
}(), "W1.5: copy construction");

static_assert([]() consteval {
    // move construction
    rl::Option<int> a = rl::Some<int>(7);
    rl::Option<int> b = std::move(a);
    return b.is_some() && b.unwrap() == 7;
}(), "W1.6: move construction");

static_assert([]() consteval {
    rl::Option<int, double, bool> tri = rl::Some<bool>(true);
    return tri.is_some<bool>() && !tri.is_some<int>() && !tri.is_some<double>();
}(), "W1.7: 3-alt typed is_some<T>");

// ============================================================
// WAVE 2 — UNWRAP VARIANTS (constexpr paths)
// ============================================================
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(99);
    return a.unwrap() == 99;
}(), "W2.1: unwrap const&");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(7);
    return std::move(a).unwrap() == 7;
}(), "W2.2: unwrap &&");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(5);
    return a.value() == 5;
}(), "W2.3: value() same as unwrap()");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.unwrap_or(42) == 42;
}(), "W2.4: unwrap_or on None");

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(10);
    return some.unwrap_or(42) == 10;
}(), "W2.5: unwrap_or on Some");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.unwrap_or_else([] { return 99; }) == 99;
}(), "W2.6: unwrap_or_else() niladic on None");

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(3);
    return some.unwrap_or_else([] { return 99; }) == 3;
}(), "W2.7: unwrap_or_else() on Some → value");

static_assert([]() consteval {
    // None-taking callback form
    rl::Option<int> none = rl::None;
    return none.unwrap_or_else([](rl::None_t) { return 55; }) == 55;
}(), "W2.8: unwrap_or_else(None_t) form");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.unwrap_or_default() == 0;
}(), "W2.9: unwrap_or_default on None");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(17);
    return opt.unwrap_or_default() == 17;
}(), "W2.10: unwrap_or_default on Some");

static_assert([]() consteval {
    // const& unwrap_or_default preserves value category
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.unwrap_or_default() == 5;
}(), "W2.11: const& unwrap_or_default");

static_assert([]() consteval {
    // rvalue unwrap_or_default moves out
    rl::Option<MoveOnly> opt = rl::Some<MoveOnly>(MoveOnly{42});
    MoveOnly val = std::move(opt).unwrap_or(MoveOnly{0});
    return val.v == 42;
}(), "W2.12: move-only unwrap_or &&");

// multi-alt typed unwrap
static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(3.14);
    return opt.unwrap<double>() == 3.14;
}(), "W2.13: multi-alt unwrap<T> correct type");

// unwrap_unchecked (no panic path, just happy path constexpr)
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(123);
    return opt.unwrap_unchecked() == 123;
}(), "W2.14: unwrap_unchecked on Some");

// ============================================================
// WAVE 3 — MAP
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    auto r = opt.map([](int x) { return x * 2; });
    return r.is_some() && r.unwrap() == 10;
}(), "W3.1: map on Some");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    auto r = opt.map([](int x) { return x * 2; });
    return r.is_none();
}(), "W3.2: map on None → None");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    auto r = opt.map([](int x) -> double { return x * 1.5; });
    return r.is_some() && r.unwrap() == 4.5;
}(), "W3.3: map int→double changes value type");

static_assert([]() consteval {
    // map on rvalue moves the value out
    rl::Option<MoveOnly> opt = rl::Some<MoveOnly>(MoveOnly{7});
    auto r = std::move(opt).map([](MoveOnly m) { return MoveOnly{m.v * 2}; });
    return r.is_some() && std::move(r).unwrap().v == 14;
}(), "W3.4: map on MoveOnly rvalue");

static_assert([]() consteval {
    // targeted map<int> on correct active type
    rl::Option<int, double> opt = rl::Some<int>(4);
    auto r = opt.map<int>([](int x) { return x + 1; });
    return r.is_some() && r.unwrap() == 5;
}(), "W3.5: map<Target> on matching type");

static_assert([]() consteval {
    // targeted map<int> on wrong active type → None
    rl::Option<int, double> opt = rl::Some<double>(2.0);
    auto r = opt.map<int>([](int x) { return x + 1; });
    return r.is_none();
}(), "W3.6: map<Target> on non-matching type → None");

static_assert([]() consteval {
    // map on None multi-alt → None
    rl::Option<int, double> opt = rl::None;
    auto r = opt.map<int>([](int x) { return x + 1; });
    return r.is_none();
}(), "W3.7: map<Target> on None → None");

static_assert([]() consteval {
    // map returns new Option wrapping mapped value
    rl::Option<int> opt = rl::Some<int>(10);
    rl::Option<int> r = opt.map([](int x) { return x + 5; });
    return r.unwrap() == 15;
}(), "W3.8: map result is Option<R>");

// ============================================================
// WAVE 4 — MAP_OR / MAP_OR_ELSE / MAP_OR_TRANSFORM
// ============================================================
static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(3);
    return some.map_or([](int x) { return x * 10; }, 0) == 30;
}(), "W4.1: map_or on Some");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.map_or([](int x) { return x * 10; }, -1) == -1;
}(), "W4.2: map_or on None uses fallback");

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(5);
    return some.map_or_else([](int x) { return x + 1; }, [] { return 0; }) == 6;
}(), "W4.3: map_or_else on Some");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.map_or_else([](int x) { return x + 1; }, [] { return 99; }) == 99;
}(), "W4.4: map_or_else on None uses fallback_fn");

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(2);
    return some.map_or_transform([](int x) { return x * 3; }, 0) == 6;
}(), "W4.5: map_or_transform on Some");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.map_or_transform([](int x) { return x * 3; }, 42) == 42;
}(), "W4.6: map_or_transform on None");

// targeted map_or with specific type
static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<int>(4);
    return opt.map_or<int>([](int x) { return x * 2; }, 0) == 8;
}(), "W4.7: targeted map_or<int> on correct type");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(3.5);
    return opt.map_or<int>([](int x) { return x * 2; }, -99) == -99;
}(), "W4.8: targeted map_or<int> on wrong type → fallback");

// ============================================================
// WAVE 5 — AND_THEN / OR_ELSE
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(4);
    auto r = opt.and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x * x); });
    return r.is_some() && r.unwrap() == 16;
}(), "W5.1: and_then on Some → Some");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    auto r = opt.and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x * x); });
    return r.is_none();
}(), "W5.2: and_then on None → None");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    auto r = opt.and_then([](int x) -> rl::Option<double> {
        return rl::Some<double>(x * 1.5);
    });
    return r.is_some() && r.unwrap() == 4.5;
}(), "W5.3: and_then int→double Option");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    // callback returns None — should propagate
    auto r = opt.and_then([](int) -> rl::Option<int> { return rl::None; });
    return r.is_none();
}(), "W5.4: and_then callback returns None");

static_assert([]() consteval {
    // targeted and_then<int> on correct type
    rl::Option<int, double> opt = rl::Some<int>(3);
    auto r = opt.and_then<int>([](int x) -> rl::Option<int> {
        return rl::Some<int>(x + 10);
    });
    return r.is_some() && r.unwrap() == 13;
}(), "W5.5: targeted and_then on correct type");

static_assert([]() consteval {
    // targeted and_then<int> on wrong active type → None
    rl::Option<int, double> opt = rl::Some<double>(9.0);
    auto r = opt.and_then<int>([](int x) -> rl::Option<int> {
        return rl::Some<int>(x + 10);
    });
    return r.is_none();
}(), "W5.6: targeted and_then on wrong type → None");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    auto r = none.or_else([] { return rl::Option<int>(rl::Some<int>(99)); });
    return r.is_some() && r.unwrap() == 99;
}(), "W5.7: or_else on None recovers");

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(7);
    auto r = some.or_else([] { return rl::Option<int>(rl::Some<int>(99)); });
    return r.is_some() && r.unwrap() == 7;
}(), "W5.8: or_else on Some returns original");

static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    auto r = none.or_else([] { return rl::Option<int>(rl::None); });
    return r.is_none();
}(), "W5.9: or_else on None, callback also None");

// or_else with None-taking callback
static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    auto r = none.or_else([](rl::None_t) { return rl::Option<int>(rl::Some<int>(42)); });
    return r.is_some() && r.unwrap() == 42;
}(), "W5.10: or_else with None_t callback");

// ============================================================
// WAVE 6 — PREDICATES
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    return opt.is_some_and([](int x) { return x > 3; });
}(), "W6.1: is_some_and true");

static_assert([]() consteval {
    return !rl::Option<int>(rl::Some<int>(1)).is_some_and([](int x) { return x > 3; });
}(), "W6.2: is_some_and false (pred fails)");

static_assert([]() consteval {
    return !rl::Option<int>(rl::None).is_some_and([](int x) { return x > 3; });
}(), "W6.3: is_some_and false (None)");

static_assert([]() consteval {
    return rl::Option<int>(rl::None).is_none_or([](int x) { return x > 100; });
}(), "W6.4: is_none_or on None → true");

static_assert([]() consteval {
    return rl::Option<int>(rl::Some<int>(5)).is_none_or([](int x) { return x > 3; });
}(), "W6.5: is_none_or on Some, pred true");

static_assert([]() consteval {
    return !rl::Option<int>(rl::Some<int>(1)).is_none_or([](int x) { return x > 3; });
}(), "W6.6: is_none_or on Some, pred false");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<int>(5);
    return opt.is_some_and<int>([](int x) { return x > 3; });
}(), "W6.7: targeted is_some_and<T> correct type");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(5.0);
    return !opt.is_some_and<int>([](int x) { return x > 3; });
}(), "W6.8: targeted is_some_and<T> wrong type → false");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(5.0);
    return opt.is_none_or<int>([](int x) { return x > 3; });
}(), "W6.9: targeted is_none_or<T> wrong type → true (treats as none-or)");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::None;
    return opt.is_none_or<int>([](int x) { return x > 3; });
}(), "W6.10: targeted is_none_or<T> on None → true");

// ============================================================
// WAVE 7 — IS_DEFAULT
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(0);
    return opt.is_default();
}(), "W7.1: is_default true (zero == int{})");

static_assert([]() consteval {
    return !rl::Option<int>(rl::Some<int>(1)).is_default();
}(), "W7.2: is_default false (non-zero)");

static_assert([]() consteval {
    return !rl::Option<int>(rl::None).is_default();
}(), "W7.3: is_default false on None");

static_assert([]() consteval {
    rl::Option<double> opt = rl::Some<double>(0.0);
    return opt.is_default();
}(), "W7.4: is_default true for double{}");

// ============================================================
// WAVE 8 — MATCH
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(42);
    return opt.match(
        rl::SomeAny >>= [](int x) { return x; },
        rl::None    >>= []()      { return -1; }
    ).unwrap() == 42;
}(), "W8.1: match SomeAny on Some");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.match(
        rl::SomeAny >>= [](int x) { return x; },
        rl::None    >>= []()      { return -1; }
    ).unwrap() == -1;
}(), "W8.2: match None arm on None");

static_assert([]() consteval {
    // typed arm via Some<int>(val) — matches if int AND value == val
    rl::Option<int, double> opt = rl::Some<int>(10);
    return opt.match(
        rl::Some<int>(10)    >>= [](int x)    { return x * 2; },
        rl::SomeAny          >>= [](auto&&)   { return -1; },
        rl::None             >>= []()         { return 0; }
    ).unwrap() == 20;
}(), "W8.3: typed value arm matches exact value");

static_assert([]() consteval {
    // SomeAny falls through when typed arm doesn't match value
    rl::Option<int, double> opt = rl::Some<int>(99);
    return opt.match(
        rl::Some<int>(10)    >>= [](int)    { return 1; },
        rl::SomeAny          >>= [](auto&&) { return 2; },
        rl::None             >>= []()       { return 0; }
    ).unwrap() == 2;
}(), "W8.4: SomeAny catches non-matching value arm");

static_assert([]() consteval {
    // match returns Option — unwrap it
    rl::Option<int> opt = rl::Some<int>(5);
    int r = opt.match(
        rl::SomeAny >>= [](int x) -> int { return x + 100; },
        rl::None    >>= []()      -> int { return 0; }
    ).unwrap();
    return r == 105;
}(), "W8.5: match return type wrapping");

static_assert([]() consteval {
    // Wild arm matches everything including None
    rl::Option<int> opt = rl::None;
    return opt.match(
        rl::Wild >>= []() { return 77; }
    ).unwrap() == 77;
}(), "W8.6: Wild arm on None");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    return opt.match(
        rl::Wild >>= []() { return 77; }
    ).unwrap() == 77;
}(), "W8.7: Wild arm on Some");

// ============================================================
// WAVE 9 — VALUE CATEGORY PROPAGATION
// ============================================================
static_assert([]() consteval {
    // const& propagation through map
    const rl::Option<int> opt = rl::Some<int>(5);
    auto r = opt.map([](int x) { return x + 1; }); // should copy, not move
    return r.unwrap() == 6;
}(), "W9.1: const& Option map copies");

static_assert([]() consteval {
    // rvalue propagation moves into map
    rl::Option<Tracker> opt = rl::Some<Tracker>(Tracker{7});
    auto r = std::move(opt).map([](Tracker t) -> int { return t.val; });
    return r.unwrap() == 7;
}(), "W9.2: && Option map moves Tracker");

static_assert([]() consteval {
    // rvalue and_then moves
    rl::Option<Tracker> opt = rl::Some<Tracker>(Tracker{3});
    auto r = std::move(opt).and_then([](Tracker t) -> rl::Option<int> {
        return rl::Some<int>(t.val * 10);
    });
    return r.unwrap() == 30;
}(), "W9.3: && Option and_then moves");

static_assert([]() consteval {
    // const& unwrap_or makes a copy
    const rl::Option<int> opt = rl::Some<int>(8);
    return opt.unwrap_or(0) == 8;
}(), "W9.4: const& unwrap_or copies");

static_assert([]() consteval {
    // move-only type: unwrap_or on Some moves out
    rl::Option<MoveOnly> opt = rl::Some<MoveOnly>(MoveOnly{55});
    MoveOnly val = std::move(opt).unwrap_or(MoveOnly{0});
    return val.v == 55;
}(), "W9.5: MoveOnly unwrap_or on Some");

static_assert([]() consteval {
    // move-only: unwrap_or on None uses fallback
    rl::Option<MoveOnly> opt = rl::None;
    MoveOnly val = std::move(opt).unwrap_or(MoveOnly{99});
    return val.v == 99;
}(), "W9.6: MoveOnly unwrap_or on None");

// ============================================================
// WAVE 10 — CHAINING
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    int r = opt
        .map([](int x) { return x + 1; })
        .and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x * 2); })
        .map_or([](int x) { return x + 10; }, 0);
    return r == 18;
}(), "W10.1: map → and_then → map_or chain");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    int r = opt
        .map([](int x) { return x + 1; })
        .and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x * 2); })
        .unwrap_or(0);
    return r == 0;
}(), "W10.2: None propagates through chain");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(2);
    int r = opt
        .and_then([](int) -> rl::Option<int> { return rl::None; })
        .or_else([] { return rl::Option<int>(rl::Some<int>(99)); })
        .unwrap();
    return r == 99;
}(), "W10.3: and_then→None→or_else recovery");

static_assert([]() consteval {
    bool r = rl::Option<int>(rl::Some<int>(5))
        .map([](int x) { return x * 2; })
        .is_some_and([](int x) { return x > 5; });
    return r;
}(), "W10.4: map → is_some_and chain");

static_assert([]() consteval {
    // deep chain: map→map→and_then→or_else→map
    rl::Option<int> opt = rl::Some<int>(1);
    int r = opt
        .map([](int x) { return x + 1; })       // Some(2)
        .map([](int x) { return x * 3; })       // Some(6)
        .and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x - 1); }) // Some(5)
        .or_else([] { return rl::Option<int>(rl::Some<int>(0)); }) // Some(5)
        .map([](int x) { return x * x; })       // Some(25)
        .unwrap_or(0);
    return r == 25;
}(), "W10.5: deep chaining map×2 and_then or_else map");

static_assert([]() consteval {
    // safe_add pattern
    auto safe_add = [](rl::Option<int> a, rl::Option<int> b) -> rl::Option<int> {
        return a.and_then([&](int av) -> rl::Option<int> {
            return b.map([av](int bv) { return av + bv; });
        });
    };
    auto r1 = safe_add(rl::Option<int>(rl::Some<int>(3)), rl::Option<int>(rl::Some<int>(4)));
    auto r2 = safe_add(rl::Option<int>(rl::Some<int>(3)), rl::Option<int>(rl::None));
    auto r3 = safe_add(rl::Option<int>(rl::None), rl::Option<int>(rl::Some<int>(4)));
    return r1.unwrap() == 7 && r2.is_none() && r3.is_none();
}(), "W10.6: safe_add via and_then+map");

static_assert([]() consteval {
    // divide pattern
    auto divide = [](int a, int b) -> rl::Option<int> {
        if (b == 0) return rl::None;
        return rl::Some<int>(a / b);
    };
    int r1 = divide(10, 2).map([](int x) { return x + 1; }).unwrap_or(0);
    int r2 = divide(10, 0).map([](int x) { return x + 1; }).unwrap_or(-1);
    return r1 == 6 && r2 == -1;
}(), "W10.7: safe divide with None propagation");

// ============================================================
// WAVE 11 — NOEXCEPT PROPAGATION
// ============================================================
static_assert(noexcept(
    std::declval<rl::Option<int>>().is_some_and([](int) noexcept { return true; })
), "W11.1: is_some_and noexcept with noexcept pred");

static_assert(noexcept(
    std::declval<rl::Option<int>>().is_none_or([](int) noexcept { return true; })
), "W11.2: is_none_or noexcept with noexcept pred");

static_assert(noexcept(
    std::declval<rl::Option<int>>().unwrap_or_else([]() noexcept { return 0; })
), "W11.3: unwrap_or_else noexcept with noexcept callback");

static_assert(noexcept(
    std::declval<rl::Option<int>>().unwrap_or(0)
), "W11.4: unwrap_or is noexcept for trivial type");

// ============================================================
// WAVE 12 — AS_MUT_PTR / AS_IMMUT_PTR
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(7);
    auto ptr_opt = opt.as_immut_ptr();
    return ptr_opt.is_some() && *ptr_opt.unwrap() == 7;
}(), "W12.1: as_immut_ptr on Some");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.as_immut_ptr().is_none();
}(), "W12.2: as_immut_ptr on None");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    auto ptr_opt = opt.as_mut_ptr();
    if (!ptr_opt.is_some()) return false;
    *ptr_opt.unwrap() = 99;
    return opt.unwrap() == 99;
}(), "W12.3: as_mut_ptr mutation");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.as_mut_ptr().is_none();
}(), "W12.4: as_mut_ptr on None");

// ============================================================
// WAVE 13 — MULTI-ALTERNATIVE EDGE CASES
// ============================================================
static_assert([]() consteval {
    using O = rl::Option<int, double, bool>;
    O a = rl::Some<int>(1);
    O b = rl::Some<double>(2.0);
    O c = rl::Some<bool>(true);
    O d = rl::None;
    return a.is_some<int>() && !a.is_some<double>() && !a.is_some<bool>()
        && b.is_some<double>() && !b.is_some<int>()
        && c.is_some<bool>() && !c.is_some<int>()
        && d.is_none();
}(), "W13.1: 3-alt Option type discrimination");

static_assert([]() consteval {
    using O = rl::Option<int, double, bool>;
    // match all 3 arms
    auto match_it = [](O opt) {
        return opt.match(
            rl::SomeAny >>= [](auto&& v) -> int {
                using T = std::remove_cvref_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int>)    return 1;
                else if constexpr (std::is_same_v<T, double>) return 2;
                else if constexpr (std::is_same_v<T, bool>)  return 3;
                else return -1;
            },
            rl::None >>= []() -> int { return 0; }
        ).unwrap();
    };
    return match_it(O(rl::Some<int>(5))) == 1
        && match_it(O(rl::Some<double>(2.0))) == 2
        && match_it(O(rl::Some<bool>(true))) == 3
        && match_it(O(rl::None)) == 0;
}(), "W13.2: 3-alt SomeAny constexpr-if dispatch");

static_assert([]() consteval {
    // map on all-alternatives callback
    using O = rl::Option<int, double>;
    O opt = rl::Some<int>(5);
    auto r = opt.map([](auto v) -> long { return static_cast<long>(v) * 2; });
    return r.is_some() && r.unwrap() == 10L;
}(), "W13.3: multi-alt map with generic lambda");

static_assert([]() consteval {
    // map<T> on a 3-alt option
    using O = rl::Option<int, double, bool>;
    O opt = rl::Some<bool>(true);
    auto r = opt.map<bool>([](bool b) { return !b; });
    return r.is_some() && r.unwrap() == false;
}(), "W13.4: targeted map<bool> on 3-alt Option");

static_assert([]() consteval {
    // and_then over multi-alt
    using O = rl::Option<int, double>;
    O opt = rl::Some<double>(2.5);
    auto r = opt.and_then([](auto v) -> rl::Option<int> {
        return rl::Some<int>(static_cast<int>(v * 2));
    });
    return r.is_some() && r.unwrap() == 5;
}(), "W13.5: multi-alt and_then with generic lambda");

// ============================================================
// WAVE 14 — EQUALITY / SPACESHIP
// ============================================================
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(5);
    rl::Option<int> b = rl::Some<int>(5);
    return a == b;
}(), "W14.1: Option == Option same value");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(5);
    rl::Option<int> b = rl::Some<int>(6);
    return a != b;
}(), "W14.2: Option != Option different value");

static_assert([]() consteval {
    rl::Option<int> a = rl::None;
    rl::Option<int> b = rl::None;
    return a == b;
}(), "W14.3: None == None");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(5);
    rl::Option<int> b = rl::None;
    return a != b && b != a;
}(), "W14.4: Some != None");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(3);
    return (a == rl::Some<int>(3)) && !(a == rl::Some<int>(4));
}(), "W14.5: Option == Some<T>(val)");

static_assert([]() consteval {
    // spaceship: Some(3) < Some(5)
    rl::Option<int> a = rl::Some<int>(3);
    rl::Option<int> b = rl::Some<int>(5);
    return (a <=> b) < 0;
}(), "W14.6: spaceship Some(3) < Some(5)");

static_assert([]() consteval {
    // spaceship: None < Some
    rl::Option<int> none = rl::None;
    rl::Option<int> some = rl::Some<int>(0);
    return (none <=> some) < 0;
}(), "W14.7: spaceship None < Some");

static_assert([]() consteval {
    // compare Option with raw value
    rl::Option<int> opt = rl::Some<int>(7);
    return opt == 7 && !(opt == 8);
}(), "W14.8: Option == raw value");

// ============================================================
// WAVE 15 — TRACKER: VERIFY COPY/MOVE COUNTS
// ============================================================
static_assert([]() consteval {
    // const& unwrap returns reference, no extra copy
    rl::Option<Tracker> opt = rl::Some<Tracker>(Tracker{1});
    const Tracker& ref = opt.unwrap();
    return ref.val == 1 && ref.copies == 0;
}(), "W15.1: const& unwrap returns ref, zero extra copies");

static_assert([]() consteval {
    // rvalue unwrap moves, no extra copy
    rl::Option<Tracker> opt = rl::Some<Tracker>(Tracker{5});
    Tracker t = std::move(opt).unwrap();
    return t.val == 5 && t.copies == 0;
}(), "W15.2: rvalue unwrap moves, zero extra copies");

static_assert([]() consteval {
    // map on && moves, not copies
    rl::Option<Tracker> opt = rl::Some<Tracker>(Tracker{3});
    auto r = std::move(opt).map([](Tracker t) -> int {
        return t.copies; // should be 0
    });
    return r.unwrap() == 0;
}(), "W15.3: map on rvalue doesn't copy (move-only path)");

// ============================================================
// WAVE 16 — ASSIGNMENT OPERATORS
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    opt = rl::Some<int>(42);
    return opt.is_some() && opt.unwrap() == 42;
}(), "W16.1: assign Some to None Option");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    opt = rl::None;
    return opt.is_none();
}(), "W16.2: assign None to Some Option");

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(1);
    opt = rl::Some<int>(99);
    return opt.unwrap() == 99;
}(), "W16.3: assign Some to Some Option");

// ============================================================
// WAVE 17 — ADVERSARIAL / BOUNDARY
// ============================================================
static_assert([]() consteval {
    // Option of Option (legal!)
    rl::Option<rl::Option<int>> outer = rl::Some<rl::Option<int>>(rl::Some<int>(7));
    return outer.is_some() && outer.unwrap().unwrap() == 7;
}(), "W17.1: Option of Option");

static_assert([]() consteval {
    // map producing None inner
    rl::Option<rl::Option<int>> outer = rl::Some<rl::Option<int>>(rl::None);
    return outer.is_some() && outer.unwrap().is_none();
}(), "W17.2: Option<Option<int>> with inner None");

static_assert([]() consteval {
    // very long chain doesn't overflow constexpr
    rl::Option<int> opt = rl::Some<int>(0);
    for (int i = 0; i < 10; ++i) {
        opt = std::move(opt).map([i](int x) { return x + i; });
    }
    // 0+0+1+2+3+4+5+6+7+8+9 = 45
    return opt.unwrap() == 45;
}(), "W17.3: 10-iteration map loop");

static_assert([]() consteval {
    // unwrap_or_else with stateful lambda
    int counter = 0;
    rl::Option<int> none = rl::None;
    int r = none.unwrap_or_else([&counter]() -> int {
        counter++;
        return 42;
    });
    return r == 42 && counter == 1;
}(), "W17.4: unwrap_or_else with stateful lambda");

static_assert([]() consteval {
    // map_or_else fallback not called when Some
    int called = 0;
    rl::Option<int> some = rl::Some<int>(5);
    int r = some.map_or_else(
        [](int x) { return x; },
        [&called]() -> int { called++; return 0; }
    );
    return r == 5 && called == 0;
}(), "W17.5: map_or_else fallback not called on Some");

static_assert([]() consteval {
    // is_some_and doesn't call pred on None
    int called = 0;
    rl::Option<int> none = rl::None;
    bool r = none.is_some_and([&called](int) { called++; return true; });
    return !r && called == 0;
}(), "W17.6: is_some_and doesn't invoke pred on None");

// ============================================================
// RUNTIME SECTION — panic + non-constexpr paths
// ============================================================
int main() {
    // Core runtime ops
    {
        rl::Option<int> opt = rl::Some<int>(42);
        assert(opt.unwrap() == 42);
        assert(opt.expect("test") == 42);
        assert(opt.value() == 42);
        assert(opt.unwrap_unchecked() == 42);
    }

    // unwrap_or / unwrap_or_else / unwrap_or_default runtime
    {
        rl::Option<int> none = rl::None;
        assert(none.unwrap_or(0) == 0);
        assert(none.unwrap_or_default() == 0);
        assert(none.unwrap_or_else([] { return 55; }) == 55);
        assert(none.unwrap_or_else([](rl::None_t) { return 77; }) == 77);
    }

    // as_mut_ptr mutation
    {
        rl::Option<int> opt = rl::Some<int>(5);
        auto ptr_opt = opt.as_mut_ptr();
        assert(ptr_opt.is_some());
        *ptr_opt.unwrap() = 100;
        assert(opt.unwrap() == 100);
    }

    // 3-alt match runtime
    {
        using O3 = rl::Option<int, double, bool>;
        auto match3 = [](O3 opt) -> double {
            return opt.match(
                rl::SomeAny >>= [](auto&& v) -> double {
                    using T = std::remove_cvref_t<decltype(v)>;
                    if constexpr (std::is_same_v<T,int>)    return (double)v;
                    else if constexpr (std::is_same_v<T,double>) return v;
                    else return v ? 1.0 : 0.0;
                },
                rl::None >>= []() -> double { return -1.0; }
            ).unwrap();
        };
        assert(match3(O3(rl::Some<int>(10))) == 10.0);
        assert(match3(O3(rl::Some<double>(2.5))) == 2.5);
        assert(match3(O3(rl::Some<bool>(true))) == 1.0);
        assert(match3(O3(rl::None)) == -1.0);
    }

    // Deep chain runtime
    {
        rl::Option<int> opt = rl::Some<int>(5);
        int r = opt
            .map([](int x) { return x * 2; })
            .and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x + 1); })
            .map_or([](int x) { return x; }, 0);
        assert(r == 11);
    }

    // Spaceship runtime
    {
        rl::Option<int> a = rl::Some<int>(3);
        rl::Option<int> b = rl::Some<int>(5);
        rl::Option<int> none = rl::None;
        assert((a <=> b) < 0);
        assert((b <=> a) > 0);
        assert((none <=> a) < 0);
    }

    // is_default runtime
    {
        rl::Option<int> opt0 = rl::Some<int>(0);
        rl::Option<int> opt1 = rl::Some<int>(1);
        rl::Option<int> none = rl::None;
        assert(opt0.is_default());
        assert(!opt1.is_default());
        assert(!none.is_default());
    }

    // as_immut_ptr runtime
    {
        rl::Option<int> opt = rl::Some<int>(7);
        auto p = opt.as_immut_ptr();
        assert(p.is_some() && *p.unwrap() == 7);
        rl::Option<int> none = rl::None;
        assert(none.as_immut_ptr().is_none());
    }

    std::puts("stress_constexpr_battery: ALL RUNTIME TESTS PASSED");
    return 0;
}
