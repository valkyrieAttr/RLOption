/** @file include/Option.hpp
 *  This is a re-implementation of std::optional<T> with other advanced features
 * and subsets included.
 */

#ifndef RL_OPTION_CXX26_HPP
#if (defined(__clang__))
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-macros"
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
#endif

#define RL_OPTION_CXX26_HPP 1

#if (__cplusplus < 202302L)
#error "Option requires C++23 or later"
#else
#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#if (__has_cpp_attribute(clang::lifetimebound))
#define RL_LIFETIME_BOUND clang::lifetimebound
#elif (__has_cpp_attribute(gnu::lifetimebound))
#define RL_LIFETIME_BOUND gnu::lifetimebound
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

template <std::size_t Count_> struct alignas(1) option_index_type_ {
  public:
    using type = std::conditional_t<
        Count_ <= std::numeric_limits<unsigned char>::max(), unsigned char,
        std::conditional_t<Count_ <= std::numeric_limits<unsigned short>::max(),
                           unsigned short, std::size_t>>;
};

template <typename Qp_, typename... Types_> struct alignas(1) type_index_;

template <typename Qp_, typename First_, typename... Rest_>
struct alignas(1) type_index_<Qp_, First_, Rest_...>
    : std::integral_constant<std::size_t,
                             std::same_as<Qp_, First_>
                                 ? 0
                                 : 1 + type_index_<Qp_, Rest_...>::value> {};

template <typename Qp_>
struct alignas(1) type_index_<Qp_> : std::integral_constant<std::size_t, 0> {};

template <typename Qp_> class alignas(Qp_) [[nodiscard]] Optional_;

template <typename Tp_, typename... Args_>
class alignas(
    max_alignof_<Tp_, Args_...,
                 typename option_index_type_<sizeof...(Args_) + 1>::type>())
    OptionStorage_;

namespace Opt {
template <typename Tp_, typename... Args_>
class alignas(
    details::max_alignof_<
        Tp_, Args_...,
        typename details::option_index_type_<sizeof...(Args_) + 1>::type>())
    [[nodiscard]] Option;

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
    constexpr Tp_ &&get(void) &&[[RL_LIFETIME_BOUND]] {
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

template <typename Tp_> Some(Tp_) -> Some<Tp_>;

class alignas(1) [[nodiscard]] None {
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

class alignas(1) [[nodiscard]] Default {
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
template <std::size_t N_> struct alignas(1) Pad_ {
  public:
    unsigned char _pad[N_]{};
};

template <

    >
struct alignas(1) Pad_<0> {};

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

struct alignas(1) Wild_ {};

struct alignas(1) AnySome_ {};

template <typename Pr_> class alignas(Pr_) Where_ {
  public:
    using where_tag_ = void;

  public:
    Pr_ pred;
};

template <typename Tp_> struct alignas(1) is_some_pattern_ : std::false_type {};

template <typename Tp_>
struct alignas(1) is_some_pattern_<Opt::Some<Tp_>> : std::true_type {
  public:
    using type = Tp_;
};

template <typename Pp_>
concept MatchPattern =
    (std::same_as<std::remove_cvref_t<Pp_>, Opt::None> ||
     std::same_as<std::remove_cvref_t<Pp_>, details::Wild_> ||
     std::same_as<std::remove_cvref_t<Pp_>, details::AnySome_> ||
     details::is_some_pattern_<std::remove_cvref_t<Pp_>>::value);

template <typename Pp_, typename Fp_> struct Arm_ {
  public:
    using pattern_type = std::remove_cvref_t<Pp_>;

  public:
    using handler_type = std::remove_cvref_t<Fp_>;

  public:
    [[no_unique_address]] pattern_type pattern;

  public:
    handler_type handler;
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
using ForwardLike_t = std::conditional_t<
    std::is_rvalue_reference_v<From_ &&>,
    std::conditional_t<std::is_const_v<std::remove_reference_t<From_>>,
                       const To_ &&, To_ &&>,
    std::conditional_t<std::is_const_v<std::remove_reference_t<From_>>,
                       const To_ &, To_ &>>;

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

    else if constexpr (requires(const Pd_ &p_, const Tp_ &v_) {
                           typename Pd_::where_tag_;
                           { p_.pred(v_) } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (requires(const Pd_ &p_, const Tp_ &v_) {
                           { p_(v_) } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (!std::same_as<Pd_, bool> && requires(const Pd_ &p_) {
                           { p_() } -> std::convertible_to<bool>;
                       }) {
        return true;
    }

    else if constexpr (std::same_as<Pd_, bool> && !std::same_as<Tp_, bool>) {
        return true;
    }

    else {
        return (requires(const Tp_ &v_, const Pd_ &p_) {
            { v_ == p_ } -> std::convertible_to<bool>;
        });
    }
}

template <typename Rp_, typename Self_, typename... Types_>
class alignas(void const *) ArmMulti_ {
  private:
    using AcceptFn_ = bool (*)(void const *, std::size_t, void const *);

  private:
    using CallFn_ = Rp_ (*)(void *, std::size_t, void *);

  private:
    static constexpr std::size_t _pad_len_{
        (alignof(void const *) - ((sizeof(bool) * 3) % alignof(void const *))) %
        alignof(void const *)};

  private:
    template <typename From_, typename To_>
    using ForwardLike_t = details::ForwardLike_t<From_, To_>;

  private:
    template <typename Pp_, typename Tp_>
    static constexpr auto pattern_matches_type_(void) noexcept(true) -> bool {
        return details::pattern_matches_type_<Pp_, Tp_>();
    }

  private:
    template <typename Pp_, typename Tp_>
    static constexpr auto matches_value_(const Pp_ &pat_, const Tp_ &value_)
        -> bool {
        using Pd_ = std::remove_cvref_t<Pp_>;

        if constexpr (std::same_as<Pd_, Opt::None>) {
            return false;
        }

        else if constexpr (std::same_as<Pd_, Opt::Default>) {
            if constexpr (std::is_default_constructible_v<Tp_> &&
                          std::equality_comparable<Tp_>) {
                const Tp_ def_{};
                return value_ == def_;
            }

            else {
                return false;
            }
        }

        else if constexpr (std::same_as<Pd_, details::Wild_> ||
                           std::same_as<Pd_, details::AnySome_>) {
            return true;
        }

        else if constexpr (is_some_pattern_<Pd_>::value) {
            if constexpr (std::same_as<Tp_,
                                       typename is_some_pattern_<Pd_>::type> &&
                          std::equality_comparable<Tp_>) {
// Intentional: pattern matching uses equality comparison including for floats.
// This mirrors std::optional behaviour.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
                return value_ == pat_.get();
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
            }

            else {
                return false;
            }
        }

        else if constexpr (requires(const Pd_ &p_, const Tp_ &v_) {
                               typename Pd_::where_tag_;
                               { p_.pred(v_) } -> std::convertible_to<bool>;
                           }) {
            return static_cast<bool>(pat_.pred(value_));
        }

        else if constexpr (requires(const Pd_ &p_, const Tp_ &v_) {
                               { p_(v_) } -> std::convertible_to<bool>;
                           }) {
            return static_cast<bool>(pat_(value_));
        }

        else if constexpr (!std::same_as<Pd_, bool> && requires(const Pd_ &p_) {
                               { p_() } -> std::convertible_to<bool>;
                           }) {
            return static_cast<bool>(pat_());
        }

        else if constexpr (std::same_as<Pd_, bool> &&
                           !std::same_as<Tp_, bool>) {
            return pat_;
        }

        else {
            return value_ == pat_;
        }
    }

  private:
    template <typename Pp_, typename Tp_>
    static constexpr auto accept_for_(void const *pat_, void const *value_)
        -> bool {
        return (value_ != nullptr &&
                matches_value_(*static_cast<const Pp_ *>(pat_),
                               *static_cast<const Tp_ *>(value_)));
    }

  private:
    template <typename Fp_, typename Tp_>
    static constexpr auto can_call_type_(void) -> bool {
        using Vp_ = ForwardLike_t<Self_, Tp_>;

        return (
            std::is_invocable_r_v<Rp_, Fp_ &, Vp_> ||
            (std::is_void_v<Rp_> && std::is_invocable_v<Fp_ &, Vp_>) ||
            std::is_invocable_r_v<Rp_, Fp_ &> ||
            (std::is_void_v<Rp_> && std::is_invocable_v<Fp_ &>) ||
            (!std::is_void_v<Rp_> && (std::is_convertible_v<Fp_ &, Rp_> ||
                                      std::is_constructible_v<Rp_, Fp_ &>)));
    }

  private:
    template <typename Fp_, typename Tp_>
    static constexpr auto call_for_(void *fn_, void *value_) -> Rp_ {
        using Vp_ = ForwardLike_t<Self_, Tp_>;
        using Obj_ = std::remove_reference_t<Vp_>;

        auto &handler_ = *static_cast<Fp_ *>(fn_);
        auto &&value = static_cast<Vp_>(*static_cast<Obj_ *>(value_));

        if constexpr (std::is_invocable_r_v<Rp_, Fp_ &, Vp_> ||
                      (std::is_void_v<Rp_> &&
                       std::is_invocable_v<Fp_ &, Vp_>)) {
            return (std::invoke_r<Rp_>(handler_, static_cast<Vp_>(value)));
        }

        else if constexpr (std::is_invocable_r_v<Rp_, Fp_ &> ||
                           (std::is_void_v<Rp_> &&
                            std::is_invocable_v<Fp_ &>)) {
            return std::invoke_r<Rp_>(handler_);
        }

        else if constexpr (!std::is_void_v<Rp_> &&
                           (std::is_convertible_v<Fp_ &, Rp_> ||
                            std::is_constructible_v<Rp_, Fp_ &>)) {
            return static_cast<Rp_>(handler_);
        }

        else {
            std::unreachable();
        }
    }

  private:
    template <typename Fp_> static constexpr auto call_none_(void *fn_) -> Rp_ {
        auto &handler_ = *static_cast<Fp_ *>(fn_);

        if constexpr (std::is_invocable_r_v<Rp_, Fp_ &> ||
                      (std::is_void_v<Rp_> && std::is_invocable_v<Fp_ &>)) {
            return std::invoke_r<Rp_>(handler_);
        }

        else if constexpr (!std::is_void_v<Rp_> &&
                           (std::is_convertible_v<Fp_ &, Rp_> ||
                            std::is_constructible_v<Rp_, Fp_ &>)) {
            return static_cast<Rp_>(handler_);
        }

        else {
            std::unreachable();
        }
    }

  private:
    template <typename OpTp_, typename... OpArgs_> friend class Opt::Option;

  private:
    bool present_{false};

  private:
    bool matches_none_{false};

  private:
    bool matches_some_{false};

  private:
    [[no_unique_address]] details::Pad_<_pad_len_> _pad{};

  private:
    void const *pat_{nullptr};

  private:
    void *fn_{nullptr};

  private:
    bool (*accept_)(void const *, std::size_t, void const *){nullptr};

  private:
    Rp_ (*call_)(void *, std::size_t, void *){nullptr};

  private:
    template <typename Pd_, typename First_, typename... Rest_>
    static constexpr auto accept_chain_(std::size_t index_, void const *pat_,
                                        void const *value_) -> bool {
        if constexpr (pattern_matches_type_<Pd_, First_>()) {
            if (index_ == 1) {
                return accept_for_<Pd_, First_>(pat_, value_);
            }
        }

        if constexpr (sizeof...(Rest_) > 0) {
            return (accept_chain_<Pd_, Rest_...>(index_ - 1, pat_, value_));
        }

        return false;
    }

  private:
    template <typename Fd_, typename First_, typename... Rest_>
    static constexpr auto call_chain_(std::size_t index_, void *fn_,
                                      void *value_) -> Rp_ {
        if (index_ == 1) {
            return call_for_<Fd_, First_>(fn_, value_);
        }

        if constexpr (sizeof...(Rest_) > 0) {
            return (call_chain_<Fd_, Rest_...>(index_ - 1, fn_, value_));
        }

        std::unreachable();
    }

  public:
    constexpr ArmMulti_(void) = default;

  private:
    template <typename Pp_, typename Fp_>
    constexpr auto init_arm_(void) -> void {
        using Pd_ = std::remove_cvref_t<Pp_>;
        using Fd_ = std::remove_reference_t<Fp_>;

        static_assert(((!pattern_matches_type_<Pd_, Types_>() ||
                        can_call_type_<Fd_, Types_>()) &&
                       ...),
                      "match_unwrapped() arm handler cannot accept a matched "
                      "alternative");

        static_assert(
            (!std::same_as<Pd_, Opt::None> &&
             !std::same_as<Pd_, details::Wild_>) ||
                std::is_invocable_r_v<Rp_, Fd_ &> ||
                (std::is_void_v<Rp_> && std::is_invocable_v<Fd_ &>) ||
                (!std::is_void_v<Rp_> && (std::is_convertible_v<Fd_ &, Rp_> ||
                                          std::is_constructible_v<Rp_, Fd_ &>)),
            "match_unwrapped() None arm handler must be callable "
            "without an argument");

        this->accept_ = [](void const *pat, std::size_t index,
                           void const *value) -> bool {
            if (index == 0) {
                return (std::same_as<Pd_, Opt::None> ||
                        std::same_as<Pd_, details::Wild_>);
            }

            if (index > sizeof...(Types_)) {
                return false;
            }

            return (accept_chain_<Pd_, Types_...>(index, pat, value));
        };

        this->call_ = [](void *fn, std::size_t index, void *value) -> Rp_ {
            if (index == 0) {
                return call_none_<Fd_>(fn);
            }

            if (index > sizeof...(Types_)) {
                std::unreachable();
            }

            return (call_chain_<Fd_, Types_...>(index, fn, value));
        };
    }

  public:
    template <typename ArmType_>
    constexpr ArmMulti_(ArmType_ &&arm [[RL_LIFETIME_BOUND]]) noexcept(true)
        requires(requires {
                    arm.pattern;
                    arm.handler;
                })
        : present_{true},
          matches_none_{
              std::same_as<typename std::remove_cvref_t<ArmType_>::pattern_type,
                           Opt::None> ||
              std::same_as<typename std::remove_cvref_t<ArmType_>::pattern_type,
                           details::Wild_>},
          matches_some_{
              std::same_as<typename std::remove_cvref_t<ArmType_>::pattern_type,
                           details::Wild_> ||
              std::same_as<typename std::remove_cvref_t<ArmType_>::pattern_type,
                           details::AnySome_>},
          pat_{std::addressof(arm.pattern)},
          fn_{const_cast<void *>(
              static_cast<void const *>(std::addressof(arm.handler)))} {
        using Pp_ = typename std::remove_cvref_t<ArmType_>::pattern_type;
        using Fp_ = typename std::remove_cvref_t<ArmType_>::handler_type;
        this->template init_arm_<Pp_, Fp_>();
    }

    // Catch-all constructor for AnySome_ (no .pattern / .handler
    // members). AnySome_ is a sentinel meaning "any Some value
    // regardless of type"; we store fn_ as a pointer to the
    // AnySome_ value itself, which acts as the callable (operator()
    // with any value).
  public:
    template <typename ArmType_>
    constexpr ArmMulti_(ArmType_ &&arm [[RL_LIFETIME_BOUND]]) noexcept(true)
        requires(std::same_as<std::remove_cvref_t<ArmType_>, details::AnySome_>)
        : present_{true}, matches_none_{false}, matches_some_{true},
          pat_{nullptr}, fn_{const_cast<void *>(
                             static_cast<void const *>(std::addressof(arm)))} {
        // accept_: match any active Some alternative
        this->accept_ = [](void const * /*pat*/, std::size_t index,
                           void const * /*value*/
                           ) -> bool {
            // AnySome_ matches any Some (index > 0)
            return index > 0;
        };

        // call_: invoke the AnySome_ as a callable with no value
        // arg
        this->call_ = [](void *fn, std::size_t /*index*/, void * /*value*/
                         ) -> Rp_ {
            auto &handler_ = *static_cast<std::remove_cvref_t<ArmType_> *>(fn);
            if constexpr (std::is_invocable_r_v<Rp_, decltype(handler_)>) {
                return static_cast<Rp_>(handler_());
            } else if constexpr (std::is_void_v<Rp_> &&
                                 std::is_invocable_v<decltype(handler_)>) {
                handler_();
            } else {
                std::unreachable();
            }
        };
    }

  private:
    template <typename Tp_, typename... Args_> friend class Option;
};

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

struct alignas(1) Empty_ {};

template <typename... Types_> struct alignas(1) first_default_constructible_ {
  public:
    using type = void;
};

template <typename Tp_, typename... Args_>
struct alignas(1) first_default_constructible_<Tp_, Args_...> {
  public:
    using type = std::conditional_t<
        std::is_default_constructible_v<Tp_>, Tp_,
        typename first_default_constructible_<Args_...>::type>;
};

template <typename... Types_>
struct alignas(1) unique_types_ : std::true_type {};

template <typename Tp_, typename... Args_>
struct alignas(1) unique_types_<Tp_, Args_...>
    : std::bool_constant<
          ((!std::same_as<std::remove_cv_t<Tp_>, std::remove_cv_t<Args_>>) &&
           ...) &&
          unique_types_<Args_...>::value> {};

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

template <typename> struct alignas(1) __is_option_type_ : std::false_type {};

template <typename Up_, typename... Args_>
struct alignas(1)
    __is_option_type_<Opt::Option<Up_, Args_...>> : std::true_type {};

template <typename Up_>
concept __is_option_ = (__is_option_type_<std::remove_cvref_t<Up_>>::value);

template <typename Up_> struct alignas(1) __compare_three_way_result {
  public:
    using type = void;
};

template <typename Up_>
    requires(std::three_way_comparable<Up_>)
struct alignas(1) __compare_three_way_result<Up_> {
  public:
    using type = std::compare_three_way_result_t<Up_>;
};

template <typename Up_>
using __compare_three_way_result_t =
    typename __compare_three_way_result<Up_>::type;

template <typename List_, typename T_> struct alignas(1) __append_unique_;

template <typename... Ts_, typename T_>
struct alignas(1) __append_unique_<std::tuple<Ts_...>, T_> {
  public:
    using type = std::conditional_t<(std::same_as<T_, Ts_> || ...),
                                    std::tuple<Ts_...>, std::tuple<Ts_..., T_>>;
};

template <typename Acc_, typename... Rest_>
struct alignas(1) __dedup_pack_impl_;

template <typename Acc_> struct alignas(1) __dedup_pack_impl_<Acc_> {
  public:
    using type = Acc_;
};

template <typename Acc_, typename First_, typename... Rest_>
struct alignas(1) __dedup_pack_impl_<Acc_, First_, Rest_...> {
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

template <typename Tuple_> struct alignas(1) __tuple_to_option_;

template <

    >
struct alignas(1) __tuple_to_option_<std::tuple<>> {
  public:
    using type = Opt::None;
};

template <typename First_, typename... Rest_>
struct alignas(1) __tuple_to_option_<std::tuple<First_, Rest_...>> {
  public:
    using type = Opt::Option<First_, Rest_...>;
};

template <typename... Rs_>
concept __has_common_type_ =
    (!(std::same_as<std::remove_cvref_t<Rs_>, Opt::None> || ...)) &&
    (!(is_some_pattern_<std::remove_cvref_t<Rs_>>::value || ...)) &&
    requires { typename std::common_type_t<Rs_...>; };

template <typename Up_, typename... Types_>
struct alignas(1) match_alternative_ {
  private:
    template <typename T_>
    static constexpr bool is_exact_ =
        std::same_as<std::remove_cvref_t<Up_>, T_>;

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

template <typename Arm_, typename... ForwardAlts_>
struct alignas(1) __deduce_arm_res_ {
  public:
    using ArmD_ = std::remove_cvref_t<Arm_>;

  public:
    template <typename A_> static constexpr auto get_arm_res(void) {
        if constexpr (requires {
                          typename A_::pattern_type;
                          typename A_::handler_type;
                      }) {
            using Pp_ = typename A_::pattern_type;
            using Fp_ = typename A_::handler_type;
            return get_ph_res<Pp_, Fp_>();
        }

        else {
            return std::type_identity<std::decay_t<A_>>{};
        }
    }

  private:
    template <typename Pp_, typename Fp_>
    static constexpr auto get_ph_res(void) {
        using Pd_ = std::remove_cvref_t<Pp_>;
        using Fd_ = std::remove_reference_t<Fp_>;

        if constexpr (std::same_as<Pd_, Opt::None>) {
            if constexpr (std::is_invocable_v<Fd_ &>) {
                return std::type_identity<std::invoke_result_t<Fd_ &>>{};
            } else {
                return std::type_identity<std::decay_t<Fd_>>{};
            }
        } else {
            return test_matching_alts<Pd_, Fd_, ForwardAlts_...>();
        }
    }

  private:
    template <typename Pd_, typename Fd_>
    static constexpr auto test_matching_alts(void) {
        return test_any_alts<Fd_, ForwardAlts_...>();
    }

  private:
    template <typename Pd_, typename Fd_, typename First_, typename... Rest_>
    static constexpr auto test_matching_alts(void) {
        using FirstVal_ = std::remove_cvref_t<First_>;
        if constexpr (pattern_matches_type_<Pd_, FirstVal_>() &&
                      std::is_invocable_v<Fd_ &, First_>) {
            return std::type_identity<std::invoke_result_t<Fd_ &, First_>>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return test_matching_alts<Pd_, Fd_, Rest_...>();
        } else {
            return test_any_alts<Fd_, ForwardAlts_...>();
        }
    }

  private:
    template <typename Fd_> static constexpr auto test_any_alts(void) {
        if constexpr (std::is_invocable_v<Fd_ &>) {
            return std::type_identity<std::invoke_result_t<Fd_ &>>{};
        } else {
            return std::type_identity<std::decay_t<Fd_>>{};
        }
    }

  private:
    template <typename Fd_, typename First_, typename... Rest_>
    static constexpr auto test_any_alts(void) {
        if constexpr (std::is_invocable_v<Fd_ &, First_>) {
            return std::type_identity<std::invoke_result_t<Fd_ &, First_>>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return test_any_alts<Fd_, Rest_...>();
        } else if constexpr (std::is_invocable_v<Fd_ &>) {
            return std::type_identity<std::invoke_result_t<Fd_ &>>{};
        } else {
            return std::type_identity<std::decay_t<Fd_>>{};
        }
    }

  public:
    using type = typename decltype(get_arm_res<ArmD_>())::type;
};

template <typename T_> struct alignas(1) __wrap_in_option_ {
  public:
    using type = Opt::Option<T_>;
};

template <typename T_>
    requires(details::__is_option_<T_>)
struct alignas(1) __wrap_in_option_<T_> {
  public:
    using type = T_;
};

template <

    >
struct alignas(1) __wrap_in_option_<void> {
  public:
    using type = void;
};

template <

    >
struct alignas(1) __wrap_in_option_<Opt::None> {
  public:
    using type = Opt::None;
};

template <typename T_>
using __wrap_in_option_t = typename __wrap_in_option_<T_>::type;

template <typename Tuple_> struct alignas(1) __tuple_to_wrapped_option_;

template <

    >
struct alignas(1) __tuple_to_wrapped_option_<std::tuple<>> {
  public:
    using type = Opt::None;
};

template <typename First_>
struct alignas(1) __tuple_to_wrapped_option_<std::tuple<First_>> {
  public:
    using type = __wrap_in_option_t<First_>;
};

template <typename First_, typename... Rest_>
struct alignas(1) __tuple_to_wrapped_option_<std::tuple<First_, Rest_...>> {
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

template <typename... Rs_> struct alignas(1) __deduce_res_from_types_ {
  private:
    static constexpr bool all_void_ = (std::is_void_v<Rs_> && ...);

  public:
    using type = std::conditional_t<
        all_void_, void,
        typename __tuple_to_wrapped_option_<__dedup_tuple_t<Rs_...>>::type>;
};

template <typename AltsTuple_, typename... Arms_>
struct alignas(1) __deduce_match_res_;

template <typename... ForwardAlts_, typename... Arms_>
struct alignas(1) __deduce_match_res_<std::tuple<ForwardAlts_...>, Arms_...> {
  public:
    using type = typename __deduce_res_from_types_<
        typename __deduce_arm_res_<Arms_, ForwardAlts_...>::type...>::type;
};

template <typename Tp_, typename... Arms_> struct alignas(1) __deduce_res_ {
    using type = typename __deduce_res_from_types_<
        typename __deduce_arm_res_<Arms_, Tp_>::type...>::type;
};

template <typename ExplicitTuple_, typename AltsTuple_, typename... Arms_>
struct alignas(1) __match_unwrapped_return_type_;

template <typename AltsTuple_, typename... Arms_>
struct alignas(1)
    __match_unwrapped_return_type_<std::tuple<>, AltsTuple_, Arms_...> {
  public:
    using type = typename __deduce_match_res_<AltsTuple_, Arms_...>::type;
};

template <typename Single_, typename AltsTuple_, typename... Arms_>
struct alignas(1)
    __match_unwrapped_return_type_<std::tuple<Single_>, AltsTuple_, Arms_...> {
  public:
    using type = __wrap_in_option_t<Single_>;
};

template <typename R1_, typename R2_, typename... Rest_, typename AltsTuple_,
          typename... Arms_>
struct alignas(1) __match_unwrapped_return_type_<std::tuple<R1_, R2_, Rest_...>,
                                                 AltsTuple_, Arms_...> {
    static_assert((!std::is_void_v<R1_> && !std::is_void_v<R2_> &&
                   (!std::is_void_v<Rest_> && ...)),
                  "match_unwrapped: all handlers must return "
                  "either void or non-void");

  public:
    using type = Opt::Option<R1_, R2_, Rest_...>;
};

}; /* namespace details */

namespace details {
template <typename... Types_> union alignas(1) Alternative_;

template <

    >
union alignas(1) Alternative_<> {
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
                 typename option_index_type_<sizeof...(Args_) + 1>::type>())
    OptionStorage_ {
  private:
    using Index_ = typename option_index_type_<sizeof...(Args_) + 1>::type;

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
            return (this->index_ == static_cast<Index_>(1));
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
        }

        else {
            destroy_alternative_<Qp_>(alternative_.rest_);
        }
    }

  private:
    template <typename Qp_, typename... CtorArgs_>
    constexpr auto construct_(CtorArgs_ &&...args) & noexcept(
        std::is_nothrow_constructible_v<std::remove_cv_t<Qp_>, CtorArgs_...>)
        [[RL_LIFETIME_BOUND]] -> Qp_ & {
        if constexpr (std::same_as<Qp_, Tp_>) {
            auto *ptr_ = std::addressof(this->storage.value);
            std::construct_at(const_cast<std::remove_cv_t<Tp_> *>(ptr_),
                              std::forward<CtorArgs_>(args)...);

            this->is_some.index_ = this->index_of_<Qp_>();
            return static_cast<Qp_ &>(*ptr_);
        }

        else {
            std::construct_at(std::addressof(this->storage.alternatives));

            auto &value_ = construct_alternative_<Qp_>(
                this->storage.alternatives, std::forward<CtorArgs_>(args)...);

            this->is_some.index_ = this->index_of_<Qp_>();
            return static_cast<Qp_ &>(value_);
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

        if (other_.is_some.index_ != 0) {
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

        if (other_.is_some.index_ != 0) {
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

        if (other_.is_some.index_ != 0) {
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

        if (other_.is_some.index_ != 0) {
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
    constexpr auto operator=(const OptionStorage_ &other_) -> OptionStorage_ &
        requires((std::is_copy_constructible_v<Tp_> &&
                  (std::is_copy_constructible_v<Args_> && ...)) &&
                 !_trivial_copy_assign_)
    {
        if (this != std::addressof(other_)) {
            if (this->is_some.index_ != 0 &&
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
            if (this->is_some.index_ != 0 &&
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
    constexpr auto reset_(void) noexcept(true) -> void {
        const std::size_t index_{
            static_cast<std::size_t>(this->is_some.index_)};

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
    template <typename Qp_>
    constexpr auto has_type_(void) const noexcept(true) -> bool {
        return (static_cast<std::size_t>(this->is_some.index_) ==
                static_cast<std::size_t>(this->index_of_<Qp_>()));
    }

  private:
    template <typename Qp_>
    static constexpr auto
    active_alternative_ptr_(OptionStorage_ &self_
                            [[RL_LIFETIME_BOUND]]) noexcept(true) -> void * {
        return static_cast<void *>(
            alternative_ptr_<Qp_>(std::addressof(self_.storage.alternatives)));
    }

  private:
    template <typename Qp_>
    static constexpr auto
    active_alternative_ptr_(const OptionStorage_ &self_
                            [[RL_LIFETIME_BOUND]]) noexcept(true)
        -> void const * {
        return static_cast<void const *>(
            alternative_ptr_<Qp_>(std::addressof(self_.storage.alternatives)));
    }

  private:
    constexpr auto active_value_ptr_(void) & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> void * {
        if (this->is_some.index_ == 0) {
            return nullptr;
        }

        if (this->is_some.index_ == 1) {
            return std::addressof(this->storage.value);
        }

        using Fn_ = void *(*)(OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_)> dispatch_{
            &active_alternative_ptr_<Args_>...};

        return (dispatch_[static_cast<std::size_t>(this->is_some.index_ - 2)](
            *this));
    }

// active_value_ptr_ returns nullptr when None; clang lifetime-safety cannot
// statically verify the binding for the nullptr branch — suppress the
// suggestion.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-intra-tu-suggestions"
#endif
  private:
    constexpr auto active_value_ptr_(void) const & noexcept(true)
        -> void const * {
        if (this->is_some.index_ == 0) {
            return nullptr;
        }

        if (this->is_some.index_ == 1) {
            return std::addressof(this->storage.value);
        }

        using Fn_ = void const *(*)(const OptionStorage_ &);

        static constexpr std::array<Fn_, sizeof...(Args_)> dispatch_{
            &active_alternative_ptr_<Args_>...};

        return (dispatch_[static_cast<std::size_t>(this->is_some.index_ - 2)](
            *this));
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  private:
    constexpr auto swap_(OptionStorage_ &other_) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_move_assignable_v<Tp_> &&
         (std::is_nothrow_move_assignable_v<Args_> && ...)) &&
        (std::is_nothrow_destructible_v<Tp_> &&
         (std::is_nothrow_destructible_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::is_move_assignable_v<Tp_> &&
                 (std::is_move_assignable_v<Args_> && ...))
    {
        if (this != std::addressof(other_)) {
            OptionStorage_ temp_{std::move(*this)};
            *this = std::move(other_);
            other_ = std::move(temp_);
        }
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
    using Index_ = typename option_index_type_<sizeof...(Args_) + 1>::type;
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
        typename details::option_index_type_<sizeof...(Args_) + 1>::type>())
    [[nodiscard]] Option {
  private:
    using Data_ = details::OptionStorage_<Tp_, Args_...>;

  private:
    Data_ data;

  private:
    template <typename From_, typename To_>
    using ForwardLike_t = std::conditional_t<
        std::is_rvalue_reference_v<From_ &&>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<From_>>,
                           const To_ &&, To_ &&>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<From_>>,
                           const To_ &, To_ &>>;

  private:
    template <typename Rp_, typename Self_>
    using ArmFor_ = details::ArmMulti_<Rp_, Self_, Tp_, Args_...>;

  private:
    template <typename Ap_> using ArmSpan_ = std::span<const Ap_>;

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
            std::abort();
        }

        return this->data.storage.value;
    }

  private:
    constexpr auto primary_value_(void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> const Tp_ & {
        if (!this->has_primary_()) {
            std::abort();
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
        return static_cast<std::size_t>(this->data.is_some.index_);
    }

  private:
    constexpr auto has_primary_(void) const noexcept(true) -> bool {
        return this->template has_type_<Tp_>();
    }

// active_value_ptr_ can return nullptr (None case); suppress false-positive
// lifetimebound-violation — the return value is a raw pointer to storage in
// *this when non-null, which is correct.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-lifetimebound-violation"
#endif
  private:
    constexpr auto active_value_ptr_(void) & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> void * {
        return this->data.active_value_ptr_();
    }

  private:
    constexpr auto active_value_ptr_(void) const & noexcept(true)
        [[RL_LIFETIME_BOUND]] -> void const * {
        return this->data.active_value_ptr_();
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// Intentional float equality: mirroring std::optional<T>::operator==.
// Suppress -Wfloat-equal for the noexcept specifier and the body.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
  private:
    template <typename Qp_>
    static constexpr auto equal_values_(void const *lhs_, void const *rhs_)

#endif
        noexcept(noexcept(*static_cast<const Qp_ *>(lhs_) ==
                          *static_cast<const Qp_ *>(rhs_))) -> bool {
        return (*static_cast<const Qp_ *>(lhs_) ==
                *static_cast<const Qp_ *>(rhs_));
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  private:
    static constexpr auto equal_active_(std::size_t index_, void const *lhs_,
                                        void const *rhs_) -> bool {
        constexpr std::array<bool (*)(void const *, void const *),
                             sizeof...(Args_) + 1>
            dispatch_{&equal_values_<Tp_>, &equal_values_<Args_>...};

        if (index_ == 0 || index_ > dispatch_.size()) {
            return false;
        }

        return dispatch_[index_ - 1](lhs_, rhs_);
    }

    static_assert(details::unique_types_<Tp_, Args_...>::value,
                  "Option<T, Args...> requires distinct alternative types");

    static_assert(std::is_object_v<Tp_> && !std::is_array_v<Tp_> &&
                      ((std::is_object_v<Args_> && !std::is_array_v<Args_>) &&
                       ...),
                  "Option<T, Args...> requires non-array object types");

    static_assert(details::option_storage_layout_valid_<Tp_, Args_...>());

  private:
    using value_type = Tp_;

  public:
    constexpr explicit operator bool(void) const noexcept(true) {
        return this->is_some();
    }

  public:
    constexpr auto has_any_engaged_value [[nodiscard]] (void) const
        noexcept(true) -> bool {
        return this->is_some();
    }

  public:
    constexpr Option(void) = delete;

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
    template <typename Up_, typename Target_ = typename details::
                                match_alternative_<Up_, Tp_, Args_...>::type>
    constexpr Option(Up_ &&val_) noexcept(
        std::is_nothrow_constructible_v<Target_, Up_> &&
        std::is_nothrow_move_constructible_v<Target_>)
        requires(!details::__is_option_<Up_> &&
                 !details::is_some_pattern_<std::remove_cvref_t<Up_>>::value &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::None> &&
                 !std::same_as<std::remove_cvref_t<Up_>, Opt::Default> &&
                 details::match_alternative_<Up_, Tp_, Args_...>::valid)
        : data{Opt::Some<Target_>(std::forward<Up_>(val_))} {}

  public:
    constexpr Option(const Option &)
        requires(std::is_copy_constructible_v<Tp_> &&
                 (std::is_copy_constructible_v<Args_> && ...))
    = default;

  public:
    constexpr Option(const Option &)
        requires(not(std::is_copy_constructible_v<Tp_> &&
                     (std::is_copy_constructible_v<Args_> && ...)))
    = delete;

  public:
    constexpr Option(Option &&)
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...))
    = default;

  public:
    constexpr Option(Option &&)
        requires(not(std::is_move_constructible_v<Tp_> &&
                     (std::is_move_constructible_v<Args_> && ...)))
    = delete;

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
    = delete;

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
    = delete;

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
    template <typename Qp_, typename Up_>
    constexpr auto map_or_transform [[nodiscard]] (Qp_ &&op, Up_ &&v_) const &
        requires(std::invocable<Qp_, const Tp_ &> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, const Tp_ &>> &&
                 std::is_constructible_v<
                     std::remove_cvref_t<Up_>,
                     std::invoke_result_t<Qp_, const Tp_ &>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>) ||
                (std::invocable<Qp_, Opt::None> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<
                     std::remove_cvref_t<Up_>,
                     std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (this->is_none()) {
            if constexpr (std::invocable<Qp_, Opt::None>) {
                return (static_cast<Rp_>(
                    std::invoke(std::forward<Qp_>(op), Opt::None())));
            }

            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        if (!this->has_primary_()) {
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }

        if constexpr (std::invocable<Qp_, const Tp_ &>) {
            return (static_cast<Rp_>(
                std::invoke(std::forward<Qp_>(op), this->primary_value_())));
        }

        else {
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or_transform [[nodiscard]] (Qp_ &&op, Up_ &&v_) &
        requires(std::invocable<Qp_, Tp_ &> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>,
                                         std::invoke_result_t<Qp_, Tp_ &>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>) ||
                (std::invocable<Qp_, Opt::None> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<
                     std::remove_cvref_t<Up_>,
                     std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (this->is_none()) {
            if constexpr (std::invocable<Qp_, Opt::None>) {
                return (static_cast<Rp_>(
                    std::invoke(std::forward<Qp_>(op), Opt::None())));
            }

            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        if (!this->has_primary_()) {
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }

        if constexpr (std::invocable<Qp_, Tp_ &>) {
            return (static_cast<Rp_>(
                std::invoke(std::forward<Qp_>(op), this->primary_value_())));
        }

        else {
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or_transform [[nodiscard]] (Qp_ &&op, Up_ &&v_) &&
        requires(std::invocable<Qp_, Tp_ &&> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &&>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>,
                                         std::invoke_result_t<Qp_, Tp_ &&>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>) ||
                (std::invocable<Qp_, Opt::None> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<
                     std::remove_cvref_t<Up_>,
                     std::invoke_result_t<Qp_, Opt::None>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (this->is_none()) {
            if constexpr (std::invocable<Qp_, Opt::None>) {
                return (static_cast<Rp_>(
                    std::invoke(std::forward<Qp_>(op), Opt::None())));
            }

            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        if (!this->has_primary_()) {
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }

        if constexpr (std::invocable<Qp_, Tp_ &&>) {
            return (this->consume_unchecked([&op](Tp_ &&value) -> Rp_ {
                return static_cast<Rp_>(
                    std::invoke(std::forward<Qp_>(op), std::move(value)));
            }));
        }

        else {
            this->reset();
            return static_cast<Rp_>(std::forward<Up_>(v_));
        }
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or_else [[nodiscard]] (Qp_ &&op, Up_ &&f_) const &
        requires(
            std::is_invocable_v<Up_> && std::is_invocable_v<Qp_, const Tp_ &> &&
            std::is_same_v<
                std::remove_cvref_t<std::invoke_result_t<Up_>>,
                std::remove_cvref_t<std::invoke_result_t<Qp_, const Tp_ &>>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, const Tp_ &>>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::invoke(std::forward<Up_>(f_))));
        }

        return (static_cast<Rp_>(
            std::invoke(std::forward<Qp_>(op), this->primary_value_())));
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or_else [[nodiscard]] (Qp_ &&op, Up_ &&f_) &
        requires(std::is_invocable_v<Up_> && std::is_invocable_v<Qp_, Tp_ &> &&
                 std::is_same_v<
                     std::remove_cvref_t<std::invoke_result_t<Up_>>,
                     std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &>>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &>>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::invoke(std::forward<Up_>(f_))));
        }

        return (static_cast<Rp_>(
            std::invoke(std::forward<Qp_>(op), this->primary_value_())));
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or_else [[nodiscard]] (Qp_ &&op, Up_ &&f_) &&
        requires(std::is_invocable_v<Up_> && std::is_invocable_v<Qp_, Tp_ &&> &&
                 std::is_same_v<
                     std::remove_cvref_t<std::invoke_result_t<Up_>>,
                     std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &&>>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &&>>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::invoke(std::forward<Up_>(f_))));
        }

        return (this->consume_unchecked([&op](Tp_ &&value) -> Rp_ {
            return static_cast<Rp_>(
                std::invoke(std::forward<Qp_>(op), std::move(value)));
        }));
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or_else [[nodiscard]] (Qp_ &&op) const & -> Tp_
        requires(
            std::is_copy_constructible_v<Tp_> &&
            ((std::is_invocable_v<Qp_> &&
              (std::convertible_to<std::invoke_result_t<Qp_>, Tp_> ||
               std::constructible_from<Tp_, std::invoke_result_t<Qp_>>)) ||
             (std::invocable<Qp_, Opt::None> &&
              (std::convertible_to<std::invoke_result_t<Qp_, Opt::None>, Tp_> ||
               std::constructible_from<Tp_,
                                       std::invoke_result_t<Qp_, Opt::None>>))))
    {
        if (this->has_primary_()) {
            return {this->primary_value_()};
        }

        if constexpr (std::invocable<Qp_>) {
            return (static_cast<Tp_>(std::invoke(std::forward<Qp_>(op))));
        }

        else {
            return (static_cast<Tp_>(
                std::invoke(std::forward<Qp_>(op), Opt::None())));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or_else [[nodiscard]] (Qp_ &&op) & -> Tp_
        requires(
            std::is_copy_constructible_v<Tp_> &&
            ((std::invocable<Qp_> &&
              (std::convertible_to<std::invoke_result_t<Qp_>, Tp_> ||
               std::constructible_from<Tp_, std::invoke_result_t<Qp_>>)) ||
             (std::invocable<Qp_, Opt::None> &&
              (std::convertible_to<std::invoke_result_t<Qp_, Opt::None>, Tp_> ||
               std::constructible_from<Tp_,
                                       std::invoke_result_t<Qp_, Opt::None>>))))
    {
        if (this->has_primary_()) {
            return this->primary_value_();
        }

        if constexpr (std::invocable<Qp_>) {
            return (static_cast<Tp_>(std::invoke(std::forward<Qp_>(op))));
        }

        else {
            return (static_cast<Tp_>(
                std::invoke(std::forward<Qp_>(op), Opt::None())));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or_else [[nodiscard]] (Qp_ &&op) && -> Tp_
        requires(
            std::is_move_constructible_v<Tp_> &&
            ((std::invocable<Qp_> &&
              (std::convertible_to<std::invoke_result_t<Qp_>, Tp_> ||
               std::constructible_from<Tp_, std::invoke_result_t<Qp_>>)) ||
             (std::invocable<Qp_, Opt::None> &&
              (std::convertible_to<std::invoke_result_t<Qp_, Opt::None>, Tp_> ||
               std::constructible_from<Tp_,
                                       std::invoke_result_t<Qp_, Opt::None>>))))
    {
        if (this->has_primary_()) {
            return this->take_unchecked();
        }

        if constexpr (std::invocable<Qp_>) {
            return (static_cast<Tp_>(std::invoke(std::forward<Qp_>(op))));
        }

        else {
            return (static_cast<Tp_>(
                std::invoke(std::forward<Qp_>(op), Opt::None())));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto or_else
        [[nodiscard]] (Qp_ &&op) const & -> Opt::Option<Tp_, Args_...>
        requires(std::is_copy_constructible_v<Tp_> &&
                 (std::is_copy_constructible_v<Args_> && ...) &&
                 ((std::invocable<Qp_> &&
                   std::convertible_to<std::invoke_result_t<Qp_>,
                                       Opt::Option<Tp_, Args_...>>) ||
                  (std::invocable<Qp_, Opt::None> &&
                   std::convertible_to<std::invoke_result_t<Qp_, Opt::None>,
                                       Opt::Option<Tp_, Args_...>>)))
    {
        if (this->is_some()) {
            return *this;
        }

        if constexpr (std::invocable<Qp_>) {
            return (std::invoke(std::forward<Qp_>(op)));
        }

        else {
            return (std::invoke(std::forward<Qp_>(op), Opt::None()));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto or_else
        [[nodiscard]] (Qp_ &&op) & -> Opt::Option<Tp_, Args_...>
        requires(std::is_copy_constructible_v<Tp_> &&
                 (std::is_copy_constructible_v<Args_> && ...) &&
                 ((std::invocable<Qp_> &&
                   std::convertible_to<std::invoke_result_t<Qp_>,
                                       Opt::Option<Tp_, Args_...>>) ||
                  (std::invocable<Qp_, Opt::None> &&
                   std::convertible_to<std::invoke_result_t<Qp_, Opt::None>,
                                       Opt::Option<Tp_, Args_...>>)))
    {
        if (this->is_some()) {
            return *this;
        }

        if constexpr (std::invocable<Qp_>) {
            return std::invoke(std::forward<Qp_>(op));
        }

        else {
            return (std::invoke(std::forward<Qp_>(op), Opt::None()));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto or_else
        [[nodiscard]] (Qp_ &&op) && -> Opt::Option<Tp_, Args_...>
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 ((std::invocable<Qp_> &&
                   std::convertible_to<std::invoke_result_t<Qp_>,
                                       Opt::Option<Tp_, Args_...>>) ||
                  (std::invocable<Qp_, Opt::None> &&
                   std::convertible_to<std::invoke_result_t<Qp_, Opt::None>,
                                       Opt::Option<Tp_, Args_...>>)))
    {
        if (this->is_some()) {
            return std::move(*this);
        }

        if constexpr (std::invocable<Qp_>) {
            return std::invoke(std::forward<Qp_>(op));
        }

        else {
            return (std::invoke(std::forward<Qp_>(op), Opt::None()));
        }
    }

  public:
    template <typename Qp_>
    constexpr auto map [[nodiscard]] (Qp_ &&op) const &
        requires(std::invocable<Qp_, const Tp_ &> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, const Tp_ &>>)
    {
        using Up_ = std::remove_cvref_t<std::invoke_result_t<Qp_, const Tp_ &>>;

        if (!this->has_primary_()) {
            return Opt::Option<Up_>(Opt::None());
        }

        return (Opt::Option<Up_>(Opt::Some<Up_>{
            std::invoke(std::forward<Qp_>(op), this->primary_value_())}));
    }

  public:
    template <typename Qp_>
    constexpr auto map [[nodiscard]] (Qp_ &&op) &
        requires(std::invocable<Qp_, Tp_ &> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &>>)
    {
        using Up_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &>>;

        if (!this->has_primary_()) {
            return Opt::Option<Up_>(Opt::None());
        }

        return (Opt::Option<Up_>(Opt::Some<Up_>{
            std::invoke(std::forward<Qp_>(op), this->primary_value_())}));
    }

  public:
    template <typename Qp_>
    constexpr auto map [[nodiscard]] (Qp_ &&op) &&
        requires(std::invocable<Qp_, Tp_ &&> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &&>>)
    {
        using Up_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &&>>;

        if (!this->has_primary_()) {
            return Opt::Option<Up_>(Opt::None());
        }

        return this->consume_unchecked([&op](Tp_ &&value) -> Opt::Option<Up_> {
            return Opt::Option<Up_>(Opt::Some<Up_>{
                std::invoke(std::forward<Qp_>(op), std::move(value))});
        });
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or [[nodiscard]] (Qp_ &&op, Up_ &&v_) const &
        requires(
            std::invocable<Qp_, const Tp_ &> &&
            !std::is_void_v<std::invoke_result_t<Qp_, const Tp_ &>> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>,
                                    std::invoke_result_t<Qp_, const Tp_ &>> &&
            std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        return (static_cast<Rp_>(
            std::invoke(std::forward<Qp_>(op), this->primary_value_())));
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or [[nodiscard]] (Qp_ &&op, Up_ &&v_) &
        requires(std::invocable<Qp_, Tp_ &> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>,
                                         std::invoke_result_t<Qp_, Tp_ &>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        return (static_cast<Rp_>(
            std::invoke(std::forward<Qp_>(op), this->primary_value_())));
    }

  public:
    template <typename Qp_, typename Up_>
    constexpr auto map_or [[nodiscard]] (Qp_ &&op, Up_ &&v_) &&
        requires(std::invocable<Qp_, Tp_ &&> &&
                 !std::is_void_v<std::invoke_result_t<Qp_, Tp_ &&>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>,
                                         std::invoke_result_t<Qp_, Tp_ &&>> &&
                 std::is_constructible_v<std::remove_cvref_t<Up_>, Up_>)
    {
        using Rp_ = std::remove_cvref_t<Up_>;

        if (!this->has_primary_()) {
            return (static_cast<Rp_>(std::forward<Up_>(v_)));
        }

        return this->consume_unchecked([&op](Tp_ &&value) -> Rp_ {
            return (static_cast<Rp_>(
                std::invoke(std::forward<Qp_>(op), std::move(value))));
        });
    }

  public:
    template <typename... CtorArgs_>
    constexpr auto emplace(CtorArgs_ &&...args) & noexcept(
        std::is_nothrow_constructible_v<Tp_, CtorArgs_...>)
        [[RL_LIFETIME_BOUND]] -> Tp_ &
        requires(std::is_constructible_v<Tp_, CtorArgs_...>)
    {
        this->reset();

        std::construct_at(const_cast<std::remove_cv_t<Tp_> *>(
                              std::addressof(this->data.storage.value)),
                          std::forward<CtorArgs_>(args)...);

        this->data.is_some = true;

        return this->primary_value_();
    }

  public:
    template <typename... CtorArgs_>
    constexpr auto emplace(CtorArgs_ &&...args) && -> Tp_
        requires(std::is_constructible_v<Tp_, CtorArgs_...> &&
                 std::is_move_constructible_v<Tp_>)
    {
        this->emplace(std::forward<CtorArgs_>(args)...);

        return this->take_unchecked();
    }

  public:
    template <typename Qp_>
    constexpr auto and_then [[nodiscard]] (Qp_ &&op) const &
        requires(std::invocable<Qp_, const Tp_ &> &&
                 details::__is_option_<std::invoke_result_t<Qp_, const Tp_ &>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, const Tp_ &>>;

        if (!this->has_primary_()) {
            return Rp_(Opt::None());
        }

        return (std::invoke(std::forward<Qp_>(op), this->primary_value_()));
    }

  public:
    template <typename Qp_>
    constexpr auto and_then [[nodiscard]] (Qp_ &&op) &
        requires(std::invocable<Qp_, Tp_ &> &&
                 details::__is_option_<std::invoke_result_t<Qp_, Tp_ &>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &>>;

        if (!this->has_primary_()) {
            return Rp_(Opt::None());
        }

        return (std::invoke(std::forward<Qp_>(op), this->primary_value_()));
    }

  public:
    template <typename Qp_>
    constexpr auto and_then [[nodiscard]] (Qp_ &&op) &&
        requires(std::invocable<Qp_, Tp_ &&> &&
                 details::__is_option_<std::invoke_result_t<Qp_, Tp_ &&>>)
    {
        using Rp_ = std::remove_cvref_t<std::invoke_result_t<Qp_, Tp_ &&>>;

        if (!this->has_primary_()) {
            return Rp_(Opt::None());
        }

        return this->consume_unchecked([&op](Tp_ &&value) -> Rp_ {
            return (std::invoke(std::forward<Qp_>(op), std::move(value)));
        });
    }

  public:
    template <typename... Rp_, typename Self_, typename... Arms_>
    constexpr auto match [[nodiscard]] (this Self_ &&self, Arms_ &&...arms) ->
        typename details::__match_unwrapped_return_type_<
            std::tuple<Rp_...>,
            std::tuple<ForwardLike_t<Self_, Tp_>,
                       ForwardLike_t<Self_, Args_>...>,
            Arms_...>::type
        requires(sizeof...(Arms_) > 0)
    {
        using ReturnType_ = typename details::__match_unwrapped_return_type_<
            std::tuple<Rp_...>,
            std::tuple<ForwardLike_t<Self_, Tp_>,
                       ForwardLike_t<Self_, Args_>...>,
            Arms_...>::type;

        using ArmT_ = ArmFor_<ReturnType_, Self_>;
        const ArmT_ arm_arr_[] = {ArmT_(std::forward<Arms_>(arms))...};

        return (std::forward<Self_>(self).template match_unwrapped<ReturnType_>(
            ArmSpan_<ArmT_>(arm_arr_, sizeof...(Arms_))));
    }

  private:
    template <typename Rp_, typename Self_>
    constexpr auto match_unwrapped
        [[nodiscard]] (this Self_ &&self, ArmSpan_<ArmFor_<Rp_, Self_>> arms)
            -> Rp_ {

        using ArmT_ = ArmFor_<Rp_, Self_>;

        const bool has_none_ =
            (std::ranges::find_if(arms, [](const ArmT_ &arm_) {
                 return arm_.present_ && arm_.matches_none_;
             }) != arms.end());

        const bool has_some_ =
            (std::ranges::find_if(arms, [](const ArmT_ &arm_) {
                 return arm_.present_ && arm_.matches_some_;
             }) != arms.end());

        const std::size_t active_{self.active_index_()};
        void *value_{const_cast<void *>(self.active_value_ptr_())};

        auto indexed_arms_ = std::views::zip(
            std::views::iota(std::size_t{0}, arms.size()), arms);

        const auto matched_ = std::ranges::find_if(
            indexed_arms_, [active_, value_](const auto &indexed_arm_) {
                const ArmT_ &arm_ = std::get<1>(indexed_arm_);

                return (arm_.present_ && arm_.accept_ != nullptr &&
                        arm_.call_ != nullptr &&
                        arm_.accept_(arm_.pat_, active_, value_));
            });

        if (matched_ != indexed_arms_.end()) {
            const ArmT_ &arm_ = arms[std::get<0>(*matched_)];
            return arm_.call_(arm_.fn_, active_, value_);
        }

        if (!has_none_ || !has_some_) {
            auto _x = ((!has_none_ && !has_some_)
                           ? "None, Some(_)"
                           : (!has_none_ ? "None" : "Some(_)"));

            std::println(stderr,
                         "Option<{}>::match_unwrapped() failed - "
                         "non-exhaustive, missing arm for: {}",
                         self.template get_type_name<Tp_>(), _x);

            std::abort();
        }

        std::println(stderr,
                     "Option<{}>::match_unwrapped() failed - "
                     "no matching pattern arm",
                     self.template get_type_name<Tp_>());

        std::abort();
    }

  public:
    constexpr auto expect
        [[nodiscard]] (std::string_view msg) const &[[RL_LIFETIME_BOUND]]
        -> const Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr, "Option<{}>::expect() failed: {}",
                         this->get_type_name<Tp_>(), msg);
            std::abort();
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto expect
        [[nodiscard]] (std::string_view msg) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr, "Option<{}>::expect() failed: {}",
                         this->get_type_name<Tp_>(), msg);

            std::abort();
        }

        return this->primary_value_();
    }

  public:
    constexpr auto expect [[nodiscard]] (std::string_view msg) && -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            std::println(stderr, "Option<{}>::expect() failed: {}",
                         this->get_type_name<Tp_>(), msg);

            std::abort();
        }

        return this->take_unchecked();
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::unwrap() failed: "
                         "Option does not wield any value.",
                         this->get_type_name<Tp_>());

            std::abort();
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::unwrap() failed: "
                         "Option does not wield any value.",
                         this->get_type_name<Tp_>());

            std::abort();
        }

        return this->primary_value_();
    }

  public:
    constexpr auto unwrap [[nodiscard]] (void) && -> Tp_
        requires(std::is_move_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::unwrap(): Option does not "
                         "wield any value.",
                         this->get_type_name<Tp_>());

            std::abort();
        }

        return this->take_unchecked();
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Qp_ &
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            std::println(stderr,
                         "Option<{}>::unwrap<{}>() failed: active "
                         "alternative does not match.",
                         this->get_type_name<Tp_>(),
                         this->template get_type_name<Qp_>());

            std::abort();
        }

        return this->template get_<Qp_>();
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Qp_ &
        requires(std::same_as<Qp_, Tp_> || (std::same_as<Qp_, Args_> || ...))
    {
        if (!this->template has_type_<Qp_>()) {
            std::println(stderr,
                         "Option<{}>::unwrap<{}>() failed: active "
                         "alternative does not match.",
                         this->get_type_name<Tp_>(),
                         this->template get_type_name<Qp_>());

            std::abort();
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
            std::println(stderr,
                         "Option<{}>::unwrap<{}>() failed: active "
                         "alternative does not match.",
                         this->get_type_name<Tp_>(),
                         this->template get_type_name<Qp_>());

            std::abort();
        }

// ResetOnSuccess_ is a local RAII guard; mixed int+pointer layout causes
// unavoidable padding. Suppress -Wpadded for this local struct only.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
        struct alignas(void *) ResetOnSuccess_ {
            // initial_uncaught_ first to avoid tail-padding
            // between int and pointer members.
            int initial_uncaught_ = 0;
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
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

        ResetOnSuccess_ guard_{this};
        return static_cast<Qp_ &&>(this->template get_<Qp_>());
    }

  public:
    constexpr auto value [[nodiscard]] (void) const &[[RL_LIFETIME_BOUND]]
    -> const Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::value() failed - Bad access: "
                         "Option does not hold a value!",
                         this->get_type_name<Tp_>());
            std::abort();
        }

        return {this->primary_value_()};
    }

  public:
    constexpr auto value [[nodiscard]] (void) &[[RL_LIFETIME_BOUND]] -> Tp_ & {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::value() failed - Bad access: "
                         "Option does not hold a value!",
                         this->get_type_name<Tp_>());
            std::abort();
        }

        return this->primary_value_();
    }

  public:
    constexpr auto value [[nodiscard]] (void) &&[[RL_LIFETIME_BOUND]]
    -> Tp_ && {
        if (!this->has_primary_()) {
            std::println(stderr,
                         "Option<{}>::value() failed - Bad access: "
                         "Option does not hold a value!",
                         this->get_type_name<Tp_>());
            std::abort();
        }

        return {std::move(this->primary_value_())};
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) const & -> Opt::None {
        if (this->is_some()) {
            std::println(stderr,
                         "Option<{}>::unwrap_none() failed - Bad access: "
                         "Option holds a value!",
                         this->get_type_name<Tp_>());
            std::abort();
        }

        return Opt::None();
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) & -> Opt::None {
        if (this->is_some()) {
            std::println(stderr,
                         "Option<{}>::unwrap_none() failed - Bad access: "
                         "Option holds a value!",
                         this->get_type_name<Tp_>());
            std::abort();
        }

        return Opt::None();
    }

  public:
    constexpr auto unwrap_none [[nodiscard]] (void) && -> Opt::None {
        if (this->is_some()) {
            std::println(stderr,
                         "Option<{}>::unwrap_none() failed - Bad access: "
                         "Option holds a value!",
                         this->get_type_name<Tp_>());
            std::abort();
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
// ResetOnSuccess_ is a local RAII guard; mixed int+pointer layout causes
// unavoidable padding. Suppress -Wpadded for this local struct only.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
        struct alignas(void *) ResetOnSuccess_ {
            // initial_uncaught_ first to avoid tail-padding
            // between int and pointer members.
            int initial_uncaught_ = 0;
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
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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
        Option<Tp_, Args_...> old_ = this->take();
        *this = std::forward<Up_>(new_val);
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
        if (!this->has_primary_()) {
            return false;
        }
        const Tp_ def_{};
        return this->primary_value_() == def_;
    }

  public:
    template <typename Qp_>
    constexpr auto is_some_and [[nodiscard]] (Qp_ &&prop_) const & noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, const Tp_ &>) -> bool
        requires(
            std::invocable<Qp_, const Tp_ &> &&
            std::convertible_to<std::invoke_result_t<Qp_, const Tp_ &>, bool>)
    {
        return (this->has_primary_() &&
                static_cast<bool>(std::invoke(std::forward<Qp_>(prop_),
                                              this->primary_value_())));
    }

  public:
    template <typename Qp_>
    constexpr auto is_some_and [[nodiscard]] (Qp_ &&prop_) & noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &>) -> bool
        requires(std::invocable<Qp_, Tp_ &> &&
                 std::convertible_to<std::invoke_result_t<Qp_, Tp_ &>, bool>)
    {
        return (this->has_primary_() &&
                static_cast<bool>(std::invoke(std::forward<Qp_>(prop_),
                                              this->primary_value_())));
    }

  public:
    template <typename Qp_>
    constexpr auto is_some_and [[nodiscard]] (Qp_ &&prop_) && noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &&>) -> bool
        requires(std::invocable<Qp_, Tp_ &&> &&
                 std::convertible_to<std::invoke_result_t<Qp_, Tp_ &&>, bool>)
    {
        if (!this->has_primary_()) {
            return false;
        }

        return this->consume_unchecked(
            [&prop_](Tp_ &&value) noexcept(
                std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &&>) -> bool {
                return (static_cast<bool>(
                    std::invoke(std::forward<Qp_>(prop_), std::move(value))));
            });
    }

  public:
    template <typename Qp_>
    constexpr auto is_none_or [[nodiscard]] (Qp_ &&prop_) const & noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, const Tp_ &>) -> bool
        requires(
            std::invocable<Qp_, const Tp_ &> &&
            std::convertible_to<std::invoke_result_t<Qp_, const Tp_ &>, bool>)
    {
        return (this->is_none() ||
                (this->has_primary_() &&
                 static_cast<bool>(std::invoke(std::forward<Qp_>(prop_),
                                               this->primary_value_()))));
    }

  public:
    template <typename Qp_>
    constexpr auto is_none_or [[nodiscard]] (Qp_ &&prop_) & noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &>) -> bool
        requires(std::invocable<Qp_, Tp_ &> &&
                 std::convertible_to<std::invoke_result_t<Qp_, Tp_ &>, bool>)
    {
        return (this->is_none() ||
                (this->has_primary_() &&
                 static_cast<bool>(std::invoke(std::forward<Qp_>(prop_),
                                               this->primary_value_()))));
    }

  public:
    template <typename Qp_>
    constexpr auto is_none_or [[nodiscard]] (Qp_ &&prop_) && noexcept(
        std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &&>) -> bool
        requires(std::invocable<Qp_, Tp_ &&> &&
                 std::convertible_to<std::invoke_result_t<Qp_, Tp_ &&>, bool>)
    {
        if (this->is_none()) {
            return true;
        }

        if (!this->has_primary_()) {
            return false;
        }

        return this->consume_unchecked(
            [&prop_](Tp_ &&value) noexcept(
                std::is_nothrow_invocable_r_v<bool, Qp_, Tp_ &&>) -> bool {
                return (static_cast<bool>(
                    std::invoke(std::forward<Qp_>(prop_), std::move(value))));
            });
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or [[nodiscard]] (Qp_ &&v_) const & -> Tp_
        requires(std::is_copy_constructible_v<Tp_> &&
                 std::convertible_to<Qp_, Tp_>)
    {
        if (this->has_primary_()) {
            return {this->primary_value_()};
        }

        return {static_cast<Tp_>(std::forward<Qp_>(v_))};
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or [[nodiscard]] (Qp_ &&v_) & -> Tp_
        requires(std::is_copy_constructible_v<Tp_> &&
                 std::convertible_to<Qp_, Tp_>)
    {
        if (this->has_primary_()) {
            return {this->primary_value_()};
        }

        return {static_cast<Tp_>(std::forward<Qp_>(v_))};
    }

  public:
    template <typename Qp_>
    constexpr auto unwrap_or [[nodiscard]] (Qp_ &&v_) && -> Tp_
        requires(std::is_move_constructible_v<Tp_> &&
                 std::convertible_to<Qp_, Tp_>)
    {
        if (this->has_primary_()) {
            return this->take_unchecked();
        }

        return {static_cast<Tp_>(std::forward<Qp_>(v_))};
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
        return std::move(
            *static_cast<Tp_ *>(std::addressof(this->data.storage.value)));
    }

  public:
    constexpr auto unwrap_or_default [[nodiscard]] (void) const & -> Tp_
        requires(std::is_copy_constructible_v<Tp_> &&
                 std::is_default_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            return Tp_{};
        }

        return this->primary_value_();
    }

  public:
    constexpr auto unwrap_or_default [[nodiscard]] (void) & -> Tp_
        requires(std::is_copy_constructible_v<Tp_> &&
                 std::is_default_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            return Tp_{};
        }

        return this->primary_value_();
    }

  public:
    constexpr auto unwrap_or_default [[nodiscard]] (void) && -> Tp_
        requires(std::is_move_constructible_v<Tp_> &&
                 std::is_default_constructible_v<Tp_>)
    {
        if (!this->has_primary_()) {
            return Tp_{};
        }

        return this->take_unchecked();
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

        return (Option::equal_active_(lhs_index_, lhs.active_value_ptr_(),
                                      rhs.active_value_ptr_()));
    }

  public:
    friend constexpr auto operator<=>(const Option<Tp_, Args_...> &lhs,
                                      const Option<Tp_, Args_...> &rhs)
        -> details::__compare_three_way_result_t<Tp_>
        requires(sizeof...(Args_) == 0 && std::three_way_comparable<Tp_>)
    {
        if (lhs.is_some() && rhs.is_some()) {
            return *lhs <=> *rhs;
        }
        return lhs.is_some() <=> rhs.is_some();
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
    constexpr auto swap(Option &other) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_move_assignable_v<Tp_> &&
         (std::is_nothrow_move_assignable_v<Args_> && ...)) &&
        (std::is_nothrow_destructible_v<Tp_> &&
         (std::is_nothrow_destructible_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::is_move_assignable_v<Tp_> &&
                 (std::is_move_assignable_v<Args_> && ...))
    {
        this->data.swap_(other.data);
    }

  public:
    friend constexpr auto swap(Option &lhs, Option &rhs) noexcept(
        (std::is_nothrow_move_constructible_v<Tp_> &&
         (std::is_nothrow_move_constructible_v<Args_> && ...)) &&
        (std::is_nothrow_move_assignable_v<Tp_> &&
         (std::is_nothrow_move_assignable_v<Args_> && ...)) &&
        (std::is_nothrow_destructible_v<Tp_> &&
         (std::is_nothrow_destructible_v<Args_> && ...))) -> void
        requires(std::is_move_constructible_v<Tp_> &&
                 (std::is_move_constructible_v<Args_> && ...) &&
                 std::is_move_assignable_v<Tp_> &&
                 (std::is_move_assignable_v<Args_> && ...))
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
        this->emplace(some.get());
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
        this->emplace(std::move(some).get());
        return *this;
    }

  public:
    template <typename Qp_>
    constexpr auto operator=(const typename Opt::Some<Qp_> &some) noexcept(
        std::is_nothrow_copy_constructible_v<Qp_>) -> Option &
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_copy_constructible_v<Qp_>)
    {
        this->reset();
        this->data.template emplace_<Qp_>(some.get());
        return *this;
    }

  public:
    template <typename Qp_>
    constexpr auto operator=(typename Opt::Some<Qp_> &&some) noexcept(
        std::is_nothrow_move_constructible_v<Qp_>) -> Option &
        requires((std::same_as<Qp_, Args_> || ...) &&
                 std::is_move_constructible_v<Qp_>)
    {
        this->reset();
        this->data.template emplace_<Qp_>(std::move(some).get());
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

        this->reset();
        this->data.template emplace_<Target_>(std::forward<Up_>(val_));
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
};

template <typename Tp_> Option(Some<Tp_>) -> Option<Tp_>;

using details::operator>>=;

}; /* namespace Opt */
}; // namespace details

namespace details {
// option_hash_base_ uses alignas(Tp_) which can produce padding when
// Tp_ has smaller alignment than std::size_t. Suppress -Wpadded here.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
template <typename Tp_>
concept is_hashable_ =
    requires(const std::hash<std::remove_const_t<Tp_>> &h_, const Tp_ &v_) {
        { h_(v_) } -> std::same_as<std::size_t>;
    };

template <typename Tp_, bool = is_hashable_<Tp_>>
struct alignas(Tp_) option_hash_base_ {
  public:
    constexpr option_hash_base_(void) = delete;

  public:
    constexpr option_hash_base_(const option_hash_base_ &) = delete;

  public:
    constexpr option_hash_base_(option_hash_base_ &&) = delete;

  public:
    constexpr auto operator=(const option_hash_base_ &)
        -> option_hash_base_ & = delete;

  public:
    constexpr auto operator=(option_hash_base_ &&)
        -> option_hash_base_ & = delete;
};

template <typename Tp_> struct alignas(Tp_) option_hash_base_<Tp_, true> {
  public:
    constexpr auto operator()(const Opt::Option<Tp_> &opt) const
        noexcept(noexcept(std::hash<std::remove_const_t<Tp_>>{}(
            std::declval<const Tp_ &>()))) -> std::size_t {
        if (opt.is_some()) {
            return std::hash<std::remove_const_t<Tp_>>{}(*opt);
        }

        constexpr std::size_t magic_disengaged_hash{
            static_cast<std::size_t>(-3333)};

        return magic_disengaged_hash;
    }
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}; // namespace details
}; /* namespace rl */

#undef RL_LIFETIME_BOUND
#undef RL_SUPPORTS_REFLECTION
#undef RL_IS_SUBVIEW_OR_SUBSTR

namespace rl {

/**
 * @brief Class `Option<Tp_>` template instantiation which accepts `T` type
 * or `None`.
 */

using details::Opt::Option;

/**
 * @brief Class `Some<Tp_>` template instantiation which accepts `T` or
 * `None` Type of `Option<Tp_>`.
 */
using details::Opt::Some;

/**
 * @brief Class `None` which accepts nothing. Tag to disengage optional
 * objects.
 */
inline constexpr auto None{details::Opt::None{}};

/**
 * @brief Class `Default` which constructs a default value if possible.
 */
inline constexpr auto Default{details::Opt::Default{}};

inline constexpr auto Wild{rl::details::Wild_{}};

inline constexpr auto SomeAny{rl::details::AnySome_{}};

using rl::details::operator>>=;

}; // namespace rl

using rl::details::operator>>=;

template <typename Tp_, typename... Args_>
struct std::hash<rl::details::Opt::Option<Tp_, Args_...>>
    : rl::details::option_hash_base_<Tp_, rl::details::is_hashable_<Tp_> &&
                                              (sizeof...(Args_) == 0)> {};

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
#endif
#endif /* RL_OPTION_CXX26_HPP */
