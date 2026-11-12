#include "rl_option.hpp"

#include <cassert>
#include <string>
#include <type_traits>

struct Both {
    constexpr auto operator()(void) const -> int { return -1; }
    constexpr auto operator()(int value) const -> int { return value + 10; }
};

struct Immovable {
    constexpr Immovable(void) = default;
    Immovable(const Immovable &) = delete;
    Immovable(Immovable &&) = delete;
};

constexpr auto constexpr_match(void) -> int {
    rl::Option<int> value{rl::Some{42}};
    auto result = value.match(
        rl::None >>= [] { return 0; },
        rl::SomeAny >>= [](int current) { return current; });
    return *result;
}

static_assert(constexpr_match() == 42);

int main() {
    rl::Option<int> some{rl::Some{7}};
    auto some_result = some.match(
        rl::None >>= [] { return -1; },
        rl::SomeAny >>= [](int value) { return value + 1; });
    assert(some_result.is_some());
    assert(*some_result == 8);

    auto single_custom_result = some.match<int>(rl::Wild >>= Both{});
    static_assert(
        std::same_as<decltype(single_custom_result), rl::Option<int>>);
    assert(*single_custom_result == 17);

    rl::Option<int> none{rl::None};
    auto none_result = none.match(
        rl::Wild >>= Both{},
        rl::SomeAny >>= [](int value) { return value + 1; });
    assert(none_result.is_some());
    assert(*none_result == -1);

    rl::Option<int, std::string> text{rl::Some{std::string{"hello"}}};
    auto multi_result = text.match(
        rl::None >>= [] { return 0; },
        rl::Some{0} >>= [](int value) { return value; },
        [](const std::string &) { return true; } >>=
            [](const std::string &value) { return value.size(); });
    assert(multi_result.is_some());
    assert(*multi_result == 5);

    auto custom_result = text.match<int, long>(
        rl::None >>= [] { return 0; },
        [](const int &) { return true; } >>= [](int value) { return value; },
        [](const std::string &) { return true; } >>=
            [](const std::string &value) {
                return static_cast<long>(value.size());
            });
    static_assert(std::same_as<decltype(custom_result), rl::Option<int, long>>);
    assert(custom_result.is_some());

    rl::Option<int, std::string> integer{rl::Some{9}};
    auto custom_integer_result = integer.match<int, long>(
        rl::None >>= [] { return 0; },
        [](const int &) { return true; } >>= [](int value) { return value + 1; },
        [](const std::string &) { return true; } >>=
            [](const std::string &value) {
                return static_cast<long>(value.size());
            });
    static_assert(
        std::same_as<decltype(custom_integer_result), rl::Option<int, long>>);
    assert(*custom_integer_result == 10);

    auto constant_result = some.match(rl::None >>= 0, rl::SomeAny >>= 12);
    assert(*constant_result == 12);

    bool called = false;
    text.match<void>(
        rl::None >>= [&] { called = true; },
        [](const int &) { return true; } >>= [&](int) { called = true; },
        [](const std::string &) { return true; } >>=
            [&](const std::string &) { called = true; });
    assert(called);

    const rl::Option<int> const_value{rl::Some{4}};
    auto const_result = const_value.match(
        rl::None >>= [] { return 0; },
        rl::SomeAny >>= [](const int &value) { return value; });
    assert(*const_result == 4);

    auto move_result =
        rl::Option<std::string>{rl::Some{std::string{"move"}}}.match(
            rl::None >>= [] { return 0; },
            [](const std::string &) { return true; } >>=
                [](std::string &&value) {
                    return static_cast<int>(value.size());
                });
    assert(*move_result == 4);

    rl::Option<Immovable> immovable{rl::Default};
    auto immovable_result = immovable.match(
        rl::None >>= [] { return rl::Option<Immovable>{rl::None}; },
        rl::SomeAny >>= [](Immovable &) {
            return rl::Option<Immovable>{rl::Default};
        });
    assert(immovable_result.is_some());
}
