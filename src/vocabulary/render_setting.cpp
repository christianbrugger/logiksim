#include "vocabulary/render_setting.h"

namespace logicsim {

auto RenderSettings::format() const -> std::string {
    return fmt::format(
        "RenderSettings(\n"
        "  view_config = {},\n"
        "  background_grid_min_distance_device = {},\n"
        "  thread_count = {})",
        view_config, background_grid_min_distance_device, thread_count);
}

}  // namespace logicsim
