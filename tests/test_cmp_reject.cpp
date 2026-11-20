
#include <compare>
#include <concepts>
#include <type_traits>

template<typename Up_>
struct __compare_three_way_result { using type = void; };

template<typename Up_>
    requires std::three_way_comparable<Up_>
struct __compare_three_way_result<Up_> {
    using type = std::compare_three_way_result_t<Up_>;
};

template<typename Up_>
using __compare_three_way_result_t = typename __compare_three_way_result<Up_>::type;

template<typename Tp_>
struct Myrl {
    bool is_some() const { return true; }
    const Tp_& operator*() const { return val; }
    Tp_ val{};

    friend constexpr auto operator<=>
        ( const Myrl<Tp_>& lhs , const Myrl<Tp_>& rhs )
    -> __compare_three_way_result_t<Tp_>
        requires ( std::three_way_comparable<Tp_> )
    {
        if ( lhs.is_some ( ) && rhs.is_some ( ) )
        {
            return *lhs <=> *rhs;
        }
        return lhs.is_some ( ) <=> rhs.is_some ( );
    }
};

struct Immovable {};

int main() {
    static_assert(!std::three_way_comparable<Myrl<Immovable>>);
    return 0;
}
