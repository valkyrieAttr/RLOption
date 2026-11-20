#include "rl_option.hpp"

namespace {

struct OldTag {};

struct OldValue {
    int member;

    explicit OldValue(OldTag, int value) : member(value) {}
    ~OldValue() noexcept { member = -1; }
};

struct NonMovableTarget {
    int observed;

    explicit NonMovableTarget(int &source) : observed(source) {}
    NonMovableTarget(const NonMovableTarget &) = delete;
    NonMovableTarget(NonMovableTarget &&) = delete;
};

} // namespace

int main() {
    rl::Option<OldValue, NonMovableTarget> option{rl::Some<OldValue>(OldValue{OldTag{}, 42})};
    int &aliased_member = option.unwrap<OldValue>().member;

    option = aliased_member;

    if (!option.is_some<NonMovableTarget>() ||
        option.unwrap<NonMovableTarget>().observed != 42) {
        return 1;
    }
}
