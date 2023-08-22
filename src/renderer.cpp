
#include "renderer.h"

#include "algorithm.h"
#include "collision.h"
#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
#include "format.h"
#include "geometry.h"
#include "layout.h"
#include "layout_calculation.h"
#include "range.h"
#include "timer.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <numbers>
#include <numeric>
#include <utility>

namespace logicsim {

auto draw_line_cross_point(BLContext& ctx, const point_t point, bool is_enabled,
                           const RenderSettings& settings) -> void {
    int lc_width = settings.view_config.line_cross_width();
    if (lc_width <= 0) {
        return;
    }

    const int wire_width = settings.view_config.stroke_width();
    const int wire_offset = (wire_width - 1) / 2;

    const int size = 2 * lc_width + wire_width;
    const int offset = wire_offset + lc_width;

    const auto [x, y] = to_context(point, settings.view_config);
    const auto color = is_enabled ? defaults::color_red : defaults::color_black;

    ctx.setFillStyle(color);
    ctx.fillRect(x - offset, y - offset, size, size);
}

auto draw_line_segment(BLContext& ctx, line_fine_t line, bool is_enabled,
                       const RenderSettings& settings) -> void {
    const auto color = is_enabled ? defaults::color_red : defaults::color_black;
    draw_line(ctx, line, {.color = color}, settings);
}

auto draw_line_segment(BLContext& ctx, line_t line, bool is_enabled,
                       const RenderSettings& settings) -> void {
    draw_line_segment(ctx, line_fine_t {line}, is_enabled, settings);

    draw_point(ctx, line.p0, PointShape::circle, defaults::color_orange, 0.2, settings);
    draw_point(ctx, line.p1, PointShape::cross, defaults::color_orange, 0.2, settings);
}

auto draw_line_segment(BLContext& ctx, point_t p_from, point_t p_until, time_t time_from,
                       time_t time_until, const simulation::HistoryView& history,
                       const RenderSettings& settings) -> void {
    assert(time_from < time_until);

    const auto it_from = history.from(time_from);
    const auto it_until = history.until(time_until);

    for (const auto& entry : std::ranges::subrange(it_from, it_until)) {
        const auto p_start =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.first_time);
        const auto p_end =
            interpolate_line_1d(p_from, p_until, time_from, time_until, entry.last_time);

        // TODO !!! why do we need this? fix history.from, history.until?
        if (p_start != p_end) {
            draw_line_segment(ctx, line_fine_t {p_start, p_end}, entry.value, settings);
        }
    }
}

auto draw_wire_no_history(BLContext& ctx, layout::ConstElement element, bool wire_enabled,
                          const RenderSettings& settings) -> void {
    for (auto&& segment : element.line_tree().sized_segments()) {
        draw_line_segment(ctx, segment.line, wire_enabled, settings);

        if (segment.has_cross_point_p0) {
            draw_line_cross_point(ctx, segment.line.p0, wire_enabled, settings);
        }
    }
}

auto draw_wire_with_history(BLContext& ctx, Schematic::ConstElement element,
                            const Layout& layout, const Simulation& simulation,
                            const RenderSettings& settings) -> void {
    const auto to_time = [time = simulation.time(),
                          delay = element.schematic().wire_delay_per_distance()](
                             LineTree::length_t length_) {
        return time_t {time.value - static_cast<int64_t>(length_) * delay.value};
    };

    const auto history = simulation.input_history(element);

    for (auto&& segment : layout.line_tree(element).sized_segments()) {
        draw_line_segment(ctx, segment.line.p1, segment.line.p0,
                          to_time(segment.p1_length), to_time(segment.p0_length), history,
                          settings);

        if (segment.has_cross_point_p0) {
            bool wire_enabled = history.value(to_time(segment.p0_length));
            draw_line_cross_point(ctx, segment.line.p0, wire_enabled, settings);
        }
    }
}

auto draw_wire(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
               const Simulation& simulation, const RenderSettings& settings) -> void {
    if (element.history_length() == delay_t {0ns}) {
        auto enabled = simulation.input_value(element.input(connection_id_t {0})) ^
                       element.input_inverters().at(0);
        draw_wire_no_history(ctx, layout.element(element), enabled, settings);
    } else {
        draw_wire_with_history(ctx, element, layout, simulation, settings);
    }
}

auto draw_segment_tree(BLContext& ctx, layout::ConstElement element,
                       const RenderSettings& settings) -> void {
    for (const segment_info_t& segment : element.segment_tree().segment_infos()) {
        draw_line_segment(ctx, segment.line, false, settings);

        if (is_cross_point(segment.p0_type)) {
            draw_line_cross_point(ctx, segment.line.p0, false, settings);
        }
        if (is_cross_point(segment.p1_type)) {
            draw_line_cross_point(ctx, segment.line.p1, false, settings);
        }
    }
}

auto get_alpha_value(display_state_t display_state) -> uint8_t {
    switch (display_state) {
        using enum display_state_t;

        case normal:
        case valid:
            return 0xFF;
        case colliding:
            return 0x40;
        case temporary:
            return 0x80;
    }
    throw_exception("unknown display state");
}

auto set_body_draw_styles(BLContext& ctx, display_state_t display_state, bool selected) {
    const auto alpha = get_alpha_value(display_state);

    const auto fill_color = [&] {
        if (display_state == display_state_t::normal) {
            if (selected) {
                return BLRgba32(224, 224, 224, alpha);
            }
            return BLRgba32(255, 255, 128, alpha);
        }
        return BLRgba32(192, 192, 192, alpha);
    }();

    ctx.setFillStyle(fill_color);
    ctx.setStrokeStyle(BLRgba32(0, 0, 0, alpha));
}

auto draw_connector_inverted(BLContext& ctx, point_t position, orientation_t orientation,
                             bool enabled, display_state_t display_state,
                             const RenderSettings& settings) {
    const auto radius = 0.2;

    const auto alpha = get_alpha_value(display_state);
    const auto color = enabled ? BLRgba32(255, 0, 0, alpha) : BLRgba32(0, 0, 0, alpha);
    const auto width = settings.view_config.stroke_width();
    const auto offset = stroke_offset(width);

    const auto r = radius * settings.view_config.pixel_scale();
    const auto p = to_context(position, settings.view_config);
    const auto p_center = connector_point(p, orientation, r + width);

    ctx.setFillStyle(BLRgba32(defaults::color_white.value));
    ctx.fillCircle(BLCircle {p_center.x + offset, p_center.y + offset, r});

    ctx.setStrokeStyle(color);
    ctx.setStrokeWidth(width);
    ctx.strokeCircle(BLCircle {p_center.x + offset, p_center.y + offset, r});
}

auto draw_connector_normal(BLContext& ctx, point_t position, orientation_t orientation,
                           bool enabled, display_state_t display_state,
                           const RenderSettings& settings) -> void {
    const auto endpoint = connector_endpoint(position, orientation);

    const auto alpha = get_alpha_value(display_state);
    const auto color_bl = enabled ? BLRgba32(255, 0, 0, alpha) : BLRgba32(0, 0, 0, alpha);
    const auto color = color_t {color_bl.value};

    draw_line(ctx, line_fine_t {position, endpoint}, {color}, settings);
}

auto draw_connector(BLContext& ctx, point_t position, orientation_t orientation,
                    bool enabled, bool inverted, display_state_t display_state,
                    const RenderSettings& settings) -> void {
    if (inverted) {
        draw_connector_inverted(ctx, position, orientation, enabled, display_state,
                                settings);
    } else {
        draw_connector_normal(ctx, position, orientation, enabled, display_state,
                              settings);
    }
}

auto draw_logic_item_connectors(BLContext& ctx, layout::ConstElement element,
                                const RenderSettings& settings) -> void {
    const auto layout_data = to_layout_calculation_data(element.layout(), element);
    const auto display_state = element.display_state();

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            const auto inverted = element.input_inverted(input_id);
            draw_connector(ctx, position, orientation, false, inverted, display_state,
                           settings);
            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            const auto inverted = element.output_inverted(output_id);
            draw_connector(ctx, position, orientation, false, inverted, display_state,
                           settings);
            return true;
        });
}

auto draw_logic_item_connectors(BLContext& ctx, Schematic::ConstElement element,
                                const Layout& layout, const Simulation& simulation,
                                const RenderSettings& settings) -> void {
    const auto layout_data = to_layout_calculation_data(layout, element.element_id());

    iter_input_location_and_id(
        layout_data,
        [&](connection_id_t input_id, point_t position, orientation_t orientation) {
            const auto inverted = layout.element(element).input_inverted(input_id);
            const auto input = element.input(input_id);

            if (inverted || !input.has_connected_element()) {
                const auto enabled = simulation.input_value(input);
                draw_connector(ctx, position, orientation, enabled, inverted,
                               display_state_t::normal, settings);
            }

            return true;
        });

    iter_output_location_and_id(
        layout_data,
        [&](connection_id_t output_id, point_t position, orientation_t orientation) {
            const auto inverted = layout.element(element).output_inverted(output_id);
            const auto output = element.output(output_id);

            if (inverted || !output.has_connected_element() ||
                output.connected_element().is_placeholder()) {
                const auto enabled = simulation.output_value(output);
                draw_connector(ctx, position, orientation, enabled, inverted,
                               display_state_t::normal, settings);
            }
            return true;
        });
}

constexpr static double BODY_OVERDRAW = 0.4;    // grid values
constexpr static double BUTTON_OVERDRAW = 0.5;  // grid values

auto to_label(layout::ConstElement element) -> std::string {
    switch (element.element_type()) {
        using enum ElementType;

        case and_element:
            return "&";
        case or_element:
            return ">1";
        case xor_element:
            return "=1";

        default:
            return "";
    }
}

auto draw_standard_element_body(BLContext& ctx, layout::ConstElement element,
                                bool selected, const RenderSettings& settings) -> void {
    const auto position = element.position();
    const auto element_height = std::max(element.input_count(), element.output_count());

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value * 1.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 2.0,
            position.y.value + BODY_OVERDRAW + element_height - 1,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    // text
    const auto label = to_label(element);
    const auto size = 0.9 * settings.view_config.pixel_scale();

    if (!label.empty() && size > 3.0) {
        const auto center = point_fine_t {
            position.x.value + 1.0,
            position.y.value + (element_height - 1.0) / 2.0,
        };

        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(center, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_standard_element(BLContext& ctx, layout::ConstElement element, bool selected,
                           const RenderSettings& settings) -> void {
    draw_standard_element_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_standard_element(BLContext& ctx, Schematic::ConstElement element,
                           const Layout& layout, const Simulation& simulation,
                           bool selected, const RenderSettings& settings) -> void {
    draw_standard_element_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// Button
//

auto draw_button_body(BLContext& ctx, layout::ConstElement element, bool selected,
                      const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value - BUTTON_OVERDRAW,
            position.y.value - BUTTON_OVERDRAW,
        },
        point_fine_t {
            position.x.value + BUTTON_OVERDRAW,
            position.y.value + BUTTON_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);

    const auto alpha = get_alpha_value(element.display_state());
    ctx.setFillStyle(BLRgba32(229, 229, 229, alpha));

    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);
}

auto draw_binary_value(BLContext& ctx, point_fine_t position, bool enabled,
                       display_state_t display_state, const RenderSettings& settings) {
    const auto label = enabled ? std::string {"1"} : std::string {"0"};
    const auto size = 0.7 * settings.view_config.pixel_scale();

    if (size > 3.0) {
        const auto alpha = get_alpha_value(display_state);
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(position, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_binary_value(BLContext& ctx, point_t position, bool enabled,
                       display_state_t display_state, const RenderSettings& settings) {
    draw_binary_value(ctx, point_fine_t {position}, enabled, display_state, settings);
}

auto draw_button(BLContext& ctx, layout::ConstElement element, bool selected,
                 const RenderSettings& settings) -> void {
    draw_button_body(ctx, element, selected, settings);
    draw_binary_value(ctx, element.position(), false, element.display_state(), settings);
}

auto draw_button(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
                 const Simulation& simulation, bool selected,
                 const RenderSettings& settings) -> void {
    bool enabled = simulation.internal_state(element).at(0);
    draw_button_body(ctx, layout.element(element), selected, settings);
    draw_binary_value(ctx, layout.position(element), enabled, display_state_t::normal,
                      settings);
}

//
// Buffer Element
//

auto draw_buffer_body(BLContext& ctx, layout::ConstElement element, bool selected,
                      const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 1.0,
            position.y.value + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    const auto size = 0.6 * settings.view_config.pixel_scale();
    if (size > 3.0) {
        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        const auto p = point_fine_t {position.x.value + 0.5, position.y.value + 0.0};
        settings.text.draw_text(ctx, to_context(p, settings.view_config), "1", size,
                                color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_buffer(BLContext& ctx, layout::ConstElement element, bool selected,
                 const RenderSettings& settings) -> void {
    draw_buffer_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_buffer(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
                 const Simulation& simulation, bool selected,
                 const RenderSettings& settings) -> void {
    draw_buffer_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// Clock Generator
//

auto draw_clock_generator_body(BLContext& ctx, layout::ConstElement element,
                               bool selected, const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 3.0,
            position.y.value + 2.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);
}

auto draw_clock_generator(BLContext& ctx, layout::ConstElement element, bool selected,
                          const RenderSettings& settings) -> void {
    draw_clock_generator_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_clock_generator(BLContext& ctx, Schematic::ConstElement element,
                          const Layout& layout, const Simulation& simulation,
                          bool selected, const RenderSettings& settings) -> void {
    draw_clock_generator_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// JK-FlipFlop
//

auto draw_flipflop_jk_body(BLContext& ctx, layout::ConstElement element, bool selected,
                           const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 4.0,
            position.y.value + 2.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    // text
    const auto label = std::string {"JK-FF"};
    const auto size = 0.9 * settings.view_config.pixel_scale();

    if (size > 3.0) {
        const auto center = point_fine_t {
            position.x.value + 2.0,
            position.y.value + 1.0,
        };

        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(center, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_flipflop_jk(BLContext& ctx, layout::ConstElement element, bool selected,
                      const RenderSettings& settings) -> void {
    draw_flipflop_jk_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_flipflop_jk(BLContext& ctx, Schematic::ConstElement element,
                      const Layout& layout, const Simulation& simulation, bool selected,
                      const RenderSettings& settings) -> void {
    draw_flipflop_jk_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// Shift Register
//

auto draw_shift_register_body(BLContext& ctx, layout::ConstElement element, bool selected,
                              const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 8.0,
            position.y.value + 2.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);
}

auto draw_shift_register_state(BLContext& ctx, layout::ConstElement element,
                               const RenderSettings& settings) {
    const auto position = element.position();
    const auto display_state = element.display_state();

    const auto output_count = element.output_count();
    const auto state_size = std::size_t {10};

    for (auto n : range(output_count, state_size)) {
        const auto x = -1 + 2.0 * (n / output_count);
        const auto y = 0.25 + 1.5 * (n % output_count);

        const auto p = point_fine_t {position.x.value + x, position.y.value + y};

        draw_binary_value(ctx, p, false, display_state, settings);
    }
}

auto draw_shift_register_state(BLContext& ctx, Schematic::ConstElement element,
                               const Layout& layout, const Simulation& simulation,
                               const RenderSettings& settings) {
    const auto position = layout.position(element);
    const auto display_state = layout.display_state(element);

    const auto output_count = layout.output_count(element);
    const auto& state = simulation.internal_state(element);

    for (auto n : range(output_count, state.size())) {
        const auto x = -1 + 2.0 * (n / output_count);
        const auto y = 0.25 + 1.5 * (n % output_count);

        const auto p = point_fine_t {position.x.value + x, position.y.value + y};

        draw_binary_value(ctx, p, state.at(n), display_state, settings);
    }
}

auto draw_shift_register(BLContext& ctx, layout::ConstElement element, bool selected,
                         const RenderSettings& settings) -> void {
    draw_shift_register_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
    draw_shift_register_state(ctx, element, settings);
}

auto draw_shift_register(BLContext& ctx, Schematic::ConstElement element,
                         const Layout& layout, const Simulation& simulation,
                         bool selected, const RenderSettings& settings) -> void {
    draw_shift_register_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
    draw_shift_register_state(ctx, element, layout, simulation, settings);
}

//
// D-Latch
//

auto draw_latch_d_body(BLContext& ctx, layout::ConstElement element, bool selected,
                       const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 2.0,
            position.y.value + 1.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    // text
    const auto label = std::string {"L"};
    const auto size = 0.9 * settings.view_config.pixel_scale();

    if (size > 3.0) {
        const auto center = point_fine_t {
            position.x.value + 1.0,
            position.y.value + 0.5,
        };

        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(center, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_latch_d(BLContext& ctx, layout::ConstElement element, bool selected,
                  const RenderSettings& settings) -> void {
    draw_latch_d_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_latch_d(BLContext& ctx, Schematic::ConstElement element, const Layout& layout,
                  const Simulation& simulation, bool selected,
                  const RenderSettings& settings) -> void {
    draw_latch_d_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// D-FlipFlop
//

auto draw_flipflop_d_body(BLContext& ctx, layout::ConstElement element, bool selected,
                          const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 3.0,
            position.y.value + 2.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    // text
    const auto label = std::string {"FF"};
    const auto size = 0.9 * settings.view_config.pixel_scale();

    if (size > 3.0) {
        const auto center = point_fine_t {
            position.x.value + 1.5,
            position.y.value + 1.0,
        };

        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(center, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_flipflop_d(BLContext& ctx, layout::ConstElement element, bool selected,
                     const RenderSettings& settings) -> void {
    draw_flipflop_d_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_flipflop_d(BLContext& ctx, Schematic::ConstElement element,
                     const Layout& layout, const Simulation& simulation, bool selected,
                     const RenderSettings& settings) -> void {
    draw_flipflop_d_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// MS-D-FlipFlop
//

auto draw_flipflop_ms_d_body(BLContext& ctx, layout::ConstElement element, bool selected,
                             const RenderSettings& settings) -> void {
    const auto position = element.position();

    const auto rect = rect_fine_t {
        point_fine_t {
            position.x.value + 0.0,
            position.y.value - BODY_OVERDRAW,
        },
        point_fine_t {
            position.x.value + 4.0,
            position.y.value + 2.0 + BODY_OVERDRAW,
        },
    };

    set_body_draw_styles(ctx, element.display_state(), selected);
    draw_rect(ctx, rect, {.draw_type = DrawType::fill_and_stroke}, settings);

    // text
    const auto label = std::string {"MS-FF"};
    const auto size = 0.9 * settings.view_config.pixel_scale();

    if (size > 3.0) {
        const auto center = point_fine_t {
            position.x.value + 2.0,
            position.y.value + 1.0,
        };

        const auto alpha = get_alpha_value(element.display_state());
        const auto color = color_t {0, 0, 0, alpha};

        settings.text.draw_text(ctx, to_context(center, settings.view_config), label,
                                size, color, HorizontalAlignment::center,
                                VerticalAlignment::center);
    }
}

auto draw_flipflop_ms_d(BLContext& ctx, layout::ConstElement element, bool selected,
                        const RenderSettings& settings) -> void {
    draw_flipflop_ms_d_body(ctx, element, selected, settings);
    draw_logic_item_connectors(ctx, element, settings);
}

auto draw_flipflop_ms_d(BLContext& ctx, Schematic::ConstElement element,
                        const Layout& layout, const Simulation& simulation, bool selected,
                        const RenderSettings& settings) -> void {
    draw_flipflop_ms_d_body(ctx, layout.element(element), selected, settings);
    draw_logic_item_connectors(ctx, element, layout, simulation, settings);
}

//
// Logic Item
//

auto draw_logic_item(BLContext& ctx, layout::ConstElement element, bool selected,
                     const RenderSettings& settings) -> void {
    switch (element.element_type()) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            throw_exception("not supported");

        case buffer_element:
            return draw_buffer(ctx, element, selected, settings);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, element, selected, settings);

        case button:
            return draw_button(ctx, element, selected, settings);
        case led:
        case display_number:
        case display_ascii:
            return;

        case clock_generator:
            return draw_clock_generator(ctx, element, selected, settings);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, element, selected, settings);
        case shift_register:
            return draw_shift_register(ctx, element, selected, settings);
        case latch_d:
            return draw_latch_d(ctx, element, selected, settings);
        case flipflop_d:
            return draw_flipflop_d(ctx, element, selected, settings);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, element, selected, settings);

        case sub_circuit:
            return draw_standard_element(ctx, element, selected, settings);
    }
    throw_exception("not supported");
}

auto draw_logic_item(BLContext& ctx, Schematic::ConstElement element,
                     const Layout& layout, const Simulation& simulation, bool selected,
                     const RenderSettings& settings) -> void {
    switch (element.element_type()) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            throw_exception("not supported");

        case buffer_element:
            return draw_buffer(ctx, element, layout, simulation, selected, settings);

        case and_element:
        case or_element:
        case xor_element:
            return draw_standard_element(ctx, element, layout, simulation, selected,
                                         settings);

        case button:
            return draw_button(ctx, element, layout, simulation, selected, settings);
        case led:
        case display_number:
        case display_ascii:
            return;

        case clock_generator:
            return draw_clock_generator(ctx, element, layout, simulation, selected,
                                        settings);
        case flipflop_jk:
            return draw_flipflop_jk(ctx, element, layout, simulation, selected, settings);
        case shift_register:
            return draw_shift_register(ctx, element, layout, simulation, selected,
                                       settings);
        case latch_d:
            return draw_latch_d(ctx, element, layout, simulation, selected, settings);
        case flipflop_d:
            return draw_flipflop_d(ctx, element, layout, simulation, selected, settings);
        case flipflop_ms_d:
            return draw_flipflop_ms_d(ctx, element, layout, simulation, selected,
                                      settings);

        case sub_circuit:
            return draw_standard_element(ctx, element, layout, simulation, selected,
                                         settings);
    }
    throw_exception("not supported");
}

auto draw_element_shadow(BLContext& ctx, layout::ConstElement element, bool selected,
                         const RenderSettings& settings) -> void {
    if (!element.is_logic_item()) {
        return;
    }

    const auto display_state = element.display_state();

    if (display_state == display_state_t::normal && !selected) {
        return;
    }

    const auto data = to_layout_calculation_data(element.layout(), element);
    const auto selection_rect = element_selection_rect(data);

    if (display_state == display_state_t::normal && selected) {
        ctx.setFillStyle(BLRgba32(0, 128, 255, 96));
    } else if (display_state == display_state_t::colliding) {
        ctx.setFillStyle(BLRgba32(255, 0, 0, 96));
    } else if (display_state == display_state_t::valid) {
        ctx.setFillStyle(BLRgba32(0, 192, 0, 96));
    } else if (display_state == display_state_t::temporary) {
        ctx.setFillStyle(BLRgba32(0, 128, 255, 96));
    } else {
        throw_exception("unknown state");
    }

    draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
}

auto draw_wire_selected_parts_shadow(BLContext& ctx, ordered_line_t line,
                                     std::span<const part_t> parts,
                                     const RenderSettings& settings) -> void {
    for (auto&& part : parts) {
        const auto selected_line = to_line(line, part);
        const auto selection_rect = element_selection_rect(selected_line);

        ctx.setFillStyle(BLRgba32(0, 128, 255, 96));
        draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
    }
}

auto draw_wire_temporary_shadow(BLContext& ctx, const SegmentTree& segment_tree,
                                const RenderSettings& settings) {
    ctx.setFillStyle(BLRgba32(0, 128, 255, 96));

    for (const auto info : segment_tree.segment_infos()) {
        const auto selection_rect = element_selection_rect(info.line);
        draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
    }
}

auto draw_wire_colliding_shadow(BLContext& ctx, const SegmentTree& segment_tree,
                                const RenderSettings& settings) -> void {
    ctx.setFillStyle(BLRgba32(255, 0, 0, 96));

    for (const auto info : segment_tree.segment_infos()) {
        const auto selection_rect = element_selection_rect(info.line);
        draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
    }
}

auto draw_wire_valid_shadow(BLContext& ctx, const SegmentTree& segment_tree,
                            const RenderSettings& settings) {
    ctx.setFillStyle(BLRgba32(0, 192, 0, 96));

    for (const auto index : segment_tree.indices()) {
        const auto& parts = segment_tree.valid_parts(index);
        if (parts.empty()) {
            continue;
        }
        const auto full_line = segment_tree.segment_line(index);

        for (const auto part : parts) {
            const auto line = to_line(full_line, part);
            const auto selection_rect = element_selection_rect(line);
            draw_rect(ctx, selection_rect, {.draw_type = DrawType::fill}, settings);
        }
    }
}

auto draw_wire_shadows(BLContext& ctx, const Layout& layout, const Selection& selection,
                       const visibility_mask_t& visibility,
                       const RenderSettings& settings) {
    const auto is_visible = [&](element_id_t element_id) {
        return visibility.at(element_id.value);
    };

    for (const auto element : layout.elements()) {
        if (!element.is_wire() || !is_visible(element)) {
            continue;
        }

        const auto display_state = element.display_state();
        const auto& segment_tree = element.segment_tree();

        if (display_state == display_state_t::temporary) {
            draw_wire_temporary_shadow(ctx, segment_tree, settings);
        }

        else if (display_state == display_state_t::normal) {
            draw_wire_valid_shadow(ctx, segment_tree, settings);
        }

        else if (display_state == display_state_t::colliding) {
            draw_wire_colliding_shadow(ctx, segment_tree, settings);
        }
    }

    for (auto&& [segment, parts] : selection.selected_segments()) {
        if (is_visible(segment.element_id) &&
            layout.display_state(segment.element_id) == display_state_t::normal) {
            const auto line = get_line(layout, segment);
            draw_wire_selected_parts_shadow(ctx, line, parts, settings);
        }
    }
}

auto render_circuit(BLContext& ctx, render_args_t args) -> void {
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);

    auto visibility = visibility_mask_t(args.layout.element_count(), false);
    auto scene_rect = get_scene_rect(args.settings.view_config);

    {
        for (const auto element : args.layout.elements()) {
            const auto index = element.element_id().value;
            visibility.at(index) = is_colliding(element.bounding_rect(), scene_rect);
        }
    }

    const auto& selection = args.selection != nullptr ? *args.selection : Selection {};
    const auto selection_mask = create_selection_mask(args.layout, selection);
    const auto is_selected = [&](element_id_t element_id) {
        const auto id = element_id.value;
        return id < std::ssize(selection_mask) ? selection_mask[id] : false;
    };
    const auto is_visible = [&](element_id_t element_id) {
        return visibility.at(element_id.value);
    };

    // wires
    if (args.simulation == nullptr || args.schematic == nullptr) {
        for (auto element : args.layout.elements()) {
            if (element.element_type() == ElementType::wire && element.is_inserted() &&
                is_visible(element)) {
                draw_segment_tree(ctx, element, args.settings);
            }
        }
    } else {
        for (auto element : args.schematic->elements()) {
            if (element.element_type() == ElementType::wire && is_visible(element)) {
                if (element.input_count() == 0) {
                    draw_segment_tree(ctx, args.layout.element(element), args.settings);
                } else {
                    draw_wire(ctx, element, args.layout, *args.simulation, args.settings);
                }
            }
        }
    }

    // unselected elements
    if (args.simulation == nullptr || args.schematic == nullptr) {
        for (auto element : args.layout.elements()) {
            if (!is_selected(element) && element.is_logic_item() && is_visible(element)) {
                draw_logic_item(ctx, element, false, args.settings);
            }
        }
    } else {
        for (auto element : args.schematic->elements()) {
            if (!is_selected(element) && element.is_logic_item() && is_visible(element)) {
                draw_logic_item(ctx, element, args.layout, *args.simulation, false,
                                args.settings);
            }
        }
    }

    // draw uninserted wires
    if (args.simulation == nullptr || args.schematic == nullptr) {
        for (auto element : args.layout.elements()) {
            if (!element.is_inserted() && element.element_type() == ElementType::wire &&
                is_visible(element)) {
                draw_segment_tree(ctx, element, args.settings);
            }
        }
    }

    // selected elements
    if (args.simulation == nullptr || args.schematic == nullptr) {
        for (auto element : args.layout.elements()) {
            if (is_selected(element) && element.is_logic_item() && is_visible(element)) {
                draw_logic_item(ctx, element, true, args.settings);
            }
        }
    } else {
        for (auto element : args.schematic->elements()) {
            if (is_selected(element) && element.is_logic_item() && is_visible(element)) {
                draw_logic_item(ctx, element, args.layout, *args.simulation, true,
                                args.settings);
            }
        }
    }

    // shadow
    for (auto element : args.layout.elements()) {
        if (is_visible(element)) {
            bool selected = is_selected(element);
            draw_element_shadow(ctx, element, selected, args.settings);
        }
    }

    // wire shadow
    draw_wire_shadows(ctx, args.layout, selection, visibility, args.settings);
}

auto render_circuit(render_args_t args, int width, int height, std::string filename)
    -> void {
    auto img = BLImage {width, height, BL_FORMAT_PRGB32};
    auto ctx = BLContext {img};

    render_background(ctx, args.settings);
    render_circuit(ctx, args);

    ctx.end();

    std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
    img.writeToFile(filename.c_str());
}

//
// Background
//

auto draw_grid_space_limit(BLContext& ctx, const RenderSettings& settings) {
    const auto p0 =
        to_context(point_t {grid_t::min(), grid_t::min()}, settings.view_config);
    const auto p1 =
        to_context(point_t {grid_t::max(), grid_t::max()}, settings.view_config);

    ctx.setStrokeStyle(BLRgba32(0xFF808080u));
    ctx.setStrokeWidth(std::max(5.0, to_context(5.0, settings.view_config)));
    ctx.strokeRect(p0.x + 0.5, p0.y + 0.5, p1.x - p0.x, p1.y - p0.y);
}

constexpr auto monochrome(uint8_t value) -> color_t {
    return color_t {0xFF000000u + value * 0x1u + value * 0x100u + value * 0x10000u};
}

auto draw_background_pattern_checker(BLContext& ctx, rect_fine_t scene_rect, int delta,
                                     color_t color, int width,
                                     const RenderSettings& settings) {
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
    const auto p0 = to_context(g0, settings.view_config);
    const auto p1 = to_context(g1, settings.view_config);

    const auto offset = settings.view_config.offset();
    const auto scale = settings.view_config.pixel_scale();

    // vertical
    for (int x = g0.x.value; x <= g1.x.value; x += delta) {
        const auto cx = round_fast((x + offset.x) * scale);
        draw_orthogonal_line(ctx, BLLine {cx, p0.y, cx, p1.y}, {color, width}, settings);
    }
    // horizontal
    for (int y = g0.y.value; y <= g1.y.value; y += delta) {
        const auto cy = round_fast((y + offset.y) * scale);
        draw_orthogonal_line(ctx, BLLine {p0.x, cy, p1.x, cy}, {color, width}, settings);
    }
}

auto draw_background_patterns(BLContext& ctx, const RenderSettings& settings) {
    auto scene_rect = get_scene_rect_fine(settings.view_config);

    constexpr static auto grid_definition = {
        std::tuple {1, monochrome(0xF0), 1},    //
        std::tuple {8, monochrome(0xE4), 1},    //
        std::tuple {64, monochrome(0xE4), 2},   //
        std::tuple {512, monochrome(0xD8), 2},  //
        std::tuple {4096, monochrome(0xC0), 2},
    };

    for (auto&& [delta, color, width] : grid_definition) {
        if (delta * settings.view_config.device_scale() >=
            settings.background_grid_min_distance) {
            const auto draw_width_f = width * settings.view_config.device_pixel_ratio();
            // we substract a little, as we want 150% scaling to round down
            const auto epsilon = 0.01;
            const auto draw_width = std::max(1, round_to<int>(draw_width_f - epsilon));
            draw_background_pattern_checker(ctx, scene_rect, delta, color, draw_width,
                                            settings);
        }
    }
}

auto render_background(BLContext& ctx, const RenderSettings& settings) -> void {
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.setFillStyle(BLRgba32(defaults::color_white.value));
    ctx.fillAll();

    draw_background_patterns(ctx, settings);
    draw_grid_space_limit(ctx, settings);
}

auto create_selection_mask(const Layout& layout, const Selection& selection)
    -> selection_mask_t {
    if (selection.selected_logic_items().empty()) {
        return {};
    }

    auto mask = selection_mask_t(layout.element_count(), false);
    for (element_id_t element_id : selection.selected_logic_items()) {
        mask.at(element_id.value) = true;
    }

    return mask;
}

}  // namespace logicsim
