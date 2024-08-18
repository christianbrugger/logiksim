#ifndef LOGICSIM_VOCABULARY_ORIENTATION_H
#define LOGICSIM_VOCABULARY_ORIENTATION_H

#include "format/enum.h"

#include <array>
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
[[nodiscard]] auto format(orientation_t state) -> std::string;

constexpr inline auto all_orientations = std::array {
    orientation_t::right,  //
    orientation_t::left,   //
    orientation_t::up,     //
    orientation_t::down,   //

    orientation_t::undirected,  //
};

[[nodiscard]] auto is_directed(orientation_t orientation) -> bool;
[[nodiscard]] auto is_undirected(orientation_t orientation) -> bool;

}  // namespace logicsim

#endif
