#include "rl_option.hpp"
#include <iostream>
#include <cassert>

int main() {
    rl::Option<const int> opt = rl::Some<const int>(42);
    assert(opt.is_some());
    assert(opt.unwrap() == 42);
    auto opt2 = std::move(opt);
    assert(opt2.unwrap() == 42);
    std::cout << "Option<const int> works!\n";
    return 0;
}
