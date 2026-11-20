#include "rl_option.hpp"
#include <iostream>
#include <type_traits>
#include <tuple>

namespace test {

template<typename T>
struct underlying_val {
    using type = std::remove_cvref_t<T>;
};

template<typename T>
struct underlying_val<rl::Some<T>> {
    using type = std::remove_cvref_t<T>;
};

template<typename T, typename... Rest>
struct underlying_val<rl::Option<T, Rest...>> {
    using type = std::remove_cvref_t<T>;
};

template<typename T>
using underlying_val_t = typename underlying_val<T>::type;

// Filter None
template<typename... Ts>
struct filter_none;

template<>
struct filter_none<> {
    using type = std::tuple<>;
};

template<typename First, typename... Rest>
struct filter_none<First, Rest...> {
    using rest_tuple = typename filter_none<Rest...>::type;
    using type = std::conditional_t<
        std::same_as<std::remove_cvref_t<First>, rl::None_t>,
        rest_tuple,
        decltype(std::tuple_cat(std::declval<std::tuple<First>>(), std::declval<rest_tuple>()))
    >;
};

template<typename Tuple>
struct tuple_common_type;

template<>
struct tuple_common_type<std::tuple<>> {
    using type = rl::None_t;
};

template<typename First, typename... Rest>
struct tuple_common_type<std::tuple<First, Rest...>> {
    using type = std::common_type_t<First, Rest...>;
};

template<typename T>
struct wrap_option {
    using type = rl::Option<T>;
};

template<typename T>
requires (requires { typename T::value_type; })
struct wrap_option<T> {
    using type = T;
};

template<>
struct wrap_option<void> {
    using type = void;
};

template<>
struct wrap_option<rl::None_t> {
    using type = rl::None_t;
};

} // namespace test

int main() {
    using T1 = test::tuple_common_type<typename test::filter_none<const char*, const char*, const char*, rl::None_t>::type>::type;
    std::cout << "T1 is const char*: " << std::is_same_v<T1, const char*> << "\n";
    using W1 = test::wrap_option<T1>::type;
    std::cout << "W1 is Option<const char*>: " << std::is_same_v<W1, rl::Option<const char*>> << "\n";

    using T2 = test::tuple_common_type<typename test::filter_none<int, rl::None_t>::type>::type;
    std::cout << "T2 is int: " << std::is_same_v<T2, int> << "\n";
    using W2 = test::wrap_option<T2>::type;
    std::cout << "W2 is Option<int>: " << std::is_same_v<W2, rl::Option<int>> << "\n";
}
