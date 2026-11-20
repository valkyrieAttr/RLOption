#include "rl_option.hpp"

#include <memory>
#include <type_traits>

namespace {

consteval auto verify_reference_results_are_materialized() -> bool {
    int true_source = 11;
    int false_source = 22;
    int none_source = 33;
    rl::Option<bool> yes{rl::Some(true)};

    auto result = yes.match(
        rl::True >>= [&true_source]() constexpr -> int & {
            return true_source;
        },
        rl::False >>= [&false_source]() constexpr -> int & {
            return false_source;
        },
        rl::None >>= [&none_source]() constexpr -> int & {
            return none_source;
        });

    static_assert(std::same_as<decltype(result), rl::Option<int>>);
    result.unwrap() = 99;
    return result.unwrap() == 99 && true_source == 11 &&
           false_source == 22 && none_source == 33;
}

static_assert(verify_reference_results_are_materialized());

} // namespace

int main() {
    rl::Option<bool> yes{rl::Some(true)};
    rl::Option<bool> no{rl::Some(false)};
    rl::Option<bool> empty{rl::None};
    const auto yes_handler = [] { return std::make_unique<int>(1); };
    const auto no_handler = [] { return std::make_unique<int>(2); };
    const auto none_handler = [] { return std::make_unique<int>(3); };

    auto yes_result = yes.match(rl::True >>= yes_handler,
                                rl::False >>= no_handler,
                                rl::None >>= none_handler);
    auto no_result = no.match(rl::True >>= yes_handler,
                              rl::False >>= no_handler,
                              rl::None >>= none_handler);
    auto none_result = empty.match(rl::True >>= yes_handler,
                                   rl::False >>= no_handler,
                                   rl::None >>= none_handler);

    static_assert(std::same_as<decltype(yes_result),
                               rl::Option<std::unique_ptr<int>>>);
    return *yes_result.unwrap() == 1 && *no_result.unwrap() == 2 &&
                   *none_result.unwrap() == 3
               ? 0
               : 1;
}
