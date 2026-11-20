// Compile-time regression suite for the supplied rl::Option C++26 header.
// Intended invocation: clang++ -std=c++2c -stdlib=libstdc++ -fsyntax-only RLOption_tests.cpp
#include "rl_option.hpp"

#include <concepts>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace tests {

using rl::Option;
using rl::Some;
using rl::None;

struct MapAllDifferent {
    constexpr long operator()(int value) const noexcept { return value + 10L; }
    constexpr float operator()(double value) const noexcept {
        return static_cast<float>(value * 2.0);
    }
};

struct MapAllSame {
    constexpr int operator()(int value) const noexcept { return value + 1; }
    constexpr int operator()(double value) const noexcept {
        return static_cast<int>(value) + 2;
    }
};

struct MapOnlyInt {
    constexpr long operator()(int value) const noexcept { return value * 3L; }
    long operator()(double) const = delete;
};

struct MapOnlyFloat {
    constexpr float operator()(float value) const noexcept { return value * 2.0F; }
    float operator()(const std::string &) const = delete;
    float operator()(int) const = delete;
};

struct MapOrAll {
    constexpr int operator()(int value) const noexcept { return value * 2; }
    constexpr int operator()(double value) const noexcept {
        return static_cast<int>(value * 2.0);
    }
};

struct MapOrAllWithNone : MapOrAll {
    using MapOrAll::operator();
    constexpr int operator()(rl::None_t) const noexcept { return 99; }
};

struct NoneOnlyMapOrTransform {
    constexpr int operator()(rl::None_t) const noexcept { return 99; }
    int operator()(int) const = delete;
    int operator()(double) const = delete;
};

struct MapLvalueOnly {
    constexpr int operator()(int &) & noexcept { return 1; }
    int operator()(int &) && = delete;
};

struct VoidMap {
    constexpr void operator()(int) const noexcept {}
};

struct VoidChain {
    constexpr void operator()(int) const noexcept {}
};

template <typename O, typename F>
concept CanMapOrTransform = requires(O &option, F &&fn) {
    option.map_or_transform(std::forward<F>(fn), 0);
};

template <typename O, typename F>
concept CanUntargetedMap = requires(O &option, F &&fn) {
    option.map(std::forward<F>(fn));
};

template <typename O>
concept CanIncompleteBoolMatch = requires(O &option) {
    option.match(rl::True >>= [](bool value) { return value ? 1 : 0; },
                 rl::None >>= [] { return 0; });
};

template <typename O>
concept CanCompleteBoolMatch = requires(O &option) {
    option.match(rl::True >>= [](bool value) { return value ? 1 : 0; },
                 rl::False >>= [](bool value) { return value ? 1 : 0; },
                 rl::None >>= [] { return 0; });
};

template <typename O>
concept CanWildcardMatch = requires(O &option) {
    option.match(rl::SomeAny >>= [](auto &&) { return 1; },
                 rl::None >>= [] { return 0; });
};

static_assert(!CanIncompleteBoolMatch<Option<bool>>);
static_assert(CanCompleteBoolMatch<Option<bool>>);
static_assert(CanWildcardMatch<Option<int, double>>);

template <typename O, typename F>
concept CanUntargetedAndThen = requires(O &option, F &&fn) {
    option.and_then(std::forward<F>(fn));
};

template <typename O, typename F>
concept CanTargetMapInt = requires(O &option, F &&fn) {
    option.template map<int>(std::forward<F>(fn));
};

template <typename O, typename F>
concept CanTargetMapFloat = requires(O &option, F &&fn) {
    option.template map<float>(std::forward<F>(fn));
};

static_assert(CanTargetMapInt<Option<int, double>, MapOnlyInt>);
static_assert(CanTargetMapFloat<Option<std::string, int, float>, MapOnlyFloat>);
static_assert(!CanUntargetedMap<Option<std::string, int, float>, MapOnlyFloat>);
static_assert(!CanUntargetedMap<Option<int, double>, MapOnlyInt>,
              "multi-alternative untargeted map must require every alternative");
static_assert(!CanUntargetedMap<Option<int>, MapLvalueOnly>,
              "a temporary lvalue-only callable must be rejected");
static_assert(!CanMapOrTransform<Option<int, double>, NoneOnlyMapOrTransform>,
              "a None-only callback must not bypass active-alternative checks");
static_assert(!CanUntargetedMap<Option<int>, VoidMap>,
              "map must reject void callback results");
static_assert(!CanUntargetedAndThen<Option<int>, VoidChain>,
              "and_then must reject non-Option callback results");

constexpr bool test_ctad_single_type_map() {
    Option deduced = Some(21);
    static_assert(std::same_as<decltype(deduced), Option<int>>);
    auto result = deduced.map([](int value) constexpr { return value * 2; });
    static_assert(std::same_as<decltype(result), Option<int>>);
    return result.unwrap() == 42;
}
static_assert(test_ctad_single_type_map());

constexpr bool test_user_multialternative_sample_with_target() {
    Option<std::string, int, float> source{Some<float>(4.99F)};
    auto result = source.map<float>([](float value) constexpr {
        return value * 2.0F;
    });
    static_assert(std::same_as<decltype(result), Option<float>>);
    return result.unwrap() > 9.97F && result.unwrap() < 9.99F;
}
static_assert(test_user_multialternative_sample_with_target());

constexpr bool test_targeted_map_active() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.map<double>([](double value) constexpr {
        return value * 2.0;
    });
    static_assert(std::same_as<decltype(result), Option<double>>);
    return result.unwrap() == 9.0;
}
static_assert(test_targeted_map_active());

constexpr bool test_targeted_map_inactive_returns_none() {
    Option<int, double> source{Some<int>(7)};
    auto result = source.map<double>([](double value) constexpr {
        return value * 2.0;
    });
    return result.is_none();
}
static_assert(test_targeted_map_inactive_returns_none());

constexpr bool test_targeted_map_none_returns_none() {
    Option<int, double> source{None};
    int calls = 0;
    auto result = source.map<double>([&](double value) constexpr {
        ++calls;
        return value * 2.0;
    });
    return result.is_none() && calls == 0;
}
static_assert(test_targeted_map_none_returns_none());

constexpr bool test_untargeted_map_dispatches_every_alternative() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.map(MapAllDifferent{});
    static_assert(std::same_as<decltype(result), Option<long, float>>);
    return result.unwrap<float>() == 9.0F;
}
static_assert(test_untargeted_map_dispatches_every_alternative());

constexpr bool test_untargeted_map_deduplicates_results() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.map(MapAllSame{});
    static_assert(std::same_as<decltype(result), Option<int>>);
    return result.unwrap() == 6;
}
static_assert(test_untargeted_map_deduplicates_results());

constexpr bool test_untargeted_map_none_does_not_call() {
    Option<int, double> source{None};
    int calls = 0;
    auto result = source.map([&](auto value) constexpr {
        ++calls;
        return value;
    });
    return result.is_none() && calls == 0;
}
static_assert(test_untargeted_map_none_does_not_call());

constexpr bool test_ctad_single_type_and_then() {
    Option deduced = Some(21);
    auto result = deduced.and_then([](int value) constexpr {
        return Option<long>{Some<long>(static_cast<long>(value * 2))};
    });
    static_assert(std::same_as<decltype(result), Option<long>>);
    return result.unwrap() == 42L;
}
static_assert(test_ctad_single_type_and_then());

constexpr bool test_targeted_and_then_active() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.and_then<double>([](double value) constexpr {
        return Option<long>{Some<long>(static_cast<long>(value * 2.0))};
    });
    static_assert(std::same_as<decltype(result), Option<long>>);
    return result.unwrap() == 9L;
}
static_assert(test_targeted_and_then_active());

constexpr bool test_targeted_and_then_inactive() {
    Option<int, double> source{Some<int>(4)};
    auto result = source.and_then<double>([](double value) constexpr {
        return Option<long>{Some<long>(static_cast<long>(value))};
    });
    return result.is_none();
}
static_assert(test_targeted_and_then_inactive());

constexpr bool test_untargeted_and_then_merges_result_alternatives() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.and_then([](auto value) constexpr {
        using T = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::same_as<T, int>) {
            return Option<long>{Some<long>(static_cast<long>(value))};
        } else {
            return Option<float>{Some<float>(static_cast<float>(value))};
        }
    });
    static_assert(std::same_as<decltype(result), Option<long, float>>);
    return result.unwrap<float>() == 4.5F;
}
static_assert(test_untargeted_and_then_merges_result_alternatives());

constexpr bool test_untargeted_and_then_deduplicates_inner_types() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = source.and_then([](auto value) constexpr {
        return Option<long>{Some<long>(static_cast<long>(value))};
    });
    static_assert(std::same_as<decltype(result), Option<long>>);
    return result.unwrap() == 4L;
}
static_assert(test_untargeted_and_then_deduplicates_inner_types());

constexpr bool test_map_or_family() {
    Option<int, double> source{Some<double>(4.5)};
    const auto a = source.map_or(MapOrAll{}, -1);
    const auto b = source.map_or_else(MapOrAll{}, []() constexpr { return -1; });
    const auto c = source.map_or_transform(MapOrAll{}, -1);
    return a == 9 && b == 9 && c == 9;
}
static_assert(test_map_or_family());

constexpr bool test_map_or_transform_optional_none_handler() {
    Option<int, double> source{None};
    const auto with_handler = source.map_or_transform(MapOrAllWithNone{}, -1);
    const auto without_handler = source.map_or_transform(MapOrAll{}, -1);
    return with_handler == 99 && without_handler == -1;
}
static_assert(test_map_or_transform_optional_none_handler());

struct FallbackRefQualified {
    constexpr short operator()() & noexcept { return 1; }
    constexpr float operator()() && noexcept { return 3.5F; }
};

constexpr bool test_map_or_else_uses_actual_fallback_category() {
    Option<int> source{None};
    auto result = source.map_or_else(
        [](int value) constexpr { return value * 2; }, FallbackRefQualified{});
    static_assert(std::same_as<decltype(result), float>);
    return result == 3.5F;
}
static_assert(test_map_or_else_uses_actual_fallback_category());

constexpr bool test_map_or_else_preserves_lvalue_fallback_category() {
    Option<int> source{None};
    FallbackRefQualified fallback{};
    auto result = source.map_or_else(
        [](int value) constexpr { return value * 2; }, fallback);
    static_assert(std::same_as<decltype(result), short>);
    return result == 1;
}
static_assert(test_map_or_else_preserves_lvalue_fallback_category());

constexpr bool test_targeted_map_or_family_uses_fallback_for_mismatch() {
    Option<int, double> source{Some<int>(4)};
    const auto a = source.map_or<double>([](double value) constexpr {
        return static_cast<int>(value * 2);
    }, -7);
    const auto b = source.map_or_else<double>([](double value) constexpr {
        return static_cast<int>(value * 2);
    }, []() constexpr { return -7; });
    const auto c = source.map_or_transform<double>([](double value) constexpr {
        return static_cast<int>(value * 2);
    }, -7);
    return a == -7 && b == -7 && c == -7;
}
static_assert(test_targeted_map_or_family_uses_fallback_for_mismatch());

struct RefQualifiedMap {
    constexpr short operator()(int &) & noexcept { return 1; }
    constexpr float operator()(int &) && noexcept { return 2.75F; }
};

// Regression: the callback is a temporary, so operator() && must determine
// both the call and the inferred mapped alternative.
constexpr bool test_map_uses_actual_callable_value_category() {
    Option<int> source{Some(10)};
    auto result = source.map(RefQualifiedMap{});
    static_assert(std::same_as<decltype(result), Option<float>>,
                  "map must infer from operator() && for a temporary callable");
    return result.unwrap() == 2.75F;
}
static_assert(test_map_uses_actual_callable_value_category());

constexpr bool test_map_preserves_lvalue_callable_category() {
    Option<int> source{Some(10)};
    RefQualifiedMap callable{};
    auto result = source.map(callable);
    static_assert(std::same_as<decltype(result), Option<short>>);
    return result.unwrap() == 1;
}
static_assert(test_map_preserves_lvalue_callable_category());

struct RefQualifiedChain {
    constexpr auto operator()(int &) & noexcept -> Option<short> {
        return Option<short>{Some<short>(1)};
    }
    constexpr auto operator()(int &) && noexcept -> Option<float> {
        return Option<float>{Some<float>(2.75F)};
    }
};

constexpr bool test_and_then_uses_actual_callable_value_category() {
    Option<int> source{Some(10)};
    auto result = source.and_then(RefQualifiedChain{});
    static_assert(std::same_as<decltype(result), Option<float>>,
                  "and_then must infer from operator() && for a temporary callable");
    return result.unwrap() == 2.75F;
}
static_assert(test_and_then_uses_actual_callable_value_category());

constexpr bool test_and_then_preserves_lvalue_callable_category() {
    Option<int> source{Some(10)};
    RefQualifiedChain callable{};
    auto result = source.and_then(callable);
    static_assert(std::same_as<decltype(result), Option<short>>);
    return result.unwrap() == 1;
}
static_assert(test_and_then_preserves_lvalue_callable_category());

constexpr bool test_rvalue_map_consumes_only_after_successful_mapping() {
    Option<int> source{Some(10)};
    auto result = std::move(source).map([](int &&value) constexpr {
        return value + 5;
    });
    return result.unwrap() == 15 && source.is_none();
}
static_assert(test_rvalue_map_consumes_only_after_successful_mapping());

constexpr bool test_rvalue_targeted_map_consumes_non_primary_alternative() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = std::move(source).map<double>([](double &&value) constexpr {
        return value * 2.0;
    });
    return result.unwrap() == 9.0 && source.is_none();
}
static_assert(test_rvalue_targeted_map_consumes_non_primary_alternative());

constexpr bool test_rvalue_targeted_and_then_consumes_non_primary_alternative() {
    Option<int, double> source{Some<double>(4.5)};
    auto result = std::move(source).and_then<double>([](double &&value) constexpr {
        return Option<long>{Some<long>(static_cast<long>(value * 2.0))};
    });
    return result.unwrap() == 9L && source.is_none();
}
static_assert(test_rvalue_targeted_and_then_consumes_non_primary_alternative());

constexpr bool test_rvalue_target_mismatch_preserves_source() {
    Option<int, double> source{Some<int>(7)};
    auto result = std::move(source).map<double>([](double value) constexpr {
        return value * 2.0;
    });
    return result.is_none() && source.is_some() && source.unwrap() == 7;
}
static_assert(test_rvalue_target_mismatch_preserves_source());

constexpr bool test_targeted_map_unsupported_active_alternative_is_none() {
    Option<std::string, int, float> source{Some<std::string>("not a float")};
    int calls = 0;
    auto result = source.map<float>([&](float value) constexpr {
        ++calls;
        return value * 2.0F;
    });
    return result.is_none() && calls == 0 && source.is_some();
}
static_assert(test_targeted_map_unsupported_active_alternative_is_none());

constexpr bool test_predicates_are_exhaustive() {
    Option<int, double> source{Some<double>(4.5)};
    return source.is_some_and([](auto value) constexpr { return value > 0; }) &&
           source.is_none_or([](auto value) constexpr { return value > 0; });
}
static_assert(test_predicates_are_exhaustive());

struct SourceCategoryMap {
    constexpr int operator()(int &) const noexcept { return 1; }
    constexpr long operator()(const int &) const noexcept { return 2L; }
    constexpr float operator()(int &&) const noexcept { return 3.0F; }
    constexpr double operator()(const int &&) const noexcept { return 4.0; }
};

constexpr bool test_map_preserves_all_source_cvref_categories() {
    Option<int> mutable_lvalue{Some(10)};
    const Option<int> const_lvalue{Some(10)};
    Option<int> mutable_rvalue{Some(10)};
    const Option<int> const_rvalue{Some(10)};

    auto a = mutable_lvalue.map(SourceCategoryMap{});
    auto b = const_lvalue.map(SourceCategoryMap{});
    auto c = std::move(mutable_rvalue).map(SourceCategoryMap{});
    auto d = std::move(const_rvalue).map(SourceCategoryMap{});

    static_assert(std::same_as<decltype(a), Option<int>>);
    static_assert(std::same_as<decltype(b), Option<long>>);
    static_assert(std::same_as<decltype(c), Option<float>>);
    static_assert(std::same_as<decltype(d), Option<double>>);

    return a.unwrap() == 1 && b.unwrap() == 2L && c.unwrap() == 3.0F &&
           d.unwrap() == 4.0;
}
static_assert(test_map_preserves_all_source_cvref_categories());

constexpr bool test_and_then_none_does_not_call() {
    Option<int> source{None};
    int calls = 0;
    auto result = source.and_then([&](int value) constexpr {
        ++calls;
        return Option<long>{Some<long>(static_cast<long>(value))};
    });
    return result.is_none() && calls == 0;
}
static_assert(test_and_then_none_does_not_call());

constexpr bool test_or_else_preserves_engaged_non_primary_alternative() {
    Option<int, double> source{Some<double>(4.5)};
    int fallback_calls = 0;
    auto result = source.or_else([&]() constexpr {
        ++fallback_calls;
        return Option<int, double>{Some<int>(-1)};
    });
    return result.unwrap<double>() == 4.5 && fallback_calls == 0;
}
static_assert(test_or_else_preserves_engaged_non_primary_alternative());

constexpr bool test_or_else_fallback_only_runs_for_none() {
    Option<int, double> source{None};
    int fallback_calls = 0;
    auto result = source.or_else([&]() constexpr {
        ++fallback_calls;
        return Option<int, double>{Some<double>(6.5)};
    });
    return result.unwrap<double>() == 6.5 && fallback_calls == 1;
}
static_assert(test_or_else_fallback_only_runs_for_none());

constexpr bool test_targeted_predicates() {
    Option<int, double> source{Some<double>(4.5)};
    return source.is_some_and<double>([](double value) constexpr {
               return value > 4.0;
           }) &&
           !source.is_some_and<int>([](int value) constexpr { return value > 0; }) &&
           source.is_none_or<int>([](int value) constexpr { return value < 0; });
}
static_assert(test_targeted_predicates());

struct ThrowingMoveResult {
    bool throw_on_move{true};
    ThrowingMoveResult() = default;
    ThrowingMoveResult(const ThrowingMoveResult &) = delete;
    ThrowingMoveResult(ThrowingMoveResult &&other) {
        if (other.throw_on_move) {
            throw 2;
        }
    }
};

constexpr bool test_runtime_throwing_callback_preserves_rvalue_source() {
    Option<int> source{Some(10)};
    try {
        (void)std::move(source).map([](int &&) -> int {
            throw 1;
        });
    } catch (int value) {
        return value == 1 && source.is_some();
    }
    return false;
}

bool test_runtime_result_move_throw_occurs_after_source_reset() {
    Option<int> source{Some(10)};
    try {
        (void)std::move(source).map([](int &&) {
            return ThrowingMoveResult{};
        });
    } catch (int value) {
        return value == 2 && source.is_none();
    }
    return false;
}

constexpr bool test_match_bool_true_and_false_runtime_dispatch() {
    Option<bool> yes{Some(true)};
    Option<bool> no{Some(false)};
    Option<bool> empty{None};
    const auto handler = [](bool value) constexpr { return value ? 11 : 22; };
    const auto none_handler = []() constexpr { return 33; };
    return yes.match(rl::True >>= handler, rl::False >>= handler,
                     rl::None >>= none_handler).unwrap() == 11 &&
           no.match(rl::True >>= handler, rl::False >>= handler,
                    rl::None >>= none_handler).unwrap() == 22 &&
           empty.match(rl::True >>= handler, rl::False >>= handler,
                       rl::None >>= none_handler).unwrap() == 33;
}

constexpr bool test_match_someany_runtime_dispatch() {
    Option<int, double> integer{Some(7)};
    Option<int, double> real{Some<double>(2.5)};
    Option<int, double> empty{None};
    const auto handler = [](auto &&value) constexpr {
        return static_cast<int>(value) + 1;
    };
    const auto none_handler = []() constexpr { return -1; };
    return integer.match(rl::SomeAny >>= handler,
                         rl::None >>= none_handler).unwrap() == 8 &&
           real.match(rl::SomeAny >>= handler,
                      rl::None >>= none_handler).unwrap() == 3 &&
           empty.match(rl::SomeAny >>= handler,
                       rl::None >>= none_handler).unwrap() == -1;
}

static_assert(test_match_bool_true_and_false_runtime_dispatch());
static_assert(test_match_someany_runtime_dispatch());

} // namespace tests

int main() {
#define RL_TEST(name) do { if (!tests::name()) return __LINE__; } while (false)
    RL_TEST(test_ctad_single_type_map);
    RL_TEST(test_user_multialternative_sample_with_target);
    RL_TEST(test_targeted_map_active);
    RL_TEST(test_targeted_map_inactive_returns_none);
    RL_TEST(test_targeted_map_none_returns_none);
    RL_TEST(test_untargeted_map_dispatches_every_alternative);
    RL_TEST(test_untargeted_map_deduplicates_results);
    RL_TEST(test_untargeted_map_none_does_not_call);
    RL_TEST(test_ctad_single_type_and_then);
    RL_TEST(test_targeted_and_then_active);
    RL_TEST(test_targeted_and_then_inactive);
    RL_TEST(test_untargeted_and_then_merges_result_alternatives);
    RL_TEST(test_untargeted_and_then_deduplicates_inner_types);
    RL_TEST(test_map_or_family);
    RL_TEST(test_map_or_transform_optional_none_handler);
    RL_TEST(test_map_or_else_uses_actual_fallback_category);
    RL_TEST(test_map_or_else_preserves_lvalue_fallback_category);
    RL_TEST(test_targeted_map_or_family_uses_fallback_for_mismatch);
    RL_TEST(test_map_uses_actual_callable_value_category);
    RL_TEST(test_map_preserves_lvalue_callable_category);
    RL_TEST(test_and_then_uses_actual_callable_value_category);
    RL_TEST(test_and_then_preserves_lvalue_callable_category);
    RL_TEST(test_rvalue_map_consumes_only_after_successful_mapping);
    RL_TEST(test_rvalue_targeted_map_consumes_non_primary_alternative);
    RL_TEST(test_rvalue_targeted_and_then_consumes_non_primary_alternative);
    RL_TEST(test_rvalue_target_mismatch_preserves_source);
    RL_TEST(test_targeted_map_unsupported_active_alternative_is_none);
    RL_TEST(test_predicates_are_exhaustive);
    RL_TEST(test_map_preserves_all_source_cvref_categories);
    RL_TEST(test_and_then_none_does_not_call);
    RL_TEST(test_or_else_preserves_engaged_non_primary_alternative);
    RL_TEST(test_or_else_fallback_only_runs_for_none);
    RL_TEST(test_targeted_predicates);
    RL_TEST(test_runtime_throwing_callback_preserves_rvalue_source);
    RL_TEST(test_runtime_result_move_throw_occurs_after_source_reset);
    RL_TEST(test_match_bool_true_and_false_runtime_dispatch);
    RL_TEST(test_match_someany_runtime_dispatch);
#undef RL_TEST
    return 0;
}
