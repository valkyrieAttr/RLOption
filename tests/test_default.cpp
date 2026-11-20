
#include <concepts>
#include <type_traits>
#include <utility>
#include <memory>

struct DefaultTag {};

template<typename Tp_>
struct Myrl {
    union {
        Tp_ val;
    };
    bool engaged;

    constexpr Myrl(DefaultTag) : engaged(true) {
        std::construct_at(std::addressof(val));
    }

    constexpr ~Myrl() {
        if (engaged) std::destroy_at(std::addressof(val));
    }

    template<typename... Args>
    constexpr Tp_& emplace(Args&&... args) {
        if (engaged) {
            std::destroy_at(std::addressof(val));
            engaged = false;
        }
        std::construct_at(std::addressof(val), std::forward<Args>(args)...);
        engaged = true;
        return val;
    }

    constexpr auto operator=(DefaultTag) noexcept(
        std::is_nothrow_default_constructible_v<Tp_> &&
        ( std::is_move_assignable_v<Tp_> ? std::is_nothrow_move_assignable_v<Tp_> :
          std::is_copy_assignable_v<Tp_> ? std::is_nothrow_copy_assignable_v<Tp_> : true )
    ) -> Myrl& requires ( std::is_default_constructible_v<Tp_> ) {
        if (engaged) {
            if constexpr ( std::is_move_assignable_v<Tp_> ) {
                val = Tp_{};
                return *this;
            } else if constexpr ( std::is_copy_assignable_v<Tp_> ) {
                const Tp_ def_{};
                val = def_;
                return *this;
            }
        }
        this->emplace();
        return *this;
    }
};

struct Immovable {
    Immovable() = default;
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
};

struct CopyOnly {
    CopyOnly() = default;
    CopyOnly(const CopyOnly&) = default;
    CopyOnly(CopyOnly&&) = delete;
    CopyOnly& operator=(const CopyOnly&) = default;
    CopyOnly& operator=(CopyOnly&&) = delete;
};

struct ThrowMove {
    ThrowMove() = default;
    ThrowMove(const ThrowMove&) = default;
    ThrowMove(ThrowMove&&) = default;
    ThrowMove& operator=(const ThrowMove&) noexcept { return *this; }
    ThrowMove& operator=(ThrowMove&&) noexcept(false) { return *this; }
};

int main() {
    Myrl<Immovable> imm(DefaultTag{});
    imm = DefaultTag{};

    Myrl<CopyOnly> co(DefaultTag{});
    co = DefaultTag{};

    Myrl<ThrowMove> tm(DefaultTag{});
    static_assert(!noexcept(tm = DefaultTag{}));

    return 0;
}
