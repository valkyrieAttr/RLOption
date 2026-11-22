// stress_00001.cpp — Wave 1: Core construction, is_some/is_none, unwrap, expect, value
#include "rl_option.hpp"
#include <cassert>
#include <print>
#include <type_traits>
#include <string>

// rl::None is a constexpr variable of type rl::None_t
// rl::Some<T>(val) constructs a Some<T>
// rl::Option<T> is the type

// --- constexpr construction ---
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(42);
    return opt.is_some() && !opt.is_none();
}());

static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return opt.is_none() && !opt.is_some();
}());

static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(99);
    return opt.unwrap() == 99;
}());

// value() alias
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(7);
    return opt.value() == 7;
}());

// expect()
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    return opt.expect("should not fail") == 3;
}());

// is_some<T>() (typed check)
static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<int>(5);
    return opt.is_some<int>() && !opt.is_some<double>();
}());

static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(3.14);
    return !opt.is_some<int>() && opt.is_some<double>();
}());

// None is_some<T>() must be false
static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    return !opt.is_some<int>();
}());

// explicit operator bool
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(1);
    rl::Option<int> b = rl::None;
    return static_cast<bool>(a) && !static_cast<bool>(b);
}());

// unwrap_or
static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.unwrap_or(42) == 42;
}());

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(10);
    return some.unwrap_or(42) == 10;
}());

// unwrap_or_default
static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return none.unwrap_or_default() == 0;
}());

static_assert([]() consteval {
    rl::Option<int> some = rl::Some<int>(5);
    return some.unwrap_or_default() == 5;
}());

// unwrap_unchecked (copy version)
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(123);
    return opt.unwrap_unchecked() == 123;
}());

// Copy construction
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(55);
    rl::Option<int> b = a;
    return b.unwrap() == 55 && a.unwrap() == 55;
}());

// Move construction
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(77);
    rl::Option<int> b = std::move(a);
    return b.unwrap() == 77;
}());

// Assignment from None
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(3);
    opt = rl::None;
    return opt.is_none();
}());

// Assignment from Some
static_assert([]() consteval {
    rl::Option<int> opt = rl::None;
    opt = rl::Some<int>(8);
    return opt.unwrap() == 8;
}());

// const ref unwrap
static_assert([]() consteval {
    const rl::Option<int> opt = rl::Some<int>(17);
    return opt.unwrap() == 17;
}());

// lvalue & rvalue unwrap value category
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    static_assert(std::is_same_v<decltype(opt.unwrap()), int&>);
    static_assert(std::is_same_v<decltype(std::as_const(opt).unwrap()), const int&>);
    static_assert(std::is_same_v<decltype(std::move(opt).unwrap()), int>);
    return true;
}());

// value() categories
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(5);
    static_assert(std::is_same_v<decltype(opt.value()), int&>);
    static_assert(std::is_same_v<decltype(std::as_const(opt).value()), const int&>);
    // operator*
    static_assert(std::is_same_v<decltype(*opt), int&>);
    static_assert(std::is_same_v<decltype(*std::as_const(opt)), const int&>);
    return true;
}());

// equality operators
static_assert([]() consteval {
    rl::Option<int> a = rl::Some<int>(3);
    rl::Option<int> b = rl::Some<int>(3);
    rl::Option<int> c = rl::Some<int>(4);
    rl::Option<int> d = rl::None;
    return (a == b) && !(a == c) && !(a == d) && (d == rl::Option<int>(rl::None));
}());

// spaceship
static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    rl::Option<int> some3 = rl::Some<int>(3);
    rl::Option<int> some5 = rl::Some<int>(5);
    return (none <=> some3) == std::strong_ordering::less &&
           (some5 <=> some3) == std::strong_ordering::greater &&
           (some3 <=> rl::Option<int>(rl::Some<int>(3))) == std::strong_ordering::equal;
}());

// None equality
static_assert([]() consteval {
    rl::Option<int> none = rl::None;
    return (none == rl::None) && !(none == rl::Some<int>(0));
}());

// operator== with raw value
static_assert([]() consteval {
    rl::Option<int> opt = rl::Some<int>(42);
    return (opt == 42) && !(opt == 43);
}());

// typed unwrap<T>
static_assert([]() consteval {
    rl::Option<int, double> opt = rl::Some<double>(9.9);
    return opt.unwrap<double>() > 9.89 && opt.unwrap<double>() < 9.91;
}());

// is_default
static_assert([]() consteval {
    rl::Option<int> def = rl::Some<int>(0);
    rl::Option<int> non_def = rl::Some<int>(1);
    rl::Option<int> none = rl::None;
    return def.is_default() && !non_def.is_default() && !none.is_default();
}());

// --- Runtime tests ---
int main() {
    // Test that unwrap on None panics (abort) - only test the success path at runtime
    {
        rl::Option<int> opt = rl::Some<int>(42);
        assert(opt.is_some());
        assert(!opt.is_none());
        assert(opt.unwrap() == 42);
        assert(opt.value() == 42);
        assert(opt.expect("msg") == 42);
        assert(*opt == 42);
    }
    {
        rl::Option<int> opt = rl::None;
        assert(!opt.is_some());
        assert(opt.is_none());
        assert(opt.unwrap_or(99) == 99);
        assert(opt.unwrap_or_default() == 0);
    }
    // Multi-alternative is_some<T>
    {
        rl::Option<int, double> opt = rl::Some<double>(2.5);
        assert(opt.is_some());
        assert(!opt.is_some<int>());
        assert(opt.is_some<double>());
    }
    // String (non-trivial) construction & copy
    {
        rl::Option<std::string> opt = rl::Some<std::string>(std::string("hello"));
        assert(opt.is_some());
        assert(opt.unwrap() == "hello");
        rl::Option<std::string> opt2 = opt;
        assert(opt2.unwrap() == "hello");
    }
    // Move string
    {
        rl::Option<std::string> opt = rl::Some<std::string>(std::string("world"));
        rl::Option<std::string> opt2 = std::move(opt);
        assert(opt2.unwrap() == "world");
    }
    // Self-assignment guard
    {
        rl::Option<int> opt = rl::Some<int>(7);
        auto &self_assignment_source = opt;
        opt = self_assignment_source;
        assert(opt.unwrap() == 7);
    }
    // is_some<T> on multi-alt
    {
        rl::Option<int, double> opt_i = rl::Some<int>(5);
        rl::Option<int, double> opt_d = rl::Some<double>(3.14);
        assert(opt_i.is_some<int>() && !opt_i.is_some<double>());
        assert(!opt_d.is_some<int>() && opt_d.is_some<double>());
    }
    // typed unwrap<T> on multi-alt
    {
        rl::Option<int, double> opt = rl::Some<double>(9.9);
        assert(opt.unwrap<double>() > 9.89 && opt.unwrap<double>() < 9.91);
    }
    // copy assign Some<T> to Option
    {
        rl::Option<int> opt = rl::None;
        opt = rl::Some<int>(55);
        assert(opt.unwrap() == 55);
        opt = rl::None;
        assert(opt.is_none());
    }
    // is_default
    {
        rl::Option<int> def = rl::Some<int>(0);
        assert(def.is_default());
        rl::Option<int> non_def = rl::Some<int>(5);
        assert(!non_def.is_default());
    }
    // operator== with raw value
    {
        rl::Option<int> opt = rl::Some<int>(10);
        assert(opt == 10);
        assert(!(opt == 11));
    }
    // operator== with Some<T>
    {
        rl::Option<int> opt = rl::Some<int>(3);
        assert(opt == rl::Some<int>(3));
        assert(!(opt == rl::Some<int>(4)));
    }
    std::println("Wave 1: all tests PASSED");
    return 0;
}
