#include <rl_option.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

void require_(bool condition_) {
    if (!condition_) {
        std::abort();
    }
}

template <std::size_t Exponent_, std::size_t PayloadSize_, std::size_t Tag_>
struct alignas((std::size_t{1} << Exponent_)) LayoutValue_ {
    static constexpr std::size_t alignment_{std::size_t{1} << Exponent_};

    std::array<std::byte, PayloadSize_> bytes_{};
    [[no_unique_address]] rl::details::PadToAlignment_<PayloadSize_, alignment_>
        padding_{};

    static_assert(Tag_ < 2);
};

template <std::size_t FirstExponent_, std::size_t SecondExponent_>
void verify_option_pair_() {
    constexpr std::size_t first_size_{1 + (FirstExponent_ * 5 +
                                            SecondExponent_ * 3) % 17};
    constexpr std::size_t second_size_{1 + (SecondExponent_ * 7 +
                                             FirstExponent_ * 2) % 19};
    using First_ =
        LayoutValue_<FirstExponent_, first_size_, 0>;
    using Second_ =
        LayoutValue_<SecondExponent_, second_size_, 1>;
    using Option_ = rl::Option<First_, Second_>;
    using Storage_ = rl::details::OptionStorage_<First_, Second_>;

    static_assert(std::is_standard_layout_v<First_>);
    static_assert(std::is_standard_layout_v<Second_>);
    static_assert(sizeof(First_) ==
                  rl::details::align_up_(first_size_, alignof(First_)));
    static_assert(sizeof(Second_) ==
                  rl::details::align_up_(second_size_, alignof(Second_)));
    static_assert(rl::details::option_storage_layout_valid_<First_, Second_>());
    static_assert(sizeof(Option_) == sizeof(Storage_));
    static_assert(alignof(Option_) == alignof(Storage_));
    static_assert(alignof(Option_) >= alignof(First_));
    static_assert(alignof(Option_) >= alignof(Second_));
    static_assert(std::is_standard_layout_v<Storage_>);

    std::vector<Option_> values_;
    values_.reserve(4);
    for (std::size_t index_ = 0; index_ < 4; ++index_) {
        First_ first_{};
        first_.bytes_.front() = static_cast<std::byte>(index_ + 1U);
        values_.emplace_back(rl::Some<First_>(std::move(first_)));

        Second_ second_{};
        second_.bytes_.back() = static_cast<std::byte>(index_ + 5U);
        values_.emplace_back(rl::Some<Second_>(std::move(second_)));
    }

    for (std::size_t index_ = 0; index_ < values_.size(); ++index_) {
        const auto &option_ = values_[index_];
        require_(std::is_sufficiently_aligned<alignof(Option_)>(
            std::addressof(option_)));
        require_(reinterpret_cast<std::uintptr_t>(std::addressof(option_)) %
                     alignof(Option_) ==
                 0);

        if (index_ % 2U == 0) {
            const auto &value_ = option_.template unwrap<First_>();
            require_(std::is_sufficiently_aligned<alignof(First_)>(
                std::addressof(value_)));
            require_(reinterpret_cast<std::uintptr_t>(
                         std::addressof(value_)) %
                         alignof(First_) ==
                     0);
            require_(value_.bytes_.front() ==
                     static_cast<std::byte>(index_ / 2U + 1U));
        } else {
            const auto &value_ = option_.template unwrap<Second_>();
            require_(std::is_sufficiently_aligned<alignof(Second_)>(
                std::addressof(value_)));
            require_(reinterpret_cast<std::uintptr_t>(
                         std::addressof(value_)) %
                         alignof(Second_) ==
                     0);
            require_(value_.bytes_.back() ==
                     static_cast<std::byte>(index_ / 2U + 5U));
        }
    }
}

template <std::size_t FirstExponent_, std::size_t... SecondExponents_>
void verify_option_row_(std::index_sequence<SecondExponents_...>) {
    (verify_option_pair_<FirstExponent_, SecondExponents_>(), ...);
}

template <std::size_t... FirstExponents_, std::size_t... SecondExponents_>
void verify_option_matrix_(std::index_sequence<FirstExponents_...>,
                            std::index_sequence<SecondExponents_...>) {
    (verify_option_row_<FirstExponents_>(
         std::index_sequence<SecondExponents_...>{}),
     ...);
}

} // namespace

int main() {
    using AlignmentExponents_ = std::index_sequence<0, 1, 2, 3, 4, 5, 8, 12>;
    verify_option_matrix_(AlignmentExponents_{}, AlignmentExponents_{});
}
