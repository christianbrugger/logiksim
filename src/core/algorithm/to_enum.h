#ifndef LOGICSIM_CORE_ALGORITHM_TO_ENUM_H
#define LOGICSIM_CORE_ALGORITHM_TO_ENUM_H

#include "core/concept/enum.h"
#include "core/concept/integral.h"

#include <gsl/gsl>

#include <type_traits>

namespace logicsim {

/**
 * @brief: Converts integral value to enum type safely.
 */
template <enum_type Enum>
constexpr auto to_enum(integral auto value) noexcept -> Enum {
    const auto value_rep = gsl::narrow<std::underlying_type_t<Enum>>(value);
    return static_cast<Enum>(value_rep);
}

}  // namespace logicsim

#endif
