#include "element/logicitem/render_logicitem_base.h"

#include "layout.h"
#include "layout_info.h"
#include "element/logicitem/layout_display.h"         // TODO remove
#include "element/logicitem/layout_display_ascii.h"   // TODO remove
#include "element/logicitem/layout_display_number.h"  // TODO remove
#include "render/circuit/alpha_values.h"
#include "render/circuit/render_connector_label.h"
#include "render/circuit/render_logicitem_base_generic.h"
#include "render/context.h"
#include "render/primitive/circle.h"
#include "spatial_simulation.h"
#include "vocabulary/drawable_element.h"
#include "vocabulary/element_draw_state.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/internal_state.h"
#include "vocabulary/rect_fine.h"

namespace logicsim {

namespace defaults {

// TODO sort defaults
constexpr static inline auto button_body_color = defaults::color_gray_90;
constexpr static inline auto led_radius = grid_fine_t {0.45};

namespace font {

constexpr static inline auto buffer_label_size = grid_fine_t {0.6};

constexpr static inline auto clock_name_size = grid_fine_t {0.7};
constexpr static inline auto clock_name_color = defaults::color_black;
constexpr static inline auto clock_name_style = FontStyle::bold;
constexpr static inline auto clock_period_size = grid_fine_t {0.7};
constexpr static inline auto clock_period_color = defaults::color_black;
constexpr static inline auto clock_period_style = FontStyle::regular;

constexpr static inline auto display_ascii_controll_color = defaults::color_dark_orange;
constexpr static inline auto display_normal_color = defaults::color_black;
constexpr static inline auto display_font_style = display::font_style;
constexpr static inline auto display_font_size = display::font_size;
constexpr static inline auto display_ascii_control_size = grid_fine_t {0.7};

}  // namespace font

constexpr static inline auto led_color_disabled = defaults::color_light_gray;
constexpr static inline auto led_color_enabled = defaults::color_red;

}  // namespace defaults

constexpr auto standard_element_label(LogicItemType element_type) -> std::string_view {
    switch (element_type) {
        using enum LogicItemType;

        case and_element:
            return "&";
        case or_element:
            return ">1";
        case xor_element:
            return "=1";

        case sub_circuit:
            return "C";

        default:
            throw std::runtime_error("element type has no standard label");
    }
}

auto draw_standard_element(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);
    const auto type = layout.logic_items().type(logicitem_id);
    draw_logic_item_label(ctx, layout, logicitem_id, standard_element_label(type), state);
}

auto draw_standard_element(Context& ctx, const SpatialSimulation& spatial_simulation,
                           logicitem_id_t logicitem_id) -> void {
    draw_standard_element(ctx, spatial_simulation.layout(), logicitem_id,
                          ElementDrawState::normal);
}

auto draw_button(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                 ElementDrawState state, bool logic_value = false) -> void {
    const auto center = get_logic_item_center(layout, logicitem_id);

    draw_logic_item_rect(ctx, layout, logicitem_id, state,
                         {.custom_fill_color = defaults::button_body_color});
    draw_binary_value(ctx, center, logic_value, state);
}

auto draw_button(Context& ctx, const SpatialSimulation& spatial_simulation,
                 logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto is_enabled = spatial_simulation.simulation().internal_state(
        internal_state_t {element_id, internal_state_index_t {0}});

    draw_button(ctx, spatial_simulation.layout(), logicitem_id, ElementDrawState::normal,
                is_enabled);
}

auto draw_led(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
              ElementDrawState state, bool logic_value = false) -> void {
    const auto base_color =
        logic_value ? defaults::led_color_enabled : defaults::led_color_disabled;

    const auto position = layout.logic_items().position(logicitem_id);

    draw_circle(ctx, point_fine_t {position}, grid_fine_t {defaults::led_radius},
                CircleAttributes {
                    .fill_color = with_alpha_runtime(base_color, state),
                    .stroke_color = get_logic_item_stroke_color(state),
                });
}

auto draw_led(Context& ctx, const SpatialSimulation& spatial_simulation,
              logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto is_enabled = spatial_simulation.simulation().input_value(
        input_t {element_id, connection_id_t {0}});

    draw_led(ctx, spatial_simulation.layout(), logicitem_id, ElementDrawState::normal,
             is_enabled);
}

constexpr static auto power_of_two_labels = string_array<64> {
    "2⁰",  "2¹",  "2²",  "2³",  "2⁴",  "2⁵",  "2⁶",  "2⁷",  "2⁸",  "2⁹",   //
    "2¹⁰", "2¹¹", "2¹²", "2¹³", "2¹⁴", "2¹⁵", "2¹⁶", "2¹⁷", "2¹⁸", "2¹⁹",  //
    "2²⁰", "2²¹", "2²²", "2²³", "2²⁴", "2²⁵", "2²⁶", "2²⁷", "2²⁸", "2²⁹",  //
    "2³⁰", "2³¹", "2³²", "2³³", "2³⁴", "2³⁵", "2³⁶", "2³⁷", "2³⁸", "2³⁹",  //
    "2⁴⁰", "2⁴¹", "2⁴²", "2⁴³", "2⁴⁴", "2⁴⁵", "2⁴⁶", "2⁴⁷", "2⁴⁸", "2⁴⁹",  //
    "2⁵⁰", "2⁵¹", "2⁵²", "2⁵³", "2⁵⁴", "2⁵⁵", "2⁵⁶", "2⁵⁷", "2⁵⁸", "2⁵⁹",  //
    "2⁶⁰", "2⁶¹", "2⁶²", "2⁶³"                                             //
};

namespace {

static_assert(display_number::max_value_inputs <=
              connection_count_t {power_of_two_labels.size()});
static_assert(display_ascii::value_inputs <=
              connection_count_t {power_of_two_labels.size()});

auto _is_display_enabled(const Layout& layout, logicitem_id_t logicitem_id,
                         const logic_small_vector_t* input_values) -> bool {
    if (input_values == nullptr) {
        return true;
    }

    const auto input_id = display::enable_input_id;
    const auto is_inverted = layout.logic_items().input_inverted(logicitem_id, input_id);
    return input_values->at(input_id.value) ^ is_inverted;
}

auto _is_display_twos_complement(const Layout& layout, logicitem_id_t logicitem_id,
                                 const logic_small_vector_t* input_values) -> bool {
    const auto input_id = display_number::negative_input_id;
    const auto is_inverted = layout.logic_items().input_inverted(logicitem_id, input_id);

    if (input_values == nullptr) {
        return is_inverted;
    }

    return input_values->at(input_id.value) ^ is_inverted;
}

auto _draw_number_display_input_labels(Context& ctx, const Layout& layout,
                                       logicitem_id_t logicitem_id,
                                       ElementDrawState state, bool two_complement) {
    const auto input_count = layout.logic_items().input_count(logicitem_id);
    // TODO can we simplify this?
    const auto last_input_id = last_id(input_count);
    const auto has_space = display_number::input_shift(input_count) > grid_t {0};

    const auto to_label = [last_input_id, two_complement,
                           has_space](connection_id_t input_id) -> std::string_view {
        if (input_id == display::enable_input_id) {
            return "En";
        }
        if (input_id == display_number::negative_input_id) {
            return "n";
        }
        if (two_complement && input_id == last_input_id) {
            return has_space ? "sign" : "s";
        }
        // TODO checked math
        return power_of_two_labels.at(std::size_t {input_id} -
                                      std::size_t {display_number::control_inputs});
    };

    draw_input_connector_labels(ctx, layout, logicitem_id, state, to_label);
}

auto _draw_ascii_display_input_labels(Context& ctx, const Layout& layout,
                                      logicitem_id_t logicitem_id,
                                      ElementDrawState state) {
    const auto to_label = [](connection_id_t input_id) -> std::string_view {
        if (input_id == display::enable_input_id) {
            return "En";
        }
        // TODO checked math
        return power_of_two_labels.at(std::size_t {input_id} -
                                      std::size_t {display_ascii::control_inputs});
    };

    draw_input_connector_labels(ctx, layout, logicitem_id, state, to_label);
}

auto _inputs_to_number(const Layout& layout, logicitem_id_t logicitem_id,
                       const connection_count_t control_inputs,
                       const logic_small_vector_t& input_values) -> uint64_t {
    const auto& inverters = layout.logic_items().input_inverters(logicitem_id);

    if (input_values.size() - std::size_t {control_inputs} > std::size_t {64})
        [[unlikely]] {
        throw std::runtime_error("input size too large");
    }

    auto number = uint64_t {0};
    for (const auto& i : range(std::size_t {control_inputs}, input_values.size())) {
        const auto value = input_values.at(i) ^ inverters.at(i);
        number |= (static_cast<uint64_t>(value) << (i - std::size_t {control_inputs}));
    }
    return number;
}

struct styled_display_text_t {
    std::string text;
    color_t color {defaults::font::display_normal_color};
    grid_fine_t font_size {defaults::font::display_font_size};
    HTextAlignment horizontal_alignment {HTextAlignment::center};
    VTextAlignment vertical_alignment {VTextAlignment::center};
};

// to_text = [](uint64_t number) -> styled_display_text_t { ... };
template <typename Func>
auto _draw_number_display(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state, grid_fine_t element_width,
                          grid_fine_t element_height, Func to_text,
                          std::string_view interactive_mode_text,
                          connection_count_t control_inputs,
                          const logic_small_vector_t* input_values) {
    // TODO handle width / height differently

    // white background
    const auto text_x = grid_fine_t {1.} + (element_width - grid_fine_t {1.}) / 2.;
    const auto text_y =
        std::min(grid_fine_t {3.}, (element_height - grid_fine_t {1.}) / 2.);

    const auto h_margin = display::margin_horizontal;
    const auto v_padding = display::padding_vertical;

    const auto rect = rect_fine_t {
        point_fine_t {
            grid_fine_t {1.} + h_margin,  // x
            text_y - v_padding,           // y
        },
        point_fine_t {
            element_width - h_margin,  // x
            text_y + v_padding,        // y
        },
    };
    const auto position = layout.logic_items().position(logicitem_id);
    const auto text_position = point_fine_t {text_x, text_y} + position;

    draw_logic_item_rect(
        ctx, rect + position, state,
        LogicItemRectAttributes {.custom_fill_color = defaults::color_white});

    // number
    if (input_values != nullptr) {
        if (_is_display_enabled(layout, logicitem_id, input_values)) {
            auto number =
                _inputs_to_number(layout, logicitem_id, control_inputs, *input_values);
            const auto text = styled_display_text_t {to_text(number)};
            draw_logic_item_label(ctx, text_position, text.text, state,
                                  LogicItemTextAttributes {
                                      .custom_font_size = text.font_size,
                                      .custom_text_color = text.color,
                                      .horizontal_alignment = text.horizontal_alignment,
                                      .vertical_alignment = text.vertical_alignment,
                                      .style = defaults::font::display_font_style});
        }
    } else {
        draw_logic_item_label(
            ctx, text_position, interactive_mode_text, state,
            LogicItemTextAttributes {
                .custom_font_size = defaults::font::display_font_size,
                .custom_text_color = defaults::font::display_normal_color,
                .style = defaults::font::display_font_style,
            });
    }
}

auto _number_value_to_text(bool two_complement, std::size_t digit_count) {
    if (digit_count > 64) [[unlikely]] {
        throw std::runtime_error("too many digits");
    }

    return [two_complement, digit_count](uint64_t number) -> styled_display_text_t {
        if (two_complement) {
            uint64_t unsigned_value = number;

            // sign extensions
            if (0 < digit_count && digit_count < 64) {
                constexpr auto all_ones = ~uint64_t {0};
                const auto sign = (number >> (digit_count - std::size_t {1})) > 0;
                const auto sign_flag = sign ? all_ones : uint64_t {0};
                unsigned_value = ((all_ones << digit_count) & sign_flag) | number;
            }

            const auto signed_value = std::bit_cast<int64_t>(unsigned_value);
            return styled_display_text_t {
                fmt::format(std::locale("en_US.UTF-8"), "{:L}", signed_value)};
        }

        return styled_display_text_t {
            fmt::format(std::locale("en_US.UTF-8"), "{:L}", number)};
    };
}

}  // namespace

auto draw_display_number(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                         ElementDrawState state,
                         const logic_small_vector_t* input_values = nullptr) -> void {
    const auto input_count = layout.logic_items().input_count(logicitem_id);
    // TODO remove
    const auto element_width = grid_fine_t {display_number::width(input_count)};
    const auto element_height = grid_fine_t {display_number::height(input_count)};

    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    const auto two_complement =
        _is_display_twos_complement(layout, logicitem_id, input_values);
    const auto edit_mode_text = "0";
    const auto control_inputs = display_number::control_inputs;
    const auto value_inputs = display_number::value_inputs(input_count);
    const auto to_text =
        _number_value_to_text(two_complement, std::size_t {value_inputs});
    _draw_number_display(ctx, layout, logicitem_id, state, element_width, element_height,
                         to_text, edit_mode_text, control_inputs, input_values);
    _draw_number_display_input_labels(ctx, layout, logicitem_id, state, two_complement);
}

auto draw_display_number(Context& ctx, const SpatialSimulation& spatial_simulation,
                         logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto& input_values = spatial_simulation.simulation().input_values(element_id);

    draw_display_number(ctx, spatial_simulation.layout(), logicitem_id,
                        ElementDrawState::normal, &input_values);
}

namespace {
auto _asci_value_to_text(uint64_t number) -> styled_display_text_t {
    constexpr auto vertical_alignment = VTextAlignment::center_baseline;

    if (number > 127) [[unlikely]] {
        throw std::runtime_error("value out of range");
    }

    const auto control_chars =
        string_array<32> {"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",  //
                          "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",   //
                          "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",  //
                          "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US"};  //

    if (number < control_chars.size()) {
        return styled_display_text_t {
            .text = std::string {control_chars.at(number)},
            .color = defaults::font::display_ascii_controll_color,
            .font_size = defaults::font::display_ascii_control_size,
            .vertical_alignment = vertical_alignment,
        };
    }
    if (number == 127) {
        return styled_display_text_t {
            .text = std::string {"DEL"},
            .color = defaults::font::display_ascii_controll_color,
            .font_size = defaults::font::display_ascii_control_size,
            .vertical_alignment = vertical_alignment,
        };
    }
    return styled_display_text_t {
        .text = std::string {static_cast<char>(number)},
        .vertical_alignment = vertical_alignment,
    };
}
}  // namespace

auto draw_display_ascii(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                        ElementDrawState state,
                        const logic_small_vector_t* input_values = nullptr) -> void {
    // TODO remove
    const auto element_width = grid_fine_t {display_ascii::width};
    const auto element_height = grid_fine_t {display_ascii::height};

    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    const auto edit_mode_text = "A";
    const auto control_inputs = display_ascii::control_inputs;
    _draw_number_display(ctx, layout, logicitem_id, state, element_width, element_height,
                         _asci_value_to_text, edit_mode_text, control_inputs,
                         input_values);
    _draw_ascii_display_input_labels(ctx, layout, logicitem_id, state);
}

auto draw_display_ascii(Context& ctx, const SpatialSimulation& spatial_simulation,
                        logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto& input_values = spatial_simulation.simulation().input_values(element_id);

    draw_display_ascii(ctx, spatial_simulation.layout(), logicitem_id,
                       ElementDrawState::normal, &input_values);
}

auto draw_buffer(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                 ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);
    draw_logic_item_label(ctx, layout, logicitem_id, "1", state,
                          {.custom_font_size = defaults::font::buffer_label_size});
}

auto draw_buffer(Context& ctx, const SpatialSimulation& spatial_simulation,
                 logicitem_id_t logicitem_id) -> void {
    draw_buffer(ctx, spatial_simulation.layout(), logicitem_id, ElementDrawState::normal);
}

auto draw_clock_generator(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state) -> void {
    const auto& attrs = layout.logic_items().attrs_clock_generator(logicitem_id);
    const auto position = layout.logic_items().position(logicitem_id);

    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    // labels
    static constexpr auto input_labels = string_array<1> {"En"};
    static constexpr auto output_labels = string_array<1> {"C"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);

    // name
    draw_logic_item_label(ctx, position + point_fine_t {2.5, 0}, attrs.name, state,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::clock_name_size,
                              .custom_text_color = defaults::font::clock_name_color,
                              .horizontal_alignment = HTextAlignment::center,
                              .vertical_alignment = VTextAlignment::top_baseline,
                              .style = defaults::font::clock_name_style,
                          });

    // generator delay
    const auto duration_text = attrs.format_period();
    draw_logic_item_label(ctx, position + point_fine_t {2.5, 1}, duration_text, state,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::clock_period_size,
                              .custom_text_color = defaults::font::clock_period_color,
                              .horizontal_alignment = HTextAlignment::center,
                              .vertical_alignment = VTextAlignment::top_baseline,
                              .style = defaults::font::clock_period_style,
                          });
}

auto draw_clock_generator(Context& ctx, const SpatialSimulation& spatial_simulation,
                          logicitem_id_t logicitem_id) -> void {
    draw_clock_generator(ctx, spatial_simulation.layout(), logicitem_id,
                         ElementDrawState::normal);
}

auto draw_flipflop_jk(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                      ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<5> {"> C", "J", "K", "S", "R"};
    static constexpr auto output_labels = string_array<2> {"Q", "Q\u0305"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_flipflop_jk(Context& ctx, const SpatialSimulation& spatial_simulation,
                      logicitem_id_t logicitem_id) -> void {
    draw_flipflop_jk(ctx, spatial_simulation.layout(), logicitem_id,
                     ElementDrawState::normal);
}

auto draw_shift_register(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                         ElementDrawState state,
                         const logic_small_vector_t* internal_state = nullptr) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    // content
    const auto output_count =
        std::size_t {layout.logic_items().output_count(logicitem_id)};
    const auto state_size = std::size_t {10};

    const auto position = layout.logic_items().position(logicitem_id);
    for (auto n : range(output_count, state_size)) {
        const auto point = point_fine_t {
            -1.0 + 2.0 * static_cast<double>(n / output_count),
            0.25 + 1.5 * static_cast<double>(n % output_count),
        };
        const auto logic_value =
            internal_state != nullptr ? internal_state->at(n) : false;
        draw_binary_value(ctx, position + point, logic_value, state);
    }

    // labels
    static constexpr auto input_labels = string_array<3> {">", "", ""};
    static constexpr auto output_labels = string_array<2> {"", ""};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_shift_register(Context& ctx, const SpatialSimulation& spatial_simulation,
                         logicitem_id_t logicitem_id) -> void {
    const auto element_id = to_element_id(spatial_simulation, logicitem_id);
    const auto& internal_state =
        spatial_simulation.simulation().internal_state(element_id);

    draw_shift_register(ctx, spatial_simulation.layout(), logicitem_id,
                        ElementDrawState::normal, &internal_state);
}

auto draw_latch_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                  ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<2> {"E", "D"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_latch_d(Context& ctx, const SpatialSimulation& spatial_simulation,
                  logicitem_id_t logicitem_id) -> void {
    draw_latch_d(ctx, spatial_simulation.layout(), logicitem_id,
                 ElementDrawState::normal);
}

auto draw_flipflop_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                     ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_flipflop_d(Context& ctx, const SpatialSimulation& spatial_simulation,
                     logicitem_id_t logicitem_id) -> void {
    draw_flipflop_d(ctx, spatial_simulation.layout(), logicitem_id,
                    ElementDrawState::normal);
}

auto draw_flipflop_ms_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                        ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_flipflop_ms_d(Context& ctx, const SpatialSimulation& spatial_simulation,
                        logicitem_id_t logicitem_id) -> void {
    draw_flipflop_ms_d(ctx, spatial_simulation.layout(), logicitem_id,
                       ElementDrawState::normal);
}

//
// All Elements
//

auto draw_logic_item_base(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state) -> void {
    switch (layout.logic_items().type(logicitem_id)) {
        using enum LogicItemType;

        case buffer_element:
            return draw_buffer(ctx, layout, logicitem_id, state);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, layout, logicitem_id, state);

        case button:
            return draw_button(ctx, layout, logicitem_id, state);
        case led:
            return draw_led(ctx, layout, logicitem_id, state);
        case display_number:
            return draw_display_number(ctx, layout, logicitem_id, state);
        case display_ascii:
            return draw_display_ascii(ctx, layout, logicitem_id, state);

        case clock_generator:
            return draw_clock_generator(ctx, layout, logicitem_id, state);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, layout, logicitem_id, state);
        case shift_register:
            return draw_shift_register(ctx, layout, logicitem_id, state);
        case latch_d:
            return draw_latch_d(ctx, layout, logicitem_id, state);
        case flipflop_d:
            return draw_flipflop_d(ctx, layout, logicitem_id, state);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, layout, logicitem_id, state);

        case sub_circuit:
            return draw_standard_element(ctx, layout, logicitem_id, state);
    }
    throw std::runtime_error("not supported");
}

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements) -> void {
    for (const auto& entry : elements) {
        draw_logic_item_base(ctx, layout, entry.logicitem_id, entry.state);
    }
}

auto draw_logic_item_base(Context& ctx, const SpatialSimulation& spatial_simulation,
                          logicitem_id_t logicitem_id) -> void {
    switch (spatial_simulation.layout().logic_items().type(logicitem_id)) {
        using enum LogicItemType;

        case buffer_element:
            return draw_buffer(ctx, spatial_simulation, logicitem_id);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, spatial_simulation, logicitem_id);

        case button:
            return draw_button(ctx, spatial_simulation, logicitem_id);
        case led:
            return draw_led(ctx, spatial_simulation, logicitem_id);
        case display_number:
            return draw_display_number(ctx, spatial_simulation, logicitem_id);
        case display_ascii:
            return draw_display_ascii(ctx, spatial_simulation, logicitem_id);

        case clock_generator:
            return draw_clock_generator(ctx, spatial_simulation, logicitem_id);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, spatial_simulation, logicitem_id);
        case shift_register:
            return draw_shift_register(ctx, spatial_simulation, logicitem_id);
        case latch_d:
            return draw_latch_d(ctx, spatial_simulation, logicitem_id);
        case flipflop_d:
            return draw_flipflop_d(ctx, spatial_simulation, logicitem_id);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, spatial_simulation, logicitem_id);

        case sub_circuit:
            return draw_standard_element(ctx, spatial_simulation, logicitem_id);
    }
    throw std::runtime_error("not supported");
}

auto draw_logic_items_base(Context& ctx, const SpatialSimulation& spatial_simulation,
                           std::span<const logicitem_id_t> elements) -> void {
    for (const auto& logicitem_id : elements) {
        draw_logic_item_base(ctx, spatial_simulation, logicitem_id);
    }
}

}  // namespace logicsim
