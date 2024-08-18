#ifndef LOGICSIM_ALGORITHM_TO_UNDERLYING_H
#define LOGICSIM_ALGORITHM_TO_UNDERLYING_H

#include <type_traits>

namespace logicsim {

/**
 * @brief: Converts an enumeration to its underlying type.
 *
 * Note this is back-ported from C++23
 */
template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

}  // namespace logicsim

#endif
