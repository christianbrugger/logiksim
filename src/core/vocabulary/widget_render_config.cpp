#include "vocabulary/widget_render_config.h"

namespace logicsim {

auto WidgetRenderConfig::format() const -> std::string {
    return fmt::format(
        "RenderConfig{{\n"
        "  thread_count = {},\n"
        "  wire_render_style = {},\n"
        "  \n"
        "  do_benchmark = {},\n"
        "  show_circuit = {},\n"
        "  show_collision_cache = {},\n"
        "  show_connection_cache = {},\n"
        "  show_selection_cache = {},\n"
        "  \n"
        "  show_render_borders = {},\n"
        "  show_mouse_position = {},\n"
        "  direct_rendering = {},\n"
        "  jit_rendering = {},\n"
        "}}",
        thread_count, wire_render_style,  //
        do_benchmark, show_circuit, show_collision_cache, show_connection_cache,
        show_selection_cache,  //
        show_render_borders, show_mouse_position, direct_rendering, jit_rendering);
}

}  // namespace logicsim
