
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc++98-compat"
    #pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma clang diagnostic ignored "-Wpre-c++14-compat"
    #pragma clang diagnostic ignored "-Wpre-c++17-compat"
    #pragma clang diagnostic ignored "-Wpre-c++20-compat"
    #pragma clang diagnostic ignored "-Wpre-c++23-compat"
    #pragma clang diagnostic ignored "-Wpre-c++26-compat"
    #pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#include "rl_option.hpp"
#include <cassert>
#include <concepts>
#include <iostream>
#include <type_traits>

struct Immovable {
    int x{42};
    Immovable() = default;
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
};

struct CopyOnly {
    int x{100};
    CopyOnly() = default;
    explicit CopyOnly(int v) : x(v) {}
    CopyOnly(const CopyOnly&) = default;
    CopyOnly(CopyOnly&&) = delete;
    CopyOnly& operator=(const CopyOnly&) = default;
    CopyOnly& operator=(CopyOnly&&) = delete;
    bool operator==(const CopyOnly& o) const { return x == o.x; }
};

struct ThrowMove {
    int x{0};
    ThrowMove() = default;
    ThrowMove(const ThrowMove&) = default;
    ThrowMove(ThrowMove&&) = default;
    ThrowMove& operator=(const ThrowMove&) noexcept { return *this; }
    ThrowMove& operator=(ThrowMove&&) noexcept(false) { return *this; }
};

struct Unhashable {
    int val;
};

static void test_immovable() {
    // 1. rl::Default constructor on immovable type
    rl::Option<Immovable> opt(rl::Default);
    assert(opt.is_some());
    assert(opt->x == 42);

    // 2. rl::Default assignment on immovable type
    opt = rl::None;
    assert(opt.is_none());
    opt = rl::Default;
    assert(opt.is_some());
    assert(opt->x == 42);
}

static void test_copy_only() {
    rl::Option<CopyOnly> opt(rl::Some(CopyOnly{99}));
    assert(opt.is_some());
    assert(opt->x == 99);

    // Default assignment on copy-only type
    opt = rl::Default;
    assert(opt.is_some());
    assert(opt->x == 100);
}

static void test_throw_move_noexcept() {
    rl::Option<ThrowMove> opt = rl::None;
    static_assert(!noexcept(opt = rl::Default), "ThrowMove move assignment must make noexcept false");
}

static void test_is_default() {
    rl::Option<int> opt(rl::Default);
    assert(opt.is_some());
    assert(opt.is_default());
    assert(*opt == 0);

    opt = rl::Some(5);
    assert(!opt.is_default());

    opt = rl::None;
    assert(!opt.is_default());
}

static void test_spaceship() {
    rl::Option<float> a = rl::Some(1.5f);
    rl::Option<float> b = rl::Some(2.5f);
    rl::Option<float> none = rl::None;

    // Option <=> Option
    auto r1 = (a <=> b);
    static_assert(std::same_as<decltype(r1), std::partial_ordering>);
    assert(r1 == std::partial_ordering::less);
    assert((b <=> a) == std::partial_ordering::greater);
    assert((a <=> a) == std::partial_ordering::equivalent);
    assert((none <=> a) == std::partial_ordering::less);
    assert((a <=> none) == std::partial_ordering::greater);
    assert((none <=> none) == std::partial_ordering::equivalent);

    // Option <=> None
    auto r2 = (a <=> rl::None);
    static_assert(std::same_as<decltype(r2), std::strong_ordering>);
    assert(r2 == std::strong_ordering::greater);
    assert((none <=> rl::None) == std::strong_ordering::equal);

    // None <=> Option
    auto r3 = (rl::None <=> a);
    static_assert(std::same_as<decltype(r3), std::strong_ordering>);
    assert(r3 == std::strong_ordering::less);
    assert((rl::None <=> none) == std::strong_ordering::equal);

    // Option <=> Default
    auto r4 = (a <=> rl::Default);
    static_assert(std::same_as<decltype(r4), std::partial_ordering>);
    assert(r4 == std::partial_ordering::greater);
    assert((none <=> rl::Default) == std::partial_ordering::less);

    // Default <=> Option
    auto r5 = (rl::Default <=> a);
    static_assert(std::same_as<decltype(r5), std::partial_ordering>);
    assert(r5 == std::partial_ordering::less);
    assert((rl::Default <=> none) == std::partial_ordering::greater);

    // Strong ordering type
    rl::Option<int> i1 = rl::Some(10);
    rl::Option<int> i2 = rl::Some(20);
    auto ri = (i1 <=> i2);
    static_assert(std::same_as<decltype(ri), std::strong_ordering>);
    assert(ri == std::strong_ordering::less);
}

static void test_swap() {
    rl::Option<int> x = rl::Some(1);
    rl::Option<int> y = rl::Some(2);

    swap(x, y);
    assert(*x == 2);
    assert(*y == 1);

    rl::Option<int> z = rl::None;
    swap(x, z);
    assert(x.is_none());
    assert(z.is_some() && *z == 2);

    swap(x, z);
    assert(x.is_some() && *x == 2);
    assert(z.is_none());
}

static void test_hash() {
    rl::Option<int> engaged = rl::Some(42);
    rl::Option<int> empty = rl::None;

    std::hash<rl::Option<int>> hasher;
    assert(hasher(engaged) == std::hash<int>{}(42));
    assert(hasher(empty) == static_cast<std::size_t>(-3333));

    // Const type
    rl::Option<const int> const_opt = rl::Some<const int>(42);
    std::hash<rl::Option<const int>> const_hasher;
    assert(const_hasher(const_opt) == std::hash<int>{}(42));

    // Non-hashable type must disable std::hash
    static_assert(!std::is_default_constructible_v<std::hash<rl::Option<Unhashable>>>);
}

static void test_pattern_matching() {
    rl::Option<int> opt = rl::Some(100);
    auto res = opt.match<std::string>(
        [](int x) { return x < 50; } >>= "small",
        [](int x) { return x >= 50; } >>= "large",
        rl::SomeAny >>= "fallback_some",
        rl::None >>= "empty"
    );
    assert(res == std::string{"large"});
}

int main() {
    test_immovable();
    test_copy_only();
    test_throw_move_noexcept();
    test_is_default();
    test_spaceship();
    test_swap();
    test_hash();
    test_pattern_matching();
    std::cout << "All bulletproof tests passed successfully!\n";
    return 0;
}
