
#include <compare>
#include <concepts>
#include <type_traits>

struct None {};
struct Default {};

template<typename Tp_>
struct Myrl {
    bool is_some() const { return true; }
    const Tp_& operator*() const { return val; }
    Tp_ val{};

    friend constexpr auto operator<=>
        ( const Myrl<Tp_>& lhs , const Myrl<Tp_>& rhs )
    -> std::compare_three_way_result_t<Tp_>
        requires ( std::three_way_comparable<Tp_> )
    {
        if ( lhs.is_some ( ) && rhs.is_some ( ) )
        {
            return *lhs <=> *rhs;
        }
        return lhs.is_some ( ) <=> rhs.is_some ( );
    }

    friend constexpr auto operator<=>
        ( const Myrl<Tp_>& lhs , None ) noexcept
    -> std::strong_ordering
    {
        return lhs.is_some ( ) <=> false;
    }

    friend constexpr auto operator<=>
        ( None , const Myrl<Tp_>& rhs ) noexcept
    -> std::strong_ordering
    {
        return false <=> rhs.is_some ( );
    }

    friend constexpr auto operator<=>
        ( const Myrl<Tp_>& lhs , Default )
    -> std::compare_three_way_result_t<Tp_>
        requires ( std::is_default_constructible_v<Tp_> && std::three_way_comparable<Tp_> )
    {
        if ( lhs.is_some ( ) )
        {
            return *lhs <=> Tp_ { };
        }
        return std::strong_ordering::less;
    }

    friend constexpr auto operator<=>
        ( Default , const Myrl<Tp_>& rhs )
    -> std::compare_three_way_result_t<Tp_>
        requires ( std::is_default_constructible_v<Tp_> && std::three_way_comparable<Tp_> )
    {
        if ( rhs.is_some ( ) )
        {
            return Tp_ { } <=> *rhs;
        }
        return std::strong_ordering::greater;
    }
};

int main() {
    Myrl<float> a{1.0f}, b{2.0f};
    auto r1 = a <=> b;
    static_assert(std::same_as<decltype(r1), std::partial_ordering>);
    auto r2 = a <=> None{};
    static_assert(std::same_as<decltype(r2), std::strong_ordering>);
    auto r3 = None{} <=> a;
    static_assert(std::same_as<decltype(r3), std::strong_ordering>);
    auto r4 = a <=> Default{};
    static_assert(std::same_as<decltype(r4), std::partial_ordering>);
    auto r5 = Default{} <=> a;
    static_assert(std::same_as<decltype(r5), std::partial_ordering>);
}
