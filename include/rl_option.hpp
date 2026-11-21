/** @file include/rl_option.hpp
 * rl::Option<T, Ts...> is a class that covers one or more alternative types of
 * arguments and it may or may not return the T or the Ts.... rl::Option<T> is
 * comparable to std::optional<T> behavior. The behavior of std::variant<Ts...>,
 * std::expected<T, E> and std::optional<T> are integrated into this library.
 * Additionally, this library provides pattern matching support on values,
 * enabling ADT and DOD.
 */

#ifndef RL_OPTION_CXX26_HPP
#if (defined(__clang__))
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wc++98-compat-local-type-template-args"
#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Wpre-c++14-compat"
#pragma clang diagnostic ignored "-Wpre-c++14-compat-pedantic"
#pragma clang diagnostic ignored "-Wpre-c++17-compat"
#pragma clang diagnostic ignored "-Wpre-c++17-compat-pedantic"
#pragma clang diagnostic ignored "-Wpre-c++20-compat"
#pragma clang diagnostic ignored "-Wpre-c++20-compat-pedantic"
#pragma clang diagnostic ignored "-Wpre-c++23-compat"
#pragma clang diagnostic ignored "-Wpre-c++23-compat-pedantic"
#pragma clang diagnostic ignored "-Wpre-c++26-compat"
#pragma clang diagnostic ignored "-Wpre-c++26-compat-pedantic"
#elif (defined(__GNUC__) || defined(__GNUG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wtemplate-body"
#endif

#define RL_OPTION_CXX26_HPP 1

#if (__cplusplus < 202400L)
#error "Option requires C++26 or later"
#else
#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#if (__has_cpp_attribute(clang::lifetimebound))
#define RL_LIFETIME_BOUND clang::lifetimebound
#elif (__has_cpp_attribute(gnu::lifetimebound))
#define RL_LIFETIME_BOUND gnu::lifetimebound
#elif (__has_cpp_attribute(msvc::lifetimebound))
#define RL_LIFETIME_BOUND msvc::lifetimebound
#else
#define RL_LIFETIME_BOUND
#endif

#if (defined(__cpp_lib_reflection) && defined(__cpp_impl_reflection))
#include <meta>
#define RL_SUPPORTS_REFLECTION 1
#else
#define RL_SUPPORTS_REFLECTION 0
#endif

#if (defined(__cpp_lib_string_subview))
#define RL_IS_SUBVIEW_OR_SUBSTR subview
#elif (__cplusplus >= 202302L)
#define RL_IS_SUBVIEW_OR_SUBSTR substr
#endif

namespace rl {
namespace details {
consteval auto max_sizeof_(void) noexcept(true) -> std::size_t { return 1; }

consteval auto max_alignof_(void) noexcept(true) -> std::size_t { return 1; }

template <typename First_, typename... Rest_>
consteval auto max_sizeof_(void) noexcept(true) -> std::size_t {
    return (std::max({sizeof(First_), sizeof(Rest_)...}));
}

template <typename First_, typename... Rest_>
consteval auto max_alignof_(void) noexcept(true) -> std::size_t {
    return (std::max({alignof(First_), alignof(Rest_)...}));
}

consteval auto align_up_(std::size_t size_,
                         std::size_t alignment_) noexcept(true) -> std::size_t {
    return ((size_ + alignment_ - 1) / alignment_ * alignment_);
}

template <typename... Args_>
[[noreturn]] constexpr auto panic_(std::format_string<Args_...> fmt,
                                   Args_ &&...args) -> void {
    if consteval {
#if defined(__cpp_exceptions)
        throw "rl::Option panic: precondition violated";
#else
        struct NonConstexprPanic_ {
            [[noreturn]] static auto fail() -> void { std::abort(); }
        };
        NonConstexprPanic_::fail();
#endif
    } else {
        std::println(stderr, fmt, std::forward<Args_>(args)...);
        std::abort();
    }
}

// Count_ is the largest state value that must fit, including the
// reserved in-progress-destruction state.
template <std::size_t Count_> struct option_index_type_ {
  public:
    using type = std::conditional_t<
        Count_ <= std::numeric_limits<unsigned char>::max(), unsigned char,
        std::conditional_t<Count_ <= std::numeric_limits<unsigned short>::max(),
                           unsigned short, std::size_t>>;
};

template <typename Qp_, typename... Types_> struct type_index_ {
    static constexpr std::size_t value = [] consteval -> std::size_t {
        std::size_t idx = 0;
        bool found = false;
        auto step = [&]<typename T_>() {
            if (!found) {
                if constexpr (std::same_as<Qp_, T_>) {
                    found = true;
                } else {
                    ++idx;
                }
            }
        };
        (step.template operator()<Types_>(), ...);
        return idx;
    }();
};

template <typename Qp_> class alignas(Qp_) [[nodiscard]] Optional_;

template <typename Tp_, typename... Args_>
class alignas(
    max_alignof_<Tp_, Args_...,
                 typename option_index_type_<sizeof...(Args_) + 2>::type>())
    OptionStorage_;

template <typename Types_, bool Enabled> struct option_hash_base_;

namespace Opt {
template <typename Tp_, typename... Args_>
class alignas(
    details::max_alignof_<
        Tp_, Args_...,
        typename details::option_index_type_<sizeof...(Args_) + 2>::type>())
    [[nodiscard]] Option;

template <typename Tp_ = void> struct Some;

template <typename Tp_> struct alignas(Tp_) [[nodiscard]] Some {
  private:
    Tp_ value;

  public:
    explicit constexpr Some(void) noexcept(
        std::is_nothrow_default_constructible_v<Tp_>)
        requires(std::is_default_constructible_v<Tp_>)
        : value{} {}

  public:
    explicit constexpr Some(const Tp_ &v_) noexcept(
        std::is_nothrow_copy_constructible_v<Tp_>)
        requires(std::is_copy_constructible_v<Tp_>)
        : value{v_} {}

  public:
    explicit constexpr Some(Tp_ &&v_) noexcept(
        std::is_nothrow_move_constructible_v<Tp_>)
        requires(std::is_move_constructible_v<Tp_>)
        : value{std::move(v_)} {}

  public:
    constexpr const Tp_ &get(void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] {
        return this->value;
    }

  public:
    constexpr Tp_ &get(void) & noexcept(true) [[RL_LIFETIME_BOUND]] {
        return this->value;
    }

  public:
    constexpr Tp_ &&get(void) && noexcept(true) [[RL_LIFETIME_BOUND]] {
        return std::move(this->value);
    }

  public:
    constexpr const Tp_ &&get(void) const && noexcept(true) {
        return std::move(this->value);
    }

  public:
    template <typename OpTp_, typename... OpArgs_> friend class Opt::Option;

  public:
    friend class details::Optional_<Tp_>;

  public:
    template <typename OpTp_, typename... OpArgs_>
    friend class details::OptionStorage_;

  public:
    friend constexpr auto operator==(const Some<Tp_> &lhs, const Some<Tp_> &rhs)
        -> bool
        requires(std::equality_comparable<Tp_>)
    {
        return lhs.value == rhs.value;
    }
};

template <> struct [[nodiscard]] Some<void> {
  public:
    constexpr Some(void) noexcept(true) = default;

  public:
    template <typename Cp_>
        requires(std::is_default_constructible_v<Cp_>)
    constexpr operator Some<Cp_>(void) const
        noexcept(std::is_nothrow_default_constructible_v<Cp_>) {
        return Some<Cp_>{};
    }
};

template <typename Tp_> Some(Tp_) -> Some<Tp_>;
Some() -> Some<void>;

class [[nodiscard]] None {
  public:
    constexpr None(void) noexcept(true) {}

  public:
    constexpr auto operator()(void) const noexcept(true) -> None {
        return *this;
    }

  public:
    friend constexpr auto operator==(typename Opt::None, typename Opt::None)
        -> bool {
        return true;
    }

  public:
    template <typename Tp_>
    friend constexpr auto operator==(typename Opt::None, const Some<Tp_> &)
        -> bool {
        return false;
    }

  public:
    template <typename Tp_>
    friend constexpr auto operator==(const Some<Tp_> &, typename Opt::None)
        -> bool {
        return false;
    }
};

class [[nodiscard]] Default {
  public:
    constexpr Default(void) noexcept(true) {}

  public:
    constexpr auto operator()(void) const noexcept(true) -> Default {
        return *this;
    }

  public:
    template <typename Cp_>
    constexpr explicit operator Cp_(void) const
        noexcept(std::is_nothrow_default_constructible_v<Cp_>)
        requires(std::is_default_constructible_v<Cp_> &&
                 !std::same_as<Cp_, Opt::None> &&
                 !std::same_as<Cp_, Opt::Default>)
    {
        return Cp_{};
    }

  public:
    friend constexpr auto operator==(typename Opt::Default,
                                     typename Opt::Default) -> bool {
        return true;
    }

  public:
    friend constexpr auto operator==(typename Opt::Default, typename Opt::None)
        -> bool {
        return false;
    }

  public:
    friend constexpr auto operator==(typename Opt::None, typename Opt::Default)
        -> bool {
        return false;
    }

  public:
    template <typename Tp_>
    friend constexpr auto operator==(typename Opt::Default,
                                     const Some<Tp_> &rhs_) -> bool
        requires(std::is_default_constructible_v<Tp_> &&
                 std::equality_comparable<Tp_>)
    {
        const Tp_ def_{};
        return def_ == rhs_.get();
    }

  public:
    template <typename Tp_>
    friend constexpr auto operator==(const Some<Tp_> &lhs_,
                                     typename Opt::Default) -> bool
        requires(std::is_default_constructible_v<Tp_> &&
                 std::equality_comparable<Tp_>)
    {
        const Tp_ def_{};
        return lhs_.get() == def_;
    }
};

}; /* namespace Opt */

}; // namespace details

namespace details {
template <std::size_t N_> struct Pad_ {
  public:
    unsigned char _pad[N_]{};
};

template <> struct Pad_<0> {};

template <typename Qp_, bool TrivialDtor_ = (std::is_trivially_destructible_v<
                                             std::remove_cv_t<Qp_>>)>
union alignas(Qp_) Storage_;

template <typename Qp_> union alignas(Qp_) Storage_<Qp_, true> {
  public:
    std::remove_cv_t<Qp_> value;

  public:
    constexpr Storage_(void) noexcept(true) {}

  public:
    template <typename... Args_>
    constexpr Storage_(std::in_place_t, Args_ &&...args) noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, Args_...>)
        : value(std::forward<Args_>(args)...) {}
};

template <typename Qp_> union alignas(Qp_) Storage_<Qp_, false> {
  public:
    std::remove_cv_t<Qp_> value;

  public:
    constexpr Storage_(void) noexcept(true) {}

  public:
    template <typename... Args_>
    constexpr Storage_(std::in_place_t, Args_ &&...args) noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, Args_...>)
        : value(std::forward<Args_>(args)...) {}

  public:
    constexpr ~Storage_(void) noexcept(true) {}
};

template <typename Qp_,
          bool = (std::is_trivially_destructible_v<std::remove_cv_t<Qp_>>)>
struct OptionalDtor_ {
  public:
    Storage_<Qp_> storage;

  public:
    bool is_some;

  public:
    [[no_unique_address]] Pad_<alignof(Qp_) - 1> _pad{};

  public:
    constexpr OptionalDtor_(void) noexcept(true) : is_some{false} {}

  public:
    template <typename... Args_>
    constexpr OptionalDtor_(std::in_place_t, Args_ &&...args) noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, Args_...>)
        : storage{std::in_place, std::forward<Args_>(args)...}, is_some{true} {}

  protected:
    constexpr auto reset_(void) noexcept(std::is_nothrow_destructible_v<Qp_>)
        -> void {
        if (this->is_some) {
            this->is_some = false;

            std::destroy_at(const_cast<std::remove_cv_t<Qp_> *>(
                std::addressof(this->storage.value)));
        }
    }

  public:
    constexpr ~OptionalDtor_(void) noexcept(
        std::is_nothrow_destructible_v<Qp_>) {
        this->reset_();
    }
};

template <typename Qp_> struct alignas(Qp_) OptionalDtor_<Qp_, true> {
  public:
    Storage_<Qp_> storage;

  public:
    bool is_some;

  public:
    [[no_unique_address]] Pad_<alignof(Qp_) - 1> _pad{};

  public:
    constexpr OptionalDtor_(void) noexcept(true) : is_some{false} {}

  public:
    template <typename... Args_>
    constexpr OptionalDtor_(std::in_place_t, Args_ &&...args) noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, Args_...>)
        : storage{std::in_place, std::forward<Args_>(args)...}, is_some{true} {}

  protected:
    constexpr auto reset_(void) noexcept(true) -> void {
        this->is_some = false;
    }
};

// Layer 1: copy construction.
template <typename Qp_, bool = (std::is_trivially_copy_constructible_v<Qp_>)>
struct OptionalCopy_ : OptionalDtor_<Qp_> {
    using OptionalDtor_<Qp_>::OptionalDtor_;

  public:
    constexpr OptionalCopy_(void) = default;

  public:
    constexpr OptionalCopy_(OptionalCopy_ &&) = default;

  public:
    constexpr auto operator=(const OptionalCopy_ &)
        -> OptionalCopy_ & = default;

  public:
    constexpr auto operator=(OptionalCopy_ &&) -> OptionalCopy_ & = default;

  public:
    constexpr OptionalCopy_(const OptionalCopy_ &other) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_>)
        requires(std::is_copy_constructible_v<Qp_>)
        : OptionalDtor_<Qp_>{} {
        if (other.is_some) {
            std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                  std::addressof(this->storage.value)),
                              other.storage.value);

            this->is_some = true;
        }
    }

  public:
    constexpr OptionalCopy_(const OptionalCopy_ &)
        requires(!std::is_copy_constructible_v<Qp_>)
    = delete;
};

template <typename Qp_> struct OptionalCopy_<Qp_, true> : OptionalDtor_<Qp_> {
    using OptionalDtor_<Qp_>::OptionalDtor_;
};

template <typename Qp_, bool = std::is_trivially_move_constructible_v<Qp_>>
struct OptionalMove_ : OptionalCopy_<Qp_> {
    using OptionalCopy_<Qp_>::OptionalCopy_;

  public:
    constexpr OptionalMove_(void) = default;

  public:
    constexpr OptionalMove_(const OptionalMove_ &) = default;

  public:
    constexpr auto operator=(const OptionalMove_ &)
        -> OptionalMove_ & = default;

  public:
    constexpr auto operator=(OptionalMove_ &&) -> OptionalMove_ & = default;

  public:
    constexpr OptionalMove_(OptionalMove_ &&other) noexcept(
        std::is_nothrow_move_constructible_v<Qp_>)
        requires(std::is_move_constructible_v<Qp_>)
        : OptionalCopy_<Qp_>{} {
        if (other.is_some) {
            std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                  std::addressof(this->storage.value)),
                              static_cast<Qp_ &&>(other.storage.value));

            this->is_some = true;
        }
    }

  public:
    constexpr OptionalMove_(OptionalMove_ &&)
        requires(!std::is_move_constructible_v<Qp_>)
    = delete;
};

template <typename Qp_> struct OptionalMove_<Qp_, true> : OptionalCopy_<Qp_> {
    using OptionalCopy_<Qp_>::OptionalCopy_;
};

struct Wild_ {};

struct AnySome_ {};

template <bool B_> struct BoolCase_ : std::bool_constant<B_> {
    using bool_case_tag_ = void;
};

using True_ = BoolCase_<true>;
using False_ = BoolCase_<false>;

template <typename Pr_> class alignas(Pr_) Where_ {
  public:
    using where_tag_ = void;

  public:
    Pr_ pred;
};

template <typename Pr_> Where_(Pr_) -> Where_<Pr_>;

template <typename Tp_> struct is_some_pattern_ : std::false_type {};

template <typename Tp_>
struct is_some_pattern_<Opt::Some<Tp_>> : std::true_type {
  public:
    using type = Tp_;
};

template <typename Pp_>
concept MatchPattern =
    (std::same_as<std::remove_cvref_t<Pp_>, Opt::None> ||
     std::same_as<std::remove_cvref_t<Pp_>, details::Wild_> ||
     std::same_as<std::remove_cvref_t<Pp_>, details::AnySome_> ||
     details::is_some_pattern_<std::remove_cvref_t<Pp_>>::value ||
     requires { typename std::remove_cvref_t<Pp_>::bool_case_tag_; });

template <typename Pp_, typename Fp_> struct Arm_ {
  public:
    using pattern_type = std::remove_cvref_t<Pp_>;

  public:
    using handler_type = std::remove_cvref_t<Fp_>;

  private:
    static constexpr std::size_t pattern_footprint_ =
        std::is_empty_v<pattern_type> ? std::size_t{0} : sizeof(pattern_type);

    // This pad aligns the handler after the pattern. If the pattern is more
    // strongly aligned, Arm_ can also have valid implicit tail padding.
  private:
    static constexpr std::size_t mid_pad_ =
        align_up_(pattern_footprint_, alignof(handler_type)) -
        pattern_footprint_;

  public:
    [[no_unique_address]] pattern_type pattern;

  public:
    [[no_unique_address]] Pad_<mid_pad_> _mid_pad{};

  public:
    handler_type handler;

  public:
    template <typename PpCtor_, typename FpCtor_>
    constexpr Arm_(PpCtor_ &&pattern_, FpCtor_ &&handler_) noexcept(
        std::is_nothrow_constructible_v<pattern_type, PpCtor_> &&
        std::is_nothrow_constructible_v<handler_type, FpCtor_>)
        requires(std::is_constructible_v<pattern_type, PpCtor_> &&
                 std::is_constructible_v<handler_type, FpCtor_>)
        : pattern(std::forward<PpCtor_>(pattern_)),
          handler(std::forward<FpCtor_>(handler_)) {}
};

template <typename Pp_, typename Fp_> Arm_(Pp_, Fp_) -> Arm_<Pp_, Fp_>;

template <typename Pp_, typename Fp_>
constexpr auto operator>>=(Pp_ &&pattern_, Fp_ &&handler_)
    -> Arm_<std::decay_t<Pp_>, std::decay_t<Fp_>>
    requires(!std::is_integral_v<std::remove_cvref_t<Fp_>> ||
             MatchPattern<Pp_> || !std::is_integral_v<std::remove_cvref_t<Pp_>>)
{
    return {std::forward<Pp_>(pattern_), std::forward<Fp_>(handler_)};
}

template <typename From_, typename To_>
using ForwardLike_t = decltype(std::forward_like<From_>(std::declval<To_ &>()));

template <typename Pp_, typename Tp_>
constexpr auto pattern_matches_type_(void) noexcept(true) -> bool {
    using Pd_ = std::remove_cvref_t<Pp_>;

    if constexpr (std::same_as<Pd_, Opt::None>) {
        return false;
    }

    else if constexpr (std::same_as<Pd_, Opt::Default>) {
        return (std::is_default_constructible_v<Tp_> &&
                std::equality_comparable<Tp_>);
    }

    else if constexpr (std::same_as<Pd_, details::Wild_> ||
                       std::same_as<Pd_, details::AnySome_>) {
        return true;
    }

    else if constexpr (is_some_pattern_<Pd_>::value) {
        return (std::same_as<Tp_, typename is_some_pattern_<Pd_>::type> &&
                std::equality_comparable<Tp_>);
    }

    else if constexpr (requires(Pd_ &p_, const Tp_ &v_) {
                           typename Pd_::where_tag_;
                           { p_.pred(v_) } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (requires(Pd_ &p_) {
                           typename Pd_::where_tag_;
                           { p_.pred() } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (requires(Pd_ &p_, const Tp_ &v_) {
                           { p_(v_) } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (
        !std::same_as<Pd_, bool> &&
        !requires { typename Pd_::bool_case_tag_; } &&
        requires(Pd_ &p_) {
            { p_() } -> std::convertible_to<bool>;
        }) {
        return true;
    }

    else if constexpr (std::same_as<Pd_, bool> ||
                       requires { typename Pd_::bool_case_tag_; }) {
        return std::same_as<Tp_, bool>;
    }

    else {
        return (requires(const Tp_ &v_, const Pd_ &p_) {
            { v_ == p_ } -> std::convertible_to<bool>;
        });
    }
}

// `match()` keeps the original Arm_ objects and dispatches directly over
// their types. No arm table, erased callback, or runtime arm loop is built.
template <typename T_>
concept is_callable_type_ = requires {
    &std::remove_cvref_t<T_>::operator();
} || std::is_function_v<std::remove_pointer_t<std::decay_t<T_>>>;

template <typename Handler_, typename Tp_>
inline constexpr bool handler_accepts_type_ =
    std::is_invocable_v<Handler_ &, Tp_ &> ||
    std::is_invocable_v<Handler_ &, const Tp_ &> ||
    std::is_invocable_v<Handler_ &, Tp_ &&> ||
    std::is_invocable_v<Handler_ &, const Tp_ &&> ||
    std::is_invocable_v<Handler_ &> || (!is_callable_type_<Handler_>);

template <typename Arm_>
consteval auto match_arm_is_explicit_none_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires { typename ArmD_::pattern_type; }) {
        return std::same_as<std::remove_cvref_t<typename ArmD_::pattern_type>,
                            Opt::None>;
    } else {
        return false;
    }
}

template <typename Arm_> consteval auto match_arm_is_wild_none_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires {
                      typename ArmD_::pattern_type;
                      typename ArmD_::handler_type;
                  }) {
        if constexpr (std::same_as<
                          std::remove_cvref_t<typename ArmD_::pattern_type>,
                          details::Wild_>) {
            using Handler_ = typename ArmD_::handler_type;
            if constexpr (std::is_invocable_v<Handler_ &>) {
                return true;
            } else if constexpr (!is_callable_type_<Handler_>) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}

template <typename Arm_> consteval auto match_arm_covers_none_(void) -> bool {
    return match_arm_is_explicit_none_<Arm_>() ||
           match_arm_is_wild_none_<Arm_>();
}

template <typename Arm_, typename Tp_>
consteval auto match_arm_is_catch_all_for_type_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires {
                      typename ArmD_::pattern_type;
                      typename ArmD_::handler_type;
                  }) {
        using Pattern_ = typename ArmD_::pattern_type;
        using Handler_ = typename ArmD_::handler_type;
        if constexpr (std::same_as<Pattern_, details::Wild_> ||
                      std::same_as<Pattern_, details::AnySome_>) {
            return handler_accepts_type_<Handler_, Tp_>;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

template <typename Arm_>
consteval auto match_arm_is_some_catch_all_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires {
                      typename ArmD_::pattern_type;
                      typename ArmD_::handler_type;
                  }) {
        using Pattern_ = typename ArmD_::pattern_type;
        return (std::same_as<Pattern_, details::Wild_> ||
                std::same_as<Pattern_, details::AnySome_>);
    } else {
        return false;
    }
}

template <typename Arm_, typename AlternativesTuple_>
struct match_arm_is_universal_catch_all_;

template <typename Arm_, typename... Alternatives_>
struct match_arm_is_universal_catch_all_<Arm_, std::tuple<Alternatives_...>>
    : std::bool_constant<(
          match_arm_is_catch_all_for_type_<Arm_, Alternatives_>() && ...)> {};

template <typename Arm_, typename Tp_>
consteval auto match_arm_matches_type_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires {
                      typename ArmD_::pattern_type;
                      typename ArmD_::handler_type;
                  }) {
        using Pattern_ = typename ArmD_::pattern_type;
        using Handler_ = typename ArmD_::handler_type;
        return pattern_matches_type_<Pattern_, Tp_>() &&
               handler_accepts_type_<Handler_, Tp_>;
    } else {
        return false;
    }
}

template <typename Tp_, typename Arms_> struct match_type_covered_;

template <typename Tp_, typename... Arms_>
struct match_type_covered_<Tp_, std::tuple<Arms_...>>
    : std::bool_constant<(match_arm_matches_type_<Arms_, Tp_>() || ...)> {};

template <typename Types_, typename Arms_> struct match_all_types_covered_;

template <typename... Types_, typename... Arms_>
struct match_all_types_covered_<std::tuple<Types_...>, std::tuple<Arms_...>>
    : std::bool_constant<(
          match_type_covered_<Types_, std::tuple<Arms_...>>::value && ...)> {};

template <typename Arm_> consteval auto match_arm_is_catch_all_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires { typename ArmD_::pattern_type; }) {
        using P_ = std::remove_cvref_t<typename ArmD_::pattern_type>;
        return (std::same_as<P_, details::Wild_> ||
                std::same_as<P_, details::AnySome_>);
    } else {
        return false;
    }
}

template <typename Arm_> consteval auto match_arm_is_true_case_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires { typename ArmD_::pattern_type; }) {
        using P_ = std::remove_cvref_t<typename ArmD_::pattern_type>;
        return (std::same_as<P_, details::BoolCase_<true>>);
    } else {
        return false;
    }
}

template <typename Arm_> consteval auto match_arm_is_false_case_(void) -> bool {
    using ArmD_ = std::remove_cvref_t<Arm_>;
    if constexpr (requires { typename ArmD_::pattern_type; }) {
        using P_ = std::remove_cvref_t<typename ArmD_::pattern_type>;
        return (std::same_as<P_, details::BoolCase_<false>>);
    } else {
        return false;
    }
}

template <typename Tp_, typename... Arms_>
consteval auto match_type_exhaustively_covered_(void) -> bool {
    if constexpr ((match_arm_is_catch_all_for_type_<Arms_, Tp_>() || ...)) {
        return true;
    } else if constexpr (std::same_as<std::remove_cvref_t<Tp_>, bool>) {
        return (match_arm_is_true_case_<Arms_>() || ...) &&
               (match_arm_is_false_case_<Arms_>() || ...);
    } else {
        return false;
    }
}

template <typename Types_, typename Arms_> struct match_all_types_exhaustive_;

template <typename... Types_, typename... Arms_>
struct match_all_types_exhaustive_<std::tuple<Types_...>, std::tuple<Arms_...>>
    : std::bool_constant<(
          match_type_exhaustively_covered_<Types_, Arms_...>() && ...)> {};

// Exhaustiveness is part of overload participation, not merely a body check.
template <typename TypesTuple_, typename ArmsTuple_>
struct match_is_exhaustive_;

template <typename... Types_, typename... Arms_>
struct match_is_exhaustive_<std::tuple<Types_...>, std::tuple<Arms_...>>
    : std::bool_constant<
          (match_arm_covers_none_<Arms_>() || ...) &&
          match_all_types_covered_<std::tuple<Types_...>,
                                   std::tuple<Arms_...>>::value &&
          match_all_types_exhaustive_<std::tuple<Types_...>,
                                      std::tuple<Arms_...>>::value> {};

template <typename Pp_, typename Tp_>
constexpr auto matches_value_(Pp_ &pat_, const Tp_ &value_) -> bool {
    using Pd_ = std::remove_cvref_t<Pp_>;

    if constexpr (std::same_as<Pd_, Opt::None>) {
        return false;
    }

    else if constexpr (std::same_as<Pd_, Opt::Default>) {
        if constexpr (std::is_default_constructible_v<Tp_> &&
                      std::equality_comparable<Tp_>) {
            const Tp_ def_{};
            return value_ == def_;
        } else {
            return false;
        }
    }

    else if constexpr (std::same_as<Pd_, details::Wild_> ||
                       std::same_as<Pd_, details::AnySome_>) {
        return true;
    }

    else if constexpr (is_some_pattern_<Pd_>::value) {
        if constexpr (std::same_as<Tp_, typename is_some_pattern_<Pd_>::type> &&
                      std::equality_comparable<Tp_>) {
// Intentional: pattern matching uses equality comparison including for floats.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
            return value_ == pat_.get();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        } else {
            return false;
        }
    }

    else if constexpr (requires(Pd_ &p_, const Tp_ &v_) {
                           typename Pd_::where_tag_;
                           { p_.pred(v_) } -> std::convertible_to<bool>;
                       }) {
        return static_cast<bool>(pat_.pred(value_));
    }

    else if constexpr (requires(Pd_ &p_) {
                           typename Pd_::where_tag_;
                           { p_.pred() } -> std::convertible_to<bool>;
                       }) {
        return static_cast<bool>(pat_.pred());
    }

    else if constexpr (requires(Pd_ &p_, const Tp_ &v_) {
                           { p_(v_) } -> std::convertible_to<bool>;
                       }) {
        return static_cast<bool>(pat_(value_));
    }

    else if constexpr (
        !std::same_as<Pd_, bool> &&
        !requires { typename Pd_::bool_case_tag_; } &&
        requires(Pd_ &p_) {
            { p_() } -> std::convertible_to<bool>;
        }) {
        return static_cast<bool>(pat_());
    }

    else if constexpr (std::same_as<Pd_, bool>) {
        if constexpr (std::same_as<Tp_, bool>) {
            return value_ == pat_;
        } else {
            return false;
        }
    }

    else if constexpr (requires { typename Pd_::bool_case_tag_; }) {
        if constexpr (std::same_as<Tp_, bool>) {
            return value_ == Pd_::value;
        } else {
            return false;
        }
    }

    else {
        return value_ == pat_;
    }
}

template <typename Rp_, typename T_>
concept can_wrap_result_ =
    std::same_as<Rp_, void> || std::same_as<std::remove_cvref_t<T_>, Rp_> ||
    std::same_as<std::remove_cvref_t<T_>, Opt::None> ||
    details::is_some_pattern_<std::remove_cvref_t<T_>>::value ||
    std::constructible_from<Rp_, T_> || std::convertible_to<T_, Rp_> ||
    requires(T_ &&t) { Rp_(Opt::Some(std::forward<T_>(t))); } ||
    requires(T_ &&t) {
        Rp_(Opt::Some<std::remove_cvref_t<T_>>(std::forward<T_>(t)));
    } || requires(T_ &&t) {
        typename Rp_::value_type;
        requires std::constructible_from<typename Rp_::value_type, T_>;
    };

template <typename Rp_, typename T_>
constexpr auto wrap_result_(T_ &&val) -> Rp_ {
    using RawVal_ = std::remove_cvref_t<T_>;
    if constexpr (std::same_as<Rp_, void>) {
        return;
    } else if constexpr (std::same_as<RawVal_, Rp_>) {
        return std::forward<T_>(val);
    } else if constexpr (std::same_as<RawVal_, Opt::None>) {
        return Rp_(Opt::None{});
    } else if constexpr (details::is_some_pattern_<RawVal_>::value) {
        return Rp_(std::forward<T_>(val));
    } else if constexpr (std::constructible_from<Rp_, T_>) {
        return Rp_(std::forward<T_>(val));
    } else if constexpr (requires { Rp_(Opt::Some(std::forward<T_>(val))); }) {
        return Rp_(Opt::Some(std::forward<T_>(val)));
    } else if constexpr (requires {
                             Rp_(Opt::Some<RawVal_>(std::forward<T_>(val)));
                         }) {
        return Rp_(Opt::Some<RawVal_>(std::forward<T_>(val)));
    } else if constexpr (requires {
                             typename Rp_::value_type;
                             requires std::constructible_from<
                                 typename Rp_::value_type, T_>;
                         }) {
        return Rp_(Opt::Some<typename Rp_::value_type>{std::forward<T_>(val)});
    } else {
        return static_cast<Rp_>(std::forward<T_>(val));
    }
}

template <typename Rp_, typename Hp_>
constexpr auto invoke_none_(Hp_ &handler_) -> Rp_ {
    if constexpr (std::is_invocable_v<Hp_ &>) {
        using Ret_ = std::invoke_result_t<Hp_ &>;
        static_assert(can_wrap_result_<Rp_, Ret_>,
                      "None/Wild arm handler return type cannot be converted "
                      "to match result");
        if constexpr (std::is_void_v<Ret_>) {
            std::invoke(handler_);
            if constexpr (std::same_as<Rp_, void>) {
                return;
            }
        } else if constexpr (!std::is_reference_v<Ret_> &&
                             std::same_as<std::remove_cvref_t<Ret_>, Rp_>) {
            return std::invoke(handler_);
        } else {
            return wrap_result_<Rp_>(std::invoke(handler_));
        }
    } else {
        static_assert(
            can_wrap_result_<Rp_, Hp_ &>,
            "None/Wild arm handler value cannot be converted to match result");
        return wrap_result_<Rp_>(handler_);
    }
}

template <typename Rp_, typename Hp_, typename Value_>
constexpr auto invoke_some_(Hp_ &handler_, Value_ &&value_) -> Rp_ {
    if constexpr (std::is_invocable_v<Hp_ &, Value_>) {
        using Ret_ = std::invoke_result_t<Hp_ &, Value_>;
        static_assert(can_wrap_result_<Rp_, Ret_>,
                      "match() arm handler return type cannot be converted to "
                      "match result");
        if constexpr (std::is_void_v<Ret_>) {
            std::invoke(handler_, std::forward<Value_>(value_));
            if constexpr (std::same_as<Rp_, void>) {
                return;
            }
        } else if constexpr (!std::is_reference_v<Ret_> &&
                             std::same_as<std::remove_cvref_t<Ret_>, Rp_>) {
            return std::invoke(handler_, std::forward<Value_>(value_));
        } else {
            return wrap_result_<Rp_>(
                std::invoke(handler_, std::forward<Value_>(value_)));
        }
    } else if constexpr (std::is_invocable_v<Hp_ &>) {
        using Ret_ = std::invoke_result_t<Hp_ &>;
        static_assert(can_wrap_result_<Rp_, Ret_>,
                      "match() arm handler return type cannot be converted to "
                      "match result");
        if constexpr (std::is_void_v<Ret_>) {
            std::invoke(handler_);
            if constexpr (std::same_as<Rp_, void>) {
                return;
            }
        } else if constexpr (!std::is_reference_v<Ret_> &&
                             std::same_as<std::remove_cvref_t<Ret_>, Rp_>) {
            return std::invoke(handler_);
        } else {
            return wrap_result_<Rp_>(std::invoke(handler_));
        }
    } else {
        static_assert(
            can_wrap_result_<Rp_, Hp_ &>,
            "match() arm handler value cannot be converted to match result");
        return wrap_result_<Rp_>(handler_);
    }
}

template <typename Rp_, typename... Arms_>
constexpr auto match_none_(Arms_ &...arms) -> Rp_ {
    constexpr bool has_explicit_ =
        (match_arm_is_explicit_none_<Arms_>() || ...);

    auto try_invoke_ = [&]<typename Arm_>(auto &&self_fn, Arm_ &arm,
                                          auto &...rest) -> Rp_ {
        using ArmD_ = std::remove_cvref_t<Arm_>;
        if constexpr (has_explicit_) {
            if constexpr (match_arm_is_explicit_none_<ArmD_>()) {
                return invoke_none_<Rp_>(arm.handler);
            } else if constexpr (sizeof...(rest) > 0) {
                return self_fn(self_fn, rest...);
            } else {
                std::unreachable();
            }
        } else {
            if constexpr (match_arm_is_wild_none_<ArmD_>()) {
                return invoke_none_<Rp_>(arm.handler);
            } else if constexpr (sizeof...(rest) > 0) {
                return self_fn(self_fn, rest...);
            } else {
                std::unreachable();
            }
        }
    };
    return try_invoke_(try_invoke_, arms...);
}

template <typename Rp_, typename CurrentType_, typename Value_,
          typename FirstArm_, typename... RestArms_>
constexpr auto match_some_(std::string_view type_name_, Value_ &&value_,
                           FirstArm_ &arm_, RestArms_ &...rest_) -> Rp_ {
    using ArmD_ = std::remove_cvref_t<FirstArm_>;
    using Pattern_ = typename ArmD_::pattern_type;
    using Handler_ = typename ArmD_::handler_type;

    constexpr bool arm_matches_this_type_ =
        pattern_matches_type_<Pattern_, CurrentType_>() &&
        handler_accepts_type_<Handler_, CurrentType_>;

    if constexpr (arm_matches_this_type_ &&
                  match_arm_is_catch_all_for_type_<FirstArm_, CurrentType_>()) {
        return invoke_some_<Rp_, Handler_, Value_>(
            arm_.handler, std::forward<Value_>(value_));
    } else {
        if constexpr (arm_matches_this_type_) {
            if (matches_value_(arm_.pattern,
                               static_cast<const CurrentType_ &>(value_))) {
                return invoke_some_<Rp_, Handler_, Value_>(
                    arm_.handler, std::forward<Value_>(value_));
            }
        }
        if constexpr (sizeof...(RestArms_) > 0) {
            return match_some_<Rp_, CurrentType_, Value_>(
                type_name_, std::forward<Value_>(value_), rest_...);
        } else {
            panic_("Option<{}>::match() failed - no matching pattern arm",
                   type_name_);
        }
    }
}

// Layer 3: copy assignment
template <typename Qp_, bool = (std::is_trivially_copy_assignable_v<Qp_> &&
                                std::is_trivially_copy_constructible_v<Qp_> &&
                                std::is_trivially_destructible_v<Qp_>)>
struct OptionalCopyAssign_ : OptionalMove_<Qp_> {
    using OptionalMove_<Qp_>::OptionalMove_;

  public:
    constexpr OptionalCopyAssign_(void) = default;

  public:
    constexpr OptionalCopyAssign_(const OptionalCopyAssign_ &) = default;

  public:
    constexpr OptionalCopyAssign_(OptionalCopyAssign_ &&) = default;

  public:
    constexpr auto operator=(OptionalCopyAssign_ &&)
        -> OptionalCopyAssign_ & = default;

  public:
    constexpr ~OptionalCopyAssign_(void) = default;

  public:
    constexpr auto operator=(const OptionalCopyAssign_ &other) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_> &&
        (!std::is_copy_assignable_v<Qp_> ||
         std::is_nothrow_copy_assignable_v<Qp_>) &&
        std::is_nothrow_destructible_v<Qp_>) -> OptionalCopyAssign_ &
        requires(std::is_copy_constructible_v<Qp_>)
    {
        if (this == std::addressof(other)) {
            return *this;
        }

        if (this->is_some) {
            if (other.is_some) {
                if constexpr (std::is_copy_assignable_v<Qp_>) {
                    this->storage.value = other.storage.value;
                }

                else {
                    this->reset_();
                    std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                          std::addressof(this->storage.value)),
                                      other.storage.value);
                    this->is_some = true;
                }
            }

            else {
                this->reset_();
            }
        }

        else {
            if (other.is_some) {
                std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                      std::addressof(this->storage.value)),
                                  other.storage.value);
                this->is_some = true;
            }
        }

        return *this;
    }

  public:
    constexpr auto operator=(const OptionalCopyAssign_ &)
        -> OptionalCopyAssign_ &
        requires(!std::is_copy_constructible_v<Qp_>)
    = delete;
};

template <typename Qp_>
struct OptionalCopyAssign_<Qp_, true> : OptionalMove_<Qp_> {
    using OptionalMove_<Qp_>::OptionalMove_;
};

// Layer 4: move assignment
template <typename Qp_, bool = (std::is_trivially_move_assignable_v<Qp_> &&
                                std::is_trivially_move_constructible_v<Qp_> &&
                                std::is_trivially_destructible_v<Qp_>)>
struct OptionalMoveAssign_ : OptionalCopyAssign_<Qp_> {
    using OptionalCopyAssign_<Qp_>::OptionalCopyAssign_;

  public:
    constexpr OptionalMoveAssign_(void) = default;

  public:
    constexpr OptionalMoveAssign_(const OptionalMoveAssign_ &) = default;

  public:
    constexpr OptionalMoveAssign_(OptionalMoveAssign_ &&) = default;

  public:
    constexpr auto operator=(const OptionalMoveAssign_ &)
        -> OptionalMoveAssign_ & = default;

  public:
    constexpr ~OptionalMoveAssign_(void) = default;

  public:
    constexpr auto operator=(OptionalMoveAssign_ &&other) noexcept(
        std::is_nothrow_move_constructible_v<Qp_> &&
        (!std::is_move_assignable_v<Qp_> ||
         std::is_nothrow_move_assignable_v<Qp_>) &&
        std::is_nothrow_destructible_v<Qp_>) -> OptionalMoveAssign_ &
        requires(std::is_move_constructible_v<Qp_>)
    {
        if (this == std::addressof(other)) {
            return *this;
        }

        if (this->is_some) {
            if (other.is_some) {
                if constexpr (std::is_move_assignable_v<Qp_>) {
                    this->storage.value =
                        (static_cast<Qp_ &&>(other.storage.value));
                }

                else {
                    this->reset_();
                    std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                          std::addressof(this->storage.value)),
                                      static_cast<Qp_ &&>(other.storage.value));
                    this->is_some = true;
                }
            }

            else {
                this->reset_();
            }
        }

        else {
            if (other.is_some) {
                std::construct_at(const_cast<std::remove_cv_t<Qp_> *>(
                                      std::addressof(this->storage.value)),
                                  static_cast<Qp_ &&>(other.storage.value));
                this->is_some = true;
            }
        }

        return *this;
    }

  public:
    constexpr auto operator=(OptionalMoveAssign_ &&) -> OptionalMoveAssign_ &
        requires(!std::is_move_constructible_v<Qp_>)
    = delete;
};

template <typename Qp_>
struct alignas(Qp_) OptionalMoveAssign_<Qp_, true> : OptionalCopyAssign_<Qp_> {
    using OptionalCopyAssign_<Qp_>::OptionalCopyAssign_;

  public:
    constexpr auto operator=(OptionalMoveAssign_ &&)
        -> OptionalMoveAssign_ & = default;
};

struct Empty_ {};

template <typename... Types_> struct first_default_constructible_ {
  private:
    static consteval auto find_() {
        if constexpr (sizeof...(Types_) == 0) {
            return std::type_identity<void>{};
        } else {
            constexpr std::size_t index_ = [] consteval -> std::size_t {
                std::size_t idx = 0;
                bool found = false;
                auto step = [&]<typename T_>() {
                    if (!found) {
                        if constexpr (std::is_default_constructible_v<T_>) {
                            found = true;
                        } else {
                            ++idx;
                        }
                    }
                };
                (step.template operator()<Types_>(), ...);
                return idx;
            }();
            if constexpr (index_ < sizeof...(Types_)) {
                return std::type_identity<Types_... [index_]> {};
            } else {
                return std::type_identity<void>{};
            }
        }
    }

  public:
    using type = typename decltype(find_())::type;
};

template <typename... Types_> struct unique_types_ {
  private:
    static consteval auto check_() -> bool {
        if constexpr (sizeof...(Types_) <= 1) {
            return true;
        } else {
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                return (
                    ([]<std::size_t J>() {
                        return (
                            (J == Is ||
                             !std::same_as<std::remove_cv_t<Types_...[J]>,
                                           std::remove_cv_t<Types_...[Is]>>) &&
                            ...);
                    }.template operator()<Is>()) &&
                    ...);
            }(std::make_index_sequence<sizeof...(Types_)>{});
        }
    }

  public:
    static constexpr bool value = check_();
};

template <typename Qp_>
class alignas(Qp_) [[nodiscard]] Optional_ : public OptionalMoveAssign_<Qp_> {
  private:
    using Base_ = OptionalMoveAssign_<Qp_>;

  public:
    using Base_::storage;

  public:
    using Base_::is_some;

  public:
    explicit constexpr Optional_(const typename Opt::Some<Qp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_>)
        requires(std::is_copy_constructible_v<Qp_>)
        : Base_{std::in_place, some.get()} {}

  public:
    explicit constexpr Optional_(typename Opt::Some<Qp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Qp_>)
        requires(std::is_move_constructible_v<Qp_>)
        : Base_{std::in_place, std::move(some).get()} {}

  public:
    explicit constexpr Optional_(typename Opt::None) noexcept(true) : Base_{} {}

  public:
    explicit constexpr Optional_(typename Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<Qp_>)
        requires(std::is_default_constructible_v<Qp_>)
        : Base_{std::in_place} {}

  public:
    constexpr Optional_(const Optional_ &) = default;

  public:
    constexpr Optional_(Optional_ &&) = default;

  public:
    constexpr ~Optional_(void) = default;

  public:
    constexpr auto operator=(const Optional_ &) -> Optional_ & = default;

  public:
    constexpr auto operator=(Optional_ &&) -> Optional_ & = default;
};

template <typename> struct __is_option_type_ : std::false_type {};

template <typename Up_, typename... Args_>
struct __is_option_type_<Opt::Option<Up_, Args_...>> : std::true_type {};

template <typename Up_>
concept __is_option_ = (__is_option_type_<std::remove_cvref_t<Up_>>::value);

template <typename Up_> struct __compare_three_way_result {
  public:
    using type = void;
};

template <typename Up_>
    requires(std::three_way_comparable<Up_>)
struct __compare_three_way_result<Up_> {
  public:
    using type = std::compare_three_way_result_t<Up_>;
};

template <typename Up_>
using __compare_three_way_result_t =
    typename __compare_three_way_result<Up_>::type;

template <typename... Types_> struct __compare_three_way_results {
    using type = void;
};

template <typename First_, typename... Rest_>
    requires(std::three_way_comparable<First_> &&
             (std::three_way_comparable<Rest_> && ...))
struct __compare_three_way_results<First_, Rest_...> {
    using type = std::common_comparison_category_t<
        std::compare_three_way_result_t<First_>,
        std::compare_three_way_result_t<Rest_>...>;
};

template <typename... Types_>
using __compare_three_way_results_t =
    typename __compare_three_way_results<Types_...>::type;

template <typename Up_, typename... Types_> struct __option_cmp_with_category;

template <typename Up_> struct __option_cmp_with_category<Up_> {
    using type = std::strong_ordering;
};

template <typename Up_, typename First_, typename... Rest_>
struct __option_cmp_with_category<Up_, First_, Rest_...> {
  private:
    using rest_cat = typename __option_cmp_with_category<Up_, Rest_...>::type;

  public:
    using type = std::conditional_t<
        std::three_way_comparable_with<First_, Up_>,
        std::common_comparison_category_t<
            std::compare_three_way_result_t<First_, Up_>, rest_cat>,
        rest_cat>;
};

template <typename Up_, typename... Types_>
using __option_cmp_with_category_t =
    typename __option_cmp_with_category<Up_, Types_...>::type;

template <typename List_, typename T_> struct __append_unique_;

template <typename... Ts_, typename T_>
struct __append_unique_<std::tuple<Ts_...>, T_> {
  public:
    using type = std::conditional_t<(std::same_as<T_, Ts_> || ...),
                                    std::tuple<Ts_...>, std::tuple<Ts_..., T_>>;
};

template <typename Acc_, typename... Rest_> struct __dedup_pack_impl_;

template <typename Acc_> struct __dedup_pack_impl_<Acc_> {
  public:
    using type = Acc_;
};

template <typename Acc_, typename First_, typename... Rest_>
struct __dedup_pack_impl_<Acc_, First_, Rest_...> {
  private:
    using CleanFirst_ = std::decay_t<First_>;

  private:
    using Unwrapped_ =
        typename std::conditional_t<is_some_pattern_<CleanFirst_>::value,
                                    is_some_pattern_<CleanFirst_>,
                                    std::type_identity<CleanFirst_>>::type;

  private:
    using NextAcc_ =
        std::conditional_t<std::same_as<Unwrapped_, Opt::None>, Acc_,
                           typename __append_unique_<Acc_, Unwrapped_>::type>;

  public:
    using type = typename __dedup_pack_impl_<NextAcc_, Rest_...>::type;
};

template <typename... Ts_>
using __dedup_tuple_t = typename __dedup_pack_impl_<std::tuple<>, Ts_...>::type;

template <typename Tuple_> struct __tuple_to_option_;

template <> struct __tuple_to_option_<std::tuple<>> {
  public:
    using type = Opt::None;
};

template <typename First_, typename... Rest_>
struct __tuple_to_option_<std::tuple<First_, Rest_...>> {
  public:
    using type = Opt::Option<First_, Rest_...>;
};

template <typename... Rs_>
concept __has_common_type_ =
    (!(std::same_as<std::remove_cvref_t<Rs_>, Opt::None> || ...)) &&
    (!(is_some_pattern_<std::remove_cvref_t<Rs_>>::value || ...)) &&
    requires { typename std::common_type_t<Rs_...>; };

template <typename Up_, typename... Types_> struct match_alternative_ {
  private:
    template <typename T_>
    static constexpr bool is_exact_ =
        std::same_as<std::remove_cvref_t<Up_>, T_> &&
        std::is_constructible_v<T_, Up_>;

  private:
    static constexpr std::size_t exact_count_ =
        ((is_exact_<Types_> ? 1 : 0) + ...);

  private:
    template <typename T_>
    static constexpr bool is_conv_ = std::is_constructible_v<T_, Up_>;

  private:
    static constexpr std::size_t conv_count_ =
        ((is_conv_<Types_> ? 1 : 0) + ...);

  private:
    template <typename First_, typename... Rest_>
    static constexpr auto find_exact_(void) {
        if constexpr (is_exact_<First_>) {
            return std::type_identity<First_>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return find_exact_<Rest_...>();
        }
    }

  private:
    template <typename First_, typename... Rest_>
    static constexpr auto find_conv_(void) {
        if constexpr (is_conv_<First_>) {
            return std::type_identity<First_>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return find_conv_<Rest_...>();
        }
    }

  public:
    static constexpr bool valid =
        ((exact_count_ == 1) || (exact_count_ == 0 && conv_count_ == 1));

  public:
    using type = typename decltype([] {
        if constexpr (exact_count_ == 1) {
            return find_exact_<Types_...>();
        }

        else if constexpr (exact_count_ == 0 && conv_count_ == 1) {
            return find_conv_<Types_...>();
        }

        else {
            return std::type_identity<void>{};
        }
    }())::type;
};

template <typename Handler_>
consteval auto __deduce_nullary_handler_result_(void) {
    if constexpr (std::invocable<Handler_ &>) {
        return std::type_identity<std::invoke_result_t<Handler_ &>>{};
    } else {
        return std::type_identity<std::decay_t<Handler_>>{};
    }
}

template <typename Handler_, typename Argument_>
consteval auto __deduce_handler_result_(void) {
    if constexpr (std::invocable<Handler_ &, Argument_>) {
        return std::type_identity<
            std::invoke_result_t<Handler_ &, Argument_>>{};
    } else {
        return __deduce_nullary_handler_result_<Handler_>();
    }
}

template <typename... Tuples_> struct __tuple_cat_types_;

template <typename... Types_> struct __tuple_cat_types_<std::tuple<Types_...>> {
    using type = std::tuple<Types_...>;
};

template <typename... First_, typename... Second_, typename... Rest_>
struct __tuple_cat_types_<std::tuple<First_...>, std::tuple<Second_...>,
                          Rest_...>
    : __tuple_cat_types_<std::tuple<First_..., Second_...>, Rest_...> {};

template <
    typename Pattern_, typename Handler_, typename Alternative_,
    bool Matches_ =
        (pattern_matches_type_<Pattern_, std::remove_cvref_t<Alternative_>>() &&
         handler_accepts_type_<Handler_, std::remove_cvref_t<Alternative_>>)>
struct __deduce_arm_alt_result_ {
    using type = std::tuple<>;
};

template <typename Pattern_, typename Handler_, typename Alternative_>
struct __deduce_arm_alt_result_<Pattern_, Handler_, Alternative_, true> {
    using Result_ =
        typename decltype(__deduce_handler_result_<Handler_,
                                                   Alternative_>())::type;
    using type = std::tuple<Result_>;
};

template <typename Arm_, typename Alternatives_,
          bool IsArm_ =
              requires {
                  typename std::remove_cvref_t<Arm_>::pattern_type;
                  typename std::remove_cvref_t<Arm_>::handler_type;
              }>
struct __deduce_arm_results_ {
    using type = std::tuple<>;
};

template <typename Arm_, typename... Alternatives_>
struct __deduce_arm_results_<Arm_, std::tuple<Alternatives_...>, true> {
  private:
    using ArmD_ = std::remove_cvref_t<Arm_>;
    using Pattern_ = typename ArmD_::pattern_type;
    using Handler_ = typename ArmD_::handler_type;

  public:
    using type = typename __tuple_cat_types_<typename __deduce_arm_alt_result_<
        Pattern_, Handler_, Alternatives_>::type...>::type;
};

template <typename Alternatives_, typename Arms_>
struct __collect_some_match_results_;

template <typename... Alternatives_>
struct __collect_some_match_results_<std::tuple<Alternatives_...>,
                                     std::tuple<>> {
    using type = std::tuple<>;
};

template <typename Alternatives_, typename Arm_, typename Remaining_,
          bool Stop_>
struct __collect_some_match_results_step_;

template <typename Alternatives_, typename Arm_, typename... Rest_>
struct __collect_some_match_results_step_<Alternatives_, Arm_,
                                          std::tuple<Rest_...>, true> {
    using type = typename __deduce_arm_results_<Arm_, Alternatives_>::type;
};

template <typename Alternatives_, typename Arm_, typename... Rest_>
struct __collect_some_match_results_step_<Alternatives_, Arm_,
                                          std::tuple<Rest_...>, false> {
    using Current_ = typename __deduce_arm_results_<Arm_, Alternatives_>::type;
    using Remaining_ =
        typename __collect_some_match_results_<Alternatives_,
                                               std::tuple<Rest_...>>::type;
    using type = typename __tuple_cat_types_<Current_, Remaining_>::type;
};

template <typename... Alternatives_, typename First_, typename... Rest_>
struct __collect_some_match_results_<std::tuple<Alternatives_...>,
                                     std::tuple<First_, Rest_...>>
    : __collect_some_match_results_step_<
          std::tuple<Alternatives_...>, First_, std::tuple<Rest_...>,
          match_arm_is_universal_catch_all_<
              First_, std::tuple<Alternatives_...>>::value> {};

template <typename Tuple_> struct __find_explicit_none_arm_;
template <> struct __find_explicit_none_arm_<std::tuple<>> {
    using type = void;
};
template <typename First_, typename... Rest_>
struct __find_explicit_none_arm_<std::tuple<First_, Rest_...>> {
    using type = std::conditional_t<
        match_arm_is_explicit_none_<First_>(), First_,
        typename __find_explicit_none_arm_<std::tuple<Rest_...>>::type>;
};

template <typename Tuple_> struct __find_wild_none_arm_;
template <> struct __find_wild_none_arm_<std::tuple<>> {
    using type = void;
};
template <typename First_, typename... Rest_>
struct __find_wild_none_arm_<std::tuple<First_, Rest_...>> {
    using type = std::conditional_t<
        match_arm_is_wild_none_<First_>(), First_,
        typename __find_wild_none_arm_<std::tuple<Rest_...>>::type>;
};

template <typename Arms_> struct __collect_none_match_result_;

template <typename... Arms_>
struct __collect_none_match_result_<std::tuple<Arms_...>> {
  private:
    static constexpr bool has_explicit_ =
        (match_arm_is_explicit_none_<Arms_>() || ...);
    using ExplicitArm_ =
        typename __find_explicit_none_arm_<std::tuple<Arms_...>>::type;
    using WildArm_ = typename __find_wild_none_arm_<std::tuple<Arms_...>>::type;
    using SelectedArm_ =
        std::conditional_t<has_explicit_, ExplicitArm_, WildArm_>;

    static consteval auto deduce_() {
        if constexpr (!std::is_void_v<SelectedArm_>) {
            using Handler_ =
                typename std::remove_cvref_t<SelectedArm_>::handler_type;
            using Res_ = typename decltype(__deduce_nullary_handler_result_<
                                           Handler_>())::type;
            return std::type_identity<std::tuple<Res_>>{};
        } else {
            return std::type_identity<std::tuple<>>{};
        }
    }

  public:
    using type = typename decltype(deduce_())::type;
};

template <typename ResultsTuple_> struct __match_results_are_consistent_;
template <typename... Results_>
struct __match_results_are_consistent_<std::tuple<Results_...>>
    : std::bool_constant<(std::is_void_v<Results_> && ...) ||
                         !(std::is_void_v<Results_> || ...)> {
    static constexpr bool has_void = (std::is_void_v<Results_> || ...);
};

template <typename AlternativesTuple_, typename ArmsTuple_>
struct __match_return_types_are_consistent_;
template <typename... Alternatives_, typename... Arms_>
struct __match_return_types_are_consistent_<std::tuple<Alternatives_...>,
                                            std::tuple<Arms_...>> {
  private:
    using NoneResults_ =
        typename __collect_none_match_result_<std::tuple<Arms_...>>::type;
    using SomeResults_ =
        typename __collect_some_match_results_<std::tuple<Alternatives_...>,
                                               std::tuple<Arms_...>>::type;
    using AllResults_ =
        typename __tuple_cat_types_<NoneResults_, SomeResults_>::type;

  public:
    using type = AllResults_;
    using ResultProperties_ = __match_results_are_consistent_<AllResults_>;
    static constexpr bool value = ResultProperties_::value;
    static constexpr bool has_void = ResultProperties_::has_void;
};

template <typename T_> struct __wrap_in_option_ {
  public:
    using type = Opt::Option<T_>;
};

template <typename T_>
    requires(details::__is_option_<T_>)
struct __wrap_in_option_<T_> {
  public:
    using type = T_;
};

template <> struct __wrap_in_option_<void> {
  public:
    using type = void;
};

template <> struct __wrap_in_option_<Opt::None> {
  public:
    using type = Opt::None;
};

template <typename T_>
using __wrap_in_option_t = typename __wrap_in_option_<T_>::type;

template <typename ReturnType_, typename ResultsTuple_>
struct __match_results_convertible_to_;
template <typename ReturnType_, typename... Results_>
struct __match_results_convertible_to_<ReturnType_, std::tuple<Results_...>>
    : std::bool_constant<(can_wrap_result_<ReturnType_, Results_> && ...)> {};

template <typename ReturnTypesTuple_, typename ResultsTuple_>
struct __match_explicit_results_convertible_ : std::true_type {};
template <typename ReturnType_, typename ResultsTuple_>
struct __match_explicit_results_convertible_<std::tuple<ReturnType_>,
                                             ResultsTuple_>
    : __match_results_convertible_to_<__wrap_in_option_t<ReturnType_>,
                                      ResultsTuple_> {};
template <typename First_, typename Second_, typename... Rest_,
          typename ResultsTuple_>
struct __match_explicit_results_convertible_<
    std::tuple<First_, Second_, Rest_...>, ResultsTuple_>
    : __match_results_convertible_to_<Opt::Option<First_, Second_, Rest_...>,
                                      ResultsTuple_> {};

template <typename Tuple_> struct __tuple_to_wrapped_option_;

template <> struct __tuple_to_wrapped_option_<std::tuple<>> {
  public:
    using type = Opt::None;
};

template <typename First_>
struct __tuple_to_wrapped_option_<std::tuple<First_>> {
  public:
    using type = __wrap_in_option_t<First_>;
};

template <typename First_, typename... Rest_>
struct __tuple_to_wrapped_option_<std::tuple<First_, Rest_...>> {
  private:
    template <typename T_, typename... Us_>
    static constexpr auto deduce_(void) {
        if constexpr (requires { typename std::common_type_t<T_, Us_...>; }) {
            return (std::type_identity<
                    __wrap_in_option_t<std::common_type_t<T_, Us_...>>>{});
        }

        else {
            return std::type_identity<Opt::Option<T_, Us_...>>{};
        }
    }

  public:
    using type = typename decltype(deduce_<First_, Rest_...>())::type;
};

template <typename... Rs_> struct __deduce_res_from_types_ {
  private:
    static constexpr bool all_void_ = (std::is_void_v<Rs_> && ...);
    static constexpr bool any_void_ = (std::is_void_v<Rs_> || ...);

    static consteval auto deduce_(void) {
        if constexpr (all_void_) {
            return std::type_identity<void>{};
        } else if constexpr (any_void_) {
            static_assert(!any_void_,
                          "match(): handlers must return either all void "
                          "or all non-void values");
            return std::type_identity<void>{};
        } else {
            return std::type_identity<typename __tuple_to_wrapped_option_<
                __dedup_tuple_t<Rs_...>>::type>{};
        }
    }

  public:
    using type = typename decltype(deduce_())::type;
};

template <typename AltsTuple_, typename ArmsTuple_> struct __deduce_match_res_;

template <typename... ForwardAlts_, typename... Arms_>
struct __deduce_match_res_<std::tuple<ForwardAlts_...>, std::tuple<Arms_...>> {
  private:
    using NoneResults_ =
        typename __collect_none_match_result_<std::tuple<Arms_...>>::type;
    using SomeResults_ =
        typename __collect_some_match_results_<std::tuple<ForwardAlts_...>,
                                               std::tuple<Arms_...>>::type;
    using AllResults_ =
        typename __tuple_cat_types_<NoneResults_, SomeResults_>::type;

  public:
    template <typename Tuple_> struct results_to_type_;
    template <typename... Rs_>
    struct results_to_type_<std::tuple<Rs_...>>
        : __deduce_res_from_types_<Rs_...> {};

    using type = typename results_to_type_<AllResults_>::type;
};

template <typename ExplicitTuple_, typename AltsTuple_, typename... Arms_>
struct __match_unwrapped_return_type_;

template <typename AltsTuple_, typename... Arms_>
struct __match_unwrapped_return_type_<std::tuple<>, AltsTuple_, Arms_...> {
  public:
    using type =
        typename __deduce_match_res_<AltsTuple_, std::tuple<Arms_...>>::type;
};

template <typename Single_, typename AltsTuple_, typename... Arms_>
struct __match_unwrapped_return_type_<std::tuple<Single_>, AltsTuple_,
                                      Arms_...> {
  public:
    using type = __wrap_in_option_t<Single_>;
};

template <typename R1_, typename R2_, typename... Rest_, typename AltsTuple_,
          typename... Arms_>
struct __match_unwrapped_return_type_<std::tuple<R1_, R2_, Rest_...>,
                                      AltsTuple_, Arms_...> {
    static_assert((!std::is_void_v<R1_> && !std::is_void_v<R2_> &&
                   (!std::is_void_v<Rest_> && ...)),
                  "match_unwrapped: all handlers must return "
                  "either void or non-void");

  public:
    using type = Opt::Option<R1_, R2_, Rest_...>;
};

template <typename AlternativesTuple_, typename ArmsTuple_,
          bool Consistent_ = __match_return_types_are_consistent_<
              AlternativesTuple_, ArmsTuple_>::value>
struct __match_inferred_results_convertible_ : std::false_type {};

template <typename... Alternatives_, typename... Arms_>
struct __match_inferred_results_convertible_<std::tuple<Alternatives_...>,
                                             std::tuple<Arms_...>, true> {
  private:
    using ResultTypes_ = typename __match_return_types_are_consistent_<
        std::tuple<Alternatives_...>, std::tuple<Arms_...>>::type;
    using ReturnType_ = typename __match_unwrapped_return_type_<
        std::tuple<>, std::tuple<Alternatives_...>, Arms_...>::type;

  public:
    static constexpr bool value =
        __match_results_convertible_to_<ReturnType_, ResultTypes_>::value;
};

// -----------------------------------------------------------------------------
// Active-Alternative Mapping & Combinator Metaprogramming
// -----------------------------------------------------------------------------

// C++26 Pack Indexing (P2662R3)
template <std::size_t Index_, typename... Ts_>
    requires(Index_ < sizeof...(Ts_))
struct __pack_element_impl_ {
    using type = Ts_...[Index_];
};

template <std::size_t Index_, typename... Ts_>
using __pack_element_t = typename __pack_element_impl_<Index_, Ts_...>::type;

// The callable type F_ is the actual forwarded call category: for a
// temporary argument it is F, and for an lvalue argument it is F&.
// Never prefer F_& here: doing so can infer a different overload from the one
// that std::forward<F_>(op) invokes.
template <typename F_, typename Arg_, bool = std::is_invocable_v<F_, Arg_>>
struct __map_invoke_result_materializable_ : std::false_type {};

template <typename F_, typename Arg_>
struct __map_invoke_result_materializable_<F_, Arg_, true> {
  private:
    using R_ = std::invoke_result_t<F_, Arg_>;

  public:
    static constexpr bool value =
        !std::is_void_v<R_> &&
        std::is_constructible_v<std::remove_cvref_t<R_>, R_>;
};

template <typename F_, typename Arg_>
concept __map_invocable_non_void_ =
    __map_invoke_result_materializable_<F_, Arg_>::value;

template <typename F_, bool = std::is_invocable_v<F_>>
struct __nullary_invoke_result_materializable_ : std::false_type {};

template <typename F_>
struct __nullary_invoke_result_materializable_<F_, true> {
  private:
    using R_ = std::invoke_result_t<F_>;

  public:
    static constexpr bool value =
        !std::is_void_v<R_> &&
        std::is_constructible_v<std::remove_cvref_t<R_>, R_>;
};

template <typename F_, typename Arg_,
          bool = __map_invocable_non_void_<F_, Arg_>>
struct __map_single_result_ {
    using type = void;
};

template <typename F_, typename Arg_>
struct __map_single_result_<F_, Arg_, true> {
    using type = std::remove_cvref_t<std::invoke_result_t<F_, Arg_>>;
};

template <typename F_, typename Arg_>
using __map_single_result_t = typename __map_single_result_<F_, Arg_>::type;

template <typename AccTuple_, typename F_, typename... Args_>
struct __collect_map_results_;

template <typename AccTuple_, typename F_>
struct __collect_map_results_<AccTuple_, F_> {
    using type = AccTuple_;
};

template <typename AccTuple_, typename F_, typename FirstArg_,
          typename... RestArgs_>
struct __collect_map_results_<AccTuple_, F_, FirstArg_, RestArgs_...> {
  private:
    using SingleRes_ = __map_single_result_t<F_, FirstArg_>;
    using NextAcc_ = std::conditional_t<
        __map_invocable_non_void_<F_, FirstArg_>,
        typename __append_unique_<AccTuple_, SingleRes_>::type, AccTuple_>;

  public:
    using type =
        typename __collect_map_results_<NextAcc_, F_, RestArgs_...>::type;
};

template <typename F_, typename... Args_>
using __collect_map_results_t =
    typename __collect_map_results_<std::tuple<>, F_, Args_...>::type;

template <typename F_, typename... Args_>
concept __all_mappable_alternatives_ =
    (__map_invocable_non_void_<F_, Args_> && ...);

template <typename F_, typename... Args_>
using __map_option_return_type_t =
    typename __tuple_to_option_<__collect_map_results_t<F_, Args_...>>::type;

template <typename TargetResult_, typename F_, typename Arg_>
concept __map_alt_invocable_and_constructible_ =
    __map_invocable_non_void_<F_, Arg_> &&
    std::is_constructible_v<TargetResult_, __map_single_result_t<F_, Arg_>>;

template <typename TargetResult_, typename F_, typename... Args_>
concept __all_map_or_valid_ =
    (__map_alt_invocable_and_constructible_<TargetResult_, F_, Args_> && ...);

template <typename F_, typename Arg_>
concept __predicate_invocable_ = requires(F_ &&f, Arg_ &&arg) {
    {
        std::invoke(static_cast<F_ &&>(f), static_cast<Arg_ &&>(arg))
    } -> std::convertible_to<bool>;
};

template <typename F_, typename... Args_>
concept __all_predicate_invocable_ = (__predicate_invocable_<F_, Args_> && ...);

template <typename Alternative_, typename Value_>
concept option_value_comparable_ =
    std::constructible_from<Alternative_, Value_> &&
    std::equality_comparable_with<Alternative_, Value_>;

template <typename> struct __option_types_tuple_ {
    using type = std::tuple<>;
};

template <typename Up_, typename... Args_>
struct __option_types_tuple_<Opt::Option<Up_, Args_...>> {
    using type = std::tuple<Up_, Args_...>;
};

template <typename Tuple1_, typename Tuple2_> struct __tuple_cat_unique_;

template <typename Tuple1_> struct __tuple_cat_unique_<Tuple1_, std::tuple<>> {
    using type = Tuple1_;
};

template <typename Tuple1_, typename First_, typename... Rest_>
struct __tuple_cat_unique_<Tuple1_, std::tuple<First_, Rest_...>> {
    using type = typename __tuple_cat_unique_<
        typename __append_unique_<Tuple1_, First_>::type,
        std::tuple<Rest_...>>::type;
};

// The returned Option must be materializable from the actual invoke result.
// F_ is the forwarded callable category used by the implementation.
template <typename F_, typename Arg_, bool = std::is_invocable_v<F_, Arg_>>
struct __and_then_invocable_impl_ : std::false_type {};

template <typename F_, typename Arg_>
struct __and_then_invocable_impl_<F_, Arg_, true> {
  private:
    using InvokeResult_ = std::invoke_result_t<F_, Arg_>;
    using ResultOption_ = std::remove_cvref_t<InvokeResult_>;

  public:
    static constexpr bool value =
        details::__is_option_<ResultOption_> &&
        std::is_constructible_v<ResultOption_, InvokeResult_>;
};

template <typename F_, typename Arg_>
concept __and_then_invocable_ = __and_then_invocable_impl_<F_, Arg_>::value;

template <typename F_, typename Arg_, bool = __and_then_invocable_<F_, Arg_>>
struct __and_then_single_types_ {
    using type = std::tuple<>;
};

template <typename F_, typename Arg_>
struct __and_then_single_types_<F_, Arg_, true> {
  private:
    using InvocableRes_ = std::remove_cvref_t<std::invoke_result_t<F_, Arg_>>;

  public:
    using type = typename __option_types_tuple_<InvocableRes_>::type;
};

template <typename AccTuple_, typename F_, typename... Args_>
struct __collect_and_then_results_;

template <typename AccTuple_, typename F_>
struct __collect_and_then_results_<AccTuple_, F_> {
    using type = AccTuple_;
};

template <typename AccTuple_, typename F_, typename FirstArg_,
          typename... RestArgs_>
struct __collect_and_then_results_<AccTuple_, F_, FirstArg_, RestArgs_...> {
  private:
    using FirstTypesTuple_ =
        typename __and_then_single_types_<F_, FirstArg_>::type;
    using NextAcc_ =
        typename __tuple_cat_unique_<AccTuple_, FirstTypesTuple_>::type;

  public:
    using type =
        typename __collect_and_then_results_<NextAcc_, F_, RestArgs_...>::type;
};

template <typename F_, typename... Args_>
using __collect_and_then_results_t =
    typename __collect_and_then_results_<std::tuple<>, F_, Args_...>::type;

template <typename F_, typename... Args_>
concept __all_and_then_alternatives_ =
    (__and_then_invocable_<F_, Args_> && ...);

template <typename F_, typename... Args_>
using __and_then_return_type_t = typename __tuple_to_option_<
    __collect_and_then_results_t<F_, Args_...>>::type;

}; /* namespace details */

namespace details {
template <typename... Types_> union Alternative_;

template <> union Alternative_<> {
  public:
    std::byte layout_[1];

  public:
    constexpr Alternative_(void) noexcept(true) : layout_{} {}

  public:
    constexpr Alternative_(const Alternative_ &) = default;

  public:
    constexpr Alternative_(Alternative_ &&) = default;

  public:
    constexpr auto operator=(const Alternative_ &) -> Alternative_ & = default;

  public:
    constexpr auto operator=(Alternative_ &&) -> Alternative_ & = default;

  public:
    constexpr ~Alternative_(void) noexcept(true) = default;
};

template <typename First_, typename... Rest_>
union alignas(max_alignof_<First_, Rest_...>()) Alternative_<First_, Rest_...> {
  private:
    using FirstType_ = std::remove_cv_t<First_>;

  private:
    using RestType_ = Alternative_<Rest_...>;

  private:
    static constexpr std::size_t _alignment_{max_alignof_<First_, Rest_...>()};

  private:
    static constexpr std::size_t _storage_size_{
        align_up_((sizeof(FirstType_) > sizeof(RestType_)) ? sizeof(FirstType_)
                                                           : sizeof(RestType_),
                  _alignment_)};

  private:
    static constexpr bool _trivial_dtor_{
        std::is_trivially_destructible_v<FirstType_> &&
        std::is_trivially_destructible_v<RestType_>};

  private:
    static constexpr bool _trivial_copy_{
        _trivial_dtor_ && std::is_trivially_copy_constructible_v<FirstType_> &&
        std::is_trivially_copy_constructible_v<RestType_>};

  private:
    static constexpr bool _trivial_move_{
        _trivial_dtor_ && std::is_trivially_move_constructible_v<FirstType_> &&
        std::is_trivially_move_constructible_v<RestType_>};

  private:
    static constexpr bool _trivial_copy_assign_{
        _trivial_copy_ && std::is_trivially_copy_assignable_v<FirstType_> &&
        std::is_trivially_copy_assignable_v<RestType_>};

  private:
    static constexpr bool _trivial_move_assign_{
        _trivial_move_ && std::is_trivially_move_assignable_v<FirstType_> &&
        std::is_trivially_move_assignable_v<RestType_>};

  public:
    FirstType_ first_;

  public:
    RestType_ rest_;

  public:
    std::byte layout_[_storage_size_];

  public:
    constexpr Alternative_(void) noexcept(true) : layout_{} {}

  public:
    constexpr Alternative_(const Alternative_ &)
        requires(_trivial_copy_)
    = default;

  public:
    constexpr Alternative_(Alternative_ &&)
        requires(_trivial_move_)
    = default;

  public:
    constexpr auto operator=(const Alternative_ &) -> Alternative_ &
        requires(_trivial_copy_assign_)
    = default;

  public:
    constexpr auto operator=(Alternative_ &&) -> Alternative_ &
        requires(_trivial_move_assign_)
    = default;

  public:
    constexpr ~Alternative_(void) noexcept(true)
        requires(_trivial_dtor_)
    = default;

  public:
    constexpr ~Alternative_(void) noexcept(true)
        requires(!_trivial_dtor_)
    {}
};

}; /* namespace details */

namespace details {
template <typename Tp_, typename... Args_>
class alignas(
    max_alignof_<Tp_, Args_...,
                 typename option_index_type_<sizeof...(Args_) + 2>::type>())
    OptionStorage_ {
  private:
    using Index_ = typename option_index_type_<sizeof...(Args_) + 2>::type;

  private:
    static constexpr Index_ destroying_index_{
        static_cast<Index_>(sizeof...(Args_) + 2)};

  private:
    using Alternative_ = details::Alternative_<Args_...>;

  private:
    static constexpr std::size_t _alignment_{
        max_alignof_<Tp_, Args_..., Index_>()};

  private:
    static constexpr std::size_t _payload_size_{
        align_up_((sizeof(Tp_) > sizeof(Alternative_)) ? sizeof(Tp_)
                                                       : sizeof(Alternative_),
                  _alignment_)};

  private:
    static constexpr std::size_t _padding_size_{
        (_alignment_ - ((_payload_size_ + sizeof(Index_)) % _alignment_)) %
        _alignment_};

  private:
    static constexpr bool _trivial_dtor_{
        std::is_trivially_destructible_v<Tp_> &&
        (std::is_trivially_destructible_v<Args_> && ...)};

  private:
    static constexpr bool _trivial_copy_{
        _trivial_dtor_ && std::is_trivially_copy_constructible_v<Tp_> &&
        (std::is_trivially_copy_constructible_v<Args_> && ...)};

  private:
    static constexpr bool _trivial_move_{
        _trivial_dtor_ && std::is_trivially_move_constructible_v<Tp_> &&
        (std::is_trivially_move_constructible_v<Args_> && ...)};

  private:
    static constexpr bool _trivial_copy_assign_{
        _trivial_copy_ && std::is_trivially_copy_assignable_v<Tp_> &&
        (std::is_trivially_copy_assignable_v<Args_> && ...)};

  private:
    static constexpr bool _trivial_move_assign_{
        _trivial_move_ && std::is_trivially_move_assignable_v<Tp_> &&
        (std::is_trivially_move_assignable_v<Args_> && ...)};

  private:
    union alignas(_alignment_) Storage_ {
      public:
        std::remove_cv_t<Tp_> value;

      public:
        Alternative_ alternatives;

      public:
        std::byte layout_[_payload_size_];

      public:
        constexpr Storage_(void) noexcept(true) : layout_{} {}

      public:
        constexpr Storage_(const Storage_ &)
            requires(_trivial_copy_)
        = default;

      public:
        constexpr Storage_(Storage_ &&)
            requires(_trivial_move_)
        = default;

      public:
        constexpr auto operator=(const Storage_ &) -> Storage_ &
            requires(_trivial_copy_assign_)
        = default;

      public:
        constexpr auto operator=(Storage_ &&) -> Storage_ &
            requires(_trivial_move_assign_)
        = default;

      public:
        constexpr ~Storage_(void) noexcept(true)
            requires(_trivial_dtor_)
        = default;

      public:
        constexpr ~Storage_(void) noexcept(true)
            requires(!_trivial_dtor_)
        {}
    };

  private:
    struct alignas(alignof(Index_)) State_ {
      public:
        Index_ index_;

      public:
        constexpr operator bool(void) const noexcept(true) {
            return (this->index_ != static_cast<Index_>(0) &&
                    this->index_ != destroying_index_);
        }

      public:
        constexpr auto operator=(bool value_) noexcept(true) -> State_ & {
            this->index_ = static_cast<Index_>(value_ ? 1 : 0);
            return *this;
        }
    };

  public:
    Storage_ storage;

  public:
    State_ is_some;

  public:
    [[no_unique_address]] Pad_<_padding_size_> _padding_{};

  private:
    template <typename Qp_>
    static constexpr auto index_of_(void) noexcept(true) -> Index_ {
        if constexpr (std::same_as<Qp_, Tp_>) {
            return static_cast<Index_>(1);
        }

        else {
            return static_cast<Index_>(2 + type_index_<Qp_, Args_...>::value);
        }
    }

  private:
    template <typename Qp_, typename First_, typename... Rest_>
    static constexpr auto
    alternative_ptr_(details::Alternative_<First_, Rest_...> *alternative_
                     [[RL_LIFETIME_BOUND]]) noexcept(true)
        -> std::remove_cv_t<Qp_> * {
        if constexpr (std::same_as<std::remove_cv_t<Qp_>,
                                   std::remove_cv_t<First_>>) {
            return std::addressof(alternative_->first_);
        }

        else {
            return alternative_ptr_<Qp_>(std::addressof(alternative_->rest_));
        }
    }

  private:
    template <typename Qp_, typename First_, typename... Rest_>
    static constexpr auto
    alternative_ptr_(const details::Alternative_<First_, Rest_...> *alternative_
                     [[RL_LIFETIME_BOUND]]) noexcept(true)
        -> const std::remove_cv_t<Qp_> * {
        if constexpr (std::same_as<std::remove_cv_t<Qp_>,
                                   std::remove_cv_t<First_>>) {
            return std::addressof(alternative_->first_);
        }

        else {
            return alternative_ptr_<Qp_>(std::addressof(alternative_->rest_));
        }
    }

  private:
    template <typename Qp_, typename First_, typename... Rest_,
              typename... CtorArgs_>
    static constexpr auto construct_alternative_(
        details::Alternative_<First_, Rest_...> &alternative_
        [[RL_LIFETIME_BOUND]],
        CtorArgs_
            &&...args) noexcept(std::
                                    is_nothrow_constructible_v<
                                        std::remove_cv_t<Qp_>, CtorArgs_...>)
        -> std::remove_cv_t<Qp_> & {
        if constexpr (std::same_as<std::remove_cv_t<Qp_>,
                                   std::remove_cv_t<First_>>) {
            std::construct_at(std::addressof(alternative_.first_),
                              std::forward<CtorArgs_>(args)...);

            return alternative_.first_;
        }

        else {
            std::construct_at(std::addressof(alternative_.rest_));

            return (construct_alternative_<Qp_>(
                alternative_.rest_, std::forward<CtorArgs_>(args)...));
        }
    }

  private:
    template <typename Qp_, typename First_, typename... Rest_>
    static constexpr auto destroy_alternative_(
        details::Alternative_<First_, Rest_...> &alternative_) noexcept(true)
        -> void {
        if constexpr (std::same_as<std::remove_cv_t<Qp_>,
                                   std::remove_cv_t<First_>>) {
            std::destroy_at(std::addressof(alternative_.first_));
        } else {
            destroy_alternative_<Qp_>(alternative_.rest_);
        }

        // Every recursively selected `rest_` is itself a live union object.
        // End its lifetime after its active child has been destroyed.
        std::destroy_at(std::addressof(alternative_));
    }

  private:
    // A leaf constructor can throw after the recursive union wrappers have
    // been started, but before the leaf's lifetime begins. Roll back just the
    // wrappers; there is no live leaf object to destroy on this path.
    template <typename Qp_, typename First_, typename... Rest_>
    static constexpr auto destroy_partial_alternative_(
        details::Alternative_<First_, Rest_...> &alternative_) noexcept(true)
        -> void {
        if constexpr (!std::same_as<std::remove_cv_t<Qp_>,
                                    std::remove_cv_t<First_>>) {
            destroy_partial_alternative_<Qp_>(alternative_.rest_);
        }

        std::destroy_at(std::addressof(alternative_));
    }

  private:
    template <typename Qp_, typename... CtorArgs_>
    constexpr auto construct_(CtorArgs_ &&...args) & noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, CtorArgs_...>)
        [[RL_LIFETIME_BOUND]] -> Qp_ & {
        this->ensure_not_destroying_();
        if constexpr (std::same_as<Qp_, Tp_>) {
            auto *ptr_ = std::addressof(this->storage.value);
            std::construct_at(const_cast<std::remove_cv_t<Tp_> *>(ptr_),
                              std::forward<CtorArgs_>(args)...);

            this->is_some.index_ = this->index_of_<Qp_>();
            return static_cast<Qp_ &>(*ptr_);
        }

        else {
            auto *alternatives_ =
                std::construct_at(std::addressof(this->storage.alternatives));

#if defined(__cpp_exceptions)
            if constexpr (std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>,
                                                          CtorArgs_...>) {
                auto &value_ = construct_alternative_<Qp_>(
                    *alternatives_, std::forward<CtorArgs_>(args)...);

                this->is_some.index_ = this->index_of_<Qp_>();
                return static_cast<Qp_ &>(value_);
            } else {
                try {
                    auto &value_ = construct_alternative_<Qp_>(
                        *alternatives_, std::forward<CtorArgs_>(args)...);

                    this->is_some.index_ = this->index_of_<Qp_>();
                    return static_cast<Qp_ &>(value_);
                } catch (...) {
                    destroy_partial_alternative_<Qp_>(*alternatives_);
                    throw;
                }
            }
#else
            auto &value_ = construct_alternative_<Qp_>(
                *alternatives_, std::forward<CtorArgs_>(args)...);

            this->is_some.index_ = this->index_of_<Qp_>();
            return static_cast<Qp_ &>(value_);
#endif
        }
    }

  private:
    template <typename Qp_>
    constexpr auto get_(void) & noexcept(true) [[RL_LIFETIME_BOUND]] -> Qp_ & {
        if constexpr (std::same_as<Qp_, Tp_>) {
            return static_cast<Qp_ &>(this->storage.value);
        }

        else {
            return (static_cast<Qp_ &>(*alternative_ptr_<Qp_>(
                std::addressof(this->storage.alternatives))));
        }
    }

  private:
    template <typename Qp_>
    constexpr auto get_(void) const & noexcept(true) [[RL_LIFETIME_BOUND]]
    -> const Qp_ & {
        if constexpr (std::same_as<Qp_, Tp_>) {
            return static_cast<const Qp_ &>(this->storage.value);
        } else {
            return (static_cast<const Qp_ &>(*alternative_ptr_<Qp_>(
                std::addressof(this->storage.alternatives))));
        }
    }

  private:
    constexpr auto copy_construct_from_(const OptionStorage_ &other_) -> void {
        using Fn_ = void (*)(OptionStorage_ &, const OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &copy_one_<Tp_>, &copy_one_<Args_>...};

        if (other_.is_some) {
            dispatch_[static_cast<std::size_t>(other_.is_some.index_ - 1)](
                *this, other_);
        }
    }

  private:
    template <typename Qp_>
    static constexpr auto copy_one_(OptionStorage_ &target_,
                                    const OptionStorage_ &source_) -> void {
        if constexpr (std::is_copy_constructible_v<std::remove_cv_t<Qp_>>) {
            target_.template construct_<Qp_>(source_.template get_<Qp_>());
        }
    }

  private:
    constexpr auto move_construct_from_(OptionStorage_ &other_) -> void {
        using Fn_ = void (*)(OptionStorage_ &, OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &move_one_<Tp_>, &move_one_<Args_>...};

        if (other_.is_some) {
            dispatch_[static_cast<std::size_t>(other_.is_some.index_ - 1)](
                *this, other_);
        }
    }

  private:
    template <typename Qp_>
    static constexpr auto move_one_(OptionStorage_ &target_,
                                    OptionStorage_ &source_) -> void {
        if constexpr (std::is_move_constructible_v<std::remove_cv_t<Qp_>>) {
            target_.template construct_<Qp_>(
                std::move(source_.template get_<Qp_>()));
        }
    }

  private:
    constexpr auto copy_assign_from_(const OptionStorage_ &other_) -> void {
        using Fn_ = void (*)(OptionStorage_ &, const OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &copy_assign_one_<Tp_>, &copy_assign_one_<Args_>...};

        if (other_.is_some) {
            dispatch_[static_cast<std::size_t>(other_.is_some.index_ - 1)](
                *this, other_);
        }
    }

  private:
    template <typename Qp_>
    static constexpr auto copy_assign_one_(OptionStorage_ &target_,
                                           const OptionStorage_ &source_)
        -> void {
        if constexpr (std::is_copy_assignable_v<std::remove_cv_t<Qp_>>) {
            target_.template get_<Qp_>() = source_.template get_<Qp_>();
        } else {
            target_.reset_();
            target_.template construct_<Qp_>(source_.template get_<Qp_>());
        }
    }

  private:
    constexpr auto move_assign_from_(OptionStorage_ &other_) -> void {
        using Fn_ = void (*)(OptionStorage_ &, OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &move_assign_one_<Tp_>, &move_assign_one_<Args_>...};

        if (other_.is_some) {
            dispatch_[static_cast<std::size_t>(other_.is_some.index_ - 1)](
                *this, other_);
        }
    }

  private:
    template <typename Qp_>
    static constexpr auto move_assign_one_(OptionStorage_ &target_,
                                           OptionStorage_ &source_) -> void {
        if constexpr (std::is_move_assignable_v<std::remove_cv_t<Qp_>>) {
            target_.template get_<Qp_>() =
                std::move(source_.template get_<Qp_>());
        } else {
            target_.reset_();
            target_.template construct_<Qp_>(
                std::move(source_.template get_<Qp_>()));
        }
    }

  private:
    constexpr auto destroy_alternative_(std::size_t index_) noexcept(true)
        -> void {
        using Fn_ = void (*)(OptionStorage_ &) noexcept(true);
        static constexpr std::array<Fn_, sizeof...(Args_)> dispatch_{
            &destroy_one_<Args_>...};

        if (index_ >= 2 && index_ < sizeof...(Args_) + 2) {
            dispatch_[index_ - 2](*this);
        }
    }

  private:
    template <typename Qp_>
    static constexpr auto destroy_one_(OptionStorage_ &self_) noexcept(true)
        -> void {
        destroy_alternative_<Qp_>(self_.storage.alternatives);
    }

  public:
    constexpr OptionStorage_(void) noexcept(true)
        : storage{}, is_some{0}, _padding_{} {}

  public:
    constexpr OptionStorage_(typename Opt::None) noexcept(true)
        : OptionStorage_{} {}

  public:
    template <typename Qp_, typename... CtorArgs_>
    constexpr explicit OptionStorage_(
        std::in_place_type_t<Qp_>,
        CtorArgs_
            &&...args) noexcept(std::
                                    is_nothrow_constructible_v<
                                        std::remove_cv_t<Qp_>, CtorArgs_...>)
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_constructible_v<std::remove_cv_t<Qp_>, CtorArgs_...>)
        : OptionStorage_{} {
        this->template construct_<Qp_>(std::forward<CtorArgs_>(args)...);
    }

  public:
    template <typename Qp_>
    constexpr OptionStorage_(const Opt::Some<Qp_> &some_) noexcept(
        std::is_nothrow_copy_constructible_v<std::remove_cv_t<Qp_>>)
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_copy_constructible_v<std::remove_cv_t<Qp_>>)
        : OptionStorage_{} {
        this->template construct_<Qp_>(some_.get());
    }

  public:
    template <typename Qp_>
    constexpr OptionStorage_(Opt::Some<Qp_> &&some_) noexcept(
        std::is_nothrow_move_constructible_v<std::remove_cv_t<Qp_>>)
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_move_constructible_v<std::remove_cv_t<Qp_>>)
        : OptionStorage_{} {
        this->template construct_<Qp_>(std::move(some_).get());
    }

  public:
    constexpr OptionStorage_(typename Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<
            typename first_default_constructible_<Tp_, Args_...>::type>)
        requires(!std::same_as<
                 typename first_default_constructible_<Tp_, Args_...>::type,
                 void>)
        : OptionStorage_{} {
        using Default_ =
            typename first_default_constructible_<Tp_, Args_...>::type;

        this->template construct_<Default_>();
    }

  public:
    constexpr OptionStorage_(typename Opt::Some<void>) noexcept(
        std::is_nothrow_default_constructible_v<
            typename first_default_constructible_<Tp_, Args_...>::type>)
        requires(!std::same_as<
                 typename first_default_constructible_<Tp_, Args_...>::type,
                 void>)
        : OptionStorage_{Opt::Default{}} {}

  public:
    constexpr OptionStorage_(const OptionStorage_ &other_)
        requires(_trivial_copy_)
    = default;

  public:
    constexpr OptionStorage_(const OptionStorage_ &other_) noexcept(
        std::is_nothrow_copy_constructible_v<Tp_> &&
        (std::is_nothrow_copy_constructible_v<Args_> && ...))
        requires((std::is_copy_constructible_v<Tp_> &&
                  (std::is_copy_constructible_v<Args_> && ...)) &&
                 !_trivial_copy_)
        : OptionStorage_{} {
        this->copy_construct_from_(other_);
    }

  public:
    constexpr OptionStorage_(const OptionStorage_ &)
        requires(not(std::is_copy_constructible_v<Tp_> &&
                     (std::is_copy_constructible_v<Args_> && ...)))
    = delete;

  public:
    constexpr OptionStorage_(OptionStorage_ &&other_)
        requires(_trivial_move_)
    = default;

  public:
    constexpr OptionStorage_(OptionStorage_ &&other_) noexcept(
        std::is_nothrow_move_constructible_v<Tp_> &&
        (std::is_nothrow_move_constructible_v<Args_> && ...))
        requires((std::is_move_constructible_v<Tp_> &&
                  (std::is_move_constructible_v<Args_> && ...)) &&
                 !_trivial_move_)
        : OptionStorage_{} {
        this->move_construct_from_(other_);
    }

  public:
    constexpr OptionStorage_(OptionStorage_ &&)
        requires(not(std::is_move_constructible_v<Tp_> &&
                     (std::is_move_constructible_v<Args_> && ...)))
    = delete;

  public:
    constexpr auto operator=(const OptionStorage_ &other_) -> OptionStorage_ &
        requires(_trivial_copy_assign_)
    = default;

  public:
    constexpr auto operator=(const OptionStorage_ &other_) noexcept(
        (std::is_nothrow_copy_constructible_v<Tp_> &&
         (std::is_nothrow_copy_constructible_v<Args_> && ...)) &&
        ((!std::is_copy_assignable_v<Tp_> ||
          std::is_nothrow_copy_assignable_v<Tp_>) &&
         ((!std::is_copy_assignable_v<Args_> ||
           std::is_nothrow_copy_assignable_v<Args_>) &&
          ...)) &&
        (std::is_nothrow_destructible_v<Tp_> &&
         (std::is_nothrow_destructible_v<Args_> && ...))) -> OptionStorage_ &
        requires((std::is_copy_constructible_v<Tp_> &&
                  (std::is_copy_constructible_v<Args_> && ...)) &&
                 !_trivial_copy_assign_)
    {
        if (this != std::addressof(other_)) {
            if (this->is_some && other_.is_some &&
                this->is_some.index_ == other_.is_some.index_) {
                this->copy_assign_from_(other_);
            }

            else {
                this->reset_();
                this->copy_construct_from_(other_);
            }
        }

        return *this;
    }

  public:
    constexpr auto operator=(const OptionStorage_ &) -> OptionStorage_ &
        requires(not(std::is_copy_constructible_v<Tp_> &&
                     (std::is_copy_constructible_v<Args_> && ...)))
    = delete;

  public:
    constexpr auto operator=(OptionStorage_ &&other_) -> OptionStorage_ &
        requires(_trivial_move_assign_)
    = default;

  public:
    constexpr auto operator=(OptionStorage_ &&other_) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_move_assignable_v<Tp_> &&
         (std::is_nothrow_move_assignable_v<Args_> && ...)) &&
        (std::is_nothrow_destructible_v<Tp_> &&
         (std::is_nothrow_destructible_v<Args_> && ...))) -> OptionStorage_ &
        requires((std::is_move_constructible_v<Tp_> &&
                  (std::is_move_constructible_v<Args_> && ...)) &&
                 !_trivial_move_assign_)
    {
        if (this != std::addressof(other_)) {
            if (this->is_some && other_.is_some &&
                this->is_some.index_ == other_.is_some.index_) {
                this->move_assign_from_(other_);
            }

            else {
                this->reset_();
                this->move_construct_from_(other_);
            }
        }

        return *this;
    }

  public:
    constexpr auto operator=(OptionStorage_ &&) -> OptionStorage_ &
        requires(not(std::is_move_constructible_v<Tp_> &&
                     (std::is_move_constructible_v<Args_> && ...)))
    = delete;

  public:
    constexpr ~OptionStorage_(void) noexcept(true)
        requires(_trivial_dtor_)
    = default;

  public:
    constexpr ~OptionStorage_(void) noexcept(true)
        requires(!_trivial_dtor_)
    {
        this->reset_();
    }

  private:
    constexpr auto ensure_not_destroying_(void) const noexcept(true) -> void {
        if (this->is_some.index_ == destroying_index_) {
            details::panic_(
                "cannot mutate Option while an alternative is being destroyed");
        }
    }

  private:
    constexpr auto reset_(void) noexcept(true) -> void {
        const std::size_t index_{
            static_cast<std::size_t>(this->is_some.index_)};
        if (index_ == 0 ||
            index_ == static_cast<std::size_t>(destroying_index_)) {
            return;
        }
        this->is_some.index_ = destroying_index_;

        if (index_ == 1) {
            std::destroy_at(const_cast<std::remove_cv_t<Tp_> *>(
                std::addressof(this->storage.value)));
        }

        else {
            this->destroy_alternative_(index_);
        }

        this->is_some.index_ = 0;
    }

  private:
    template <typename Qp_, typename... CtorArgs_>
    constexpr auto emplace_(CtorArgs_ &&...args) noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, CtorArgs_...>)
        [[RL_LIFETIME_BOUND]] -> Qp_ &
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_constructible_v<std::remove_cv_t<Qp_>, CtorArgs_...>)
    {
        this->reset_();
        return this->template construct_<Qp_>(std::forward<CtorArgs_>(args)...);
    }

  private:
    constexpr auto active_index_(void) const noexcept(true) -> std::size_t {
        const std::size_t index_{
            static_cast<std::size_t>(this->is_some.index_)};
        return (index_ == static_cast<std::size_t>(destroying_index_)) ? 0
                                                                       : index_;
    }

  private:
    template <typename Qp_>
    constexpr auto has_type_(void) const noexcept(true) -> bool {
        return (static_cast<std::size_t>(this->is_some.index_) ==
                static_cast<std::size_t>(this->index_of_<Qp_>()));
    }

  private:
    template <typename Qp_>
    static constexpr auto
    swap_one_(OptionStorage_ &lhs_, OptionStorage_ &rhs_) noexcept(
        std::is_nothrow_swappable_v<std::remove_cv_t<Qp_>>) -> void {
        std::ranges::swap(lhs_.template get_<Qp_>(), rhs_.template get_<Qp_>());
    }

  private:
    constexpr auto
    swap_same_active_(std::size_t index_, OptionStorage_ &other_) noexcept(
        (std::is_nothrow_swappable_v<Tp_> &&
         (std::is_nothrow_swappable_v<Args_> && ...))) -> void {
        using Fn_ = void (*)(OptionStorage_ &, OptionStorage_ &);
        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &swap_one_<Tp_>, &swap_one_<Args_>...};
        dispatch_[index_ - 1](*this, other_);
    }

  private:
    constexpr auto swap_(OptionStorage_ &other_) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_swappable_v<Tp_> &&
         (std::is_nothrow_swappable_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::swappable<Tp_> && (std::swappable<Args_> && ...))
    {
        if (this == std::addressof(other_)) {
            return;
        }

        this->ensure_not_destroying_();
        other_.ensure_not_destroying_();

        const std::size_t lhs_index_{
            static_cast<std::size_t>(this->is_some.index_)};
        const std::size_t rhs_index_{
            static_cast<std::size_t>(other_.is_some.index_)};

        if (lhs_index_ == rhs_index_) {
            if (lhs_index_ != 0) {
                this->swap_same_active_(lhs_index_, other_);
            }
            return;
        }

        if (lhs_index_ == 0) {
            this->move_construct_from_(other_);
            other_.reset_();
            return;
        }

        if (rhs_index_ == 0) {
            other_.move_construct_from_(*this);
            this->reset_();
            return;
        }

        // Different active alternatives: move-construct through an empty
        // temporary. No alternative's move-assignment operator is required.
        OptionStorage_ temp_{std::move(*this)};
        this->reset_();
        this->move_construct_from_(other_);
        other_.reset_();
        other_.move_construct_from_(temp_);
    }

  private:
    static_assert(sizeof(Index_) == sizeof(State_));

  private:
    static_assert(alignof(State_) == alignof(Index_));

  private:
    static_assert(alignof(State_) <= _alignment_);

  private:
    static_assert(sizeof(Storage_) == _payload_size_);

  private:
    static_assert(alignof(Storage_) == _alignment_);

  private:
    friend class Opt::Option<Tp_, Args_...>;
};

template <typename Tp_, typename... Args_>
consteval auto option_storage_layout_valid_(void) noexcept(true) -> bool {
    using Storage_ = OptionStorage_<Tp_, Args_...>;
    using Index_ = typename option_index_type_<sizeof...(Args_) + 2>::type;
    using Alternative_ = details::Alternative_<Args_...>;

    constexpr std::size_t alignment_{max_alignof_<Tp_, Args_..., Index_>()};

    constexpr std::size_t payload_{align_up_(
        sizeof(Tp_) > sizeof(Alternative_) ? sizeof(Tp_) : sizeof(Alternative_),
        alignment_)};
    constexpr std::size_t padding_{
        (alignment_ - ((payload_ + sizeof(Index_)) % alignment_)) % alignment_};

    if constexpr (std::is_standard_layout_v<Tp_> &&
                  (std::is_standard_layout_v<Args_> && ...)) {
        static_assert(std::is_standard_layout_v<Storage_>);
        static_assert(offsetof(Storage_, storage) == 0);
        static_assert(offsetof(Storage_, is_some) == payload_);

        if constexpr (padding_ != 0) {
            static_assert(offsetof(Storage_, _padding_) ==
                          payload_ + sizeof(Index_));
        }

        if constexpr (sizeof...(Args_) != 0) {
            static_assert(offsetof(Alternative_, first_) == 0);
            static_assert(offsetof(Alternative_, rest_) == 0);
            static_assert(offsetof(Alternative_, layout_) == 0);
        }
    }

    static_assert(sizeof(Storage_) == payload_ + sizeof(Index_) + padding_);

    static_assert(alignof(Storage_) == alignment_);

    if constexpr (sizeof...(Args_) == 0) {
        static_assert(sizeof(Storage_) == sizeof(Optional_<Tp_>));
        static_assert(alignof(Storage_) == alignof(Optional_<Tp_>));
    }

    else {
        static_assert(
            sizeof(Alternative_) ==
            align_up_(max_sizeof_<Args_...>(), max_alignof_<Args_...>()));

        static_assert(alignof(Alternative_) == max_alignof_<Args_...>());
    }

    return true;
}

namespace Opt {
template <typename Tp_, typename... Args_>
class alignas(
    details::max_alignof_<
        Tp_, Args_...,
        typename details::option_index_type_<sizeof...(Args_) + 2>::type>())
    [[nodiscard]] Option {
  private:
    template <typename, typename...> friend class Option;

  private:
    using Data_ = details::OptionStorage_<Tp_, Args_...>;

  private:
    Data_ data;

  private:
    template <typename From_, typename To_>
    using ForwardLike_t = details::ForwardLike_t<From_, To_>;

  private:
    template <typename Qp_>
    constexpr auto has_type_(void) const noexcept(true) -> bool {
        return this->data.template has_type_<Qp_>();
    }

  private:
    template <typename Qp_>
    constexpr auto get_(void) & noexcept(true) [[RL_LIFETIME_BOUND]] -> Qp_ & {
        return this->data.template get_<Qp_>();
    }

  private:
    template <typename Qp_>
    constexpr auto get_(void) const & noexcept(true) [[RL_LIFETIME_BOUND]]
    -> const Qp_ & {
        return this->data.template get_<Qp_>();
    }

  private:
    constexpr auto primary_value_(void) & noexcept(true) [[RL_LIFETIME_BOUND]]
    -> Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>: attempt to access disengaged or "
                            "inactive primary value",
                            this->template get_type_name<Tp_>());
        }

        return this->data.storage.value;
    }

  private:
    constexpr auto primary_value_(void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> const Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>: attempt to access disengaged or "
                            "inactive primary value",
                            this->template get_type_name<Tp_>());
        }

        return this->data.storage.value;
    }

  private:
    using default_alternative_type_ =
        typename details::first_default_constructible_<Args_...>::type;

  private:
    static constexpr bool has_default_alternative_ =
        (!std::same_as<default_alternative_type_, void>);

  private:
    constexpr auto active_index_(void) const noexcept(true) -> std::size_t {
        return this->data.active_index_();
    }

  private:
    constexpr auto has_primary_(void) const noexcept(true) -> bool {
        return this->template has_type_<Tp_>();
    }

// Intentional float equality: mirroring std::optional<T>::operator==.
// Suppress -Wfloat-equal for the noexcept specifier and the body.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
  private:
    template <typename Qp_>
    static constexpr auto
    equal_values_(const Option &lhs_, const Option &rhs_) noexcept(noexcept(
        lhs_.template get_<Qp_>() == rhs_.template get_<Qp_>())) -> bool {
        return (lhs_.template get_<Qp_>() == rhs_.template get_<Qp_>());
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  private:
    static constexpr auto equal_active_(std::size_t index_, const Option &lhs_,
                                        const Option &rhs_) -> bool {
        using Fn_ = bool (*)(const Option &, const Option &);
        constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &equal_values_<Tp_>, &equal_values_<Args_>...};

        if (index_ == 0 || index_ > dispatch_.size()) {
            return false;
        }

        return dispatch_[index_ - 1](lhs_, rhs_);
    }

  private:
    using ComparisonCategory_ =
        details::__compare_three_way_results_t<Tp_, Args_...>;

  private:
    template <typename Qp_>
    static constexpr auto compare_values_(const Option &lhs_,
                                          const Option &rhs_)
        -> ComparisonCategory_ {
        return static_cast<ComparisonCategory_>(lhs_.template get_<Qp_>() <=>
                                                rhs_.template get_<Qp_>());
    }

  private:
    static constexpr auto
    compare_active_(std::size_t index_, const Option &lhs_, const Option &rhs_)
        -> ComparisonCategory_ {
        using Fn_ = ComparisonCategory_ (*)(const Option &, const Option &);
        constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &compare_values_<Tp_>, &compare_values_<Args_>...};
        return dispatch_[index_ - 1](lhs_, rhs_);
    }

  private:
    template <typename Types_, bool Enabled>
    friend struct details::option_hash_base_;

    static_assert(details::unique_types_<Tp_, Args_...>::value,
                  "Option<T, Args...> requires distinct alternative types");

    static_assert(std::is_object_v<Tp_> && !std::is_array_v<Tp_> &&
                      ((std::is_object_v<Args_> && !std::is_array_v<Args_>) &&
                       ...),
                  "Option<T, Args...> requires non-array object types");

    static_assert(details::option_storage_layout_valid_<Tp_, Args_...>());

  private:
    using value_type = Tp_;
    static constexpr auto SomeAny{details::AnySome_{}};
    static constexpr auto None{Opt::None{}};
    static constexpr auto Default{Opt::Default{}};
    static constexpr auto Wild{details::Wild_{}};

  public:
    constexpr explicit operator bool(void) const noexcept(true) {
        return this->is_some();
    }

  public:
    template <typename Up_>
        requires((std::same_as<std::remove_cvref_t<Up_>, Tp_> ||
                  (std::same_as<std::remove_cvref_t<Up_>, Args_> || ...) ||
                  (sizeof...(Args_) == 0 &&
                   std::convertible_to<const Tp_ &, Up_>)) &&
                 !std::same_as<std::remove_cvref_t<Up_>, Option> &&
                 !details::__is_option_<Up_>)
    constexpr explicit operator Up_() const & {
        if constexpr (std::same_as<std::remove_cvref_t<Up_>, Tp_> ||
                      (std::same_as<std::remove_cvref_t<Up_>, Args_> || ...)) {
            return this->template unwrap<std::remove_cvref_t<Up_>>();
        } else {
            return static_cast<Up_>(this->unwrap());
        }
    }

  public:
    template <typename Up_>
        requires((std::same_as<std::remove_cvref_t<Up_>, Tp_> ||
                  (std::same_as<std::remove_cvref_t<Up_>, Args_> || ...) ||
                  (sizeof...(Args_) == 0 &&
                   std::convertible_to<Tp_ &&, Up_>)) &&
                 !std::same_as<std::remove_cvref_t<Up_>, Option> &&
                 !details::__is_option_<Up_>)
    constexpr explicit operator Up_() && {
        if constexpr (std::same_as<std::remove_cvref_t<Up_>, Tp_> ||
                      (std::same_as<std::remove_cvref_t<Up_>, Args_> || ...)) {
            return std::move(*this).template unwrap<std::remove_cvref_t<Up_>>();
        } else {
            return static_cast<Up_>(std::move(*this).unwrap());
        }
    }

  public:
    constexpr auto has_any_engaged_value [[nodiscard]] (void) const
        noexcept(true) -> bool {
        return this->is_some();
    }

  public:
    constexpr Option(void) = delete (
        "Option does not support empty-handed initialization. Use "
        "Some<T>(v) or Default to prevent deletion.");

  public:
    constexpr Option(const typename details::Opt::Some<Tp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Tp_>)
        requires(std::is_copy_constructible_v<Tp_>)
        : data{some} {}

  public:
    constexpr Option(typename details::Opt::Some<Tp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Tp_>)
        requires(std::is_move_constructible_v<Tp_>)
        : data{std::move(some)} {}

  public:
    template <typename Qp_>
    constexpr Option(const typename details::Opt::Some<Qp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_>)
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_copy_constructible_v<Qp_>)
        : data{some} {}

  public:
    template <typename Qp_>
    constexpr Option(typename details::Opt::Some<Qp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Qp_>)
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_move_constructible_v<Qp_>)
        : data{std::move(some)} {}

  public:
    template <typename Qp_,
              typename Target_ = typename details::match_alternative_<
                  const Qp_ &, Tp_, Args_...>::type>
    constexpr Option(const typename details::Opt::Some<Qp_> &some) noexcept(
        std::is_nothrow_constructible_v<Target_, const Qp_ &>)
        requires(
            (!std::same_as<Qp_, Tp_> && !(std::same_as<Qp_, Args_> || ...)) &&
            details::match_alternative_<const Qp_ &, Tp_, Args_...>::valid &&
            std::is_constructible_v<Target_, const Qp_ &>)
        : data{std::in_place_type<Target_>, some.get()} {}

  public:
    template <typename Qp_, typename Target_ = typename details::
                                match_alternative_<Qp_, Tp_, Args_...>::type>
    constexpr Option(typename details::Opt::Some<Qp_> &&some) noexcept(
        std::is_nothrow_constructible_v<Target_, Qp_>)
        requires((!std::same_as<Qp_, Tp_> &&
                  !(std::same_as<Qp_, Args_> || ...)) &&
                 details::match_alternative_<Qp_, Tp_, Args_...>::valid &&
                 std::is_constructible_v<Target_, Qp_>)
        : data{std::in_place_type<Target_>, std::move(some).get()} {}

  public:
    template <typename Up_, typename Target_ = typename details::
                                match_alternative_<Up_, Tp_, Args_...>::type>
    constexpr Option(Up_ &&val_) noexcept(
        std::is_nothrow_constructible_v<Target_, Up_>)
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 details::match_alternative_<Up_, Tp_, Args_...>::valid &&
                 std::is_constructible_v<Target_, Up_>)
    = delete ("Option does not support default T constructors.");

  public:
    constexpr Option(typename details::Opt::None none) noexcept(true)
        : data{none} {}

  public:
    constexpr Option(typename details::Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<Tp_>)
        requires(std::is_default_constructible_v<Tp_>)
        : data{Opt::Default{}} {}

  public:
    constexpr Option(typename details::Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<default_alternative_type_>)
        requires(!std::is_default_constructible_v<Tp_> &&
                 has_default_alternative_)
        : data{Opt::Default{}} {}

  public:
    constexpr Option(typename details::Opt::Some<void>) noexcept(
        std::is_nothrow_default_constructible_v<
            typename details::first_default_constructible_<Tp_,
                                                           Args_...>::type>)
        requires(!std::same_as<typename details::first_default_constructible_<
                                   Tp_, Args_...>::type,
                               void>)
        : Option{Opt::Default{}} {}

  public:
    constexpr Option(const Option &)
        requires(std::is_copy_constructible_v<Tp_> &&
                 (std::is_copy_constructible_v<Args_> && ...))
    = default;

  public:
    constexpr Option(const Option &)
        requires(not(std::is_copy_constructible_v<Tp_> &&
                     (std::is_copy_constructible_v<Args_> && ...)))
    = delete ("Option copy constructor is deleted because one or more "
              "alternative types are not copy-constructible");

  public:
    constexpr Option(Option &&)
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...))
    = default;

  public:
    constexpr Option(Option &&)
        requires(not(std::is_move_constructible_v<Tp_> &&
                     (std::is_move_constructible_v<Args_> && ...)))
    = delete ("Option move constructor is deleted because one or more "
              "alternative types are not move-constructible");

  public:
    constexpr Option &operator=(const Option &)
        requires(std::is_copy_constructible_v<Tp_> &&
                 (std::is_copy_constructible_v<Args_> && ...) &&
                 (std::is_copy_assignable_v<Tp_> &&
                  (std::is_copy_assignable_v<Args_> && ...)))
    = default;

  public:
    constexpr Option &operator=(const Option &)
        requires(not(std::is_copy_constructible_v<Tp_> &&
                     (std::is_copy_constructible_v<Args_> && ...) &&
                     (std::is_copy_assignable_v<Tp_> &&
                      (std::is_copy_assignable_v<Args_> && ...))))
    = delete (
        "Option copy assignment operator is deleted because one or more "
        "alternative types are not copy-assignable or copy-constructible");

  public:
    constexpr Option &operator=(Option &&)
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 (std::is_move_assignable_v<Tp_> &&
                  (std::is_move_assignable_v<Args_> && ...)))
    = default;

  public:
    constexpr Option &operator=(Option &&)
        requires(not(std::is_move_constructible_v<Tp_> &&
                     (std::is_move_constructible_v<Args_> && ...) &&
                     (std::is_move_assignable_v<Tp_> &&
                      (std::is_move_assignable_v<Args_> && ...))))
    = delete (
        "Option move assignment operator is deleted because one or more "
        "alternative types are not move-assignable or move-constructible");

    static_assert(std::is_object_v<Tp_> && !std::is_array_v<Tp_>,
                  "Option<T> requires a non-array object type");

    static_assert(std::is_nothrow_destructible_v<Tp_>,
                  "Option<T> requires a nothrow-destructible type");

    static_assert(!std::is_volatile_v<Tp_>,
                  "Option<T> does not support volatile-qualified types");

    static_assert(((std::is_nothrow_destructible_v<Args_>) && ...),
                  "Option<T, Args...> requires "
                  "nothrow-destructible alternatives");

    static_assert(((!std::is_volatile_v<Args_>) && ...),
                  "Option<T, Args...> does not support volatile "
                  "alternatives");

  private:
    template <typename Qp_>
    constexpr auto get_type_name [[nodiscard]] (void) const & noexcept(true)
        -> std::string_view {
#if RL_SUPPORTS_REFLECTION
        return std::meta::display_string_of(^^Qp_);
#else
        constexpr std::string_view name{__PRETTY_FUNCTION__};
        constexpr std::size_t open_bracket{name.find('[')};
        constexpr std::size_t close_bracket{name.rfind(']')};

        static_assert(open_bracket != std::string_view::npos &&
                          close_bracket != std::string_view::npos,
                      "Compiler signature format not recognized");

        constexpr std::string_view inner{name.RL_IS_SUBVIEW_OR_SUBSTR(
            open_bracket + 1, close_bracket - open_bracket - 1)};

        constexpr std::string_view prefix{"Qp_ = "};
        constexpr std::size_t pos{inner.find(prefix)};

        if constexpr (pos != std::string_view::npos) {
            constexpr std::string_view rest{
                inner.RL_IS_SUBVIEW_OR_SUBSTR(pos + prefix.size())};

            constexpr auto find_type_end =
                [](std::string_view s_) constexpr -> std::size_t {
                std::size_t angle_depth{0};
                std::size_t paren_depth{0};
                std::size_t bracket_depth{0};

                auto indexed_chars = std::views::zip(
                    std::views::iota(std::size_t{0}, s_.size()), s_);

                auto it = std::ranges::find_if(
                    indexed_chars,
                    [&](auto const &pair_) {
                        auto [i, c] = pair_;

                        switch (c) {
                        case ('<'): {
                            ++angle_depth;
                            break;
                        }

                        case ('>'): {
                            if (angle_depth > 0) {
                                --angle_depth;
                            }

                            break;
                        }

                        case ('('): {
                            ++paren_depth;
                            break;
                        }

                        case (')'): {
                            if (paren_depth > 0) {
                                --paren_depth;
                            }

                            break;
                        }

                        case ('['): {
                            ++bracket_depth;
                            break;
                        }

                        case (']'): {
                            if (bracket_depth > 0) {
                                --bracket_depth;
                            }

                            else if (angle_depth == 0 && paren_depth == 0) {
                                return true;
                            }

                            break;
                        }

                        default: {
                            if (angle_depth == 0 && paren_depth == 0 &&
                                bracket_depth == 0 && (c == ',' || c == ';')) {
                                return true;
                            }

                            break;
                        }
                        }

                        return false;
                    })

                    ;

                auto _x =
                    (it != indexed_chars.end() ? std::get<0>(*it) : s_.size());

                return _x;
            };

            constexpr std::size_t sep{find_type_end(rest)};

            return (rest.RL_IS_SUBVIEW_OR_SUBSTR(0, sep));
        }

        else {
            return inner;
        }
#endif
    }

  public:
    template <typename Self_, typename Qp_, typename Up_>
        requires(std::same_as<std::remove_cvref_t<Self_>, Option> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_> &&
                 details::__all_map_or_valid_<std::remove_cvref_t<Up_>, Qp_,
                                              ForwardLike_t<Self_, Tp_>,
                                              ForwardLike_t<Self_, Args_>...> &&
                 (!std::is_invocable_v<Qp_, Opt::None> ||
                  details::__map_invocable_non_void_<Qp_, Opt::None>))
    constexpr auto map_or_transform
        [[nodiscard]] (this Self_ &&self, Qp_ &&op, Up_ &&v_)
            -> std::remove_cvref_t<Up_> {
        using Rp_ = std::remove_cvref_t<Up_>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    auto out = static_cast<Rp_>(
                        std::invoke(std::forward<Qp_>(op),
                                    std::forward<decltype(val)>(val)));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                },
                None >>= [&]() -> Rp_ {
                    if constexpr (std::is_invocable_v<Qp_, Opt::None>) {
                        return static_cast<Rp_>(
                            std::invoke(std::forward<Qp_>(op), Opt::None{}));
                    }
                    return static_cast<Rp_>(std::forward<Up_>(v_));
                })
            .unwrap();
    }

  public:
    template <typename Target_, typename Self_, typename Qp_, typename Up_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>, Up_> &&
            details::__map_invocable_non_void_<Qp_,
                                               ForwardLike_t<Self_, Target_>> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>,
                                    details::__map_single_result_t<
                                        Qp_, ForwardLike_t<Self_, Target_>>> &&
            (!std::is_invocable_v<Qp_, Opt::None> ||
             details::__map_invocable_non_void_<Qp_, Opt::None>))
    constexpr auto map_or_transform
        [[nodiscard]] (this Self_ &&self, Qp_ &&op, Up_ &&v_)
            -> std::remove_cvref_t<Up_> {
        using Rp_ = std::remove_cvref_t<Up_>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    using Actual = std::remove_cvref_t<decltype(val)>;
                    if constexpr (std::same_as<Actual, Target_>) {
                        auto out = static_cast<Rp_>(
                            std::invoke(std::forward<Qp_>(op),
                                        std::forward<decltype(val)>(val)));
                        if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                      !std::is_const_v<
                                          std::remove_reference_t<Self_>>) {
                            self.reset();
                        }
                        return out;
                    } else {
                        return static_cast<Rp_>(std::forward<Up_>(v_));
                    }
                },
                None >>= [&]() -> Rp_ {
                    if constexpr (std::is_invocable_v<Qp_, Opt::None>) {
                        return static_cast<Rp_>(
                            std::invoke(std::forward<Qp_>(op), Opt::None{}));
                    }
                    return static_cast<Rp_>(std::forward<Up_>(v_));
                })
            .unwrap();
    }

  public:
    template <typename Self_, typename Qp_, typename FallbackFn_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__nullary_invoke_result_materializable_<
                FallbackFn_>::value &&
            details::__all_map_or_valid_<
                std::remove_cvref_t<std::invoke_result_t<FallbackFn_>>, Qp_,
                ForwardLike_t<Self_, Tp_>, ForwardLike_t<Self_, Args_>...>)
    constexpr auto map_or_else
        [[nodiscard]] (this Self_ &&self, Qp_ &&op, FallbackFn_ &&f_)
            -> std::remove_cvref_t<std::invoke_result_t<FallbackFn_>> {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<FallbackFn_>>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    auto out = static_cast<Rp_>(
                        std::invoke(std::forward<Qp_>(op),
                                    std::forward<decltype(val)>(val)));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                },
                None >>= [&]() -> Rp_ {
                    return static_cast<Rp_>(
                        std::invoke(std::forward<FallbackFn_>(f_)));
                })
            .unwrap();
    }

  public:
    template <typename Target_, typename Self_, typename Qp_,
              typename FallbackFn_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__nullary_invoke_result_materializable_<
                FallbackFn_>::value &&
            details::__map_invocable_non_void_<Qp_,
                                               ForwardLike_t<Self_, Target_>> &&
            std::is_constructible_v<
                std::remove_cvref_t<std::invoke_result_t<FallbackFn_>>,
                details::__map_single_result_t<Qp_,
                                               ForwardLike_t<Self_, Target_>>>)
    constexpr auto map_or_else
        [[nodiscard]] (this Self_ &&self, Qp_ &&op, FallbackFn_ &&f_)
            -> std::remove_cvref_t<std::invoke_result_t<FallbackFn_>> {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<FallbackFn_>>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    using Actual = std::remove_cvref_t<decltype(val)>;
                    if constexpr (std::same_as<Actual, Target_>) {
                        auto out = static_cast<Rp_>(
                            std::invoke(std::forward<Qp_>(op),
                                        std::forward<decltype(val)>(val)));
                        if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                      !std::is_const_v<
                                          std::remove_reference_t<Self_>>) {
                            self.reset();
                        }
                        return out;
                    } else {
                        return static_cast<Rp_>(
                            std::invoke(std::forward<FallbackFn_>(f_)));
                    }
                },
                None >>= [&]() -> Rp_ {
                    return static_cast<Rp_>(
                        std::invoke(std::forward<FallbackFn_>(f_)));
                })
            .unwrap();
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            ((std::invocable<Qp_> &&
              (std::convertible_to<std::invoke_result_t<Qp_>, Tp_> ||
               std::constructible_from<Tp_, std::invoke_result_t<Qp_>>)) ||
             (std::invocable<Qp_, Opt::None> &&
              (std::convertible_to<std::invoke_result_t<Qp_, Opt::None>, Tp_> ||
               std::constructible_from<
                   Tp_, std::invoke_result_t<Qp_, Opt::None>>))) &&
            (std::is_rvalue_reference_v<Self_ &&>
                 ? std::is_move_constructible_v<Tp_>
                 : std::is_copy_constructible_v<Tp_>))
    constexpr auto unwrap_or_else
        [[nodiscard]] (this Self_ &&self, Qp_ &&op) noexcept(
            std::is_rvalue_reference_v<Self_ &&>
                ? std::is_nothrow_move_constructible_v<Tp_>
                : std::is_nothrow_copy_constructible_v<Tp_>) -> Tp_ {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Tp_ {
                    return static_cast<Tp_>(std::forward<decltype(val)>(val));
                },
                None >>= [&]() -> Tp_ {
                    if constexpr (std::invocable<Qp_>) {
                        return static_cast<Tp_>(
                            std::invoke(std::forward<Qp_>(op)));
                    } else {
                        return static_cast<Tp_>(
                            std::invoke(std::forward<Qp_>(op), Opt::None{}));
                    }
                })
            .unwrap();
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            (std::is_constructible_v<Tp_, ForwardLike_t<Self_, Tp_>> &&
             (std::is_constructible_v<Args_, ForwardLike_t<Self_, Args_>> &&
              ...)) &&
            ((std::invocable<Qp_> &&
              std::convertible_to<std::invoke_result_t<Qp_>, Option>) ||
             (std::invocable<Qp_, Opt::None> &&
              std::convertible_to<std::invoke_result_t<Qp_, Opt::None>,
                                  Option>)))
    constexpr auto or_else [[nodiscard]] (this Self_ &&self, Qp_ &&op)
        -> Option {
        return std::forward<Self_>(self).match(
            SomeAny >>= [&](auto &&val) -> Option {
                using Actual_ = std::remove_cvref_t<decltype(val)>;
                auto res = Option(
                    Opt::Some<Actual_>(std::forward<decltype(val)>(val)));
                if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                              !std::is_const_v<
                                  std::remove_reference_t<Self_>>) {
                    self.reset();
                }
                return res;
            },
            None >>= [&]() -> Option {
                if constexpr (std::invocable<Qp_>) {
                    return std::invoke(std::forward<Qp_>(op));
                } else {
                    return std::invoke(std::forward<Qp_>(op), Opt::None{});
                }
            });
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__all_mappable_alternatives_<
                Qp_, ForwardLike_t<Self_, Tp_>, ForwardLike_t<Self_, Args_>...>)
    constexpr auto map [[nodiscard]] (this Self_ &&self, Qp_ &&op) {
        using ResultOption =
            details::__map_option_return_type_t<Qp_, ForwardLike_t<Self_, Tp_>,
                                                ForwardLike_t<Self_, Args_>...>;

        return std::forward<Self_>(self).match(
            SomeAny >>= [&](auto &&val) -> ResultOption {
                using Arg_ = decltype(val);
                using Res_ = details::__map_single_result_t<Qp_, Arg_>;
                auto out =
                    std::invoke(std::forward<Qp_>(op), std::forward<Arg_>(val));
                if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                              !std::is_const_v<
                                  std::remove_reference_t<Self_>>) {
                    self.reset();
                }
                return ResultOption(Opt::Some<Res_>(std::move(out)));
            },
            None >>= []() -> ResultOption { return ResultOption(None); });
    }

  public:
    template <typename Target_, typename Self_, typename Qp_>
        requires((std::same_as<Target_, Tp_> ||
                  (std::same_as<Target_, Args_> || ...)) &&
                 std::same_as<std::remove_cvref_t<Self_>, Option> &&
                 details::__map_invocable_non_void_<
                     Qp_, ForwardLike_t<Self_, Target_>>)
    constexpr auto
    map [[nodiscard]] (this Self_ &&self, Qp_ &&op) -> Opt::Option<
        details::__map_single_result_t<Qp_, ForwardLike_t<Self_, Target_>>> {
        using Res_ =
            details::__map_single_result_t<Qp_, ForwardLike_t<Self_, Target_>>;
        using ResultOption = Opt::Option<Res_>;

        return std::forward<Self_>(self).match(
            SomeAny >>= [&](auto &&val) -> ResultOption {
                using Actual = std::remove_cvref_t<decltype(val)>;
                if constexpr (std::same_as<Actual, Target_>) {
                    auto out = std::invoke(std::forward<Qp_>(op),
                                           std::forward<decltype(val)>(val));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return ResultOption(Opt::Some<Res_>(std::move(out)));
                } else {
                    return ResultOption(None);
                }
            },
            None >>= []() -> ResultOption { return ResultOption(None); });
    }

  public:
    template <typename Self_, typename Qp_, typename Up_>
        requires(std::same_as<std::remove_cvref_t<Self_>, Option> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_> &&
                 details::__all_map_or_valid_<std::remove_cvref_t<Up_>, Qp_,
                                              ForwardLike_t<Self_, Tp_>,
                                              ForwardLike_t<Self_, Args_>...>)
    constexpr auto map_or [[nodiscard]] (this Self_ &&self, Qp_ &&op, Up_ &&v_)
        -> std::remove_cvref_t<Up_> {
        using Rp_ = std::remove_cvref_t<Up_>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    auto out = static_cast<Rp_>(
                        std::invoke(std::forward<Qp_>(op),
                                    std::forward<decltype(val)>(val)));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                },
                None >>= [&]() -> Rp_ {
                    return static_cast<Rp_>(std::forward<Up_>(v_));
                })
            .unwrap();
    }

  public:
    template <typename Target_, typename Self_, typename Qp_, typename Up_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>, Up_> &&
            details::__map_invocable_non_void_<Qp_,
                                               ForwardLike_t<Self_, Target_>> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>,
                                    details::__map_single_result_t<
                                        Qp_, ForwardLike_t<Self_, Target_>>>)
    constexpr auto map_or [[nodiscard]] (this Self_ &&self, Qp_ &&op, Up_ &&v_)
        -> std::remove_cvref_t<Up_> {
        using Rp_ = std::remove_cvref_t<Up_>;
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> Rp_ {
                    using Actual = std::remove_cvref_t<decltype(val)>;
                    if constexpr (std::same_as<Actual, Target_>) {
                        auto out = static_cast<Rp_>(
                            std::invoke(std::forward<Qp_>(op),
                                        std::forward<decltype(val)>(val)));
                        if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                      !std::is_const_v<
                                          std::remove_reference_t<Self_>>) {
                            self.reset();
                        }
                        return out;
                    } else {
                        return static_cast<Rp_>(std::forward<Up_>(v_));
                    }
                },
                None >>= [&]() -> Rp_ {
                    return static_cast<Rp_>(std::forward<Up_>(v_));
                })
            .unwrap();
    }

  public:
    template <typename Qp_ = Tp_, typename... CtorArgs_>
    constexpr auto emplace(CtorArgs_ &&...args) & noexcept(
        std::is_nothrow_constructible_v<Qp_, CtorArgs_...>)
        [[RL_LIFETIME_BOUND]] -> Qp_ &
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_constructible_v<Qp_, CtorArgs_...>)
    {
        return this->data.template emplace_<Qp_>(
            std::forward<CtorArgs_>(args)...);
    }

  public:
    template <typename... CtorArgs_>
    constexpr auto emplace(CtorArgs_ &&...args) && -> Tp_
        requires(std::is_constructible_v<Tp_, CtorArgs_...> &&
                 std::is_move_constructible_v<Tp_>)
    {
        this->template emplace<Tp_>(std::forward<CtorArgs_>(args)...);

        return this->take_unchecked();
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__all_and_then_alternatives_<
                Qp_, ForwardLike_t<Self_, Tp_>, ForwardLike_t<Self_, Args_>...>)
    constexpr auto and_then [[nodiscard]] (this Self_ &&self, Qp_ &&op) {
        using ResultOption =
            details::__and_then_return_type_t<Qp_, ForwardLike_t<Self_, Tp_>,
                                              ForwardLike_t<Self_, Args_>...>;

        return std::forward<Self_>(self).match(
            SomeAny >>= [&](auto &&val) -> ResultOption {
                auto res = std::invoke(std::forward<Qp_>(op),
                                       std::forward<decltype(val)>(val));
                if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                              !std::is_const_v<
                                  std::remove_reference_t<Self_>>) {
                    self.reset();
                }
                using ResOpt_ = std::remove_cvref_t<decltype(res)>;
                if constexpr (std::same_as<ResOpt_, ResultOption>) {
                    return std::move(res);
                } else {
                    return std::move(res).match(
                        SomeAny >>= [](auto &&inner_val) -> ResultOption {
                            return ResultOption(Opt::Some(
                                std::forward<decltype(inner_val)>(inner_val)));
                        },
                        None >>=
                        []() -> ResultOption { return ResultOption(None); });
                }
            },
            None >>= []() -> ResultOption { return ResultOption(None); });
    }

  public:
    template <typename Target_, typename Self_, typename Qp_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__and_then_invocable_<Qp_, ForwardLike_t<Self_, Target_>>)
    constexpr auto and_then [[nodiscard]] (this Self_ &&self, Qp_ &&op)
        -> std::remove_cvref_t<
            std::invoke_result_t<Qp_, ForwardLike_t<Self_, Target_>>> {
        using ResultOption = std::remove_cvref_t<
            std::invoke_result_t<Qp_, ForwardLike_t<Self_, Target_>>>;

        return std::forward<Self_>(self).match(
            SomeAny >>= [&](auto &&val) -> ResultOption {
                using Actual = std::remove_cvref_t<decltype(val)>;
                if constexpr (std::same_as<Actual, Target_>) {
                    auto out = std::invoke(std::forward<Qp_>(op),
                                           std::forward<decltype(val)>(val));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                } else {
                    return ResultOption(None);
                }
            },
            None >>= []() -> ResultOption { return ResultOption(None); });
    }

  public:
    template <typename... Rp_, typename Self_, typename... Arms_>
    constexpr auto match [[nodiscard]] (this Self_ &&self, Arms_ &&...arms)
        requires(sizeof...(Arms_) > 0 &&
                 details::match_is_exhaustive_<std::tuple<Tp_, Args_...>,
                                               std::tuple<Arms_...>>::value &&
                 ((sizeof...(Rp_) == 0 &&
                   details::__match_inferred_results_convertible_<
                       std::tuple<ForwardLike_t<Self_, Tp_>,
                                  ForwardLike_t<Self_, Args_>...>,
                       std::tuple<Arms_...>>::value) ||
                  (sizeof...(Rp_) == 1 &&
                   ((std::same_as<Rp_, void> || ...) ||
                    !details::__match_return_types_are_consistent_<
                        std::tuple<ForwardLike_t<Self_, Tp_>,
                                   ForwardLike_t<Self_, Args_>...>,
                        std::tuple<Arms_...>>::has_void)) ||
                  (sizeof...(Rp_) > 1 &&
                   !details::__match_return_types_are_consistent_<
                       std::tuple<ForwardLike_t<Self_, Tp_>,
                                  ForwardLike_t<Self_, Args_>...>,
                       std::tuple<Arms_...>>::has_void &&
                   (!std::is_void_v<Rp_> && ...))) &&
                 (sizeof...(Rp_) == 0 ||
                  details::__match_explicit_results_convertible_<
                      std::tuple<Rp_...>,
                      typename details::__match_return_types_are_consistent_<
                          std::tuple<ForwardLike_t<Self_, Tp_>,
                                     ForwardLike_t<Self_, Args_>...>,
                          std::tuple<Arms_...>>::type>::value))
    {
        using ReturnType_ = typename details::__match_unwrapped_return_type_<
            std::tuple<Rp_...>,
            std::tuple<ForwardLike_t<Self_, Tp_>,
                       ForwardLike_t<Self_, Args_>...>,
            Arms_...>::type;

        static_assert((details::match_arm_covers_none_<Arms_>() || ...),
                      "match(): no arm covers None");
        static_assert((details::match_arm_matches_type_<Arms_, Tp_>() || ...),
                      "match(): no arm covers the primary type");
        static_assert(
            details::match_all_types_covered_<std::tuple<Tp_, Args_...>,
                                              std::tuple<Arms_...>>::value,
            "match(): an alternative type has no covering arm");
        static_assert(
            details::match_all_types_exhaustive_<std::tuple<Tp_, Args_...>,
                                                 std::tuple<Arms_...>>::value,
            "match(): alternative types are not exhaustively covered; provide "
            "a Wild or SomeAny fallback arm, or cover both true and false for "
            "bool");

        const std::size_t active_{self.active_index_()};
        if (active_ == 0) {
            return details::match_none_<ReturnType_>(arms...);
        }

        auto dispatch_some_ =
            [&]<std::size_t Index_ = 0>(auto &&dispatch_) -> ReturnType_ {
            if constexpr (Index_ < sizeof...(Args_) + 1) {
                using CurrentType_ =
                    details::__pack_element_t<Index_, Tp_, Args_...>;

                if (active_ == Index_ + 1) {
                    using Value_ = ForwardLike_t<Self_, CurrentType_>;
                    auto &&value_ = std::forward_like<Self_>(
                        self.template get_<CurrentType_>());

                    return details::match_some_<ReturnType_, CurrentType_,
                                                Value_>(
                        self.template get_type_name<Tp_>(),
                        std::forward<Value_>(value_), arms...);
                }

                return dispatch_.template operator()<Index_ + 1>(dispatch_);
            } else {
                std::unreachable();
            }
        };

        return dispatch_some_.template operator()<0>(dispatch_some_);
    }

  public:
    constexpr auto expect
        [[nodiscard]] (std::string_view msg) const &[[RL_LIFETIME_BOUND]]
        -> const Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::expect() failed: {}",
                            this->template get_type_name<Tp_>(), msg);
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto expect
        [[nodiscard]] (std::string_view msg) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::expect() failed: {}",
                            this->template get_type_name<Tp_>(), msg);
        }

        return this->primary_value_();
    }

  public:
    constexpr auto expect [[nodiscard]] (std::string_view msg) && -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::expect() failed: {}",
                            this->template get_type_name<Tp_>(), msg);
        }

        return this->take_unchecked();
    }

  public:
    constexpr auto expect [[nodiscard]] (std::string_view msg) const && -> Tp_
        requires(std::is_copy_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::expect() failed: {}",
                            this->template get_type_name<Tp_>(), msg);
        }

        return this->primary_value_();
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ & {
        if (!this->has_primary_()) {
            details::panic_(
                "Option<{}>::unwrap() failed: Option does not wield any value.",
                this->template get_type_name<Tp_>());
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            details::panic_(
                "Option<{}>::unwrap() failed: Option does not wield any value.",
                this->template get_type_name<Tp_>());
        }

        return this->primary_value_();
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) && -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            details::panic_(
                "Option<{}>::unwrap() failed: Option does not wield any value.",
                this->template get_type_name<Tp_>());
        }

        return this->take_unchecked();
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) const && -> Tp_
        requires(std::is_copy_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            details::panic_(
                "Option<{}>::unwrap(): Option does not wield any value.",
                this->template get_type_name<Tp_>());
        }

        return this->primary_value_();
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Qp_ &
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            details::panic_("Option<{}>::unwrap<{}>() failed: active "
                            "alternative does not match.",
                            this->template get_type_name<Tp_>(),
                            this->template get_type_name<Qp_>());
        }

        return this->template get_<Qp_>();
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Qp_ &
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            details::panic_("Option<{}>::unwrap<{}>() failed: active "
                            "alternative does not match.",
                            this->template get_type_name<Tp_>(),
                            this->template get_type_name<Qp_>());
        }

        return this->template get_<Qp_>();
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) && -> Qp_
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_move_constructible_v<Qp_>)
    {
        if (!this->template has_type_<Qp_>()) {
            details::panic_("Option<{}>::unwrap<{}>() failed: active "
                            "alternative does not match.",
                            this->template get_type_name<Tp_>(),
                            this->template get_type_name<Qp_>());
        }

        // ResetOnSuccess_ is a local RAII guard; mixed int+pointer layout
        // needs a gap before owner_. It's declared explicitly below (not
        // left implicit) so there's nothing for -Wpadded to flag.
        struct alignas(void *) ResetOnSuccess_ {
            // initial_uncaught_ first to avoid tail-padding
            // between int and pointer members.
            int initial_uncaught_ = 0;

            [[no_unique_address]] Pad_<
                align_up_(sizeof(int), alignof(Option *)) - sizeof(int)> _pad{};

            Option *owner_;

            constexpr explicit ResetOnSuccess_(Option *owner
                                               [[RL_LIFETIME_BOUND]]) noexcept
                : owner_(owner) {
#if defined(__cpp_exceptions)
                if !consteval {
                    initial_uncaught_ = std::uncaught_exceptions();
                }
#endif
            }

            constexpr ~ResetOnSuccess_(void) noexcept {
#if defined(__cpp_exceptions)
                if !consteval {
                    if (std::uncaught_exceptions() > initial_uncaught_) {
                        return;
                    }
                }
#endif
                owner_->reset();
            }
        };

        ResetOnSuccess_ guard_{this};
        return static_cast<Qp_ &&>(this->template get_<Qp_>());
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) const && -> Qp_
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::is_copy_constructible_v<Qp_>)
    {
        if (!this->template has_type_<Qp_>()) {
            details::panic_("Option<{}>::unwrap<{}>() failed: active "
                            "alternative does not match.",
                            this->template get_type_name<Tp_>(),
                            this->template get_type_name<Qp_>());
        }

        return this->template get_<Qp_>();
    }

  public:
    constexpr auto value [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::value() failed - Bad access: Option "
                            "does not hold a value!",
                            this->template get_type_name<Tp_>());
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto value [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::value() failed - Bad access: Option "
                            "does not hold a value!",
                            this->template get_type_name<Tp_>());
        }

        return this->primary_value_();
    }

  public:
    constexpr auto value [[nodiscard]] (void) &&[[RL_LIFETIME_BOUND]]
    -> Tp_ && {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::value() failed - Bad access: Option "
                            "does not hold a value!",
                            this->template get_type_name<Tp_>());
        }

        return {std::move(this->primary_value_())};
    }

  public:
    constexpr auto value [[nodiscard]] (void) const &&[[RL_LIFETIME_BOUND]]
    -> const Tp_ && {
        if (!this->has_primary_()) {
            details::panic_("Option<{}>::value() failed - Bad access: Option "
                            "does not hold a value!",
                            this->template get_type_name<Tp_>());
        }

        return {std::move(this->primary_value_())};
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) const & -> Opt::None {
        if (this->is_some()) {
            details::panic_("Option<{}>::unwrap_none() failed - Bad access: "
                            "Option holds a value!",
                            this->template get_type_name<Tp_>());
        }

        return Opt::None();
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) & -> Opt::None {
        if (this->is_some()) {
            details::panic_("Option<{}>::unwrap_none() failed - Bad access: "
                            "Option holds a value!",
                            this->template get_type_name<Tp_>());
        }

        return Opt::None();
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) && -> Opt::None {
        if (this->is_some()) {
            details::panic_("Option<{}>::unwrap_none() failed - Bad access: "
                            "Option holds a value!",
                            this->template get_type_name<Tp_>());
        }

        return Opt::None();
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) const && -> Opt::None {
        if (this->is_some()) {
            details::panic_("Option<{}>::unwrap_none() failed - Bad access: "
                            "Option holds a value!",
                            this->template get_type_name<Tp_>());
        }

        return Opt::None();
    }

  public:
    constexpr auto reset(void) & noexcept(true) -> void { this->data.reset_(); }

  private:
    template <typename Fn_>
    constexpr auto consume_unchecked [[nodiscard]] (Fn_ &&fn) & noexcept(
        std::is_nothrow_invocable_v<Fn_, Tp_ &&>)
        -> std::invoke_result_t<Fn_, Tp_ &&>
        requires(std::invocable<Fn_, Tp_ &&> &&
                 !std::is_reference_v<std::invoke_result_t<Fn_, Tp_ &&>>)
    {
        struct alignas(void *) ResetOnExit_ {
            Option *owner_;

            constexpr ~ResetOnExit_() noexcept { owner_->reset(); }
        };

        ResetOnExit_ reset_{this};

        return (std::invoke(std::forward<Fn_>(fn),
                            std::move(this->primary_value_())));
    }

    // Extract by value. If the move throws, the Option stays
    // engaged.
  private:
    constexpr auto take_unchecked [[nodiscard]] (void) & noexcept(
        std::is_nothrow_move_constructible_v<Tp_>) -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        // ResetOnSuccess_ is a local RAII guard; mixed int+pointer layout
        // needs a gap before owner_. It's declared explicitly below (not
        // left implicit) so there's nothing for -Wpadded to flag.
        struct alignas(void *) ResetOnSuccess_ {
            // initial_uncaught_ first to avoid tail-padding
            // between int and pointer members.
            int initial_uncaught_ = 0;

            [[no_unique_address]] Pad_<
                align_up_(sizeof(int), alignof(Option *)) - sizeof(int)> _pad{};

            Option *owner_;

            constexpr explicit ResetOnSuccess_(Option *owner
                                               [[RL_LIFETIME_BOUND]]) noexcept
                : owner_(owner) {
#if defined(__cpp_exceptions)
                if !consteval {
                    initial_uncaught_ = std::uncaught_exceptions();
                }
#endif
            }

            constexpr ~ResetOnSuccess_(void) noexcept {
#if defined(__cpp_exceptions)
                if !consteval {
                    if (std::uncaught_exceptions() > initial_uncaught_) {
                        return;
                    }
                }
#endif
                owner_->reset();
            }
        };

        ResetOnSuccess_ guard_{this};
        return static_cast<Tp_ &&>(this->primary_value_());
    }

// as_mut_ptr / as_immut_ptr return Option<T*> by value; the internal pointer
// points into *this. Clang cannot verify the lifetimebound through the
// Option<T*> wrapper — suppress the false-positive violation.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-lifetimebound-violation"
#endif
  public:
    constexpr auto as_mut_ptr [[nodiscard]] (void) & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> Option<Tp_ *> {
        if (!this->has_primary_()) {
            return Opt::None();
        }

        return (Opt::Some<Tp_ *>(std::addressof(this->primary_value_())));
    }

  public:
    constexpr auto as_mut_ptr(void) && = delete;

  public:
    constexpr auto as_mut_ptr(void) const && = delete;

  public:
    template <typename Qp_>
    constexpr auto as_mut_ptr [[nodiscard]] (void) & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> Option<Qp_ *>
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            return Opt::None();
        }

        return (Opt::Some<Qp_ *>(std::addressof(this->template get_<Qp_>())));
    }

  public:
    template <typename Qp_> constexpr auto as_mut_ptr(void) && = delete;

  public:
    template <typename Qp_> constexpr auto as_mut_ptr(void) const && = delete;

  public:
    constexpr auto as_immut_ptr [[nodiscard]] (void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> Option<const Tp_ *> {
        if (!this->has_primary_()) {
            return Opt::None();
        }

        return (Opt::Some<const Tp_ *>(std::addressof(this->primary_value_())));
    }

  public:
    constexpr auto as_immut_ptr(void) && = delete;

  public:
    constexpr auto as_immut_ptr(void) const && = delete;

  public:
    template <typename Qp_>
    constexpr auto as_immut_ptr [[nodiscard]] (void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> Option<const Qp_ *>
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            return Opt::None();
        }

        return (
            Opt::Some<const Qp_ *>(std::addressof(this->template get_<Qp_>())));
    }

  public:
    template <typename Qp_> constexpr auto as_immut_ptr(void) && = delete;

  public:
    template <typename Qp_> constexpr auto as_immut_ptr(void) const && = delete;

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  public:
    constexpr auto take [[nodiscard]] (void) & noexcept(
        std::is_nothrow_move_constructible_v<Tp_> &&
        (std::is_nothrow_move_constructible_v<Args_> && ...))
        -> Option<Tp_, Args_...>
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...))
    {
        Option<Tp_, Args_...> out_ = std::move(*this);
        this->reset();
        return out_;
    }

  public:
    constexpr auto take(void) && = delete;

  public:
    constexpr auto take(void) const && = delete;

  public:
    template <typename Up_>
    constexpr auto replace [[nodiscard]] (Up_ &&new_val) & noexcept(
        std::is_nothrow_move_constructible_v<Tp_> &&
        (std::is_nothrow_move_constructible_v<Args_> && ...) &&
        std::is_nothrow_constructible_v<
            typename details::match_alternative_<Up_, Tp_, Args_...>::type,
            Up_>) -> Option<Tp_, Args_...>
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 details::match_alternative_<Up_, Tp_, Args_...>::valid)
    {
        using Target_ =
            typename details::match_alternative_<Up_, Tp_, Args_...>::type;

        Target_ replacement_(std::forward<Up_>(new_val));
        Option<Tp_, Args_...> old_ = this->take();
        this->data.template emplace_<Target_>(std::move(replacement_));
        return old_;
    }

  public:
    template <typename Up_> constexpr auto replace(Up_ &&) && = delete;

  public:
    template <typename Up_> constexpr auto replace(Up_ &&) const && = delete;

  public:
    constexpr auto is_none [[nodiscard]] (void) const noexcept(true) -> bool {
        return (this->active_index_() == 0);
    }

  public:
    constexpr auto is_some [[nodiscard]] (void) const noexcept(true) -> bool {
        return (this->active_index_() != 0);
    }

  public:
    template <typename Qp_>
    constexpr auto is_some [[nodiscard]] (void) const noexcept(true) -> bool {
        return this->template has_type_<Qp_>();
    }

  public:
    constexpr auto is_default [[nodiscard]] (void) const
        noexcept(std::is_nothrow_default_constructible_v<Tp_> &&
                 noexcept(std::declval<const Tp_ &>() ==
                          std::declval<const Tp_ &>())) -> bool
        requires(std::is_default_constructible_v<Tp_> &&
                 std::equality_comparable<Tp_>)
    {
        return match(
                   SomeAny >>= [](auto const &val) -> bool {
                       using Actual_ = std::remove_cvref_t<decltype(val)>;
                       if constexpr (std::same_as<Actual_, Tp_>) {
                           const Tp_ def_{};
                           return val == def_;
                       } else {
                           return false;
                       }
                   },
                   None >>= []() -> bool { return false; })
            .unwrap();
    }

  public:
    constexpr auto is_default [[nodiscard]] (void) const noexcept(
        std::is_nothrow_default_constructible_v<default_alternative_type_> &&
        noexcept(std::declval<const default_alternative_type_ &>() ==
                 std::declval<const default_alternative_type_ &>())) -> bool
        requires(!std::is_default_constructible_v<Tp_> &&
                 has_default_alternative_ &&
                 std::equality_comparable<default_alternative_type_>)
    {
        if (!this->template has_type_<default_alternative_type_>()) {
            return false;
        }
        const default_alternative_type_ def_{};
        return this->template get_<default_alternative_type_>() == def_;
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__all_predicate_invocable_<Qp_, ForwardLike_t<Self_, Tp_>,
                                                ForwardLike_t<Self_, Args_>...>)
    constexpr auto
    is_some_and [[nodiscard]] (this Self_ &&self, Qp_ &&prop_) noexcept((
        std::is_nothrow_invocable_v<Qp_, ForwardLike_t<Self_, Tp_>> &&
        (std::is_nothrow_invocable_v<Qp_, ForwardLike_t<Self_, Args_>> && ...)))
        -> bool {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> bool {
                    auto out = static_cast<bool>(
                        std::invoke(std::forward<Qp_>(prop_),
                                    std::forward<decltype(val)>(val)));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                },
                None >>= []() -> bool { return false; })
            .unwrap();
    }

  public:
    template <typename Target_, typename Self_, typename Qp_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__predicate_invocable_<Qp_, ForwardLike_t<Self_, Target_>>)
    constexpr auto is_some_and [[nodiscard]] (this Self_ &&self, Qp_ &&prop_)
        -> bool {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> bool {
                    using Actual = std::remove_cvref_t<decltype(val)>;
                    if constexpr (std::same_as<Actual, Target_>) {
                        auto out = static_cast<bool>(
                            std::invoke(std::forward<Qp_>(prop_),
                                        std::forward<decltype(val)>(val)));
                        if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                      !std::is_const_v<
                                          std::remove_reference_t<Self_>>) {
                            self.reset();
                        }
                        return out;
                    } else {
                        return false;
                    }
                },
                None >>= []() -> bool { return false; })
            .unwrap();
    }

  public:
    template <typename Self_, typename Qp_>
        requires(
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__all_predicate_invocable_<Qp_, ForwardLike_t<Self_, Tp_>,
                                                ForwardLike_t<Self_, Args_>...>)
    constexpr auto
    is_none_or [[nodiscard]] (this Self_ &&self, Qp_ &&prop_) noexcept((
        std::is_nothrow_invocable_v<Qp_, ForwardLike_t<Self_, Tp_>> &&
        (std::is_nothrow_invocable_v<Qp_, ForwardLike_t<Self_, Args_>> && ...)))
        -> bool {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> bool {
                    auto out = static_cast<bool>(
                        std::invoke(std::forward<Qp_>(prop_),
                                    std::forward<decltype(val)>(val)));
                    if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                  !std::is_const_v<
                                      std::remove_reference_t<Self_>>) {
                        self.reset();
                    }
                    return out;
                },
                None >>= []() -> bool { return true; })
            .unwrap();
    }

  public:
    template <typename Target_, typename Self_, typename Qp_>
        requires(
            (std::same_as<Target_, Tp_> ||
             (std::same_as<Target_, Args_> || ...)) &&
            std::same_as<std::remove_cvref_t<Self_>, Option> &&
            details::__predicate_invocable_<Qp_, ForwardLike_t<Self_, Target_>>)
    constexpr auto is_none_or [[nodiscard]] (this Self_ &&self, Qp_ &&prop_)
        -> bool {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [&](auto &&val) -> bool {
                    using Actual = std::remove_cvref_t<decltype(val)>;
                    if constexpr (std::same_as<Actual, Target_>) {
                        auto out = static_cast<bool>(
                            std::invoke(std::forward<Qp_>(prop_),
                                        std::forward<decltype(val)>(val)));
                        if constexpr (std::is_rvalue_reference_v<Self_ &&> &&
                                      !std::is_const_v<
                                          std::remove_reference_t<Self_>>) {
                            self.reset();
                        }
                        return out;
                    } else {
                        return true;
                    }
                },
                None >>= []() -> bool { return true; })
            .unwrap();
    }

  public:
    template <typename Self_, typename Qp_>
        requires(std::same_as<std::remove_cvref_t<Self_>, Option> &&
                 std::convertible_to<Qp_, Tp_> &&
                 (std::is_rvalue_reference_v<Self_ &&>
                      ? std::is_move_constructible_v<Tp_>
                      : std::is_copy_constructible_v<Tp_>))
    constexpr auto unwrap_or
        [[nodiscard]] (this Self_ &&self, Qp_ &&v_) noexcept(
            std::is_rvalue_reference_v<Self_ &&>
                ? (std::is_nothrow_move_constructible_v<Tp_> &&
                   std::is_nothrow_constructible_v<Tp_, Qp_>)
                : (std::is_nothrow_copy_constructible_v<Tp_> &&
                   std::is_nothrow_constructible_v<Tp_, Qp_>)) -> Tp_ {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [](auto &&val) -> Tp_ {
                    return static_cast<Tp_>(std::forward<decltype(val)>(val));
                },
                None >>= [&]() -> Tp_ {
                    return static_cast<Tp_>(std::forward<Qp_>(v_));
                })
            .unwrap();
    }

  public:
    constexpr auto unwrap_unchecked [[nodiscard]] (void) const & noexcept(
        std::is_nothrow_copy_constructible_v<Tp_>) -> Tp_
        requires(std::is_copy_constructible_v<Tp_>)
    {
        return *static_cast<const Tp_ *>(
            std::addressof(this->data.storage.value));
    }

  public:
    constexpr auto unwrap_unchecked [[nodiscard]] (void) & noexcept(
        std::is_nothrow_copy_constructible_v<Tp_>) -> Tp_
        requires(std::is_copy_constructible_v<Tp_>)
    {
        return *static_cast<Tp_ *>(std::addressof(this->data.storage.value));
    }

  public:
    constexpr auto unwrap_unchecked [[nodiscard]] (void) && noexcept(
        std::is_nothrow_move_constructible_v<Tp_>) -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        return this->take_unchecked();
    }

  public:
    constexpr auto unwrap_unchecked [[nodiscard]] (void) const && noexcept(
        std::is_nothrow_constructible_v<Tp_, const Tp_ &&>) -> Tp_
        requires(std::is_constructible_v<Tp_, const Tp_ &&>)
    {
        return std::move(*static_cast<const Tp_ *>(
            std::addressof(this->data.storage.value)));
    }

  public:
    template <typename Self_>
        requires(std::same_as<std::remove_cvref_t<Self_>, Option> &&
                 std::is_default_constructible_v<Tp_> &&
                 (std::is_rvalue_reference_v<Self_ &&>
                      ? std::is_move_constructible_v<Tp_>
                      : std::is_copy_constructible_v<Tp_>))
    constexpr auto unwrap_or_default [[nodiscard]] (this Self_ &&self) noexcept(
        std::is_nothrow_default_constructible_v<Tp_> &&
        (std::is_rvalue_reference_v<Self_ &&>
             ? std::is_nothrow_move_constructible_v<Tp_>
             : std::is_nothrow_copy_constructible_v<Tp_>)) -> Tp_ {
        return std::forward<Self_>(self)
            .match(
                SomeAny >>= [](auto &&val) -> Tp_ {
                    return static_cast<Tp_>(std::forward<decltype(val)>(val));
                },
                None >>= []() -> Tp_ { return Tp_{}; })
            .unwrap();
    }

  public:
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     const Option<Tp_, Args_...> &rhs) -> bool
        requires(std::equality_comparable<Tp_> &&
                 (std::equality_comparable<Args_> && ...))
    {
        const std::size_t lhs_index_{lhs.active_index_()};

        if (lhs_index_ != rhs.active_index_()) {
            return false;
        }

        if (lhs_index_ == 0) {
            return true;
        }

        return Option::equal_active_(lhs_index_, lhs, rhs);
    }

  public:
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      const Option<Tp_, Args_...> &rhs)
        -> details::__compare_three_way_results_t<Tp_, Args_...>
        requires(std::three_way_comparable<Tp_> &&
                 (std::three_way_comparable<Args_> && ...))
    {
        using Category_ = details::__compare_three_way_results_t<Tp_, Args_...>;
        const std::size_t lhs_index_{lhs.active_index_()};
        const std::size_t rhs_index_{rhs.active_index_()};

        if (lhs_index_ < rhs_index_) {
            return Category_::less;
        }
        if (lhs_index_ > rhs_index_) {
            return Category_::greater;
        }
        if (lhs_index_ == 0) {
            return Category_::equivalent;
        }
        return Option::compare_active_(lhs_index_, lhs, rhs);
    }

  public:
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      typename Opt::None) noexcept(true)
        -> std::strong_ordering {
        return lhs.is_some() <=> false;
    }

  public:
    friend constexpr auto
    operator<=>(typename Opt::None,
                const Option<Tp_, Args_...> &rhs) noexcept(true)
        -> std::strong_ordering {
        return false <=> rhs.is_some();
    }

  public:
    template <typename Qp_>
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      const Opt::Some<Qp_> &rhs)
        -> std::compare_three_way_result_t<Qp_>
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::three_way_comparable<Qp_>)
    {
        using Cat_ = std::compare_three_way_result_t<Qp_>;
        if constexpr (std::same_as<Qp_, Tp_>) {
            if (!lhs.has_primary_()) {
                return static_cast<Cat_>(std::strong_ordering::less);
            }
            return static_cast<Cat_>(lhs.primary_value_() <=> rhs.get());
        } else {
            if (!lhs.template has_type_<Qp_>()) {
                return static_cast<Cat_>(std::strong_ordering::less);
            }
            return static_cast<Cat_>(lhs.template get_<Qp_>() <=> rhs.get());
        }
    }

  public:
    template <typename Qp_>
    friend constexpr auto operator<=>(const Opt::Some<Qp_> &lhs,
                                      const Option<Tp_, Args_...> &rhs)
        -> std::compare_three_way_result_t<Qp_>
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::three_way_comparable<Qp_>)
    {
        using Cat_ = std::compare_three_way_result_t<Qp_>;
        if constexpr (std::same_as<Qp_, Tp_>) {
            if (!rhs.has_primary_()) {
                return static_cast<Cat_>(std::strong_ordering::greater);
            }
            return static_cast<Cat_>(lhs.get() <=> rhs.primary_value_());
        } else {
            if (!rhs.template has_type_<Qp_>()) {
                return static_cast<Cat_>(std::strong_ordering::greater);
            }
            return static_cast<Cat_>(lhs.get() <=> rhs.template get_<Qp_>());
        }
    }

  public:
    template <typename Up_>
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      const Up_ &rhs)
        -> details::__option_cmp_with_category_t<Up_, Tp_, Args_...>
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 (std::three_way_comparable_with<Tp_, Up_> ||
                  (std::three_way_comparable_with<Args_, Up_> || ...)))
    {
        using Cat_ = details::__option_cmp_with_category_t<Up_, Tp_, Args_...>;
        if (lhs.is_none()) {
            return static_cast<Cat_>(std::strong_ordering::less);
        }
        if constexpr (std::three_way_comparable_with<Tp_, Up_>) {
            if (lhs.has_primary_()) {
                return static_cast<Cat_>(lhs.primary_value_() <=> rhs);
            }
        }
        auto check_alts = [&]<std::size_t Index_ = 0>(auto &&self_fn) -> Cat_ {
            if constexpr (Index_ < sizeof...(Args_)) {
                using Alt_ = details::__pack_element_t<Index_, Args_...>;
                if constexpr (std::three_way_comparable_with<Alt_, Up_>) {
                    if (lhs.template has_type_<Alt_>()) {
                        return static_cast<Cat_>(lhs.template get_<Alt_>() <=>
                                                 rhs);
                    }
                }
                return self_fn.template operator()<Index_ + 1>(self_fn);
            } else {
                return static_cast<Cat_>(std::strong_ordering::less);
            }
        };
        return check_alts.template operator()<0>(check_alts);
    }

  public:
    template <typename Up_>
    friend constexpr auto operator<=>(const Up_ &lhs,
                                      const Option<Tp_, Args_...> &rhs)
        -> details::__option_cmp_with_category_t<Up_, Tp_, Args_...>
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 (std::three_way_comparable_with<Tp_, Up_> ||
                  (std::three_way_comparable_with<Args_, Up_> || ...)))
    {
        using Cat_ = details::__option_cmp_with_category_t<Up_, Tp_, Args_...>;
        if (rhs.is_none()) {
            return static_cast<Cat_>(std::strong_ordering::greater);
        }
        if constexpr (std::three_way_comparable_with<Up_, Tp_>) {
            if (rhs.has_primary_()) {
                return static_cast<Cat_>(lhs <=> rhs.primary_value_());
            }
        }
        auto check_alts = [&]<std::size_t Index_ = 0>(auto &&self_fn) -> Cat_ {
            if constexpr (Index_ < sizeof...(Args_)) {
                using Alt_ = details::__pack_element_t<Index_, Args_...>;
                if constexpr (std::three_way_comparable_with<Up_, Alt_>) {
                    if (rhs.template has_type_<Alt_>()) {
                        return static_cast<Cat_>(lhs <=>
                                                 rhs.template get_<Alt_>());
                    }
                }
                return self_fn.template operator()<Index_ + 1>(self_fn);
            } else {
                return static_cast<Cat_>(std::strong_ordering::greater);
            }
        };
        return check_alts.template operator()<0>(check_alts);
    }

  public:
    constexpr auto swap(Option &other) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_swappable_v<Tp_> &&
         (std::is_nothrow_swappable_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::swappable<Tp_> && (std::swappable<Args_> && ...))
    {
        this->data.swap_(other.data);
    }

  public:
    friend constexpr auto swap(Option &lhs, Option &rhs) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_swappable_v<Tp_> &&
         (std::is_nothrow_swappable_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::swappable<Tp_> && (std::swappable<Args_> && ...))
    {
        lhs.swap(rhs);
    }

  public:
    constexpr auto operator* [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ & {
        return {this->value()};
    }

  public:
    constexpr auto operator* [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]]
    -> Tp_ & {
        return this->value();
    }

  public:
    constexpr auto operator* [[nodiscard]] (void) &&[[RL_LIFETIME_BOUND]]
    -> Tp_ && {
        return std::move(this->value());
    }

  public:
    constexpr auto operator* [[nodiscard]] (void) const &&[[RL_LIFETIME_BOUND]]
    -> const Tp_ && {
        return std::move(this->value());
    }

  public:
    constexpr auto operator->[[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ * {
        return std::addressof(this->value());
    }

  public:
    constexpr auto operator->[[nodiscard]] (void) &[[RL_LIFETIME_BOUND]]
    -> Tp_ * {
        return std::addressof(this->value());
    }

  public:
    constexpr auto operator->[[nodiscard]] (void) &&[[RL_LIFETIME_BOUND]]
    -> Tp_ * {
        return std::addressof(this->value());
    }

  public:
    constexpr auto operator->[[nodiscard]] (void) const &&[[RL_LIFETIME_BOUND]]
    -> const Tp_ * {
        return std::addressof(this->value());
    }

  public:
    constexpr auto operator=(const typename Opt::Some<Tp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Tp_> &&
        (!std::is_copy_assignable_v<Tp_> ||
         std::is_nothrow_copy_assignable_v<Tp_>)) -> Option &
        requires(std::is_copy_constructible_v<Tp_>)
    {
        if constexpr (std::is_copy_assignable_v<Tp_>) {
            if (this->has_primary_()) {
                this->primary_value_() = some.get();
                return *this;
            }
        }
        Tp_ tmp(some.get());
        this->reset();
        this->data.template emplace_<Tp_>(std::move(tmp));
        return *this;
    }

  public:
    constexpr auto operator=(typename Opt::Some<Tp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Tp_> &&
        (!std::is_move_assignable_v<Tp_> ||
         std::is_nothrow_move_assignable_v<Tp_>)) -> Option &
        requires(std::is_move_constructible_v<Tp_>)
    {
        if constexpr (std::is_move_assignable_v<Tp_>) {
            if (this->has_primary_()) {
                this->primary_value_() = std::move(some).get();
                return *this;
            }
        }
        Tp_ tmp(std::move(some).get());
        this->reset();
        this->data.template emplace_<Tp_>(std::move(tmp));
        return *this;
    }

  public:
    template <typename Qp_>
    constexpr auto operator=(const typename Opt::Some<Qp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_> &&
        (!std::is_copy_assignable_v<Qp_> ||
         std::is_nothrow_copy_assignable_v<Qp_>)) -> Option &
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_copy_constructible_v<Qp_>)
    {
        if constexpr (std::is_copy_assignable_v<Qp_>) {
            if (this->template has_type_<Qp_>()) {
                this->template get_<Qp_>() = some.get();
                return *this;
            }
        }
        Qp_ tmp(some.get());
        this->reset();
        this->data.template emplace_<Qp_>(std::move(tmp));
        return *this;
    }

  public:
    template <typename Qp_>
    constexpr auto operator=(typename Opt::Some<Qp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Qp_> &&
        (!std::is_move_assignable_v<Qp_> ||
         std::is_nothrow_move_assignable_v<Qp_>)) -> Option &
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_move_constructible_v<Qp_>)
    {
        if constexpr (std::is_move_assignable_v<Qp_>) {
            if (this->template has_type_<Qp_>()) {
                this->template get_<Qp_>() = std::move(some).get();
                return *this;
            }
        }
        Qp_ tmp(std::move(some).get());
        this->reset();
        this->data.template emplace_<Qp_>(std::move(tmp));
        return *this;
    }

  public:
    constexpr auto operator=(typename Opt::None) noexcept(true) -> Option & {
        this->reset();
        return *this;
    }

  public:
    constexpr auto operator=(typename Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<Tp_> &&
        (std::is_move_assignable_v<Tp_> ? std::is_nothrow_move_assignable_v<Tp_>
         : std::is_copy_assignable_v<Tp_>
             ? std::is_nothrow_copy_assignable_v<Tp_>
             : true)) -> Option &
        requires(std::is_default_constructible_v<Tp_>)
    {
        if (this->has_primary_()) {
            if constexpr (std::is_move_assignable_v<Tp_>) {
                this->primary_value_() = Tp_{};
                return *this;
            }

            else if constexpr (std::is_copy_assignable_v<Tp_>) {
                const Tp_ def_{};
                this->primary_value_() = def_;
                return *this;
            }
        }
        this->emplace();
        return *this;
    }

  public:
    constexpr auto operator=(typename Opt::Default) noexcept(
        std::is_nothrow_default_constructible_v<default_alternative_type_>)
        -> Option &
        requires(!std::is_default_constructible_v<Tp_> &&
                 has_default_alternative_)
    {
        this->reset();
        this->data.template emplace_<default_alternative_type_>();
        return *this;
    }

  public:
    constexpr auto operator=(typename Opt::Some<void>) noexcept(
        noexcept(std::declval<Option &>() = Opt::Default{})) -> Option &
        requires(std::is_default_constructible_v<Tp_> ||
                 has_default_alternative_)
    {
        return (*this = Opt::Default{});
    }

  public:
    template <typename Up_, typename Target_ = typename details::
                                match_alternative_<Up_, Tp_, Args_...>::type>
    constexpr auto operator=(Up_ &&val_) -> Option &
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 details::match_alternative_<Up_, Tp_, Args_...>::valid)
    {
        if (this->template has_type_<Target_>()) {
            if constexpr (std::is_assignable_v<Target_ &, Up_>) {
                this->template get_<Target_>() = std::forward<Up_>(val_);
                return *this;
            }
        }

        if constexpr (std::is_move_constructible_v<Target_>) {
            // Construct while the old alternative is still alive. Besides
            // improving the exception guarantee, this lets converting
            // constructors safely read from references into the old value.
            Target_ replacement_(std::forward<Up_>(val_));
            this->data.template emplace_<Target_>(std::move(replacement_));
        } else if constexpr (std::is_constructible_v<std::decay_t<Up_>, Up_> &&
                             std::is_constructible_v<Target_,
                                                     std::decay_t<Up_> &&>) {
            // Target is non-movable; stage the argument if possible so reading
            // from references into the old alternative succeeds before
            // reset_(). Preserve an rvalue conversion when the target accepts
            // it.
            std::decay_t<Up_> staged_val_(std::forward<Up_>(val_));
            this->data.template emplace_<Target_>(std::move(staged_val_));
        } else if constexpr (std::is_constructible_v<std::decay_t<Up_>, Up_> &&
                             std::is_constructible_v<Target_,
                                                     std::decay_t<Up_> &>) {
            // Some non-movable targets accept only an lvalue source. Build a
            // stable copy before reset_(), then pass that staged object by
            // lvalue.
            std::decay_t<Up_> staged_val_(std::forward<Up_>(val_));
            this->data.template emplace_<Target_>(staged_val_);
        } else {
            // A non-movable target cannot be staged and then transferred.
            // Preserve direct construction support for such alternatives.
            this->data.template emplace_<Target_>(std::forward<Up_>(val_));
        }
        return *this;
    }

  public:
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     typename Opt::None) -> bool {
        return lhs.is_none();
    }

  public:
    friend constexpr auto operator==(typename Opt::None,
                                     const Option<Tp_, Args_...> &rhs) -> bool {
        return rhs.is_none();
    }

  public:
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     typename Opt::Default) -> bool
        requires(std::is_default_constructible_v<Tp_> &&
                 std::equality_comparable<Tp_>)
    {
        if (!lhs.has_primary_()) {
            return false;
        }

        const Tp_ def_{};
        return *lhs == def_;
    }

  public:
    friend constexpr auto operator==(typename Opt::Default,
                                     const Option<Tp_, Args_...> &rhs) -> bool
        requires(std::is_default_constructible_v<Tp_> &&
                 std::equality_comparable<Tp_>)
    {
        return rhs == Opt::Default{};
    }

  public:
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     typename Opt::Default) -> bool
        requires(!std::is_default_constructible_v<Tp_> &&
                 has_default_alternative_ &&
                 std::equality_comparable<default_alternative_type_>)
    {
        if (!lhs.template has_type_<default_alternative_type_>()) {
            return false;
        }

        const default_alternative_type_ def_{};
// Intentional equality comparison (Opt::Default pattern matching).
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
        return lhs.template get_<default_alternative_type_>() == def_;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    }

  public:
    friend constexpr auto operator==(typename Opt::Default,
                                     const Option<Tp_, Args_...> &rhs) -> bool
        requires(!std::is_default_constructible_v<Tp_> &&
                 has_default_alternative_ &&
                 std::equality_comparable<default_alternative_type_>)
    {
        return rhs == Opt::Default{};
    }

  public:
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      typename Opt::Default)
        -> details::__compare_three_way_result_t<Tp_>
        requires(sizeof...(Args_) == 0 &&
                 std::is_default_constructible_v<Tp_> &&
                 std::three_way_comparable<Tp_>)
    {
        if (lhs.is_some()) {
            const Tp_ def_{};
            return *lhs <=> def_;
        }

        return std::strong_ordering::less;
    }

  public:
    friend constexpr auto operator<=>(typename Opt::Default,
                                      const Option<Tp_, Args_...> &rhs)
        -> details::__compare_three_way_result_t<Tp_>
        requires(sizeof...(Args_) == 0 &&
                 std::is_default_constructible_v<Tp_> &&
                 std::three_way_comparable<Tp_>)
    {
        if (rhs.is_some()) {
            const Tp_ def_{};
            return def_ <=> *rhs;
        }
        return std::strong_ordering::greater;
    }

  public:
    template <typename Qp_>
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     const Opt::Some<Qp_> &rhs) -> bool
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::equality_comparable<Qp_>)
    {
        if constexpr (std::same_as<Qp_, Tp_>) {
            return (lhs.has_primary_() && (lhs.primary_value_() == rhs.get()));
        }

        else {
            return (lhs.template has_type_<Qp_>() &&
                    lhs.template get_<Qp_>() == rhs.get());
        }
    }

  public:
    template <typename Qp_>
    friend constexpr auto operator==(const Opt::Some<Qp_> &lhs,
                                     const Option<Tp_, Args_...> &rhs) -> bool
        requires((std::same_as<Qp_, Tp_> ||
                  (std::same_as<Qp_, Args_> || ...)) &&
                 std::equality_comparable<Qp_>)
    {
        return rhs == lhs;
    }

  public:
    template <typename Up_>
    friend constexpr auto operator==(const Option<Tp_, Args_...> &lhs,
                                     const Up_ &rhs) -> bool
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 (details::option_value_comparable_<Tp_, Up_> ||
                  (details::option_value_comparable_<Args_, Up_> || ...)))
    {
        if constexpr (details::option_value_comparable_<Tp_, Up_>) {
            if (lhs.has_primary_()) {
                return lhs.primary_value_() == rhs;
            }
        }
        auto check_alts = [&]<std::size_t Index_ = 0>(auto &&self_fn) -> bool {
            if constexpr (Index_ < sizeof...(Args_)) {
                using Alt_ = details::__pack_element_t<Index_, Args_...>;
                if constexpr (details::option_value_comparable_<Alt_, Up_>) {
                    if (lhs.template has_type_<Alt_>()) {
                        return lhs.template get_<Alt_>() == rhs;
                    }
                }
                return self_fn.template operator()<Index_ + 1>(self_fn);
            } else {
                return false;
            }
        };
        return check_alts.template operator()<0>(check_alts);
    }

  public:
    template <typename Up_>
    friend constexpr auto operator==(const Up_ &lhs,
                                     const Option<Tp_, Args_...> &rhs) -> bool
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 (details::option_value_comparable_<Tp_, Up_> ||
                  (details::option_value_comparable_<Args_, Up_> || ...)))
    {
        return rhs == lhs;
    }

  public:
    template <typename Stream_>
    friend auto operator<<(Stream_ &os_, const Option<Tp_, Args_...> &opt_)
        -> Stream_ &
        requires requires { os_ << opt_.unwrap(); }
    {
        if (opt_.is_some()) {
            const std::size_t active_{opt_.active_index_()};
            auto print_active =
                [&]<std::size_t Index_ = 0>(auto &&self_fn) -> void {
                if constexpr (Index_ < sizeof...(Args_) + 1) {
                    using CurrentType_ =
                        details::__pack_element_t<Index_, Tp_, Args_...>;
                    if (active_ == Index_ + 1) {
                        if constexpr (requires {
                                          os_ << opt_.template get_<
                                              CurrentType_>();
                                      }) {
                            os_ << opt_.template get_<CurrentType_>();
                        }
                        return;
                    }
                    self_fn.template operator()<Index_ + 1>(self_fn);
                }
            };
            print_active.template operator()<0>(print_active);
        } else {
            os_ << "None";
        }
        return os_;
    }
};

template <typename Tp_> Option(Some<Tp_>) -> Option<Tp_>;

using details::operator>>=;
using details::AnySome_;
using details::False_;
using details::True_;
using details::Wild_;

inline constexpr auto Wild{details::Wild_{}};
inline constexpr auto SomeAny{details::AnySome_{}};
inline constexpr auto True{details::True_{}};
inline constexpr auto False{details::False_{}};

}; /* namespace Opt */
}; // namespace details

namespace details {
template <typename Tp_>
concept is_hashable_ =
    requires(const std::hash<std::remove_cv_t<Tp_>> &h_, const Tp_ &v_) {
        { h_(v_) } -> std::same_as<std::size_t>;
    };

template <typename Types_, bool Enabled> struct option_hash_base_ {
  public:
    constexpr option_hash_base_(void) = delete;
    constexpr option_hash_base_(const option_hash_base_ &) = delete;
    constexpr option_hash_base_(option_hash_base_ &&) = delete;
    constexpr auto operator=(const option_hash_base_ &)
        -> option_hash_base_ & = delete;
    constexpr auto operator=(option_hash_base_ &&)
        -> option_hash_base_ & = delete;
};

template <typename Tp_, typename... Args_>
struct option_hash_base_<std::tuple<Tp_, Args_...>, true> {
  private:
    using Option_ = Opt::Option<Tp_, Args_...>;
    static constexpr std::size_t none_hash_{static_cast<std::size_t>(-3333)};

    static constexpr auto combine_(std::size_t index_,
                                   std::size_t value_hash_) noexcept
        -> std::size_t {
        if constexpr (sizeof...(Args_) == 0) {
            return value_hash_;
        } else {
            constexpr std::size_t golden_ratio_{
                static_cast<std::size_t>(0x9e3779b9U)};
            return value_hash_ ^ (golden_ratio_ + index_ + (value_hash_ << 6) +
                                  (value_hash_ >> 2));
        }
    }

    template <typename Qp_>
    static constexpr auto hash_one_(const Option_ &option_) noexcept(noexcept(
        std::hash<std::remove_cv_t<Qp_>>{}(std::declval<const Qp_ &>())))
        -> std::size_t {
        const std::size_t value_hash_{
            std::hash<std::remove_cv_t<Qp_>>{}(option_.template get_<Qp_>())};
        constexpr std::size_t index_ = [] {
            if constexpr (std::same_as<Qp_, Tp_>) {
                return std::size_t{1};
            } else {
                return std::size_t{2} + type_index_<Qp_, Args_...>::value;
            }
        }();
        return combine_(index_, value_hash_);
    }

  public:
    constexpr auto operator()(const Option_ &option_) const
        noexcept((noexcept(std::hash<std::remove_cv_t<Tp_>>{}(
                      std::declval<const Tp_ &>())) &&
                  (noexcept(std::hash<std::remove_cv_t<Args_>>{}(
                       std::declval<const Args_ &>())) &&
                   ...))) -> std::size_t {
        const std::size_t index_{option_.active_index_()};
        if (index_ == 0) {
            return none_hash_;
        }

        using Fn_ = std::size_t (*)(const Option_ &);
        static constexpr std::array<Fn_, sizeof...(Args_) + 1> dispatch_{
            &hash_one_<Tp_>, &hash_one_<Args_>...};
        return dispatch_[index_ - 1](option_);
    }
};
}; // namespace details
}; /* namespace rl */

#undef RL_LIFETIME_BOUND
#undef RL_SUPPORTS_REFLECTION
#undef RL_IS_SUBVIEW_OR_SUBSTR

namespace rl {

/**
 * @brief Class `Option<Tp_, Ts...>` template instantiation which accepts `T`
 * type or `None`.
 */

using details::Opt::Option;

/**
 * @brief Class `Some<Tp_>` template instantiation which accepts `T` or
 * `None` Type of `Option<Tp_>`.
 */
using details::Opt::Some;

/**
 * @brief Class `None` which accepts nothing. Tag to disengage optional objects
 * regardless of any invariants of types is default constructible. The internal
 * layout secretly takes `Option<None, Tp_, Ts...>` for the disengage optional
 * object for nullstate.
 */
inline constexpr auto None{details::Opt::None{}};

/**
 * @brief Class `Default` which constructs a default value if possible from
 * `Option<T, Ts...>` regardless of any Type alternatives that are
 * non-constructible. If a non-constructible type is found and checked during
 * compile time, it automatically ignores the index and looks at the next state
 * and evaluates the type signature for default value construction. If any
 * variants of types isn't default constructible. It is thus raised as a compile
 * time error through static assertion.
 */
inline constexpr auto Default{details::Opt::Default{}};

/**
 * @brief Class `Wild`, which evaluates and
 * falls to every expression branch to match Some<T>, SomeAny, True_t, False_t
 * even when the data is uninitialized, nullstate.
 */
inline constexpr auto Wild{rl::details::Wild_{}};

/**
 * @brief Class `SomeAny` only accepts any type of
 * Some Types regardless of their indifference of variants from `Option<T,
 * Ts...>`
 */
inline constexpr auto SomeAny{rl::details::AnySome_{}};

/**
 * @brief Class `True` is a boolean evaluator which is derived and replacement
 * to `true` of `bool` type. An `Option<bool>` can't pattern match via `true`
 * even when the value is initialized by default with `true`. The pattern
 * matcher nullifies the `true` arm and discards it without issuing an error
 * before, completely ignoring the section immediately. This class helps to
 * ensure the `true` state remains a valid arm for pattern matching.
 */
inline constexpr auto True{rl::details::True_{}};

/**
 * @brief Class `False` is a boolean evaluator which is derived and replacement
 * to `false` of `bool` type. An `Option<bool>` can't pattern match via `false`
 * even when the value is initialized by default with `false`. The pattern
 * matcher nullfies the `false` arm and discards it without issuing an error
 * before, completely ignoring the section immediately. This class ensures that
 * the `false` state remains a valid arm for pattern matching.
 */
inline constexpr auto False{rl::details::False_{}};

using rl::details::operator>>=;

}; // namespace rl

/**
 * @brief Class `None_t` which accepts nothing. Tag to disengage optional
 * objects regardless of any invariants of types is default constructible. The
 * internal layout secretly takes `Option<None_t, Tp_, Ts...>` for the disengage
 * optional object for nullstate.
 */
inline constexpr auto None_t{rl::details::Opt::None{}};

/**
 * @brief Class `Default_t` which constructs a default value if possible from
 * `Option<T, Ts...>` regardless of any Type alternatives that are
 * non-constructible. If a non-constructible type is found and checked during
 * compile time, it automatically ignores the index and looks at the next state
 * and evaluates the type signature for default value construction. If any
 * invariants of types isn't default constructible. It is thus raised as a
 * compile time error through static assertion.
 */
inline constexpr auto Default_t{rl::details::Opt::Default{}};

/**
 * @brief Class `Wild_t` an alias of `rl::Wild`, which evaluates and
 * falls to every expression branch to match Some<T>, SomeAny, True_t, False_t
 * even when the data is uninitialized, nullstate.
 */
inline constexpr auto Wild_t{rl::details::Wild_{}};

/**
 * @brief Class `SomeAny_t` an alias of `rl::SomeAny` only accepts any type of
 * Some Types regardless of their indifference of invariants from `Option<T,
 * Ts...>`
 */
inline constexpr auto SomeAny_t{rl::details::AnySome_{}};

/**
 * @brief Class `True_t` an alias of `rl::True` is a boolean evaluator which is
 * derived and replacement to `true` of `bool` type. An `Option<bool>` can't
 * pattern match via `true` even when the value is initialized by default with
 * `true`. The pattern matcher nullifies the `true` arm and discards it without
 * issuing an error before, completely ignoring the section immediately. This
 * class helps to ensure the `true` state remains a valid arm for pattern
 * matching.
 */
inline constexpr auto True_t{rl::details::True_{}};

/**
 * @brief Class `False_t` an alias of `rl::False` is a boolean evaluator which
 * is derived and replacement to `false` of `bool` type. An `Option<bool>` can't
 * pattern match via `false` even when the value is initialized by default with
 * `false`. The pattern matcher nullfies the `false` arm and discards it without
 * issuing an error before, completely ignoring the section immediately. This
 * class ensures that the `false` state remains a valid arm for pattern
 * matching.
 */
inline constexpr auto False_t{rl::details::False_{}};

using rl::operator>>=;

template <typename Tp_, typename... Args_>
struct std::hash<rl::details::Opt::Option<Tp_, Args_...>>
    : rl::details::option_hash_base_<std::tuple<Tp_, Args_...>,
                                     (rl::details::is_hashable_<Tp_> &&
                                      (rl::details::is_hashable_<Args_> &&
                                       ...))> {};

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
#endif
#endif /* RL_OPTION_CXX26_HPP */
