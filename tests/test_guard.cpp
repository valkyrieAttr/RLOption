
#include <rl_option.hpp>

#include <iostream>
#include <utility>
#include <cassert>
#include <exception>

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

    struct ResetOnSuccess_ {
        Testrl* owner_;
        int initial_uncaught_ = 0;
        [[no_unique_address]] rl::details::PadToAlignment_<
            rl::details::align_up_(sizeof(Testrl *), alignof(int)) +
                sizeof(int),
            rl::details::max_alignof_<Testrl *, int>()> tail_padding{};

        explicit ResetOnSuccess_(Testrl* owner) noexcept : owner_(owner) {
            initial_uncaught_ = std::uncaught_exceptions();
        }

        ~ResetOnSuccess_() noexcept {
            if (std::uncaught_exceptions() > initial_uncaught_) {
                std::cout << "Exception in flight! Staying engaged!\n";
                return; // DO NOT RESET!
            }
            owner_->reset();
        }
    };

    ThrowSecond take_current() {
        ResetOnSuccess_ guard{this};
        return std::move(val);
    }
};

int main() {
    Testrl opt{ThrowSecond{1}};
    ThrowSecond::moves = 1; // next move is move 2, which will throw!
    try {
        ThrowSecond x = opt.take_current();
        static_cast<void>(x);
        assert(false);
    } catch (...) {
        std::cout << "Caught! opt.engaged = " << opt.engaged << "\n";
        assert(opt.engaged == true);
    }

    // Now test success case:
    Testrl opt2{ThrowSecond{2}};
    ThrowSecond::moves = 0; // will not throw
    ThrowSecond y = opt2.take_current();
    static_cast<void>(y);
    std::cout << "opt2.engaged = " << opt2.engaged << "\n";
    assert(opt2.engaged == false);

    std::cout << "ALL PASSED!\n";
}
