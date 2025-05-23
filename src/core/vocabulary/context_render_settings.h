#ifndef LOGICSIM_VOCABULARY_CONTEXT_RENDER_SETTINGS_H
#define LOGICSIM_VOCABULARY_CONTEXT_RENDER_SETTINGS_H

#include "core/format/struct.h"
#include "core/vocabulary/thread_count.h"
#include "core/vocabulary/view_config.h"
#include "core/vocabulary/wire_render_style.h"

#include <string>

namespace logicsim {

struct ContextRenderSettings {
    ViewConfig view_config {};

    /**
     * @brief: background features are not drawn if they are closer than
     *         this in device coordinates.
     */
    int background_grid_min_distance_device {10};  // device pixels

    /**
     * @brief: Number of render threads used.
     */
    ThreadCount thread_count {ThreadCount::four};

    /**
     * @brief: Style in wich to draw enabled wires, cross points and connectors
     *         during simulation.
     */
    WireRenderStyle wire_render_style {WireRenderStyle::red};

    /**
     * @brief: If true the JIT is used for Blend2D, if available.
     */
    bool jit_rendering {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const ContextRenderSettings &) const -> bool = default;
};

[[nodiscard]] auto create_context_render_settings(BLSizeI size_px)
    -> ContextRenderSettings;

}  // namespace logicsim

#endif
