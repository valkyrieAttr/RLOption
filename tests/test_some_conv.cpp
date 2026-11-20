#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

template<typename Tp_>
struct Some {
    Tp_ value;

    explicit constexpr Some() requires (std::is_default_constructible_v<Tp_>) : value{} {}
    explicit constexpr Some(const Tp_& v) : value{v} {}
    explicit constexpr Some(Tp_&& v) : value{std::move(v)} {}

    template<typename Up_>
    requires (!std::same_as<Up_, Tp_> && std::is_constructible_v<Tp_, const Up_&>)
    constexpr explicit(!std::is_convertible_v<const Up_&, Tp_>) Some(const Some<Up_>& other)
        : value{other.value} {}

    template<typename Up_>
    requires (!std::same_as<Up_, Tp_> && std::is_constructible_v<Tp_, Up_&&>)
    constexpr explicit(!std::is_convertible_v<Up_&&, Tp_>) Some(Some<Up_>&& other)
        : value{std::move(other.value)} {}

    constexpr const Tp_& get() const& { return value; }
    constexpr Tp_&& get() && { return std::move(value); }
};

template<typename Tp_> Some(Tp_) -> Some<Tp_>;

int main() {
    Some<std::string> s = Some("hello");
    std::cout << s.get() << std::endl;
    Some<const int> ci = Some(42);
    std::cout << ci.get() << std::endl;
    return 0;
}
