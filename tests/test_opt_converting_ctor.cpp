#include <rl_option.hpp>

#include <concepts>
#include <string>
#include <type_traits>

namespace {

using StringOption_ = rl::Option<int, std::string>;
using TextLiteral_ = const char (&)[6];

static_assert(std::same_as<
              rl::details::match_alternative_<int, int, std::string>::type,
              int>);
static_assert(std::same_as<rl::details::match_alternative_<
                               TextLiteral_, int, std::string>::type,
                           std::string>);
static_assert(rl::details::match_alternative_<
              TextLiteral_, int, std::string>::valid);

// Raw values stay explicitly rejected; Some<T> is the construction path.
static_assert(!std::constructible_from<StringOption_, int>);
static_assert(!std::constructible_from<StringOption_, const char *>);
static_assert(std::constructible_from<StringOption_, decltype(rl::Some(7))>);
static_assert(std::constructible_from<
              StringOption_, decltype(rl::Some(std::string{"text"}))>);
static_assert(std::constructible_from<
              StringOption_, decltype(rl::Some<const char *>("text"))>);

} // namespace

int main() {
    StringOption_ primary{rl::Some(7)};
    StringOption_ converted{rl::Some<const char *>("mapped")};

    if (!primary.is_some<int>() || primary.unwrap<int>() != 7) {
        return 1;
    }

    if (!converted.is_some<std::string>() ||
        converted.unwrap<std::string>() != "mapped") {
        return 2;
    }

    return 0;
}
