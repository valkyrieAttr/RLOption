#include "rl_option.hpp"

namespace {

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

consteval auto verify_match_receiver_categories() -> bool {
    rl::Option<int> lvalue{rl::Some(0)};
    const rl::Option<int> const_lvalue{rl::Some(0)};
    rl::Option<int> rvalue{rl::Some(0)};
    const rl::Option<int> const_rvalue{rl::Some(0)};
    const auto none_handler = [] constexpr { return NoneResult{0}; };

    const auto lvalue_result =
        lvalue.match(rl::SomeAny >>= ReceiverCategoryHandler{},
                     rl::None >>= none_handler);
    const auto const_lvalue_result =
        const_lvalue.match(rl::SomeAny >>= ReceiverCategoryHandler{},
                           rl::None >>= none_handler);
    const auto rvalue_result =
        std::move(rvalue).match(rl::SomeAny >>= ReceiverCategoryHandler{},
                                rl::None >>= none_handler);
    const auto const_rvalue_result =
        std::move(const_rvalue).match(rl::SomeAny >>= ReceiverCategoryHandler{},
                                      rl::None >>= none_handler);

    return lvalue_result.unwrap<LvalueResult>().value == 1 &&
           const_lvalue_result.unwrap<ConstLvalueResult>().value == 2 &&
           rvalue_result.unwrap<RvalueResult>().value == 3 &&
           const_rvalue_result.unwrap<ConstRvalueResult>().value == 4;
}

static_assert(verify_match_receiver_categories());

} // namespace

int main() {}
