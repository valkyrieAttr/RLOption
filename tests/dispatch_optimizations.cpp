#include "rl_option.hpp"

#include <cassert>
#include <string>
#include <type_traits>

struct MustNotInstantiateAfterCatchAll {
    template <typename T>
    constexpr auto operator()(const T &) const -> bool {
        static_assert(!std::same_as<T, T>,
                      "pattern after a catch-all must not be instantiated");
        return true;
    }
};

constexpr auto constexpr_equality() -> bool {
    const rl::Option<int, long> first{rl::Some{42}};
    const rl::Option<int, long> second{rl::Some{42}};
    const rl::Option<int, long> third{rl::Some{43}};
    const rl::Option<int, long> none{rl::None};
    return first == second && first != third &&
           none == rl::Option<int, long>{rl::None};
}

static_assert(constexpr_equality());

int main() {
    // Equality dispatch compares the active, statically selected alternative.
    const rl::Option<int, long, std::string> primary_a{rl::Some{42}};
    const rl::Option<int, long, std::string> primary_b{rl::Some{42}};
    const rl::Option<int, long, std::string> primary_c{rl::Some{43}};
    assert(primary_a == primary_b);
    assert(primary_a != primary_c);

    const rl::Option<int, long, std::string> long_a{rl::Some{42L}};
    const rl::Option<int, long, std::string> long_b{rl::Some{42L}};
    const rl::Option<int, long, std::string> long_c{rl::Some{43L}};
    assert(long_a == long_b);
    assert(long_a != long_c);
    assert(primary_a != long_a);

    const rl::Option<int, long, std::string> string_a{
        rl::Some{std::string{"same"}}};
    const rl::Option<int, long, std::string> string_b{
        rl::Some{std::string{"same"}}};
    const rl::Option<int, long, std::string> string_c{
        rl::Some{std::string{"different"}}};
    assert(string_a == string_b);
    assert(string_a != string_c);

    const rl::Option<int, long, std::string> none_a{rl::None};
    const rl::Option<int, long, std::string> none_b{rl::None};
    assert(none_a == none_b);
    assert(none_a != primary_a);

    // Wild is an unconditional Some catch-all. The following predicate's
    // call operator must not be instantiated during dispatch compilation.
    rl::Option<int> value{rl::Some{17}};
    auto wild_result = value.match<int>(
        rl::None >>= [] { return -1; },
        rl::Wild >>= [](int current) { return current; },
        MustNotInstantiateAfterCatchAll{} >>= [](int) { return 999; });
    assert(*wild_result == 17);

    // SomeAny is also an unconditional Some catch-all, but does not cover None.
    auto any_result = value.match<int>(
        rl::None >>= [] { return -1; },
        rl::SomeAny >>= [](int current) { return current + 1; },
        MustNotInstantiateAfterCatchAll{} >>= [](int) { return 999; });
    assert(*any_result == 18);

    // Catch-all termination must not reorder or skip earlier predicates.
    bool first_predicate_called = false;
    auto ordered_result = value.match<int>(
        rl::None >>= [] { return -1; },
        rl::Where([&](const int &current) {
            first_predicate_called = true;
            return current == 17;
        }) >>= [](int current) { return current * 2; },
        rl::Wild >>= [](int) { return 999; });
    assert(first_predicate_called);
    assert(*ordered_result == 34);
}
