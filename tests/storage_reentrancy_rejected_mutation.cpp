#include "rl_option.hpp"

struct ReentrantValue;
using ReentrantOption = rl::Option<ReentrantValue, int>;

struct ReentrantValue {
    ReentrantOption *owner = nullptr;

    explicit ReentrantValue(ReentrantOption *owner_arg = nullptr)
        : owner(owner_arg) {}

    ~ReentrantValue() noexcept {
        if (owner != nullptr) {
            owner->emplace<int>(7);
        }
    }
};

int main() {
    ReentrantOption option{rl::Some<ReentrantValue>(ReentrantValue{nullptr})};
    option.unwrap<ReentrantValue>().owner = &option;
    option.reset();
}
