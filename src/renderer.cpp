
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

auto draw_line_segment(BLContext& ctx, point_t p_from, point_t p_until, time_t time_from,
                       time_t time_until, const Simulation::HistoryView& history,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));
        settings.text.draw_text(ctx, to_context(center, settings.view_config), size,
                                label, HorizontalAlignment::center,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));

        settings.text.draw_text(ctx, to_context(position, settings.view_config), size,
                                label, HorizontalAlignment::center,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));

        const auto p = point_fine_t {position.x.value + 0.5, position.y.value + 0.0};
        settings.text.draw_text(ctx, to_context(p, settings.view_config), size, "1",
                                HorizontalAlignment::center, VerticalAlignment::center);
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));
        settings.text.draw_text(ctx, to_context(center, settings.view_config), size,
                                label, HorizontalAlignment::center,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));
        settings.text.draw_text(ctx, to_context(center, settings.view_config), size,
                                label, HorizontalAlignment::center,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));
        settings.text.draw_text(ctx, to_context(center, settings.view_config), size,
                                label, HorizontalAlignment::center,
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
        ctx.setFillStyle(BLRgba32(0, 0, 0, alpha));
        settings.text.draw_text(ctx, to_context(center, settings.view_config), size,
                                label, HorizontalAlignment::center,
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

//
// Caches
//

auto render_input_marker(BLContext& ctx, point_t point, color_t color,
                         orientation_t orientation, double size,
                         const RenderSettings& settings) -> void {
    auto _ = ContextGuard {ctx};

    ctx.setStrokeWidth(1);
    ctx.setStrokeStyle(BLRgba32(color.value));

    const auto [x, y] = to_context(point, settings.view_config);
    const auto d = to_context(size, settings.view_config);
    const auto angle = to_angle(orientation);

    ctx.translate(BLPoint {x, y});
    ctx.rotate(angle);

    const auto pi = std::numbers::pi;

    ctx.strokeArc(BLArc {0, 0, d, d, -pi / 2, pi});
    ctx.strokeLine(BLLine {-d, -d, 0, -d});
    ctx.strokeLine(BLLine {-d, +d, 0, +d});
}

auto render_undirected_output(BLContext& ctx, point_t position, double size,
                              const RenderSettings& settings) {
    draw_point(ctx, position, PointShape::cross, defaults::color_green, size / 4,
               settings);
    draw_point(ctx, position, PointShape::plus, defaults::color_green, size / 3,
               settings);
}

auto render_editable_circuit_connection_cache(BLContext& ctx,
                                              const EditableCircuit& editable_circuit,
                                              const RenderSettings& settings) -> void {
    const auto scene_rect = get_scene_rect(settings.view_config);
    const auto& caches = editable_circuit.caches();

    for (auto [position, orientation] : caches.input_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = 1.0 / 3.0;
        render_input_marker(ctx, position, defaults::color_green, orientation, size,
                            settings);
    }

    for (auto [position, orientation] : caches.output_positions_and_orientations()) {
        if (!is_colliding(position, scene_rect)) {
            continue;
        }

        const auto size = 0.8;
        if (orientation == orientation_t::undirected) {
            render_undirected_output(ctx, position, size, settings);
        } else {
            draw_arrow(ctx, position, defaults::color_green, orientation, size, settings);
        }
    }
}

auto render_editable_circuit_collision_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void {
    constexpr static auto color = defaults::color_orange;
    constexpr static auto size = 0.25;

    const auto scene_rect = get_scene_rect(settings.view_config);

    for (auto [point, state] : editable_circuit.caches().collision_states()) {
        if (!is_colliding(point, scene_rect)) {
            continue;
        }

        switch (state) {
            using enum collision_cache::CacheState;

            case element_body: {
                draw_point(ctx, point, PointShape::square, color, size, settings);
                break;
            }
            case element_connection: {
                draw_point(ctx, point, PointShape::circle, color, size, settings);
                break;
            }
            case wire_connection: {
                draw_point(ctx, point, PointShape::full_square, color, size * (2. / 3),
                           settings);
                break;
            }
            case wire_horizontal: {
                draw_point(ctx, point, PointShape::horizontal, color, size, settings);
                break;
            }
            case wire_vertical: {
                draw_point(ctx, point, PointShape::vertical, color, size, settings);
                break;
            }
            case wire_corner_point: {
                draw_point(ctx, point, PointShape::diamond, color, size, settings);
                break;
            }
            case wire_cross_point: {
                draw_point(ctx, point, PointShape::cross, color, size, settings);
                break;
            }
            case wire_crossing: {
                draw_point(ctx, point, PointShape::plus, color, size, settings);
                break;
            }
            case element_wire_connection: {
                draw_point(ctx, point, PointShape::full_circle, color, size, settings);
                break;
            }
            case invalid_state: {
                throw_exception("invalid state encountered");
                break;
            }
        }
    }
}

auto render_editable_circuit_selection_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void {
    const auto scene_rect = get_scene_rect_fine(settings.view_config);
    ctx.setStrokeStyle(BLRgba32(0, 255, 0));

    for (const rect_fine_t& rect : editable_circuit.caches().selection_rects()) {
        if (!is_colliding(rect, scene_rect)) {
            continue;
        }

        draw_rect(ctx, rect,
                  RectAttributes {.draw_type = DrawType::stroke, .stroke_width = 1},
                  settings);
    }
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

//
// benchmark
//

struct RenderBenchmarkConfig {
    grid_t min_grid {1};
    grid_t max_grid {99};

    grid_t max_segment_length {5};

    std::size_t min_line_segments {1};
    std::size_t max_line_segments {5};

    int n_outputs_min {1};
    int n_outputs_max {5};

    int min_event_spacing_us {5};
    int max_event_spacing_us {30};
};

namespace {

template <typename T>
using UDist = boost::random::uniform_int_distribution<T>;

template <std::uniform_random_bit_generator G>
auto get_udist(grid_t a, grid_t b, G& rng) {
    return [a, b, &rng]() -> grid_t {
        return grid_t {UDist<grid_t::value_type> {a.value, b.value}(rng)};
    };
}

template <std::uniform_random_bit_generator G>
auto random_segment_value(grid_t last, const RenderBenchmarkConfig& config, G& rng) {
    // auto grid_dist = boost::random::uniform_int_distribution<grid_t> {
    //     std::max(config.min_grid, last - config.max_segment_length),
    //     std::min(config.max_grid, last + config.max_segment_length)};

    auto grid_dist =
        get_udist(std::max(config.min_grid, last - config.max_segment_length),
                  std::min(config.max_grid, last + config.max_segment_length), rng);

    grid_t res;
    while ((res = grid_dist()) == last) {
    }
    return res;
}

template <std::uniform_random_bit_generator G>
auto new_line_point(point_t origin, bool horizontal, const RenderBenchmarkConfig& config,
                    G& rng) -> point_t {
    if (horizontal) {
        return point_t {random_segment_value(origin.x, config, rng), origin.y};
    }
    return point_t {origin.x, random_segment_value(origin.y, config, rng)};
}

template <std::uniform_random_bit_generator G>
auto new_line_point(point_t origin, point_t previous, const RenderBenchmarkConfig& config,
                    G& rng) -> point_t {
    return new_line_point(origin, is_vertical(line_t {previous, origin}), config, rng);
}

// pick random point on line
template <std::uniform_random_bit_generator G>
auto pick_line_point(ordered_line_t line, G& rng) -> point_t {
    // return point2d_t {UDist<grid_t> {line.p0.x, line.p1.x}(rng),
    //                   UDist<grid_t> {line.p0.y, line.p1.y}(rng)};
    return point_t {get_udist(line.p0.x, line.p1.x, rng)(),
                    get_udist(line.p0.y, line.p1.y, rng)()};
}

template <std::uniform_random_bit_generator G>
auto create_line_tree_segment(point_t start_point, bool horizontal,
                              const RenderBenchmarkConfig& config, G& rng) -> LineTree {
    auto segment_count_dist =
        UDist<std::size_t> {config.min_line_segments, config.max_line_segments};
    auto n_segments = segment_count_dist(rng);

    auto line_tree = std::optional<LineTree> {};
    do {
        auto points = std::vector<point_t> {
            start_point,
            new_line_point(start_point, horizontal, config, rng),
        };
        std::generate_n(std::back_inserter(points), n_segments - 1, [&]() {
            return new_line_point(points.back(), *(points.end() - 2), config, rng);
        });

        line_tree = LineTree::from_points(points);
    } while (!line_tree.has_value());

    assert(line_tree->segment_count() == n_segments);
    return std::move(line_tree.value());
}

template <std::uniform_random_bit_generator G>
auto create_first_line_tree_segment(const RenderBenchmarkConfig& config, G& rng)
    -> LineTree {
    // const auto grid_dist = UDist<grid_t> {config.min_grid, config.max_grid};
    const auto grid_dist = get_udist(config.min_grid, config.max_grid, rng);
    const auto p0 = point_t {grid_dist(), grid_dist()};

    const auto is_horizontal = UDist<int> {0, 1}(rng);
    return create_line_tree_segment(p0, is_horizontal, config, rng);
}

template <std::uniform_random_bit_generator G>
auto create_random_line_tree(std::size_t n_outputs, const RenderBenchmarkConfig& config,
                             G& rng) -> LineTree {
    auto line_tree = create_first_line_tree_segment(config, rng);

    while (line_tree.output_count() < n_outputs) {
        auto new_tree = std::optional<LineTree> {};
        // TODO flatten loop
        do {
            const auto segment_index =
                UDist<int> {0, gsl::narrow<int>(line_tree.segment_count()) - 1}(rng);
            const auto segment = line_tree.segment(segment_index);
            const auto origin = pick_line_point(ordered_line_t {segment}, rng);

            const auto sub_tree =
                create_line_tree_segment(origin, is_vertical(segment), config, rng);
            new_tree = merge({line_tree, sub_tree});
        } while (!new_tree.has_value());

        line_tree = std::move(new_tree.value());
    }

    return line_tree;
}

auto calculate_tree_length(const LineTree& line_tree) -> int {
    return std::transform_reduce(line_tree.segments().begin(), line_tree.segments().end(),
                                 0, std::plus {},
                                 [](line_t line) { return distance(line); });
}

}  // namespace

auto fill_line_scene(BenchmarkScene& scene, int n_lines) -> int64_t {
    auto rng = boost::random::mt19937 {0};
    const auto config = RenderBenchmarkConfig();
    auto tree_length_sum = int64_t {0};

    // create initial schematics
    auto& schematic = scene.schematic;
    for (auto _ [[maybe_unused]] : range(n_lines)) {
        UDist<int> output_dist {config.n_outputs_min, config.n_outputs_max};
        schematic.add_element(Schematic::ElementData {
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = gsl::narrow<std::size_t>(output_dist(rng)),
        });
    }
    add_output_placeholders(schematic);

    // create layout
    auto& layout = scene.layout = Layout {};
    for (auto element : schematic.elements()) {
        layout.add_element(Layout::ElementData {
            .element_type = element.element_type(),
            .input_count = element.input_count(),
            .output_count = element.output_count(),
        });
    }

    // add line trees
    auto& simulation = scene.simulation = Simulation {schematic};
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto line_tree = create_random_line_tree(element.output_count(), config, rng);

            // delays
            auto lengths = line_tree.calculate_output_lengths();
            assert(lengths.size() == element.output_count());
            auto delays =
                transform_to_vector(lengths, [&](LineTree::length_t length) -> delay_t {
                    return delay_t {schematic.wire_delay_per_distance().value * length};
                });
            element.set_output_delays(delays);

            // history
            auto tree_max_delay = std::ranges::max(delays);
            element.set_history_length(delay_t {tree_max_delay.value});

            tree_length_sum += calculate_tree_length(line_tree);
            layout.set_line_tree(element, std::move(line_tree));
        }
    }

    // init simulation
    simulation.initialize();

    // calculate simulation time
    delay_t max_delay {0us};
    for (auto element : schematic.elements()) {
        for (auto output : element.outputs()) {
            max_delay = std::max(max_delay, output.delay());
        }
    }
    auto max_time {max_delay.value};

    // add events
    for (auto element : schematic.elements()) {
        if (element.element_type() == ElementType::wire) {
            auto spacing_dist_us =
                UDist<int> {config.min_event_spacing_us, config.max_event_spacing_us};
            bool next_value = true;
            auto next_time = spacing_dist_us(rng) * 1us;

            while (next_time < max_time) {
                simulation.submit_event(element.input(connection_id_t {0}), next_time,
                                        next_value);

                next_value = next_value ^ true;
                next_time = next_time + spacing_dist_us(rng) * 1us;
            }
        }
    }

    // run simulation
    simulation.run(max_time);

    return tree_length_sum;
}

auto benchmark_line_renderer(int n_lines, bool save_image) -> int64_t {
    BenchmarkScene scene;

    auto tree_length_sum = fill_line_scene(scene, n_lines);

    // render image
    BLImage img(1200, 1200, BL_FORMAT_PRGB32);
    BLContext ctx(img);
    render_background(ctx);
    {
        auto timer = Timer {"Render", Timer::Unit::ms, 3};
        render_circuit(ctx, render_args_t {
                                .layout = scene.layout,
                                .schematic = &scene.schematic,
                                .simulation = &scene.simulation,
                                .selection = {},
                            });
    }
    ctx.end();

    if (save_image) {
        BLImageCodec codec;
        codec.findByName("PNG");
        img.writeToFile("benchmark_line_renderer.png", codec);
    }

    return tree_length_sum;
}

}  // namespace logicsim
