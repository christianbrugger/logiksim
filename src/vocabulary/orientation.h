#ifndef LOGICSIM_VOCABULARY_ORIENTATION_H
#define LOGICSIM_VOCABULARY_ORIENTATION_H

#include "format/enum.h"

#include <cstdint>

namespace logicsim {

/**
 * @brief: Orientation of a logic item.
 */
enum class orientation_t : uint8_t {
    right,
    left,
    up,
    down,

    undirected,
};

template <>
auto format(orientation_t state) -> std::string;

}  // namespace logicsim

#endif
