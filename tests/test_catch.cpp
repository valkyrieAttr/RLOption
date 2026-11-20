
#include <iostream>
#include <utility>
#include <cassert>

struct ThrowFirst {
    ThrowFirst() = default;
    ThrowFirst(ThrowFirst&&) {
        throw 1;
    }
};

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
    ThrowSecond& operator=(ThrowSecond&&) = default;
};

ThrowSecond make_throw_second_move(ThrowSecond &&value) {
    return ThrowSecond{std::move(value)};
}

struct Testrl {
    ThrowSecond val;
    bool engaged = true;

    void reset() {
        engaged = false;
        std::cout << "reset called\n";
    }

    void emplace(ThrowSecond&& v) {
        val = std::move(v);
        engaged = true;
    }

    ThrowSecond take_current() {
        try {
            ThrowSecond result { std::move(val) };
            reset();
            return make_throw_second_move(std::move(result));
        } catch (...) {
            engaged = true; // stays engaged!
            throw;
        }
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
        assert(opt.engaged == true);
    }
    std::cout << "Success!\n";
}
