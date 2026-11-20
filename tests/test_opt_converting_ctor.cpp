#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>

namespace __ZN4Lib_5impl_4opt_6cxx26_8symbols_E::__1::__details {

template<typename Up_, typename... Types_>
struct match_alternative_ {
    template<typename T_>
    static constexpr bool is_exact = std::same_as<std::remove_cvref_t<Up_>, T_>;

    static constexpr std::size_t exact_count = ( (is_exact<Types_> ? 1 : 0) + ... );

    template<typename T_>
    static constexpr bool is_conv = std::is_constructible_v<T_, Up_>;

    static constexpr std::size_t conv_count = ( (is_conv<Types_> ? 1 : 0) + ... );

    template<typename First_, typename... Rest_>
    static constexpr auto find_exact() {
        if constexpr (is_exact<First_>) {
            return std::type_identity<First_>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return find_exact<Rest_...>();
        }
    }

    template<typename First_, typename... Rest_>
    static constexpr auto find_conv() {
        if constexpr (is_conv<First_>) {
            return std::type_identity<First_>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return find_conv<Rest_...>();
        }
    }

    static constexpr bool valid = (exact_count == 1) || (exact_count == 0 && conv_count == 1);

    using type = typename decltype([] {
        if constexpr (exact_count == 1) {
            return find_exact<Types_...>();
        } else if constexpr (exact_count == 0 && conv_count == 1) {
            return find_conv<Types_...>();
        } else {
            return std::type_identity<void>{};
        }
    }())::type;
};

}

int main() {
    using rlIntStr [[maybe_unused]] = rl::Option<int, std::string>;
    // Check match_alternative_
    static_assert(std::same_as<__ZN4Lib_5impl_4opt_6cxx26_8symbols_E::__1::__details::match_alternative_<int, int, std::string>::type, int>);
    static_assert(std::same_as<__ZN4Lib_5impl_4opt_6cxx26_8symbols_E::__1::__details::match_alternative_<const char*, int, std::string>::type, std::string>);

    std::cout << "Traits valid!\n";
    return 0;
}
