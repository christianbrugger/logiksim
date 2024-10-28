#include "core/serialize_gui_setting.h"

#include "core/logging.h"

#include <fmt/core.h>
#include <glaze/exceptions/json_exceptions.hpp>
#include <glaze/glaze.hpp>
#include <glaze/glaze_exceptions.hpp>

template <>
struct glz::meta<logicsim::WireRenderStyle> {
    using enum logicsim::WireRenderStyle;

    static constexpr auto value = enumerate(  //
        "red", red,                           //
        "bold", bold,                         //
        "bold_red", bold_red                  //
    );
};

template <>
struct glz::meta<logicsim::ThreadCount> {
    using enum logicsim::ThreadCount;

    static constexpr auto value = enumerate(  //
        "synchronous", synchronous,           //
        "two", two,                           //
        "four", four,                         //
        "eight", eight                        //
    );
};

namespace logicsim {

auto GuiDebugSettings::format() const -> std::string {
    return fmt::format(
        "GuiSettings{{\n"
        "  show_debug_menu = {},\n"
        "  show_render_frames_per_second = {},\n"
        "  show_simulation_events_per_second = {},\n"
        "}}",
        show_debug_menu, show_render_frames_per_second,
        show_simulation_events_per_second);
}

auto GuiSettings::format() const -> std::string {
    return fmt::format(
        "GuiSettings{{\n"
        "  version = {},\n"
        "  \n"
        "  thread_count = {},\n"
        "  wire_render_style = {},\n"
        "  direct_rendering = {},\n"
        "  jit_rendering = {},\n"
        "  \n"
        "  debug = {},\n"
        "}}",
        version, thread_count, wire_render_style, direct_rendering, jit_rendering, debug);
}

auto is_valid(const GuiSettings& settings) -> void;

auto serialize_gui_settings(const GuiSettings& settings) -> std::string {
    return glz::ex::write<glz::opts {.prettify = 1, .indentation_width = 4}>(settings);
}

auto load_gui_settings(const std::string& text) -> tl::expected<GuiSettings, LoadError> {
    // read version
    const auto version = glz::get_as_json<int, "/version">(text);
    if (!version.has_value()) {
        return tl::unexpected<LoadError> {
            LoadErrorType::json_parse_error,
            glz::format_error(version.error(), text),
        };
    }

    // check version
    if (version.value() > CURRENT_GUI_SETTING_VERSION) {
        return tl::unexpected<LoadError> {
            LoadErrorType::json_version_error,
            "GUI Setting version is too new.",
        };
    }

    // parse json
    auto result = tl::expected<GuiSettings, LoadError> {GuiSettings {}};
    const auto error = glz::read_json<GuiSettings>(result.value(), text);
    if (error) {
        return tl::unexpected<LoadError> {
            LoadErrorType::json_parse_error,
            glz::format_error(error, text),
        };
    }

    return result;
}

}  // namespace logicsim
