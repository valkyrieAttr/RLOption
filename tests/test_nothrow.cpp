
#include <iostream>
#include <type_traits>

struct ThrowOnSecondMove {
    int id{0};
    ThrowOnSecondMove(int i) : id(i) {}
    ThrowOnSecondMove(ThrowOnSecondMove&& other) : id(other.id) {}
};

int main() {
    std::cout << "is_nothrow: " << std::is_nothrow_move_constructible_v<ThrowOnSecondMove> << "\n";
}
