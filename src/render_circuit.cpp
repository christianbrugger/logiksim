﻿#include "render_circuit.h"

#include "algorithm/range.h"
#include "algorithm/round.h"
#include "algorithm/u8_conversion.h"
#include "allocated_size/std_vector.h"
#include "allocated_size/trait.h"
#include "component/simulation/history_view.h"
#include "concept/input_range.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/grid.h"
#include "geometry/interpolation.h"
#include "geometry/layout_calculation.h"
#include "geometry/orientation.h"
#include "geometry/rect.h"
#include "geometry/scene.h"
#include "layout.h"
#include "layout_info.h"
#include "line_tree.h"
#include "logging.h"
#include "logic_item/layout_display_ascii.h"
#include "logic_item/layout_display_number.h"
#include "render_circuit.h"
#include "selection.h"
#include "setting_handle.h"
#include "simulation.h"
#include "simulation_view.h"
#include "size_handle.h"
#include "timer.h"  // TODO remove
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/length.h"
#include "vocabulary/logicitem_id.h"

#include <blend2d.h>
#include <fmt/format.h>  // TODO why?
#include <gsl/gsl>

#include <locale>
#include <numbers>

namespace logicsim {

//
// Background
//
namespace {

auto draw_grid_space_limit(Context& ctx) {
    constexpr auto stroke_color = defaults::color_gray;
    constexpr auto stroke_width = grid_fine_t {5.0};

    const auto stroke_width_px = std::max(5.0, to_context(stroke_width, ctx));

    const auto p0 = to_context(point_t {grid_t::min(), grid_t::min()}, ctx);
    const auto p1 = to_context(point_t {grid_t::max(), grid_t::max()}, ctx);

    ctx.bl_ctx.setStrokeWidth(stroke_width_px);
    ctx.bl_ctx.strokeRect(BLRect {p0.x + 0.5, p0.y + 0.5, p1.x - p0.x, p1.y - p0.y},
                          stroke_color);
}

constexpr auto monochrome(uint8_t value) -> color_t {
    return color_t {value, value, value, 255};
}

auto draw_background_pattern_checker(Context& ctx, rect_fine_t scene_rect, int delta,
                                     color_t color, int width) {
    const auto g0 = point_t {
        to_floored(floor(scene_rect.p0.x / delta) * delta),
        to_floored(floor(scene_rect.p0.y / delta) * delta),
    };
    const auto g1 = point_t {
        to_ceiled(ceil(scene_rect.p1.x / delta) * delta),
        to_ceiled(ceil(scene_rect.p1.y / delta) * delta),
    };

    /*
    for (int x = int {g0.x}; x <= int {g1.x}; x += delta) {
        const auto x_grid = grid_t {x};
        draw_line(ctx, line_t {{x_grid, g0.y}, {x_grid, g1.y}}, {color, width}, config);
    }
    for (int y = int {g0.y}; y <= int {g1.y}; y += delta) {
        const auto y_grid = grid_t {y};
        draw_line(ctx, line_t {{g0.x, y_grid}, {g1.x, y_grid}}, {color, width}, config);
    }
    */

    // this version is a bit faster
    const auto p0 = to_context(g0, ctx);
    const auto p1 = to_context(g1, ctx);

    const auto offset = ctx.view_config().offset();
    const auto scale = ctx.view_config().pixel_scale();

    // vertical
    for (int x = int {g0.x}; x <= int {g1.x}; x += delta) {
        const auto cx = round_fast(double {(grid_fine_t {x} + offset.x) * scale});
        draw_orthogonal_line(ctx, BLLine {cx, p0.y, cx, p1.y}, {color, width});
    }
    // horizontal
    for (int y = int {g0.y}; y <= int {g1.y}; y += delta) {
        const auto cy = round_fast(double {(grid_fine_t {y} + offset.y) * scale});
        draw_orthogonal_line(ctx, BLLine {p0.x, cy, p1.x, cy}, {color, width});
    }
}

auto draw_background_patterns(Context& ctx) {
    auto scene_rect = get_scene_rect_fine(ctx.view_config());

    constexpr static auto grid_definition = {
        std::tuple {1, monochrome(0xF0), 1},    //
        std::tuple {8, monochrome(0xE4), 1},    //
        std::tuple {64, monochrome(0xE4), 2},   //
        std::tuple {512, monochrome(0xD8), 2},  //
        std::tuple {4096, monochrome(0xC0), 2},
    };

    for (auto&& [delta, color, width] : grid_definition) {
        if (delta * ctx.view_config().device_scale() >=
            ctx.settings.background_grid_min_distance_device) {
            const auto draw_width_f = width * ctx.view_config().device_pixel_ratio();
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
    const auto width = ctx.view_config().stroke_width();
    const auto offset = stroke_offset(width);

    const auto r = to_context_unrounded(radius, ctx);
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

auto draw_logic_item_connectors(Context& ctx, const Layout& layout,
                                logicitem_id_t logicitem_id, ElementDrawState state)
    -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        draw_connector(ctx, ConnectorAttributes {
                                .state = state,
                                .position = info.position,
                                .orientation = info.orientation,
                                .is_inverted = layout.logic_items().input_inverted(
                                    logicitem_id, info.input_id),
                                .is_enabled = false,
                            });
    }

    for (auto info : output_locations_and_id(layout_data)) {
        draw_connector(ctx, ConnectorAttributes {
                                .state = state,
                                .position = info.position,
                                .orientation = info.orientation,
                                .is_inverted = layout.logic_items().output_inverted(
                                    logicitem_id, info.output_id),
                                .is_enabled = false,
                            });
    }
}

auto draw_logic_item_connectors(Context& ctx, const Layout& layout,
                                logicitem_id_t logicitem_id, ElementDrawState state,
                                simulation_view::ConstElement logic_state) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        const auto is_inverted =
            layout.logic_items().input_inverted(logicitem_id, info.input_id);

        if (is_inverted || !logic_state.has_connected_input(info.input_id)) {
            draw_connector(ctx, ConnectorAttributes {
                                    .state = state,
                                    .position = info.position,
                                    .orientation = info.orientation,
                                    .is_inverted = is_inverted,
                                    .is_enabled = logic_state.input_value(info.input_id),
                                });
        }
    }

    for (auto info : output_locations_and_id(layout_data)) {
        const auto is_inverted =
            layout.logic_items().output_inverted(logicitem_id, info.output_id);

        if (is_inverted || !logic_state.has_connected_output(info.output_id)) {
            draw_connector(
                ctx, ConnectorAttributes {
                         .state = state,
                         .position = info.position,
                         .orientation = info.orientation,
                         .is_inverted = is_inverted,
                         .is_enabled = logic_state.output_value(info.output_id).value(),
                     });
        }
    }
}

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const DrawableElement> elements) -> void {
    if (do_draw_connector(ctx.view_config())) {
        for (const auto entry : elements) {
            draw_logic_item_connectors(ctx, layout, entry.logicitem_id, entry.state);
        }
    }
}

auto draw_logic_items_connectors(Context& ctx, const Layout& layout,
                                 std::span<const logicitem_id_t> elements,
                                 SimulationView simulation_view) -> void {
    if (do_draw_connector(ctx.view_config())) {
        for (const auto logicitem_id : elements) {
            const auto state = ElementDrawState::normal;
            draw_logic_item_connectors(ctx, layout, logicitem_id, state,
                                       simulation_view.element(logicitem_id));
        }
    }
}

auto connector_horizontal_alignment(orientation_t orientation) -> HTextAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return HTextAlignment::right;
        case left:
            return HTextAlignment::left;
        case up:
            return HTextAlignment::center;
        case down:
            return HTextAlignment::center;

        default:
            throw_exception("orientation has no horizontal alignment");
    };
}

auto connector_vertical_alignment(orientation_t orientation) -> VTextAlignment {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return VTextAlignment::center;
        case left:
            return VTextAlignment::center;
        case up:
            return VTextAlignment::top;
        case down:
            return VTextAlignment::baseline;

        default:
            throw_exception("orienation has no vertical alignment");
    };
}

auto draw_connector_label(Context& ctx, point_t position, orientation_t orientation,
                          std::string_view label, ElementDrawState state) -> void {
    const auto point = !label.empty() && label.at(0) == '>'
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

auto draw_connector_labels(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ConnectorLabels labels,
                           ElementDrawState state) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (auto info : input_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             labels.input_labels[std::size_t {info.input_id}], state);
    }

    for (auto info : output_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             labels.output_labels[std::size_t {info.output_id}], state);
    }
}

template <typename Func>
auto draw_input_connector_labels(Context& ctx, const Layout& layout,
                                 logicitem_id_t logicitem_id, ElementDrawState state,
                                 Func to_input_label) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);

    for (const auto&& info : input_locations_and_id(layout_data)) {
        draw_connector_label(ctx, info.position, info.orientation,
                             to_input_label(info.input_id), state);
    }
}

//
// Logic Items Body
//

auto draw_logic_item_above(LogicItemType type) -> bool {
    using enum LogicItemType;
    return type == button || type == led;
}

auto get_logic_item_state(const Layout& layout, logicitem_id_t logicitem_id,
                          const Selection* selection) -> ElementDrawState {
    const auto is_selected = [&]() {
        return (selection != nullptr) ? selection->is_selected(logicitem_id) : false;
    };

    const auto display_state = layout.logic_items().display_state(logicitem_id);

    if (is_inserted(display_state)) {
        if (display_state == display_state_t::valid) {
            return ElementDrawState::valid;
        }
        if (is_selected()) {
            return ElementDrawState::normal_selected;
        }
        return ElementDrawState::normal;
    }

    if (display_state == display_state_t::colliding) {
        return ElementDrawState::colliding;
    }
    if (is_selected()) {
        return ElementDrawState::temporary_selected;
    }
    throw_exception("cannot draw temporary items");
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

auto draw_logic_item_rect(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state, LogicItemRectAttributes attributes)
    -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_body_draw_rect(layout_data);
    draw_logic_item_rect(ctx, rect, state, attributes);
}

auto draw_logic_item_rect(Context& ctx, rect_fine_t rect, ElementDrawState state,
                          LogicItemRectAttributes attributes)

    -> void {
    const auto fill_color = attributes.custom_fill_color
                                ? with_alpha_runtime(*attributes.custom_fill_color, state)
                                : get_logic_item_fill_color(state);
    const auto stroke_color =
        attributes.custom_stroke_color
            ? with_alpha_runtime(*attributes.custom_stroke_color, state)
            : get_logic_item_stroke_color(state);

    draw_rect(ctx, rect,
              RectAttributes {
                  .draw_type = ShapeDrawType::fill_and_stroke,
                  .fill_color = fill_color,
                  .stroke_color = stroke_color,
              });
}

auto get_logic_item_center(const Layout& layout, logicitem_id_t logicitem_id)
    -> point_fine_t {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_body_draw_rect(layout_data);
    return get_center(rect);
}

auto draw_logic_item_label(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, std::string_view text,
                           ElementDrawState state, LogicItemTextAttributes attributes)
    -> void {
    const auto center = get_logic_item_center(layout, logicitem_id);
    draw_logic_item_label(ctx, center, text, state, attributes);
}

auto draw_logic_item_label(Context& ctx, point_fine_t center, std::string_view text,
                           ElementDrawState state, LogicItemTextAttributes attributes)
    -> void {
    if (text.empty()) {
        return;
    }

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
                  .cutoff_size_px = defaults::font::text_cutoff_px,
              });
}

auto draw_binary_value(Context& ctx, point_fine_t point, bool is_enabled,
                       ElementDrawState state) -> void {
    const auto text = is_enabled ? std::string_view {"1"} : std::string_view {"0"};
    draw_logic_item_label(ctx, point, text, state,
                          LogicItemTextAttributes {
                              .custom_font_size = defaults::font::binary_value_size,
                          });
}

auto draw_binary_false(Context& ctx, point_fine_t point, ElementDrawState state) -> void {
    const auto is_enabled = false;
    draw_binary_value(ctx, point, is_enabled, state);
}

//
// Individual Elements
//

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
            throw_exception("element type has no standard label");
    }
}

auto draw_standard_element(Context& ctx, const Layout& layout,
                           logicitem_id_t logicitem_id, ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);
    const auto type = layout.logic_items().type(logicitem_id);
    draw_logic_item_label(ctx, layout, logicitem_id, standard_element_label(type), state);
}

auto draw_button(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                 ElementDrawState state,

                 std::optional<simulation_view::ConstElement> logic_state) -> void {
    const auto logic_value = logic_state ? logic_state->internal_state(0) : false;
    const auto center = get_logic_item_center(layout, logicitem_id);

    draw_logic_item_rect(ctx, layout, logicitem_id, state,
                         {.custom_fill_color = defaults::button_body_color});
    draw_binary_value(ctx, center, logic_value, state);
}

auto draw_led(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
              ElementDrawState state,

              std::optional<simulation_view::ConstElement> logic_state) -> void {
    const auto logic_value =
        logic_state ? logic_state->input_value(connection_id_t {0}) : false;

    const auto base_color =
        logic_value ? defaults::led_color_enabled : defaults::led_color_disabled;

    const auto position = layout.logic_items().position(logicitem_id);

    draw_circle(ctx, point_fine_t {position}, grid_fine_t {defaults::led_radius},
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

static_assert(display_number::max_value_inputs <=
              connection_count_t {power_of_two_labels.size()});
static_assert(display_ascii::value_inputs <=
              connection_count_t {power_of_two_labels.size()});

auto _is_display_enabled(const Layout& layout, logicitem_id_t logicitem_id,
                         std::optional<simulation_view::ConstElement> logic_state)
    -> bool {
    const auto input_id = display::enable_input_id;

    if (!logic_state) {
        return true;
    }

    const auto inverted = layout.logic_items().input_inverted(logicitem_id, input_id);
    return logic_state->input_value(input_id) ^ inverted;
}

auto _is_display_twos_complement(const Layout& layout, logicitem_id_t logicitem_id,
                                 std::optional<simulation_view::ConstElement> logic_state)
    -> bool {
    const auto input_id = display_number::negative_input_id;
    const auto inverted = layout.logic_items().input_inverted(logicitem_id, input_id);

    if (!logic_state) {
        return inverted;
    }

    return logic_state->input_value(input_id) ^ inverted;
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
                       simulation_view::ConstElement logic_state,
                       const connection_count_t control_inputs) -> uint64_t {
    const auto& values = logic_state.input_values();
    const auto& inverters = layout.logic_items().input_inverters(logicitem_id);

    if (values.size() - std::size_t {control_inputs} > std::size_t {64}) {
        throw_exception("input size too large");
    }

    auto number = uint64_t {0};
    for (const auto& i : range(std::size_t {control_inputs}, values.size())) {
        const auto value = values.at(i) ^ inverters.at(i);
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
                          std::optional<simulation_view::ConstElement> logic_state) {
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
    if (logic_state) {
        if (_is_display_enabled(layout, logicitem_id, logic_state)) {
            auto number =
                _inputs_to_number(layout, logicitem_id, *logic_state, control_inputs);
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
        throw_exception("too many digits");
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
                         std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    const auto input_count = layout.logic_items().input_count(logicitem_id);
    // TODO remove
    const auto element_width = grid_fine_t {display_number::width(input_count)};
    const auto element_height = grid_fine_t {display_number::height(input_count)};

    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    const auto two_complement =
        _is_display_twos_complement(layout, logicitem_id, logic_state);
    const auto edit_mode_text = "0";
    const auto control_inputs = display_number::control_inputs;
    const auto value_inputs = display_number::value_inputs(input_count);
    const auto to_text =
        _number_value_to_text(two_complement, std::size_t {value_inputs});
    _draw_number_display(ctx, layout, logicitem_id, state, element_width, element_height,
                         to_text, edit_mode_text, control_inputs, logic_state);
    _draw_number_display_input_labels(ctx, layout, logicitem_id, state, two_complement);
}

namespace {
auto _asci_value_to_text(uint64_t number) -> styled_display_text_t {
    constexpr auto vertical_alignment = VTextAlignment::center_baseline;

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

auto draw_display_ascii(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                        ElementDrawState state,
                        std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    // TODO remove
    const auto element_width = grid_fine_t {display_ascii::width};
    const auto element_height = grid_fine_t {display_ascii::height};

    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    const auto edit_mode_text = "A";
    const auto control_inputs = display_ascii::control_inputs;
    _draw_number_display(ctx, layout, logicitem_id, state, element_width, element_height,
                         _asci_value_to_text, edit_mode_text, control_inputs,
                         logic_state);
    _draw_ascii_display_input_labels(ctx, layout, logicitem_id, state);
}

auto draw_buffer(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                 ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);
    draw_logic_item_label(ctx, layout, logicitem_id, "1", state,
                          {.custom_font_size = defaults::font::buffer_label_size});
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

auto draw_flipflop_jk(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                      ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<5> {"> C", "J", "K", "S", "R"};
    static constexpr auto output_labels = string_array<2> {"Q", "Q\u0305"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_shift_register(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                         ElementDrawState state,
                         std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    // content
    const auto output_count =
        std::size_t {layout.logic_items().output_count(logicitem_id)};
    const auto state_size = std::size_t {10};

    const auto position = layout.logic_items().position(logicitem_id);
    for (auto n : range(output_count, state_size)) {
        const auto point = point_fine_t {
            -1 + 2.0 * static_cast<double>(n / output_count),
            0.25 + 1.5 * static_cast<double>(n % output_count),
        };
        const auto logic_value = logic_state ? logic_state->internal_state(n) : false;
        draw_binary_value(ctx, position + point, logic_value, state);
    }

    // labels
    static constexpr auto input_labels = string_array<3> {">", "", ""};
    static constexpr auto output_labels = string_array<2> {"", ""};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_latch_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                  ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<2> {"E", "D"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_flipflop_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                     ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

auto draw_flipflop_ms_d(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                        ElementDrawState state) -> void {
    draw_logic_item_rect(ctx, layout, logicitem_id, state);

    static constexpr auto input_labels = string_array<4> {"> C", "D", "S", "R"};
    static constexpr auto output_labels = string_array<1> {"Q"};
    draw_connector_labels(ctx, layout, logicitem_id,
                          ConnectorLabels {input_labels, output_labels}, state);
}

//
// All Elements
//

auto draw_logic_item_base(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          ElementDrawState state,
                          std::optional<simulation_view::ConstElement> logic_state)
    -> void {
    switch (layout.logic_items().type(logicitem_id)) {
        using enum LogicItemType;

        case buffer_element:
            return draw_buffer(ctx, layout, logicitem_id, state);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, layout, logicitem_id, state);

        case button:
            return draw_button(ctx, layout, logicitem_id, state, logic_state);
        case led:
            return draw_led(ctx, layout, logicitem_id, state, logic_state);
        case display_number:
            return draw_display_number(ctx, layout, logicitem_id, state, logic_state);
        case display_ascii:
            return draw_display_ascii(ctx, layout, logicitem_id, state, logic_state);

        case clock_generator:
            return draw_clock_generator(ctx, layout, logicitem_id, state);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, layout, logicitem_id, state);
        case shift_register:
            return draw_shift_register(ctx, layout, logicitem_id, state, logic_state);
        case latch_d:
            return draw_latch_d(ctx, layout, logicitem_id, state);
        case flipflop_d:
            return draw_flipflop_d(ctx, layout, logicitem_id, state);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, layout, logicitem_id, state);

        case sub_circuit:
            return draw_standard_element(ctx, layout, logicitem_id, state);
    }
    throw_exception("not supported");
}

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const DrawableElement> elements) -> void {
    for (const auto& entry : elements) {
        draw_logic_item_base(ctx, layout, entry.logicitem_id, entry.state);
    }
}

auto draw_logic_items_base(Context& ctx, const Layout& layout,
                           std::span<const logicitem_id_t> elements,
                           SimulationView simulation_view) -> void {
    const auto state = ElementDrawState::normal;

    for (const auto& logicitem_id : elements) {
        draw_logic_item_base(ctx, layout, logicitem_id, state,
                             simulation_view.element(logicitem_id));
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
    int lc_width = ctx.view_config().line_cross_width();
    if (lc_width <= 0) {
        return;
    }

    const int wire_width = ctx.view_config().stroke_width();
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

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       bool is_enabled, ElementDrawState state) -> void {
    for (const segment_info_t& info : layout.wires().segment_tree(wire_id)) {
        draw_line_segment(ctx, info, is_enabled, state);
    }
}

auto draw_segment_tree(Context& ctx, const Layout& layout, wire_id_t wire_id,
                       ElementDrawState state) -> void {
    bool is_enabled = false;
    draw_segment_tree(ctx, layout, wire_id, is_enabled, state);
}

auto _draw_line_segment_with_history(Context& ctx, point_t p_from, point_t p_until,
                                     time_t time_from, time_t time_until,
                                     const simulation::HistoryView& history,
                                     bool p0_is_corner, bool p1_is_corner) -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.first_time);
        const auto p_end =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.last_time);

        if (p_start != p_end) [[likely]] {
            draw_line_segment(
                ctx, line_fine_t {p_start, p_end},
                {
                    .is_enabled = entry.value,
                    .p0_endcap = p0_is_corner && (p_start == point_fine_t {p_from}),
                    .p1_endcap = p1_is_corner && (p_end == point_fine_t {p_until}),
                },
                ElementDrawState::normal);
        }
    }
}

auto _draw_wire_with_history(Context& ctx, const Layout& layout [[maybe_unused]],
                             wire_id_t wire_id [[maybe_unused]],
                             simulation_view::ConstElement logic_state,
                             const simulation::HistoryView& history) -> void {
    if (history.size() < 2) [[unlikely]] {
        throw_exception("requires history view with at least 2 entries");
    }

    // TODO access length_.value ?
    const auto to_time = [time = logic_state.time(),
                          delay = logic_state.wire_delay_per_distance()](
                             length_t length_) { return time - length_.value * delay; };
    const auto& line_tree = logic_state.line_tree();

    for (auto&& index : indices(line_tree)) {
        const auto line = line_tree.line(index);
        _draw_line_segment_with_history(ctx,                                  //
                                        line.p1,                              //
                                        line.p0,                              //
                                        to_time(line_tree.length_p1(index)),  //
                                        to_time(line_tree.length_p0(index)),  //
                                        history,                              //
                                        line_tree.is_corner_p1(index),        //
                                        line_tree.is_corner_p0(index)         //
        );

        if (line_tree.has_cross_point_p0(index)) {
            bool wire_enabled = history.value(to_time(line_tree.length_p0(index)));
            draw_line_cross_point(ctx, line.p0, wire_enabled, ElementDrawState::normal);
        }
    }
}

auto draw_wire(Context& ctx, const Layout& layout, wire_id_t wire_id,
               simulation_view::ConstElement logic_state) -> void {
    const auto history = logic_state.input_history();

    if (history.size() <= 1) {
        draw_segment_tree(ctx, layout, wire_id, history.last_value(),
                          ElementDrawState::normal);
        return;
    }

    _draw_wire_with_history(ctx, layout, wire_id, logic_state, history);
}

//
//
//

/*
auto draw_wires(Context& ctx, const Layout& layout,
                std::span<const DrawableElement> elements) -> void {
    for (const auto entry : elements) {
        draw_segment_tree(ctx, layout, entry.wire, entry.state);
    }
}
*/

auto draw_wires(Context& ctx, const Layout& layout, std::span<const wire_id_t> elements,
                ElementDrawState state) -> void {
    for (const auto& wire_id : elements) {
        draw_segment_tree(ctx, layout, wire_id, state);
    }
}

auto draw_wires(Context& ctx, const Layout& layout, std::span<const wire_id_t> elements,
                SimulationView simulation_view) -> void {
    for (const auto& wire_id : elements) {
        draw_wire(ctx, layout, wire_id, simulation_view.element(wire_id));
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
    auto stroke_width = std::max(1., round_fast(attributes.stroke_width_device *
                                                ctx.view_config().device_pixel_ratio()));

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
    auto rect = size_handle_rect_px(position, ctx.view_config());

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
    const auto rect = setting_handle_rect(handle);
    const auto icon_height =
        defaults::setting_handle_size * defaults::setting_handle_icon_scale;

    // button rect
    draw_rect(ctx, rect,
              RectAttributes {
                  .draw_type = ShapeDrawType::fill_and_stroke,
                  .fill_color = defaults::setting_handle_color_fill,
                  .stroke_color = defaults::setting_handle_color_stroke,
              });

    // button icon
    draw_icon(ctx, get_center(rect), handle.icon,
              IconAttributes {
                  .icon_height = icon_height,
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

auto element_shadow_rounding(LogicItemType type) -> grid_fine_t {
    return type == LogicItemType::button ? grid_fine_t {0.} : line_selection_padding();
}

auto draw_logic_item_shadow(Context& ctx, const Layout& layout,
                            logicitem_id_t logicitem_id, shadow_t shadow_type) -> void {
    const auto layout_data = to_layout_calculation_data(layout, logicitem_id);
    const auto rect = element_shadow_rect(layout_data);

    draw_round_rect(ctx, rect,
                    {
                        .draw_type = ShapeDrawType::fill,
                        .rounding = element_shadow_rounding(layout_data.logicitem_type),
                        .fill_color = shadow_color(shadow_type),
                    });
}

auto draw_logic_item_shadows(Context& ctx, const Layout& layout,
                             std::span<const logicitem_id_t> elements,
                             shadow_t shadow_type) -> void {
    for (const auto& logicitem_id : elements) {
        draw_logic_item_shadow(ctx, layout, logicitem_id, shadow_type);
    }
}

template <input_range_of<ordered_line_t> View>
auto draw_wire_shadows_impl(Context& ctx, View lines, shadow_t shadow_type) -> void {
    const auto color = shadow_color(shadow_type);

    for (const ordered_line_t line : lines) {
        const auto selection_rect = element_shadow_rect(line);
        draw_round_rect(ctx, selection_rect,
                        {
                            .draw_type = ShapeDrawType::fill,
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

auto InteractiveLayers::size() const -> std::size_t {
    return normal_below.size() +           //
           normal_wires.size() +           //
           normal_above.size() +           //
                                           //
           uninserted_below.size() +       //
           uninserted_above.size() +       //
                                           //
           selected_logic_items.size() +   //
           selected_wires.size() +         //
           temporary_wires.size() +        //
           valid_logic_items.size() +      //
           valid_wires.size() +            //
           colliding_logic_items.size() +  //
           colliding_wires.size();         //
}

auto InteractiveLayers::empty() const -> bool {
    return this->size() == std::size_t {0};
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

auto SimulationLayers::allocated_size() const -> std::size_t {
    return get_allocated_size(items_below) +  //
           get_allocated_size(wires) +        //
           get_allocated_size(items_above);
}

auto SimulationLayers::size() const -> std::size_t {
    return items_below.size() +  //
           wires.size() +        //
           items_above.size();   //
}

auto SimulationLayers::empty() const -> bool {
    return this->size() == std::size_t {0};
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
                               const InteractiveLayers& layers, ImageSurface& surface)
    -> void {
    if (layers.has_inserted()) {
        render_inserted(ctx, layout, layers);
    }

    const auto layer_enabled = true;

    if (layers.uninserted_bounding_rect.has_value()) {
        const auto rect =
            get_dirty_rect(layers.uninserted_bounding_rect.value(), ctx.view_config());

        render_layer(ctx, surface, rect, [&](Context& layer_ctx) {
            render_uninserted(layer_ctx, layout, layers, layer_enabled);
        });
    }

    if (layers.overlay_bounding_rect.has_value()) {
        const auto rect =
            get_dirty_rect(layers.overlay_bounding_rect.value(), ctx.view_config());

        render_layer(ctx, surface, rect, [&](Context& layer_ctx) {
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

auto add_valid_wire_parts(const Layout& layout, wire_id_t wire_id,
                          std::vector<ordered_line_t>& output) -> bool {
    auto found = false;

    const auto& tree = layout.wires().segment_tree(wire_id);

    for (const segment_index_t& index : tree.indices()) {
        for (const ordered_line_t& valid_line : all_valid_lines(tree, index)) {
            output.push_back(valid_line);
            found = true;
        }
    }

    return found;
}

auto add_selected_wire_parts(const Layout& layout, wire_id_t wire_id,
                             const Selection& selection,
                             std::vector<ordered_line_t>& output) -> void {
    const auto& tree = layout.wires().segment_tree(wire_id);

    for (const auto segment : tree.indices(wire_id)) {
        const auto& parts = selection.selected_segments(segment);

        if (parts.empty()) {
            continue;
        }

        const auto full_line = tree.line(segment.segment_index);

        for (const auto part : parts) {
            output.push_back(to_line(full_line, part));
        }
    }
}

auto insert_logic_item(InteractiveLayers& layers, const Layout& layout,
                       logicitem_id_t logicitem_id, rect_t bounding_rect,
                       ElementDrawState state) -> void {
    const auto logicitem_type = layout.logic_items().type(logicitem_id);

    if (is_inserted(state)) {
        if (draw_logic_item_above(logicitem_type)) {
            layers.normal_above.push_back({logicitem_id, state});
        } else {
            layers.normal_below.push_back({logicitem_id, state});
        }
    } else {
        update_uninserted_rect(layers, bounding_rect);

        if (draw_logic_item_above(logicitem_type)) {
            layers.uninserted_above.push_back({logicitem_id, state});
        } else {
            layers.uninserted_below.push_back({logicitem_id, state});
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
            layers.selected_logic_items.push_back(logicitem_id);
            break;
        case valid:
            layers.valid_logic_items.push_back(logicitem_id);
            break;
        case colliding:
            layers.colliding_logic_items.push_back(logicitem_id);
            break;
    }
}

auto build_interactive_layers(const Layout& layout, const Selection* selection,
                              rect_t scene_rect) -> InteractiveLayers {
    auto layers = InteractiveLayers {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.logic_items().bounding_rect(logicitem_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        const auto state = get_logic_item_state(layout, logicitem_id, selection);
        insert_logic_item(layers, layout, logicitem_id, bounding_rect, state);
    }

    for (const auto wire_id : inserted_wire_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.wires().bounding_rect(wire_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        layers.normal_wires.push_back(wire_id);

        // TODO add: tree.has_valid_parts()
        const auto found_valid =
            add_valid_wire_parts(layout, wire_id, layers.valid_wires);

        if (!found_valid && selection != nullptr) {
            add_selected_wire_parts(layout, wire_id, *selection, layers.selected_wires);
        }
    }
    // fine grained check, as uninserted trees can contain a lot of segments
    for (const auto& info : layout.wires().segment_tree(temporary_wire_id)) {
        if (is_colliding(info.line, scene_rect)) {
            update_uninserted_rect(layers, info.line);
            layers.temporary_wires.push_back(info);
        }
    }
    for (const auto& info : layout.wires().segment_tree(colliding_wire_id)) {
        if (is_colliding(info.line, scene_rect)) {
            update_uninserted_rect(layers, info.line);
            layers.colliding_wires.push_back(info);
        }
    }

    layers.calculate_overlay_bounding_rect();

    // const auto t = Timer {fmt::format("build_interactive_layers {} items, {:.3f} kb",
    //                                   layers.size(), layers.allocated_size() / 1024.)};

    return layers;
}

auto build_simulation_layers(const Layout& layout, rect_t scene_rect)
    -> SimulationLayers {
    // const auto t = Timer {};

    auto layers = SimulationLayers {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.logic_items().bounding_rect(logicitem_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        if (layout.logic_items().display_state(logicitem_id) == display_state_t::normal) {
            const auto type = layout.logic_items().type(logicitem_id);
            if (draw_logic_item_above(type)) {
                layers.items_above.push_back(logicitem_id);
            } else {
                layers.items_below.push_back(logicitem_id);
            }
        }
    }

    for (const auto wire_id : inserted_wire_ids(layout)) {
        // visibility
        const auto bounding_rect = layout.wires().bounding_rect(wire_id);
        if (!is_colliding(bounding_rect, scene_rect)) {
            continue;
        }

        layers.wires.push_back(wire_id);
    }

    // fmt::format("build_simulation_layers {} items, {:.3f} kb: {}",
    //                                   layers.size(), layers.allocated_size() / 1024.,
    //                                   t);

    return layers;
}

//
// File Rendering
//

namespace {

template <std::invocable<Context&> Func>
auto render_circuit_to_file(BLSizeI size, const std::filesystem::path& filename,
                            const ViewConfig& view_config, const ContextCache& cache,
                            Func render_function) {
    auto bl_image = BLImage {size.w, size.h, BL_FORMAT_PRGB32};

    // TODO !!! generate settings differently
    auto settings = ContextRenderSettings {.view_config = view_config};
    settings.view_config.set_size(bl_image.size());

    render_to_image(bl_image, settings, cache, [&render_function](Context& ctx) {
        std::invoke(render_function, ctx);
    });

    std::filesystem::create_directories(filename.parent_path());
    bl_image.writeToFile(to_string(filename.u8string()).c_str());
}

}  // namespace

//
// Layout
//

auto _render_layout(Context& ctx, ImageSurface& surface, const Layout& layout,
                    const Selection* selection) -> void {
    const auto scene_rect = get_scene_rect(ctx.settings.view_config);
    const auto layers = build_interactive_layers(layout, selection, scene_rect);

    render_interactive_layers(ctx, layout, layers, surface);
}

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout) -> void {
    _render_layout(ctx, surface, layout, nullptr);
}

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout,
                   const Selection& selection) -> void {
    if (selection.empty()) {
        _render_layout(ctx, surface, layout, nullptr);
    } else {
        _render_layout(ctx, surface, layout, &selection);
    }
}

auto render_layout_to_file(const Layout& layout, BLSizeI size,
                           const std::filesystem::path& filename,
                           const ViewConfig& view_config, const ContextCache& cache)
    -> void {
    auto surface = ImageSurface {};

    render_circuit_to_file(size, filename, view_config, cache, [&](Context& ctx) {
        render_background(ctx);
        render_layout(ctx, surface, layout);
    });
}

auto render_layout_to_file(const Layout& layout, const Selection& selection, BLSizeI size,
                           const std::filesystem::path& filename,
                           const ViewConfig& view_config, const ContextCache& cache)
    -> void {
    auto surface = ImageSurface {};

    render_circuit_to_file(size, filename, view_config, cache, [&](Context& ctx) {
        render_background(ctx);
        render_layout(ctx, surface, layout, selection);
    });
}

//
// Simulation
//

auto render_simulation(Context& ctx, const Layout& layout, SimulationView simulation_view)
    -> void {
    const auto scene_rect = get_scene_rect(ctx.view_config());
    const auto layers = build_simulation_layers(layout, scene_rect);

    render_simulation_layers(ctx, layout, simulation_view, layers);
}

auto render_simulation_to_file(const Layout& layout, SimulationView simulation_view,
                               BLSizeI size, const std::filesystem::path& filename,
                               const ViewConfig& view_config, const ContextCache& cache)
    -> void {
    render_circuit_to_file(size, filename, view_config, cache, [&](Context& ctx) {
        render_background(ctx);
        render_simulation(ctx, layout, simulation_view);
    });
}
}  // namespace logicsim