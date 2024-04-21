#include "resource.h"

#include "executable_path.h"
#include "timer.h"

#include <gsl/gsl>

#include <exception>

namespace logicsim {

namespace {

[[nodiscard]] auto to_absolute_resource_path(const std::filesystem::path &relative)
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

        case direct_rendering: {
            return std::filesystem::path {"icons/lucide/grid-2x2.svg"};
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
    };
    std::terminate();
}

}  // namespace

auto get_icon_path(icon_t icon) -> std::filesystem::path {
    return to_absolute_resource_path(get_icon_path_relative(icon));
}

}  // namespace logicsim