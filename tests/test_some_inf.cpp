#include "rl_option.hpp"

int main() {
    rl::Option<int> opt = rl::Some();
    return opt.is_some() ? 0 : 1;
}
