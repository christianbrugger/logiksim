#ifndef LOGICSIM_VOCABULARY_CONTEXT_RENDER_CONFIG_H
#define LOGICSIM_VOCABULARY_CONTEXT_RENDER_CONFIG_H

#include "format/struct.h"
#include "vocabulary/view_config.h"

#include <string>

namespace logicsim {

struct ContextRenderConfig {
    ViewConfig view_config {};

    /**
     * @brief: background features are not drawn if they are closer than
     *         this in device coordinates.
     */
    int background_grid_min_distance_device {10};  // device pixels

    /**
     * @brief: Number of render threads used.
     */
    int thread_count {4};

    auto format() const -> std::string;
};

}  // namespace logicsim

#endif
