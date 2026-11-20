#include <iostream>

struct S {
    template<typename... Rp, typename Self, typename... Arms>
    void test(this Self&&, Arms&&...) {
        std::cout << "Rp: " << sizeof...(Rp) << " Arms: " << sizeof...(Arms) << std::endl;
    }
};

int main() {
    S s;
    s.test<int, double>(1, 2);
    s.test<int>(1, 2);
    s.test(1, 2);
    s.test<int, double, char, float, long>(1, 2, 3);
}
