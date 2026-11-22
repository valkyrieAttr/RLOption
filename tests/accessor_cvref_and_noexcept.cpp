#include "rl_option.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace {

using IntOption = rl::Option<int>;

static_assert(std::same_as<decltype(std::declval<IntOption &>().value()), int &>);
static_assert(std::same_as<decltype(std::declval<const IntOption &>().value()),
                           const int &>);
static_assert(std::same_as<decltype(std::declval<IntOption &&>().value()),
                           int &&>);
static_assert(std::same_as<decltype(std::declval<const IntOption &&>().value()),
                           const int &&>);

static_assert(std::same_as<decltype(*std::declval<IntOption &>()), int &>);
static_assert(std::same_as<decltype(*std::declval<const IntOption &>()),
                           const int &>);
static_assert(std::same_as<decltype(*std::declval<IntOption &&>()), int &&>);
static_assert(std::same_as<decltype(*std::declval<const IntOption &&>()),
                           const int &&>);

static_assert(std::same_as<decltype(std::declval<IntOption &>().operator->()),
                           int *>);
static_assert(std::same_as<decltype(std::declval<const IntOption &>().operator->()),
                           const int *>);
static_assert(std::same_as<decltype(std::declval<IntOption &&>().operator->()),
                           int *>);
static_assert(std::same_as<decltype(std::declval<const IntOption &&>().operator->()),
                           const int *>);

static_assert(std::same_as<decltype(std::declval<IntOption &>().unwrap()), int &>);
static_assert(std::same_as<decltype(std::declval<const IntOption &>().unwrap()),
                           const int &>);
static_assert(std::same_as<decltype(std::declval<IntOption &&>().unwrap()), int>);
static_assert(std::same_as<decltype(std::declval<const IntOption &&>().unwrap()),
                           int>);
static_assert(std::same_as<decltype(std::declval<IntOption &>().expect("x")),
                           int &>);
static_assert(std::same_as<decltype(std::declval<const IntOption &>().expect("x")),
                           const int &>);
static_assert(std::same_as<decltype(std::declval<IntOption &&>().expect("x")),
                           int>);
static_assert(std::same_as<decltype(std::declval<const IntOption &&>().expect("x")),
                           int>);

static_assert(noexcept(std::declval<rl::Some<int> &>().get()));
static_assert(noexcept(std::declval<const rl::Some<int> &>().get()));
static_assert(noexcept(std::declval<rl::Some<int> &&>().get()));
static_assert(noexcept(std::declval<const rl::Some<int> &&>().get()));

struct NothrowNontrivial {
    int value = 0;

    NothrowNontrivial() = default;
    NothrowNontrivial(const NothrowNontrivial &other) noexcept
        : value(other.value) {}
    NothrowNontrivial(NothrowNontrivial &&other) noexcept
        : value(other.value) {}
    NothrowNontrivial &operator=(const NothrowNontrivial &other) noexcept {
        value = other.value;
        return *this;
    }
    NothrowNontrivial &operator=(NothrowNontrivial &&other) noexcept {
        value = other.value;
        return *this;
    }
    ~NothrowNontrivial() noexcept {}
};

static_assert(std::is_nothrow_copy_assignable_v<NothrowNontrivial>);
static_assert(std::is_nothrow_copy_assignable_v<rl::Option<NothrowNontrivial>>);

struct Mutable {
    int calls = 0;
    void mutate() noexcept { ++calls; }
};

static_assert(requires(IntOption &&option) { std::move(option).operator->(); });
static_assert(requires(rl::Option<Mutable> &&option) { std::move(option)->mutate(); });

} // namespace

int main() {
    NothrowNontrivial original{};
    NothrowNontrivial copied{original};
    NothrowNontrivial moved{std::move(copied)};
    original = moved;
    original = std::move(moved);

    rl::Option<NothrowNontrivial> first{rl::Some(original)};
    rl::Option<NothrowNontrivial> second{rl::Some(NothrowNontrivial{})};
    second = first;

    rl::Option<Mutable> option{rl::Some(Mutable{})};
    std::move(option)->mutate();
    return option.unwrap().calls == 1 && second.unwrap().value == 0 ? 0 : 1;
}
