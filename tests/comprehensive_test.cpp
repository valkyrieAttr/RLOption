#include "rl_option.hpp"
#include <cassert>
#include <string>
#include <memory>
#include <type_traits>
#include <vector>

// Move-only type
struct MoveOnly {
    int id;
    explicit MoveOnly(int i) noexcept : id(i) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& o) noexcept : id(o.id) { o.id = -1; }
    MoveOnly& operator=(MoveOnly&& o) noexcept {
        if (this != &o) {
            id = o.id;
            o.id = -1;
        }
        return *this;
    }
};

// Immovable default-constructible type
struct ImmovableDefault {
    int val = 42;
    ImmovableDefault() = default;
    ImmovableDefault(const ImmovableDefault&) = delete;
    ImmovableDefault& operator=(const ImmovableDefault&) = delete;
    ImmovableDefault(ImmovableDefault&&) = delete;
    ImmovableDefault& operator=(ImmovableDefault&&) = delete;
};

int main() {
    // =========================================================================
    // 1. Triviality traits for single-type Option<int>
    // =========================================================================
    using Singlerl = rl::Option<int>;
    static_assert(std::is_trivially_destructible_v<Singlerl>);
    static_assert(std::is_trivially_copy_constructible_v<Singlerl>);
    static_assert(std::is_trivially_move_constructible_v<Singlerl>);
    static_assert(std::is_trivially_copy_assignable_v<Singlerl>);
    static_assert(std::is_trivially_move_assignable_v<Singlerl>);

    // =========================================================================
    // 2. Triviality traits for multi-alternative Option<int, float, double>
    // =========================================================================
    using Multirl = rl::Option<int, float, double>;
    static_assert(std::is_trivially_destructible_v<Multirl>);
    static_assert(std::is_trivially_copy_constructible_v<Multirl>);
    static_assert(std::is_trivially_move_constructible_v<Multirl>);
    static_assert(std::is_trivially_copy_assignable_v<Multirl>);
    static_assert(std::is_trivially_move_assignable_v<Multirl>);

    // =========================================================================
    // 3. Non-trivial types correctly report non-trivial
    // =========================================================================
    using Stringrl = rl::Option<std::string, int>;
    static_assert(!std::is_trivially_destructible_v<Stringrl>);
    static_assert(!std::is_trivially_copy_constructible_v<Stringrl>);
    static_assert(!std::is_trivially_move_constructible_v<Stringrl>);
    static_assert(!std::is_trivially_copy_assignable_v<Stringrl>);
    static_assert(!std::is_trivially_move_assignable_v<Stringrl>);
    static_assert(std::is_copy_constructible_v<Stringrl>);
    static_assert(std::is_move_constructible_v<Stringrl>);

    // =========================================================================
    // 4. Move-only alternative types
    // =========================================================================
    using MoveOnlyrl = rl::Option<MoveOnly, int>;
    static_assert(!std::is_copy_constructible_v<MoveOnlyrl>);
    static_assert(!std::is_copy_assignable_v<MoveOnlyrl>);
    static_assert(std::is_move_constructible_v<MoveOnlyrl>);
    static_assert(std::is_move_assignable_v<MoveOnlyrl>);

    // =========================================================================
    // 5. Functional test of single-type copy & move
    // =========================================================================
    Singlerl s1 = rl::Some(42);
    Singlerl s2 = s1;
    assert(s2.unwrap() == 42);
    Singlerl s3 = std::move(s1);
    assert(s3.unwrap() == 42);

    // =========================================================================
    // 6. Functional test of multi-type copy & move
    // =========================================================================
    Multirl m1 = rl::Some(3.14f);
    assert(m1.is_some());
    assert(m1.is_some<float>());
    assert(!m1.is_some<int>());
    assert(!m1.is_some<double>());
    Multirl m2 = m1;
    assert(m2.is_some<float>());
    assert(m2.unwrap<float>() == 3.14f);
    Multirl m3 = std::move(m1);
    assert(m3.is_some<float>());
    assert(m3.unwrap<float>() == 3.14f);

    // =========================================================================
    // 7. Test operator>>= and pattern matching
    // =========================================================================
    rl::Option<int> opt = rl::Some(100);
    auto res1 = opt.match(
        [](auto x) { return x == 100; } >>= "hit-100",
        rl::Some(42) >>= "hit-42",
        rl::SomeAny >>= "other",
        rl::None >>= "none"
    );
    assert(std::string(res1) == "hit-100");

    // =========================================================================
    // 8. Test explicit Rp template argument
    // =========================================================================
    auto res2 = opt.template match<int>(
        [](auto x) { return x == 100; } >>= [] { return 1; },
        rl::Some(42) >>= [] { return 2; },
        rl::SomeAny >>= [] { return 3; },
        rl::None >>= [] { return -1; }
    );
    assert(res2 == 1);

    // =========================================================================
    // 9. Test None matching
    // =========================================================================
    rl::Option<int> none_opt = rl::None;
    auto res3 = none_opt.match(
        rl::SomeAny >>= "some",
        rl::None >>= "none"
    ).unwrap();
    assert(std::string(res3) == "none");

    // =========================================================================
    // 10. Test Multirl pattern matching
    // =========================================================================
    Multirl m_double = rl::Some(2.718);
    auto res4 = m_double.match(
        [](auto x) { return x > 2; } >>= "greater than 2",
        rl::SomeAny >>= "other",
        rl::None >>= "none"
    ).unwrap();
    assert(std::string(res4) == "greater than 2");

    // =========================================================================
    // 11. Test Default construction and assignment
    // =========================================================================
    rl::Option<int> def_opt = rl::Default;
    assert(def_opt.is_some());
    assert(def_opt.unwrap() == 0);
    assert(def_opt.is_default());

    rl::Option<ImmovableDefault> immov_opt = rl::Default;
    assert(immov_opt.is_some());
    assert(immov_opt.unwrap().val == 42);

    // =========================================================================
    // 12. Test Option<float> partial ordering
    // =========================================================================
    rl::Option<float> f1 = rl::Some(1.0f);
    rl::Option<float> f2 = rl::Some(2.0f);
    assert((f1 <=> f2) == std::partial_ordering::less);

    // =========================================================================
    // 13. Test member and free swap
    // =========================================================================
    Singlerl swap_a = rl::Some(1);
    Singlerl swap_b = rl::Some(2);
    swap_a.swap(swap_b);
    assert(swap_a.unwrap() == 2 && swap_b.unwrap() == 1);
    using std::swap;
    swap(swap_a, swap_b);
    assert(swap_a.unwrap() == 1 && swap_b.unwrap() == 2);

    // =========================================================================
    // 14. Test map, and_then, unwrap_or
    // =========================================================================
    rl::Option<int> o = rl::Some(5);
    auto mapped = o.map([](int x) { return x * 10; });
    assert(mapped.unwrap() == 50);

    auto chained = o.and_then([](int x) { return rl::Option<int>(rl::Some(x + 2)); });
    assert(chained.unwrap() == 7);

    rl::Option<int> empty = rl::None;
    assert(empty.unwrap_or(99) == 99);
    assert(empty.unwrap_or_else([] { return 101; }) == 101);

    return 0;
}
