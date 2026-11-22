#include <rl_option.hpp>

#include <tuple>
#include <utility>

namespace {

struct RuntimeOnlyFallback_ {
    auto operator()(int value_) const noexcept -> int { return -value_; }
};

consteval auto evaluate_constant_match_() -> bool {
    int calls_{};
    rl::Option<int, double> value_{rl::Some<double>(3.5)};

    const auto result_ = value_.match(
        rl::Some<int>{-1} >>= [&](int) constexpr {
            ++calls_;
            return -1;
        },
        rl::Some<double>{-4.5} >>= [&](double) constexpr {
            calls_ += 10000;
            return -2;
        },
        rl::Some<double>{3.5} >>= [&](double current_) constexpr {
            ++calls_;
            return static_cast<int>(current_ * 2.0);
        },
        rl::SomeAny >>= [&](auto &&) constexpr {
            calls_ += 100;
            return 0;
        },
        rl::None >>= [&] constexpr {
            calls_ += 1000;
            return 0;
        });

    return result_.is_some() && result_.unwrap() == 7 && calls_ == 1;
}

static_assert(evaluate_constant_match_());

consteval auto evaluate_reused_lvalue_arms_() -> bool {
    int handler_calls_{};
    const auto exact_handler_ = [&handler_calls_](int value_) constexpr {
        ++handler_calls_;
        return value_ * 2;
    };
    const auto fallback_handler_ = [&handler_calls_](int value_) constexpr {
        ++handler_calls_;
        return -value_;
    };
    const auto none_handler_ = [&handler_calls_] constexpr {
        ++handler_calls_;
        return -1;
    };
    auto arms_ = std::tuple{
        rl::Some<int>{7} >>= exact_handler_,
        rl::SomeAny >>= fallback_handler_,
        rl::None >>= none_handler_};

    rl::Option<int> exact_{rl::Some(7)};
    rl::Option<int> fallback_{rl::Some(9)};
    rl::Option<int> empty_{rl::None};
    const auto invoke_ = [&arms_](auto &option_) constexpr {
        return std::apply(
            [&option_](auto &...arm_) constexpr {
                return option_.match(arm_...);
            },
            arms_);
    };

    const auto exact_result_ = invoke_(exact_);
    const auto fallback_result_ = invoke_(fallback_);
    const auto none_result_ = invoke_(empty_);
    return exact_result_.unwrap() == 14 &&
           fallback_result_.unwrap() == -9 &&
           none_result_.unwrap() == -1 && handler_calls_ == 3;
}

static_assert(evaluate_reused_lvalue_arms_());

consteval auto evaluate_without_running_runtime_only_fallback_() -> bool {
    rl::Option<int> value_{rl::Some(6)};
    const auto result_ = value_.match(
        rl::Some<int>{6} >>= [](int current_) constexpr {
            return current_ * 3;
        },
        rl::SomeAny >>= RuntimeOnlyFallback_{},
        rl::None >>= [] constexpr { return -1; });
    return result_.unwrap() == 18;
}

static_assert(evaluate_without_running_runtime_only_fallback_());

} // namespace

int main() {
    rl::Option<int, double> value_{rl::Some<double>(4.5)};
    const auto result_ = value_.match(
        rl::Some<int>{-1} >>= [](int) constexpr { return -1; },
        rl::Some<double>{-4.5} >>= [](double) constexpr { return -2; },
        rl::Some<double>{4.5} >>= [](double current_) constexpr {
            return static_cast<int>(current_ * 2.0);
        },
        rl::SomeAny >>= [](auto &&) constexpr { return 0; },
        rl::None >>= [] constexpr { return 0; });

    return result_.is_some() && result_.unwrap() == 9 ? 0 : 1;
}
