#ifndef LOGICSIM_CORE_VOCABULARY_WIRE_RENDER_STYLE_H
#define LOGICSIM_CORE_VOCABULARY_WIRE_RENDER_STYLE_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

/**
 * @brief: Style in wich to draw enabled wires, cross points and connectors
 *         during simulation.
 */
enum class WireRenderStyle : uint8_t {
    red,
    bold,
    bold_red,
};

template <>
[[nodiscard]] auto format(WireRenderStyle style) -> std::string;

}  // namespace logicsim

#endif
