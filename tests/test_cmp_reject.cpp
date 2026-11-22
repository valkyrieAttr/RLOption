#include <compare>
#include <concepts>
#include <type_traits>

namespace {

template <class T_>
struct compare_three_way_result_for_test_ {
    using type = void;
};

template <class T_>
    requires std::three_way_comparable<T_>
struct compare_three_way_result_for_test_<T_> {
    using type = std::compare_three_way_result_t<T_>;
};

template <class T_>
using compare_three_way_result_for_test_t_ =
    typename compare_three_way_result_for_test_<T_>::type;

template <class T_>
struct MyOption_ {
    T_ value_{};

    [[nodiscard]] constexpr auto is_some() const noexcept -> bool {
        return true;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> const T_ & {
        return value_;
    }

    friend constexpr auto operator<=>(const MyOption_ &lhs_,
                                      const MyOption_ &rhs_)
        -> compare_three_way_result_for_test_t_<T_>
        requires std::three_way_comparable<T_>
    {
        if (lhs_.is_some() && rhs_.is_some()) {
            return *lhs_ <=> *rhs_;
        }
        return lhs_.is_some() <=> rhs_.is_some();
    }
};

struct Immovable_ {};

static_assert(!std::three_way_comparable<MyOption_<Immovable_>>);

} // namespace

int main() {}
