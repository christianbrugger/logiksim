#include "ressource.h"

#include "exception.h"
#include "format.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>

namespace logicsim {

auto to_absolute_ressource_path(QString relative) -> QString {
    if (relative.isEmpty()) {
        return QString {};
    }

    const auto app_dir = QApplication::instance()->applicationDirPath();
    return QFileInfo(app_dir + '/' + "ressources" + '/' + relative).absoluteFilePath();
}

auto get_font_path_relative(font_t font) -> QString {
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
    return to_absolute_ressource_path(get_font_path_relative(font));
}

// Browse Icons: https://lucide.dev/icons/
auto get_icon_path_relative(icon_t icon) {
    switch (icon) {
        using enum icon_t;

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
    };

    throw_exception("unknown icon_t");
}

auto get_icon_path(icon_t icon) -> QString {
    return to_absolute_ressource_path(get_icon_path_relative(icon));
}

}  // namespace logicsim