#include "rl_option.hpp"
#include <iostream>

int main() {
    auto ok = rl::Option<int, std::string>(rl::None)
        .match
        (
            [](auto x) { return x; } >>= "string",
            rl::Some(2) >>= "string",
            rl::SomeAny >>= "rl::Any",
            rl::None >>= 1
        );
    std::cout << "ok is_some: " << ok.is_some() << "\n";
    std::cout << "ok holds int: " << ok.is_some<int>() << "\n";
    if (ok.is_some<int>()) {
        std::cout << "ok unwrapped int: " << ok.unwrap<int>() << "\n";
    }
}
