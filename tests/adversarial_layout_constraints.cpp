#include <rl_option.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

static_assert(rl::details::PaddingForAlignment_<1, 8>::value == 7);
static_assert(rl::details::PaddingForAlignment_<8, 8>::value == 0);
static_assert(rl::details::PaddingForAlignment_<65535, 65536>::value == 1);
static_assert(rl::details::PaddingForAlignment_<
                  std::numeric_limits<std::size_t>::max(),
                  2>::value == 1);
static_assert(rl::details::PaddingForAlignment_<
                  std::numeric_limits<std::size_t>::max() - 1,
                  2>::value == 0);
static_assert(rl::details::PaddingForAlignment_<
                  1, std::numeric_limits<std::size_t>::max()>::value ==
              std::numeric_limits<std::size_t>::max() - 1);
static_assert(rl::details::align_up_(
                  std::numeric_limits<std::size_t>::max(), 1) ==
              std::numeric_limits<std::size_t>::max());
static_assert(rl::details::align_up_(
                  std::numeric_limits<std::size_t>::max() - 8, 8) ==
              std::numeric_limits<std::size_t>::max() - 7);
static_assert(rl::details::checked_layout_add_(
                  std::numeric_limits<std::size_t>::max(), 0) ==
              std::numeric_limits<std::size_t>::max());
static_assert(rl::details::checked_layout_add_(
                  std::numeric_limits<std::size_t>::max() - 1, 1) ==
              std::numeric_limits<std::size_t>::max());

template <std::size_t Size_, std::size_t Alignment_>
concept AlignUpRepresentable_ = requires {
    std::integral_constant<std::size_t,
                           rl::details::align_up_(Size_, Alignment_)>{};
};

template <std::size_t Left_, std::size_t Right_>
concept LayoutSumRepresentable_ = requires {
    std::integral_constant<std::size_t,
                           rl::details::checked_layout_add_(Left_, Right_)>{};
};

static_assert(!AlignUpRepresentable_<
              std::numeric_limits<std::size_t>::max(), 8>);
static_assert(!AlignUpRepresentable_<1, 0>);
static_assert(!LayoutSumRepresentable_<
              std::numeric_limits<std::size_t>::max(), 1>);

constexpr std::size_t unsigned_char_max_{
    std::numeric_limits<unsigned char>::max()};
constexpr std::size_t unsigned_short_max_{
    std::numeric_limits<unsigned short>::max()};

static_assert(std::same_as<typename rl::details::option_index_type_<
                               unsigned_char_max_>::type,
                           unsigned char>);
static_assert(std::same_as<
              typename rl::details::option_index_type_<
                  unsigned_char_max_ + std::size_t{1}>::type,
              std::conditional_t<(unsigned_char_max_ < unsigned_short_max_),
                                 unsigned short, std::size_t>>);
static_assert(std::same_as<
              typename rl::details::option_index_type_<
                  unsigned_short_max_>::type,
              std::conditional_t<(unsigned_short_max_ <= unsigned_char_max_),
                                 unsigned char, unsigned short>>);
static_assert(std::same_as<typename rl::details::option_index_type_<
                               unsigned_short_max_ + std::size_t{1}>::type,
                           std::size_t>);

template <std::size_t Alignment_, std::size_t PayloadSize_>
struct alignas(Alignment_) BytePayload_ {
    std::array<std::byte, PayloadSize_> bytes_{};
    [[no_unique_address]] rl::details::PadToAlignment_<PayloadSize_, Alignment_>
        padding_{};
};

struct alignas(65536) HugeValue_ {
    int value_{};
    [[no_unique_address]] rl::details::PadToAlignment_<sizeof(int), 65536>
        padding_{};
};

template <typename Option_, typename Value_>
concept DirectlyConstructible_ = requires(Value_ &&value_) {
    Option_{std::forward<Value_>(value_)};
};

static_assert(sizeof(BytePayload_<65536, 13>) == 65536);
static_assert(alignof(BytePayload_<65536, 13>) == 65536);
static_assert(sizeof(BytePayload_<32768, 7>) == 32768);
static_assert(alignof(BytePayload_<32768, 7>) == 32768);
static_assert(sizeof(HugeValue_) == 65536);
static_assert(alignof(HugeValue_) == 65536);
static_assert(sizeof(rl::Option<HugeValue_>) % 65536 == 0);
static_assert(alignof(rl::Option<HugeValue_>) == 65536);
static_assert(!DirectlyConstructible_<rl::Option<int>, int>);

void require_(bool condition_) {
    if (!condition_) {
        std::abort();
    }
}

} // namespace

int main() {
    using WidePattern_ = BytePayload_<65536, 13>;
    using MidHandler_ = BytePayload_<32768, 7>;
    using FirstArm_ = rl::details::Arm_<WidePattern_, MidHandler_>;
    using NarrowPattern_ = BytePayload_<1, 5>;
    using WideHandler_ = BytePayload_<65536, 9>;
    using SecondArm_ = rl::details::Arm_<NarrowPattern_, WideHandler_>;

    static_assert(alignof(FirstArm_) == 65536);
    static_assert(sizeof(FirstArm_) == 131072);
    static_assert(alignof(SecondArm_) == 65536);
    static_assert(sizeof(SecondArm_) == 131072);

    std::vector<FirstArm_> first_arms_;
    std::vector<SecondArm_> second_arms_;
    for (std::size_t index_ = 0; index_ < 8; ++index_) {
        first_arms_.emplace_back(WidePattern_{}, MidHandler_{});
        second_arms_.emplace_back(NarrowPattern_{}, WideHandler_{});
    }

    for (const auto &arm_ : first_arms_) {
        const auto pattern_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.pattern));
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));
        require_(pattern_address_ % 65536 == 0);
        require_(handler_address_ % 32768 == 0);
        require_(handler_address_ - pattern_address_ == 65536);
    }

    for (const auto &arm_ : second_arms_) {
        const auto pattern_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.pattern));
        const auto handler_address_ = reinterpret_cast<std::uintptr_t>(
            std::addressof(arm_.handler));
        require_(pattern_address_ % alignof(NarrowPattern_) == 0);
        require_(handler_address_ % 65536 == 0);
        require_(handler_address_ - pattern_address_ == 65536);
    }

    std::vector<rl::Option<HugeValue_>> values_;
    for (int value_ = 0; value_ < 16; ++value_) {
        values_.emplace_back(rl::Some(HugeValue_{.value_ = value_}));
    }
    for (std::size_t index_ = 0; index_ < values_.size(); ++index_) {
        require_(values_[index_].is_some());
        require_(std::addressof(values_[index_].unwrap())
                     ->value_ == static_cast<int>(index_));
        require_(reinterpret_cast<std::uintptr_t>(
                     std::addressof(values_[index_].unwrap())) %
                 65536 == 0);
    }
}
