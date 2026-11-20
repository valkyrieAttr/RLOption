#include <iostream>
#include <string>

template<typename... T>
struct OptionReturn {};

struct S {
    // 0 explicit types (deduced):
    template<typename Self, typename... Arms>
    int match_unwrapped(this Self&&, Arms&&...) {
        std::cout << "0 explicit types (deduced)\n";
        return 0;
    }

    // 1 explicit type:
    template<typename R, typename Self, typename... Arms>
    R match_unwrapped(this Self&&, Arms&&...) {
        std::cout << "1 type\n";
        return R{};
    }

    // 2 explicit types:
    template<typename R1, typename R2, typename Self, typename... Arms>
    OptionReturn<R1, R2> match_unwrapped(this Self&&, Arms&&...) {
        std::cout << "2 types\n";
        return {};
    }

    // 3 types:
    template<typename R1, typename R2, typename R3, typename Self, typename... Arms>
    OptionReturn<R1, R2, R3> match_unwrapped(this Self&&, Arms&&...) {
        std::cout << "3 types\n";
        return {};
    }
};

int main() {
    S s;
    s.match_unwrapped(1, 2);
    s.match_unwrapped<int>(1, 2);
    s.match_unwrapped<int, std::string>(1, 2);
    s.match_unwrapped<int, std::string, double>(1, 2);
    return 0;
}
