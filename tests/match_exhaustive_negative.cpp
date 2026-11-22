#include "rl_option.hpp"

namespace {

template <class O>
concept CanMatchTrueOnly = requires(O &option) {
    option.match(rl::True >>= [](bool) { return 1; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchFalseOnly = requires(O &option) {
    option.match(rl::False >>= [](bool) { return 1; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchTrueOnInteger = requires(O &option) {
    option.match(rl::True >>= [](bool) { return 1; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchFalseOnInteger = requires(O &option) {
    option.match(rl::False >>= [](bool) { return 1; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchBoolWithoutNone = requires(O &option) {
    option.match(rl::True >>= [](bool) { return 1; },
                 rl::False >>= [](bool) { return 0; });
};

template <class O>
concept CanMatchOnlyPrimaryAlternative = requires(O &option) {
    option.match(rl::Some<int>{1} >>= [](int value) { return value; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchPartialValueOnly = requires(O &option) {
    option.match(rl::Some<int>{1} >>= [](int value) { return value; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchDefaultOnly = requires(O &option) {
    option.match(rl::Default >>= [](int value) { return value; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept CanMatchPredicateOnly = requires(O &option) {
    option.match([](const int &value) { return value > 10; } >>=
                     [](int value) { return value; },
                 rl::None >>= [] { return 0; });
};

template <class O>
concept HasPublicValueType = requires { typename O::value_type; };

template <class O> concept HasPublicSome = requires { &O::Some; };

template <class O> concept HasPublicWild = requires { &O::Wild; };

template <class O> concept HasPublicNone = requires { &O::None; };

template <class O> concept HasPublicNoneFunction = requires { &O::none; };

template <class O> concept HasPublicSomeAny = requires { &O::SomeAny; };

static_assert(!CanMatchTrueOnly<rl::Option<bool>>);
static_assert(!CanMatchFalseOnly<rl::Option<bool>>);
static_assert(!CanMatchTrueOnInteger<rl::Option<int>>);
static_assert(!CanMatchFalseOnInteger<rl::Option<int>>);
static_assert(!CanMatchBoolWithoutNone<rl::Option<bool>>);
static_assert(!CanMatchOnlyPrimaryAlternative<rl::Option<int, double>>);
static_assert(!CanMatchPartialValueOnly<rl::Option<int>>);
static_assert(!CanMatchDefaultOnly<rl::Option<int>>);
static_assert(!CanMatchPredicateOnly<rl::Option<int>>);
static_assert(!HasPublicValueType<rl::Option<int>>);
static_assert(!HasPublicSome<rl::Option<int>>);
static_assert(!HasPublicWild<rl::Option<int>>);
static_assert(!HasPublicNone<rl::Option<int>>);
static_assert(!HasPublicNoneFunction<rl::Option<int>>);
static_assert(!HasPublicSomeAny<rl::Option<int>>);

} // namespace

int main() {}
