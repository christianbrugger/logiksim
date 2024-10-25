#include "core/serialize_detail.h"

#include "core/logging.h"

#include <glaze/glaze.hpp>
#include <glaze/glaze_exceptions.hpp>

namespace logicsim {

namespace {

constexpr static inline auto debug_print_json = false;

}

}  // namespace logicsim

template <>
struct glz::meta<logicsim::LogicItemType> {
    using enum logicsim::LogicItemType;

    static constexpr auto value = enumerate(  //
        "buffer_element", buffer_element,     //
        "and_element", and_element,           //
        "or_element", or_element,             //
        "xor_element", xor_element,           //

        "button", button,                  //
        "led", led,                        //
        "display_number", display_number,  //
        "display_ascii", display_ascii,    //

        "clock_generator", clock_generator,        //
        "flipflop_jk", flipflop_jk,                //
        "shift_register", shift_register,          //
        "latch_d", latch_d,                        //
        "flipflop_d", flipflop_d,                  //
        "flipflop_master_slave_d", flipflop_ms_d,  //

        "sub_circuit", sub_circuit  //
    );
};

template <>
struct glz::meta<logicsim::DecorationType> {
    using enum logicsim::DecorationType;

    static constexpr auto value = enumerate(  //
        "text_element", text_element          //
    );
};

template <>
struct glz::meta<logicsim::orientation_t> {
    using enum logicsim::orientation_t;

    static constexpr auto value = enumerate(  //
        "right", right,                       //
        "left", left,                         //
        "up", up,                             //
        "down", down,                         //
        "undirected", undirected              //
    );
};

template <>
struct glz::meta<logicsim::grid_t> {
    using T = logicsim::grid_t;

    static constexpr auto value = &T::value;
};

template <>
struct glz::meta<logicsim::point_t> {
    using T = logicsim::point_t;

    static constexpr auto value = array(&T::x, &T::y);
};

template <>
struct glz::meta<logicsim::offset_t> {
    using T = logicsim::offset_t;

    static constexpr auto value = &T::value;
};

template <>
struct glz::meta<logicsim::size_2d_t> {
    using T = logicsim::size_2d_t;

    static constexpr auto value = glz::object(  //
        "width", &T::width,                     //
        "height", &T::height                    //
    );
};

template <>
struct glz::meta<logicsim::grid_fine_t> {
    using T = logicsim::grid_fine_t;

    static constexpr auto value = &T::value;
};

using logicsim::serialize::SerializedLine;

template <>
struct glz::meta<SerializedLine> {
    using T = SerializedLine;
    static constexpr auto value = glz::array(&T::p0, &T::p1);
};

using logicsim::serialize::SerializedAttributesClockGenerator;

template <>
struct glz::meta<SerializedAttributesClockGenerator> {
    using T = SerializedAttributesClockGenerator;

    static constexpr auto value = glz::object(  //
        "name", &T::name,                       //

        "time_symmetric_ns", &T::time_symmetric_ns,  //
        "time_on_ns", &T::time_on_ns,                //
        "time_off_ns", &T::time_off_ns,              //

        "is_symmetric", &T::is_symmetric,                         //
        "show_simulation_controls", &T::show_simulation_controls  //
    );
};

using logicsim::serialize::SerializedLogicItem;

template <>
struct glz::meta<SerializedLogicItem> {
    using T = SerializedLogicItem;

    static constexpr auto value = glz::object(  //
        "element_type", &T::logicitem_type,     //
        "input_count", &T::input_count,         //

        "output_count", &T::output_count,          //
        "input_inverters", &T::input_inverters,    //
        "output_inverters", &T::output_inverters,  //

        "position", &T::position,        //
        "orientation", &T::orientation,  //

        "attributes_clock_generator", &T::attributes_clock_generator  //
    );
};

template <>
struct glz::meta<logicsim::HTextAlignment> {
    using enum logicsim::HTextAlignment;

    static constexpr auto value = enumerate(  //
        "left", left,                         //
        "right", right,                       //
        "center", center                      //
    );
};

template <>
struct glz::meta<logicsim::FontStyle> {
    using enum logicsim::FontStyle;

    static constexpr auto value = enumerate(  //
        "regular", regular,                   //
        "italic", italic,                     //
        "bold", bold,                         //
        "monospace", monospace                //
    );
};

using logicsim::serialize::SerializedRgbColor;

template <>
struct glz::meta<SerializedRgbColor> {
    using T = SerializedRgbColor;

    static constexpr auto value = glz::object(  //
        "red", &T::red,                         //
        "green", &T::green,                     //
        "blue", &T::blue                        //
    );
};

using logicsim::serialize::SerializedAttributesTextElement;

template <>
struct glz::meta<SerializedAttributesTextElement> {
    using T = SerializedAttributesTextElement;

    static constexpr auto value = glz::object(             //
        "text", &T::text,                                  //
        "horizontal_alignment", &T::horizontal_alignment,  //
        "font_style", &T::font_style,                      //
        "text_color", &T::text_color                       //
    );
};

using logicsim::serialize::SerializedDecoration;

template <>
struct glz::meta<SerializedDecoration> {
    using T = SerializedDecoration;

    static constexpr auto value = glz::object(   //
        "decoration_type", &T::decoration_type,  //
        "position", &T::position,                //
        "size", &T::size,                        //

        "attributes_text_element", &T::attributes_text_element  //
    );
};

using logicsim::serialize::SerializedViewPoint;

template <>
struct glz::meta<SerializedViewPoint> {
    using T = SerializedViewPoint;

    static constexpr auto value = glz::object(  //
        "device_scale", &T::device_scale,       //
        "grid_offset_x", &T::grid_offset_x,     //
        "grid_offset_y", &T::grid_offset_y      //
    );
};

using logicsim::serialize::SerializedSimulationConfig;

template <>
struct glz::meta<SerializedSimulationConfig> {
    using T = SerializedSimulationConfig;

    static constexpr auto value = glz::object(                   //
        "simulation_time_rate_ns", &T::simulation_time_rate_ns,  //
        "use_wire_delay", &T::use_wire_delay                     //
    );
};

using logicsim::serialize::SerializedLayout;

template <>
struct glz::meta<SerializedLayout> {
    using T = SerializedLayout;

    static constexpr auto value = glz::object(                     //
        "version", &T::version,                                    //
        "minimum_logiksim_version", &T::minimum_logiksim_version,  //
        "save_position", &T::save_position,                        //
        "view_config", &T::view_point,                             //
        "simulation_settings", &T::simulation_config,              //

        "logic_items", &T::logicitems,      //
        "decorations", &T::decorations,     //
        "wire_segments", &T::wire_segments  //

    );
};

namespace logicsim {

auto json_dumps(const serialize::SerializedLayout& data) -> std::string {
    const auto json_text = glz::ex::write_json(data);

    if constexpr (debug_print_json) {
        print(glz::prettify_json(json_text));
    }
    return json_text;
}

auto json_loads(std::string_view text)
    -> tl::expected<serialize::SerializedLayout, LoadError> {
    // read version
    const auto version = glz::get_as_json<int, "/version">(text);
    if (!version.has_value()) {
        return tl::unexpected<LoadError> {
            LoadErrorType::json_parse_error,
            glz::format_error(version.error(), text),
        };
    }

    // handle future versions
    if (version.value() > serialize::CURRENT_VERSION) {
        const auto min_logiksim_version =
            glz::get_as_json<std::string, "/minimum_logiksim_version">(text);

        if (min_logiksim_version) {
            return tl::unexpected<LoadError> {
                LoadErrorType::json_version_error,
                fmt::format("File version is too new. "
                            "Update LogikSim to version '{}' or newer.",
                            min_logiksim_version.value()),
            };
        }
        return tl::unexpected<LoadError> {
            LoadErrorType::json_version_error,
            "File version is too new. To open the file update LogikSim.",

        };
    }

    // parse json
    auto result = tl::expected<SerializedLayout, LoadError> {SerializedLayout {}};
    const auto error = glz::read_json<SerializedLayout>(result.value(), text);
    if (error) {
        return tl::unexpected<LoadError> {
            LoadErrorType::json_parse_error,
            glz::format_error(error, text),
        };
    }

    return result;
}

}  // namespace logicsim
