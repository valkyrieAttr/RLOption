#include "rl_option.hpp"
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>

// =============================================================================
// C++26 Compile-Time Constexpr Evaluation (All Combinators on Active Alternative)
// =============================================================================
constexpr bool test_constexpr_active_mapping() {
    rl::Option<int, double> opt_dbl = rl::Some<double>(4.5);
    rl::Option<int, double> opt_int = rl::Some<int>(20);
    rl::Option<int, double> opt_none = rl::None;

    // 1. Homogeneous map on multi-alternative Option (both int & double supported)
    auto mapped = opt_dbl.map([](auto x) -> decltype(x * 2) { return x * 2; });
    if (!mapped.is_some<double>() || mapped.unwrap<double>() != 9.0) return false;

    auto mapped_i = opt_int.map([](auto x) -> decltype(x * 2) { return x * 2; });
    if (!mapped_i.is_some<int>() || mapped_i.unwrap<int>() != 40) return false;

    auto mapped_n = opt_none.map([](auto x) -> decltype(x * 2) { return x * 2; });
    if (mapped_n.is_some()) return false;

    // 2. Targeted map<Target_> (Rule 2)
    auto tgt_dbl = opt_dbl.map<double>([](double d) { return d * 2.0; });
    if (!tgt_dbl.is_some() || tgt_dbl.unwrap() != 9.0) return false;
    auto tgt_none = opt_dbl.map<int>([](int i) { return i * 2; });
    if (!tgt_none.is_none()) return false;

    // 3. map_or
    if (opt_dbl.map_or([](auto x) -> decltype(x * 2.0) { return x * 2.0; }, 0.0) != 9.0) return false;
    if (opt_none.map_or([](auto x) -> decltype(x * 2.0) { return x * 2.0; }, 99.0) != 99.0) return false;
    if (opt_dbl.map_or<double>([](double d) { return d * 2.0; }, 0.0) != 9.0) return false;
    if (opt_dbl.map_or<int>([](int i) { return i * 2; }, 77) != 77) return false;

    // 4. map_or_else
    if (opt_dbl.map_or_else([](auto x) -> decltype(x * 2.0) { return x * 2.0; }, []() { return 0.0; }) != 9.0) return false;
    if (opt_none.map_or_else([](auto x) -> decltype(x * 2.0) { return x * 2.0; }, []() { return 99.0; }) != 99.0) return false;
    if (opt_dbl.map_or_else<double>([](double d) { return d * 2.0; }, []() { return 0.0; }) != 9.0) return false;
    if (opt_dbl.map_or_else<int>([](int i) { return i * 2; }, []() { return 77; }) != 77) return false;

    // 5. map_or_transform
    if (opt_dbl.map_or_transform([](auto x) -> decltype(x * 2.0) { return x * 2.0; }, 0.0) != 9.0) return false;
    if (opt_none.map_or_transform([](auto x) {
        if constexpr (std::same_as<std::remove_cvref_t<decltype(x)>, rl::details::Opt::None>) {
            return 42.0;
        } else {
            return static_cast<double>(x * 2);
        }
    }, 0.0) != 42.0) return false;
    if (opt_dbl.map_or_transform<double>([](double d) { return d * 2.0; }, 0.0) != 9.0) return false;
    if (opt_dbl.map_or_transform<int>([](int i) { return static_cast<double>(i * 2); }, 77.0) != 77.0) return false;

    // 6. is_some_and & is_none_or
    if (!opt_dbl.is_some_and([](auto x) -> decltype(x > 0) { return x > 0; })) return false;
    if (!opt_dbl.is_some_and<double>([](double d) { return d > 4.0; })) return false;
    if (opt_dbl.is_some_and<int>([](int i) { return i > 0; })) return false;
    if (!opt_none.is_none_or([](auto x) -> decltype(x > 0) { return x > 0; })) return false;
    if (!opt_dbl.is_none_or([](auto x) -> decltype(x > 0) { return x > 0; })) return false;
    if (opt_dbl.is_none_or([](auto x) -> decltype(x < 0) { return x < 0; })) return false;
    if (!opt_dbl.is_none_or<double>([](double d) { return d > 4.0; })) return false;
    if (opt_dbl.is_none_or<double>([](double d) { return d < 0.0; })) return false;

    // 7. and_then
    auto chained = opt_dbl.and_then([](auto x) -> rl::Option<std::remove_cvref_t<decltype(x)>> {
        return rl::Some(x + 1);
    });
    if (!chained.is_some<double>() || chained.unwrap<double>() != 5.5) return false;

    auto chained_tgt = opt_dbl.and_then<double>([](double d) -> rl::Option<double> {
        return rl::Some(d + 10.0);
    });
    if (!chained_tgt.is_some() || chained_tgt.unwrap() != 14.5) return false;
    auto chained_tgt_none = opt_dbl.and_then<int>([](int i) -> rl::Option<int> {
        return rl::Some(i + 1);
    });
    if (!chained_tgt_none.is_none()) return false;

    // 8. Rule 1: Single-type initialization
    rl::Option<double> single_dbl = rl::Some(3.14);
    auto single_mapped = single_dbl.map([](double d) { return d * 2.0; });
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    if (!single_mapped.is_some() || single_mapped.unwrap() != 6.28) return false;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    return true;
}

// C++26 User-generated static_assert message (P2741R3)
static_assert(test_constexpr_active_mapping(),
              std::string_view("Constexpr active mapping & combinators must pass in C++26"));

// =============================================================================
// Metaprogramming SFINAE / Concept Checks for Rules 1, 2, and 3
// =============================================================================
struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(NonCopyable &&) = default;
    NonCopyable(const NonCopyable &) = delete;
};

template <typename O, typename F>
concept CanMap = requires(O o, F f) { o.map(f); };

template <typename Target_, typename O, typename F>
concept CanTargetedMap = requires(O o, F f) { o.template map<Target_>(f); };

template <typename O, typename F>
concept CanAndThen = requires(O o, F f) { o.and_then(f); };

template <typename Target_, typename O, typename F>
concept CanTargetedAndThen = requires(O o, F f) { o.template and_then<Target_>(f); };

// Materializability rejection
static_assert(!CanMap<const rl::Option<int> &, NonCopyable &(*)(const int &)>,
              std::string_view("map() must reject callback returning non-copyable lvalue ref"));
static_assert(!CanAndThen<const rl::Option<int> &, rl::Option<NonCopyable> &(*)(const int &)>,
              std::string_view("and_then() must reject callback returning non-copyable Option lvalue ref"));

// Rule 3: Rejection of untargeted map when alternatives have heterogeneous behavior
using HeteroOption = rl::Option<std::string, int, float>;
using HomoOption = rl::Option<int, float>;
using SingleOption = rl::Option<float>;

using Mul2Lambda = decltype([](auto x) -> decltype(x * 2) { return x * 2; });

static_assert(CanMap<SingleOption &, Mul2Lambda>,
              std::string_view("Rule 1: Single-type Option allows untargeted map"));
static_assert(CanMap<HomoOption &, Mul2Lambda>,
              std::string_view("Homogeneous multi-type Option allows untargeted map"));
static_assert(!CanMap<HeteroOption &, Mul2Lambda>,
              std::string_view("Rule 3: Heterogeneous multi-type Option REJECTS untargeted map"));

// Rule 2: Targeted mapping is always available for valid alternatives
static_assert(CanTargetedMap<float, HeteroOption &, Mul2Lambda>,
              std::string_view("Rule 2: Targeted map<float> allowed on heterogeneous Option"));
static_assert(CanTargetedMap<int, HeteroOption &, Mul2Lambda>,
              std::string_view("Rule 2: Targeted map<int> allowed on heterogeneous Option"));

int main() {
    // Rule 1: Single-type stack initialization infers active type directly
    {
        rl::Option a = rl::Some(4.99);
        static_assert(std::same_as<decltype(a), rl::Option<double>>);
        auto l1 = a.map([](auto y) { return y * 2.0; });
        static_assert(std::same_as<decltype(l1), rl::Option<double>>);
        assert(l1.is_some());
        assert(std::abs(l1.unwrap() - 9.98) < 0.001);

        auto l1_at = a.and_then([](double d) -> rl::Option<double> {
            return rl::Some(d * 3.0);
        });
        static_assert(std::same_as<decltype(l1_at), rl::Option<double>>);
        assert(l1_at.is_some());
        assert(std::abs(l1_at.unwrap() - 14.97) < 0.001);
    }

    // Rule 2: Multi-alternative Option enforces targeted map<Target_>() returning Option<Res>
    {
        rl::Option<std::string, int, float> ts = rl::Some<float>(4.99f);
        auto s = ts.map<float>([](auto x) { return x * 2.0f; });
        static_assert(std::same_as<decltype(s), rl::Option<float>>);
        assert(s.is_some());
        assert(std::abs(s.unwrap() - 9.98f) < 0.001f);

        // Targeted on non-active alternative yields None of type Option<float>
        auto s_int = ts.map<int>([](int x) { return static_cast<float>(x * 2); });
        static_assert(std::same_as<decltype(s_int), rl::Option<float>>);
        assert(s_int.is_none());

        // Targeted and_then preserves Option container
        auto s_at = ts.and_then<float>([](float f) -> rl::Option<float> {
            return rl::Some(f * 2.0f);
        });
        static_assert(std::same_as<decltype(s_at), rl::Option<float>>);
        assert(s_at.is_some());
        assert(std::abs(s_at.unwrap() - 9.98f) < 0.001f);
    }

    // Rule 3: Heterogeneous alternatives require targeted mapping or match()
    {
        rl::Option<std::string, int, float> ts_str = rl::Some(std::string("hello"));
        auto matched = ts_str.match(
            rl::Some(std::string("hello")) >>= [](const std::string &s) { return s + " world"; },
            rl::SomeAny                    >>= []() { return std::string("other"); },
            rl::None                       >>= []() { return std::string("none"); }
        );
        assert(matched.is_some());
        assert(matched.unwrap() == "hello world");

        // or_else preserves container
        rl::Option<std::string, int, float> none_opt = rl::None;
        auto recovered = none_opt.or_else([]() -> rl::Option<std::string, int, float> {
            return rl::Some<int>(42);
        });
        assert(recovered.is_some<int>());
        assert(recovered.unwrap<int>() == 42);
    }

    // Homogeneous multi-type Option untargeted map
    {
        rl::Option<int, float> ts = rl::Some<float>(4.99f);
        auto s = ts.map([](auto x) -> decltype(x * 2) { return x * 2; });
        static_assert(std::same_as<decltype(s), rl::Option<int, float>>);
        assert(s.is_some<float>());
        assert(std::abs(s.unwrap<float>() - 9.98f) < 0.001f);
    }

    // Rvalue move consumption
    {
        struct MoveTracker {
            bool moved = false;
            constexpr MoveTracker() = default;
            constexpr MoveTracker(MoveTracker const &) = default;
            constexpr MoveTracker(MoveTracker &&o) noexcept : moved(false) { o.moved = true; }
        };

        rl::Option<std::string, MoveTracker> opt = rl::Some<MoveTracker>(MoveTracker{});
        auto res = std::move(opt).map<MoveTracker>([](MoveTracker &&m) {
            return m.moved;
        });
        assert(res.is_some());
        assert(opt.is_none());
    }

    // map_or & map_or_else
    {
        rl::Option<std::string, int, float> ts = rl::Some<float>(5.0f);
        auto val = ts.map_or<float>([](float x) { return x * 3.0f; }, 0.0f);
        assert(val == 15.0f);
        auto val_none = ts.map_or<int>([](int x) { return static_cast<float>(x * 2); }, 77.0f);
        assert(val_none == 77.0f);

        auto else_val = ts.map_or_else<float>([](float x) { return x * 2.0f; }, []() { return 0.0f; });
        assert(else_val == 10.0f);
    }

    // map_or_transform
    {
        rl::Option<std::string, int, float> ts = rl::Some<float>(5.0f);
        auto targeted_val = ts.map_or_transform<float>([](float x) { return x * 3.0f; }, 0.0f);
        assert(targeted_val == 15.0f);
        auto targeted_fallback = ts.map_or_transform<int>([](int x) { return static_cast<float>(x * 2); }, 77.0f);
        assert(targeted_fallback == 77.0f);
    }

    // is_some_and & is_none_or
    {
        rl::Option<std::string, int, float> ts = rl::Some<float>(5.0f);
        assert(ts.is_some_and<float>([](float x) { return x == 5.0f; }));
        assert(!ts.is_some_and<float>([](float x) { return x == 0.0f; }));
        assert(!ts.is_some_and<int>([](int) { return true; }));

        rl::Option<std::string, int, float> tn = rl::None;
        assert(ts.is_none_or<float>([](float x) { return x == 5.0f; }));
        assert(!ts.is_none_or<float>([](float x) { return x == 0.0f; }));
        assert(tn.is_none_or<float>([](float) { return false; }));
    }

    // C++26 placeholder variables '_' (P2169R4)
    {
        rl::Option<int, double> opt = rl::Some<int>(42);
        [[maybe_unused]] auto _ = opt.map([](auto x) -> decltype(x * 2) { return x * 2; });
        [[maybe_unused]] auto _ = opt.and_then([](auto x) -> rl::Option<decltype(x)> { return rl::Some(x); });
    }

    return 0;
}
