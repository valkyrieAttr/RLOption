
#include <functional>
#include <type_traits>

constexpr auto test() {
    return std::hash<int>{}(42);
}

int main() {
    return test() == 0 ? 0 : 0;
}
