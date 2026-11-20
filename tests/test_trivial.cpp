
#include <iostream>
#include <type_traits>

struct CustomCopy {
    CustomCopy() = default;
    CustomCopy(const CustomCopy&) {}
    CustomCopy(CustomCopy&&) = default;
};

int main() {
    std::cout << "is_trivially_move_constructible_v<CustomCopy>: "
              << std::is_trivially_move_constructible_v<CustomCopy> << "\n";
    std::cout << "is_trivially_move_constructible_v<const CustomCopy>: "
              << std::is_trivially_move_constructible_v<const CustomCopy> << "\n";
}
