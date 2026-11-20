
#include <compare>
#include <concepts>

namespace N1 {
    namespace details {
        template<typename Up_>
        struct __compare_three_way_result { using type = void; };

        template<typename Up_>
            requires ( std::three_way_comparable<Up_> )
        struct __compare_three_way_result<Up_> { using type = std::compare_three_way_result_t<Up_>; };

        template<typename Up_>
        using __compare_three_way_result_t = typename __compare_three_way_result<Up_>::type;
    }

    namespace rl {
        template<typename Tp_>
        struct Option {
            friend constexpr auto operator<=>(const Option&, const Option&)
                -> details::__compare_three_way_result_t<Tp_>
                requires ( std::three_way_comparable<Tp_> )
            {
                return std::strong_ordering::equal;
            }
        };
    }
}

int main() {
    N1::rl::Option<int> a;
    static_cast<void>(a);
    return 0;
}
