#include "rl_option.hpp"

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace {

struct none_marker {};

template <class... Ts>
struct filter_none;

template <>
struct filter_none<> {
    using type = std::tuple<>;
};

template <class First, class... Rest>
struct filter_none<First, Rest...> {
private:
    using rest_tuple = typename filter_none<Rest...>::type;

public:
    using type = std::conditional_t<
        std::same_as<std::remove_cvref_t<First>, none_marker>,
        rest_tuple,
        decltype(std::tuple_cat(std::declval<std::tuple<First>>(),
                                std::declval<rest_tuple>()))>;
};

template <class Tuple>
struct tuple_common_type;

template <>
struct tuple_common_type<std::tuple<>> {
    using type = none_marker;
};

template <class First, class... Rest>
struct tuple_common_type<std::tuple<First, Rest...>> {
    using type = std::common_type_t<First, Rest...>;
};

template <class T>
struct wrap_option {
    using type = rl::Option<T>;
};

template <class T, class... Rest>
struct wrap_option<rl::Option<T, Rest...>> {
    using type = rl::Option<T, Rest...>;
};

static_assert(std::same_as<
              typename filter_none<const char *, const char *, none_marker,
                                   const char *>::type,
              std::tuple<const char *, const char *, const char *>>);

using StringCommon = typename tuple_common_type<typename filter_none<
    const char *, const char *, none_marker, const char *>::type>::type;
static_assert(std::same_as<StringCommon, const char *>);
static_assert(std::same_as<typename wrap_option<StringCommon>::type,
                           rl::Option<const char *>>);

using IntCommon = typename tuple_common_type<
    typename filter_none<int, none_marker>::type>::type;
static_assert(std::same_as<IntCommon, int>);
static_assert(std::same_as<typename wrap_option<IntCommon>::type,
                           rl::Option<int>>);

static_assert(std::same_as<
              typename tuple_common_type<std::tuple<>>::type, none_marker>);
static_assert(std::same_as<typename wrap_option<none_marker>::type,
                           rl::Option<none_marker>>);

} // namespace

int main() {}
