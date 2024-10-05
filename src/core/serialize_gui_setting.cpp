#include "serialize_gui_setting.h"

#include "fmt/core.h"
#include "logging.h"

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

auto GuiSettings::format() const -> std::string {
    return fmt::format(
        "GuiSettings{{\n"
        "  version = {},\n"
        "  \n"
        "  thread_count = {},\n"
        "  wire_render_style = {},\n"
        "  direct_rendering = {},\n"
        "  jit_rendering = {},\n"
        "}}",
        version, thread_count, wire_render_style, direct_rendering, jit_rendering);
}

auto is_valid(const GuiSettings& settings) -> void;

auto serialize_gui_settings(const GuiSettings& settings) -> std::string {
    return glz::ex::write<glz::opts {.prettify = true, .indentation_width = 4}>(settings);
}

auto load_gui_settings(const std::string& text) -> std::optional<GuiSettings> {
    auto version = glz::get_as_json<int, "/version">(text);
    if (!version.has_value()) {
        try {
            print(glz::format_error(version.error(), text));
        } catch (std::runtime_error&) {
            print("error parsing gui settings json");
        }
        return std::nullopt;
    }
    if (version.value() != CURRENT_GUI_SETTING_VERSION) {
        print("Error wrong version. Expected", CURRENT_GUI_SETTING_VERSION, "got",
              version.value());
        return std::nullopt;
    }

    auto result = std::optional<GuiSettings> {GuiSettings {}};
    const auto error = glz::read_json<GuiSettings>(result.value(), text);
    if (error) {
        try {
            print(glz::format_error(error, text));
        } catch (std::runtime_error&) {
            print("error parsing gui settings json");
        }
        return std::nullopt;
    }

    return result;
}

}  // namespace logicsim
