#include "rl_option.hpp"
#include <iostream>
#include <cassert>

namespace rl::inline literals {
    constexpr auto operator""_p(unsigned long long v) {
        return rl::Some(static_cast<int>(v));
    }
    constexpr auto operator""_p(long double v) {
        return rl::Some(static_cast<double>(v));
    }
    constexpr auto operator""_p(const char* s, std::size_t len) {
        return rl::Some(std::string_view(s, len));
    }
}

int main() {
    using namespace rl::literals;
    rl::Option<int> opt = rl::Some(2);
    auto res = opt.match(
        2_p >>= "matched-2",
        rl::SomeAny >>= "other",
        rl::None >>= "none"
    );
    assert(std::string(res.unwrap()) == "matched-2");
    std::cout << "UDL 2_p >>= works perfectly!\n";
    return 0;
}
