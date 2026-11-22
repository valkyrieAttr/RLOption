#include <rl_option.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

void require(bool condition_) {
    if (!condition_) {
        std::abort();
    }
}

struct alignas(4096) OverAlignedValue_ {
    int value_{};
    [[no_unique_address]] rl::details::PadToAlignment_<sizeof(value_), 4096>
        padding_{};

    friend constexpr bool operator==(const OverAlignedValue_ &lhs_,
                                     const OverAlignedValue_ &rhs_) {
        return lhs_.value_ == rhs_.value_;
    }
};

struct ValueFallback_ {
    constexpr int operator()(const OverAlignedValue_ &value_) const noexcept {
        return 1000 + value_.value_;
    }
    constexpr int operator()() const noexcept { return -1; }
};

struct alignas(4096) OverAlignedIntHandler_ {
    int bias_{5};
    [[no_unique_address]] rl::details::PadToAlignment_<sizeof(bias_), 4096>
        padding_{};

    constexpr int operator()(int value_) const noexcept {
        return value_ + bias_;
    }
};

struct IntFallback_ {
    constexpr int operator()(int value_) const noexcept {
        return -100000 - value_;
    }
    constexpr int operator()() const noexcept { return -2; }
};

template <class Tp_>
std::uintptr_t address_(Tp_ *pointer_) {
    return reinterpret_cast<std::uintptr_t>(pointer_);
}

} // namespace

int main() {
    constexpr auto value_handler_ =
        [](const OverAlignedValue_ &value_) { return value_.value_; };

    const auto pattern_arm_ =
        rl::Some<OverAlignedValue_>(OverAlignedValue_{.value_ = 17}) >>=
        value_handler_;
    using PatternArm_ = decltype(pattern_arm_);
    require(address_(std::addressof(pattern_arm_.pattern)) %
                alignof(typename PatternArm_::pattern_type) ==
            0);
    require(address_(std::addressof(pattern_arm_.handler)) %
                alignof(typename PatternArm_::handler_type) ==
            0);
    require(sizeof(PatternArm_) % alignof(PatternArm_) == 0);

    const auto handler_arm_ =
        rl::Some<int>(17) >>= OverAlignedIntHandler_{};
    using HandlerArm_ = decltype(handler_arm_);
    require(address_(std::addressof(handler_arm_.pattern)) %
                alignof(typename HandlerArm_::pattern_type) ==
            0);
    require(address_(std::addressof(handler_arm_.handler)) %
                alignof(typename HandlerArm_::handler_type) ==
            0);
    require(sizeof(HandlerArm_) % alignof(HandlerArm_) == 0);

    std::cout << "Extreme Arm sizes: over-aligned pattern = "
              << sizeof(PatternArm_) << " bytes; over-aligned handler = "
              << sizeof(HandlerArm_) << " bytes\n";

    rl::Option<OverAlignedValue_> value{
        rl::Some<OverAlignedValue_>(OverAlignedValue_{.value_ = 0})};
    for (int iteration_ = 0; iteration_ < 4096; ++iteration_) {
        value = rl::Some<OverAlignedValue_>(
            OverAlignedValue_{.value_ = iteration_});
        const auto exact_ = value.match(
            rl::Some<OverAlignedValue_>(
                OverAlignedValue_{.value_ = iteration_}) >>= value_handler_,
            rl::Wild >>= ValueFallback_{});
        require(exact_ == iteration_);

        value = rl::Some<OverAlignedValue_>(
            OverAlignedValue_{.value_ = iteration_ + 1});
        const auto fallback_ = value.match(
            rl::Some<OverAlignedValue_>(
                OverAlignedValue_{.value_ = iteration_}) >>= value_handler_,
            rl::Wild >>= ValueFallback_{});
        require(fallback_ == 1001 + iteration_);

        if (iteration_ % 257 == 256) {
            value = rl::None;
            require(value.match(
                        rl::Some<OverAlignedValue_>(
                            OverAlignedValue_{.value_ = iteration_}) >>=
                            value_handler_,
                        rl::Wild >>= ValueFallback_{}) == -1);
        }
    }

    rl::Option<int> integer{rl::Some<int>(0)};
    for (int iteration_ = 0; iteration_ < 8192; ++iteration_) {
        const auto matches_ = iteration_ % 2 == 0;
        const auto pattern_value_ = matches_ ? iteration_ : iteration_ + 1;
        integer = rl::Some<int>(iteration_);
        const auto result_ = integer.match(
            rl::Some<int>(pattern_value_) >>= OverAlignedIntHandler_{},
            rl::Wild >>= IntFallback_{});
        require(result_ == (matches_ ? iteration_ + 5
                                     : -100000 - iteration_));

        if (iteration_ % 257 == 256) {
            integer = rl::None;
            require(integer.match(
                        rl::Some<int>(pattern_value_) >>=
                            OverAlignedIntHandler_{},
                        rl::Wild >>= IntFallback_{}) == -2);
        }
    }

    std::cout << "4096-aligned patterns/handlers passed 12288 primary match "
                 "cases plus repeated None cases\n";
}
