#ifndef LOGICSIM_VOCABULARY_RENDER_SETTING_H
#define LOGICSIM_VOCABULARY_RENDER_SETTING_H

#include "format/struct.h"
#include "vocabulary/view_config.h"

#include <string>

namespace logicsim {

struct RenderSettings {
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
