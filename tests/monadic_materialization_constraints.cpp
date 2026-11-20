#include "rl_option.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace {

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = delete;
};

using NonCopyableOption = rl::Option<NonCopyable>;

struct MapNonCopyableReference {
    NonCopyable &operator()(int) const;
};

struct FallbackNonCopyableReference {
    NonCopyable &operator()() const;
};

inline std::unique_ptr<int> unique_result = std::make_unique<int>(1);

struct MapUniqueValue {
    std::unique_ptr<int> operator()(int) const { return std::make_unique<int>(1); }
};

struct FallbackUniqueReference {
    std::unique_ptr<int> &operator()() const { return unique_result; }
};

struct MapUniqueByNone {
    std::unique_ptr<int> operator()(int) const { return std::make_unique<int>(1); }
    std::unique_ptr<int> &operator()(rl::None_t) const { return unique_result; }
};

struct ChainNonCopyableReference {
    NonCopyableOption &operator()(int) const;
};

struct Copyable {
    Copyable() = default;
    Copyable(const Copyable &) = default;
    Copyable(Copyable &&) = default;
};

inline Copyable copyable_result{};
struct MapCopyValue { Copyable operator()(int) const { return {}; } };
struct MapCopyLvalue { Copyable &operator()(int) const { return copyable_result; } };
struct MapCopyConstLvalue {
    const Copyable &operator()(int) const { return copyable_result; }
};
struct MapCopyRvalue {
    Copyable &&operator()(int) const { return std::move(copyable_result); }
};
struct FallbackCopyValue { Copyable operator()() const { return {}; } };
struct FallbackCopyLvalue { Copyable &operator()() const { return copyable_result; } };
struct FallbackCopyConstLvalue {
    const Copyable &operator()() const { return copyable_result; }
};
struct FallbackCopyRvalue {
    Copyable &&operator()() const { return std::move(copyable_result); }
};

struct MapInteger {
    int operator()(int value) const { return value + 1; }
};

struct FallbackInteger {
    int operator()() const { return 0; }
};

struct ChainInteger {
    rl::Option<int> operator()(int value) const { return rl::Some(value + 1); }
};

inline rl::Option<int> chain_option{rl::Some(1)};
struct ChainOptionLvalue {
    rl::Option<int> &operator()(int) const { return chain_option; }
};
struct ChainOptionConstLvalue {
    const rl::Option<int> &operator()(int) const { return chain_option; }
};
struct ChainOptionRvalue {
    rl::Option<int> &&operator()(int) const { return std::move(chain_option); }
};

template <typename O, typename F>
concept CanMap = requires(O &option, F &callback) {
    option.map(callback);
};

template <typename O, typename F, typename G>
concept CanMapOrElse = requires(O &option, F &callback, G &fallback) {
    option.map_or_else(callback, fallback);
};

template <typename O, typename F>
concept CanAndThen = requires(O &option, F &callback) {
    option.and_then(callback);
};

template <typename O, typename F, typename U>
concept CanMapOrTransform = requires(O &option, F &callback, U &&fallback) {
    option.map_or_transform(callback, std::forward<U>(fallback));
};

template <typename O, typename F, typename U>
concept CanTargetMapOrTransform = requires(O &option, F &callback, U &&fallback) {
    option.template map_or_transform<int>(callback, std::forward<U>(fallback));
};

template <typename O, typename F, typename G>
concept CanTargetMapOrElse = requires(O &option, F &callback, G &fallback) {
    option.template map_or_else<int>(callback, fallback);
};

static_assert(!CanMap<rl::Option<int>, MapNonCopyableReference>);
static_assert(!CanMapOrElse<rl::Option<int>, MapNonCopyableReference,
                           FallbackNonCopyableReference>);
static_assert(!CanAndThen<rl::Option<int>, ChainNonCopyableReference>);
static_assert(!CanMapOrElse<rl::Option<int>, MapUniqueValue,
                           FallbackUniqueReference>);
static_assert(!CanMapOrTransform<rl::Option<int>, MapUniqueByNone,
                                 std::unique_ptr<int>>);
static_assert(!CanTargetMapOrElse<rl::Option<int>, MapUniqueValue,
                                  FallbackUniqueReference>);
static_assert(!CanTargetMapOrTransform<rl::Option<int>, MapUniqueByNone,
                                       std::unique_ptr<int>>);

static_assert(CanMap<rl::Option<int>, MapCopyValue>);
static_assert(CanMap<rl::Option<int>, MapCopyLvalue>);
static_assert(CanMap<rl::Option<int>, MapCopyConstLvalue>);
static_assert(CanMap<rl::Option<int>, MapCopyRvalue>);
static_assert(CanMapOrElse<rl::Option<int>, MapCopyValue, FallbackCopyValue>);
static_assert(CanMapOrElse<rl::Option<int>, MapCopyLvalue, FallbackCopyLvalue>);
static_assert(CanMapOrElse<rl::Option<int>, MapCopyConstLvalue,
                           FallbackCopyConstLvalue>);
static_assert(CanMapOrElse<rl::Option<int>, MapCopyRvalue, FallbackCopyRvalue>);
static_assert(CanAndThen<rl::Option<int>, ChainOptionLvalue>);
static_assert(CanAndThen<rl::Option<int>, ChainOptionConstLvalue>);
static_assert(CanAndThen<rl::Option<int>, ChainOptionRvalue>);

static_assert(CanMap<rl::Option<int>, MapInteger>);
static_assert(CanMapOrElse<rl::Option<int>, MapInteger, FallbackInteger>);
static_assert(CanAndThen<rl::Option<int>, ChainInteger>);

} // namespace

int main() {}
