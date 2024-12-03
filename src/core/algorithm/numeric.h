#ifndef LOGICSIM_CORE_ALGORITHM_NUMERIC_H
#define LOGICSIM_CORE_ALGORITHM_NUMERIC_H

#include <flux/core/numeric.hpp>

namespace logicsim {

inline constexpr auto checked_add = flux::num::checked_add;
inline constexpr auto checked_sub = flux::num::checked_sub;
inline constexpr auto checked_mul = flux::num::checked_mul;
inline constexpr auto checked_div = flux::num::checked_div;
inline constexpr auto checked_mod = flux::num::checked_mod;
inline constexpr auto checked_neg = flux::num::checked_neg;
inline constexpr auto checked_shl = flux::num::checked_shl;
inline constexpr auto checked_shr = flux::num::checked_shr;

}  // namespace logicsim

#endif
