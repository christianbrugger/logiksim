#include "vocabulary/widget_render_config.h"

namespace logicsim {

auto WidgetRenderConfig::format() const -> std::string {
    return fmt::format(
        "RenderConfig{{\n"
        "  do_benchmark = {},\n"
        "  show_circuit = {},\n"
        "  show_collision_cache = {},\n"
        "  show_connection_cache = {},\n"
        "  show_selection_cache = {},\n"
        "  \n"
        "  zoom_level = {},\n"
        "  \n"
        "  thread_count = {},\n"
        "  direct_rendering = {},\n"
        "}}",
        do_benchmark, show_circuit, show_collision_cache, show_connection_cache,
        show_selection_cache, zoom_level, thread_count, direct_rendering);
}

}  // namespace logicsim
