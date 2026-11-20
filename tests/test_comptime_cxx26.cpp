#include "rl_option.hpp"
#include <string_view>
#include <compare>
#include <cassert>
#include <iostream>

// =============================================================================
// Comprehensive C++26 Compile-Time Constexpr Verification Suite
// =============================================================================

// 1. Basic Single-Type Constexpr Lifecycle
consteval auto test_single_opt_lifecycle() {
    rl::Option<int> opt = rl::Some(42);
    if (!opt.is_some() || opt.is_none()) return false;
    if (opt.unwrap() != 42) return false;
    if (opt.value() != 42) return false;
    if (opt.expect("value expected") != 42) return false;

    // Mutate in place
    opt.unwrap() = 100;
    if (opt.unwrap() != 100) return false;

    // Reset
    opt.reset();
    if (opt.is_some() || !opt.is_none()) return false;
    if (opt.unwrap_or(999) != 999) return false;
    if (opt.unwrap_or_default() != 0) return false;

    // Emplace
    opt.emplace(777);
    if (!opt.is_some() || opt.unwrap() != 777) return false;

    return true;
}
static_assert(test_single_opt_lifecycle());

// 2. Multi-Type Variadic Option at Compile Time
consteval auto test_multi_opt_lifecycle() {
    using MultiOpt = rl::Option<int, double, std::string_view>;

    // Default initialized via None
    MultiOpt opt = rl::None;
    if (opt.is_some() || !opt.is_none()) return false;

    // Emplace alternative 0 (int)
    opt.emplace<int>(123);
    if (!opt.is_some() || !opt.is_some<int>()) return false;
    if (opt.is_some<double>() || opt.is_some<std::string_view>()) return false;
    if (opt.unwrap<int>() != 123) return false;

    // Emplace alternative 1 (double)
    opt.emplace<double>(3.14);
    if (!opt.is_some() || opt.is_some<int>() || !opt.is_some<double>()) return false;
    if (opt.unwrap<double>() != 3.14) return false;

    // Emplace alternative 2 (string_view)
    opt.emplace<std::string_view>("compile_time");
    if (!opt.is_some() || !opt.is_some<std::string_view>()) return false;
    if (opt.unwrap<std::string_view>() != "compile_time") return false;

    // is_some_and on secondary alternative
    if (!opt.is_some_and<std::string_view>([](std::string_view s) { return s.starts_with("compile"); })) return false;
    if (opt.is_some_and<std::string_view>([](std::string_view s) { return s.empty(); })) return false;

    return true;
}
static_assert(test_multi_opt_lifecycle());

// 3. Compile-Time Pattern Matching
consteval auto test_comptime_match() {
    rl::Option<int> opt = rl::Some(10);

    // Exact value and fallback match
    auto res1 = opt.match(
        rl::Some(10) >>= [](int x) { return x * 2; },
        rl::Wild >>= [](int) { return 0; },
        rl::None >>= [] { return -1; }
    );
    if (!res1.is_some() || res1.unwrap() != 20) return false;

    // Predicate guard match
    rl::Option<int> opt2 = rl::Some(75);
    auto res2 = opt2.match(
        [](int x) { return x > 50; } >>= [](int x) { return x + 10; },
        rl::Wild >>= [](int) { return 0; },
        rl::None >>= [] { return -1; }
    );
    if (!res2.is_some() || res2.unwrap() != 85) return false;

    // Boolean exhaustive match
    rl::Option<bool> opt_b = rl::Some(true);
    auto res_b = opt_b.match(
        rl::True >>= [] { return 1; },
        rl::False >>= [] { return 0; },
        rl::None >>= [] { return -1; }
    );
    if (!res_b.is_some() || res_b.unwrap() != 1) return false;

    // Multi-type pattern match
    rl::Option<int, std::string_view> multi = rl::Some<std::string_view>("cxx26");
    auto res_m = multi.match(
        rl::SomeAny >>= [](int x) { return std::size_t(x); },
        rl::SomeAny >>= [](std::string_view s) { return s.size(); },
        rl::Wild >>= [] { return std::size_t{0}; },
        rl::None >>= [] { return std::size_t{0}; }
    );
    if (!res_m.is_some() || res_m.unwrap() != 5) return false;

    return true;
}
static_assert(test_comptime_match());

// 4. Monadic APIs at Compile Time
consteval auto test_comptime_monadic() {
    rl::Option<int> opt = rl::Some(21);

    // map
    auto m = opt.map([](int x) { return x * 2; });
    if (!m.is_some() || m.unwrap() != 42) return false;

    // and_then
    auto at = opt.and_then([](int x) {
        return rl::Option<int>(rl::Some(x + 9));
    });
    if (!at.is_some() || at.unwrap() != 30) return false;

    // or_else
    rl::Option<int> none_opt = rl::None;
    auto oe = none_opt.or_else([] {
        return rl::Option<int>(rl::Some(500));
    });
    if (!oe.is_some() || oe.unwrap() != 500) return false;

    // map_or & map_or_else
    if (opt.map_or([](int x) { return x + 1; }, 0) != 22) return false;
    if (none_opt.map_or([](int x) { return x + 1; }, 0) != 0) return false;
    if (opt.map_or_else([](int x) { return x * 2; }, [] { return -1; }) != 42) return false;

    return true;
}
static_assert(test_comptime_monadic());

// 5. Heterogeneous Comparisons & Three-Way Spaceship at Compile Time
consteval auto test_comptime_comparisons() {
    rl::Option<int> a = rl::Some(10);
    rl::Option<int> b = rl::Some(20);
    rl::Option<int> none = rl::None;

    // Option <=> Option
    if ((a <=> b) != std::strong_ordering::less) return false;
    if ((b <=> a) != std::strong_ordering::greater) return false;
    if ((a <=> a) != std::strong_ordering::equal) return false;
    if ((none <=> a) != std::strong_ordering::less) return false;
    if ((a <=> none) != std::strong_ordering::greater) return false;
    if ((none <=> none) != std::strong_ordering::equal) return false;

    // Option <=> None
    if ((a <=> rl::None) != std::strong_ordering::greater) return false;
    if ((none <=> rl::None) != std::strong_ordering::equal) return false;

    // Option <=> Some<T>
    if ((a <=> rl::Some(10)) != std::strong_ordering::equal) return false;
    if ((a <=> rl::Some(5)) != std::strong_ordering::greater) return false;
    if ((a <=> rl::Some(15)) != std::strong_ordering::less) return false;
    if ((none <=> rl::Some(10)) != std::strong_ordering::less) return false;

    // Option <=> raw value
    if ((a <=> 10) != std::strong_ordering::equal) return false;
    if ((a <=> 5) != std::strong_ordering::greater) return false;
    if ((a <=> 15) != std::strong_ordering::less) return false;
    if ((none <=> 10) != std::strong_ordering::less) return false;
    if ((10 <=> a) != std::strong_ordering::equal) return false;
    if ((5 <=> a) != std::strong_ordering::less) return false;
    if ((15 <=> a) != std::strong_ordering::greater) return false;

    // Option == raw value
    if (!(a == 10) || (a == 20)) return false;
    if (!(10 == a) || (20 == a)) return false;

    return true;
}
static_assert(test_comptime_comparisons());

// 6. Zero-argument Some() Default Deduction
consteval auto test_comptime_some_void() {
    rl::Option<int> opt_def = rl::Some();
    if (!opt_def.is_some() || opt_def.unwrap() != 0) return false;

    rl::Option<double> opt_dbl = rl::Some();
    if (!opt_dbl.is_some() || opt_dbl.unwrap() != 0.0) return false;

    return true;
}
static_assert(test_comptime_some_void());

// 7. Swap in Constexpr
consteval auto test_comptime_swap() {
    rl::Option<int> a = rl::Some(100);
    rl::Option<int> b = rl::Some(200);
    a.swap(b);
    if (a.unwrap() != 200 || b.unwrap() != 100) return false;

    rl::Option<int> c = rl::None;
    a.swap(c);
    if (a.is_some() || !c.is_some() || c.unwrap() != 200) return false;

    return true;
}
static_assert(test_comptime_swap());

// 8. Universal cvref matrix on monadic and accessor APIs (const &&)
consteval auto test_comptime_const_rvalue() {
    const rl::Option<int> opt = rl::Some(50);
    // map const &&
    auto m = std::move(opt).map([](int x) { return x + 1; });
    if (!m.is_some() || m.unwrap() != 51) return false;

    // map_or const &&
    if (std::move(opt).map_or([](int x) { return x + 2; }, 0) != 52) return false;

    // map_or_else const &&
    if (std::move(opt).map_or_else([](int x) { return x + 3; }, [] { return 0; }) != 53) return false;

    // map_or_transform const &&
    if (std::move(opt).map_or_transform([](int x) { return x + 4; }, 0) != 54) return false;

    // and_then const &&
    auto at = std::move(opt).and_then([](int x) { return rl::Option<int>(rl::Some(x + 5)); });
    if (!at.is_some() || at.unwrap() != 55) return false;

    // or_else const &&
    auto oe = std::move(opt).or_else([] { return rl::Option<int>(rl::Some(999)); });
    if (!oe.is_some() || oe.unwrap() != 50) return false;

    // unwrap_or const &&
    if (std::move(opt).unwrap_or(0) != 50) return false;

    // unwrap_or_default const &&
    if (std::move(opt).unwrap_or_default() != 50) return false;

    // unwrap_unchecked const &&
    if (std::move(opt).unwrap_unchecked() != 50) return false;

    // unwrap_or_else const &&
    if (std::move(opt).unwrap_or_else([] { return 0; }) != 50) return false;

    // None case with const &&
    const rl::Option<int> none_opt = rl::None;
    if (std::move(none_opt).map_or([](int x) { return x; }, 99) != 99) return false;
    if (std::move(none_opt).unwrap_or(99) != 99) return false;
    if (std::move(none_opt).unwrap_or_default() != 0) return false;
    if (std::move(none_opt).unwrap_or_else([] { return 99; }) != 99) return false;

    return true;
}
static_assert(test_comptime_const_rvalue());

int main() {
    std::cout << "All C++26 compile-time constexpr static_asserts passed successfully!\n";
    return 0;
}
