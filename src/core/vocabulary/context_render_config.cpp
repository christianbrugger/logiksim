#include "vocabulary/context_render_config.h"

namespace logicsim {

auto ContextRenderSettings::format() const -> std::string {
    return fmt::format(
        "ContextRenderSettings{{\n"
        "  view_config = {},\n"
        "  background_grid_min_distance_device = {},\n"
        "  thread_count = {},\n"
        "}}",
        view_config, background_grid_min_distance_device, thread_count);
}

auto create_context_render_settings(BLSizeI size_px) -> ContextRenderSettings {
    return ContextRenderSettings {
        .view_config = ViewConfig {size_px},
    };
}

}  // namespace logicsim
