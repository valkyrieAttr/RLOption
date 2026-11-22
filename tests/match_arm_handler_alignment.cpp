#include <rl_option.hpp>

#include <cassert>
#include <string>

int main() {
    rl::Option<int, std::string, bool> value = rl::Some<int>(1);

    auto run_match = [&]() {
        return value.match(
            rl::Some<std::string>("Hello World") >>= [] { return 20; },
            rl::True >>= [] { return 30; },
            rl::Some<int>(1) >>= [](int matched) { return matched + 10; },
            rl::Wild >>= [] { return 40; });
    };

    assert(run_match() == 11);

    value = rl::Some<std::string>("Hello World");
    assert(run_match() == 20);

    value = rl::Some<bool>(true);
    assert(run_match() == 30);

    value = rl::Some<bool>(false);
    assert(run_match() == 40);
}
