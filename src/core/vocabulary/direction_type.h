#ifndef LOGICSIM_VOCABULARY_DIRECTION_TYPE_H
#define LOGICSIM_VOCABULARY_DIRECTION_TYPE_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

/**
 * @brief: Defines if a logic element is directed, undirected or both
 *         regarding orientation.
 */
enum class DirectionType {
    undirected,
    directed,
    any,
};

template <>
[[nodiscard]] auto format(DirectionType type) -> std::string;

}  // namespace logicsim

#endif
