#include <iostream>
#include <span>
#include <tuple>
#include <type_traits>
#include <string>

template<typename... T>
struct Option {};

template<typename Rp, typename Self>
struct ArmFor {
    int x;
};

// Trait to resolve return type
template<typename ExplicitTuple_, typename AltsTuple_, typename... Arms_>
struct match_unwrapped_return_type_;

// Case 1: 0 explicit arguments -> deduced
template<typename AltsTuple_, typename... Arms_>
struct match_unwrapped_return_type_<std::tuple<>, AltsTuple_, Arms_...> {
    using type = std::string; // mock deduction
};

// Case 2: 1 explicit argument -> Single_
template<typename Single_, typename AltsTuple_, typename... Arms_>
struct match_unwrapped_return_type_<std::tuple<Single_>, AltsTuple_, Arms_...> {
    using type = Single_;
};

// Case 3: 2+ explicit arguments -> Option<R1, R2, Rest...>
template<typename R1_, typename R2_, typename... Rest_, typename AltsTuple_, typename... Arms_>
struct match_unwrapped_return_type_<std::tuple<R1_, R2_, Rest_...>, AltsTuple_, Arms_...> {
    using type = Option<R1_, R2_, Rest_...>;
};

struct S {
    // Overload 1: Span execution
    template<typename Rp, typename Self>
    Rp match_unwrapped(this Self&&, std::span<const ArmFor<Rp, Self>> arms) {
        std::cout << "Span execution! arms size: " << arms.size() << "\n";
        return Rp{};
    }

    // Overload 2: Variadic entry point with TMP return type computation!
    template<
        typename... Rp_,
        typename Self_,
        typename... Arms_
    >
    constexpr auto match_unwrapped(this Self_&& self, Arms_&&... arms)
    -> typename match_unwrapped_return_type_<
        std::tuple<Rp_...>,
        std::tuple<int, std::string>,
        Arms_...
    >::type
    requires ( sizeof...(Arms_) > 0 )
    {
        using ReturnType_ = typename match_unwrapped_return_type_<
            std::tuple<Rp_...>,
            std::tuple<int, std::string>,
            Arms_...
        >::type;

        using ArmT_ = ArmFor<ReturnType_, Self_>;
        const ArmT_ arm_arr[] = {
            ArmT_{ arms }...
        };
        return std::forward<Self_>(self).template match_unwrapped<ReturnType_>(
            std::span<const ArmT_>(arm_arr, sizeof...(Arms_))
        );
    }
};

int main() {
    S s;
    // 0 explicit types:
    auto r0 = s.match_unwrapped(1, 2);
    static_assert(std::same_as<decltype(r0), std::string>);

    // 1 explicit type:
    auto r1 = s.match_unwrapped<int>(1, 2);
    static_assert(std::same_as<decltype(r1), int>);

    // 2 explicit types:
    auto r2 = s.match_unwrapped<int, double>(1, 2);
    static_assert(std::same_as<decltype(r2), Option<int, double>>);

    // 5 explicit types:
    auto r5 = s.match_unwrapped<int, double, char, float, long>(1, 2);
    static_assert(std::same_as<decltype(r5), Option<int, double, char, float, long>>);

    std::cout << "SUCCESS!\n";
    return 0;
}
