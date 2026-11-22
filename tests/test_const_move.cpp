
#include <rl_option.hpp>

#include <iostream>
#include <utility>
#include <type_traits>
#include <cassert>
#include <memory>

struct ThrowMoveNoexceptCopy {
    int v;
    ThrowMoveNoexceptCopy(int x) : v(x) {}
    ThrowMoveNoexceptCopy(const ThrowMoveNoexceptCopy& o) noexcept : v(o.v) {
        std::cout << "Copy ctor called (noexcept)\n";
    }
    ThrowMoveNoexceptCopy(ThrowMoveNoexceptCopy&&) noexcept(false) {
        std::cout << "Move ctor called (can throw)\n";
        throw 99;
    }
};

template<typename Qp_>
union Storage {
    std::remove_cv_t<Qp_> value;
    constexpr Storage() {}
    constexpr Storage(int x) : value(x) {}
    constexpr ~Storage() {}
};

template<typename Qp_>
struct OptionalMove {
    Storage<Qp_> storage;
    bool is_some;
    [[no_unique_address]] rl::details::PadToAlignment_<
        rl::details::align_up_(sizeof(Storage<Qp_>), alignof(bool)) +
            sizeof(bool),
        rl::details::max_alignof_<Storage<Qp_>, bool>()> tail_padding{};

    constexpr OptionalMove(int x) : storage{x}, is_some(true) {}

    constexpr OptionalMove(OptionalMove&& other)
        noexcept(std::is_nothrow_move_constructible_v<Qp_>)
        requires (std::is_move_constructible_v<Qp_>)
        : storage{}, is_some(false)
    {
        if (other.is_some) {
            std::construct_at(
                std::addressof(storage.value),
                static_cast<Qp_&&>(other.storage.value)
            );
            is_some = true;
        }
    }
};

int main() {
    using ConstType = const ThrowMoveNoexceptCopy;
    static_assert(std::is_nothrow_move_constructible_v<ConstType>);
    static_assert(std::is_nothrow_move_constructible_v<OptionalMove<ConstType>>);

    OptionalMove<ConstType> opt1{42};
    std::cout << "Moving opt1...\n";
    OptionalMove<ConstType> opt2{std::move(opt1)};
    assert(opt2.storage.value.v == 42);
    std::cout << "Successfully moved Option<const T> without termination!\n";
}
