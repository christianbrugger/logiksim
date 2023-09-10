#ifndef LOGIKSIM_RESSOURCE_H
#define LOGIKSIM_RESSOURCE_H

#include <QString>

namespace logicsim {

enum class font_t : uint8_t {
    regular,
    italic,
    bold,
    monospace,
};

auto get_font_path(font_t font) -> QString;

enum class icon_t {
    new_file,
    open_file,
    save_file,
    exit,

    cut,
    copy,
    paste,
    delete_selected,
    select_all,

    reset_zoom,
    zoom_in,
    zoom_out,

    benchmark,
    show_circuit,
    show_collision_cache,
    show_connection_cache,
    show_selection_cache,

    reload_circuit,
    load_simple_example,
    load_wire_example,
    load_element_example,
    load_elements_and_wires_example,

    direct_rendering,

    options,
};

auto get_icon_path(icon_t icon) -> QString;

}  // namespace logicsim

#endif
