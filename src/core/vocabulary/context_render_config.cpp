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

}  // namespace logicsim
