#include "rl_option.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace {

struct alignas(64) OverAlignedHandler {
    std::byte marker{};
    rl::details::PadToAlignment_<sizeof(marker), 64> padding{};

    constexpr int operator()(int value) const noexcept { return value + 1; }
};

struct alignas(64) OverAlignedPredicate {
    std::byte marker{};
    rl::details::PadToAlignment_<sizeof(marker), 64> padding{};

    constexpr bool operator()(int value) const noexcept { return value == 7; }
};

static_assert(sizeof(OverAlignedHandler) == 64);
static_assert(sizeof(OverAlignedPredicate) == 64);

template <class O>
concept CanMatchCompleteBool = requires(O &option) {
    option.match(rl::True >>= [](bool value) { return value ? 1 : 0; },
                 rl::False >>= [](bool value) { return value ? 1 : 0; },
                 rl::None >>= [] { return -1; });
};

template <class O>
concept CanMatchBoolWithWildcard = requires(O &option) {
    option.match(rl::Wild >>= [](bool value) { return value ? 1 : 0; },
                 rl::None >>= [] { return -1; });
};

template <class O>
concept CanMatchEveryAlternative = requires(O &option) {
    option.match(rl::SomeAny >>= [](auto &&value) {
                     return static_cast<int>(value);
                 },
                 rl::None >>= [] { return -1; });
};

static_assert(CanMatchCompleteBool<rl::Option<bool>>);
static_assert(CanMatchBoolWithWildcard<rl::Option<bool>>);
static_assert(CanMatchEveryAlternative<rl::Option<int, double>>);

consteval auto verify_partial_pattern_fallback() -> bool {
    rl::Option<int> exact{rl::Some(1)};
    rl::Option<int> other{rl::Some(8)};
    rl::Option<int> empty{rl::None};

    const auto exact_handler = [](int) constexpr { return 10; };
    const auto some_fallback = [](int value) constexpr { return value; };
    const auto none_handler = [] constexpr { return -1; };

    return exact.match(rl::Some<int>{1} >>= exact_handler,
                       rl::SomeAny >>= some_fallback,
                       rl::None >>= none_handler).unwrap() == 10 &&
           other.match(rl::Some<int>{1} >>= exact_handler,
                       rl::SomeAny >>= some_fallback,
                       rl::None >>= none_handler).unwrap() == 8 &&
           empty.match(rl::Some<int>{1} >>= exact_handler,
                       rl::SomeAny >>= some_fallback,
                       rl::None >>= none_handler).unwrap() == -1;
}

static_assert(verify_partial_pattern_fallback());

consteval auto verify_ordinary_and_overaligned_arms() -> bool {
    rl::Option<int> exact{rl::Some(7)};
    rl::Option<int> guarded{rl::Some(7)};

    const auto captureless = [](int value) constexpr { return value + 2; };
    const auto some_fallback = [](int value) constexpr { return value; };
    const auto none_handler = [] constexpr { return -1; };

    const auto raw_arm_result = exact.match(
        rl::Some<int>{7} >>= captureless,
        rl::SomeAny >>= some_fallback,
        rl::None >>= none_handler);
    const auto aligned_handler_result = exact.match(
        rl::Some<int>{7} >>= OverAlignedHandler{},
        rl::SomeAny >>= some_fallback,
        rl::None >>= none_handler);
    const auto aligned_predicate_result = guarded.match(
        OverAlignedPredicate{} >>= captureless,
        rl::SomeAny >>= OverAlignedHandler{},
        rl::None >>= none_handler);

    return raw_arm_result.unwrap() == 9 &&
           aligned_handler_result.unwrap() == 8 &&
           aligned_predicate_result.unwrap() == 9;
}

static_assert(verify_ordinary_and_overaligned_arms());

consteval auto verify_complete_bool_dispatch() -> bool {
    rl::Option<bool> yes{rl::Some(true)};
    rl::Option<bool> no{rl::Some(false)};
    rl::Option<bool> empty{rl::None};
    const auto yes_handler = [](bool) constexpr { return 1; };
    const auto no_handler = [](bool) constexpr { return 2; };
    const auto none_handler = [] constexpr { return 3; };

    return yes.match(rl::True >>= yes_handler, rl::False >>= no_handler,
                     rl::None >>= none_handler).unwrap() == 1 &&
           no.match(rl::True >>= yes_handler, rl::False >>= no_handler,
                    rl::None >>= none_handler).unwrap() == 2 &&
           empty.match(rl::True >>= yes_handler, rl::False >>= no_handler,
                       rl::None >>= none_handler).unwrap() == 3;
}

static_assert(verify_complete_bool_dispatch());

consteval auto verify_all_alternative_dispatch() -> bool {
    rl::Option<int, double> integer{rl::Some(7)};
    rl::Option<int, double> real{rl::Some<double>(2.5)};
    rl::Option<int, double> empty{rl::None};
    const auto some_handler = [](auto value) constexpr {
        return static_cast<int>(value);
    };
    const auto none_handler = [] constexpr { return -1; };

    return integer.match(rl::SomeAny >>= some_handler,
                         rl::None >>= none_handler).unwrap() == 7 &&
           real.match(rl::SomeAny >>= some_handler,
                      rl::None >>= none_handler).unwrap() == 2 &&
           empty.match(rl::SomeAny >>= some_handler,
                       rl::None >>= none_handler).unwrap() == -1;
}

static_assert(verify_all_alternative_dispatch());

} // namespace

int main() {}
