
template<typename T>
concept FalseConcept = false;

struct S {
    template<typename F>
    requires FalseConcept<F>
    void f(F);
};

template<typename T, typename F>
concept HasF = requires(T t, F f) { t.f(f); };

int main() {
    static_assert(!HasF<S, int>);
}
