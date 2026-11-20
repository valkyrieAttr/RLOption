#include <iostream>
#include <type_traits>
#include <string>

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

int main() {
    using M1 = typename match_alternative_<int, int, std::string>::type;
    static_assert(std::same_as<M1, int>);

    using M2 = typename match_alternative_<const char*, int, std::string>::type;
    static_assert(std::same_as<M2, std::string>);

    using M3 = typename match_alternative_<const char[6], int, std::string>::type;
    static_assert(std::same_as<M3, std::string>);

    using M4 = typename match_alternative_<std::string, int, std::string>::type;
    static_assert(std::same_as<M4, std::string>);

    // Ambiguous
    static_assert(!match_alternative_<float, int, double>::valid);

    std::cout << "match_alternative tests passed!\n";
    return 0;
}
