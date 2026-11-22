
#include "rl_option.hpp"
#include <cassert>
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// Issue 1: Const-qualified move
struct ThrowMoveNoexceptCopy {
    int v{0};
    static inline int copy_count = 0;
    static inline int move_count = 0;

    explicit ThrowMoveNoexceptCopy(int x) : v(x) {}
    ThrowMoveNoexceptCopy(const ThrowMoveNoexceptCopy& o) noexcept : v(o.v) {
        copy_count++;
    }

    [[noreturn]]
    ThrowMoveNoexceptCopy(ThrowMoveNoexceptCopy&&) noexcept(false) {
        move_count++;
        throw 99;
    }
};

void test_const_move() {
    using ConstT = const ThrowMoveNoexceptCopy;
    static_assert(std::is_nothrow_move_constructible_v<ConstT>);
    static_assert(std::is_nothrow_move_constructible_v<rl::Option<ConstT>>);

    ThrowMoveNoexceptCopy::copy_count = 0;
    ThrowMoveNoexceptCopy::move_count = 0;

    rl::Option<ConstT> opt1 = rl::Some<ConstT>(ThrowMoveNoexceptCopy{123});
    assert(opt1.is_some());

    // Moving Option<const T> must invoke copy constructor and NEVER throw or terminate!
    rl::Option<ConstT> opt2 = std::move(opt1);
    assert(opt2.is_some());
    assert(opt2->v == 123);
    assert(ThrowMoveNoexceptCopy::copy_count >= 1);
    assert(ThrowMoveNoexceptCopy::move_count == 0);
    std::cout << "[PASS] Issue 1: Option<const T> move semantics\n";
}

// Issue 2: ADL Swap
namespace CustomNS {
    struct SwappableWithDeletedMoveAssign {
        int val{0};
        static inline bool swap_called = false;

        explicit SwappableWithDeletedMoveAssign(int v) : val(v) {}
        SwappableWithDeletedMoveAssign(SwappableWithDeletedMoveAssign&&) = default;
        SwappableWithDeletedMoveAssign& operator=(SwappableWithDeletedMoveAssign&&) = delete;

        friend void swap(SwappableWithDeletedMoveAssign& a, SwappableWithDeletedMoveAssign& b) noexcept {
            swap_called = true;
            std::swap(a.val, b.val);
        }
    };
}

void test_adl_swap() {
    using T = CustomNS::SwappableWithDeletedMoveAssign;
    static_assert(std::is_swappable_v<T>);
    static_assert(!std::is_move_assignable_v<T>);

    rl::Option<T> a = rl::Some(T{10});
    rl::Option<T> b = rl::Some(T{20});

    CustomNS::SwappableWithDeletedMoveAssign::swap_called = false;
    a.swap(b);

    assert(CustomNS::SwappableWithDeletedMoveAssign::swap_called);
    assert(a->val == 20);
    assert(b->val == 10);

    // Free swap (ADL)
    CustomNS::SwappableWithDeletedMoveAssign::swap_called = false;
    swap(a, b);
    
    assert(CustomNS::SwappableWithDeletedMoveAssign::swap_called);
    assert(a->val == 10);
    assert(b->val == 20);
    std::cout << "[PASS] Issue 2: ADL-aware swap\n";
}

// Issue 3: Single move & success-only reset in take_unchecked()
struct ExtractionTestType {
    static inline int move_count = 0;
    static inline bool should_throw = false;
    int id{0};

    explicit ExtractionTestType(int i) : id(i) {}
    ExtractionTestType(ExtractionTestType&& other) noexcept(false) : id(other.id) {
        move_count++;
        if (should_throw) {
            throw 42;
        }
    }
};

void test_take_unchecked_success_only_reset() {
    // 1. Success case: single move during unwrap_unchecked()
    {
        ExtractionTestType::should_throw = false;
        rl::Option<ExtractionTestType> opt = rl::Some(ExtractionTestType{1});
        ExtractionTestType::move_count = 0; // reset counter before extraction

        ExtractionTestType val = std::move(opt).unwrap_unchecked();
        assert(val.id == 1);
        assert(ExtractionTestType::move_count == 1); // exactly 1 move!
        assert(opt.is_none()); // reset on success!
    }

    // 2. Exception case: if extraction move throws, option stays engaged!
    {
        ExtractionTestType::should_throw = false;
        rl::Option<ExtractionTestType> opt = rl::Some(ExtractionTestType{2});
        assert(opt.is_some());

        ExtractionTestType::should_throw = true; // make extraction throw!
        try {
            ExtractionTestType val = std::move(opt).unwrap_unchecked();
            (void)val;
            assert(false);
        } catch (int e) {
            assert(e == 42);
            assert(opt.is_some()); // STAYS ENGAGED!
            assert(opt->id == 2);
        }
    }
    std::cout << "[PASS] Issue 3: take_unchecked single move & success-only reset\n";
}

// Issue 4 & 5: Default comparisons with deleted rvalue comparisons & is_default noexcept
struct NoRvalueCompare {
    int val{0};

    NoRvalueCompare() = default;
    explicit NoRvalueCompare(int v) : val(v) {}

    bool operator==(const NoRvalueCompare& o) const noexcept { return val == o.val; }
    bool operator==(NoRvalueCompare&&) const = delete;

    auto operator<=>(const NoRvalueCompare& o) const noexcept { return val <=> o.val; }
    auto operator<=>(NoRvalueCompare&&) const = delete;
};

void test_default_comparisons() {
    static_assert(std::equality_comparable<NoRvalueCompare>);
    static_assert(std::three_way_comparable<NoRvalueCompare>);

    rl::Option<NoRvalueCompare> opt(rl::Default);
    assert(opt.is_some());
    assert(opt.is_default());
    assert(opt == rl::Default);
    assert(rl::Default == opt);
    assert((opt <=> rl::Default) == std::strong_ordering::equal);
    assert((rl::Default <=> opt) == std::strong_ordering::equal);

    // Some<T> == Default and Default == Some<T>
    auto some = rl::Some(NoRvalueCompare{0});
    assert(some == rl::Default);
    assert(rl::Default == some);

    // is_default noexcept test
    static_assert(noexcept(opt.is_default()));

    std::cout << "[PASS] Issues 4 & 5: Default comparison rvalue safety & is_default noexcept\n";
}

int main() {
    test_const_move();
    test_adl_swap();
    test_take_unchecked_success_only_reset();
    test_default_comparisons();
    std::cout << "\nALL 5 ISSUES VERIFIED AND FIXED SUCCESSFULLY!\n";
    return 0;
}
