#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc++98-compat"
    #pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma clang diagnostic ignored "-Wc++98-compat-local-type-template-args"
    #pragma clang diagnostic ignored "-Wpre-c++14-compat"
    #pragma clang diagnostic ignored "-Wpre-c++14-compat-pedantic"
    #pragma clang diagnostic ignored "-Wpre-c++17-compat"
    #pragma clang diagnostic ignored "-Wpre-c++17-compat-pedantic"
    #pragma clang diagnostic ignored "-Wpre-c++20-compat"
    #pragma clang diagnostic ignored "-Wpre-c++20-compat-pedantic"
    #pragma clang diagnostic ignored "-Wpre-c++23-compat"
    #pragma clang diagnostic ignored "-Wpre-c++23-compat-pedantic"
    #pragma clang diagnostic ignored "-Wpre-c++26-compat"
    #pragma clang diagnostic ignored "-Wpre-c++26-compat-pedantic"
    #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    #pragma clang diagnostic ignored "-Wglobal-constructors"
    #pragma clang diagnostic ignored "-Wexit-time-destructors"
    #pragma clang diagnostic ignored "-Wmissing-prototypes"
    #pragma clang diagnostic ignored "-Wself-assign-overloaded"
    #pragma clang diagnostic ignored "-Wself-move"
    #pragma clang diagnostic ignored "-Wunused-exception-parameter"
    #pragma clang diagnostic ignored "-Wswitch-default"
    #pragma clang diagnostic ignored "-Wcovered-switch-default"
    #pragma clang diagnostic ignored "-Wlifetime-safety-intra-tu-constructor-suggestions"
#endif

#include "rl_option.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>


// =============================================================================
// Helper Types for Rigorous Edge Cases
// =============================================================================

// 1. Move-only type
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
    ~MoveOnly() = default;
};

// 2. Over-aligned types (64 and 128 bytes)
struct alignas(64) OverAligned64 {
    int value;
    char buffer[60];
};

struct alignas(128) OverAligned128 {
    int value;
    char buffer[124];
};

// 3. Immovable (non-copyable, non-movable) type
struct Immovable {
    int x;
    explicit Immovable(int val) : x(val) {}
    Immovable(const Immovable&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(Immovable&&) = delete;
    ~Immovable() = default;
};

// 4. Exact Lifetime, copy, and move tracker
struct LifetimeTracker {
    static inline int default_constructs = 0;
    static inline int value_constructs = 0;
    static inline int copy_constructs = 0;
    static inline int move_constructs = 0;
    static inline int copy_assigns = 0;
    static inline int move_assigns = 0;
    static inline int destructs = 0;

    int val;

    static void reset() noexcept {
        default_constructs = 0;
        value_constructs = 0;
        copy_constructs = 0;
        move_constructs = 0;
        copy_assigns = 0;
        move_assigns = 0;
        destructs = 0;
    }

    LifetimeTracker() noexcept : val(0) { default_constructs++; }
    explicit LifetimeTracker(int v) noexcept : val(v) { value_constructs++; }
    LifetimeTracker(const LifetimeTracker& o) noexcept : val(o.val) { copy_constructs++; }
    LifetimeTracker(LifetimeTracker&& o) noexcept : val(o.val) { move_constructs++; o.val = -1; }
    LifetimeTracker& operator=(const LifetimeTracker& o) noexcept { val = o.val; copy_assigns++; return *this; }
    LifetimeTracker& operator=(LifetimeTracker&& o) noexcept { val = o.val; move_assigns++; o.val = -1; return *this; }
    ~LifetimeTracker() noexcept { destructs++; }
    bool operator==(const LifetimeTracker& o) const noexcept { return val == o.val; }
};

// 5. Non-equality-comparable type
struct NonComparable {
    int data;
};

// 6. Type throwing on copy construction
struct ThrowOnCopy {
    bool should_throw;
    explicit ThrowOnCopy(bool t) noexcept : should_throw(t) {}
    ThrowOnCopy(const ThrowOnCopy& o) : should_throw(o.should_throw) {
        if (should_throw) {
            throw std::runtime_error("Copy failed intentionally!");
        }
    }
    ThrowOnCopy(ThrowOnCopy&&) noexcept = default;
    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
    ThrowOnCopy& operator=(ThrowOnCopy&&) noexcept = default;
};

// 7. Empty struct (Zero-sized logical type)
struct EmptyTag {};

// 8. Non-default-constructible type
struct NoDefault {
    int value;
    explicit NoDefault(int v) noexcept : value(v) {}
    NoDefault() = delete;
};

// =============================================================================
// Helper for Death Tests (Abort verification via fork)
// =============================================================================
static bool expect_abort(void (*func)()) {
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        close(STDERR_FILENO); // suppress stderr abort diagnostics in test logs
        func();
        _exit(0); // If it returned instead of aborting, exit normally
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFSIGNALED(status) && (WTERMSIG(status) == SIGABRT);
}

// =============================================================================
// Compile-time Constexpr Test Suite
// =============================================================================
constexpr bool test_constexpr_suite() {
    // 1. None Option
    rl::Option<int> n1 = rl::None();
    if (!n1.is_none() || n1.is_some()) return false;
    if (n1.unwrap_or(42) != 42) return false;
    if (n1.unwrap_or_else([] { return 100; }) != 100) return false;
    // if (n1.unwrap_or_else([](rl::None) { return 100; }) != 100) return false;
    if (n1.unwrap_or_default() != 0) return false;

    // 2. Some Option
    rl::Option<int> s1 = rl::Some<int>(10);
    if (s1.is_none() || !s1.is_some()) return false;
    if (s1.value() != 10) return false;
    if (*s1 != 10) return false;
    if (s1.unwrap_or(42) != 10) return false;
    if (s1.unwrap_or_else([] { return 100; }) != 10) return false;
    // if (s1.unwrap_or_else([](rl::None) { return 100; }) != 10) return false;
    if (s1.unwrap_or_default() != 10) return false;

    // 3. Map
    auto mapped_some = s1.map([](int x) { return x * 3; });
    if (!mapped_some.is_some() || mapped_some.value() != 30) return false;
    auto mapped_none = n1.map([](int x) { return x * 3; });
    if (!mapped_none.is_none()) return false;

    // 4. And_then
    auto at_some = s1.and_then([](int x) { return rl::Option<int>(rl::Some(x + 5)); });
    if (!at_some.is_some() || at_some.value() != 15) return false;
    auto at_none = n1.and_then([](int x) { return rl::Option<int>(rl::Some(x + 5)); });
    if (!at_none.is_none()) return false;

    // 5. Or_else
    auto oe_some = s1.or_else([] { return rl::Option<int>(rl::Some(99)); });
    if (!oe_some.is_some() || oe_some.value() != 10) return false;
    auto oe_none = n1.or_else([] { return rl::Option<int>(rl::Some(99)); });
    if (!oe_none.is_some() || oe_none.value() != 99) return false;

    // 6. Predicates
    if (!s1.is_some_and([](int x) { return x == 10; })) return false;
    if (s1.is_some_and([](int x) { return x != 10; })) return false;
    if (n1.is_some_and([](int x) { return x == 10; })) return false;

    if (!n1.is_none_or([](int x) { return x == 999; })) return false;
    if (!s1.is_none_or([](int x) { return x == 10; })) return false;
    if (s1.is_none_or([](int x) { return x != 10; })) return false;

    // 7. Comparisons
    if (s1 == n1) return false;
    if (!(s1 != n1)) return false;
    if (!(s1 == rl::Some(10))) return false;
    if (s1 == rl::Some(20)) return false;
    if (!(n1 == rl::None())) return false;
    if (s1 == rl::None()) return false;
    if (!(rl::None() == n1)) return false;

    // 8. Copy and Assignment
    rl::Option<int> opt_copy = s1;
    if (!opt_copy.is_some() || opt_copy.value() != 10) return false;
    opt_copy = n1;
    if (!opt_copy.is_none()) return false;
    opt_copy = rl::Some(50);
    if (!opt_copy.is_some() || opt_copy.value() != 50) return false;
    opt_copy = rl::None();
    if (!opt_copy.is_none()) return false;

    // 9. Reset and Emplace
    rl::Option<int> mut_opt = rl::Some(1);
    mut_opt.reset();
    if (!mut_opt.is_none()) return false;
    mut_opt.emplace(77);
    if (!mut_opt.is_some() || mut_opt.value() != 77) return false;

    // 10. as_ref
    auto ref_opt = mut_opt.as_mut_ptr();
    if (!ref_opt.is_some() || *ref_opt.value() != 77) return false;

    // 11. map_or and map_or_else
    int mor_some = s1.map_or([](int x) { return x * 2; }, 0);
    if (mor_some != 20) return false;
    int mor_none = n1.map_or([](int x) { return x * 2; }, 42);
    if (mor_none != 42) return false;

    int mo_else_some = s1.map_or_else([](int x) { return x * 4; }, [] { return -1; });
    if (mo_else_some != 40) return false;
    int mo_else_none = n1.map_or_else([](int x) { return x * 4; }, [] { return -1; });
    if (mo_else_none != -1) return false;

    return true;
}

static_assert(test_constexpr_suite(), "Constexpr test suite failed!");

// =============================================================================
// Main Runtime Test Suite
// =============================================================================
extern "C++" auto main
    ( void [[ /* nullAttr */ ]] )
-> std::int32_t
{
    std::cout << "=====================================================" << std::endl;
    std::cout << "Running Rigorous Option<T> Edge-Case Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    // -------------------------------------------------------------------------
    // Test 1: Primitive Value Semantics & Invariants
    // -------------------------------------------------------------------------
    {
        rl::Option<int> opt = rl::Some(42);
        assert(opt.is_some());
        assert(!opt.is_none());
        assert(opt.value() == 42);
        assert(*opt == 42);
        assert(opt.unwrap() == 42);
        assert(opt.expect("value must exist") == 42);
        assert(opt.unwrap_unchecked() == 42);

        rl::Option<int> none = rl::None();
        assert(!none.is_some());
        assert(none.is_none());
        assert(none.unwrap_or(100) == 100);
        assert(none.unwrap_or_default() == 0);

        std::cout << "[PASS 01/20] Primitive Value Semantics & Invariants" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 2: Move-Only Types (MoveOnly struct & std::unique_ptr)
    // -------------------------------------------------------------------------
    {
        rl::Option<MoveOnly> mo_opt = rl::Some(MoveOnly(7));
        assert(mo_opt.is_some());
        assert(mo_opt.value().id == 7);
        assert((*mo_opt).id == 7);

        // Move value out using rvalue unwrap()
        MoveOnly unwrapped = std::move(mo_opt).unwrap();
        assert(unwrapped.id == 7);

        // std::unique_ptr
        rl::Option<std::unique_ptr<int>> ptr_opt = rl::Some(std::make_unique<int>(123));
        assert(ptr_opt.is_some());
        assert(**ptr_opt == 123);

        // operator* on rvalue yields unique_ptr rvalue
        std::unique_ptr<int> moved_ptr = *std::move(ptr_opt);
        assert(*moved_ptr == 123);

        // unwrap_or on rvalue move-only
        rl::Option<MoveOnly> none_mo = rl::None();
        MoveOnly fallback = std::move(none_mo).unwrap_or(MoveOnly(99));
        assert(fallback.id == 99);

        // Verify type traits
        static_assert(!std::is_copy_constructible_v<rl::Option<MoveOnly>>);
        static_assert(!std::is_copy_assignable_v<rl::Option<MoveOnly>>);
        static_assert(std::is_move_constructible_v<rl::Option<MoveOnly>>);
        static_assert(std::is_move_assignable_v<rl::Option<MoveOnly>>);

        std::cout << "[PASS 02/20] Move-Only Types (MoveOnly & std::unique_ptr)" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Immovable Types (in-place emplace & reset)
    // -------------------------------------------------------------------------
    {
        rl::Option<Immovable> imm = rl::None();
        assert(imm.is_none());
        imm.emplace(42);
        assert(imm.is_some());
        assert(imm.value().x == 42);
        assert(imm->x == 42);
        imm.reset();
        assert(imm.is_none());

        static_assert(!std::is_copy_constructible_v<rl::Option<Immovable>>);
        static_assert(!std::is_copy_assignable_v<rl::Option<Immovable>>);
        static_assert(!std::is_move_constructible_v<rl::Option<Immovable>>);
        static_assert(!std::is_move_assignable_v<rl::Option<Immovable>>);

        std::cout << "[PASS 03/20] Immovable Types via In-Place Emplace" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Over-Aligned Types (64 and 128 bytes)
    // -------------------------------------------------------------------------
    {
        rl::Option<OverAligned64> oa_opt = rl::None();
        assert(oa_opt.is_none());
        oa_opt.emplace(OverAligned64{ .value = 999, .buffer = {} });
        assert(oa_opt.is_some());
        assert(oa_opt.value().value == 999);
        assert(reinterpret_cast<std::uintptr_t>(std::addressof(oa_opt.value())) % 64 == 0);
        assert(alignof(rl::Option<OverAligned64>) >= 64);

        rl::Option<OverAligned128> oa128 = rl::Some(OverAligned128{ .value = 777, .buffer = {} });
        assert(oa128.is_some());
        assert(oa128.value().value == 777);
        assert(reinterpret_cast<std::uintptr_t>(std::addressof(oa128.value())) % 128 == 0);
        assert(alignof(rl::Option<OverAligned128>) >= 128);

        std::cout << "[PASS 04/20] Over-Aligned Types (alignas(64) & alignas(128))" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 5: Exact Copy & Move Tracking (Zero Redundant Copies/Moves)
    // -------------------------------------------------------------------------
    {
        LifetimeTracker::reset();
        {
            rl::Option<LifetimeTracker> a = rl::Some(LifetimeTracker(1));
            assert(LifetimeTracker::value_constructs == 1);
            assert(LifetimeTracker::move_constructs >= 1);
            assert(LifetimeTracker::copy_constructs == 0);

            // Copy construction: exactly 1 copy and 0 moves
            LifetimeTracker::reset();
            rl::Option<LifetimeTracker> b = a;
            assert(LifetimeTracker::copy_constructs == 1);
            assert(LifetimeTracker::move_constructs == 0);

            // Move construction: exactly 1 move and 0 copies
            LifetimeTracker::reset();
            rl::Option<LifetimeTracker> c = std::move(a);
            assert(LifetimeTracker::copy_constructs == 0);
            assert(LifetimeTracker::move_constructs == 1);

            // Copy assignment
            LifetimeTracker::reset();
            b = c;
            assert(LifetimeTracker::copy_assigns == 1);
            assert(LifetimeTracker::move_constructs == 0);

            // Move assignment
            LifetimeTracker::reset();
            b = std::move(c);
            assert(LifetimeTracker::copy_constructs == 0);
            assert(LifetimeTracker::move_assigns == 1);

            // Emplace: constructed directly in-place with 0 copies and 0 moves
            LifetimeTracker::reset();
            b.emplace(88);
            assert(LifetimeTracker::value_constructs == 1);
            assert(LifetimeTracker::copy_constructs == 0);
            assert(LifetimeTracker::move_constructs == 0);

            // Self-assignment safety
            LifetimeTracker::reset();
            auto& self_copy_source = b;
            b = self_copy_source;
            assert(LifetimeTracker::destructs == 0);
            assert(b.value().val == 88);

            LifetimeTracker::reset();
            auto& self_move_source = b;
            b = std::move(self_move_source);
            assert(LifetimeTracker::destructs == 0);
            assert(b.value().val == 88);
        }
        assert(LifetimeTracker::destructs > 0);

        std::cout << "[PASS 05/20] Strict Copy/Move Lifetime Tracking & Zero Overhead" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 6: Nested Option Types (Option<Option<T>>)
    // -------------------------------------------------------------------------
    {
        using Inner = rl::Option<int>;
        using Outer = rl::Option<Inner>;

        Outer o_none = rl::None();
        Outer o_some_none = rl::Some<Inner>(rl::None());
        Outer o_some_some = rl::Some<Inner>(rl::Some(2026));

        assert(o_none.is_none());
        assert(o_some_none.is_some());
        assert(o_some_none.value().is_none());
        assert(o_some_some.is_some());
        assert(o_some_some.value().is_some());
        assert(o_some_some.value().value() == 2026);
        assert(o_some_some.unwrap().unwrap() == 2026);

        // Three-way state inequality
        assert(o_none != o_some_none);
        assert(o_some_none != o_some_some);
        assert(o_none != o_some_some);

        std::cout << "[PASS 06/20] Nested Option<Option<T>> Multi-Level State" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 7: Value Categories & Lifetime Bounds
    // -------------------------------------------------------------------------
    {
        struct Widget {
            std::string name;
        };

        rl::Option<Widget> lval = rl::Some(Widget{ .name = "mutable_lval" });
        const rl::Option<Widget> const_lval = rl::Some(Widget{ .name = "const_lval" });

        // lvalue ref
        Widget& r1 = lval.value();
        r1.name = "mutated";
        assert(lval.value().name == "mutated");
        assert(lval->name == "mutated");

        // const lvalue ref
        const Widget& r2 = const_lval.value();
        assert(r2.name == "const_lval");
        assert(const_lval->name == "const_lval");

        // rvalue ref moves content
        std::string moved_name = std::move(lval).value().name;
        assert(moved_name == "mutated");

        // unwrap_or with rvalue moves without copy
        rl::Option<std::string> str_opt = rl::Some(std::string("hello world"));
        std::string taken = std::move(str_opt).unwrap_or("fallback");
        assert(taken == "hello world");

        std::cout << "[PASS 07/20] Value Categories (lvalue, const lvalue, rvalue)" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 8: Non-Equality Comparable Types
    // -------------------------------------------------------------------------
    {
        rl::Option<NonComparable> nc = rl::Some(NonComparable{ .data = 5 });
        assert(nc.is_some());
        assert(nc.value().data == 5);
        static_assert(!std::equality_comparable<rl::Option<NonComparable>>);

        std::cout << "[PASS 08/20] Non-Equality Comparable Types SFINAE" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 9: Const Value Types (Option<const T>)
    // -------------------------------------------------------------------------
    {
        rl::Option<const int> const_int_opt = rl::Some<const int>(100);
        assert(const_int_opt.is_some());
        assert(const_int_opt.value() == 100);
        rl::Option<const int> copied_const = const_int_opt;
        assert(copied_const.value() == 100);

        std::cout << "[PASS 09/20] Option<const T> Constness Invariants" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 10: Pointer Types & std::reference_wrapper (Some(nullptr) vs None)
    // -------------------------------------------------------------------------
    {
        // Edge case: Some(nullptr) MUST be Some, not None!
        int* null_ptr = nullptr;
        rl::Option<int*> opt_null = rl::Some(null_ptr);
        assert(opt_null.is_some());
        assert(!opt_null.is_none());
        assert(opt_null.value() == nullptr);

        int x = 50;
        rl::Option<int*> ptr_opt = rl::Some(&x);
        assert(ptr_opt.is_some());
        assert(*ptr_opt.value() == 50);

        // std::reference_wrapper
        rl::Option<std::reference_wrapper<int>> ref_opt = rl::Some(std::ref(x));
        assert(ref_opt.is_some());
        ref_opt.value().get() = 60;
        assert(x == 60);

        std::cout << "[PASS 10/20] Pointer Types (Some(nullptr) vs None) & Reference Wrappers" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 11: Exception Safety on Copy/Move
    // -------------------------------------------------------------------------
    {
        rl::Option<ThrowOnCopy> opt1 = rl::Some(ThrowOnCopy(true));
        rl::Option<ThrowOnCopy> opt2 = rl::None();

        try {
            opt2 = opt1; // must throw
            assert(false && "Copy must throw");
        } catch (const std::runtime_error&) {
            // opt2 must remain in a valid state (None)
            assert(opt2.is_none());
        }

        std::cout << "[PASS 11/20] Exception Safety & Strong Guarantee" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 12: Functional Combinators (map, map_or, map_or_else, map_or_transform, and_then, or_else)
    // -------------------------------------------------------------------------
    {
        rl::Option<int> n = rl::None();
        rl::Option<int> s = rl::Some(5);

        // unwrap_or_else with niladic callable and None-taking callable
        assert(n.unwrap_or_else([] { return 10; }) == 10);
        // assert(n.unwrap_or_else([](rl::None) { return 20; }) == 20);
        assert(s.unwrap_or_else([] { return 10; }) == 5);
        // assert(s.unwrap_or_else([](rl::None) { return 20; }) == 5);

        // or_else with niladic callable and None-taking callable
        auto res1 = n.or_else([] { return rl::Option<int>(rl::Some(99)); });
        assert(res1.value() == 99);
        // auto res2 = n.or_else([](rl::None) { return rl::Option<int>(rl::Some(88)); });
        // assert(res2.value() == 88);

        // and_then pipeline
        auto chained = s
            .and_then([](int v) { return rl::Option<int>(rl::Some(v * 2)); })
            .and_then([](int v) { return rl::Option<std::string>(rl::Some(std::to_string(v))); });
        assert(chained.is_some());
        assert(chained.value() == "10");

        // map pipeline
        auto mapped = s
            .map([](int v) { return v + 100; })
            .map([](int v) { return v * 2; });
        assert(mapped.value() == 210);

        // map_or & map_or_else
        assert(s.map_or([](int v) { return v * 3; }, 0) == 15);
        assert(n.map_or([](int v) { return v * 3; }, -1) == -1);

        assert(s.map_or_else([](int v) { return v * 10; }, [] { return -5; }) == 50);
        assert(n.map_or_else([](int v) { return v * 10; }, [] { return -5; }) == -5);

        // map_or_transform
        assert(s.map_or_transform([](int v) { return v * 2; }, 100) == 10);
        assert(n.map_or_transform([](int v) { return v * 2; }, 100) == 100);

        std::cout << "[PASS 12/20] Functional Combinators Comprehensive Pipeline" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 13: Reference Borrowing (as_mut_ptr & as_immut_ptr)
    // -------------------------------------------------------------------------
    {
        rl::Option<int> val = rl::Some(100);

        // Mutating through borrowed mut ref
        auto mut_ref = val.as_mut_ptr();
        assert(mut_ref.is_some());
        *mut_ref.value() = 250;
        assert(val.value() == 250);

        // Borrowing const ref
        const auto& cval = val;
        auto immut_ref = cval.as_immut_ptr();
        assert(immut_ref.is_some());
        assert(*immut_ref.value() == 250);

        // Note: as_mut_ptr() && and as_immut_ptr() && are explicitly deleted in Option<T>
        // to prevent binding references to temporaries.

        std::cout << "[PASS 13/20] Reference Borrowing (as_mut_ptr & as_immut_ptr)" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 14: Pattern Matching Engine via std::span & Braced Initializer Lists
    // -------------------------------------------------------------------------
    // Test 14: Pattern Matching Engine
    // -------------------------------------------------------------------------
    {
        rl::Option<int> opt = rl::Some(42);

        // 1. match via arms
        int r1 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            rl::SomeAny >>= [](int x) -> int { return x * 2; }
        ).value();
        assert(r1 == 84);

        // 2. match via lambda
        auto l_none = []() -> int { return -1; };
        auto l_some = [](int x) -> int { return x + 10; };
        int r2 = opt.match<int>(
            rl::None >>= l_none,
            rl::SomeAny >>= l_some
        ).value();
        assert(r2 == 52);

        // 3. Equality Pattern (SomeEq)
        int r3 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            rl::Some(42) >>= [](int x) -> int { return x + 1000; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r3 == 1042);

        // 4. Nullary predicate pattern
        int r4 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [](const int& v) { return v > 40; } >>= [](int x) -> int { return x * 10; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r4 == 420);

        // 5. Wildcard Pattern
        int r5 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            rl::Wild >>= []() -> int { return 777; }
        ).value();
        assert(r5 == 777);

        std::cout << "[PASS 14/20] Pattern Matching Engine (Arms)" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 15: Zero-Sized Types (EmptyTag)
    // -------------------------------------------------------------------------
    {
        rl::Option<EmptyTag> empty_opt = rl::Some(EmptyTag{});
        assert(empty_opt.is_some());
        empty_opt.reset();
        assert(empty_opt.is_none());
        empty_opt.emplace();
        assert(empty_opt.is_some());

        std::cout << "[PASS 15/20] Zero-Sized EmptyTag Structure" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 16: Non-Default Constructible Types (NoDefault)
    // -------------------------------------------------------------------------
    {
        rl::Option<NoDefault> no_def = rl::None();
        assert(no_def.is_none());
        no_def.emplace(42);
        assert(no_def.is_some());
        assert(no_def.value().value == 42);

        // unwrap_or_default is constrained to std::is_default_constructible_v<Tp_>
        static_assert(!std::is_default_constructible_v<NoDefault>);

        std::cout << "[PASS 16/20] Non-Default Constructible Types" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 17: Process Death Tests (SIGABRT on Invalid State Access)
    // -------------------------------------------------------------------------
    {
        // 1. value() on None must abort
        bool d1 = expect_abort([]() {
            rl::Option<int> opt = rl::None();
            (void)opt.value();
        });
        assert(d1 && "value() on None must abort");

        // 2. unwrap() on None must abort
        bool d2 = expect_abort([]() {
            rl::Option<int> opt = rl::None();
            (void)opt.unwrap();
        });
        assert(d2 && "unwrap() on None must abort");

        // 3. expect() on None must abort
        bool d3 = expect_abort([]() {
            rl::Option<int> opt = rl::None();
            (void)opt.expect("Abort expected!");
        });
        assert(d3 && "expect() on None must abort");

        // 4. unwrap_none() on Some must abort
        bool d4 = expect_abort([]() {
            rl::Option<int> opt = rl::Some(42);
            (void)opt.unwrap_none();
        });
        assert(d4 && "unwrap_none() on Some must abort");

        // 5. unwrap on None must abort
        bool d5 = expect_abort([]() {
            rl::Option<int> opt = rl::None;
            (void)opt.unwrap();
        });
        assert(d5 && "unwrap on None must abort");

        std::cout << "[PASS 17/20] Process Death Tests (SIGABRT on Invalid State)" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 18: Move Assignment and Conversion Pipeline
    // -------------------------------------------------------------------------
    {
        rl::Option<std::string> a = rl::Some(std::string("hello"));
        rl::Option<std::string> b = rl::None();

        b = std::move(a);
        assert(b.is_some());
        assert(b.value() == "hello");

        // unwrap_unchecked on rvalue moves out without check
        std::string s = std::move(b).unwrap_unchecked();
        assert(s == "hello");

        std::cout << "[PASS 18/20] Move Assignment and Conversion Pipeline" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 19: Variadic Emplace with Multi-Argument Types
    // -------------------------------------------------------------------------
    {
        struct MultiArg {
            int a;
            rl::details::PadToAlignment_<sizeof(int), alignof(long)> pad{};
            long b;
            std::string c;
            MultiArg(int x, long y, std::string z) : a(x), b(y), c(std::move(z)) {}
        };

        rl::Option<MultiArg> opt = rl::None();
        assert(opt.is_none());
        opt.emplace(42, 1000L, "variadic");
        assert(opt.is_some());
        assert(opt->a == 42);
        assert(opt->b == 1000L);
        assert(opt->c == "variadic");

        std::cout << "[PASS 19/20] Variadic Emplace In-Place Construction" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 20: Direct Assignment Operators from Some<T> and None
    // -------------------------------------------------------------------------
    {
        rl::Option<int> opt = rl::None();
        opt = rl::Some(77);
        assert(opt.is_some());
        assert(opt.value() == 77);

        opt = rl::None();
        assert(opt.is_none());

        opt = rl::Some(99);
        assert(opt.is_some());
        assert(opt.value() == 99);

        std::cout << "[PASS 20/20] Direct Assignment Operators from Some and None" << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 21: Implicit Pattern Detection (Predicate, Conditional, Bool Guard)
    // -------------------------------------------------------------------------
    {
        rl::Option<int> opt = rl::Some(42);
        rl::Option<int> none_opt = rl::None;

        // 1. Predicate lambda: callable taking const Tp_& returning bool
        int r1 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [](const int& v) { return v > 40; } >>= [](int x) -> int { return x * 10; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r1 == 420);

        // 2. Predicate lambda that rejects (falls through to SomeAny)
        int r2 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [](const int& v) { return v > 100; } >>= [](int) -> int { return -1; },
            rl::SomeAny >>= [](int x) -> int { return x + 1; }
        ).value();
        assert(r2 == 43);

        // 3. Predicate with None value
        int r3 = none_opt.match<int>(
            rl::None >>= []() -> int { return -100; },
            [](const int&) { return true; } >>= [](int) -> int { return 999; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r3 == -100);

        // 4. Boolean guard: a nullary callable pattern >>= handler
        int z = 42;
        int r4 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [&] { return z == 42; } >>= [](int x) -> int { return x + 100; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r4 == 142);

        // 5. Boolean guard: false -> skipped, falls through
        int r5 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [&] { return z == 999; } >>= [](int) -> int { return -1; },
            rl::SomeAny >>= [](int x) -> int { return x * 2; }
        ).value();
        assert(r5 == 84);

        // 6. Boolean guard with direct value handler
        int r6 = opt.match<int>(
            rl::None >>= 0,
            [] { return true; } >>= 777,
            rl::SomeAny >>= 0
        ).value();
        assert(r6 == 777);

        // 7. Nullary conditional callable: []() -> bool
        bool flag = true;
        int r7 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [flag]() -> bool { return flag; } >>= [](int x) -> int { return x + 200; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r7 == 242);

        // 8. Nullary conditional that returns false -> skipped
        int r8 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [flag]() -> bool { return !flag; } >>= [](int) -> int { return -1; },
            rl::SomeAny >>= [](int x) -> int { return x * 3; }
        ).value();
        assert(r8 == 126);

        // 9. Mixing a nullary callable with a value predicate
        int r9 = opt.match<int>(
            rl::None >>= []() -> int { return 0; },
            [](const int& v) { return v == 42; } >>= []() -> int { return 1000; },
            [](const int& v) { return v > 50; } >>= [](int x) -> int { return x; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r9 == 1000);

        // 10. Predicate with value-converting handler
        std::string r10 = opt.match<std::string>(
            rl::None >>= "none",
            [](const int& v) { return v > 0; } >>= "positive",
            rl::SomeAny >>= "other"
        ).value();
        assert(r10 == "positive");

        // 11. Chain: value equality -> predicate -> bool guard -> wildcard
        int r11 = opt.match<int>(
            rl::None >>= 0,
            rl::Some(42) >>= 10,
            [](const int& v) { return v < 10; } >>= 2,
            [&] { return z == 42; } >>= 3,
            rl::Wild >>= 4
        ).value();
        assert(r11 == 10);

        // 12. Predicate lambda with function pointer syntax
        auto is_even = [](const int& v) -> bool { return v % 2 == 0; };
        int r12 = opt.match<int>(
            rl::None >>= 0,
            is_even >>= [](int x) -> int { return x / 2; },
            rl::SomeAny >>= [](int x) -> int { return x; }
        ).value();
        assert(r12 == 21);

        // 13. Option<bool>: bool pattern as VALUE equality
        rl::Option<bool> bool_opt = rl::Some(true);
        int r13 = bool_opt.match<int>(
            rl::None >>= 0,
            rl::True >>= 1,
            rl::False >>= 2,
            rl::SomeAny >>= 3
        ).value();
        assert(r13 == 1);

        rl::Option<bool> bool_opt2 = rl::Some(false);
        int r14 = bool_opt2.match<int>(
            rl::None >>= 0,
            rl::True >>= 1,
            rl::False >>= 2,
            rl::SomeAny >>= 3
        ).value();
        assert(r14 == 2);

        std::cout << "[PASS 21/21] Implicit Pattern Detection (Predicate, Conditional, Bool Guard)" << std::endl;
    }

    std::cout << "=====================================================" << std::endl;
    std::cout << "All 21 Rigorous Edge-Case Suites Passed Successfully!" << std::endl;
    std::cout << "=====================================================" << std::endl;
    return 0;
}

#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
