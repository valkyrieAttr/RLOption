#include "rl_option.hpp"
#include <iostream>

int main() {
    auto f = [](int x) { return x * 2; };
    static_assert(std::is_invocable_r_v<rl::Option<int>, decltype(f), int>);
    
    auto fn_none = []() { return rl::None; };
    static_assert(std::is_invocable_r_v<rl::Option<int>, decltype(fn_none)>);

    auto opt1 = std::invoke_r<rl::Option<int>>(f, 21);
    std::cout << "opt1 is_some: " << opt1.is_some() << ", val: " << opt1.unwrap() << "\n";

    auto opt2 = std::invoke_r<rl::Option<int>>(fn_none);
    std::cout << "opt2 is_none: " << opt2.is_none() << "\n";
}
