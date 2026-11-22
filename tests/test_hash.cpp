
#include <rl_option.hpp>

#include <cstddef>
#include <functional>

int main() {
    rl::Option<int> engaged{rl::Some(42)};
    rl::Option<int> empty{rl::None};
    const std::hash<rl::Option<int>> hash_option{};

    if (hash_option(engaged) != std::hash<int>{}(42)) {
        return 1;
    }
    if (hash_option(empty) != static_cast<std::size_t>(-3333)) {
        return 2;
    }

    return 0;
}
