#include "rl_option.hpp"
#include <iostream>
#include <cassert>

int main() {
    rl::Option<int> opt = rl::Some(2);
    auto res = opt.match(
        2 >>= [] { return "matched-2"; },
        rl::SomeAny >>= [] { return "other"; },
        rl::None >>= [] { return "none"; }
    );
    assert(std::string(res.unwrap()) == "matched-2");
    std::cout << "Raw value matching works!\n";
    return 0;
}
