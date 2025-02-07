#ifndef LOGICSIM_CORE_CONCEPT_ENUM_H
#define LOGICSIM_CORE_CONCEPT_ENUM_H

#include <type_traits>

namespace logicsim {

/**
 * @brief: Matches enumeration types.
 */
template <typename T>
concept enum_type = std::is_enum_v<T>;

}  // namespace logicsim

#endif
