#include "core/resource.h"

#include "core/executable_path.h"
#include "core/timer.h"

#include <gsl/gsl>

#include <exception>

namespace logicsim {

template <>
auto format(icon_t icon) -> std::string {
    switch (icon) {
        using enum icon_t;
        case app_icon: {
            return "app_icon";
        }

        // file
        case new_file: {
            return "new_file";
        }
        case open_file: {
            return "open_file";
        }
        case save_file: {
            return "save_file";
        }
        case exit: {
            return "exit";
        }

        // edit
        case cut: {
            return "cut";
        }
        case copy: {
            return "copy";
        }
        case paste: {
            return "paste";
        }
        case delete_selected: {
            return "delete_selected";
        }
        case select_all: {
            return "select_all";
        }

        // view
        case reset_zoom: {
            return "reset_zoom";
        }
        case zoom_in: {
            return "zoom_in";
        }
        case zoom_out: {
            return "zoom_out";
        }

        // simulation
        case simulation_start: {
            return "simulation_start";
        }
        case simulation_stop: {
            return "simulation_stop";
        }
        case simulation_speed: {
            return "simulation_speed";
        }

        // debug
        case benchmark: {
            return "benchmark";
        }
        case show_circuit: {
            return "show_circuit";
        }
        case show_collision_cache: {
            return "show_collision_cache";
        }
        case show_connection_cache: {
            return "show_connection_cache";
        }
        case show_selection_cache: {
            return "show_selection_cache";
        }
        // --
        case reload_circuit: {
            return "reload_circuit";
        }
        case load_simple_example: {
            return "load_simple_example";
        }
        case load_wire_example: {
            return "load_wire_example";
        }
        case load_element_example: {
            return "load_element_example";
        }
        case load_elements_and_wires_example: {
            return "load_elements_and_wires_example";
        }
        // --
        case show_render_borders: {
            return "show_render_borders";
        }
        case show_mouse_position: {
            return "show_mouse_position";
        }
        case non_interactive_mode: {
            return "non_interactive_mode";
        }
        case direct_rendering: {
            return "direct_rendering";
        }
        case jit_rendering: {
            return "jit_rendering";
        }

        // options
        case options: {
            return "options";
        }

        // help
        case about: {
            return "about";
        }

        // circuit rendering
        case setting_handle_clock_generator: {
            return "setting_handle_clock_generator";
        }
        case setting_handle_text_element: {
            return "setting_handle_text_element";
        }

        // text alignment
        case text_alignment_horizontal_left: {
            return "text_alignment_horizontal_left";
        }
        case text_alignment_horizontal_center: {
            return "text_alignment_horizontal_center";
        }
        case text_alignment_horizontal_right: {
            return "text_alignment_horizontal_right";
        }
    }
    std::terminate();
}

namespace {

[[nodiscard]] auto to_absolute_resource_path(const std::filesystem::path& relative)
    -> std::filesystem::path {
    Expects(!relative.empty());

    const auto directory = get_executable_directory();
    return std::filesystem::weakly_canonical(directory / "resources" / relative);
}

[[nodiscard]] auto get_font_path_relative(FontStyle style) -> std::filesystem::path {
    switch (style) {
        using enum FontStyle;

        case regular: {
            return std::filesystem::path {"fonts/NotoSans-Regular.ttf"};
        }
        case italic: {
            return std::filesystem::path {"fonts/NotoSans-Italic.ttf"};
        }
        case bold: {
            return std::filesystem::path {"fonts/NotoSans-Bold.ttf"};
        }
        case monospace: {
            return std::filesystem::path {"fonts/NotoSansMono-Regular.ttf"};
        }
    };
    std::terminate();
}

}  // namespace

auto get_font_path(FontStyle style) -> std::filesystem::path {
    return to_absolute_resource_path(get_font_path_relative(style));
}

namespace {

// Browse Icons:
//
// https://lucide.dev/icons/
//
// https://jam-icons.com/
//
[[nodiscard]] auto get_icon_path_relative(icon_t icon) -> std::filesystem::path {
    switch (icon) {
        using enum icon_t;

        case app_icon: {
            return std::filesystem::path {"icons/own/cpu.svg"};
        }

        case new_file: {
            return std::filesystem::path {"icons/lucide/file.svg"};
        }
        case open_file: {
            return std::filesystem::path {"icons/lucide/folder-open.svg"};
        }
        case save_file: {
            return std::filesystem::path {"icons/lucide/save.svg"};
        }
        case exit: {
            return std::filesystem::path {"icons/lucide/log-out.svg"};
        }

        case cut: {
            return std::filesystem::path {"icons/lucide/scissors.svg"};
        }
        case copy: {
            return std::filesystem::path {"icons/lucide/copy.svg"};
        }
        case paste: {
            return std::filesystem::path {"icons/lucide/clipboard.svg"};
        }
        case delete_selected: {
            return std::filesystem::path {"icons/lucide/trash-2.svg"};
        }
        case select_all: {
            // maximize, grid, check-square, box-select
            return std::filesystem::path {"icons/lucide/box-select.svg"};
        }

        case reset_zoom: {
            return std::filesystem::path {"icons/lucide/rotate-ccw.svg"};
        }
        case zoom_in: {
            return std::filesystem::path {"icons/lucide/zoom-in.svg"};
        }
        case zoom_out: {
            return std::filesystem::path {"icons/lucide/zoom-out.svg"};
        }

        case simulation_start: {
            return std::filesystem::path {"icons/own/play.svg"};
        }
        case simulation_stop: {
            return std::filesystem::path {"icons/own/stop_15x15_r0_r.svg"};
        }
        case simulation_speed: {
            return std::filesystem::path {"icons/lucide/gauge.svg"};
        }

        case benchmark: {
            return std::filesystem::path {"icons/lucide/infinity.svg"};
        }
        case show_circuit: {
            return std::filesystem::path {"icons/lucide/cpu.svg"};
        }
        case show_collision_cache: {
            return std::filesystem::path {"icons/lucide/shapes.svg"};
        }
        case show_connection_cache: {
            // share-2
            return std::filesystem::path {"icons/lucide/spline.svg"};
        }
        case show_selection_cache: {
            // ungroup, group, boxes, ratio
            return std::filesystem::path {"icons/lucide/ungroup.svg"};
        }

        case reload_circuit: {
            return std::filesystem::path {"icons/lucide/refresh-ccw.svg"};
        }
        case load_simple_example: {
            return std::filesystem::path {"icons/lucide/cable.svg"};
        }
        case load_wire_example: {
            return std::filesystem::path {"icons/lucide/share-2.svg"};
        }
        case load_element_example: {
            return std::filesystem::path {"icons/lucide/workflow.svg"};
        }
        case load_elements_and_wires_example: {
            return std::filesystem::path {"icons/lucide/network.svg"};
        }

        case show_render_borders: {
            return std::filesystem::path {"icons/lucide/scan.svg"};
        }
        case show_mouse_position: {
            return std::filesystem::path {"icons/lucide/mouse-pointer-click.svg"};
        }
        case non_interactive_mode: {
            return std::filesystem::path {"icons/lucide/circle-slash.svg"};
        }
        case direct_rendering: {
            return std::filesystem::path {"icons/lucide/grid-2x2.svg"};
        }
        case jit_rendering: {
            return std::filesystem::path {"icons/lucide/circle-gauge.svg"};
        }

        case options: {
            return std::filesystem::path {"icons/lucide/settings.svg"};
        }

        case about: {
            return std::filesystem::path {"icons/lucide/info.svg"};
        }

        case setting_handle_clock_generator: {
            return std::filesystem::path {"icons/lucide/activity.svg"};
        }
        case setting_handle_text_element: {
            // text-cursor-input, chevrons-left-right-ellipsis,
            // rectangle-ellipsis, ellipsis
            return std::filesystem::path {"icons/lucide/ellipsis.svg"};
        }

        case text_alignment_horizontal_left: {
            return std::filesystem::path {"icons/lucide/align-left.svg"};
        }
        case text_alignment_horizontal_center: {
            return std::filesystem::path {"icons/lucide/align-center.svg"};
        }
        case text_alignment_horizontal_right: {
            return std::filesystem::path {"icons/lucide/align-right.svg"};
        }
    };
    std::terminate();
}

}  // namespace

auto get_icon_path(icon_t icon) -> std::filesystem::path {
    return to_absolute_resource_path(get_icon_path_relative(icon));
}

}  // namespace logicsim
