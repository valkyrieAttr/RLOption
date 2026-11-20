
namespace rl {
    struct None {};
    template<typename T> struct Option {
        Option(None) {}
    };
}
rl::Option<int> f() {
    return rl::None{};
}

int main() {
    auto opt = f();
    static_cast<void>(opt);
    return 0;
}
