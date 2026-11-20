// stress_wave2.cpp — deeper noexcept, type traits, self-assign, constexpr exhaustion
#include "rl_option.hpp"
#include <cassert>
#include <type_traits>
#include <utility>
#include <cstdio>

// ============================================================
// HELPERS
// ============================================================
struct ThrowCopy {
    int v;
    constexpr ThrowCopy() noexcept : v(0) {}
    constexpr explicit ThrowCopy(int x) noexcept : v(x) {}
    ThrowCopy(const ThrowCopy&)                      { v = 0; } // throws in theory
    ThrowCopy& operator=(const ThrowCopy&)            { v = 0; return *this; }
    constexpr ThrowCopy(ThrowCopy&&) noexcept         = default;
    constexpr ThrowCopy& operator=(ThrowCopy&&) noexcept = default;
};

struct NoThrowCopy {
    int v;
    constexpr NoThrowCopy() noexcept : v(0) {}
    constexpr explicit NoThrowCopy(int x) noexcept : v(x) {}
    constexpr NoThrowCopy(const NoThrowCopy&) noexcept = default;
    constexpr NoThrowCopy& operator=(const NoThrowCopy&) noexcept = default;
    constexpr NoThrowCopy(NoThrowCopy&&) noexcept = default;
    constexpr NoThrowCopy& operator=(NoThrowCopy&&) noexcept = default;
};

struct MoveOnly {
    int v;
    constexpr explicit MoveOnly(int x) : v(x) {}
    MoveOnly(const MoveOnly&)            = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    constexpr MoveOnly(MoveOnly&&) noexcept            = default;
    constexpr MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

// ============================================================
// WAVE A — unwrap_or_default noexcept
// ============================================================
static_assert(noexcept(
    std::declval<rl::Option<NoThrowCopy>&>().unwrap_or_default()
), "WA.1: unwrap_or_default noexcept for noexcept-copy type");

static_assert(noexcept(
    std::declval<rl::Option<NoThrowCopy>&&>().unwrap_or_default()
), "WA.2: unwrap_or_default && noexcept for noexcept-move type");

static_assert(!noexcept(
    std::declval<rl::Option<ThrowCopy>&>().unwrap_or_default()
), "WA.3: unwrap_or_default NOT noexcept for throwing-copy type");

// ============================================================
// WAVE B — unwrap_or noexcept propagation
// ============================================================
static_assert(noexcept(
    std::declval<rl::Option<int>&>().unwrap_or(0)
), "WB.1: unwrap_or & noexcept for int (trivial)");

static_assert(noexcept(
    std::declval<rl::Option<int>&&>().unwrap_or(0)
), "WB.2: unwrap_or && noexcept for int (trivial)");

static_assert(noexcept(
    std::declval<rl::Option<NoThrowCopy>&>().unwrap_or(NoThrowCopy{0})
), "WB.3: unwrap_or & noexcept for NoThrowCopy");

// ============================================================
// WAVE C — unwrap_or_else noexcept propagation
// ============================================================
static_assert(noexcept(
    std::declval<rl::Option<int>&>().unwrap_or_else([]() noexcept { return 0; })
), "WC.1: unwrap_or_else & noexcept");

static_assert(noexcept(
    std::declval<rl::Option<int>&&>().unwrap_or_else([]() noexcept { return 0; })
), "WC.2: unwrap_or_else && noexcept");

// ============================================================
// WAVE D — is_some_and / is_none_or noexcept
// ============================================================
static_assert(noexcept(
    std::declval<rl::Option<int>&>().is_some_and([](int) noexcept { return true; })
), "WD.1: is_some_and noexcept pred → noexcept");

static_assert(!noexcept(
    std::declval<rl::Option<int>&>().is_some_and([](int) { return true; })
), "WD.2: is_some_and non-noexcept pred → not noexcept");

static_assert(noexcept(
    std::declval<rl::Option<int>&>().is_none_or([](int) noexcept { return true; })
), "WD.3: is_none_or noexcept pred → noexcept");

// ============================================================
// WAVE E — type_traits on Option
// ============================================================
// Option<int> is copyable
static_assert(std::is_copy_constructible_v<rl::Option<int>>, "WE.1: Option<int> copy-constructible");
static_assert(std::is_copy_assignable_v<rl::Option<int>>,    "WE.2: Option<int> copy-assignable");
static_assert(std::is_move_constructible_v<rl::Option<int>>, "WE.3: Option<int> move-constructible");
static_assert(std::is_move_assignable_v<rl::Option<int>>,    "WE.4: Option<int> move-assignable");

// Option<MoveOnly> is move-only
static_assert(!std::is_copy_constructible_v<rl::Option<MoveOnly>>, "WE.5: Option<MoveOnly> NOT copy-constructible");
static_assert(std::is_move_constructible_v<rl::Option<MoveOnly>>,  "WE.6: Option<MoveOnly> IS move-constructible");

// Option is nothrow-move when T is nothrow-move
static_assert(std::is_nothrow_move_constructible_v<rl::Option<int>>, "WE.7: Option<int> nothrow-move");

// Option is NOT default-constructible (must explicitly specify Some or None)
static_assert(!std::is_default_constructible_v<rl::Option<int>>, "WE.8: Option<int> not default-constructible");

// ============================================================
// WAVE F — None-constructed is None
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.is_none();
}(), "WF.1: None-constructed Option is None");

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::None;
    return opt.is_none();
}(), "WF.2: None-constructed multi-alt Option is None");

// ============================================================
// WAVE G — SELF-ASSIGNMENT
// ============================================================
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    opt = rl::Some<int>(5);  // assign same type/value to itself
    return opt.is_some() && opt.unwrap() == 5;
}(), "WG.1: re-assign Some to Some");

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    opt = rl::None;
    return opt.is_none();
}(), "WG.2: assign None to None (idempotent)");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(3);
    rl::Option<int> b = rl::Some<int>(7);
    a = b;
    return a.unwrap() == 7 && b.unwrap() == 7;
}(), "WG.3: copy-assign from other Some");

static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(3);
    rl::Option<int> b = rl::None;
    a = b;
    return a.is_none();
}(), "WG.4: copy-assign None from other");

// ============================================================
// WAVE H — CONSTEXPR STRESS (deep recursion via templates)
// ============================================================

// Recursive Fibonacci option-style
template<int N>
consteval int fib_opt() {
    if constexpr (N <= 1) {
        return N;
    } else {
        auto a = rl::Option<int>(rl::Some<int>(fib_opt<N-1>()));
        auto b = rl::Option<int>(rl::Some<int>(fib_opt<N-2>()));
        return a.and_then([&](int av) -> rl::Option<int> {
            return b.map([av](int bv) { return av + bv; });
        }).unwrap();
    }
}

static_assert(fib_opt<0>() == 0,  "WH.1: fib_opt(0) == 0");
static_assert(fib_opt<1>() == 1,  "WH.2: fib_opt(1) == 1");
static_assert(fib_opt<5>() == 5,  "WH.3: fib_opt(5) == 5");
static_assert(fib_opt<10>() == 55, "WH.4: fib_opt(10) == 55");

// ============================================================
// WAVE I — CONSTEXPR MAP RETURNS CORRECT WRAPPER TYPE
// ============================================================
static_assert([]() consteval {
    // map(int→double) returns Option<double>
    rl::Option<int> opt = rl::Some<int>(3);
    auto r = opt.map([](int x) -> double { return x * 2.0; });
    using T = std::remove_cvref_t<decltype(r)>;
    // T should be Option<double>
    static_assert(std::same_as<T, rl::Option<double>>);
    return r.unwrap() == 6.0;
}(), "WI.1: map return type is Option<R>");

static_assert([]() consteval {
    // and_then(int→Option<double>) returns Option<double>
    rl::Option<int> opt = rl::Some<int>(2);
    auto r = opt.and_then([](int x) -> rl::Option<double> {
        return rl::Some<double>(x * 1.5);
    });
    using T = std::remove_cvref_t<decltype(r)>;
    static_assert(std::same_as<T, rl::Option<double>>);
    return r.unwrap() == 3.0;
}(), "WI.2: and_then return type is Option<R>");

// ============================================================
// WAVE J — MATCH EXHAUSTIVENESS & VALUE SEMANTICS
// ============================================================
static_assert([]() consteval {
    // match on const& doesn't consume
    const rl::Option<int> opt = rl::Some<int>(5);
    int r1 = opt.match(rl::SomeAny >>= [](int x) { return x; },
                       rl::None    >>= []()      { return 0; }).unwrap();
    int r2 = opt.match(rl::SomeAny >>= [](int x) { return x * 2; },
                       rl::None    >>= []()      { return 0; }).unwrap();
    return r1 == 5 && r2 == 10 && opt.is_some(); // still alive
}(), "WJ.1: match on const& doesn't consume Option");

static_assert([]() consteval {
    // match on && consumes (Option becomes moved-from)
    rl::Option<MoveOnly> opt = rl::Some<MoveOnly>(MoveOnly{42});
    int r = std::move(opt).match(
        rl::SomeAny >>= [](MoveOnly m) { return m.v; },
        rl::None    >>= []()           { return -1; }
    ).unwrap();
    return r == 42;
}(), "WJ.2: match && moves value out");

static_assert([]() consteval {
    // match produces the right Option type
    rl::Option<int> opt = rl::Some<int>(3);
    auto result_opt = opt.match(
        rl::SomeAny >>= [](int x) { return x + 10; },
        rl::None    >>= []()      { return -1; }
    );
    using T = std::remove_cvref_t<decltype(result_opt)>;
    static_assert(std::same_as<T, rl::Option<int>>);
    return result_opt.unwrap() == 13;
}(), "WJ.3: match result is Option<R>");

// ============================================================
// WAVE K — COMBINATORS ON CONST OPTION
// ============================================================
static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.map([](int x) { return x * 2; }).unwrap() == 10;
}(), "WK.1: const Option map");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(3);
    return opt.and_then([](int x) -> rl::Option<int> {
        return rl::Some<int>(x + 1);
    }).unwrap() == 4;
}(), "WK.2: const Option and_then");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::None;
    return opt.or_else([] { return rl::Option<int>(rl::Some<int>(99)); }).unwrap() == 99;
}(), "WK.3: const Option or_else");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.is_some_and([](int x) { return x > 3; });
}(), "WK.4: const Option is_some_and");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.map_or([](int x) { return x + 1; }, 0) == 6;
}(), "WK.5: const Option map_or");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.unwrap_or(0) == 5;
}(), "WK.6: const Option unwrap_or");

static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(5);
    return opt.unwrap_or_else([] { return 0; }) == 5;
}(), "WK.7: const Option unwrap_or_else");

// ============================================================
// WAVE L — NONE PROPAGATION EXHAUSTIVE
// ============================================================
static_assert([]() consteval {
    // Every combinator short-circuits on None
    rl::Option<int> none = rl::None;
    bool map_none    = none.map([](int x) { return x; }).is_none();
    bool and_none    = none.and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x); }).is_none();
    bool maport_none = (none.map_or([](int x) { return x; }, -1) == -1);
    bool maporte_none= (none.map_or_else([](int x) { return x; }, [] { return -1; }) == -1);
    bool isa_none    = !none.is_some_and([](int) { return true; });
    bool isno_none   = none.is_none_or([](int) { return false; });
    return map_none && and_none && maport_none && maporte_none && isa_none && isno_none;
}(), "WL.1: all combinators short-circuit on None");

// ============================================================
// RUNTIME SECTION
// ============================================================
int main() {
    // noexcept at runtime is irrelevant but confirm types compile right

    // Fibonacci via Option
    {
        assert(fib_opt<7>() == 13);
        assert(fib_opt<10>() == 55);
    }

    // Self-assign runtime
    {
        rl::Option<int> opt = rl::Some<int>(5);
        auto& ref = opt;
        ref = rl::Some<int>(5); // "self-assign" via same-value reassign
        assert(opt.unwrap() == 5);
    }

    // const Option ops
    {
        const rl::Option<int> opt = rl::Some<int>(7);
        assert(opt.map([](int x) { return x + 1; }).unwrap() == 8);
        assert(opt.and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x); }).unwrap() == 7);
        assert(opt.is_some_and([](int x) { return x > 5; }));
        assert(opt.unwrap_or(0) == 7);
        assert(opt.unwrap_or_else([] { return 0; }) == 7);
        assert(opt.map_or([](int x) { return x; }, 0) == 7);
    }

    // None propagation runtime  
    {
        rl::Option<int> none = rl::None;
        assert(none.map([](int x) { return x; }).is_none());
        assert(none.and_then([](int x) -> rl::Option<int> { return rl::Some<int>(x); }).is_none());
        assert(none.map_or([](int x) { return x; }, -1) == -1);
        assert(!none.is_some_and([](int) { return true; }));
        assert(none.is_none_or([](int) { return false; }));
    }

    // Fib Option style
    {
        auto fib = [](auto& self, int n) -> rl::Option<int> {
            if (n < 0) return rl::None;
            if (n <= 1) return rl::Some<int>(n);
            auto a = self(self, n - 1);
            auto b = self(self, n - 2);
            return a.and_then([&](int av) -> rl::Option<int> {
                return b.map([av](int bv) { return av + bv; });
            });
        };
        assert(fib(fib, 10).unwrap() == 55);
        assert(fib(fib, -1).is_none());
    }

    std::puts("stress_wave2: ALL RUNTIME TESTS PASSED");
    return 0;
}
