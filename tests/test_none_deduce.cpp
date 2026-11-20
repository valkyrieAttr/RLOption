#include <iostream>
#include <tuple>
#include <type_traits>
#include <string>

struct NoneType {};

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
    using CleanFirst_ = std::remove_cvref_t<First_>;
    // If CleanFirst_ is NoneType, don't add to tuple of value alternatives
    using NextAcc_ = std::conditional_t<
        std::same_as<CleanFirst_, NoneType>,
        Acc_,
        typename append_unique_<Acc_, CleanFirst_>::type
    >;
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
concept has_common_type = ( !(std::same_as<std::remove_cvref_t<Rs_>, NoneType> || ...) ) && requires {
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

int main() {
    // 1. All int -> int
    using R1 = typename deduce_res_helper<int, int>::type;
    static_assert(std::same_as<R1, int>);

    // 2. int and NoneType -> FakeOption<int>
    using R2 = typename deduce_res_helper<int, NoneType>::type;
    static_assert(std::same_as<R2, FakeOption<int>>);

    // 3. int and std::string -> FakeOption<int, std::string>
    using R3 = typename deduce_res_helper<int, std::string>::type;
    static_assert(std::same_as<R3, FakeOption<int, std::string>>);

    // 4. int, std::string, and NoneType -> FakeOption<int, std::string>
    using R4 = typename deduce_res_helper<int, std::string, NoneType>::type;
    static_assert(std::same_as<R4, FakeOption<int, std::string>>);

    std::cout << "All deduce tests with NoneType passed!\n";
    return 0;
}
