#include "rl_option.hpp"

#include <cassert>
#include <compare>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

struct MoveConstructibleSwappable {
    int value{};

    explicit constexpr MoveConstructibleSwappable(int v) noexcept : value(v) {}
    constexpr MoveConstructibleSwappable(MoveConstructibleSwappable &&) noexcept = default;
    MoveConstructibleSwappable(const MoveConstructibleSwappable &) = delete;
    MoveConstructibleSwappable &operator=(MoveConstructibleSwappable &&) = delete;
    MoveConstructibleSwappable &operator=(const MoveConstructibleSwappable &) = delete;

    friend constexpr void swap(MoveConstructibleSwappable &lhs,
                               MoveConstructibleSwappable &rhs) noexcept {
        const int tmp = lhs.value;
        lhs.value = rhs.value;
        rhs.value = tmp;
    }
};

struct NoHash {
    int value{};
    friend constexpr bool operator==(const NoHash &, const NoHash &) = default;
};


int main() {
    // B: swap only requires move construction and swappability, not move assignment.
    static_assert(std::is_move_constructible_v<MoveConstructibleSwappable>);
    static_assert(!std::is_move_assignable_v<MoveConstructibleSwappable>);
    static_assert(std::swappable<MoveConstructibleSwappable>);
    using SwapOption = rl::Option<MoveConstructibleSwappable, int>;
    static_assert(requires(SwapOption &lhs, SwapOption &rhs) { lhs.swap(rhs); });
    static_assert(std::swappable<SwapOption>);
    static_assert(noexcept(std::declval<SwapOption &>().swap(
        std::declval<SwapOption &>())));

    SwapOption same_lhs{rl::Some{MoveConstructibleSwappable{1}}};
    SwapOption same_rhs{rl::Some{MoveConstructibleSwappable{2}}};
    same_lhs.swap(same_rhs);
    assert(same_lhs.unwrap<MoveConstructibleSwappable>().value == 2);
    assert(same_rhs.unwrap<MoveConstructibleSwappable>().value == 1);

    // Different active types exercise the move-construction-only exchange path.
    SwapOption cross_lhs{rl::Some{MoveConstructibleSwappable{3}}};
    SwapOption cross_rhs{rl::Some{9}};
    cross_lhs.swap(cross_rhs);
    assert(cross_lhs.unwrap<int>() == 9);
    assert(cross_rhs.unwrap<MoveConstructibleSwappable>().value == 3);

    SwapOption empty{rl::None};
    empty.swap(cross_rhs);
    assert(empty.unwrap<MoveConstructibleSwappable>().value == 3);
    assert(cross_rhs.is_none());

    // C: collect each handler's result for every alternative it can match.
    using HeterogeneousOption = rl::Option<int, std::string>;
    HeterogeneousOption int_value{rl::Some{4}};
    auto int_result = int_value.match(
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](const auto &value) {
            if constexpr (std::same_as<std::remove_cvref_t<decltype(value)>, int>) {
                return 40;
            } else {
                return std::string{value};
            }
        });
    static_assert(std::same_as<decltype(int_result),
                               rl::Option<int, std::string>>);
    assert(int_result.unwrap<int>() == 40);

    HeterogeneousOption string_value{rl::Some{std::string{"forty"}}};
    auto string_result = string_value.match(
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](const auto &value) {
            if constexpr (std::same_as<std::remove_cvref_t<decltype(value)>, int>) {
                return 40;
            } else {
                return std::string{value};
            }
        });
    static_assert(std::same_as<decltype(string_result),
                               rl::Option<int, std::string>>);
    assert(string_result.unwrap<std::string>() == "forty");

    // A predicate arm contributes only the result for types its predicate accepts.
    auto filtered_result = string_value.match(
        rl::None >>= [] { return 0; },
        [](const int &) { return true; } >>= [](int value) { return value + 1; },
        [](const std::string &) { return true; } >>=
            [](const std::string &value) { return value.size(); });
    static_assert(std::same_as<decltype(filtered_result),
                               rl::Option<std::size_t>>);
    assert(filtered_result.unwrap<std::size_t>() == 5);

    // An unconditional catch-all also cuts off later arms during return deduction.
    rl::Option<int> single_int{rl::Some{4}};
    auto catch_all_result = single_int.match(
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](int value) { return value + 1; },
        rl::Where([](const int &) { return true; }) >>=
            [](int) { return std::string{"unreachable"}; });
    static_assert(std::same_as<decltype(catch_all_result), rl::Option<int>>);
    assert(catch_all_result.unwrap<int>() == 5);

    // E: multi-alternative ordering uses alternative index, then contained value.
    using CompareOption = rl::Option<int, double>;
    static_assert(std::same_as<decltype(std::declval<const CompareOption &>() <=>
                                        std::declval<const CompareOption &>()),
                               std::partial_ordering>);
    constexpr auto constexpr_compare = [] {
        const CompareOption lhs{rl::Some{3}};
        const CompareOption rhs{rl::Some{4}};
        return (lhs <=> rhs) == std::partial_ordering::less;
    };
    static_assert(constexpr_compare());

    using StrongCompareOption = rl::Option<int, std::string>;
    static_assert(std::same_as<decltype(std::declval<const StrongCompareOption &>() <=>
                                        std::declval<const StrongCompareOption &>()),
                               std::strong_ordering>);

    const CompareOption none{rl::None};
    const CompareOption int_low{rl::Some{2}};
    const CompareOption int_high{rl::Some{8}};
    const CompareOption double_low{rl::Some{1.0}};
    assert((none <=> int_low) == std::partial_ordering::less);
    assert((int_low <=> int_high) == std::partial_ordering::less);
    assert((int_high <=> double_low) == std::partial_ordering::less);

    const CompareOption nan_a{
        rl::Some{std::numeric_limits<double>::quiet_NaN()}};
    const CompareOption nan_b{rl::Some{1.0}};
    assert((nan_a <=> nan_b) == std::partial_ordering::unordered);

    // F: hashing is enabled when all alternatives are hashable, and uses the active tag.
    using HashOption = rl::Option<int, std::string>;
    static_assert(std::is_default_constructible_v<std::hash<HashOption>>);
    const std::hash<HashOption> hash_option{};
    const HashOption hash_none{rl::None};
    const HashOption hash_int_a{rl::Some{42}};
    const HashOption hash_int_b{rl::Some{42}};
    const HashOption hash_string{rl::Some{std::string{"42"}}};
    assert(hash_option(hash_none) == hash_option(HashOption{rl::None}));
    assert(hash_option(hash_int_a) == hash_option(hash_int_b));
    (void)hash_option(hash_string);

    using UnhashableOption = rl::Option<NoHash, int>;
    static_assert(!std::is_default_constructible_v<std::hash<UnhashableOption>>);
}
