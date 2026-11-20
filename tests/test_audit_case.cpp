#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    rl::Option<int, std::string> option = rl::Some<std::string>("hello");

    auto r_auto = option.match(
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
    std::cout << "Auto Rp passed: " << r_auto.unwrap() << std::endl;
    return 0;
}
