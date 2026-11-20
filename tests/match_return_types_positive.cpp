#include "rl_option.hpp"
#include <concepts>
#include <type_traits>

namespace {
struct TrueResult { int value; };
struct FalseResult { int value; };
struct NoneResult { int value; };

consteval bool check_distinct_results() {
    rl::Option<bool> yes{rl::Some(true)};
    rl::Option<bool> no{rl::Some(false)};
    rl::Option<bool> empty{rl::None};
    const auto result = yes.match(
        rl::True >>= [] constexpr { return TrueResult{1}; },
        rl::False >>= [] constexpr { return FalseResult{2}; },
        rl::None >>= [] constexpr { return NoneResult{3}; });
    static_assert(std::same_as<std::remove_cv_t<decltype(result)>,
                  rl::Option<NoneResult, TrueResult, FalseResult>>);
    const auto no_result = no.match(
        rl::True >>= [] constexpr { return TrueResult{1}; },
        rl::False >>= [] constexpr { return FalseResult{2}; },
        rl::None >>= [] constexpr { return NoneResult{3}; });
    const auto empty_result = empty.match(
        rl::True >>= [] constexpr { return TrueResult{1}; },
        rl::False >>= [] constexpr { return FalseResult{2}; },
        rl::None >>= [] constexpr { return NoneResult{3}; });
    return result.unwrap<TrueResult>().value == 1 &&
           no_result.unwrap<FalseResult>().value == 2 &&
           empty_result.unwrap<NoneResult>().value == 3;
}
static_assert(check_distinct_results());

consteval bool check_common_type_results() {
    rl::Option<bool> yes{rl::Some(true)};
    const auto result = yes.match(
        rl::True >>= [] constexpr { return short{4}; },
        rl::False >>= [] constexpr { return 5.5F; },
        rl::None >>= [] constexpr { return 0; });
    static_assert(std::same_as<std::remove_cv_t<decltype(result)>, rl::Option<float>>);
    return result.unwrap() == 4.0F;
}
static_assert(check_common_type_results());

consteval bool check_void_results() {
    rl::Option<bool> yes{rl::Some(true)};
    int calls = 0;
    const auto true_handler = [&calls] constexpr { ++calls; };
    const auto false_handler = [&calls] constexpr { calls += 10; };
    const auto none_handler = [&calls] constexpr { calls += 100; };
    static_assert(std::same_as<decltype(yes.match(
        rl::True >>= true_handler, rl::False >>= false_handler,
        rl::None >>= none_handler)), void>);
    yes.match(rl::True >>= true_handler, rl::False >>= false_handler,
              rl::None >>= none_handler);
    return calls == 1;
}
static_assert(check_void_results());

consteval bool check_explicit_void_mixed_results() {
    rl::Option<bool> yes{rl::Some(true)};
    int calls = 0;
    yes.match<void>(
        rl::True >>= [&calls] constexpr { ++calls; },
        rl::False >>= [] constexpr { return 7; },
        rl::None >>= [] constexpr { return 8; });
    return calls == 1;
}
static_assert(check_explicit_void_mixed_results());

consteval bool check_explicit_value_result_types() {
    rl::Option<bool> yes{rl::Some(true)};
    const auto bool_result = yes.match<int>(
        rl::True >>= [] constexpr { return 4; },
        rl::False >>= [] constexpr { return 5; },
        rl::None >>= [] constexpr { return 6; });
    static_assert(std::same_as<std::remove_cv_t<decltype(bool_result)>, rl::Option<int>>);

    rl::Option<int, double> real{rl::Some<double>(2.5)};
    const auto multi_result = real.match<int, long>(
        rl::SomeAny >>= []<typename T_>(T_ value) constexpr {
            if constexpr (std::same_as<T_, int>) {
                return static_cast<int>(value);
            } else {
                return static_cast<long>(value);
            }
        },
        rl::None >>= [] constexpr { return 0; });
    static_assert(
        std::same_as<std::remove_cv_t<decltype(multi_result)>, rl::Option<int, long>>);
    return bool_result.unwrap() == 4 &&
           multi_result.unwrap<long>() == 2L;
}
static_assert(check_explicit_value_result_types());
}
int main() {}
