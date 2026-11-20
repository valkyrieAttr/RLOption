#include "rl_option.hpp"

namespace {

template <class Option_>
concept ExposesValueType = requires { typename Option_::value_type; };

template <class Option_>
concept ExposesSome = requires { Option_::Some; } ||
                      requires { typename Option_::Some; };

template <class Option_>
concept ExposesWild = requires { Option_::Wild; };

template <class Option_>
concept ExposesLowercaseWild = requires { Option_::wild; };

template <class Option_>
concept ExposesNone = requires { Option_::None; };

template <class Option_>
concept ExposesLowercaseNone = requires { Option_::none; };

template <class Option_>
concept ExposesSomeAny = requires { Option_::SomeAny; };

using IntOption = rl::Option<int>;

static_assert(!ExposesValueType<IntOption>);
static_assert(!ExposesSome<IntOption>);
static_assert(!ExposesWild<IntOption>);
static_assert(!ExposesLowercaseWild<IntOption>);
static_assert(!ExposesNone<IntOption>);
static_assert(!ExposesLowercaseNone<IntOption>);
static_assert(!ExposesSomeAny<IntOption>);

static_assert(requires {
    rl::Some(1);
    rl::Wild;
    rl::None;
    rl::SomeAny;
});

} // namespace

int main() {}
