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
    std::cout << "Calling ok.value():\n";
    std::cout << ok.value() << "\n";
}
