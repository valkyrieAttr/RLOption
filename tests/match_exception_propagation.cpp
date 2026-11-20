#include "rl_option.hpp"

#include <stdexcept>

int main() {
    rl::Option<bool> engaged{rl::Some(true)};
    bool caught_engaged = false;
    try {
        (void)engaged.match(
            rl::True >>= []() -> int { throw std::runtime_error("some arm"); },
            rl::False >>= [] { return 0; },
            rl::None >>= [] { return -1; });
    } catch (const std::runtime_error &) {
        caught_engaged = engaged.is_some() && engaged.unwrap();
    }

    rl::Option<bool> empty{rl::None};
    bool caught_none = false;
    try {
        (void)empty.match(
            rl::True >>= [] { return 1; },
            rl::False >>= [] { return 0; },
            rl::None >>= []() -> int {
                throw std::runtime_error("none arm");
            });
    } catch (const std::runtime_error &) {
        caught_none = empty.is_none();
    }

    return caught_engaged && caught_none ? 0 : 1;
}
