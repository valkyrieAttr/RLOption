#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <stdexcept>
#include <limits>
#include <unordered_set>
#include <unordered_map>
#include <type_traits>

// ============================================================================
// Test Utilities & Assertion Macros
// ============================================================================
static int g_test_count = 0;
static int g_pass_count = 0;

#define TEST_ASSERT(cond) do { \
    g_test_count++; \
    if (!(cond)) { \
        std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ \
                  << " in " << __func__ << ": " #cond << std::endl; \
        std::abort(); \
    } \
    g_pass_count++; \
} while(0)

// ============================================================================
// SECTION 1: Lifecycle & Memory Leak Tracking
// ============================================================================
struct TrackedLifecycle {
    static inline int instances_alive = 0;
    static inline int default_ctor_count = 0;
    static inline int val_ctor_count = 0;
    static inline int copy_ctor_count = 0;
    static inline int move_ctor_count = 0;
    static inline int copy_assign_count = 0;
    static inline int move_assign_count = 0;
    static inline int dtor_count = 0;

    int id;

    static void reset_counters() {
        instances_alive = 0;
        default_ctor_count = 0;
        val_ctor_count = 0;
        copy_ctor_count = 0;
        move_ctor_count = 0;
        copy_assign_count = 0;
        move_assign_count = 0;
        dtor_count = 0;
    }

    TrackedLifecycle() : id(0) {
        instances_alive++;
        default_ctor_count++;
    }

    TrackedLifecycle(int v) : id(v) {
        instances_alive++;
        val_ctor_count++;
    }

    TrackedLifecycle(const TrackedLifecycle& o) : id(o.id) {
        instances_alive++;
        copy_ctor_count++;
    }

    TrackedLifecycle(TrackedLifecycle&& o) noexcept : id(o.id) {
        o.id = -1; // mark moved-out
        instances_alive++;
        move_ctor_count++;
    }

    TrackedLifecycle& operator=(const TrackedLifecycle& o) {
        id = o.id;
        copy_assign_count++;
        return *this;
    }

    TrackedLifecycle& operator=(TrackedLifecycle&& o) noexcept {
        id = o.id;
        o.id = -1;
        move_assign_count++;
        return *this;
    }

    ~TrackedLifecycle() {
        instances_alive--;
        dtor_count++;
    }

    bool operator==(const TrackedLifecycle& o) const = default;
};

// Forward declarations (required by -Wmissing-prototypes)
void test_section_1_lifecycles();
void test_section_2_pathological_types();
void test_section_3_floats_spaceship_pointers();
void test_section_4_exception_safety();
void test_section_5_variadic_multi_type();
void test_section_6_pattern_matching_edge_cases();
void test_section_7_monadic_apis();
void test_section_8_hash_and_containers();
void test_section_9_constexpr_evaluation();
void test_section_10_rust_lifetimes_and_stress();

void test_section_1_lifecycles() {
    std::cout << "--- Running Section 1: Lifecycles & Memory Leaks ---\n";
    TrackedLifecycle::reset_counters();

    // 1. None Option
    {
        rl::Option<TrackedLifecycle> optNone = rl::None;
        TEST_ASSERT(optNone.is_none());
        TEST_ASSERT(!optNone.is_some());
        TEST_ASSERT(TrackedLifecycle::instances_alive == 0);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 2. Some Option
    {
        rl::Option<TrackedLifecycle> optSome = rl::Some(TrackedLifecycle(42));
        TEST_ASSERT(optSome.is_some());
        TEST_ASSERT(optSome.unwrap().id == 42);
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 3. Copy Option
    {
        rl::Option<TrackedLifecycle> opt1 = rl::Some(TrackedLifecycle(10));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
        {
            rl::Option<TrackedLifecycle> opt2 = opt1;
            TEST_ASSERT(TrackedLifecycle::instances_alive == 2);
            TEST_ASSERT(opt2.unwrap().id == 10);
        }
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 4. Move Option
    {
        rl::Option<TrackedLifecycle> opt1 = rl::Some(TrackedLifecycle(20));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
        rl::Option<TrackedLifecycle> opt2 = std::move(opt1);
        TEST_ASSERT(TrackedLifecycle::instances_alive == 2 || TrackedLifecycle::instances_alive == 1);
        TEST_ASSERT(opt2.unwrap().id == 20);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 5. In-Place Copy Assignment (Finding 4 verification)
    {
        TrackedLifecycle::reset_counters();
        rl::Option<TrackedLifecycle> optA = rl::Some(TrackedLifecycle(1));
        rl::Option<TrackedLifecycle> optB = rl::Some(TrackedLifecycle(2));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 2);

        TrackedLifecycle::copy_assign_count = 0;
        int dtor_before = TrackedLifecycle::dtor_count;
        optA = optB;
        TEST_ASSERT(optA.unwrap().id == 2);
        TEST_ASSERT(TrackedLifecycle::copy_assign_count == 1);
        // Must NOT destroy and re-construct when active alternatives match!
        TEST_ASSERT(TrackedLifecycle::dtor_count == dtor_before);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 6. In-Place Move Assignment (Finding 4 verification)
    {
        TrackedLifecycle::reset_counters();
        rl::Option<TrackedLifecycle> optA = rl::Some(TrackedLifecycle(1));
        rl::Option<TrackedLifecycle> optB = rl::Some(TrackedLifecycle(2));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 2);

        TrackedLifecycle::move_assign_count = 0;
        int dtor_before = TrackedLifecycle::dtor_count;
        optA = std::move(optB);
        TEST_ASSERT(optA.unwrap().id == 2);
        TEST_ASSERT(TrackedLifecycle::move_assign_count == 1);
        TEST_ASSERT(TrackedLifecycle::dtor_count == dtor_before);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 7. Self-Assignment (both copy and move)
    {
        rl::Option<TrackedLifecycle> opt = rl::Some(TrackedLifecycle(99));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
        // Self-assignment: suppress -Wself-assign-overloaded for intentional test
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        opt = opt;
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap().id == 99);
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);

        // Self-move through an alias avoids a compiler-specific self-move warning.
        auto &self_move_source = opt;
        opt = std::move(self_move_source);
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 8. Switching alternatives in multi-type Option<TrackedLifecycle, std::string>
    {
        TrackedLifecycle::reset_counters();
        rl::Option<TrackedLifecycle, std::string> multirl = rl::Some(TrackedLifecycle(555));
        TEST_ASSERT(multirl.is_some());
        TEST_ASSERT(multirl.is_some<TrackedLifecycle>());
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);

        // Switch active alternative to std::string
        multirl = rl::Some<std::string>("hello world");
        TEST_ASSERT(multirl.is_some<std::string>());
        TEST_ASSERT(!multirl.is_some<TrackedLifecycle>());
        TEST_ASSERT(multirl.unwrap<std::string>() == "hello world");
        // TrackedLifecycle should have been destroyed!
        TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

        // Switch back to TrackedLifecycle
        multirl = rl::Some(TrackedLifecycle(777));
        TEST_ASSERT(multirl.is_some<TrackedLifecycle>());
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
        TEST_ASSERT(multirl.unwrap<TrackedLifecycle>().id == 777);

        // Reset to None
        multirl = rl::None;
        TEST_ASSERT(multirl.is_none());
        TEST_ASSERT(TrackedLifecycle::instances_alive == 0);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 9. emplace() over existing value
    {
        TrackedLifecycle::reset_counters();
        rl::Option<TrackedLifecycle> opt = rl::Some(TrackedLifecycle(100));
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);

        opt.emplace(200);
        TEST_ASSERT(opt.unwrap().id == 200);
        TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    // 10. move unwrap resets option
    {
        rl::Option<TrackedLifecycle> opt = rl::Some(TrackedLifecycle(300));
        TrackedLifecycle t = std::move(opt).unwrap();
        TEST_ASSERT(t.id == 300);
        TEST_ASSERT(opt.is_none());
    }
    TEST_ASSERT(TrackedLifecycle::instances_alive == 0);

    std::cout << "  [PASS] Section 1: Lifecycles & Memory Leaks passed!\n";
}

// ============================================================================
// SECTION 2: Pathological & Adversarial Types
// ============================================================================

// Evil Address-Of Operator
struct EvilAddrOf {
    int value;
    void operator&() const = delete;
    void operator&() = delete;
    bool operator==(const EvilAddrOf& o) const = default;
};

// Evil Comma Operator
struct EvilComma {
    int value;
    template<typename T>
    void operator,(T&&) const = delete;
    bool operator==(const EvilComma& o) const = default;
};

// Immovable & Non-Copyable Type
struct ImmovableType {
    int data;
    explicit ImmovableType(int d = 0) : data(d) {}
    ImmovableType(const ImmovableType&) = delete;
    ImmovableType(ImmovableType&&) = delete;
    ImmovableType& operator=(const ImmovableType&) = delete;
    ImmovableType& operator=(ImmovableType&&) = delete;
};

// Copy-Only Type
struct CopyOnlyType {
    int data;
    explicit CopyOnlyType(int d = 0) : data(d) {}
    CopyOnlyType(const CopyOnlyType&) = default;
    CopyOnlyType(CopyOnlyType&&) = delete;
    CopyOnlyType& operator=(const CopyOnlyType&) = default;
    CopyOnlyType& operator=(CopyOnlyType&&) = delete;
    bool operator==(const CopyOnlyType& o) const = default;
};

// Move-Only Type
struct MoveOnlyType {
    std::unique_ptr<int> ptr;
    explicit MoveOnlyType(int val = 0) : ptr(std::make_unique<int>(val)) {}
    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType(MoveOnlyType&&) noexcept = default;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(MoveOnlyType&&) noexcept = default;
};

// Over-Aligned Type
struct alignas(128) OverAlignedType {
    int val;
    char buffer[124];
};

// Empty Type
struct EmptyStruct {
    bool operator==(const EmptyStruct&) const = default;
};

void test_section_2_pathological_types() {
    std::cout << "--- Running Section 2: Pathological & Adversarial Types ---\n";

    // 1. Evil AddrOf
    {
        rl::Option<EvilAddrOf> opt = rl::Some(EvilAddrOf{42});
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap().value == 42);
        opt.reset();
        TEST_ASSERT(opt.is_none());
    }

    // 2. Evil Comma
    {
        rl::Option<EvilComma> opt = rl::Some(EvilComma{99});
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap().value == 99);
    }

    // 3. Immovable Type (via Default construction or in-place)
    {
        static_assert(!std::is_copy_constructible_v<ImmovableType>);
        static_assert(!std::is_move_constructible_v<ImmovableType>);

        rl::Option<ImmovableType> optDef{rl::Default};
        TEST_ASSERT(optDef.is_some());
        TEST_ASSERT(optDef.unwrap().data == 0);
        optDef.unwrap().data = 77;
        TEST_ASSERT(optDef.unwrap().data == 77);
    }

    // 4. Copy-Only Type
    {
        static_assert(std::is_copy_constructible_v<CopyOnlyType>);
        static_assert(!std::is_move_constructible_v<CopyOnlyType>);

        rl::Option<CopyOnlyType> opt1 = rl::Some(CopyOnlyType{50});
        TEST_ASSERT(opt1.is_some());
        TEST_ASSERT(opt1.unwrap().data == 50);

        rl::Option<CopyOnlyType> opt2 = opt1; // copy construct
        TEST_ASSERT(opt2.unwrap().data == 50);

        opt1 = opt2; // copy assign
        TEST_ASSERT(opt1.unwrap().data == 50);
    }

    // 5. Move-Only Type
    {
        static_assert(!std::is_copy_constructible_v<MoveOnlyType>);
        static_assert(std::is_move_constructible_v<MoveOnlyType>);

        rl::Option<MoveOnlyType> opt1 = rl::Some(MoveOnlyType{123});
        TEST_ASSERT(opt1.is_some());
        TEST_ASSERT(*opt1.unwrap().ptr == 123);

        rl::Option<MoveOnlyType> opt2 = std::move(opt1);
        TEST_ASSERT(opt2.is_some());
        TEST_ASSERT(*opt2.unwrap().ptr == 123);

        MoveOnlyType moved = std::move(opt2).unwrap();
        TEST_ASSERT(*moved.ptr == 123);
        TEST_ASSERT(opt2.is_none());
    }

    // 6. Over-Aligned Type
    {
        static_assert(alignof(OverAlignedType) == 128);
        static_assert(alignof(rl::Option<OverAlignedType>) >= 128);

        rl::Option<OverAlignedType> opt = rl::Some(OverAlignedType{ .val = 88, .buffer = {} });
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap().val == 88);

        // Verify runtime address alignment
        uintptr_t addr = reinterpret_cast<uintptr_t>(std::addressof(opt.unwrap()));
        TEST_ASSERT((addr % 128) == 0);
    }

    // 7. Empty Struct
    {
        rl::Option<EmptyStruct> opt = rl::Some(EmptyStruct{});
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap() == EmptyStruct{});
        opt.reset();
        TEST_ASSERT(opt.is_none());
    }

    std::cout << "  [PASS] Section 2: Pathological & Adversarial Types passed!\n";
}

// ============================================================================
// SECTION 3: Floats, NaN, Spaceship, and Pointer Types
// ============================================================================
void test_section_3_floats_spaceship_pointers() {
    std::cout << "--- Running Section 3: Floats, Spaceship, Pointers ---\n";

    // 1. Float NaN Partial Ordering
    {
        double nan = std::numeric_limits<double>::quiet_NaN();
        rl::Option<double> optNan1 = rl::Some(nan);
        rl::Option<double> optNan2 = rl::Some(nan);
        rl::Option<double> optVal  = rl::Some(1.0);
        rl::Option<double> optNone = rl::None;

        // optVal is well-formed and not NaN; not equal to NaN
        TEST_ASSERT(optVal.is_some());

        // NaN == NaN is false
        TEST_ASSERT(!(optNan1 == optNan2));
        TEST_ASSERT(optNan1 != optNan2);

        // NaN <=> NaN is partial_ordering::unordered
        auto cmp = optNan1 <=> optNan2;
        TEST_ASSERT(cmp == std::partial_ordering::unordered);

        // Some(NaN) <=> None: engaged option is always greater than None!
        auto cmpNone = optNan1 <=> optNone;
        TEST_ASSERT(cmpNone == std::partial_ordering::greater);

        // None <=> None is equivalent
        auto cmpNoneNone = optNone <=> optNone;
        TEST_ASSERT(cmpNoneNone == std::partial_ordering::equivalent);
    }

    // 2. Three-Way Comparison for Regular Values
    {
        rl::Option<int> o1 = rl::Some(10);
        rl::Option<int> o2 = rl::Some(20);
        rl::Option<int> o3 = rl::Some(10);
        rl::Option<int> oNone = rl::None;

// Spaceship returns std::strong_ordering; comparing with 0 is well-defined
// (std::strong_ordering::less < 0 is true) but clang warns about the literal.
// Use the named constants instead.
        TEST_ASSERT((o1 <=> o2) == std::strong_ordering::less);
        TEST_ASSERT((o2 <=> o1) == std::strong_ordering::greater);
        TEST_ASSERT((o1 <=> o3) == std::strong_ordering::equal);
        TEST_ASSERT((o1 <=> oNone) == std::strong_ordering::greater);
        TEST_ASSERT((oNone <=> o1) == std::strong_ordering::less);
        TEST_ASSERT((oNone <=> oNone) == std::strong_ordering::equal);
    }

    // 3. Pointer Types & nullptr differentiation
    {
        int dummy = 42;
        rl::Option<int*> optPtrNone = rl::None;
        rl::Option<int*> optPtrNull = rl::Some<int*>(nullptr);
        rl::Option<int*> optPtrValid = rl::Some<int*>(&dummy);

        // None is disengaged
        TEST_ASSERT(optPtrNone.is_none());
        TEST_ASSERT(!optPtrNone.is_some());

        // Some(nullptr) is ENGAGED holding a nullptr!
        TEST_ASSERT(optPtrNull.is_some());
        TEST_ASSERT(!optPtrNull.is_none());
        TEST_ASSERT(optPtrNull.unwrap() == nullptr);

        // Some(&dummy)
        TEST_ASSERT(optPtrValid.is_some());
        TEST_ASSERT(optPtrValid.unwrap() == &dummy);
        TEST_ASSERT(*optPtrValid.unwrap() == 42);

        // Distinct equality
        TEST_ASSERT(optPtrNone != optPtrNull);
        TEST_ASSERT(optPtrNull != optPtrValid);
    }

    std::cout << "  [PASS] Section 3: Floats, Spaceship, Pointers passed!\n";
}

// ============================================================================
// SECTION 4: Exception Safety & Throwing Types
// ============================================================================

#if defined(__cpp_exceptions)

// Suppress padding warning for ThrowOnCopy — the extra bool member is intentional.
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wpadded"
#endif
struct ThrowOnCopy {
    int val;
    bool should_throw = false;

    ThrowOnCopy(int v = 0, bool t = false) : val(v), should_throw(t) {}

    ThrowOnCopy(const ThrowOnCopy& o) : val(o.val), should_throw(o.should_throw) {
        if (should_throw) {
            throw std::runtime_error("ThrowOnCopy copy ctor triggered");
        }
    }

    ThrowOnCopy(ThrowOnCopy&&) noexcept = default;
    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
    ThrowOnCopy& operator=(ThrowOnCopy&&) noexcept = default;
};
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

void test_section_4_exception_safety() {
    std::cout << "--- Running Section 4: Exception Safety ---\n";

    // 1. Copy Construction Throw
    {
        rl::Option<ThrowOnCopy> src = rl::Some(ThrowOnCopy(10, true));
        bool caught = false;
        try {
            rl::Option<ThrowOnCopy> dst = src; // should throw
            (void)dst;
        } catch (const std::runtime_error& e) {
            caught = true;
            TEST_ASSERT(std::string(e.what()) == "ThrowOnCopy copy ctor triggered");
        }
        TEST_ASSERT(caught);
        // Source must remain valid and engaged!
        TEST_ASSERT(src.is_some());
        TEST_ASSERT(src.unwrap().val == 10);
    }

    // 2. Throwing Predicates in match
    {
        struct ThrowingPred {
            bool operator()(int) const {
                throw std::logic_error("predicate explosion");
            }
        };

        rl::Option<int> opt = rl::Some(42);
        bool caught = false;
        try {
            auto res = opt.match(
                ThrowingPred{} >>= [](int) { return 1; },
                rl::SomeAny >>= [] { return 2; },
                rl::None >>= [] { return 3; }
            );
            (void)res;
        } catch (const std::logic_error& e) {
            caught = true;
            TEST_ASSERT(std::string(e.what()) == "predicate explosion");
        }
        TEST_ASSERT(caught);
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap() == 42);
    }

    // 3. Throwing Handler in match
    {
        rl::Option<int> opt = rl::Some(42);
        bool caught = false;
        try {
            auto res = opt.match(
                rl::Some(42) >>= [](int) -> int {
                    throw std::runtime_error("handler explosion");
                },
                rl::SomeAny >>= [] { return 0; },
                rl::None >>= [] { return 0; }
            );
            (void)res;
        } catch (const std::runtime_error& e) {
            caught = true;
            TEST_ASSERT(std::string(e.what()) == "handler explosion");
        }
        TEST_ASSERT(caught);
        TEST_ASSERT(opt.is_some());
        TEST_ASSERT(opt.unwrap() == 42);
    }

    std::cout << "  [PASS] Section 4: Exception Safety passed!\n";
}

#else // !defined(__cpp_exceptions)

void test_section_4_exception_safety() {
    std::cout << "--- Skipping Section 4: Exception Safety (exceptions disabled) ---\n";
}

#endif // defined(__cpp_exceptions)

// ============================================================================
// SECTION 5: Variadic Multi-Type Options (4 Alternatives)
// ============================================================================
void test_section_5_variadic_multi_type() {
    std::cout << "--- Running Section 5: Variadic Multi-Type Options ---\n";

    using QuadOption = rl::Option<int, std::string, double, char>;

    // 1. Value construction into each alternative
    QuadOption oInt = 42;
    TEST_ASSERT(oInt.is_some<int>());
    TEST_ASSERT(!oInt.is_some<std::string>());
    TEST_ASSERT(!oInt.is_some<double>());
    TEST_ASSERT(!oInt.is_some<char>());
    TEST_ASSERT(oInt.unwrap<int>() == 42);

    QuadOption oStr = std::string("variadic string");
    TEST_ASSERT(oStr.is_some<std::string>());
    TEST_ASSERT(oStr.unwrap<std::string>() == "variadic string");

    QuadOption oDbl = 3.14159;
    TEST_ASSERT(oDbl.is_some<double>());
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    TEST_ASSERT(oDbl.unwrap<double>() == 3.14159);
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

    QuadOption oChar = 'Z';
    TEST_ASSERT(oChar.is_some<char>());
    TEST_ASSERT(oChar.unwrap<char>() == 'Z');

    // 2. Cross-alternative assignment
    oInt = 'A';
    TEST_ASSERT(oInt.is_some<char>());
    TEST_ASSERT(oInt.unwrap<char>() == 'A');

    oInt = 2.718;
    TEST_ASSERT(oInt.is_some<double>());
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    TEST_ASSERT(oInt.unwrap<double>() == 2.718);
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

    oInt = std::string("swapped to str");
    TEST_ASSERT(oInt.is_some<std::string>());
    TEST_ASSERT(oInt.unwrap<std::string>() == "swapped to str");

    oInt = 999;
    TEST_ASSERT(oInt.is_some<int>());
    TEST_ASSERT(oInt.unwrap<int>() == 999);

    // 3. Equality between multi-type options
    QuadOption oA = 42;
    QuadOption oB = 42;
    QuadOption oC = 43;
    QuadOption oD = std::string("42");
    QuadOption oNone = rl::None;

    TEST_ASSERT(oA == oB);
    TEST_ASSERT(oA != oC);
    TEST_ASSERT(oA != oD);
    TEST_ASSERT(oA != oNone);
    TEST_ASSERT(oNone == oNone);

    // 4. Swapping multi-type options
    QuadOption swap1 = 123;
    QuadOption swap2 = std::string("swapped!");
    swap1.swap(swap2);
    TEST_ASSERT(swap1.is_some<std::string>() && swap1.unwrap<std::string>() == "swapped!");
    TEST_ASSERT(swap2.is_some<int>() && swap2.unwrap<int>() == 123);

    std::cout << "  [PASS] Section 5: Variadic Multi-Type Options passed!\n";
}

// ============================================================================
// SECTION 6: Advanced Pattern Matching & Dispatch Edge Cases
// ============================================================================
void test_section_6_pattern_matching_edge_cases() {
    std::cout << "--- Running Section 6: Advanced Pattern Matching ---\n";

    // 1. Arm precedence: specific value before predicate before wildcard
    {
        rl::Option<int> opt = rl::Some(10);
        int hit = opt.match(
            rl::Some(10) >>= [](int) { return 1; },
            [](int x) { return x > 5; } >>= [](int) { return 2; },
            rl::SomeAny >>= [] { return 3; },
            rl::None >>= [] { return 4; }
        ).unwrap();
        TEST_ASSERT(hit == 1); // Specific value 10 matched first

        rl::Option<int> opt2 = rl::Some(8);
        int hit2 = opt2.match(
            rl::Some(10) >>= [](int) { return 1; },
            [](int x) { return x > 5; } >>= [](int) { return 2; },
            rl::SomeAny >>= [] { return 3; },
            rl::None >>= [] { return 4; }
        ).unwrap();
        TEST_ASSERT(hit2 == 2); // Predicate matched next

        rl::Option<int> opt3 = rl::Some(3);
        int hit3 = opt3.match(
            rl::Some(10) >>= [](int) { return 1; },
            [](int x) { return x > 5; } >>= [](int) { return 2; },
            rl::SomeAny >>= [] { return 3; },
            rl::None >>= [] { return 4; }
        ).unwrap();
        TEST_ASSERT(hit3 == 3); // SomeAny catch-all
    }

    // 2. Reference forwarding: lvalue vs const lvalue vs rvalue matching
    {
        rl::Option<std::string> lvalrl = rl::Some<std::string>("original");
        bool taken_by_ref = false;
        lvalrl.match(
            rl::SomeAny >>= [&](std::string& s) {
                taken_by_ref = true;
                s = "modified_in_place";
            },
            rl::None >>= [] {}
        );
        TEST_ASSERT(taken_by_ref);
        TEST_ASSERT(lvalrl.unwrap() == "modified_in_place");

        const rl::Option<std::string> constrl = rl::Some<std::string>("const_val");
        bool taken_by_const_ref = false;
        constrl.match(
            rl::SomeAny >>= [&](const std::string& s) {
                taken_by_const_ref = true;
                TEST_ASSERT(s == "const_val");
            },
            rl::None >>= [] {}
        );
        TEST_ASSERT(taken_by_const_ref);

        rl::Option<std::string> rvalrl = rl::Some<std::string>("move_me");
        bool taken_by_rval = false;
        std::move(rvalrl).match(
            rl::SomeAny >>= [&](std::string&& s) {
                taken_by_rval = true;
                std::string sink = std::move(s);
                TEST_ASSERT(sink == "move_me");
            },
            rl::None >>= [] {}
        );
        TEST_ASSERT(taken_by_rval);
    }

    // 3. Common return type deduction into Option<T>
    {
        rl::Option<int, std::string, double> multi = rl::Some(100);

        auto resInt = multi.match(
            rl::Some<int>(100)           >>= [](int x) { return static_cast<double>(x * 2); },
            rl::Some<std::string>("txt") >>= [](const std::string&) { return 200.0; },
            rl::Some<double>(3.14)       >>= [](double d) { return d; },
            rl::SomeAny                  >>= [] { return 0.0; },
            rl::None                     >>= [] { return rl::None; }
        );
        static_assert(std::same_as<decltype(resInt), rl::Option<double>>);
        TEST_ASSERT(resInt.is_some() && resInt.unwrap() == 200.0);

        multi = std::string("txt");
        auto resStr = multi.match(
            rl::Some<int>(100)           >>= [](int x) { return static_cast<double>(x); },
            rl::Some<std::string>("txt") >>= [](const std::string&) { return 500.0; },
            rl::Some<double>(3.14)       >>= [](double d) { return d; },
            rl::SomeAny                  >>= [] { return 0.0; },
            rl::None                     >>= [] { return rl::None; }
        );
        TEST_ASSERT(resStr.is_some() && resStr.unwrap() == 500.0);

        multi = 3.14;
        auto resDbl = multi.match(
            rl::Some<int>(100)           >>= [](int x) { return static_cast<double>(x); },
            rl::Some<std::string>("txt") >>= [](const std::string&) { return 0.0; },
            rl::Some<double>(3.14)       >>= [](double d) { return d; },
            rl::SomeAny                  >>= [] { return 0.0; },
            rl::None                     >>= [] { return rl::None; }
        );
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wfloat-equal"
#endif
        TEST_ASSERT(resDbl.is_some() && resDbl.unwrap() == 3.14);
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif
    }

    // 4. Arms returning direct values without lambdas: returns Option<T>
    {
        rl::Option<int> opt = rl::Some(1);
        auto valRes = opt.match(
            rl::Some(1) >>= 100,
            rl::SomeAny >>= 200,
            rl::None >>= 300
        );
        static_assert(std::same_as<decltype(valRes), rl::Option<int>>);
        TEST_ASSERT(valRes.is_some() && valRes.unwrap() == 100);

        rl::Option<int> optNone = rl::None;
        auto valResNone = optNone.match(
            rl::Some(1) >>= 100,
            rl::SomeAny >>= 200,
            rl::None >>= 300
        );
        static_assert(std::same_as<decltype(valResNone), rl::Option<int>>);
        TEST_ASSERT(valResNone.is_some() && valResNone.unwrap() == 300);
    }

    // 5. Explicit single-type template argument: match<std::string>
    {
        rl::Option<int> opt = rl::Some(7);
        auto explicitSingle = opt.template match<std::string>(
            rl::Some(7) >>= [](int x) { return std::to_string(x * 10); },
            rl::SomeAny >>= [] { return std::string("other"); },
            rl::None >>= [] { return rl::None; }
        );
        static_assert(std::same_as<decltype(explicitSingle), rl::Option<std::string>>);
        TEST_ASSERT(explicitSingle.is_some() && explicitSingle.unwrap() == "70");

        // 6. Test .match() alias
        auto aliasSingle = opt.template match<std::string>(
            rl::Some(7) >>= [](int x) { return std::to_string(x * 100); },
            rl::SomeAny >>= [] { return std::string("other"); },
            rl::None >>= [] { return rl::None; }
        );
        static_assert(std::same_as<decltype(aliasSingle), rl::Option<std::string>>);
        TEST_ASSERT(aliasSingle.is_some() && aliasSingle.unwrap() == "700");
    }

    // 7. Dynamic multi-type return: returns Option<int, std::string> when arms have disparate types
    {
        rl::Option<int> opt = rl::Some(5);
        auto multiRes = opt.match(
            rl::Some(5) >>= [](int x) { return x * 10; },
            rl::SomeAny >>= [] { return std::string("text"); },
            rl::None    >>= [] { return rl::None; }
        );
        static_assert(std::same_as<decltype(multiRes), rl::Option<int, std::string>>);
        TEST_ASSERT(multiRes.is_some<int>() && multiRes.unwrap<int>() == 50);

        // Explicit multi-type template argument: match<int, std::string>
        auto explicitMulti = opt.template match<int, std::string>(
            rl::Some(5) >>= [](int x) { return x * 20; },
            rl::SomeAny >>= [] { return std::string("text"); },
            rl::None    >>= [] { return rl::None; }
        );
        static_assert(std::same_as<decltype(explicitMulti), rl::Option<int, std::string>>);
        TEST_ASSERT(explicitMulti.is_some<int>() && explicitMulti.unwrap<int>() == 100);
    }

    std::cout << "  [PASS] Section 6: Advanced Pattern Matching passed!\n";
}

// ============================================================================
// SECTION 7: Monadic APIs (map, and_then, or_else, filter, inspect)
// ============================================================================
void test_section_7_monadic_apis() {
    std::cout << "--- Running Section 7: Monadic APIs ---\n";

    // 1. map() value transformation
    {
        rl::Option<int> opt = rl::Some(21);
        auto doubled = opt.map([](int x) { return x * 2; });
        static_assert(std::same_as<decltype(doubled), rl::Option<int>>);
        TEST_ASSERT(doubled.is_some() && doubled.unwrap() == 42);

        // map() changing type
        auto toStr = opt.map([](int x) { return std::to_string(x); });
        static_assert(std::same_as<decltype(toStr), rl::Option<std::string>>);
        TEST_ASSERT(toStr.is_some() && toStr.unwrap() == "21");

        // map() on None
        rl::Option<int> optNone = rl::None;
        auto mapNone = optNone.map([](int x) { return x * 2; });
        TEST_ASSERT(mapNone.is_none());
    }

    // 2. and_then() flatMap
    {
        auto safeDivide = [](int a, int b) -> rl::Option<int> {
            if (b == 0) return rl::None;
            return rl::Some(a / b);
        };

        rl::Option<int> num = rl::Some(100);
        auto div1 = num.and_then([&](int x) { return safeDivide(x, 2); });
        TEST_ASSERT(div1.is_some() && div1.unwrap() == 50);

        auto divZero = num.and_then([&](int x) { return safeDivide(x, 0); });
        TEST_ASSERT(divZero.is_none());

        rl::Option<int> numNone = rl::None;
        auto divFromNone = numNone.and_then([&](int x) { return safeDivide(x, 2); });
        TEST_ASSERT(divFromNone.is_none());
    }

    // 3. or_else() fallback
    {
        rl::Option<int> optSome = rl::Some(50);
        rl::Option<int> optNone = rl::None;

        auto res1 = optSome.or_else([]() -> rl::Option<int> { return rl::Some(999); });
        TEST_ASSERT(res1.unwrap() == 50);

        auto res2 = optNone.or_else([]() -> rl::Option<int> { return rl::Some(999); });
        TEST_ASSERT(res2.unwrap() == 999);
    }

    // 4. map_or_else()
    {
        rl::Option<int> optSome = rl::Some(10);
        rl::Option<int> optNone = rl::None;

        auto r1 = optSome.map_or_else([](int x) { return x * 5; }, [] { return 0; });
        TEST_ASSERT(r1 == 50);

        auto r2 = optNone.map_or_else([](int x) { return x * 5; }, [] { return 999; });
        TEST_ASSERT(r2 == 999);
    }

    // 6. unwrap_or() & unwrap_or_else()
    {
        rl::Option<int> optSome = rl::Some(10);
        rl::Option<int> optNone = rl::None;

        TEST_ASSERT(optSome.unwrap_or(99) == 10);
        TEST_ASSERT(optNone.unwrap_or(99) == 99);

        TEST_ASSERT(optSome.unwrap_or_else([] { return 123; }) == 10);
        TEST_ASSERT(optNone.unwrap_or_else([] { return 123; }) == 123);
    }

    std::cout << "  [PASS] Section 7: Monadic APIs passed!\n";
}

// ============================================================================
// SECTION 8: Hash & Standard Containers
// ============================================================================
struct UnhashableType {
    int v;
    bool operator==(const UnhashableType&) const = default;
};

void test_section_8_hash_and_containers() {
    std::cout << "--- Running Section 8: Hash & Standard Containers ---\n";

    // 1. std::unordered_set<Option<int>>
    {
        std::unordered_set<rl::Option<int>> optSet;
        optSet.insert(rl::Some(1));
        optSet.insert(rl::Some(2));
        optSet.insert(rl::Some(3));
        optSet.insert(rl::None);

        TEST_ASSERT(optSet.size() == 4);
        TEST_ASSERT(optSet.contains(rl::Some(1)));
        TEST_ASSERT(optSet.contains(rl::Some(2)));
        TEST_ASSERT(optSet.contains(rl::Some(3)));
        TEST_ASSERT(optSet.contains(rl::None));
        TEST_ASSERT(!optSet.contains(rl::Some(4)));

        // Duplicate insert
        optSet.insert(rl::Some(1));
        optSet.insert(rl::None);
        TEST_ASSERT(optSet.size() == 4);

        // Erasing None
        optSet.erase(rl::None);
        TEST_ASSERT(optSet.size() == 3);
        TEST_ASSERT(!optSet.contains(rl::None));
    }

    // 2. std::unordered_map<Option<std::string>, int>
    {
        std::unordered_map<rl::Option<std::string>, int> optMap;
        optMap[rl::Some<std::string>("key1")] = 100;
        optMap[rl::Some<std::string>("key2")] = 200;
        optMap[rl::None] = 999;

        TEST_ASSERT(optMap.size() == 3);
        TEST_ASSERT(optMap[rl::Some<std::string>("key1")] == 100);
        TEST_ASSERT(optMap[rl::Some<std::string>("key2")] == 200);
        TEST_ASSERT(optMap[rl::None] == 999);
    }

    // 3. Verification that std::hash is SFINAE-disabled for unhashable types
    {
        static_assert(!std::is_default_constructible_v<std::hash<rl::Option<UnhashableType>>>,
                      "std::hash must be disabled for Option<UnhashableType>");
    }

    std::cout << "  [PASS] Section 8: Hash & Standard Containers passed!\n";
}

// ============================================================================
// SECTION 9: Compile-Time / Constexpr Evaluation
// ============================================================================
constexpr bool test_constexpr_suite() {
    // 1. Constexpr construction
    constexpr rl::Option<int> crl1 = rl::Some(42);
    static_assert(crl1.is_some());
    static_assert(!crl1.is_none());
    static_assert(crl1.unwrap() == 42);

    constexpr rl::Option<int> crlNone = rl::None;
    static_assert(crlNone.is_none());
    static_assert(!crlNone.is_some());

    // 2. Constexpr unwrap_or
    static_assert(crl1.unwrap_or(0) == 42);
    static_assert(crlNone.unwrap_or(123) == 123);

    // 3. Constexpr comparison
    constexpr rl::Option<int> crl2 = rl::Some(42);
    constexpr rl::Option<int> crl3 = rl::Some(50);
    static_assert(crl1 == crl2);
    static_assert(crl1 != crl3);
    static_assert((crl1 <=> crl3) == std::strong_ordering::less);
    static_assert((crl3 <=> crl1) == std::strong_ordering::greater);
    static_assert((crl1 <=> crlNone) == std::strong_ordering::greater);

    // 4. Default construction is deleted; rl::Default must be used explicitly
    static_assert(!std::is_default_constructible_v<rl::Option<int>>);
    constexpr rl::Option<int> crlDefault = rl::Default;
    static_assert(crlDefault.is_some() && crlDefault.unwrap() == 0);

    // 5. Multi-type option in constexpr
    constexpr rl::Option<int, double> cMulti1 = rl::Some<int>(10);
    static_assert(cMulti1.is_some<int>());
    static_assert(!cMulti1.is_some<double>());
    static_assert(cMulti1.unwrap<int>() == 10);

    constexpr rl::Option<int, double> cMulti2 = rl::Some<double>(3.5);
    static_assert(cMulti2.is_some<double>());
    static_assert(!cMulti2.is_some<int>());
    static_assert(cMulti2.unwrap<double>() == 3.5);

    return true;
}

static_assert(test_constexpr_suite(), "Constexpr suite must evaluate at compile time");

void test_section_9_constexpr_evaluation() {
    std::cout << "--- Running Section 9: Compile-Time & Constexpr Evaluation ---\n";
    bool ok = test_constexpr_suite();
    TEST_ASSERT(ok);
    std::cout << "  [PASS] Section 9: Compile-Time & Constexpr Evaluation passed!\n";
}

// Concepts to verify borrowability on lvalues
template<typename T>
concept CanLvalueAsImmutRef = requires(T& opt) { opt.as_immut_ptr(); };
template<typename T>
concept CanConstLvalueAsImmutRef = requires(const T& opt) { opt.as_immut_ptr(); };
template<typename T>
concept CanLvalueAsMutRef = requires(T& opt) { opt.as_mut_ptr(); };

template<typename T>
concept CanRvalueAsImmutRef = requires(T&& opt) { std::move(opt).as_immut_ptr(); };
template<typename T>
concept CanRvalueAsMutRef = requires(T&& opt) { std::move(opt).as_mut_ptr(); };

template<typename T, typename U>
concept CanLvalueVariadicAsImmutRef = requires(T& opt) { opt.template as_immut_ptr<U>(); };
template<typename T, typename U>
concept CanLvalueVariadicAsMutRef = requires(T& opt) { opt.template as_mut_ptr<U>(); };
template<typename T, typename U>
concept CanRvalueVariadicAsImmutRef = requires(T&& opt) { std::move(opt).template as_immut_ptr<U>(); };
template<typename T, typename U>
concept CanRvalueVariadicAsMutRef = requires(T&& opt) { std::move(opt).template as_mut_ptr<U>(); };

template<typename T>
concept CanLvalueTake = requires(T& opt) { opt.take(); };
template<typename T>
concept CanLvalueReplace = requires(T& opt) { opt.replace(10); };

// ============================================================================
// SECTION 10: Intensive Rust Lifetimes, Borrowing & Stress Suite
// ============================================================================
void test_section_10_rust_lifetimes_and_stress() {
    std::cout << "--- Running Section 10: Intensive Rust Lifetimes & Stress Suite ---\n";

    // 1. Rust-style as_immut_ptr() (immutable borrow)
    {
        rl::Option<int> opt = rl::Some(42);
        auto opt_ref = opt.as_immut_ptr();
        TEST_ASSERT(opt_ref.is_some());
        TEST_ASSERT(*opt_ref.unwrap() == 42);
        // Address check: points directly to opt's inner value
        TEST_ASSERT(opt_ref.unwrap() == &opt.unwrap());

        const rl::Option<int> const_opt = rl::Some(100);
        auto const_ref = const_opt.as_immut_ptr();
        TEST_ASSERT(const_ref.is_some());
        TEST_ASSERT(*const_ref.unwrap() == 100);

        rl::Option<int> none_opt = rl::None;
        auto none_ref = none_opt.as_immut_ptr();
        TEST_ASSERT(none_ref.is_none());
    }

    // 2. Rust-style as_mut_ptr() (mutable borrow & in-place modification)
    {
        rl::Option<int> opt = rl::Some(10);
        auto opt_mut = opt.as_mut_ptr();
        TEST_ASSERT(opt_mut.is_some());
        *opt_mut.unwrap() = 999;
        // Verify in-place mutation
        TEST_ASSERT(opt.unwrap() == 999);

        rl::Option<int> none_opt = rl::None;
        auto none_mut = none_opt.as_mut_ptr();
        TEST_ASSERT(none_mut.is_none());
    }

    // 3. Variadic Multi-Type as_immut_ptr() & as_mut_ptr()
    {
        rl::Option<int, std::string, double> multi = std::string("rust_borrow");
        auto str_ref = multi.template as_immut_ptr<std::string>();
        TEST_ASSERT(str_ref.is_some());
        TEST_ASSERT(*str_ref.unwrap() == "rust_borrow");

        auto int_ref = multi.template as_immut_ptr<int>();
        TEST_ASSERT(int_ref.is_none());

        auto dbl_ref = multi.template as_immut_ptr<double>();
        TEST_ASSERT(dbl_ref.is_none());

        auto str_mut = multi.template as_mut_ptr<std::string>();
        TEST_ASSERT(str_mut.is_some());
        *str_mut.unwrap() += "_extended";
        TEST_ASSERT(multi.unwrap<std::string>() == "rust_borrow_extended");
    }

    // 4. Compile-time Prevention of Dangling References (Rvalue Rejection)
    {
        // Rvalue overloads (&& and const &&) for as_immut_ptr(), as_mut_ptr(),
        // as_immut_ptr(), as_mut_ptr(), take(), and replace() are explicitly deleted with = delete.
        // In standard C++, attempting to call a deleted function is a hard compile-time error,
        // guaranteeing that temporary rvalues cannot be borrowed from or result in dangling pointers.

        static_assert(CanLvalueAsImmutRef<rl::Option<int>>);
        static_assert(CanConstLvalueAsImmutRef<rl::Option<int>>);
        static_assert(CanLvalueAsMutRef<rl::Option<int>>);

        static_assert(!CanRvalueAsImmutRef<rl::Option<int>>);
        static_assert(!CanRvalueAsMutRef<rl::Option<int>>);
        static_assert(!CanRvalueAsImmutRef<const rl::Option<int>>);
        static_assert(!CanRvalueAsMutRef<const rl::Option<int>>);

        static_assert(CanLvalueVariadicAsImmutRef<rl::Option<int, std::string>, std::string>);
        static_assert(CanLvalueVariadicAsMutRef<rl::Option<int, std::string>, std::string>);
        static_assert(!CanRvalueVariadicAsImmutRef<rl::Option<int, std::string>, std::string>);
        static_assert(!CanRvalueVariadicAsMutRef<rl::Option<int, std::string>, std::string>);

        static_assert(CanLvalueTake<rl::Option<int>>);
        static_assert(CanLvalueReplace<rl::Option<int>>);
        static_assert(CanLvalueTake<rl::Option<int, std::string>>);

        TEST_ASSERT(true);
    }

    // 5. Rust take() & replace() Lifecycle Semantics
    {
        rl::Option<std::string> opt = rl::Some(std::string("first_value"));
        auto taken = opt.take();
        TEST_ASSERT(taken.is_some() && taken.unwrap() == "first_value");
        TEST_ASSERT(opt.is_none()); // Original option is now disengaged None!

        auto taken_again = opt.take();
        TEST_ASSERT(taken_again.is_none()); // Second take on None returns None
        TEST_ASSERT(opt.is_none());

        auto prev = opt.replace(std::string("new_value"));
        TEST_ASSERT(prev.is_none()); // Was None before replacement
        TEST_ASSERT(opt.is_some() && opt.unwrap() == "new_value");

        auto prev2 = opt.replace(std::string("third_value"));
        TEST_ASSERT(prev2.is_some() && prev2.unwrap() == "new_value");
        TEST_ASSERT(opt.is_some() && opt.unwrap() == "third_value");
    }

    // 6. Variadic Multi-Type take() & replace()
    {
        rl::Option<int, std::string> multi = std::string("variadic_payload");
        TEST_ASSERT(multi.is_some<std::string>());
        auto taken = multi.take();
        TEST_ASSERT(taken.is_some<std::string>() && taken.unwrap<std::string>() == "variadic_payload");
        TEST_ASSERT(multi.is_none());

        auto old_none = multi.replace(42);
        TEST_ASSERT(old_none.is_none());
        TEST_ASSERT(multi.is_some<int>() && multi.unwrap<int>() == 42);

        auto old_int = multi.replace(std::string("swapped_to_string"));
        TEST_ASSERT(old_int.is_some<int>() && old_int.unwrap<int>() == 42);
        TEST_ASSERT(multi.is_some<std::string>() && multi.unwrap<std::string>() == "swapped_to_string");
    }

    // 7. C++26 Constructibility & Movability Trait Assertions
    {
        // Move-only type
        static_assert(!std::is_copy_constructible_v<rl::Option<MoveOnlyType>>);
        static_assert(std::is_move_constructible_v<rl::Option<MoveOnlyType>>);
        static_assert(!std::is_copy_assignable_v<rl::Option<MoveOnlyType>>);
        static_assert(std::is_move_assignable_v<rl::Option<MoveOnlyType>>);

        // Immovable type
        static_assert(!std::is_copy_constructible_v<rl::Option<ImmovableType>>);
        static_assert(!std::is_move_constructible_v<rl::Option<ImmovableType>>);
        static_assert(!std::is_copy_assignable_v<rl::Option<ImmovableType>>);
        static_assert(!std::is_move_assignable_v<rl::Option<ImmovableType>>);

        // Copy-only type
        static_assert(std::is_copy_constructible_v<rl::Option<CopyOnlyType>>);
        static_assert(!std::is_move_constructible_v<rl::Option<CopyOnlyType>>);

        // Multi-type with move-only member
        static_assert(!std::is_copy_constructible_v<rl::Option<int, MoveOnlyType>>);
        static_assert(std::is_move_constructible_v<rl::Option<int, MoveOnlyType>>);
        static_assert(!std::is_copy_assignable_v<rl::Option<int, MoveOnlyType>>);
        static_assert(std::is_move_assignable_v<rl::Option<int, MoveOnlyType>>);

        // Triviality
        static_assert(std::is_trivially_destructible_v<rl::Option<int>>);
        static_assert(std::is_trivially_copy_constructible_v<rl::Option<int>>);
        static_assert(std::is_trivially_move_constructible_v<rl::Option<int>>);
        static_assert(std::is_trivially_destructible_v<rl::Option<int, double>>);

        TEST_ASSERT(true);
    }

    // 8. TrackedLifecycle with take(), unwrap() &&, and replace()
    {
        TrackedLifecycle::reset_counters();
        {
            rl::Option<TrackedLifecycle> opt = rl::Some(TrackedLifecycle(555));
            TEST_ASSERT(TrackedLifecycle::instances_alive == 1);
            rl::Option<TrackedLifecycle> taken = opt.take();
            TEST_ASSERT(opt.is_none());
            TEST_ASSERT(taken.is_some() && taken.unwrap().id == 555);
            TEST_ASSERT(TrackedLifecycle::instances_alive == 1);

            // Replace taken's value
            auto old = taken.replace(TrackedLifecycle(666));
            TEST_ASSERT(old.is_some() && old.unwrap().id == 555);
            TEST_ASSERT(taken.is_some() && taken.unwrap().id == 666);
            TEST_ASSERT(TrackedLifecycle::instances_alive == 2);
        }
        // All destroyed, zero leaks
        TEST_ASSERT(TrackedLifecycle::instances_alive == 0);
    }

    // 9. Intensive 100,000-Iteration Stress Loop
    {
        TrackedLifecycle::reset_counters();
        constexpr int NUM_ITERATIONS = 100'000;
        long long sum = 0;

        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            if (i % 2 == 0) {
                rl::Option<int, std::string> opt = i;
                auto r = opt.as_immut_ptr();
                if (!r.is_some() || *r.unwrap() != i) {
                    TEST_ASSERT(false);
                }

                auto m = opt.as_mut_ptr();
                if (!m.is_some()) {
                    TEST_ASSERT(false);
                }
                *m.unwrap() += 1;

                auto match_res = opt.match(
                    rl::SomeAny >>= [](auto&& x) -> int {
                        using Decayed = std::decay_t<decltype(x)>;
                        if constexpr (std::same_as<Decayed, int>) {
                            return x * 2;
                        } else {
                            return static_cast<int>(x.length());
                        }
                    },
                    rl::None >>= [] { return 0; }
                );
                sum += match_res.unwrap();

                auto taken = opt.take();
                if (!opt.is_none() || !taken.is_some<int>() || taken.unwrap<int>() != i + 1) {
                    TEST_ASSERT(false);
                }
            } else {
                rl::Option<int, std::string> opt = std::string("stress_test_payload");
                auto r = opt.template as_immut_ptr<std::string>();
                if (!r.is_some() || *r.unwrap() != "stress_test_payload") {
                    TEST_ASSERT(false);
                }

                auto m = opt.template as_mut_ptr<std::string>();
                if (!m.is_some()) {
                    TEST_ASSERT(false);
                }
                m.unwrap()->push_back('!');

                auto taken = opt.take();
                if (!opt.is_none() || !taken.is_some<std::string>() || taken.unwrap<std::string>() != "stress_test_payload!") {
                    TEST_ASSERT(false);
                }
            }
        }
        TEST_ASSERT(sum > 0);
        TEST_ASSERT(true);
    }

    // 10. Fallback Default Construction & Equality for Non-Default Primary
    {
        struct NonDefaultPrimary {
            int val;
            [[ maybe_unused ]] explicit NonDefaultPrimary(int v) : val(v) {}
            [[ maybe_unused ]] bool operator==(const NonDefaultPrimary& o) const = default;
        };
        rl::Option<NonDefaultPrimary, double> optDef = rl::Default;
        TEST_ASSERT(optDef.is_some<double>());
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wfloat-equal"
#endif
        TEST_ASSERT(optDef.unwrap<double>() == 0.0);
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif
        TEST_ASSERT(optDef == rl::Default);
        TEST_ASSERT(rl::Default == optDef);
    }

    std::cout << "  [PASS] Section 10: Intensive Rust Lifetimes & Stress Suite passed!\n";
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "RUNNING EXHAUSTIVE 10-SECTION TEST SUITE FOR OPTION<T>\n";
    std::cout << "========================================================\n\n";

    test_section_1_lifecycles();
    test_section_2_pathological_types();
    test_section_3_floats_spaceship_pointers();
    test_section_4_exception_safety();
    test_section_5_variadic_multi_type();
    test_section_6_pattern_matching_edge_cases();
    test_section_7_monadic_apis();
    test_section_8_hash_and_containers();
    test_section_9_constexpr_evaluation();
    test_section_10_rust_lifetimes_and_stress();

    std::cout << "\n========================================================\n";
    std::cout << "ALL 10 SECTIONS COMPLETED WITH 100% SUCCESS!\n";
    std::cout << "Total Assertions Run:      " << g_test_count << "\n";
    std::cout << "Total Assertions Passed:   " << g_pass_count << "\n";
    std::cout << "========================================================\n";
    return 0;
}
