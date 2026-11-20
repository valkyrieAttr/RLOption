#include <iostream>
#include <type_traits>
#include <tuple>
#include <string>

template<typename List_, typename T_>
struct append_unique_;

template<typename... Ts_, typename T_>
struct append_unique_<std::tuple<Ts_...>, T_> {
    using type = std::conditional_t<
        (std::same_as<T_, Ts_> || ...),
        std::tuple<Ts_...>,
        std::tuple<Ts_..., T_>
    >;
};

template<typename Acc_, typename... Rest_>
struct dedup_pack_impl_;

template<typename Acc_>
struct dedup_pack_impl_<Acc_> {
    using type = Acc_;
};

template<typename Acc_, typename First_, typename... Rest_>
struct dedup_pack_impl_<Acc_, First_, Rest_...> {
    using NextAcc_ = typename append_unique_<Acc_, First_>::type;
    using type = typename dedup_pack_impl_<NextAcc_, Rest_...>::type;
};

template<typename... Ts_>
using dedup_tuple_t = typename dedup_pack_impl_<std::tuple<>, Ts_...>::type;

template<typename T, typename... Args>
struct FakeOption {};

template<typename Tuple_>
struct tuple_to_option_;

template<typename First_, typename... Rest_>
struct tuple_to_option_<std::tuple<First_, Rest_...>> {
    using type = FakeOption<First_, Rest_...>;
};

template<typename... Rs_>
concept has_common_type = requires {
    typename std::common_type_t<Rs_...>;
};

template<typename... Rs_>
struct deduce_res_helper {
    using type = std::conditional_t<
        has_common_type<Rs_...>,
        std::common_type<Rs_...>,
        tuple_to_option_<dedup_tuple_t<Rs_...>>
    >::type;
};

template<typename... Rs_>
using deduce_res_t = typename deduce_res_helper<Rs_...>::type;

int main() {
    using R1 = deduce_res_t<int, int>;
    static_assert(std::same_as<R1, int>);

    using R2 = deduce_res_t<const char*, std::string>;
    static_assert(std::same_as<R2, std::string>);

    using R3 = deduce_res_t<int, std::string>;
    static_assert(std::same_as<R3, FakeOption<int, std::string>>);

    std::cout << "Type deduction prototype passed!\n";
    return 0;
}
