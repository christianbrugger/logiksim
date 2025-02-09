#ifndef LOGICSIM_CORE_ALGORITHM_NUMERIC_H
#define LOGICSIM_CORE_ALGORITHM_NUMERIC_H

#include <flux/core/numeric.hpp>

#include <optional>

namespace logicsim {

inline constexpr auto checked_add = flux::num::checked_add;
inline constexpr auto checked_sub = flux::num::checked_sub;
inline constexpr auto checked_mul = flux::num::checked_mul;
inline constexpr auto checked_div = flux::num::checked_div;
inline constexpr auto checked_mod = flux::num::checked_mod;
inline constexpr auto checked_neg = flux::num::checked_neg;
inline constexpr auto checked_shl = flux::num::checked_shl;
inline constexpr auto checked_shr = flux::num::checked_shr;

//
// try_*
//

struct try_add_fn {
    template <flux::num::integral T>
    [[nodiscard]] FLUX_ALWAYS_INLINE constexpr auto operator()(
        T lhs, T rhs) const noexcept -> std::optional<T> {
        if (const auto result = flux::num::overflowing_add(lhs, rhs);
            !result.overflowed) {
            return result.value;
        }
        return std::nullopt;
    }
};

inline constexpr auto try_add = try_add_fn {};

}  // namespace logicsim

#endif
