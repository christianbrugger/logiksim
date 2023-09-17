#ifndef LOGIKSIM_RESOURCE_H
#define LOGIKSIM_RESOURCE_H

#include <QString>

namespace logicsim {

constexpr static auto inline LS_APP_NAME = "LogikSim";
constexpr static auto inline LS_APP_VERSION_STR = "2.1.0";

enum class font_t : uint8_t {
    regular,
    italic,
    bold,
    monospace,
};

[[nodiscard]] auto get_font_path(font_t font) -> QString;

enum class setting_t {
    gui_geometry,
    gui_state,

    logfile,
};

[[nodiscard]] auto get_writable_setting_path(setting_t config) -> QString;

enum class icon_t {
    app_icon,

    // file
    new_file,
    open_file,
    save_file,
    exit,

    // edit
    cut,
    copy,
    paste,
    delete_selected,
    select_all,

    // view
    reset_zoom,
    zoom_in,
    zoom_out,

    // simulation
    simulation_start,
    simulation_stop,
    simulation_speed,

    // debug
    benchmark,
    show_circuit,
    show_collision_cache,
    show_connection_cache,
    show_selection_cache,
    // --
    reload_circuit,
    load_simple_example,
    load_wire_example,
    load_element_example,
    load_elements_and_wires_example,
    // --
    direct_rendering,

    // options
    options,

    // help
    about,

    // circuit rendering
    setting_handle_clock_generator,
};

[[nodiscard]] auto get_icon_path(icon_t icon) -> QString;

}  // namespace logicsim

#endif
