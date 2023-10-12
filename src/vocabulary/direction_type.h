#ifndef LOGICSIM_VOCABULARY_DIRECTION_TYPE_H
#define LOGICSIM_VOCABULARY_DIRECTION_TYPE_H

#include "format/enum.h"

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
auto format(DirectionType type) -> std::string;

}  // namespace logicsim

#endif
