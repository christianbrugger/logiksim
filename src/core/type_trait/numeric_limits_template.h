#ifndef LOGICSIM_TYPE_TRAIT_NUMERIC_LIMITS_TEMPLATE_H
#define LOGICSIM_TYPE_TRAIT_NUMERIC_LIMITS_TEMPLATE_H

#include <limits>
#include <type_traits>

namespace logicsim {

template <typename T>
concept has_static_member_min = requires(T obj) { T::min(); };
template <typename T>
concept has_static_member_max = requires(T obj) { T::min(); };

template <typename T>
    requires std::is_integral_v<typename T::value_type>
class numeric_limits_template {
    using value_limits = std::numeric_limits<typename T::value_type>;

   public:
    static constexpr bool is_specialized = true;

    static constexpr auto min() noexcept -> T {
        if constexpr (has_static_member_min<T>) {
            static_assert(std::is_same_v<decltype(T::min()), T>);
            return T::min();
        } else {
            return T {value_limits::min()};
        }
    }

    static constexpr auto max() noexcept -> T {
        if constexpr (has_static_member_max<T>) {
            static_assert(std::is_same_v<decltype(T::max()), T>);
            return T::max();
        } else {
            return T {value_limits::max()};
        }
    }

    static constexpr auto lowest() noexcept -> T {
        static_assert(value_limits::is_integer);
        return max();
    }

    // can be over estimated
    static constexpr int digits = value_limits::digits;
    static constexpr int digits10 = value_limits::digits10;
    static constexpr int max_digits10 = value_limits::max_digits10;

    static constexpr bool is_signed = value_limits::is_signed;
    static constexpr bool is_integer = value_limits::is_integer;
    static constexpr bool is_exact = value_limits::is_exact;
    static constexpr int radix = value_limits::radix;

    // we don't define floating point limits
    static_assert(value_limits::is_integer);
};

}  // namespace logicsim

#endif
