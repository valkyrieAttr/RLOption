#include "rl_option.hpp"
#include <iostream>

int main() {
    auto opt = rl::Option<int, std::string>(rl::None);
    std::cout << "is_none: " << opt.is_none() << "\n";
    std::cout << "is_some: " << opt.is_some() << "\n";
    
    auto ok = opt.match(
        [](auto x) { return x; } >>= "string",
        rl::Some(2) >>= "string",
        rl::SomeAny >>= "rl::Any",
        rl::None >>= 1
    );
    
    std::cout << "ok.is_some: " << ok.is_some() << "\n";
    if (ok.is_some<int>()) {
        std::cout << "ok is int: " << ok.unwrap<int>() << "\n";
    }
    if (ok.is_some<const char*>()) {
        std::cout << "ok is const char*: " << ok.unwrap<const char*>() << "\n";
    }
    std::cout << "ok.value(): " << ok.value() << "\n";
}
