#include <iostream>
#include <tuple>
#include <type_traits>
#include <string>

namespace details {

template<typename T>
struct is_some_ : std::false_type {};

template<typename T>
struct Some {
    T val;
};

template<typename T>
struct is_some_<Some<T>> : std::true_type {
    using type = T;
};

struct None {};

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
    using Unwrapped_ = typename std::conditional_t<
        is_some_<CleanFirst_>::value,
        is_some_<CleanFirst_>,
        std::type_identity<CleanFirst_>
    >::type;

    using NextAcc_ = std::conditional_t<
        std::same_as<Unwrapped_, None>,
        Acc_,
        typename append_unique_<Acc_, Unwrapped_>::type
    >;
    using type = typename dedup_pack_impl_<NextAcc_, Rest_...>::type;
};

template<typename... Ts_>
using dedup_tuple_t = typename dedup_pack_impl_<std::tuple<>, Ts_...>::type;

template<typename T, typename... Args>
struct FakeOption {};

template<typename Tuple_>
struct tuple_to_option_;

template<>
struct tuple_to_option_<std::tuple<>> {
    using type = None;
};

template<typename First_, typename... Rest_>
struct tuple_to_option_<std::tuple<First_, Rest_...>> {
    using type = FakeOption<First_, Rest_...>;
};

template<typename... Rs_>
concept has_common_type = ( !(std::same_as<std::remove_cvref_t<Rs_>, None> || ...) ) &&
                          ( !(is_some_<std::remove_cvref_t<Rs_>>::value || ...) ) &&
                          requires {
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

} // namespace details

int main() {
    using namespace details;

    // 1. Raw types with common type
    using R1 = typename deduce_res_helper<int, int>::type;
    static_assert(std::same_as<R1, int>);

    using R2 = typename deduce_res_helper<const char*, std::string>::type;
    static_assert(std::same_as<R2, std::string>);

    // 2. Disparate types -> Option<int, std::string>
    using R3 = typename deduce_res_helper<int, std::string>::type;
    static_assert(std::same_as<R3, FakeOption<int, std::string>>);

    // 3. Some<int> and Some<string> -> Option<int, std::string>
    using R4 = typename deduce_res_helper<Some<int>, Some<std::string>>::type;
    static_assert(std::same_as<R4, FakeOption<int, std::string>>);

    // 4. Some<int> and None -> Option<int>
    using R5 = typename deduce_res_helper<Some<int>, None>::type;
    static_assert(std::same_as<R5, FakeOption<int>>);

    // 5. int and None -> Option<int>
    using R6 = typename deduce_res_helper<int, None>::type;
    static_assert(std::same_as<R6, FakeOption<int>>);

    // 6. int, std::string, and None -> Option<int, std::string>
    using R7 = typename deduce_res_helper<int, std::string, None>::type;
    static_assert(std::same_as<R7, FakeOption<int, std::string>>);

    std::cout << "All deduction cases passed!\n";
    return 0;
}
