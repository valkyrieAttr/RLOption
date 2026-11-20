#include "rl_option.hpp"
#include <string>
#include <cassert>

int main() {
    rl::Option<std::string> opt = rl::Some("hello");
    assert(opt.is_some());
    assert(opt.unwrap() == "hello");
    return 0;
}
