
#include <functional>
#include <concepts>
#include <type_traits>

template<typename Tp_>
concept __is_hashable_ = requires ( const std::hash<std::remove_const_t<Tp_>>& h_ , const Tp_& v_ )
{
    { h_ ( v_ ) } -> std::same_as<std::size_t>;
};

template<typename Tp_, bool = __is_hashable_<Tp_>>
struct __option_hash_base_
{
    __option_hash_base_ ( ) = delete;
    __option_hash_base_ ( const __option_hash_base_& ) = delete;
    __option_hash_base_ ( __option_hash_base_&& ) = delete;
    auto operator= ( const __option_hash_base_& ) -> __option_hash_base_& = delete;
    auto operator= ( __option_hash_base_&& ) -> __option_hash_base_& = delete;
};

template<typename Tp_>
struct __option_hash_base_<Tp_, true>
{
    constexpr auto operator()
        ( const Tp_& /* opt */ ) const
        noexcept ( noexcept ( std::hash<std::remove_const_t<Tp_>> { } ( std::declval<const Tp_&> ( ) ) ) )
    -> std::size_t
    {
        return 42;
    }
};

template<typename T>
struct Myrl {};

template<typename T>
struct std::hash<Myrl<T>> : __option_hash_base_<T> {};

struct Unhashable {};

int main() {
    static_assert(std::is_default_constructible_v<std::hash<Myrl<int>>>);
    static_assert(!std::is_default_constructible_v<std::hash<Myrl<Unhashable>>>);
    return 0;
}
