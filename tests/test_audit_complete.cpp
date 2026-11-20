#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <cassert>

// Test deduction trait
template<typename Arm_, typename... ForwardAlts_>
struct deduce_arm_res_ {
    using ArmD_ = std::remove_cvref_t<Arm_>;

    template<typename A_>
    static constexpr auto get_arm_res() {
        if constexpr (requires { typename A_::pattern_type; typename A_::handler_type; }) {
            using Pp_ = typename A_::pattern_type;
            using Fp_ = typename A_::handler_type;
            return get_ph_res<Pp_, Fp_>();
        } else {
            return std::type_identity<std::decay_t<A_>>{};
        }
    }

    template<typename Pp_, typename Fp_>
    static constexpr auto get_ph_res() {
        if constexpr (requires { typename Pp_::is_some_tag; }) {
            // Some<T>
            return test_alts<Fp_, ForwardAlts_...>();
        } else {
            return test_alts<Fp_, ForwardAlts_...>();
        }
    }

    template<typename Fp_>
    static constexpr auto test_alts() {
        if constexpr (std::is_invocable_v<Fp_&>) {
            return std::type_identity<std::invoke_result_t<Fp_&>>{};
        } else {
            return std::type_identity<std::decay_t<Fp_>>{};
        }
    }

    template<typename Fp_, typename First_, typename... Rest_>
    static constexpr auto test_alts() {
        if constexpr (std::is_invocable_v<Fp_&, First_>) {
            return std::type_identity<std::invoke_result_t<Fp_&, First_>>{};
        } else if constexpr (sizeof...(Rest_) > 0) {
            return test_alts<Fp_, Rest_...>();
        } else if constexpr (std::is_invocable_v<Fp_&>) {
            return std::type_identity<std::invoke_result_t<Fp_&>>{};
        } else {
            return std::type_identity<std::decay_t<Fp_>>{};
        }
    }

    using type = typename decltype(get_arm_res<ArmD_>())::type;
};

struct Arm1 {
    using pattern_type = int;
    using handler_type = decltype([](const std::string&) { return std::string("matched"); });
};
struct Arm2 {
    using pattern_type = int;
    using handler_type = decltype([] { return std::string("some"); });
};
struct Arm3 {
    using pattern_type = int;
    using handler_type = decltype([] { return std::string("none"); });
};

int main() {
    using R1 = typename deduce_arm_res_<Arm1, int&, const std::string&>::type;
    static_assert(std::same_as<R1, std::string>);

    using R2 = typename deduce_arm_res_<Arm2, int&, const std::string&>::type;
    static_assert(std::same_as<R2, std::string>);

    using R3 = typename deduce_arm_res_<Arm3, int&, const std::string&>::type;
    static_assert(std::same_as<R3, std::string>);

    std::cout << "All arm deduction checks passed!\n";
    return 0;
}
