// stress_wave3.cpp — 4-alternative options, dynamic types, generic combinators, pointers, emplace & reset
#include "rl_option.hpp"
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <cstdio>

// ============================================================
// WAVE 3.1: 4-ALTERNATIVE CONSTEXPR DISPATCH
// ============================================================

using QuadOpt = rl::Option<int, float, char, long>;

static_assert([]() consteval {
    QuadOpt q1 = rl::Some<int>(42);
    QuadOpt q2 = rl::Some<float>(3.14f);
    QuadOpt q3 = rl::Some<char>('A');
    QuadOpt q4 = rl::Some<long>(99999L);
    QuadOpt qn = rl::None;

    return q1.is_some<int>() && !q1.is_some<float>() &&
           q2.is_some<float>() && !q2.is_some<char>() &&
           q3.is_some<char>() && !q3.is_some<long>() &&
           q4.is_some<long>() && !q4.is_some<int>() &&
           qn.is_none();
}(), "W3.1.1: QuadOpt type discrimination");

// Targeted map on each alternative
static_assert([]() consteval {
    QuadOpt q1 = rl::Some<int>(10);
    QuadOpt q2 = rl::Some<float>(2.5f);
    QuadOpt q3 = rl::Some<char>('x');
    QuadOpt q4 = rl::Some<long>(100L);

    auto r1 = q1.map<int>([](int x) { return x * 2; });
    auto r2 = q2.map<float>([](float f) { return f * 2.0f; });
    auto r3 = q3.map<char>([](char c) -> int { return static_cast<int>(c) + 1; });
    auto r4 = q4.map<long>([](long l) { return l + 50L; });

    // Wrong target must return None
    auto w1 = q1.map<float>([](float f) { return f * 2.0f; });
    auto w3 = q3.map<int>([](int x) { return x * 2; });

    return r1.unwrap() == 20 &&
           r2.unwrap() > 4.99f && r2.unwrap() < 5.01f &&
           r3.unwrap() == static_cast<int>('x') + 1 &&
           r4.unwrap() == 150L &&
           w1.is_none() &&
           w3.is_none();
}(), "W3.1.2: QuadOpt targeted map<Target>");

// Generic map handling all 4 alternatives
static_assert([]() consteval {
    auto double_it = [](auto val) -> double {
        return static_cast<double>(val) * 2.0;
    };

    QuadOpt q1 = rl::Some<int>(5);
    QuadOpt q2 = rl::Some<float>(1.5f);
    QuadOpt q3 = rl::Some<char>(10);
    QuadOpt q4 = rl::Some<long>(20L);
    QuadOpt qn = rl::None;

    auto r1 = q1.map(double_it);
    auto r2 = q2.map(double_it);
    auto r3 = q3.map(double_it);
    auto r4 = q4.map(double_it);
    auto rn = qn.map(double_it);

    return r1.unwrap() > 9.99 && r1.unwrap() < 10.01 &&
           r2.unwrap() > 2.99 && r2.unwrap() < 3.01 &&
           r3.unwrap() > 19.99 && r3.unwrap() < 20.01 &&
           r4.unwrap() > 39.99 && r4.unwrap() < 40.01 &&
           rn.is_none();
}(), "W3.1.3: QuadOpt generic map across all alternatives");

// Generic and_then across all 4 alternatives
static_assert([]() consteval {
    auto to_opt_int = [](auto val) -> rl::Option<int> {
        return rl::Some<int>(static_cast<int>(val));
    };

    QuadOpt q1 = rl::Some<int>(1);
    QuadOpt q2 = rl::Some<float>(2.0f);
    QuadOpt q3 = rl::Some<char>(3);
    QuadOpt q4 = rl::Some<long>(4L);
    QuadOpt qn = rl::None;

    return q1.and_then(to_opt_int).unwrap() == 1 &&
           q2.and_then(to_opt_int).unwrap() == 2 &&
           q3.and_then(to_opt_int).unwrap() == 3 &&
           q4.and_then(to_opt_int).unwrap() == 4 &&
           qn.and_then(to_opt_int).is_none();
}(), "W3.1.4: QuadOpt generic and_then");

// or_else on QuadOpt preserving active alternative
static_assert([]() consteval {
    QuadOpt q1 = rl::Some<int>(1);
    QuadOpt q2 = rl::Some<float>(2.0f);
    QuadOpt q3 = rl::Some<char>('z');
    QuadOpt q4 = rl::Some<long>(400L);
    QuadOpt qn = rl::None;

    auto fallback = []() -> QuadOpt {
        return rl::Some<int>(999);
    };

    auto r1 = q1.or_else(fallback);
    auto r2 = q2.or_else(fallback);
    auto r3 = q3.or_else(fallback);
    auto r4 = q4.or_else(fallback);
    auto rn = qn.or_else(fallback);

    return r1.is_some<int>() && r1.unwrap<int>() == 1 &&
           r2.is_some<float>() && r2.unwrap<float>() > 1.99f &&
           r2.unwrap<float>() < 2.01f &&
           r3.is_some<char>() && r3.unwrap<char>() == 'z' &&
           r4.is_some<long>() && r4.unwrap<long>() == 400L &&
           rn.is_some<int>() && rn.unwrap<int>() == 999;
}(), "W3.1.5: QuadOpt or_else alternative preservation");

// Predicates on QuadOpt
static_assert([]() consteval {
    auto is_gt_two = [](auto val) { return val > 2; };

    QuadOpt q1 = rl::Some<int>(1);
    QuadOpt q2 = rl::Some<float>(2.5f);
    QuadOpt q3 = rl::Some<char>(5);
    QuadOpt qn = rl::None;

    return !q1.is_some_and(is_gt_two) &&
            q2.is_some_and(is_gt_two) &&
            q3.is_some_and(is_gt_two) &&
           !qn.is_some_and(is_gt_two) &&
            q1.is_none_or(is_gt_two) == false &&
            q2.is_none_or(is_gt_two) == true &&
            qn.is_none_or(is_gt_two) == true;
}(), "W3.1.6: QuadOpt predicates is_some_and & is_none_or");

// map_or & map_or_else on QuadOpt
static_assert([]() consteval {
    auto to_int = [](auto val) -> int { return static_cast<int>(val); };

    QuadOpt q1 = rl::Some<char>(15);
    QuadOpt qn = rl::None;

    int r1 = q1.map_or(to_int, -1);
    int rn = qn.map_or(to_int, -1);

    int r2 = q1.map_or_else(to_int, [] { return -99; });
    int rn2 = qn.map_or_else(to_int, [] { return -99; });

    return r1 == 15 && rn == -1 && r2 == 15 && rn2 == -99;
}(), "W3.1.7: QuadOpt map_or and map_or_else");

// ============================================================
// WAVE 3.2: POINTER AND SPECIAL TYPES
// ============================================================

static_assert([]() consteval {
    int val = 42;
    rl::Option<const int*> opt = rl::Some<const int*>(&val);
    rl::Option<const int*> none = rl::None;

    return opt.is_some() &&
           *opt.unwrap() == 42 &&
           none.is_none();
}(), "W3.2.1: Option with raw pointer type");

static_assert([]() consteval {
    int val = 100;
    rl::Option<int*> opt = rl::Some<int*>(&val);
    auto r = opt.map([](int* p) { return *p + 1; });
    return r.unwrap() == 101;
}(), "W3.2.2: Option<int*> map dereference");

static_assert([]() consteval {
    rl::Option<std::nullptr_t> opt = rl::Some<std::nullptr_t>(nullptr);
    return opt.is_some() && opt.unwrap() == nullptr;
}(), "W3.2.3: Option<std::nullptr_t>");

// ============================================================
// WAVE 3.3: EMPLACE & RESET
// ============================================================

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    opt.emplace<int>(123);
    return opt.is_some() && opt.unwrap() == 123;
}(), "W3.3.1: emplace on single Option");

static_assert([]() consteval {
    QuadOpt opt = rl::None;
    opt.emplace<char>('K');
    bool c1 = opt.is_some<char>() && opt.unwrap<char>() == 'K';

    opt.emplace<float>(9.99f);
    bool c2 = opt.is_some<float>() && opt.unwrap<float>() > 9.98f &&
              opt.unwrap<float>() < 10.0f && !opt.is_some<char>();

    opt.reset();
    bool c3 = opt.is_none();

    return c1 && c2 && c3;
}(), "W3.3.2: emplace & reset on QuadOpt");

// ============================================================
// WAVE 3.4: CHAINED TYPE EVOLUTION
// ============================================================

static_assert([]() consteval {
    // int -> double -> char -> int -> long
    rl::Option<int> start = rl::Some<int>(65);

    long final_val = start
        .map([](int i) -> double { return static_cast<double>(i) + 0.5; })
        .map([](double d) -> char { return static_cast<char>(static_cast<int>(d)); }) // 'A'
        .and_then([](char c) -> rl::Option<int> {
            if (c == 'A') return rl::Some<int>(1000);
            return rl::None;
        })
        .map_or([](int x) -> long { return static_cast<long>(x) * 3L; }, 0L);

    return final_val == 3000L;
}(), "W3.4.1: multi-step heterogeneous type chain");

// ============================================================
// RUNTIME SECTION: std::string & dynamic memory
// ============================================================

int main() {
    // Multi-alternative with non-trivial dynamic types (std::string)
    using StringOpt = rl::Option<int, std::string, double>;

    {
        StringOpt s1 = rl::Some<std::string>("hello world");
        assert(s1.is_some<std::string>());
        assert(!s1.is_some<int>());
        assert(!s1.is_some<double>());
        assert(s1.unwrap<std::string>() == "hello world");

        // map on std::string
        auto r = s1.map<std::string>([](const std::string& s) {
            return s + "!!!";
        });
        assert(r.is_some());
        assert(r.unwrap() == "hello world!!!");

        // Generic map
        auto len = s1.map([](auto&& v) -> size_t {
            using T = std::remove_cvref_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v.size();
            } else {
                return 0;
            }
        });
        assert(len.unwrap() == 11);
    }

    // std::string moved properly
    {
        StringOpt s1 = rl::Some<std::string>("move me");
        std::string extracted = std::move(s1).unwrap<std::string>();
        assert(extracted == "move me");
    }

    // or_else with std::string
    {
        StringOpt s_none = rl::None;
        auto recovered = s_none.or_else([]() -> StringOpt {
            return rl::Some<std::string>("fallback string");
        });
        assert(recovered.is_some<std::string>());
        assert(recovered.unwrap<std::string>() == "fallback string");
    }

    // emplace with std::string in-place construction
    {
        StringOpt opt = rl::None;
        opt.emplace<std::string>(5, 'z');
        assert(opt.is_some<std::string>());
        assert(opt.unwrap<std::string>() == "zzzzz");

        opt.emplace<int>(42);
        assert(opt.is_some<int>());
        assert(!opt.is_some<std::string>());
        assert(opt.unwrap<int>() == 42);

        opt.reset();
        assert(opt.is_none());
    }

    // QuadOpt runtime test
    {
        QuadOpt q = rl::Some<char>('X');
        assert(q.is_some<char>());
        assert(q.unwrap<char>() == 'X');

        auto mapped = q.map<char>([](char c) { return c + 1; });
        assert(mapped.unwrap() == 'Y');
    }

    std::puts("stress_wave3: ALL RUNTIME TESTS PASSED");
    return 0;
}
