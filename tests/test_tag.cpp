
namespace N {
    struct None_tag {};
    using None = None_tag;
}
// or struct None;
namespace M {
    struct None {};
    inline constexpr None None{};
}

int main() {
    (void)M::None; // refers to object
    [[maybe_unused]] struct M::None x; // refers to type
    // but M::None as a type without struct?
}
