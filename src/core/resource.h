#ifndef LOGIKSIM_RESOURCE_H
#define LOGIKSIM_RESOURCE_H

#include "core/format/enum.h"
#include "core/vocabulary/font_style.h"

#include <filesystem>

namespace logicsim {

constexpr static auto inline LS_APP_NAME = "LogikSim";
constexpr static auto inline LS_APP_VERSION_STR = "2.2.1";

[[nodiscard]] auto get_font_path(FontStyle style) -> std::filesystem::path;

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
    show_render_borders,
    show_mouse_position,
    non_interactive_mode,
    direct_rendering,
    jit_rendering,

    // options
    options,

    // help
    about,

    // circuit rendering
    setting_handle_clock_generator,
    setting_handle_text_element,
    dialog_text_element,

    // text
    text_alignment_horizontal_left,
    text_alignment_horizontal_center,
    text_alignment_horizontal_right,
    text_style_regular,
    text_style_italic,
    text_style_bold,
    text_style_monospace,
};

template <>
[[nodiscard]] auto format(icon_t icon) -> std::string;

[[nodiscard]] auto get_icon_path(icon_t icon) -> std::filesystem::path;

}  // namespace logicsim

#endif
