#ifndef LOGICSIM_VOCABULARY_RENDER_MODE_H
#define LOGICSIM_VOCABULARY_RENDER_MODE_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

/**
 * @brief: Type of rendering.
 */
enum class RenderMode {
    /// Rendering is performed directly on the backing store without additional buffering.
    direct,
    /// Rendering is performed to an image buffer that is then copied to the widget.
    buffered,
};

template <>
[[nodiscard]] auto format(RenderMode type) -> std::string;

}  // namespace logicsim

#endif
