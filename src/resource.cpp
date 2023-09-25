#include "resource.h"

#include "exception.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace logicsim {

[[nodiscard]] auto to_absolute_resource_path(QString relative) -> QString {
    if (relative.isEmpty()) {
        return QString {};
    }

    const auto app_dir = QApplication::instance()->applicationDirPath();
    return QFileInfo(app_dir + '/' + "resources" + '/' + relative).absoluteFilePath();
}

[[nodiscard]] auto writable_standard_path(QStandardPaths::StandardLocation location,
                                          QString relative) -> QString {
    const auto folder = QStandardPaths::writableLocation(location);
    const auto file = QFileInfo(folder + '/' + LS_APP_VERSION_STR + '/' + relative);
    file.absoluteDir().mkpath(".");
    return file.absoluteFilePath();
}

[[nodiscard]] auto get_font_path_relative(font_t font) -> QString {
    switch (font) {
        using enum font_t;

        case regular: {
            return QString("fonts/NotoSans-Regular.ttf");
        }
        case italic: {
            return QString("fonts/NotoSans-Italic.ttf");
        }
        case bold: {
            return QString("fonts/NotoSans-Bold.ttf");
        }
        case monospace: {
            return QString("fonts/NotoSansMono-Regular.ttf");
        }
    };

    throw_exception("unknown font_t");
}

auto get_font_path(font_t font) -> QString {
    return to_absolute_resource_path(get_font_path_relative(font));
}

auto get_writable_setting_path(setting_t config) -> QString {
    switch (config) {
        using enum setting_t;

        case gui_geometry: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "gui_geometry.bin");
        }
        case gui_state: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "gui_state.bin");
        }
        case logfile: {
            return writable_standard_path(QStandardPaths::AppConfigLocation,
                                          "logging.txt");
        }
    };

    throw_exception("unknown font_t");
}

// Browse Icons:
//
// https://lucide.dev/icons/
//
// https://jam-icons.com/
//
[[nodiscard]] auto get_icon_path_relative(icon_t icon) -> QString {
    switch (icon) {
        using enum icon_t;

        case app_icon: {
            return QString("icons/own/cpu.svg");
        }

        case new_file: {
            return QString("icons/lucide/file.svg");
        }
        case open_file: {
            return QString("icons/lucide/folder-open.svg");
        }
        case save_file: {
            return QString("icons/lucide/save.svg");
        }
        case exit: {
            return QString("icons/lucide/log-out.svg");
        }

        case cut: {
            return QString("icons/lucide/scissors.svg");
        }
        case copy: {
            return QString("icons/lucide/copy.svg");
        }
        case paste: {
            return QString("icons/lucide/clipboard.svg");
        }
        case delete_selected: {
            return QString("icons/lucide/trash-2.svg");
        }
        case select_all: {
            // maximize, grid, check-square, box-select
            return QString("icons/lucide/box-select.svg");
        }

        case reset_zoom: {
            return QString("icons/lucide/rotate-ccw.svg");
        }
        case zoom_in: {
            return QString("icons/lucide/zoom-in.svg");
        }
        case zoom_out: {
            return QString("icons/lucide/zoom-out.svg");
        }

        case simulation_start: {
            return QString("icons/own/play.svg");
        }
        case simulation_stop: {
            return QString("icons/own/stop_15x15_r0_r.svg");
        }
        case simulation_speed: {
            return QString("icons/lucide/gauge.svg");
        }

        case benchmark: {
            return QString("icons/lucide/infinity.svg");
        }
        case show_circuit: {
            return QString("icons/lucide/cpu.svg");
        }
        case show_collision_cache: {
            return QString("icons/lucide/shapes.svg");
        }
        case show_connection_cache: {
            // share-2
            return QString("icons/lucide/spline.svg");
        }
        case show_selection_cache: {
            // ungroup, group, boxes, ratio
            return QString("icons/lucide/ungroup.svg");
        }

        case reload_circuit: {
            return QString("icons/lucide/refresh-ccw.svg");
        }
        case load_simple_example: {
            return QString("icons/lucide/cable.svg");
        }
        case load_wire_example: {
            return QString("icons/lucide/share-2.svg");
        }
        case load_element_example: {
            return QString("icons/lucide/workflow.svg");
        }
        case load_elements_and_wires_example: {
            return QString("icons/lucide/network.svg");
        }

        case direct_rendering: {
            return QString("icons/lucide/grid-2x2.svg");
        }

        case options: {
            return QString("icons/lucide/settings.svg");
        }

        case about: {
            return QString("icons/lucide/info.svg");
        }

        case setting_handle_clock_generator: {
            return QString("icons/lucide/activity.svg");
        }
    };

    throw_exception("unknown icon_t");
}

auto get_icon_path(icon_t icon) -> QString {
    return to_absolute_resource_path(get_icon_path_relative(icon));
}

}  // namespace logicsim