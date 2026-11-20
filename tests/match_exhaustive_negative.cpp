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
concept CanMatchWherePredicateOnly = requires(O &option) {
    option.match(rl::Where([](const int &value) { return value > 10; }) >>=
                     [](int value) { return value; },
                 rl::None >>= [] { return 0; });
};

static_assert(!CanMatchTrueOnly<rl::Option<bool>>);
static_assert(!CanMatchFalseOnly<rl::Option<bool>>);
static_assert(!CanMatchTrueOnInteger<rl::Option<int>>);
static_assert(!CanMatchFalseOnInteger<rl::Option<int>>);
static_assert(!CanMatchBoolWithoutNone<rl::Option<bool>>);
static_assert(!CanMatchOnlyPrimaryAlternative<rl::Option<int, double>>);
static_assert(!CanMatchPartialValueOnly<rl::Option<int>>);
static_assert(!CanMatchDefaultOnly<rl::Option<int>>);
static_assert(!CanMatchPredicateOnly<rl::Option<int>>);
static_assert(!CanMatchWherePredicateOnly<rl::Option<int>>);

} // namespace

int main() {}
