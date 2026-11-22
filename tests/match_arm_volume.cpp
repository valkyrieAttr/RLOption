#include <rl_option.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef RLOPTION_MATCH_ARM_COUNT
#define RLOPTION_MATCH_ARM_COUNT 256
#endif

namespace {

void require(bool condition_) {
    if (!condition_) {
        std::abort();
    }
}

template <std::size_t Alignment_, std::size_t Size_>
struct alignas(Alignment_) ResultHandler_ {
    std::array<std::byte, Size_> payload_{};
    std::array<std::byte, sizeof(int)> result_{};
    [[no_unique_address]] rl::details::PadToAlignment_<
        Size_ + sizeof(std::array<std::byte, sizeof(int)>), Alignment_>
        padding_{};

    constexpr void set_result(int value_) noexcept {
        result_ = std::bit_cast<decltype(result_)>(value_);
    }

    constexpr int operator()(int) const noexcept {
        return std::bit_cast<int>(result_);
    }
};

struct WildHandler_ {
    constexpr int operator()() const noexcept { return -1; }
    constexpr int operator()(int value_) const noexcept {
        return -100000 - value_;
    }
};

template <std::size_t Index_>
constexpr auto make_exact_arm_() {
    using Handler_ = ResultHandler_<(std::size_t{1} << (Index_ % 13)),
                                    1 + (Index_ % 31)>;
    static_assert(std::is_standard_layout_v<Handler_>);
    static_assert(sizeof(Handler_) == rl::details::align_up_(
                      1 + (Index_ % 31) + sizeof(std::array<std::byte, sizeof(int)>),
                      alignof(Handler_)));
    Handler_ handler_{};
    handler_.set_result(10000 + static_cast<int>(Index_));
    return rl::Some<int>(static_cast<int>(Index_)) >>= std::move(handler_);
}

template <std::size_t... Indices_>
void exercise_arm_pack_(std::index_sequence<Indices_...>,
                        std::size_t iterations_) {
    constexpr std::size_t arm_count_{sizeof...(Indices_)};
    rl::Option<int> value{rl::Some<int>(0)};
    auto arms_ = std::tuple{make_exact_arm_<Indices_>()...,
                            rl::Wild >>= WildHandler_{}};
    const auto dispatch_ = [&]() {
        return std::apply(
            [&value_ = value](auto &...arm_) { return value_.match(arm_...); },
            arms_);
    };

    for (std::size_t iteration_ = 0; iteration_ < iterations_; ++iteration_) {
        if (iteration_ % 257U == 256U) {
            value = rl::None;
            require(dispatch_() == -1);
            continue;
        }

        const auto current_ = static_cast<int>(iteration_ % (arm_count_ + 2));
        value = rl::Some<int>(current_);
        const auto result_ = dispatch_();

        if (current_ < static_cast<int>(arm_count_)) {
            require(result_ == 10000 + current_);
        } else {
            require(result_ == -100000 - current_);
        }
    }

    value = rl::None;
    require(dispatch_() == -1);
}

} // namespace

int main() {
    constexpr std::size_t arm_count_{RLOPTION_MATCH_ARM_COUNT};
    static_assert(arm_count_ > 0);
    constexpr std::size_t iterations_{500000};
    exercise_arm_pack_(std::make_index_sequence<arm_count_>{}, iterations_);
    std::cout << arm_count_ << " aligned arms x " << iterations_
              << " dispatches plus None passed\n";
}
