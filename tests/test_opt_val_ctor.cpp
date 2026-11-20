#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    // Test if Some<int> works
    rl::Option<int, std::string> o1 = rl::Some(42);
    assert(o1.unwrap() == 42);

    rl::Option<int, std::string> o2 = rl::Some<std::string>("hello");
    assert(o2.unwrap<std::string>() == "hello");

    std::cout << "Existing constructors work!\n";
    return 0;
}
