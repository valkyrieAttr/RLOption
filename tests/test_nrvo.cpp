
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

struct Testrl {
    ThrowSecond val;
    bool engaged = true;

    void reset() {
        engaged = false;
        std::cout << "reset called\n";
    }

    ThrowSecond take_current() {
        ThrowSecond result { std::move(val) };
        reset();
        return result;
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
