#include "rl_option.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

struct ImmovableFromInt {
    int value;

    constexpr explicit ImmovableFromInt(int input) noexcept : value{input} {}
    ImmovableFromInt(const ImmovableFromInt &) = delete;
    ImmovableFromInt(ImmovableFromInt &&) = delete;
};

struct MoveCounter {
    static inline int moves = 0;
    int value;

    constexpr explicit MoveCounter(int input) noexcept : value{input} {}
    MoveCounter(const MoveCounter &) = delete;
    MoveCounter(MoveCounter &&other) noexcept : value{other.value} { ++moves; }
};

struct Tracked {
    int *live;

    explicit Tracked(int *count) noexcept : live{count} { ++*live; }
    Tracked(const Tracked &) = delete;
    Tracked(Tracked &&other) noexcept : live{other.live} { ++*live; }
    ~Tracked() noexcept { --*live; }
};

struct Source {
    std::string text;
    explicit Source(std::string value) : text{std::move(value)} {}
};

struct Snapshot {
    std::string text;
    explicit Snapshot(std::string_view value) : text{value} {}
    Snapshot(Snapshot &&) noexcept = default;
    Snapshot(const Snapshot &) = delete;
};

struct ThrowingMoveSeed {};

struct ThrowingMove {
    explicit ThrowingMove(ThrowingMoveSeed) noexcept {}
    ThrowingMove(const ThrowingMove &) = delete;
    ThrowingMove(ThrowingMove &&) { throw std::runtime_error{"move failed"}; }
    ~ThrowingMove() noexcept = default;
};

int main() {
    // Converting construction directly builds the selected alternative. It
    // therefore supports immovable alternatives and performs no extra move.
    static_assert(std::is_constructible_v<rl::Option<ImmovableFromInt>, int>);
    rl::Option<ImmovableFromInt> immovable{42};
    assert(immovable.unwrap().value == 42);

    MoveCounter::moves = 0;
    rl::Option<MoveCounter> direct{7};
    assert(direct.unwrap().value == 7);
    assert(MoveCounter::moves == 0);

    // The deepest recursive alternative is destroyed exactly once when the
    // Option changes to its primary alternative and when reset is repeated.
    int live = 0;
    rl::Option<int, char, std::string, Tracked> tracked{&live};
    assert(live == 1);
    tracked = 12;
    assert(live == 0);
    assert(tracked.unwrap() == 12);
    tracked.reset();
    tracked.reset();
    assert(live == 0);

    // A converting assignment stages an owning target before destroying the
    // source alternative, so its constructor can safely read source storage.
    rl::Option<Source, Snapshot> assigned{
        rl::Some{Source{std::string{"assignment source"}}}};
    std::string_view assignment_view{assigned.unwrap<Source>().text};
    assigned = assignment_view;
    assert(assigned.unwrap<Snapshot>().text == "assignment source");

    // replace() likewise creates the replacement before moving out the old
    // alternative. This matters for small-string storage as well as heap data.
    rl::Option<Source, Snapshot> replaced{
        rl::Some{Source{std::string{"SSO"}}}};
    std::string_view replace_view{replaced.unwrap<Source>().text};
    auto old = replaced.replace(replace_view);
    assert(old.unwrap<Source>().text == "SSO");
    assert(replaced.unwrap<Snapshot>().text == "SSO");

    // If construction of a last-position alternative throws during the move
    // from the staged object, recursive union wrappers are rolled back and
    // the Option remains disengaged and reusable.
    rl::Option<int, char, std::string, ThrowingMove> throwing{rl::None};
    bool caught = false;
    try {
        throwing = ThrowingMoveSeed{};
    } catch (const std::runtime_error &) {
        caught = true;
    }
    assert(caught);
    assert(throwing.is_none());
    throwing = 5;
    assert(throwing.unwrap() == 5);

    // Where() must decay the predicate object, not name the decay trait.
    rl::Option<int> positive{rl::Some{9}};
    auto matched = positive.match(
        rl::None >>= [] { return -1; },
        rl::Where([](const int &value) { return value > 0; }) >>=
            [](int value) { return value; });
    assert(matched.is_some());
    assert(*matched == 9);
}
