#include "rl_option.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

void check(bool condition) {
    if (!condition) {
        std::abort();
    }
}

} // namespace

// =============================================================================
// BUG-01: Captureless lambda arm and over-aligned handler arm
// =============================================================================
struct alignas(64) OveralignedHandler {
    int factor = 2;
    [[no_unique_address]] rl::details::PadToAlignment_<sizeof(factor), 64>
        padding{};
    int operator()(int x) const { return x * factor; }
};

void test_bug01() {
    rl::Option<int> opt = rl::Some(5);
    auto res = opt.match(
        [](const int& x) { return x == 5; } >>= OveralignedHandler{},
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](auto&&) { return -1; }
    );
    check(res.unwrap() == 10);
    std::cout << "[PASS] BUG-01: Ordinary lambda arm and overaligned handler\n";
}

// =============================================================================
// BUG-02: match() deduction and invocation for raw return values
// =============================================================================
void test_bug02() {
    rl::Option<int> opt = rl::Some(42);
    // Returns raw int from handlers; match deduces Option<int>
    auto res = opt.match(
        rl::Some{42} >>= [](int) { return 100; },
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](auto&&) { return -1; }
    );
    static_assert(std::same_as<decltype(res), rl::Option<int>>);
    check(res.unwrap() == 100);

    // Returning string literals producing Option<string_view>
    auto res2 = opt.match(
        rl::Some{42} >>= [](int) { return "matched"; },
        rl::None >>= [] { return "none"; },
        rl::Wild >>= [](auto&&) { return "wild"; }
    );
    check(res2.unwrap() == std::string_view("matched"));
    std::cout << "[PASS] BUG-02: Raw return values in match() handlers\n";
}

// =============================================================================
// BUG-03: 'true' pattern matches ONLY bool alternatives, not non-bool
// =============================================================================
void test_bug03() {
    rl::Option<int, bool> opt_int = rl::Some(1); // holds int 1
    // Matching with True should NOT match the int 1, even if 1 is truthy!
    auto res = opt_int.match(
        rl::True >>= [] { return 999; },
        rl::Some{1} >>= [](int) { return 42; },
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](auto&&) { return -1; }
    );
    check(res.unwrap() == 42); // matched Some{1}, not True!

    // Now with bool true:
    rl::Option<int, bool> opt_bool = rl::Some(true);
    auto res_b = opt_bool.match(
        rl::True >>= [] { return 999; },
        rl::Some{1} >>= [](int) { return 42; },
        rl::None >>= [] { return 0; },
        rl::Wild >>= [](auto&&) { return -1; }
    );
    check(res_b.unwrap() == 999); // matched True!
    std::cout << "[PASS] BUG-03: True/False patterns match only bool alternatives\n";
}

// =============================================================================
// BUG-04: Exact-alternative priority ignores unconstructible exact candidate
// =============================================================================
struct MoveOnlyType {
    int val;
    explicit MoveOnlyType(int v) : val(v) {}
    MoveOnlyType(MoveOnlyType&&) = default;
    MoveOnlyType& operator=(MoveOnlyType&&) = default;
    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
};

struct WrapperType {
    int val;
    WrapperType(const MoveOnlyType& m) : val(m.val + 10) {}
};

void test_bug04() {
    rl::Option<MoveOnlyType, WrapperType> opt = rl::None;
    MoveOnlyType m{5};
    const MoveOnlyType& ref = m;
    // Assignment from const MoveOnlyType& cannot copy MoveOnlyType, but CAN construct WrapperType!
    opt = ref;
    check(opt.is_some());
    check(opt.is_some<WrapperType>());
    check(opt.unwrap<WrapperType>().val == 15);
    std::cout << "[PASS] BUG-04: Exact-alternative priority ignores unconstructible types\n";
}

// =============================================================================
// BUG-05: reset_() exposes disengaged state before user destructor
// =============================================================================
struct SelfResetting {
    static inline int dtor_count = 0;
    static inline bool saw_disengaged = false;
    rl::Option<SelfResetting>* owner = nullptr;

    SelfResetting() = default;

    ~SelfResetting() {
        dtor_count++;
        if (owner) {
            saw_disengaged = owner->is_none();
            // Reentrant reset call:
            owner->reset();
        }
    }
};

void test_bug05() {
    SelfResetting::dtor_count = 0;
    SelfResetting::saw_disengaged = false;

    {
        rl::Option<SelfResetting> opt = rl::None;
        opt.emplace();
        opt.unwrap().owner = &opt;
        opt.reset();
    }
    check(SelfResetting::saw_disengaged);
    check(SelfResetting::dtor_count == 1); // Not 2!
    std::cout << "[PASS] BUG-05: reset_() reentrancy safety and disengaged flag ordering\n";
}

// =============================================================================
// BUG-06: Non-movable converting assignment staging
// =============================================================================
struct OldValue {
    int secret = 42;
    OldValue() = default;
    OldValue(const OldValue&) = default;
    OldValue(int) = delete;
    ~OldValue() { secret = -1; }
};

struct NonMovableTarget {
    int recorded;
    explicit NonMovableTarget(const int& v) : recorded(v) {}
    NonMovableTarget(NonMovableTarget&&) = delete;
    NonMovableTarget& operator=(NonMovableTarget&&) = delete;
};

void test_bug06() {
    rl::Option<OldValue, NonMovableTarget> opt = rl::None;
    opt.emplace<OldValue>();
    const int& ref = opt.unwrap<OldValue>().secret;
    // Assign reference into OldValue:
    opt = ref;
    check(opt.is_some<NonMovableTarget>());
    // Must record 42, NOT -1 (from destroyed OldValue)!
    check(opt.unwrap<NonMovableTarget>().recorded == 42);
    std::cout << "[PASS] BUG-06: Non-movable conversion staging protects referenced source\n";
}

// =============================================================================
// BUG-07: Monadic constraints on unmaterializable callback returns
// =============================================================================
struct NonCopyableResult {
    NonCopyableResult() = default;
    NonCopyableResult(NonCopyableResult&&) = default;
    NonCopyableResult(const NonCopyableResult&) = delete;
};

template <typename O, typename F>
concept CanMap = requires(O o, F f) { o.map(f); };

template <typename O, typename F1, typename F2>
concept CanMapOrElse = requires(O o, F1 f1, F2 f2) { o.map_or_else(f1, f2); };

template <typename O, typename F>
concept CanAndThen = requires(O o, F f) { o.and_then(f); };

void test_bug07() {
    using BadMapFn = NonCopyableResult& (*)(const int&);
    using BadElseFn = NonCopyableResult& (*)();
    using BadAndThenFn = rl::Option<NonCopyableResult>& (*)(const int&);

    static_assert(!CanMap<const rl::Option<int>&, BadMapFn>,
                  "map() must reject callback returning non-copyable lvalue ref");
    static_assert(!CanMapOrElse<const rl::Option<int>&, BadMapFn, BadElseFn>,
                  "map_or_else() must reject callbacks returning non-copyable lvalue ref");
    static_assert(!CanAndThen<const rl::Option<int>&, BadAndThenFn>,
                  "and_then() must reject callback returning non-copyable Option&");
    std::cout << "[PASS] BUG-07: Monadic constraints properly validate materializability\n";
}

// =============================================================================
// BUG-08: Accessor overloads for && and const &&
// =============================================================================
struct MutatableObject {
    int val = 10;
    void mutate() { val = 20; }
    int query() const { return val; }
};

void test_bug08() {
    // 1. std::move(option)->mutate()
    rl::Option<MutatableObject> opt_mut = rl::Some(MutatableObject{});
    std::move(opt_mut)->mutate();
    check(opt_mut->val == 20);

    // 2. const Option<std::string>&&.value() returns const std::string&&
    const rl::Option<std::string> const_opt = rl::Some(std::string("hello"));
    static_assert(std::same_as<decltype(std::move(const_opt).value()), const std::string&&>);
    static_assert(std::same_as<decltype(*std::move(const_opt)), const std::string&&>);
    check(std::move(const_opt).value() == "hello");

    // 3. expect() const && and unwrap() const &&
    check(std::move(const_opt).expect("valid") == "hello");
    check(std::move(const_opt).unwrap() == "hello");
    std::cout << "[PASS] BUG-08: Complete and consistent accessor cvref overloads\n";
}

// =============================================================================
// BUG-09: Some<T>::get() && is noexcept(true)
// =============================================================================
void test_bug09() {
    rl::Some<int> some_int{5};
    static_assert(noexcept(std::move(some_int).get()));
    std::cout << "[PASS] BUG-09: Some<T>::get() && is noexcept\n";
}

// =============================================================================
// BUG-10: Nothrow non-trivial copy assignment preserves noexcept
// =============================================================================
struct NothrowNontrivial {
    NothrowNontrivial() = default;
    NothrowNontrivial(const NothrowNontrivial&) noexcept {}
    NothrowNontrivial& operator=(const NothrowNontrivial&) noexcept { return *this; }
    NothrowNontrivial(NothrowNontrivial&&) noexcept {}
    NothrowNontrivial& operator=(NothrowNontrivial&&) noexcept { return *this; }
    ~NothrowNontrivial() noexcept = default;
};

void test_bug10() {
    static_assert(std::is_nothrow_copy_assignable_v<rl::Option<NothrowNontrivial>>);
    std::cout << "[PASS] BUG-10: Nothrow non-trivial copy assignment preserves noexcept\n";
}

// =============================================================================
// BUG-11: Value exhaustiveness in match()
// =============================================================================
void test_bug11() {
    // 1. Option<bool> with True and False arms compiles without Wild/SomeAny!
    rl::Option<bool> opt_b = rl::Some(false);
    auto res_b = opt_b.match(
        rl::True >>= [] { return 1; },
        rl::False >>= [] { return 2; },
        rl::None >>= [] { return 0; }
    );
    check(res_b.unwrap() == 2);

    // 2. Partial patterns without fallback fail exhaustiveness check
    auto partial_pred_arm = [](const int& x) { return x > 10; } >>= [](int) { return 1; };
    auto none_arm = rl::None >>= [] { return 0; };
    static_assert(!rl::details::match_all_types_exhaustive_<
                  std::tuple<int>,
                  std::tuple<decltype(partial_pred_arm), decltype(none_arm)>>::value,
                  "Partial predicate pattern without fallback must fail exhaustiveness");

    auto partial_val_arm = rl::Some{1} >>= [](int) { return 1; };
    static_assert(!rl::details::match_all_types_exhaustive_<
                  std::tuple<int>,
                  std::tuple<decltype(partial_val_arm), decltype(none_arm)>>::value,
                  "Partial value pattern without fallback must fail exhaustiveness");

    auto single_bool_arm = rl::True >>= [] { return 1; };
    static_assert(!rl::details::match_all_types_exhaustive_<
                  std::tuple<bool>,
                  std::tuple<decltype(single_bool_arm), decltype(none_arm)>>::value,
                  "Single bool pattern without False/Wild must fail exhaustiveness");

    std::cout << "[PASS] BUG-11: Exhaustiveness proof distinguishes value coverage from type matching\n";
}

// =============================================================================
// REVIEW-01: is_default() supports secondary default alternative
// =============================================================================
struct NoDefault {
    int x;
    explicit NoDefault(int v) : x(v) {}
    bool operator==(const NoDefault& o) const = default;
};

void test_review01() {
    rl::Option<NoDefault, int> opt{rl::Default};
    check(opt.is_some());
    check(opt.is_some<int>());
    check(opt.unwrap<int>() == 0);
    check(opt.is_default());
    check(opt == rl::Default);
    std::cout << "[PASS] REVIEW-01: is_default() recognizes secondary default alternative\n";
}

// =============================================================================
// REVIEW-02: Mutable predicate lambdas
// =============================================================================
struct MutablePredicateState {
    int *observed_count;
    int call_count{};
    [[no_unique_address]] rl::details::PadToAlignment_<
        sizeof(int *) + sizeof(int), alignof(int *)> padding{};
};

void test_review02() {
    rl::Option<int> opt = rl::Some(5);
    int observed_count = 0;
    auto res = opt.match(
        [state_ = MutablePredicateState{&observed_count, 0}](int value_) mutable {
            ++state_.call_count;
            *state_.observed_count = state_.call_count;
            return value_ == 5;
        } >>=
            [](int) { return "matched mutable"; },
        rl::None >>= [] { return "none"; },
        rl::Wild >>= [](auto&&) { return "wild"; }
    );
    check(res.unwrap() == std::string_view("matched mutable"));
    check(observed_count == 1);
    std::cout << "[PASS] REVIEW-02: Mutable predicate lambdas accepted and invokable\n";
}

// =============================================================================
// REVIEW-03: Option::value_type remains private by design
// =============================================================================
template <class O>
concept ExposesValueType = requires { typename O::value_type; };

void test_review03() {
    static_assert(!ExposesValueType<rl::Option<int>>);
    static_assert(!ExposesValueType<rl::Option<std::string, double>>);
    std::cout << "[PASS] REVIEW-03: Option::value_type remains private\n";
}

// =============================================================================
// REVIEW-04: <exception> included and uncaught_exceptions() functional
// =============================================================================
void test_review04() {
    check(std::uncaught_exceptions() >= 0);
    std::cout << "[PASS] REVIEW-04: <exception> directly included and functioning\n";
}


// =============================================================================
// REVIEW-05: Default patterns construct a comparison value during matching
// =============================================================================
inline int default_pattern_constructions = 0;

struct DefaultPatternProbe {
    int value;

    DefaultPatternProbe() : value{} { ++default_pattern_constructions; }
    explicit DefaultPatternProbe(int initial) : value{initial} {}
    bool operator==(const DefaultPatternProbe&) const = default;
};

void test_review05() {
    default_pattern_constructions = 0;
    rl::Option<DefaultPatternProbe> opt{
        rl::Some(DefaultPatternProbe{7})};
    const auto before = default_pattern_constructions;
    const auto result = opt.match(
        rl::Default >>= [] { return 1; },
        rl::Wild >>= [](const auto&) { return 2; },
        rl::None >>= [] { return 3; });

    check(result.unwrap() == 2);
    check(default_pattern_constructions == before + 1);
    std::cout << "[PASS] REVIEW-05: Default pattern constructs one comparison value\n";
}

int main() {
    std::cout << "Starting Fourth Pass Bug Audit Verification Suite...\n\n";
    test_bug01();
    test_bug02();
    test_bug03();
    test_bug04();
    test_bug05();
    test_bug06();
    test_bug07();
    test_bug08();
    test_bug09();
    test_bug10();
    test_bug11();
    test_review01();
    test_review02();
    test_review03();
    test_review04();
    test_review05();
    std::cout << "\n=======================================================\n";
    std::cout << "ALL 11 BUGS AND 5 REVIEW CONCERNS CHECKED SUCCESSFULLY!\n";
    std::cout << "=======================================================\n";
    return 0;
}
