#include "render_circuit.h"

#include "collision.h"
#include "concept.h"
#include "editable_circuit/selection.h"
#include "layout.h"
#include "layout_calculation.h"
#include "range.h"
#include "setting_handle.h"
#include "simulation.h"
#include "simulation_view.h"
#include "size_handle.h"
#include "svg_cache.h"
#include "timer.h"

#include <fmt/format.h>
#include <gsl/gsl>

#include <QDir>
#include <QFileInfo>

#include <locale>
#include <numbers>

namespace logicsim {

//
// Background
//
namespace {

auto draw_grid_space_limit(Context& ctx) {
    constexpr auto stroke_color = defaults::color_gray;
    const auto stroke_width = std::max(5.0, to_context(5.0, ctx));

    const auto p0 = to_context(point_t {grid_t::min(), grid_t::min()}, ctx);
    const auto p1 = to_context(point_t {grid_t::max(), grid_t::max()}, ctx);

    ctx.bl_ctx.setStrokeWidth(stroke_width);
    ctx.bl_ctx.strokeRect(BLRect {p0.x + 0.5, p0.y + 0.5, p1.x - p0.x, p1.y - p0.y},
                          stroke_color);
}

constexpr auto monochrome(uint8_t value) -> color_t {
    return color_t {value, value, value, 255};
}

auto draw_background_pattern_checker(Context& ctx, rect_fine_t scene_rect, int delta,
                                     color_t color, int width) {
    const auto clamp_to_grid = [](double v_) {
        return gsl::narrow_cast<grid_t::value_type>(
            std::clamp(v_, grid_t::min() * 1.0, grid_t::max() * 1.0));
    };

    const auto g0 = point_t {
        clamp_to_grid(std::floor(scene_rect.p0.x / delta) * delta),
        clamp_to_grid(std::floor(scene_rect.p0.y / delta) * delta),
    };
    const auto g1 = point_t {
        clamp_to_grid(std::ceil(scene_rect.p1.x / delta) * delta),
        clamp_to_grid(std::ceil(scene_rect.p1.y / delta) * delta),
    };

    /*
    for (int x = g0.x.value; x <= g1.x.value; x += delta) {
        const auto x_grid = grid_t {x};
        draw_line(ctx, line_t {{x_grid, g0.y}, {x_grid, g1.y}}, {color, width}, settings);
    }
    for (int y = g0.y.value; y <= g1.y.value; y += delta) {
        const auto y_grid = grid_t {y};
        draw_line(ctx, line_t {{g0.x, y_grid}, {g1.x, y_grid}}, {color, width}, settings);
    }
    */

    // this version is a bit faster
    const auto p0 = to_context(g0, ctx);
    const auto p1 = to_context(g1, ctx);

    const auto offset = ctx.settings.view_config.offset();
    const auto scale = ctx.settings.view_config.pixel_scale();

    // vertical
    for (int x = g0.x.value; x <= g1.x.value; x += delta) {
        const auto cx = round_fast((x + offset.x) * scale);
        draw_orthogonal_line(ctx, BLLine {cx, p0.y, cx, p1.y}, {color, width});
    }
    // horizontal
    for (int y = g0.y.value; y <= g1.y.value; y += delta) {
        const auto cy = round_fast((y + offset.y) * scale);
        draw_orthogonal_line(ctx, BLLine {p0.x, cy, p1.x, cy}, {color, width});
    }
}

auto draw_background_patterns(Context& ctx) {
    auto scene_rect = get_scene_rect_fine(ctx.settings.view_config);

    constexpr static auto grid_definition = {
        std::tuple {1, monochrome(0xF0), 1},    //
        std::tuple {8, monochrome(0xE4), 1},    //
        std::tuple {64, monochrome(0xE4), 2},   //
        std::tuple {512, monochrome(0xD8), 2},  //
        std::tuple {4096, monochrome(0xC0), 2},
    };

    for (auto&& [delta, color, width] : grid_definition) {
        if (delta * ctx.settings.view_config.device_scale() >=
            ctx.settings.background_grid_min_distance) {
            const auto draw_width_f =
                width * ctx.settings.view_config.device_pixel_ratio();
            // we substract a little, as we want 150% scaling to round down
            const auto epsilon = 0.01;
            const auto draw_width = std::max(1, round_to<int>(draw_width_f - epsilon));
            draw_background_pattern_checker(ctx, scene_rect, delta, color, draw_width);
        }
    }
}
}  // namespace

auto render_background(Context& ctx) -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.bl_ctx.fillAll(defaults::color_white);

    draw_background_patterns(ctx);
    draw_grid_space_limit(ctx);
}

//
// Connectors
//

auto do_draw_connector(const ViewConfig& view_config) {
    return view_config.pixel_scale() >= defaults::connector_cutoff_px;
}

auto _draw_connector_inverted(Context& ctx, ConnectorAttributes attributes) {
    const auto radius = defaults::inverted_circle_radius;
    const auto width = ctx.settings.view_config.stroke_width();
    const auto offset = stroke_offset(width);

    const auto r = radius * ctx.settings.view_config.pixel_scale();
    const auto p = to_context(attributes.position, ctx);
    const auto p_center = connector_point(p, attributes.orientation, r + width / 2.0);
    const auto p_adjusted = is_horizontal(attributes.orientation)
                                ? BLPoint {p_center.x, p_center.y + offset}
                                : BLPoint {p_center.x + offset, p_center.y};

    const auto fill_color =
        with_alpha_runtime(defaults::inverted_connector_fill, attributes.state);
    const auto stroke_color = wire_color(attributes.is_enabled, attributes.state);

    ctx.bl_ctx.fillCircle(BLCircle {p_adjusted.x, p_adjusted.y, r + width / 2.0},
                          stroke_color);
    ctx.bl_ctx.fillCircle(BLCircle {p_adjusted.x, p_adjusted.y, r - width / 2.0},
                          fill_color);
}

auto _draw_connector_normal(Context& ctx, ConnectorAttributes attributes) -> void {
    const auto endpoint = connector_point(attributes.position, attributes.orientation,
                                          defaults::connector_length);
    draw_line(ctx, line_fine_t {attributes.position, endpoint},
              {.color = wire_color(attributes.is_enabled, attributes.state)});
}

auto draw_connector(Context& ctx, ConnectorAttributes attributes) -> void {
    if (attributes.orientation == orientation_t::undirected) {
        return;
    }

    if (attributes.is_inverted) {
        _draw_connector_inverted(ctx, attributes);
    } else {
        _draw_connector_normal(ctx, attributes);
    }
}

auto draw_logic_item_connectors(Context& ctx, layout::ConstElement element,
                                ElementDrawState state) -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            draw_connector(ctx, ConnectorAttributes {
                                    .state = state,
                                    .position = position,
                                    .orientation = orientation,
                                    .is_inverted = element.input_inverted(input_id),
                                    .is_enabled = false,
                                });
            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            draw_connector(ctx, ConnectorAttributes {
                                    .state = state,
                                    .position = position,
                                    .orientation = orientation,
                                    .is_inverted = element.output_inverted(output_id),
                                    .is_enabled = false,
                                });
            return true;
        });
}

auto draw_logic_item_connectors(Context& ctx, layout::ConstElement element,
                                ElementDrawState state,
                                simulation_view::ConstElement logic_state) -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            const auto is_inverted = element.input_inverted(input_id);

            if (is_inverted || !logic_state.has_connected_input(input_id)) {
                draw_connector(ctx, ConnectorAttributes {
                                        .state = state,
                                        .position = position,
                                        .orientation = orientation,
                                        .is_inverted = is_inverted,
                                        .is_enabled = logic_state.input_value(input_id),
                                    });
            }
            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            const auto is_inverted = element.output_inverted(output_id);

            if (is_inverted || !logic_state.has_connected_output(output_id)) {
                draw_connector(ctx, ConnectorAttributes {
                                        .state = state,
                                        .position = position,
                                        .orientation = orientation,
                                        .is_inverted = is_inverted,
                                        .is_enabled = logic_state.output_value(output_id),
                                    });
            }
            return true;
        });
}

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const DrawableElement> elements) -> void {
    if (do_draw_connector(ctx.settings.view_config)) {
        for (const auto entry : elements) {
            draw_logic_item_connectors(ctx, layout.element(entry.element_id),
                                       entry.state);
        }
    }
}

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const element_id_t> elements,
                                 SimulationView simulation_view) -> void {
    if (do_draw_connector(ctx.settings.view_config)) {
        for (const auto element_id : elements) {
            const auto state = ElementDrawState::normal;
            draw_logic_item_connectors(ctx, layout.element(element_id), state,
                                       simulation_view.element(element_id));
        }
    }
}

auto connector_horizontal_alignment(orientation_t orientation) -> HorizontalAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return HorizontalAlignment::right;
        case left:
            return HorizontalAlignment::left;
        case up:
            return HorizontalAlignment::center;
        case down:
            return HorizontalAlignment::center;

        default:
            throw_exception("orienation has no horizontal alignment");
    };
}

auto connector_vertical_alignment(orientation_t orientation) -> VerticalAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return VerticalAlignment::center;
        case left:
            return VerticalAlignment::center;
        case up:
            return VerticalAlignment::top;
        case down:
            return VerticalAlignment::baseline;

        default:
            throw_exception("orienation has no vertical alignment");
    };
}

auto draw_connector_label(Context& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state) -> void {
    const auto point = label.size() > 0 && label.at(0) == '>'
                           ? point_fine_t {position}
                           : connector_point(position, orientation,
                                             -defaults::font::connector_label_margin);

    draw_text(ctx, point, label,
              TextAttributes {
                  .font_size = defaults::font::connector_label_size,
                  .color = get_logic_item_text_color(state),
                  .horizontal_alignment = connector_horizontal_alignment(orientation),
                  .vertical_alignment = connector_vertical_alignment(orientation),
              });
}

auto draw_connector_labels(Context& ctx, ConnectorLabels labels,
                           layout::ConstElement element, ElementDrawState state) -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            draw_connector_label(ctx, position, orientation,
                                 labels.input_labels[input_id.value], state);
            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            draw_connector_label(ctx, position, orientation,
                                 labels.output_labels[output_id.value], state);
            return true;
        });
}

template <typename Func>
auto draw_input_connector_labels(Context& ctx, layout::ConstElement element,
                                 ElementDrawState state, Func to_input_label) -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);

    iter_input_location_and_id(layout_data, [&](connection_id_t input_id,
                                                point_t position,
                                                orientation_t orientation) {
        draw_connector_label(ctx, position, orientation, to_input_label(input_id), state);
        return true;
    });
}

//
// Logic Items Body
//

auto draw_logic_item_above(ElementType type) -> bool {
    using enum ElementType;
    return type == button || type == led;
}

auto get_logic_item_state(layout::ConstElement element, const Selection* selection)
    -> ElementDrawState {
    const auto is_selected = [&]() {
        return (selection != nullptr) ? selection->is_selected(element.element_id())
                                      : false;
    };

    const auto display_state = element.display_state();

    if (is_inserted(display_state)) {
        if (display_state == display_state_t::valid) {
            return ElementDrawState::valid;
        } else if (is_selected()) {
            return ElementDrawState::normal_selected;
        }
        return ElementDrawState::normal;
    } else {
        if (display_state == display_state_t::colliding) {
            return ElementDrawState::colliding;
        } else if (is_selected()) {
            return ElementDrawState::temporary_selected;
        } else [[unlikely]] {
            throw_exception("cannot draw temporary items");
        }
    }
}

auto get_logic_item_fill_color(ElementDrawState state) -> color_t {
    switch (state) {
        using enum ElementDrawState;
        using namespace defaults;

        case normal:
            return with_alpha(body_fill_color::normal, normal);
        case normal_selected:
            return with_alpha(body_fill_color::normal_selected, normal_selected);
        case valid:
            return with_alpha(body_fill_color::valid, valid);
        case simulated:
            return with_alpha(body_fill_color::normal, simulated);
        case colliding:
            return with_alpha(body_fill_color::colliding, colliding);
        case temporary_selected:
            return with_alpha(body_fill_color::temporary_selected, temporary_selected);
    };

    throw_exception("draw state has no logic item base color");
}

auto get_logic_item_stroke_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::body_stroke_color, state);
}

auto get_logic_item_text_color(ElementDrawState state) -> color_t {
    return with_alpha_runtime(defaults::font::logic_item_text_color, state);
}

auto draw_logic_item_rect(Context& ctx, rect_fine_t rect, layout::ConstElement element,
                          ElementDrawState state, LogicItemRectAttributes attributes)
    -> void {
    const auto final_rect = rect + point_fine_t {element.position()};

    const auto fill_color = attributes.custom_fill_color
                                ? with_alpha_runtime(*attributes.custom_fill_color, state)
                                : get_logic_item_fill_color(state);
    const auto stroke_color =
        attributes.custom_stroke_color
            ? with_alpha_runtime(*attributes.custom_stroke_color, state)
            : get_logic_item_stroke_color(state);

    draw_rect(ctx, final_rect,
              RectAttributes {
                  .draw_type = DrawType::fill_and_stroke,
                  .fill_color = fill_color,
                  .stroke_color = stroke_color,
              });
}

auto draw_logic_item_label(Context& ctx, point_fine_t point, std::string_view text,
                           layout::ConstElement element, ElementDrawState state,
                           LogicItemTextAttributes attributes) -> void {
    if (text.empty()) {
        return;
    }

    const auto center = point + point_fine_t {element.position()};

    const auto font_size = attributes.custom_font_size
                               ? *attributes.custom_font_size
                               : defaults::font::logic_item_label_size;

    const auto text_color = attributes.custom_text_color
                                ? with_alpha_runtime(*attributes.custom_text_color, state)
                                : get_logic_item_text_color(state);

    draw_text(ctx, center, text,
              TextAttributes {
                  .font_size = font_size,
                  .color = text_color,
                  .horizontal_alignment = attributes.horizontal_alignment,
                  .vertical_alignment = attributes.vertical_alignment,
                  .style = attributes.style,
                  .cuttoff_size_px = defaults::font::text_cutoff_px,
              });
}

auto draw_binary_value(Context& ctx, point_fine_t point, bool is_enabled,
                       layout::ConstElement element, ElementDrawState state) -> void {
    const auto text = is_enabled ? std::string_view {"1"} : std::string_view {"0"};
    draw_logic_item_label(ctx, point, text, element, state,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::binary_value_size,
                          });
}

auto draw_binary_false(Context& ctx, point_fine_t point, layout::ConstElement element,
                       ElementDrawState state) -> void {
    const auto is_enabled = false;
    draw_binary_value(ctx, point, is_enabled, element, state);
}

//
// Individual Elements
//

constexpr auto standard_element_label(ElementType element_type) -> std::string_view {
    switch (element_type) {
        using enum ElementType;

        case and_element:
            return "&";
        case or_element:
            return ">1";
        case xor_element:
            return "=1";

        case sub_circuit:
            return "C";

        default:
            throw_exception("element type has no standard label");
    }
}

auto draw_standard_element(Context& ctx, layout::ConstElement element,
                           ElementDrawState state) -> void {
    const auto element_height =
        std::max(element.input_count(), element.output_count()) - std::size_t {1};
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {2., element_height + padding},
    };

    draw_logic_item_rect(ctx, rect, element, state);
    draw_logic_item_label(ctx, point_fine_t {1., element_height / 2.0},
                          standard_element_label(element.element_type()), element, state);
}

auto draw_button(Context& ctx, layout::ConstElement element, ElementDrawState state,

                 std::optional<simulation_view::ConstElement> logic_state) -> void {
    const auto padding = defaults::button_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {-padding, -padding},
        point_fine_t {+padding, +padding},
    };
    const auto logic_value = logic_state ? logic_state->internal_state(0) : false;

    draw_logic_item_rect(ctx, rect, element, state,
                         {.custom_fill_color = defaults::button_body_color});
    draw_binary_value(ctx, point_fine_t {0, 0}, logic_value, element, state);
}

auto draw_led(Context& ctx, layout::ConstElement element, ElementDrawState state,

              std::optional<simulation_view::ConstElement> logic_state) -> void {
    const auto logic_value =
        logic_state ? logic_state->input_value(connection_id_t {0}) : false;

    const auto base_color =
        logic_value ? defaults::led_color_enabled : defaults::led_color_disabled;

    draw_circle(ctx, point_fine_t {element.position()},
                grid_fine_t {defaults::led_radius},
                CircleAttributes {
                    .fill_color = with_alpha_runtime(base_color, state),
                    .stroke_color = get_logic_item_stroke_color(state),
                });
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

static_assert(display_number::max_value_inputs <= power_of_two_labels.size());
static_assert(display_ascii::value_inputs <= power_of_two_labels.size());

auto _is_display_enabled(layout::ConstElement element,
                         std::optional<simulation_view::ConstElement> logic_state)
    -> bool {
    const auto input_id = display::enable_input_id;

    if (!logic_state) {
        return true;
    }
    return logic_state->input_value(input_id) ^ element.input_inverted(input_id);
}

auto _is_display_twos_complement(layout::ConstElement element,
                                 std::optional<simulation_view::ConstElement> logic_state)
    -> bool {
    const auto input_id = display_number::negative_input_id;

    if (!logic_state) {
        return element.input_inverted(input_id);
    }
    return logic_state->input_value(input_id) ^ element.input_inverted(input_id);
}

auto _draw_number_display_input_labels(Context& ctx, layout::ConstElement element,
                                       ElementDrawState state, bool two_complement) {
    const auto input_count = element.input_count();
    const auto last_input_id = connection_id_t {
        gsl::narrow<connection_id_t::value_type>(input_count - std::size_t {1})};
    const auto has_space =
        display_number::input_shift(element.input_count()) > grid_t {0};

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
        const auto index = gsl::narrow<std::size_t>(input_id.value);
        return power_of_two_labels.at(index - display_number::control_inputs);
    };

    draw_input_connector_labels(ctx, element, state, to_label);
}

auto _draw_ascii_display_input_labels(Context& ctx, layout::ConstElement element,
                                      ElementDrawState state) {
    const auto to_label = [](connection_id_t input_id) -> std::string_view {
        if (input_id == display::enable_input_id) {
            return "En";
        }
        const auto index = gsl::narrow<std::size_t>(input_id.value);
        return power_of_two_labels.at(index - display_ascii::control_inputs);
    };

    draw_input_connector_labels(ctx, element, state, to_label);
}

auto _inputs_to_number(layout::ConstElement element,
                       simulation_view::ConstElement logic_state,
                       const std::size_t control_inputs) -> uint64_t {
    const auto& values = logic_state.input_values();
    const auto& inverters = element.input_inverters();

    if (values.size() > 64 + control_inputs) {
        throw_exception("input size too large");
    }

    auto number = uint64_t {0};
    for (const auto& i : range(control_inputs, values.size())) {
        const auto value = values.at(i) ^ inverters.at(i);
        number |= (static_cast<uint64_t>(value) << (i - control_inputs));
    }
    return number;
}

struct styled_display_text_t {
    std::string text;
    color_t color {defaults::font::display_normal_color};
    double font_size {defaults::font::display_font_size};
    HorizontalAlignment horizontal_alignment {HorizontalAlignment::center};
    VerticalAlignment vertical_alignment {VerticalAlignment::center};
};

// to_text = [](uint64_t number) -> styled_display_text_t { ... };
template <typename Func>
auto _draw_number_display(Context& ctx, layout::ConstElement element,
                          ElementDrawState state, grid_fine_t element_width,
                          grid_fine_t element_height, Func to_text,
                          std::string_view interactive_mode_text,
                          std::size_t control_inputs,
                          std::optional<simulation_view::ConstElement> logic_state) {
    // white background
    const auto text_x = 1. + (element_width - 1.) / 2.;
    const auto text_y = std::min(3., (element_height - 1.) / 2.);

    const auto h_margin = display::margin_horizontal;
    const auto v_padding = display::padding_vertical;

    const auto rect = rect_fine_t {
        point_fine_t {
            1. + h_margin,       // x
            text_y - v_padding,  // y
        },
        point_fine_t {
            element_width - h_margin,  // x
            text_y + v_padding,        // y
        },
    };
    draw_logic_item_rect(
        ctx, rect, element, state,
        LogicItemRectAttributes {.custom_fill_color = defaults::color_white});

    // number
    if (logic_state) {
        if (_is_display_enabled(element, logic_state)) {
            auto number = _inputs_to_number(element, *logic_state, control_inputs);
            const auto text = styled_display_text_t {to_text(number)};
            draw_logic_item_label(ctx, point_fine_t {text_x, text_y}, text.text, element,
                                  state,
                                  LogicItemTextAttributes {
                                      .custom_font_size = text.font_size,
                                      .custom_text_color = text.color,
                                      .horizontal_alignment = text.horizontal_alignment,
                                      .vertical_alignment = text.vertical_alignment,
                                      .style = defaults::font::display_font_style});
        }
    } else {
        draw_logic_item_label(
            ctx, point_fine_t {text_x, text_y}, interactive_mode_text, element, state,
            LogicItemTextAttributes {
                .custom_font_size = defaults::font::display_font_size,
                .custom_text_color = defaults::font::display_normal_color,
                .style = defaults::font::display_font_style,
            });
    }
}

auto _number_value_to_text(bool two_complement, std::size_t digit_count) {
    if (digit_count > 64) [[unlikely]] {
        throw_exception("too many digits");
    }

    return [two_complement, digit_count](uint64_t number) -> styled_display_text_t {
        if (two_complement) {
            uint64_t unsigned_value = number;

            // sign extensions
            if (0 < digit_count && digit_count < 64) {
                constexpr auto all_ones = ~uint64_t {0};
                const auto sign = number >> (digit_count - std::size_t {1});
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

auto draw_display_number(Context& ctx, layout::ConstElement element,
                         ElementDrawState state,
                         std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    const auto input_count = element.input_count();
    const auto element_width = grid_fine_t {display_number::width(input_count)};
    const auto element_height = grid_fine_t {display_number::height(input_count)};
    const auto padding = defaults::logic_item_body_overdraw;

    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {element_width, element_height + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    const auto two_complement = _is_display_twos_complement(element, logic_state);
    const auto edit_mode_text = "0";
    const auto control_inputs = display_number::control_inputs;
    const auto to_text =
        _number_value_to_text(two_complement, input_count - control_inputs);
    _draw_number_display(ctx, element, state, element_width, element_height, to_text,
                         edit_mode_text, control_inputs, logic_state);
    _draw_number_display_input_labels(ctx, element, state, two_complement);
}

namespace {
auto _asci_value_to_text(uint64_t number) -> styled_display_text_t {
    constexpr auto vertical_alignment = VerticalAlignment::center_baseline;

    if (number > 127) [[unlikely]] {
        throw_exception("value out of range");
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

auto draw_display_ascii(Context& ctx, layout::ConstElement element,
                        ElementDrawState state,
                        std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    const auto element_width = grid_fine_t {display_ascii::width};
    const auto element_height = grid_fine_t {display_ascii::height};
    const auto padding = defaults::logic_item_body_overdraw;

    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {element_width, element_height + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    const auto edit_mode_text = "A";
    const auto control_inputs = display_ascii::control_inputs;
    _draw_number_display(ctx, element, state, element_width, element_height,
                         _asci_value_to_text, edit_mode_text, control_inputs,
                         logic_state);
    _draw_ascii_display_input_labels(ctx, element, state);
}

auto draw_buffer(Context& ctx, layout::ConstElement element, ElementDrawState state)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., -padding},
        point_fine_t {1., +padding},
    };

    draw_logic_item_rect(ctx, rect, element, state);
    draw_logic_item_label(ctx, point_fine_t {0.5, 0.}, "1", element, state,
                          {.custom_font_size = defaults::font::buffer_label_size});
}

auto draw_clock_generator(Context& ctx, layout::ConstElement element,
                          ElementDrawState state) -> void {
    const auto& attrs = element.attrs_clock_generator();

    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {3., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    // labels
    static constexpr auto input_labels = string_array<1> {"En"};
    static constexpr auto output_labels = string_array<1> {"C"};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);

    // generator delay
    const auto duration_text = fmt::format("{}", attrs.period);
    draw_logic_item_label(ctx, point_fine_t {1.5, 0}, duration_text, element, state,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::clock_period_size,
                              .custom_text_color = defaults::font::clock_period_color,
                              .horizontal_alignment = HorizontalAlignment::center,
                              .vertical_alignment = VerticalAlignment::top,
                              .style = FontStyle::bold,
                          });

    // name
    draw_logic_item_label(ctx, point_fine_t {1.5, -1}, attrs.name, element, state);
}

auto draw_flipflop_jk(Context& ctx, layout::ConstElement element, ElementDrawState state)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {4., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    static constexpr auto input_labels = string_array<5> {"> C", "J", "K", "S", "R"};
    static constexpr auto output_labels = string_array<2> {"Q", "Q\u0305"};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);
}

auto draw_shift_register(Context& ctx, layout::ConstElement element,
                         ElementDrawState state,
                         std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {8., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    // content
    const auto output_count = element.output_count();
    const auto state_size = std::size_t {10};

    for (auto n : range(output_count, state_size)) {
        const auto point = point_fine_t {
            -1 + 2.0 * (n / output_count),
            0.25 + 1.5 * (n % output_count),
        };
        const auto logic_value = logic_state ? logic_state->internal_state(n) : false;
        draw_binary_value(ctx, point, logic_value, element, state);
    }

    // labels
    static constexpr auto input_labels = string_array<3> {">", "", ""};
    static constexpr auto output_labels = string_array<2> {"", ""};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);
}

auto draw_latch_d(Context& ctx, layout::ConstElement element, ElementDrawState state)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {2., 1. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    static constexpr auto input_labels = string_array<2> {"E", "D"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);
}

auto draw_flipflop_d(Context& ctx, layout::ConstElement element, ElementDrawState state)
    -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {3., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);
}

auto draw_flipflop_ms_d(Context& ctx, layout::ConstElement element,
                        ElementDrawState state) -> void {
    const auto padding = defaults::logic_item_body_overdraw;
    const auto rect = rect_fine_t {
        point_fine_t {0., 0. - padding},
        point_fine_t {4., 2. + padding},
    };
    draw_logic_item_rect(ctx, rect, element, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, ConnectorLabels {input_labels, output_labels}, element,
                          state);
}

//
// All Elements
//

auto draw_logic_item_base(Context& ctx, layout::ConstElement element,
                          ElementDrawState state,
                          std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    switch (element.element_type()) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            [[unlikely]] throw_exception("not supported");

        case buffer_element:
            return draw_buffer(ctx, element, state);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, element, state);

        case button:
            return draw_button(ctx, element, state, logic_state);
        case led:
            return draw_led(ctx, element, state, logic_state);
        case display_number:
            return draw_display_number(ctx, element, state, logic_state);
        case display_ascii:
            return draw_display_ascii(ctx, element, state, logic_state);

        case clock_generator:
            return draw_clock_generator(ctx, element, state);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, element, state);
        case shift_register:
            return draw_shift_register(ctx, element, state, logic_state);
        case latch_d:
            return draw_latch_d(ctx, element, state);
        case flipflop_d:
            return draw_flipflop_d(ctx, element, state);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, element, state);

        case sub_circuit:
            return draw_standard_element(ctx, element, state);
    }
    throw_exception("not supported");
}

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements) -> void {
    for (const auto entry : elements) {
        draw_logic_item_base(ctx, layout.element(entry.element_id), entry.state);
    }
}

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const element_id_t> elements,
                           SimulationView simulation_view) -> void {
    const auto state = ElementDrawState::normal;

    for (const auto element_id : elements) {
        draw_logic_item_base(ctx, layout.element(element_id), state,
                             simulation_view.element(element_id));
    }
}

//
// Wire
//

auto wire_color(bool is_enabled) -> color_t {
    if (is_enabled) {
        return defaults::wire_color_enabled;
    }
    return defaults::wire_color_disabled;
}

auto wire_color(bool is_enabled, ElementDrawState state) -> color_t {
    return with_alpha_runtime(wire_color(is_enabled), state);
}

auto draw_line_cross_point(Context& ctx, const point_t point, bool is_enabled,
                           ElementDrawState state) -> void {
    int lc_width = ctx.settings.view_config.line_cross_width();
    if (lc_width <= 0) {
        return;
    }

    const int wire_width = ctx.settings.view_config.stroke_width();
    const int wire_offset = (wire_width - 1) / 2;

    const int size = 2 * lc_width + wire_width;
    const int offset = wire_offset + lc_width;

    const auto [x, y] = to_context(point, ctx);
    const auto color = wire_color(is_enabled, state);

    ctx.bl_ctx.fillRect(BLRect {x - offset, y - offset, 1. * size, 1. * size}, color);
}

auto draw_line_segment(Context& ctx, line_fine_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void {
    const auto color = wire_color(attributes.is_enabled, state);
    draw_line(ctx, line,
              LineAttributes {
                  .color = color,
                  .p0_endcap = attributes.p0_endcap,
                  .p1_endcap = attributes.p1_endcap,
              });
}

auto draw_line_segment(Context& ctx, ordered_line_t line, SegmentAttributes attributes,
                       ElementDrawState state) -> void {
    draw_line_segment(ctx, line_fine_t {line}, attributes, state);
}

auto draw_line_segment(Context& ctx, segment_info_t info, bool is_enabled,
                       ElementDrawState state) -> void {
    draw_line_segment(ctx, info.line,
                      SegmentAttributes {
                          .is_enabled = is_enabled,
                          .p0_endcap = info.p0_type == SegmentPointType::corner_point,
                          .p1_endcap = info.p1_type == SegmentPointType::corner_point,
                      },
                      state);

    if (is_cross_point(info.p0_type)) {
        draw_line_cross_point(ctx, info.line.p0, is_enabled, state);
    }
    if (is_cross_point(info.p1_type)) {
        draw_line_cross_point(ctx, info.line.p1, is_enabled, state);
    }
}

auto draw_segment_tree(Context& ctx, layout::ConstElement element, bool is_enabled,
                       ElementDrawState state) -> void {
    for (const segment_info_t& info : element.segment_tree().segment_infos()) {
        draw_line_segment(ctx, info, is_enabled, state);
    }
}

auto draw_segment_tree(Context& ctx, layout::ConstElement element, ElementDrawState state)
    -> void {
    bool is_enabled = false;
    draw_segment_tree(ctx, element, is_enabled, state);
}

auto _draw_line_segment_with_history(Context& ctx, point_t p_from, point_t p_until,
                                     time_t time_from, time_t time_until,
                                     const simulation::HistoryView& history) -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.first_time);
        const auto p_end =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.last_time);

        if (p_start != p_end) [[likely]] {
            // TODO !!! endcaps
            draw_line_segment(ctx, line_fine_t {p_start, p_end}, {entry.value},
                              ElementDrawState::normal);
        }
    }
}

auto _draw_wire_with_history(Context& ctx, layout::ConstElement element,
                             simulation_view::ConstElement logic_state,
                             const simulation::HistoryView& history) -> void {
    if (history.size() < 2) [[unlikely]] {
        throw_exception("requires history view with at least 2 entries");
    }

    const auto to_time =
        [time = logic_state.time(),
         delay = logic_state.wire_delay_per_distance()](LineTree::length_t length_) {
            return time_t {time.value - static_cast<int64_t>(length_) * delay.value};
        };

    for (auto&& segment : element.line_tree().sized_segments()) {
        _draw_line_segment_with_history(ctx, segment.line.p1, segment.line.p0,
                                        to_time(segment.p1_length),
                                        to_time(segment.p0_length), history);

        if (segment.has_cross_point_p0) {
            bool wire_enabled = history.value(to_time(segment.p0_length));
            draw_line_cross_point(ctx, segment.line.p0, wire_enabled,
                                  ElementDrawState::normal);
        }
    }
}

auto draw_wire(Context& ctx, layout::ConstElement element,
               simulation_view::ConstElement logic_state) -> void {
    const auto history = logic_state.input_history();

    if (history.size() < 2) {
        draw_segment_tree(ctx, element, history.last_value(), ElementDrawState::normal);
        return;
    }

    _draw_wire_with_history(ctx, element, logic_state, history);
}

//
//
//

auto draw_wires(Context& ctx, const Layout& layout,
                std::span<const DrawableElement> elements) -> void {
    for (const auto entry : elements) {
        draw_segment_tree(ctx, layout.element(entry.element_id), entry.state);
    }
}

auto draw_wires(Context& ctx, const Layout& layout,
                std::span<const element_id_t> elements, ElementDrawState state) -> void {
    for (const auto element_id : elements) {
        draw_segment_tree(ctx, layout.element(element_id), state);
    }
}

auto draw_wires(Context& ctx, const Layout& layout,
                std::span<const element_id_t> elements, SimulationView simulation_view)
    -> void {
    for (const auto element_id : elements) {
        draw_wire(ctx, layout.element(element_id), simulation_view.element(element_id));
    }
}

auto draw_wires(Context& ctx, std::span<const segment_info_t> segment_infos,
                ElementDrawState state) -> void {
    for (const auto& info : segment_infos) {
        const auto is_enabled = false;
        draw_line_segment(ctx, info, is_enabled, state);
    }
}

//
// Size Handles
//

namespace {

struct OutlinedRectAttributes {
    color_t fill_color;
    color_t stroke_color;
    double stroke_width_device;
};

auto draw_outlined_rect_px(Context& ctx, BLRect rect, OutlinedRectAttributes attributes) {
    auto stroke_width =
        std::max(1., round_fast(attributes.stroke_width_device *
                                ctx.settings.view_config.device_pixel_ratio()));

    // draw square
    ctx.bl_ctx.fillRect(rect, attributes.stroke_color);
    rect.x += stroke_width;
    rect.y += stroke_width;
    rect.w -= stroke_width * 2;
    rect.h -= stroke_width * 2;
    ctx.bl_ctx.fillRect(rect, attributes.fill_color);
}
}  // namespace

auto draw_size_handle(Context& ctx, const size_handle_t& position) -> void {
    auto rect = size_handle_rect_px(position, ctx.settings.view_config);

    draw_outlined_rect_px(
        ctx, rect,
        OutlinedRectAttributes {
            .fill_color = defaults::size_handle_color_fill,
            .stroke_color = defaults::size_handle_color_stroke,
            .stroke_width_device = defaults::size_handle_stroke_width_device,
        });
}

auto draw_size_handles(Context& ctx, std::span<const size_handle_t> handle_positions)
    -> void {
    for (const auto& position : handle_positions) {
        draw_size_handle(ctx, position);
    }
}

auto render_size_handles(Context& ctx, const Layout& layout, const Selection& selection)
    -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    draw_size_handles(ctx, size_handle_positions(layout, selection));
}

//
// Setting Handle
//

auto draw_setting_handle(Context& ctx, setting_handle_t handle) -> void {
    auto rect = setting_handle_rect(handle);

    // button rect
    draw_rect(ctx, rect,
              RectAttributes {
                  .draw_type = DrawType::fill_and_stroke,
                  .fill_color = defaults::setting_handle_color_fill,
                  .stroke_color = defaults::setting_handle_color_stroke,
              });

    // button icon
    draw_icon(ctx, get_center(rect), handle.icon,
              IconAttributes {
                  .icon_height = defaults::setting_handle_icon_scale,
                  .color = defaults::setting_handle_color_icon,
                  .horizontal_alignment = HorizontalAlignment::center,
                  .vertical_alignment = VerticalAlignment::center,
              });
}

auto render_setting_handle(Context& ctx, const Layout& layout, const Selection& selection)
    -> void {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    if (const auto handle = setting_handle_position(layout, selection)) {
        draw_setting_handle(ctx, *handle);
    }
}

//
// Overlay
//

template <>
auto format(shadow_t orientation) -> std::string {
    switch (orientation) {
        using enum shadow_t;

        case selected:
            return "selected";
        case valid:
            return "valid";
        case colliding:
            return "colliding";
    }
    throw_exception("Don't know how to convert shadow_t to string.");
}

auto shadow_color(shadow_t shadow_type) -> color_t {
    switch (shadow_type) {
        case shadow_t::selected: {
            return defaults::overlay_color::selected;
        }
        case shadow_t::valid: {
            return defaults::overlay_color::valid;
        }
        case shadow_t::colliding: {
            return defaults::overlay_color::colliding;
        }
    };

    throw_exception("unknown shadow type");
}

auto element_shadow_rounding(ElementType type [[maybe_unused]]) -> double {
    return type == ElementType::button ? 0. : defaults::line_selection_padding;
}

auto draw_logic_item_shadow(Context& ctx, layout::ConstElement element,
                            shadow_t shadow_type) -> void {
    const auto data = to_layout_calculation_data(element.layout(), element);
    const auto selection_rect = element_selection_rect(data);

    draw_round_rect(ctx, selection_rect,
                    {
                        .draw_type = DrawType::fill,
                        .rounding = element_shadow_rounding(data.element_type),
                        .fill_color = shadow_color(shadow_type),
                    });
}

auto draw_logic_item_shadows(Context& ctx, const Layout& layout,
                             std::span<const element_id_t> elements, shadow_t shadow_type)
    -> void {
    for (const auto element_id : elements) {
        draw_logic_item_shadow(ctx, layout.element(element_id), shadow_type);
    }
}

template <input_range_of<ordered_line_t> View>
auto draw_wire_shadows_impl(Context& ctx, View lines, shadow_t shadow_type) -> void {
    const auto color = shadow_color(shadow_type);

    for (const ordered_line_t line : lines) {
        const auto selection_rect = element_selection_rect_rounded(line);
        draw_round_rect(ctx, selection_rect,
                        {
                            .draw_type = DrawType::fill,
                            .stroke_width = defaults::use_view_config_stroke_width,
                            .fill_color = color,

                        });
    }
}

auto draw_wire_shadows(Context& ctx, std::span<const ordered_line_t> lines,
                       shadow_t shadow_type) -> void {
    draw_wire_shadows_impl(ctx, lines, shadow_type);
}

auto draw_wire_shadows(Context& ctx, std::span<const segment_info_t> segment_infos,
                       shadow_t shadow_type) -> void {
    draw_wire_shadows_impl(
        ctx, transform_view(segment_infos, [](segment_info_t info) { return info.line; }),
        shadow_type);
}

//
// Interactive Layers
//

auto InteractiveLayers::format() const -> std::string {
    return fmt::format(
        "InteractiveLayers("
        "\n  normal_below = {}"
        "\n  normal_wires = {}"
        "\n  normal_above = {}"
        "\n"
        "\n  uninserted_below = {}"
        "\n  uninserted_above = {}"
        "\n"
        "\n  selected_logic_items = {}"
        "\n  selected_wires = {}"
        "\n  temporary_wires = {}"
        "\n  valid_logic_items = {}"
        "\n  valid_wires = {}"
        "\n  colliding_logic_items = {}"
        "\n  colliding_wires = {}"
        "\n"
        "\n  uninserted_bounding_rect = {}"
        "\n  overlay_bounding_rect = {}"
        "\n)",

        normal_below,  //
        normal_wires,  //
        normal_above,  //

        uninserted_below,  //
        uninserted_above,  //

        selected_logic_items,   //
        selected_wires,         //
        temporary_wires,        //
        valid_logic_items,      //
        valid_wires,            //
        colliding_logic_items,  //
        colliding_wires,        //

        uninserted_bounding_rect,  //
        overlay_bounding_rect      //
    );
}

auto InteractiveLayers::clear() -> void {
    normal_below.clear();
    normal_wires.clear();
    normal_above.clear();

    uninserted_below.clear();
    uninserted_above.clear();

    selected_logic_items.clear();
    selected_wires.clear();
    temporary_wires.clear();
    valid_logic_items.clear();
    valid_wires.clear();
    colliding_logic_items.clear();
    colliding_wires.clear();

    uninserted_bounding_rect.reset();
    overlay_bounding_rect.reset();
}

auto InteractiveLayers::shrink_to_fit() -> void {
    normal_below.shrink_to_fit();
    normal_wires.shrink_to_fit();
    normal_above.shrink_to_fit();

    uninserted_below.shrink_to_fit();
    uninserted_above.shrink_to_fit();

    selected_logic_items.shrink_to_fit();
    selected_wires.shrink_to_fit();
    temporary_wires.shrink_to_fit();
    valid_logic_items.shrink_to_fit();
    valid_wires.shrink_to_fit();
    colliding_logic_items.shrink_to_fit();
    colliding_wires.shrink_to_fit();
}

auto InteractiveLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(normal_below) +           //
           get_allocated_size(normal_wires) +           //
           get_allocated_size(normal_above) +           //
                                                        //
           get_allocated_size(uninserted_below) +       //
           get_allocated_size(uninserted_above) +       //
                                                        //
           get_allocated_size(selected_logic_items) +   //
           get_allocated_size(selected_wires) +         //
           get_allocated_size(temporary_wires) +        //
           get_allocated_size(valid_logic_items) +      //
           get_allocated_size(valid_wires) +            //
           get_allocated_size(colliding_logic_items) +  //
           get_allocated_size(colliding_wires);         //
}

auto InteractiveLayers::has_inserted() const -> bool {
    return !normal_below.empty() ||  //
           !normal_wires.empty() ||  //
           !normal_above.empty();
}

auto InteractiveLayers::has_uninserted() const -> bool {
    return !uninserted_below.empty() ||  //
           !temporary_wires.empty() ||   //
           !colliding_wires.empty() ||   //
           !uninserted_above.empty();
}

auto InteractiveLayers::has_overlay() const -> bool {
    return !selected_logic_items.empty() ||   //
           !selected_wires.empty() ||         //
           !temporary_wires.empty() ||        //
           !valid_logic_items.empty() ||      //
           !valid_wires.empty() ||            //
           !colliding_logic_items.empty() ||  //
           !colliding_wires.empty();
}

auto InteractiveLayers::calculate_overlay_bounding_rect() -> void {
    const auto update = [this](ordered_line_t line) { update_overlay_rect(*this, line); };
    const auto update_info = [this](segment_info_t info) {
        update_overlay_rect(*this, info.line);
    };

    std::ranges::for_each(selected_wires, update);
    std::ranges::for_each(temporary_wires, update_info);
    std::ranges::for_each(valid_wires, update);
    std::ranges::for_each(colliding_wires, update_info);
}

auto update_bounding_rect(std::optional<rect_t>& target, rect_t new_rect) -> void {
    if (!target) {
        target = new_rect;
    } else {
        *target = enclosing_rect(*target, new_rect);
    }
}

auto update_bounding_rect(std::optional<rect_t>& target, ordered_line_t new_line)
    -> void {
    if (!target) {
        target = rect_t {new_line.p0, new_line.p1};
    } else {
        *target = enclosing_rect(*target, new_line);
    }
}

auto update_uninserted_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, bounding_rect);
}

auto update_uninserted_rect(InteractiveLayers& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.uninserted_bounding_rect, line);
}

auto update_overlay_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, bounding_rect);
}

auto update_overlay_rect(InteractiveLayers& layers, ordered_line_t line) -> void {
    update_bounding_rect(layers.overlay_bounding_rect, line);
}

//
// Simulation Layers
//

auto SimulationLayers::format() const -> std::string {
    return fmt::format(
        "InteractiveLayers("
        "\n  items_below = {}"
        "\n  wires = {}"
        "\n  items_above = {}"
        "\n)",

        items_below,  //
        wires,        //
        items_above   //
    );
}

auto SimulationLayers::clear() -> void {
    items_below.clear();
    wires.clear();
    items_above.clear();
}

auto SimulationLayers::shrink_to_fit() -> void {
    items_below.shrink_to_fit();
    wires.shrink_to_fit();
    items_above.shrink_to_fit();
}

auto SimulationLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(items_below) +  //
           get_allocated_size(wires) +        //
           get_allocated_size(items_above);
}

//
// Circuit Context
//

auto CircuitSurfaces::clear() -> void {
    layer_surface_uninserted.clear();
    layer_surface_overlay.clear();
}

auto CircuitSurfaces::shrink_to_fit() -> void {
    layer_surface_uninserted.shrink_to_fit();
    layer_surface_overlay.shrink_to_fit();
}

auto CircuitLayers::clear() -> void {
    interactive_layers.clear();
    simulation_layers.clear();
}

auto CircuitLayers::shrink_to_fit() -> void {
    interactive_layers.shrink_to_fit();
    simulation_layers.shrink_to_fit();
}

auto CircuitContext::clear() -> void {
    ctx.clear();
    layers.clear();
    surfaces.clear();
}

auto CircuitContext::shrink_to_fit() -> void {
    ctx.shrink_to_fit();
    layers.shrink_to_fit();
    surfaces.shrink_to_fit();
}

//
// Layout
//

auto render_inserted(Context& ctx, const Layout& layout,
                     const InteractiveLayers& layers) {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logic_items_base(ctx, layout, layers.normal_below);
    draw_wires(ctx, layout, layers.normal_wires, ElementDrawState::normal);
    draw_logic_items_base(ctx, layout, layers.normal_above);

    draw_logic_items_connectors(ctx, layout, layers.normal_below);
    draw_logic_items_connectors(ctx, layout, layers.normal_above);
}

auto render_uninserted(Context& ctx, const Layout& layout,
                       const InteractiveLayers& layers, bool layer_enabled) {
    if (layer_enabled) {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    draw_logic_items_base(ctx, layout, layers.uninserted_below);
    draw_wires(ctx, layers.temporary_wires, ElementDrawState::temporary_selected);
    draw_wires(ctx, layers.colliding_wires, ElementDrawState::colliding);
    draw_logic_items_base(ctx, layout, layers.uninserted_above);

    draw_logic_items_connectors(ctx, layout, layers.uninserted_below);
    draw_logic_items_connectors(ctx, layout, layers.uninserted_above);
}

auto render_overlay(Context& ctx, const Layout& layout, const InteractiveLayers& layers,
                    bool layer_enabled) -> void {
    if (layer_enabled) {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    } else {
        ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }

    // selected & temporary
    draw_logic_item_shadows(ctx, layout, layers.selected_logic_items, shadow_t::selected);
    draw_wire_shadows(ctx, layers.selected_wires, shadow_t::selected);
    draw_wire_shadows(ctx, layers.temporary_wires, shadow_t::selected);

    // valid
    draw_logic_item_shadows(ctx, layout, layers.valid_logic_items, shadow_t::valid);
    draw_wire_shadows(ctx, layers.valid_wires, shadow_t::valid);

    // colliding
    draw_logic_item_shadows(ctx, layout, layers.colliding_logic_items,
                            shadow_t::colliding);
    draw_wire_shadows(ctx, layers.colliding_wires, shadow_t::colliding);
}

auto render_interactive_layers(Context& ctx, const Layout& layout,
                               const InteractiveLayers& layers, CircuitSurfaces& surfaces)
    -> void {
    if (layers.has_inserted()) {
        render_inserted(ctx, layout, layers);
    }

    if (layers.uninserted_bounding_rect.has_value()) {
        const auto rect = get_dirty_rect(layers.uninserted_bounding_rect.value(),
                                         ctx.settings.view_config);

        render_to_layer(ctx, surfaces.layer_surface_uninserted, rect,
                        [&](Context& layer_ctx, bool layer_enabled) {
                            render_uninserted(layer_ctx, layout, layers, layer_enabled);
                        });
    }

    if (layers.overlay_bounding_rect.has_value()) {
        const auto rect = get_dirty_rect(layers.overlay_bounding_rect.value(),
                                         ctx.settings.view_config);

        render_to_layer(ctx, surfaces.layer_surface_overlay, rect,
                        [&](Context& layer_ctx, bool layer_enabled) {
                            render_overlay(layer_ctx, layout, layers, layer_enabled);
                        });
    }
}

auto render_simulation_layers(Context& ctx, const Layout& layout,
                              SimulationView simulation_view,
                              const SimulationLayers& layers) {
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    draw_logic_items_base(ctx, layout, layers.items_below, simulation_view);
    draw_wires(ctx, layout, layers.wires, simulation_view);
    draw_logic_items_base(ctx, layout, layers.items_above, simulation_view);

    draw_logic_items_connectors(ctx, layout, layers.items_below, simulation_view);
    draw_logic_items_connectors(ctx, layout, layers.items_above, simulation_view);
};

//
// Layers
//

auto add_valid_wire_parts(const layout::ConstElement wire,
                          std::vector<ordered_line_t>& output) -> bool {
    auto found = false;

    const auto& tree = wire.segment_tree();

    const auto& all_parts = tree.valid_parts();
    const auto begin = all_parts.begin();
    const auto end = all_parts.end();

    for (auto it = begin; it != end; ++it) {
        if (it->empty()) {
            continue;
        }
        const auto index =
            segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(it - begin)};
        const auto full_line = tree.segment_line(index);

        for (const auto& part : *it) {
            output.push_back(to_line(full_line, part));
            found = true;
        }
    }

    return found;
}

auto add_selected_wire_parts(const layout::ConstElement wire, const Selection& selection,
                             std::vector<ordered_line_t>& output) -> void {
    const auto& tree = wire.segment_tree();

    for (const auto segment : tree.indices(wire.element_id())) {
        const auto parts = selection.selected_segments(segment);

        if (parts.empty()) {
            continue;
        }

        const auto full_line = tree.segment_line(segment.segment_index);

        for (const auto part : parts) {
            output.push_back(to_line(full_line, part));
        }
    }
}

auto insert_logic_item(InteractiveLayers& layers, element_id_t element_id,
                       ElementType element_type, rect_t bounding_rect,
                       ElementDrawState state) -> void {
    if (is_inserted(state)) {
        if (draw_logic_item_above(element_type)) {
            layers.normal_above.push_back({element_id, state});
        } else {
            layers.normal_below.push_back({element_id, state});
        }
    } else {
        update_uninserted_rect(layers, bounding_rect);

        if (draw_logic_item_above(element_type)) {
            layers.uninserted_above.push_back({element_id, state});
        } else {
            layers.uninserted_below.push_back({element_id, state});
        }
    }

    if (has_overlay(state)) {
        update_overlay_rect(layers, bounding_rect);
    }

    switch (state) {
        using enum ElementDrawState;
        case normal:
        case simulated:
            break;

        case normal_selected:
        case temporary_selected:
            layers.selected_logic_items.push_back(element_id);
            break;
        case valid:
            layers.valid_logic_items.push_back(element_id);
            break;
        case colliding:
            layers.colliding_logic_items.push_back(element_id);
            break;
    }
}

auto build_interactive_layers(const Layout& layout, InteractiveLayers& layers,
                              const Selection* selection, rect_t scene_rect) -> void {
    layers.clear();

    for (const auto element : layout.elements()) {
        // visibility
        const auto bounding_rect = element.bounding_rect();
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }
        const auto element_type = element.element_type();

        if (is_logic_item(element_type)) {
            const auto state = get_logic_item_state(element, selection);
            insert_logic_item(layers, element, element_type, bounding_rect, state);
        }

        else if (element_type == ElementType::wire) {
            const auto display_state = element.display_state();

            if (is_inserted(display_state)) {
                layers.normal_wires.push_back(element);

                // TODO add: tree.has_valid_parts()
                const auto found_valid =
                    add_valid_wire_parts(element, layers.valid_wires);

                if (!found_valid && selection != nullptr) {
                    add_selected_wire_parts(element, *selection, layers.selected_wires);
                }
            } else {
                // fine grained check, as uninserted trees can contain a lot of
                // segments
                for (const auto& info : element.segment_tree().segment_infos()) {
                    if (is_colliding(info.line, scene_rect)) {
                        // layers.uninserted_wires.push_back(info);
                        update_uninserted_rect(layers, info.line);

                        if (display_state == display_state_t::colliding) {
                            layers.colliding_wires.push_back(info);
                        } else if (display_state == display_state_t::temporary) {
                            layers.temporary_wires.push_back(info);
                        }
                    }
                }
            }
        }
    }

    layers.calculate_overlay_bounding_rect();
}

auto build_simulation_layers(const Layout& layout, SimulationLayers& layers,
                             rect_t scene_rect) -> void {
    layers.clear();

    for (const auto element : layout.elements()) {
        // visibility
        if (!is_colliding(element.bounding_rect(), scene_rect)) {
            continue;
        }
        const auto element_type = element.element_type();

        if (is_logic_item(element_type)) {
            if (element.display_state() == display_state_t::normal) {
                if (draw_logic_item_above(element_type)) {
                    layers.items_above.push_back(element);
                } else {
                    layers.items_below.push_back(element);
                }
            }
        }

        else if (element_type == ElementType::wire) {
            if (element.display_state() == display_state_t::normal) {
                layers.wires.push_back(element);
            }
        }
    }
}

//
// File Rendering
//

namespace {

// render_function = [](CircuitContext &ctx){ ... }
template <typename Func>
auto render_circuit_to_file(int width, int height, std::string filename,
                            const ViewConfig& view_config, Func render_function) {
    auto circuit_ctx = CircuitContext {Context {
        .bl_image = BLImage {width, height, BL_FORMAT_PRGB32},
        .settings = RenderSettings {.view_config = view_config},
    }};
    auto& ctx = circuit_ctx.ctx;

    ctx.begin();
    render_background(circuit_ctx.ctx);
    render_function(circuit_ctx);
    ctx.end();

    const auto folder = QFileInfo(QString::fromUtf8(filename)).baseName();
    QDir().mkdir(folder);

    ctx.bl_image.writeToFile(filename.c_str());
}

}  // namespace

//
// Layout
//

auto _render_layout(CircuitContext& circuit_ctx, const Layout& layout,
                    const Selection* selection) -> void {
    const auto scene_rect = get_scene_rect(circuit_ctx.ctx.settings.view_config);
    auto& layers = circuit_ctx.layers.interactive_layers;

    build_interactive_layers(layout, layers, selection, scene_rect);
    render_interactive_layers(circuit_ctx.ctx, layout, layers, circuit_ctx.surfaces);
}

auto render_layout(CircuitContext& circuit_ctx, const Layout& layout) -> void {
    _render_layout(circuit_ctx, layout, nullptr);
}

auto render_layout(CircuitContext& circuit_ctx, const Layout& layout,
                   const Selection& selection) -> void {
    if (selection.empty()) {
        _render_layout(circuit_ctx, layout, nullptr);
    } else {
        _render_layout(circuit_ctx, layout, &selection);
    }
}

auto render_layout_to_file(const Layout& layout, int width, int height,
                           std::string filename, const ViewConfig& view_config) -> void {
    render_circuit_to_file(width, height, filename, view_config,
                           [&](CircuitContext& ctx) { render_layout(ctx, layout); });
}

auto render_layout_to_file(const Layout& layout, const Selection& selection, int width,
                           int height, std::string filename,
                           const ViewConfig& view_config) -> void {
    render_circuit_to_file(
        width, height, filename, view_config,
        [&](CircuitContext& ctx) { render_layout(ctx, layout, selection); });
}

//
// Simulation
//

auto render_simulation(CircuitContext& circuit_ctx, const Layout& layout,
                       SimulationView simulation_view) -> void {
    const auto scene_rect = get_scene_rect(circuit_ctx.ctx.settings.view_config);
    auto& layers = circuit_ctx.layers.simulation_layers;

    build_simulation_layers(layout, layers, scene_rect);
    render_simulation_layers(circuit_ctx.ctx, layout, simulation_view, layers);
}

auto render_simulation_to_file(const Layout& layout, SimulationView simulation_view,
                               int width, int height, std::string filename,
                               const ViewConfig& view_config) -> void {
    render_circuit_to_file(width, height, filename, view_config,
                           [&](CircuitContext& circuit_ctx) {
                               render_simulation(circuit_ctx, layout, simulation_view);
                           });
}
}  // namespace logicsim