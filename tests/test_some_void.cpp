#include "rl_option.hpp"
#include <iostream>
#include <cassert>

int main() {
    // 1. Some() type inference
    rl::Option<int> o1 = rl::Some();
    assert(o1.is_some() && o1.unwrap() == 0);

    // 2. Some(val) deduction
    rl::Option<int> o2 = rl::Some(42);
    assert(o2.is_some() && o2.unwrap() == 42);

    // 3. Some<T>()
    rl::Option<int> o3 = rl::Some<int>();
    assert(o3.is_some() && o3.unwrap() == 0);

    std::cout << "All Some tests passed!\n";
    return 0;
}
