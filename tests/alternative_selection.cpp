#include "rl_option.hpp"

#include <concepts>
#include <type_traits>

namespace {

struct Source {
    Source() = default;
    Source(const Source &) = delete;
    Source(Source &&) = default;
};

struct ConversionTarget {
    bool constructed_from_source = false;

    explicit ConversionTarget(Source &) noexcept : constructed_from_source(true) {}
    ConversionTarget(const ConversionTarget &) = delete;
    ConversionTarget(ConversionTarget &&) noexcept = default;
};

using Selection = rl::details::match_alternative_<Source &, Source,
                                                   ConversionTarget>;
static_assert(Selection::valid);
static_assert(std::same_as<typename Selection::type, ConversionTarget>);

} // namespace

int main() {
    rl::Option<Source, ConversionTarget> option{rl::None};
    Source source;
    option = source;

    if (!option.is_some<ConversionTarget>()) {
        return 1;
    }
    return option.unwrap<ConversionTarget>().constructed_from_source ? 0 : 1;
}
