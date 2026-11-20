#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    // Test Some and unwrap
    rl::Option<int, std::string> opt = rl::Some(42);
    assert(opt.unwrap() == 42);

    opt = rl::Some<std::string>("hello");
    assert(opt.unwrap<std::string>() == "hello");

    opt = rl::None;
    assert(opt.is_none());

    std::cout << "Option test passed!\n";
    return 0;
}
