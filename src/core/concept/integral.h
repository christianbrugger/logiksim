#ifndef LOGICSIM_CONCEPT_INTEGRAL_H
#define LOGICSIM_CONCEPT_INTEGRAL_H

#include <type_traits>

namespace logicsim {

/**
 * @brief: Matches build-in integer types (bool, char, int, ...).
 *
 * Never matches any class types.
 * Never matches floating point types.
 */
template <typename T>
concept integral = std::is_integral_v<T>;

}  // namespace logicsim

#endif
