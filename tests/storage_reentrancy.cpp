#include "rl_option.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace {

struct ReentrantValue;
using ReentrantOption = rl::Option<ReentrantValue, int>;

struct ReentrantValue {
    static inline int destruction_count = 0;
    static inline bool observed_none_during_destruction = false;

    ReentrantOption *owner = nullptr;

    constexpr explicit ReentrantValue(ReentrantOption *owner_arg = nullptr)
        : owner(owner_arg) {}

    constexpr ~ReentrantValue() noexcept {
        ++destruction_count;
        if (owner != nullptr) {
            observed_none_during_destruction = owner->is_none();
            owner->reset();
        }
    }
};

static_assert(std::same_as<
              rl::details::option_index_type_<
                  std::numeric_limits<unsigned char>::max()>::type,
              unsigned char>);
static_assert(std::same_as<
              rl::details::option_index_type_<
                  std::numeric_limits<unsigned char>::max() + std::size_t{1}>::type,
              unsigned short>);

} // namespace

int main() {
    ReentrantOption option{rl::Some<ReentrantValue>(ReentrantValue{nullptr})};
    ReentrantValue::destruction_count = 0;
    option.unwrap<ReentrantValue>().owner = &option;

    option.reset();

    if (ReentrantValue::destruction_count != 1 ||
        !ReentrantValue::observed_none_during_destruction ||
        !option.is_none()) {
        return 1;
    }
}
