#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    // 1. Audit case: Option<int, std::string> with typed handlers
    rl::Option<int, std::string> option = rl::Some<std::string>("hello");

    // Explicit Rp test
    auto r_explicit = option.match<std::string>(
        rl::Some<std::string>("hello") >>= [](const std::string&) {
            return std::string("matched");
        },
        rl::SomeAny >>= [] {
            return std::string("some");
        },
        rl::None >>= [] {
            return std::string("none");
        }
    );
    assert(r_explicit.unwrap() == "matched");
    std::cout << "Explicit Rp passed: " << r_explicit << std::endl;
    return 0;
}
