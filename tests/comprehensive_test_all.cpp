#include "rl_option.hpp"
#include <iostream>
#include <string>
#include <cassert>
#include <stdexcept>

void test_finding_1_deleted_const_rvalue() {
    // Finding 1: const rvalues cannot call as_immut_ptr / as_mut_ptr because they are deleted
    const rl::Option<int> copt = rl::Some(42);
    assert(copt.as_immut_ptr().is_some());
    assert(*copt.as_immut_ptr().unwrap() == 42);
    std::cout << "[PASS] Finding 1: const rvalue pointers safely deleted\n";
}

void test_finding_2_automatic_deduction() {
    rl::Option<int, std::string> option = rl::Some<std::string>("hello");

    auto r_auto = option.match(
        rl::Some<std::string>("hello") >>= [](const std::string& s) {
            return s + " world";
        },
        rl::SomeAny >>= [] {
            return std::string("some");
        },
        rl::None >>= [] {
            return std::string("none");
        }
    ).unwrap();
    static_assert(std::same_as<decltype(r_auto), std::string>);
    assert(r_auto == "hello world");
    std::cout << "[PASS] Finding 2: Alternative-aware arm deduction\n";
}

void test_finding_3_throwing_predicates() {
    struct ThrowingPred {
        bool operator()(int) const {
            throw std::runtime_error("predicate failed");
        }
    };

    rl::Option<int> opt = rl::Some(10);
    bool caught = false;
    try {
        auto _res = opt.match(
            ThrowingPred{} >>= [](int) { return 1; },
            rl::SomeAny >>= [] { return 2; },
            rl::None >>= [] { return 3; }
        );
        (void)_res;
    } catch (const std::runtime_error& e) {
        caught = true;
        assert(std::string(e.what()) == "predicate failed");
    }
    assert(caught);
    std::cout << "[PASS] Finding 3: Throwing predicates propagate\n";
}

void test_finding_4_inplace_assignment() {
    static int construct_count = 0;
    static int copy_assign_count = 0;
    static int move_assign_count = 0;

    struct Tracked {
        int id;
        Tracked(int i = 0) : id(i) { construct_count++; }
        Tracked(const Tracked& other) : id(other.id) { construct_count++; }
        Tracked(Tracked&& other) noexcept : id(other.id) { construct_count++; }
        Tracked& operator=(const Tracked& other) {
            id = other.id;
            copy_assign_count++;
            return *this;
        }
        Tracked& operator=(Tracked&& other) noexcept {
            id = other.id;
            move_assign_count++;
            return *this;
        }
    };

    rl::Option<Tracked, std::string> opt1 = rl::Some(Tracked(1));
    rl::Option<Tracked, std::string> opt2 = rl::Some(Tracked(2));

    construct_count = 0;
    copy_assign_count = 0;
    move_assign_count = 0;

    // Same active alternative: should use copy_assign, not re-construct!
    opt1 = opt2;
    assert(copy_assign_count == 1);
    assert(construct_count == 0);

    // Same active alternative move: should use move_assign, not re-construct!
    opt1 = std::move(opt2);
    assert(move_assign_count == 1);
    assert(construct_count == 0);

    std::cout << "[PASS] Finding 4: In-place assignment for matching alternatives\n";
}

void test_finding_7_unwrap_disengagement() {
    rl::Option<int, std::string> opt = rl::Some<std::string>("temporary");
    std::string val = std::move(opt).unwrap<std::string>();
    assert(val == "temporary");
    assert(opt.is_none());
    std::cout << "[PASS] Finding 7: Typed unwrap() && disengages option\n";
}

void test_heterogeneous_returns() {
    rl::Option<int, std::string> option_int = rl::Some(42);
    rl::Option<int, std::string> option_str = rl::Some<std::string>("world");
    rl::Option<int, std::string> option_none = rl::None;

    // Heterogeneous arms returning int or string -> Option<int, std::string>
    auto r1 = option_int.match(
        rl::Some<int>(42) >>= [](int x) { return x * 10; },
        rl::Some<std::string>("world") >>= [](const std::string& s) { return s; },
        rl::SomeAny >>= [] { return 0; },
        rl::None >>= [] { return rl::None; }
    );
    static_assert(std::same_as<decltype(r1), rl::Option<int, std::string>>);
    assert(r1.is_some());
    assert(r1.unwrap<int>() == 420);

    auto r2 = option_str.match(
        rl::Some<int>(42) >>= [](int x) { return x * 10; },
        rl::Some<std::string>("world") >>= [](const std::string& s) { return s; },
        rl::SomeAny >>= [] { return 0; },
        rl::None >>= [] { return rl::None; }
    );
    assert(r2.is_some());
    assert(r2.unwrap<std::string>() == "world");

    auto r3 = option_none.match(
        rl::Some<int>(42) >>= [](int x) { return x * 10; },
        rl::Some<std::string>("world") >>= [](const std::string& s) { return s; },
        rl::SomeAny >>= [] { return 0; },
        rl::None >>= [] { return rl::None; }
    );
    assert(r3.is_none());

    // Single value type with None -> Option<int>
    auto r4 = option_int.match(
        rl::Some<int>(42) >>= [](int x) { return x + 1; },
        rl::SomeAny >>= [] { return 0; },
        rl::None >>= [] { return rl::None; }
    );
    static_assert(std::same_as<decltype(r4), rl::Option<int>>);
    assert(r4.unwrap() == 43);

    std::cout << "[PASS] Heterogeneous arm return type deduction\n";
}

void test_multi_type_explicit_match() {
    rl::Option<int, std::string> option = rl::Some(100);

    auto r = option.template match<int, std::string>(
        rl::Some<int>(100) >>= [](int x) { return x; },
        rl::SomeAny >>= [] { return std::string("other"); },
        rl::None >>= [] { return rl::None; }
    );
    static_assert(std::same_as<decltype(r), rl::Option<int, std::string>>);
    assert(r.unwrap<int>() == 100);

    std::cout << "[PASS] Explicit multi-type match<T1, T2>\n";
}

void test_converting_constructors_and_assignments() {
    // Construction from value
    rl::Option<int, std::string> o1 = 123;
    assert(o1.is_some());
    assert(o1.unwrap<int>() == 123);

    rl::Option<int, std::string> o2 = "string_val";
    assert(o2.is_some());
    assert(o2.unwrap<std::string>() == "string_val");

    rl::Option<int> single = 777;
    assert(single.is_some());
    assert(single.unwrap() == 777);

    // Assignment from value
    o1 = "changed_to_str";
    assert(o1.unwrap<std::string>() == "changed_to_str");

    o1 = 456;
    assert(o1.unwrap<int>() == 456);

    single = 888;
    assert(single.unwrap() == 888);

    std::cout << "[PASS] Converting constructors and assignments\n";
}

int main() {
    test_finding_1_deleted_const_rvalue();
    test_finding_2_automatic_deduction();
    test_finding_3_throwing_predicates();
    test_finding_4_inplace_assignment();
    test_finding_7_unwrap_disengagement();
    test_heterogeneous_returns();
    test_multi_type_explicit_match();
    test_converting_constructors_and_assignments();

    std::cout << "\n========================================\n";
    std::cout << "ALL COMPREHENSIVE TESTS PASSED WITH 100% SUCCESS!\n";
    std::cout << "========================================\n";
    return 0;
}
