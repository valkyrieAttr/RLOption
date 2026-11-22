#include <rl_option.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

void require(bool condition_) {
    if (!condition_) {
        std::abort();
    }
}

template <std::size_t Alignment_, std::size_t Size_>
struct alignas(Alignment_) ByteBlock_ {
    std::array<std::byte, Size_> bytes{};
    [[no_unique_address]] rl::details::PadToAlignment_<Size_, Alignment_>
        padding{};
};

struct EmptyPattern_ {};

struct EmptyHandler_ {
    constexpr int operator()(int value_) const noexcept { return value_ + 7; }
};

template <std::size_t PatternAlignment_, std::size_t HandlerAlignment_>
void exercise_layout_pair_(std::size_t object_count_) {
    using Pattern_ = ByteBlock_<PatternAlignment_, 1 + PatternAlignment_ % 17>;
    using Handler_ = ByteBlock_<HandlerAlignment_, 1 + HandlerAlignment_ % 23>;
    using Arm_ = rl::details::Arm_<Pattern_, Handler_>;

    constexpr std::size_t handler_offset_{
        rl::details::align_up_(sizeof(Pattern_), alignof(Handler_))};
    static_assert(alignof(Arm_) ==
                  (alignof(Pattern_) > alignof(Handler_)
                       ? alignof(Pattern_)
                       : alignof(Handler_)));
    static_assert(sizeof(Arm_) % alignof(Arm_) == 0);
    static_assert(std::is_standard_layout_v<Arm_>);

    std::vector<Arm_> arms_;
    for (std::size_t index_ = 0; index_ < object_count_; ++index_) {
        Pattern_ pattern_{};
        Handler_ handler_{};
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        pattern_.bytes.front() = static_cast<std::byte>(marker_);
        pattern_.bytes.back() = static_cast<std::byte>(marker_ ^ 0x5aU);
        handler_.bytes.front() = static_cast<std::byte>(marker_ ^ 0xa5U);
        handler_.bytes.back() = static_cast<std::byte>(marker_ ^ 0xffU);
        arms_.emplace_back(std::move(pattern_), std::move(handler_));
    }

    for (std::size_t index_ = 0; index_ < arms_.size(); ++index_) {
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        const auto &arm_ = arms_[index_];
        const auto pattern_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.pattern));
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));

        require(pattern_address_ % alignof(Pattern_) == 0);
        require(handler_address_ % alignof(Handler_) == 0);
        require(handler_address_ - pattern_address_ == handler_offset_);
        require(arm_.pattern.bytes.front() == static_cast<std::byte>(marker_));
        require(arm_.pattern.bytes.back() ==
                static_cast<std::byte>(marker_ ^ 0x5aU));
        require(arm_.handler.bytes.front() ==
                static_cast<std::byte>(marker_ ^ 0xa5U));
        require(arm_.handler.bytes.back() ==
                static_cast<std::byte>(marker_ ^ 0xffU));
    }
}

template <std::size_t HandlerAlignment_>
void exercise_empty_pattern_(std::size_t object_count_) {
    using Handler_ = ByteBlock_<HandlerAlignment_, 1 + HandlerAlignment_ % 23>;
    using Arm_ = rl::details::Arm_<EmptyPattern_, Handler_>;
    static_assert(std::is_empty_v<EmptyPattern_>);

    std::vector<Arm_> arms_;
    for (std::size_t index_ = 0; index_ < object_count_; ++index_) {
        Handler_ handler_{};
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        handler_.bytes.front() = static_cast<std::byte>(marker_ ^ 0x3cU);
        handler_.bytes.back() = static_cast<std::byte>(marker_ ^ 0xc3U);
        arms_.emplace_back(EmptyPattern_{}, std::move(handler_));
    }

    for (std::size_t index_ = 0; index_ < arms_.size(); ++index_) {
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        const auto &arm_ = arms_[index_];
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));
        require(handler_address_ % alignof(Handler_) == 0);
        require(arm_.handler.bytes.front() ==
                static_cast<std::byte>(marker_ ^ 0x3cU));
        require(arm_.handler.bytes.back() ==
                static_cast<std::byte>(marker_ ^ 0xc3U));
    }
}

template <std::size_t PatternAlignment_>
void exercise_empty_handler_(std::size_t object_count_) {
    using Pattern_ = ByteBlock_<PatternAlignment_, 1 + PatternAlignment_ % 17>;
    using Arm_ = rl::details::Arm_<Pattern_, EmptyHandler_>;
    static_assert(std::is_empty_v<EmptyHandler_>);

    std::vector<Arm_> arms_;
    for (std::size_t index_ = 0; index_ < object_count_; ++index_) {
        Pattern_ pattern_{};
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        pattern_.bytes.front() = static_cast<std::byte>(marker_ ^ 0x69U);
        pattern_.bytes.back() = static_cast<std::byte>(marker_ ^ 0x96U);
        arms_.emplace_back(std::move(pattern_), EmptyHandler_{});
    }

    for (std::size_t index_ = 0; index_ < arms_.size(); ++index_) {
        const auto marker_ = static_cast<unsigned char>(index_ & 0xffU);
        const auto &arm_ = arms_[index_];
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));
        require(handler_address_ % alignof(EmptyHandler_) == 0);
        require(arm_.handler(static_cast<int>(index_)) ==
                static_cast<int>(index_ + 7));
        require(arm_.pattern.bytes.front() ==
                static_cast<std::byte>(marker_ ^ 0x69U));
        require(arm_.pattern.bytes.back() ==
                static_cast<std::byte>(marker_ ^ 0x96U));
    }
}

void exercise_empty_empty_(std::size_t object_count_) {
    using Arm_ = rl::details::Arm_<EmptyPattern_, EmptyHandler_>;
    static_assert(std::is_empty_v<EmptyPattern_>);
    static_assert(std::is_empty_v<EmptyHandler_>);

    std::vector<Arm_> arms_;
    for (std::size_t index_ = 0; index_ < object_count_; ++index_) {
        arms_.emplace_back(EmptyPattern_{}, EmptyHandler_{});
    }

    for (std::size_t index_ = 0; index_ < arms_.size(); ++index_) {
        const auto &arm_ = arms_[index_];
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));
        require(handler_address_ % alignof(EmptyHandler_) == 0);
        require(arm_.handler(static_cast<int>(index_)) ==
                static_cast<int>(index_ + 7));
    }
}

template <std::size_t PatternAlignment_, std::size_t... HandlerExponents_>
void exercise_handler_row_(std::index_sequence<HandlerExponents_...>,
                           std::size_t object_count_) {
    (exercise_layout_pair_<PatternAlignment_,
                           (std::size_t{1} << HandlerExponents_)>(
         object_count_), ...);
}

template <std::size_t... PatternExponents_, std::size_t... HandlerExponents_>
void exercise_alignment_matrix_(std::index_sequence<PatternExponents_...>,
                                std::index_sequence<HandlerExponents_...>,
                                std::size_t object_count_) {
    (exercise_handler_row_<(std::size_t{1} << PatternExponents_)>(
         std::index_sequence<HandlerExponents_...>{}, object_count_), ...);
}

template <std::size_t... Exponents_>
void exercise_empty_pattern_cases_(std::index_sequence<Exponents_...>,
                                   std::size_t object_count_) {
    (exercise_empty_pattern_<(std::size_t{1} << Exponents_)>(object_count_),
     ...);
}

template <std::size_t... Exponents_>
void exercise_empty_handler_cases_(std::index_sequence<Exponents_...>,
                                   std::size_t object_count_) {
    (exercise_empty_handler_<(std::size_t{1} << Exponents_)>(object_count_),
     ...);
}

} // namespace

int main() {
    constexpr std::size_t alignment_levels_{13};
    constexpr std::size_t object_count_{512};
    constexpr std::size_t empty_case_count_{128};

    exercise_alignment_matrix_(std::make_index_sequence<alignment_levels_>{},
                               std::make_index_sequence<alignment_levels_>{},
                               object_count_);
    exercise_empty_pattern_cases_(std::make_index_sequence<alignment_levels_>{},
                                  empty_case_count_);
    exercise_empty_handler_cases_(std::make_index_sequence<alignment_levels_>{},
                                  empty_case_count_);
    exercise_empty_empty_(empty_case_count_);

    constexpr std::size_t alignment_pairs_{
        alignment_levels_ * alignment_levels_};
    std::cout << alignment_pairs_ << " mixed alignment pairs x "
              << object_count_ << " arms; " << 2 * alignment_levels_
              << " empty/nonempty pairs x " << empty_case_count_
              << " arms plus " << empty_case_count_
              << " empty/empty pairs passed\n";
}
