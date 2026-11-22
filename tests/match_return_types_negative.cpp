#include "rl_option.hpp"

#include <memory>
#include <string>
namespace {
template<class O>
concept CanMatchMixedVoidAndValue = requires(O& option) {
    option.match(rl::True >>= [] {}, rl::False >>= [] { return 1; },
                 rl::None >>= [] { return 2; });
};
template<class O>
concept CanMatchExplicitNonVoidMixed = requires(O& option) {
    option.template match<int>(rl::True >>= [] {},
                              rl::False >>= [] { return 1; },
                              rl::None >>= [] { return 2; });
};
template<class O>
concept CanMatchExplicitAlternativesMixed = requires(O& option) {
    option.template match<int, long>(rl::True >>= [] {},
                                     rl::False >>= [] { return 1; },
                                     rl::None >>= [] { return 2; });
};
template<class O>
concept CanMatchWithoutArms = requires(O& option) { option.match(); };
template<class O>
concept CanMatchUnwrappableExplicitResult = requires(O& option) {
    option.template match<int>(
        rl::True >>= [] { return std::string{"invalid"}; },
        rl::False >>= [] { return std::string{"invalid"}; },
        rl::None >>= [] { return std::string{"invalid"}; });
};
template<class O>
concept CanMatchUnwrappableExplicitAlternatives = requires(O& option) {
    option.template match<int, long>(
        rl::True >>= [] { return std::string{"invalid"}; },
        rl::False >>= [] { return std::string{"invalid"}; },
        rl::None >>= [] { return std::string{"invalid"}; });
};
static_assert(!CanMatchMixedVoidAndValue<rl::Option<bool>>);
static_assert(!CanMatchExplicitNonVoidMixed<rl::Option<bool>>);
static_assert(!CanMatchExplicitAlternativesMixed<rl::Option<bool>>);
static_assert(!CanMatchWithoutArms<rl::Option<bool>>);
static_assert(!CanMatchUnwrappableExplicitResult<rl::Option<bool>>);
static_assert(!CanMatchUnwrappableExplicitAlternatives<rl::Option<bool>>);
struct NonCopyableReferenceResult {
    std::unique_ptr<int> value{std::make_unique<int>(1)};
    std::unique_ptr<int> &operator()(bool) { return value; }
};
template<class O, class H>
concept CanInferUnmaterializableResult = requires(O &option, H &handler) {
    option.match(rl::True >>= handler, rl::False >>= handler,
                 rl::None >>= [] { return 0; });
};
static_assert(!CanInferUnmaterializableResult<rl::Option<bool>,
                                               NonCopyableReferenceResult>);

}
int main() {
    NonCopyableReferenceResult handler;
    (void)handler(true);
}
