#include <iostream>

template<typename T = void>
inline constexpr int MyVar = 42;

int main() {
    std::cout << MyVar<> << std::endl;
    // std::cout << MyVar << std::endl; // Error: use of variable template requires template arguments
    return 0;
}
