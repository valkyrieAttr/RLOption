
#include <concepts>
#include <utility>

template<typename T>
struct Myrl {
    void swap(Myrl&) noexcept {}

    friend constexpr auto swap(Myrl& lhs, Myrl& rhs) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>
    ) -> void requires (
        std::is_move_constructible_v<T> && std::is_swappable_v<T>
    ) {
        lhs.swap(rhs);
    }
};

int main() {
    Myrl<int> a, b;
    swap(a, b);
}
