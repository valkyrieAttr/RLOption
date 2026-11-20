#include "rl_option.hpp"

#include <string>
#include <utility>

namespace {

template<class O>
concept CanIncompleteUnwrappedMatch = requires(O &option) {
    option.match(rl::True >>= [] { return 1; },
                           rl::None >>= [] { return 0; });
};

template<class O>
concept CanMixedUnwrappedMatch = requires(O &option) {
    option.match(rl::True >>= [] {},
                           rl::False >>= [] { return 1; },
                           rl::None >>= [] { return 2; });
};

template<class O>
concept CanUnwrapInvalidExplicitResult = requires(O &option) {
    option.template match<int>(
        rl::True >>= [] { return std::string{"invalid"}; },
        rl::False >>= [] { return std::string{"invalid"}; },
        rl::None >>= [] { return std::string{"invalid"}; });
};

template<class O>
concept CanCompleteUnwrappedMatch = requires(O &option) {
    option.match(rl::True >>= [] { return 1; },
                           rl::False >>= [] { return 2; },
                           rl::None >>= [] { return 0; });
};

static_assert(!CanIncompleteUnwrappedMatch<rl::Option<bool>>);
static_assert(!CanMixedUnwrappedMatch<rl::Option<bool>>);
static_assert(!CanUnwrapInvalidExplicitResult<rl::Option<bool>>);
static_assert(CanCompleteUnwrappedMatch<rl::Option<bool>>);

template<class O>
concept CanExplicitUnwrappedResult = requires(O &option) {
    option.template match<int>(
        rl::True >>= [] { return 1; },
        rl::False >>= [] { return 2; },
        rl::None >>= [] { return 0; });
};

template<class O>
concept CanDiscardMixedUnwrappedResults = requires(O &option) {
    option.template match<void>(
        rl::True >>= [] {},
        rl::False >>= [] { return 2; },
        rl::None >>= [] { return 0; });
};

static_assert(CanExplicitUnwrappedResult<rl::Option<bool>>);
static_assert(CanDiscardMixedUnwrappedResults<rl::Option<bool>>);

struct LvalueResult { int value; };
struct ConstLvalueResult { int value; };
struct RvalueResult { int value; };
struct ConstRvalueResult { int value; };
struct NoneResult { int value; };

struct ReceiverCategoryHandler {
    constexpr auto operator()(int &) const -> LvalueResult { return {1}; }
    constexpr auto operator()(const int &) const -> ConstLvalueResult {
        return {2};
    }
    constexpr auto operator()(int &&) const -> RvalueResult { return {3}; }
    constexpr auto operator()(const int &&) const -> ConstRvalueResult {
        return {4};
    }
};

consteval auto verify_unwrapped_dispatch() -> bool {
    rl::Option<bool> yes{rl::Some(true)};
    const auto result = yes.match(
        rl::True >>= [] constexpr { return 1; },
        rl::False >>= [] constexpr { return 2; },
        rl::None >>= [] constexpr { return 0; });
    return result.unwrap() == 1;
}

static_assert(verify_unwrapped_dispatch());

consteval auto verify_explicit_and_void_unwrapped() -> bool {
    rl::Option<bool> yes{rl::Some(true)};
    const auto explicit_result = yes.match<int>(
        rl::True >>= [] constexpr { return 4; },
        rl::False >>= [] constexpr { return 5; },
        rl::None >>= [] constexpr { return 6; });

    int calls = 0;
    yes.match<void>(
        rl::True >>= [&calls] constexpr { ++calls; },
        rl::False >>= [] constexpr { return 7; },
        rl::None >>= [] constexpr { return 8; });
    return explicit_result.unwrap() == 4 && calls == 1;
}

static_assert(verify_explicit_and_void_unwrapped());

consteval auto verify_unwrapped_receiver_categories() -> bool {
    rl::Option<int> lvalue{rl::Some(0)};
    const rl::Option<int> const_lvalue{rl::Some(0)};
    rl::Option<int> rvalue{rl::Some(0)};
    const rl::Option<int> const_rvalue{rl::Some(0)};
    const auto none_handler = [] constexpr { return NoneResult{0}; };

    const auto lvalue_result = lvalue.match(
        rl::SomeAny >>= ReceiverCategoryHandler{}, rl::None >>= none_handler);
    const auto const_lvalue_result = const_lvalue.match(
        rl::SomeAny >>= ReceiverCategoryHandler{}, rl::None >>= none_handler);
    const auto rvalue_result = std::move(rvalue).match(
        rl::SomeAny >>= ReceiverCategoryHandler{}, rl::None >>= none_handler);
    const auto const_rvalue_result = std::move(const_rvalue).match(
        rl::SomeAny >>= ReceiverCategoryHandler{}, rl::None >>= none_handler);

    return lvalue_result.unwrap<LvalueResult>().value == 1 &&
           const_lvalue_result.unwrap<ConstLvalueResult>().value == 2 &&
           rvalue_result.unwrap<RvalueResult>().value == 3 &&
           const_rvalue_result.unwrap<ConstRvalueResult>().value == 4;
}

static_assert(verify_unwrapped_receiver_categories());

} // namespace

int main() {}
