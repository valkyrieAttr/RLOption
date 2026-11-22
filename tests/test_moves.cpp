
#include <rl_option.hpp>

#include <iostream>
#include <utility>

struct ThrowSecond {
    static inline int moves = 0;
    int id;
    ThrowSecond(int i) : id(i) {}
    ThrowSecond(ThrowSecond&& other) : id(other.id) {
        moves++;
        std::cout << "Move " << moves << "\n";
        if (moves == 2) {
            throw 42;
        }
    }
};

ThrowSecond make_throw_second_move(ThrowSecond &&value) {
    return ThrowSecond{std::move(value)};
}

struct Testrl {
    ThrowSecond val;
    bool engaged = true;
    [[no_unique_address]] rl::details::PadToAlignment_<
        rl::details::align_up_(sizeof(ThrowSecond), alignof(bool)) +
            sizeof(bool),
        rl::details::max_alignof_<ThrowSecond, bool>()> tail_padding{};

    void reset() {
        engaged = false;
        std::cout << "reset called\n";
    }

    ThrowSecond take_current() {
        ThrowSecond result{std::move(val)};
        reset();
        return make_throw_second_move(std::move(result));
    }
};

int main() {
    Testrl opt{ThrowSecond{1}};
    ThrowSecond::moves = 0;
    try {
        ThrowSecond x = opt.take_current();
        static_cast<void>(x);
    } catch (...) {
        std::cout << "Caught! opt.engaged = " << opt.engaged << "\n";
    }
}
